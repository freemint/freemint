#include "xa_appl.h"

#if WITH_BBL_HELP
#include "menuwidg.h"
#include "xacookie.h"
#include "c_window.h"
#define BBL_MAXLEN	255

#include "xa_bubble.h"

/* if modal HIDE should come from client, but taskbar sends it too late so we handle mouse-move if ALWAYS_HIDE 1 */
#define ALWAYS_HIDE 1

static void set_bbl_vdi( struct xa_vdi_settings *v )
{
	v->api->f_color(v, G_WHITE);
	v->api->wr_mode(v, MD_REPLACE);
	v->api->t_color(v, G_BLACK);
	v->api->l_color(v, G_BLACK);
	v->api->t_effects(v, 0);
}

static void skip_trailing_blanks( unsigned char *str )
{
	unsigned char *p;
	for( p = str + strlen((char*)str)-1; *p == ' '; p-- )
	;
	*(p+1) = 0;
}

extern MFORM M_POINTSLIDE_MOUSE;
static short radius = 16, roff = 4;
/* fVDI has different rboxes */
static short xtxt_add, rox, wadd;

static short Style;
static BGEM bgem =
{
	C_BGEM,
	18,
	7,
	0,
	&M_POINTSLIDE_MOUSE,
	200
};
#define BBL_LLEN	37
#define BBL_MAXLLEN	42

/*
 * if no | in str break lines at BBL_LLEN, avoid too long lines > BBL_MAXLLEN
 * return #of lines
 */
static int format_string( unsigned char *str, int *maxl )
{
	int ret = 1, cnt, l, ml, hasnop = !strchr( (char*)str, '|' );
	unsigned char *lastbl = 0;
	for( cnt = ml = l = 0; /**str &&*/ cnt < BBL_MAXLEN; str++, l++, cnt++ )
	{
		if( !*str || *str == '|' || (hasnop && l > BBL_LLEN && *str <= ' ') )
		{
			if( hasnop )
			{
				if( l > BBL_MAXLLEN && lastbl && lastbl < str )
				{
					l -= (str - lastbl);// - 1);
					str = lastbl;// + 1;
					*lastbl = '|';
				}
				else if( *str )
				{
					*str = '|';
				}
			}
			else
			{
				if( *(str-1) == ' ' )	/* trailing blank */
				{
					l--;
				}
			}

			if( l > ml )
			{
				ml = l;
			}
			l = -1;
			if( !*str )
				break;
			ret++;
		}
		if( *str == ' ' )
			lastbl = str;
	}
	if( cnt == BBL_MAXLEN )
	{
		*str = 0;
	}
	if( l >= ml )
	{
		ml = l;
	}
	//if( C.fvdi_version && Style == 2 && fl_longest )
		//ml++;

	if( maxl )
	{
		*maxl = ml;
	}

	return ret;
}

static void set_bbl_rect_bbl( short np, RECT *r, RECT *ri, RECT *rw, short x, short y )
{
	r->x -= roff;
	r->y -= roff;
	r->h += roff * 2;

	ri->x = r->x + radius;
	ri->y = r->y + radius;
	ri->w = r->w - radius * 2;
	ri->h = r->h - radius * 2;

	rw->x = r->x - 10;	// + wadd;
	if( x > r->x )
		rw->w = r->w + (x - r->x) + 8 + wadd;
	else
		rw->w = r->w + (r->x - x) + 8 + wadd;
	if( y > r->y )
	{
		rw->y = r->y - 2;
		rw->h = (y - r->y) + 4;
	}
	else
	{
		rw->y = y;
		rw->h = r->h + (r->y - y) + 4;
	}

}

