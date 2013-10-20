#include "xa_appl.h"

#if WITH_BBL_HELP
#include "k_main.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "xacookie.h"
#include "c_window.h"
#define BBL_MAXLEN	511
#define BBL_LLEN	37
#define BBL_MAXLLEN	42
#define BBL_MINLEN	7

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

static short skip_trailing_blanks( unsigned char *str )
{
	unsigned char *p;
	short ret = strlen((char*)str);
	for( p = str + ret-1; *p == ' '; p-- )
		ret--;
	*(p+1) = 0;
	return ret;
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

/*
 * if no | in str break lines at BBL_LLEN, avoid too long lines > BBL_MAXLLEN
 * return #of lines
 *
 * todo: skip trailing blanks on every line (?)
 */
static int format_string( unsigned char *str, int *maxl )
{
	int ret = 1, cnt, l, ml, hasnop = !strchr( (char*)str, '|' );
	unsigned char *lastbl = 0;
	unsigned char *start = str;
	for( cnt = ml = l = 0; cnt < BBL_MAXLEN; str++, l++, cnt++ )
	{
		if( !*str || *str == '|' || (hasnop && l > BBL_LLEN && *str <= ' ') )
		{
			if( hasnop )
			{
				if( l > BBL_MAXLLEN && lastbl && lastbl < str )
				{
					l -= (str - lastbl);
					str = lastbl;
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
	if( ret == 1 )
		ml = skip_trailing_blanks( start );

	if( cnt == BBL_MAXLEN )
	{
		*str = 0;
	}
	if( l > ml )
	{
		ml = l;
	}

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

	rw->x = r->x;// - 10;	// + wadd;
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
	if( Style == 2 && maxl < BBL_MINLEN )
	{
		rx += (BBL_MINLEN + 1 - maxl) / 2 * screen.c_max_w;
		maxl = BBL_MINLEN;
	}

	ro->h = r->h;

	r->x = x + 8;
	r->y = y + 2;
	if( Style == 1 )
		r->y += 16;
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
		if( r->y + r->h >= screen.r.h - get_menu_height() )
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
		short fxy[8], pxy[4], yd;
		short m = 0, n = 0, xd = 20;

		pxy[0] = r->x + 1;
		pxy[1] = r->y;
		pxy[2] = r->x + r->w + wadd;
		pxy[3] = r->y + r->h;
		memcpy( fxy, pxy, sizeof(pxy) );

		/* border rect */
		fxy[0]--;
		fxy[2]++;
		fxy[1]--;
		fxy[3]++;
		v_rbox( v->handle, fxy );

		/* arrow -> down-left!*/
		if( x >= r->x + (r->w/2))
		{
			n = 1;	// left
		}
		if( y > r->y + r->h )	// up
		{
			m = 1;	// up
			yd = y - (r->y+r->h);
		}
		else		// down
		{
			if( n == 0 )
			{
				y -= 4;
			}
			x -= 4;
			y -= 4;
			yd = r->y - y;
		}

		x += 4;
		fxy[0] = x;
		fxy[1] = y;
		if( n )
			fxy[2] = x - yd;
		else
			fxy[2] = x + yd;

		if( m )
			fxy[3] = r->y + r->h;
		else
			fxy[3] = r->y + 1;

		if( fxy[2] < r->x + 10 )
			fxy[2] = r->x + 10;
		if( fxy[2] + xd > r->x + r->w - 6 )
			fxy[2] = r->x + r->w - 6 - xd;
		fxy[4] = fxy[2];
		fxy[2] += xd;
		fxy[5] = fxy[3];

		fxy[6] = x;
		fxy[7] = y;

		hidem();

		v_fillarea( v->handle, 3, fxy );
		if( !m )
			fxy[3]--;
		v_pline(v->handle, 2, fxy);
		v_pline(v->handle, 2, fxy+4);

		/* filled round rect */
		v_rfbox( v->handle, pxy );
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
			r.y = y - ( r.h + 32 );
			ro.y = r.y;
		}
		if( r.y < get_menu_height() )	// too high
		{
			y2 += 2;	// avoid window covering mouse-point
			r.y = ro.y = y2 + 32;
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
	v->api->set_clip( v, &rw);
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

struct bbl_arg {
	char *str;
};

static struct bbl_arg bbl_arg={0};

BBL_STATUS xa_bubble( enum locks lock, BBL_MD md, union msg_buf *msg, short destID )
{
	static BGEM *c_bgem = 0;
	static int XaBubble = bs_none, OldState = bs_none, XaModal = 0, in_xa_bubble = -1;

	static union msg_buf m;
	static short intin[8], intout[8], control[8], global[8];
	static long addrin[4] = {(long)&m}, addrout[4];
	static AESPB pb = {control, global, intin, intout, addrin, addrout};
	int ret = XaBubble;

	if( md == bbl_close_bubble2 )
		bbl_arg.str = 0;	// cancel pending SHOW

	if( md == bbl_get_status
		|| (( md == bbl_close_bubble1 || md == bbl_close_bubble2)
			&& ( XaBubble == bs_closed || (!ALWAYS_HIDE && XaModal == 1)) ) )
	{
		/* destID=1 means if in_xa_bubble never close */
		//if( destID == 1 && in_xa_bubble != -1 )
			//return bs_none;
		//if( Style == 3 )
			//return bs_closed;
		return XaBubble;
	}

	if( in_xa_bubble >= 0 )
	{
		if( md == bbl_send_request || md == bbl_close_bubble1 || md == bbl_close_bubble2 )
		{
			return bs_none;
		}
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
		case BUBBLEGEM_HIDE:
			close_window( lock, bgem_window );
			c_bgem->active = 0;

			/* send ACK-message saved at SHOW */
			XA_appl_write( lock, C.Hlp, &pb );
			XaBubble = bs_closed;
		goto xa_bubble_ret;
		case BUBBLEGEM_REQUEST:
		break;
		case BUBBLEGEM_SHOW:
			if( !bgem_window )
				goto xa_bubble_ret;	/* ERROR! */
			if( Style == 3 )
			{
				close_window( lock, bgem_window );
				XaBubble = bs_closed;
			}
			if( XaBubble == bs_closed || XaBubble == bs_none )
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
				else if( m.m[2] != 0 )
						Style = m.m[2];
				else
					Style = cfg.xa_bubble;

				strncpy( (char*)bp, (char*)str, BBL_MAXLEN );

				bbl_buf[BBL_MAXLEN] = 0;
				c_bgem->active = 1;
				if( open_bbl_window( lock,  bbl_buf, m.m[3], m.m[4]) )
				{
					BLOG((1, "xa_bubble: could not open window for %s", get_curproc()->name));
					ret = -1;
					goto xa_bubble_ret;
				}
				XaModal = (m.m[7] & (BGS7_USRHIDE | BGS7_USRHIDE2));
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
		if( XaModal & 2 )
			goto xa_bubble_ret;
	case bbl_close_bubble2:
		if( msg )
		{
			ALERT(( "xa_bubble:msg=%lx,destID=%d (md=%d)", msg, destID, md));
			ret = -3;
			goto xa_bubble_ret;
		}
		if( XaBubble == bs_open && (!S.open_nlwindows.top || !(S.open_nlwindows.top->dial & created_for_ALERT)))
		{
			close_window( lock, bgem_window );
			c_bgem->active = 0;
			Style = cfg.xa_bubble;

			if( intin[0] )
			{
				/* send ACK-message saved at SHOW */
				XA_appl_write( lock, C.Hlp, &pb );
			}
			ret = XaBubble = bs_closed;
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

/*
 * to be called by post_cevent
 */
void
XA_bubble_event(enum locks lock, struct c_event *ce, short cancel)
{
	switch( ce->d0 )
	{
	case BBL_EVNT_CLOSE1:
		xa_bubble( 0, bbl_close_bubble1, 0, 5 );
	break;
	case BBL_EVNT_ENABLE:
		xa_bubble( 0, bbl_enable_bubble, 0, 0 );
	break;
	}
}

static void do_bubble_show(enum locks lock, struct c_event *ce, short cancel)
//static void do_bubble_show(void)
{
	short x, y, b;
	if( bbl_arg.str == 0 )
		return;

	{
	union msg_buf m;
	BBL_STATUS status = xa_bubble( 0, bbl_get_status, 0, 0 );
	if( status == bs_open )
	{
		status = xa_bubble( 0, bbl_close_bubble2, 0, 20 );
	}
	check_mouse( C.Aes, &b, &x, &y );
	if( !(status == bs_closed || status == bs_none) || b )
	{
		bbl_arg.str = 0;
		return;
	}
	m.m[0] = BUBBLEGEM_SHOW;
	m.m[1] = C.AESpid;
	m.m[2] = cfg.describe_widgets;
	m.m[3] = x;
	m.m[4] = y + 4;
	m.sb.p56 = bbl_arg.str;
	m.m[7] = BGS7_USRHIDE2;

	xa_bubble( 0, bbl_process_event, &m, C.AESpid );
	}
}

void bubble_request( short pid, short whndl, short x, short y )
{
	if( bbl_arg.str != 0 )
	{
		static short cnt = 0;
		static char *str = 0;
		if( cnt == 1 )
		{
			if( bbl_arg.str == str )
			{
				post_cevent(C.Aes, do_bubble_show, NULL, NULL, 0, 0, NULL, NULL);
				//do_bubble_show();
			}
			cnt = 0;
		}
		else
		{
			str = bbl_arg.str;
			cnt++;
		}
		return;
	}

	if( pid != C.AESpid && pid != C.Hlp->p->pid )
	{
		union msg_buf m;
		m.m[0] = BUBBLEGEM_REQUEST;
		m.m[1] = pid;
		m.m[2] = 0;
		m.m[3] = whndl;
		m.m[4] = x;
		m.m[5] = y;
		m.m[6] = 0;	//kbshift
		m.m[7] = 0;
		xa_bubble( 0, bbl_send_request, &m, pid );
	}
}

void bubble_show( char *str )
{
	if( bbl_arg.str && str == NULL )
		xa_bubble( 0, bbl_close_bubble2, 0, 22 );
	bbl_arg.str = str;
}

void display_launched( enum locks lock, char *str )
{
	union msg_buf m = {{0}};
	if( xa_bubble( 0, bbl_get_status, 0, 21 ) == bs_open )
	{
		xa_bubble( 0, bbl_close_bubble1, 0, 0 );
	}
	m.m[0] = BUBBLEGEM_SHOW;
	m.m[1] = C.AESpid;
	m.m[7] = BGS7_DISPCL;
	m.sb.p56 = str;

	xa_bubble( lock, bbl_process_event, &m, C.AESpid );
}

#endif

