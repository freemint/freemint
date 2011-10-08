#include "xa_appl.h"
#include "xacookie.h"
#include "c_window.h"
#include "xa_bubble.h"

#if WITH_BBL_HELP
#define BBL_MAXLEN	255

/* if modal HIDE should come from client, but taskbar sends it too late so we handle mouse-move if ALWAYS_HIDE 1 */
#define ALWAYS_HIDE 1

/*
 * if no | in str break lines at BBL_LLEN, avoid too long lines > BBL_MAXLLEN
 */
static int count_pipes( unsigned char *str, int *maxl )
{
	int ret = 0, cnt, l, ml, hasnop = !strchr( (char*)str, '|' );
	unsigned char *lastbl = 0;
	for( cnt = ml = l = 0; *str && cnt < BBL_MAXLEN; str++, l++, cnt++ )
	{
		if( *str == '|' || (hasnop && l > BBL_LLEN && *str <= ' ') )
		{
			ret++;
			if( hasnop )
			{
				if( l > BBL_MAXLLEN && lastbl && lastbl < str )
				{
					l -= (str - lastbl);// - 1);
					str = lastbl;// + 1;
					*lastbl = '|';
				}
				else
				{
					*str = '|';
				}
			}
			else
				if( *(str-1) == ' ')	/* trailing blank */
				{
					l--;
				}

			if( l > ml )
				ml = l;
			l = -1;
		}
		if( *str == ' ' )
			lastbl = str;
	}
	if( cnt == BBL_MAXLEN )
		*str = 0;
	if( l > ml )
		ml = l;
	if( maxl )
	{
		*maxl = ml;
	}
	return ret;
}
static int open_bbl_window( enum locks lock, unsigned char *str, short x, short y )
{
	int maxl;
	int np = count_pipes( str, &maxl ) + 1;
	short th;
	RECT r;
	struct xa_vdi_settings *v = C.Aes->vdi_settings;

	r.x = x + 16;
	r.y = y + 16;
	v->api->t_font( v, C.Aes->options.standard_font_point, cfg.font_id);
	v->api->t_extent( v, "X", &r.w, &r.h );
	r.w *= maxl;
	th = r.h;
	r.w += 12;
	r.h *= np;
	r.h += 10;
	if( r.x + r.w > screen.r.w )
		r.x = screen.r.w - r.w - 4;
	if( r.y + r.h > screen.r.h - 32 )
		r.y = screen.r.h - r.h - ( 4 + 32);
	if( r.y < 4 )
		r.y = 4;
	if( r.x < 4 )
		r.x = 4;

	if( !open_window(lock, bgem_window, r) )
	{
		BLOG((0,"open_bbl_window: could not open window"));
		return 1;
	}

	v->api->wr_mode(v, MD_REPLACE);
	v->api->f_color(v, G_WHITE);
	v->api->t_color(v, G_BLACK);
	v->api->gbar( v, 0, &r );
	for( ; np; np--, r.y += th )
	{
		unsigned char *s = (unsigned char *)strchr( (char*)str, '|' );
		if( s )
			*s = 0;
		v_gtext(v->handle, r.x + 4, r.y + 4, (char*)str);
		if( s )
			str = s + 1;
		else
			break;
	}

	return 0;
}

BBL_STATUS xa_bubble( enum locks lock, BBL_MD md, union msg_buf *msg, short destID )
{
	static BGEM *c_bgem = 0;
	static int XaBubble = bs_none, XaModal = 0, in_xa_bubble = -1;

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
		return XaBubble;
	}

	if( in_xa_bubble >= 0 )
	{
		if( md == bbl_send_request || md == bbl_close_bubble1 || md == bbl_close_bubble2 )
			return bs_none;
	}
	in_xa_bubble = get_curproc()->pid;
	ret = 0;
	if( md == bbl_send_request && XaBubble == bs_open )	//|| get_curproc()->pid == 0 )
		goto xa_bubble_ret;	// bubble open: no request
	if( msg )
	{
		memcpy( &m, msg, sizeof( m ) );
		intin[1] = sizeof( union msg_buf );	//len
	}

	switch( md )
	{
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
			if( XaBubble == bs_closed )
			{
				unsigned char bbl_buf[BBL_MAXLEN+1];
				unsigned long l5 = (unsigned long)m.m[5] << 16L, l6 = (l5 | ((unsigned long)m.m[6] & 0xffffL));
				char *str = (char*)l6;
				if( !str || !*str )
				{
					ret = -2;
					goto xa_bubble_ret;
				}
				strncpy( (char*)bbl_buf, str, sizeof(bbl_buf)-1);
				c_bgem->active = 1;
				if( open_bbl_window( lock,  bbl_buf, m.m[3], m.m[4]) )
				{
					BLOG((0,"xa_bubble: could not open window for %s", get_curproc()->name));
					ret = -1;
					goto xa_bubble_ret;
				}
				XaModal = (m.m[7] & BGS7_USRHIDE);
				XaBubble = bs_open;

				//if( !XaModal )
				{
					/* ACK-message comes after (unmodal) close */
					intin[0] = m.m[1];	//destID
					m.m[0] = BUBBLEGEM_ACK;
					m.m[1] = C.AESpid;
					m.m[2] = m.m[3] = m.m[4] = 0;
				}
			}
		break;
		case BUBBLEGEM_ACK:
		break;
		}
	break;
	case bbl_enable_bubble:
	{
		//static MFORM bms;
		extern  MFORM M_POINTSLIDE_MOUSE;
		BGEM bgem =
		{
			C_BGEM,
			18,
			7,
			0,
			&M_POINTSLIDE_MOUSE,
			200
		};
		if( !install_cookie( (void**)&c_bgem, (void*)&bgem, sizeof(*c_bgem), C_BGEM ) )
			XaBubble = bs_closed;
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
		if( msg || destID )
		{
			ALERT(( "xa_bubble:msg=%lx,destID=%d (md=%d)", msg, destID, md));
			ret = -3;
			goto xa_bubble_ret;
		}
		if( XaBubble == bs_open /*&& XaModal == 0*/ )
		{
			close_window( lock, bgem_window );
			c_bgem->active = 0;

			/* send ACK-message saved at SHOW */
			XA_appl_write( lock, C.Hlp, &pb );
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

void
XA_bubble_event(enum locks lock, struct c_event *ce, bool cancel)
{
	xa_bubble( 0, bbl_close_bubble1, 0, 0 );
}

#endif