static short set_bbl_rect( short np, short x, short y, short maxl, RECT *r, RECT *ro )
{

	short rx = rox;
	if( Style == 2 && maxl < 6 )
	{
		rx += (7 - maxl) / 2 * screen.c_max_w;
		maxl = 6;
	}

	ro->h = r->h;

	r->x = x + 8;
	r->y = y + 16;
	r->w *= (maxl+1);

	r->h *= np;
	r->h += 4;

	if( Style == 3 )
	{
		r->y = 0;
		ro->x = r->x = 2;
		r->h = screen.c_max_h + 2;
	}
	else if( Style == 1 )
	{
		if( r->y + r->h >= screen.r.h - get_menu_height() )	//22 )
		{
			r->y = y - r->h - get_menu_height();//	16;
			r->x = x + 4;
		}
	}

	if( r->x + r->w > screen.r.w )
		r->x = screen.r.w - r->w - 10;
	if( Style != 3 )
	{
		if( r->y < 4 )
			r->y = 4;
		if( r->x < rx )
			r->x = rx;
		ro->x = r->x + rx;
	}
	ro->y = r->y;
	ro->w = 0;
	return maxl;
}

static void draw_bbl_window( struct xa_vdi_settings *v, RECT *r, RECT *ri, short x, short y )
{
	switch( Style )
	{
	case 1:	// tooltip
	case 3:	// app
		v->api->gbar( v, 0, r );
		v->api->box( v, 0, r->x, r->y, r->w, r->h );
	break;
	case 2:	// balloon
	{
		short fxy[8], yd;
		short m = 0, n = 0, xd = 20;

		/* filled rect */
		fxy[0] = r->x;
		fxy[1] = r->y;	//-1;
		fxy[2] = r->x + r->w + wadd;
		fxy[3] = r->y + r->h;
		//v->api->f_color(v, G_BLUE);
		v_rfbox( v->handle, fxy );
		//v->api->f_color(v, G_WHITE);

		/* border rect */
		v_rbox( v->handle, fxy );


		/* arrow */
		if( x >= r->x + (r->w/2))
		{
			n = 1;	// left
		}
		if( y > r->y + r->h )	// up
		{
			m = 1;	// up
			yd = y - (r->y+r->h) + xd;
		}
		else		// down
			yd = r->y - y;

		fxy[0] = x;
		fxy[1] = y;
		if( n )
			fxy[2] = x - yd - 2;
		else
			fxy[2] = x + yd - 10;

		if( m )
			fxy[3] = r->y + r->h - 1;
		else
			fxy[3] = r->y + 1;

		fxy[4] = fxy[2];
		fxy[2] += xd;
		fxy[5] = fxy[3];
		fxy[6] = x;
		fxy[7] = y;

		hidem();

		//v->api->f_color(v, G_CYAN);
		v_fillarea( v->handle, 3, fxy );
		//v->api->f_color(v, G_BLUE);
		if( m )
			fxy[3]++;
		else
			fxy[3]--;
		fxy[5] = fxy[3];
		v_pline(v->handle, 2, fxy);
		//v->api->f_color(v, G_GREEN);
		v_pline(v->handle, 2, fxy+4);

		showm();
	}
	break;
	}
}

static void bbl_text( struct xa_vdi_settings *v, RECT *ro, unsigned char *str, short np )
{
	short yt = ro->y, rx = ro->x, h = ro->h, to;
	if( Style == 2 )
		rx += xtxt_add;

	if( Style == 3 )
		to = 1;
	else
		to = 2;
	v->api->t_font( v, C.Aes->options.standard_font_point, cfg.font_id);
	for( ; np; np--, yt += h )
	{
		unsigned char *s = (unsigned char *)strchr( (char*)str, '|' );
		if( s )
			*s = 0;
		skip_trailing_blanks(str);
		v_gtext(v->handle, rx, yt + to, (char*)str);
		if( s )
			str = s + 1;
		else
			break;
	}
}

static int open_bbl_window( enum locks lock, unsigned char *str, short x, short y )
{
	int maxl;
	int np = format_string( str, &maxl );
	RECT r, ro, rw;
	struct xa_vdi_settings *v = C.Aes->vdi_settings;
	short y2 = y;

	RECT ri;

	v->api->t_font( v, C.Aes->options.standard_font_point, cfg.font_id);
	r.w = v->cell_w;
	r.h = v->cell_h;

	maxl = set_bbl_rect( np, x, y, maxl, &r, &ro );
	if( Style == 2 )
	{
		if( r.y > y )	// prefer up
		{
			r.y -= ( r.h + 42 );
			ro.y -= ( r.h + 42 );
		}
		if( r.y < get_menu_height() )	// too high
		{
			y2 += 2;	// avoid window covering mouse-point
			r.y = ro.y = y + 42;
		}
		else
		{
			y2 -= 2;
		}
		rw.y = y2;
		set_bbl_rect_bbl( np, &r, &ri, &rw, x, y2);
		// control-box
		//v->api->box( v, 0, rw.x, rw.y, rw.w, rw.h );
	}
	else
		rw = r;

	if( !open_window(lock, bgem_window, rw) )
	{
		return 1;
	}
	bgem_window->window_status |= XAWS_FLOAT;
	set_bbl_vdi( v );
	//v->api->set_clip( v, &rw);
	draw_bbl_window( v, &r, &ri, x, y2 );
	bbl_text( v, &ro, str, np );

	return 0;
}
#if 0
static void xstrncpy( unsigned char *dst, unsigned char *src, long len )
{
	while( *src && len-- )
	{
		*dst++ = *src++;
	}
	if( len )
		*dst = 0;
}
#endif
BBL_STATUS xa_bubble( enum locks lock, BBL_MD md, union msg_buf *msg, short destID )
{
	static BGEM *c_bgem = 0;
	static int XaBubble = bs_none, OldState = bs_none, XaModal = 0, in_xa_bubble = -1;

	static union msg_buf m;
	static short intin[8], intout[8], control[8], global[8];
	static long addrin[4] = {(long)&m}, addrout[4];
	static AESPB pb = {control, global, intin, intout, addrin, addrout};
	int ret;

	if( md == bbl_get_status
		|| ( (md == bbl_close_bubble1 || md == bbl_close_bubble2)
			&& ( XaBubble == bs_closed || (!ALWAYS_HIDE && XaModal == 1)) ) )
	{
		/* destID=1 means if in_xa_bubble never close */
		if( destID == 1 && in_xa_bubble != -1 )
			return bs_none;
		if( Style == 3 )
			return bs_closed;
		return XaBubble;
	}

	if( in_xa_bubble >= 0 )
	{
		if( md == bbl_send_request || md == bbl_close_bubble1 || md == bbl_close_bubble2 )
			return bs_none;
	}
	in_xa_bubble = get_curproc()->pid;
	ret = 0;
	if( md == bbl_send_request && Style != 3 && XaBubble == bs_open )
		goto xa_bubble_ret;	// bubble open: no request
	if( msg )
	{
		memcpy( &m, msg, sizeof( m ) );
		intin[1] = sizeof( union msg_buf );	//len
	}

	switch( md )
	{
	case bbl_tmp_inact:
		if( XaBubble != bbl_tmp_inact )
		{
			OldState = XaBubble;
			XaBubble = bbl_tmp_inact;
		}
		else
			XaBubble = OldState;
	goto xa_bubble_ret;
	case bbl_process_event:
		switch( m.m[0] )
		{
		case BUBBLEGEM_REQUEST:
		break;
		case BUBBLEGEM_HIDE:
			close_window( lock, bgem_window );
			c_bgem->active = 0;

			/* send ACK-message saved at SHOW */
			XA_appl_write( lock, C.Hlp, &pb );
			XaBubble = bs_closed;
		goto xa_bubble_ret;
		case BUBBLEGEM_SHOW:
			if( Style == 3 )
			{
				close_window( lock, bgem_window );
				XaBubble = bs_closed;
			}
			if( XaBubble == bs_closed )
			{
				unsigned char bbl_buf[BBL_MAXLEN+1], *bp = bbl_buf;
				unsigned char *str = m.sb.p56;
				if( !str || !*str )
				{
					ret = -2;
					goto xa_bubble_ret;
				}
				if( m.m[7] & BGS7_DISPCL )
				{
					Style = 3;
					for( ; *str == ' '; str++ );
					skip_trailing_blanks(str);
					*bp++ = ' ';
				}
				else
					Style = cfg.xa_bubble;

				strncpy( (char*)bp, (char*)str, BBL_MAXLEN );
				//xstrncpy( bp, str, BBL_MAXLEN );
				bbl_buf[BBL_MAXLEN] = 0;
				c_bgem->active = 1;
				if( open_bbl_window( lock,  bbl_buf, m.m[3], m.m[4]) )
				{
					BLOG((0,"xa_bubble: could not open window for %s", get_curproc()->name));
					ret = -1;
					goto xa_bubble_ret;
				}
				XaModal = (m.m[7] & BGS7_USRHIDE);
				XaBubble = bs_open;

				if( destID != C.AESpid /* && !XaModal*/ )
				{
					/* ACK-message comes after (unmodal) close */
					intin[0] = m.m[1];	//destID
					m.m[0] = BUBBLEGEM_ACK;
					m.m[1] = C.AESpid;
					m.m[2] = m.m[3] = m.m[4] = 0;
				}
				else
					intin[0] = 0;
			}
		break;
		case BUBBLEGEM_ACK:
		break;
		}
	break;
	case bbl_enable_bubble:
	{
		XaBubble = bs_closed;

		if( !install_cookie( (void**)&c_bgem, (void*)&bgem, sizeof(*c_bgem), C_BGEM, false ) )
		{
			XaBubble = bs_closed;
			/* fVDI has a different rbox */
			if( C.fvdi_version )
			{
				rox = 4;
				xtxt_add = 0;
				wadd = 6;
			}
			else
			{
				rox = 3;
				xtxt_add = -2;
				wadd = 0;
			}
		}
	}
	break;
	case bbl_disable_bubble:
	case bbl_disable_and_free:
		XaBubble = bs_inactive;
		if( c_bgem )
			delete_cookie( (void**)&c_bgem, C_BGEM, md == bbl_disable_and_free );
	goto xa_bubble_ret;
	case bbl_send_request:
		intin[0] = destID;
		XA_appl_write( 0, C.Aes, &pb );
	goto xa_bubble_ret;
	case bbl_close_bubble1:
	case bbl_close_bubble2:
		if( msg )
		{
			ALERT(( "xa_bubble:msg=%lx,destID=%d (md=%d)", msg, destID, md));
			ret = -3;
			goto xa_bubble_ret;
		}
		if( XaBubble == bs_open && !(S.open_nlwindows.top->dial & created_for_ALERT) /*&& XaModal == 0*/ )
		{
			close_window( lock, bgem_window );
			c_bgem->active = 0;
			Style = cfg.xa_bubble;

			if( intin[0] )
			{
				/* send ACK-message saved at SHOW */
				XA_appl_write( lock, C.Hlp, &pb );
			}
			XaBubble = bs_closed;
			goto xa_bubble_ret;
		}
	break;
	default:
		BLOG((0,"xa_bubble: unhandled code: %d", md));
	}
xa_bubble_ret:
	in_xa_bubble = -1;
	return ret;
}

void display_launched( enum locks lock, char *str )
{
	//return;
	union msg_buf m = {{0}};
	if( xa_bubble( 0, bbl_get_status, 0, 1 ) == bs_open )
	{
		xa_bubble( 0, bbl_close_bubble1, 0, 0 );
	}
	m.m[0] = BUBBLEGEM_SHOW;
	m.m[1] = C.AESpid;
	m.m[7] = BGS7_DISPCL;
	///*m.m[3] =*/ m.m[4] = -16;
	m.sb.p56 = str;

	xa_bubble( lock, bbl_process_event, &m, C.AESpid );
}

void
XA_bubble_event(enum locks lock, struct c_event *ce, bool cancel)
{
	switch( ce->d0 )
	{
	case 0:
		xa_bubble( 0, bbl_close_bubble1, 0, 5 );
	break;
	case 1:
		xa_bubble( 0, bbl_enable_bubble, 0, 0 );
	break;
	}
}

#endif

