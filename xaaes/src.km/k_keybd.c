/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "k_keybd.h"
#include "c_keybd.h"
#include "xa_global.h"

#include "xaaes.h"

#include "about.h"
#include "app_man.h"
#include "xa_appl.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_main.h"
#include "k_mouse.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "util.h"
#include "widgets.h"
#include "menuwidg.h"

#include "obtree.h"

#include "xa_form.h"
#include "xa_rsrc.h"
#include "xa_shel.h"
#include "trnfm.h"
#include "render_obj.h"
#include "keycodes.h"
#if WITH_BBL_HELP
#include "xa_bubble.h"
#endif

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/ioctl.h"
#include "mint/signal.h"
#include "mint/ssystem.h"

/* Keystrokes must be put in a queue if no apps are waiting yet.
 * There also may be an error in the timer event code.
 */

/*
 * Keyboard input handler
 * ======================
 *
 * Courtesy Harald Siegmund:
 *
 * nkc_tconv: TOS key code converter
 *
 * This is the most important function within NKCC: it takes a key code
 * returned by TOS and converts it to the sophisticated normalized format.
 *
 * Note: the raw converter does no deadkey handling, ASCII input or
 *       Control key emulation.
 *
 * In:   D0.L           key code in TOS format:
 *                                  0                    1
 *                      bit 31:     ignored              ignored
 *                      bit 30:     ignored              ignored
 *                      bit 29:     ignored              ignored
 *                      bit 28:     no CapsLock          CapsLock
 *                      bit 27:     no Alternate         Alternate pressed
 *                      bit 26:     no Control           Control pressed
 *                      bit 25:     no left Shift key    left Shift pressed
 *                      bit 24:     no right Shift key   right Shift pressed
 *
 *                      bits 23...16: scan code
 *                      bits 15...08: ignored
 *                      bits 07...00: ASCII code (or rubbish in most cases
 *                         when Control or Alternate is pressed ...)
 *
 * Out:  D0.W           normalized key code:
 *                      bits 15...08: flags:
 *                                  0                    1
 *                      NKF?_FUNC   printable char       "function key"
 *                      NKF?_RESVD  ignore it            ignore it
 *                      NKF?_NUM    main keypad          numeric keypad
 *                      NKF?_CAPS   no CapsLock          CapsLock
 *                      NKF?_ALT    no Alternate         Alternate pressed
 *                      NKF?_CTRL   no Control           Control pressed
 *                      NKF?_LSH    no left Shift key    left Shift pressed
 *                      NKF?_RSH    no right Shift key   right Shift pressed
 *
 *                      bits 07...00: key code
 *                      function (NKF?_FUNC set):
 *                         < 32: special key (NK_...)
 *                         >=32: printable char + Control and/or Alternate
 *                      no function (NKF?_FUNC not set):
 *                         printable character (0...255!!!)
 */
void
cancel_keyqueue(struct xa_client *client)
{
	while (client->kq_tail)
	{
		struct keyqueue *kq = client->kq_tail;
		client->kq_tail = kq->next;
		kfree(kq);
	}
	client->kq_tail = client->kq_head = NULL;
	client->kq_count = 0;
}


void
queue_key(struct xa_client *client, const struct rawkey *key)
{
	struct keyqueue *kq;
	DIAGS(("queue key for %s", client->proc_name));
	if (client->kq_count < MAX_KEYQUEUE)
	{
		kq = kmalloc(sizeof(*kq));
		if (kq)
		{
			kq->next = NULL;
			kq->key = *key;

			if (client->kq_head)
			{
				client->kq_head->next = kq;
				client->kq_head = kq;
			}
			else
				client->kq_head = client->kq_tail = kq;
			client->kq_count++;
		}
	}
}

bool
unqueue_key(struct xa_client *client, struct rawkey *key)
{
	bool ret = false;
	struct keyqueue *kq;

	if ((kq = client->kq_tail))
	{
		*key = kq->key;

		DIAGS(("unqueue key for %s", client->proc_name));

		if (client->kq_tail == client->kq_head)
			client->kq_tail = client->kq_head = NULL;
		else
			client->kq_tail = kq->next;

		client->kq_count--;
		kfree(kq);
		ret = true;
	}
	return ret;
}

static void
XA_keyboard_event(int lock, const struct rawkey *key)
{
	struct xa_window *keywind;
	struct xa_client *locked_client;
	struct xa_client *client;
	struct rawkey *rk;
	bool waiting, chlp_lock = update_locked() == C.Hlp->p;

	if (!chlp_lock && TAB_LIST_START)
	{
		rk = kmalloc(sizeof(*rk));
		if (rk)
		{
			*rk = *key;
			post_cevent(TAB_LIST_START->client, cXA_menu_key, rk, NULL, 0,0, NULL,NULL);
		}
		return;
	}

	if (!(client = find_focus(true, &waiting, &locked_client, &keywind)))
	{
		return;
	}

	DIAG((D_keybd,client,"XA_keyboard_event: %s; update_lock:%d, focus: %s, window_list: %s",
		waiting ? "waiting" : "", update_locked() ? update_locked()->pid : 0,
		c_owner(client), keywind ? w_owner(keywind) : "no keywind"));


	/* Found either (MU_KEYBD|MU_NORM_KEYBD) or keypress handler. */
	if (waiting)
	{
		rk = kmalloc(sizeof(*rk));
		if (rk)
		{
			*rk = *key;
			rk->norm = nkc_tconv(key->raw.bcon);

			if (keywind && keywind == client->alert)
			{
				post_cevent(client, cXA_keypress, rk, keywind, 0,0, NULL,NULL);
			}
			else if (client->fmd.keypress)
			{
				post_cevent(client, cXA_fmdkey, rk, NULL, 0,0, NULL, NULL);
			}
			else if (keywind && keywind == client->fmd.wind)
			{
				post_cevent(client, cXA_keypress, rk, keywind, 0,0, NULL,NULL);
			}
			else
			{
				if (keywind && is_hidden(keywind))
					unhide_window(lock, keywind, true);
				if (keywind && keywind->keypress)
				{
					post_cevent(client, cXA_keypress, rk, keywind, 0,0, NULL,NULL);
				}
				else if (client->waiting_pb)
				{
					post_cevent(client, cXA_keybd_event, rk, NULL, 0,0, NULL,NULL);
				} else
				{
					DIAGS(("XA_keyboard_event: INTERNAL ERROR: No waiting pb."));
				}
			}
		}
	}
	else
	{
		/*!!!TEST!!!*/
		queue_key(client, key);
		/*
		 * We dont queue the key when we are sure the client dont want it
		 */
		/*if ( !client->waiting_pb )
			queue_key(client, key);
		else
			cancel_keyqueue(client);
			*/
	}
}

int switch_keyboard( const char *tbname )
{
			long out;
			unsigned long dummy;
			char tblpath[PATH_MAX];

			if( tbname )
			{
				sprintf( tblpath, sizeof(tblpath), "%s%s.tbl", sysdir, tbname );
			}

			/* 0 should restore the power-up setting */
			out = s_system(S_LOADKBD, tbname ? (unsigned long)tblpath : 0L, (unsigned long)&dummy);
			if( out )
			{
				ALERT((xa_strings(AL_KBD), tbname, sysdir, out));
				return 1;
			}
			return 0;
}

#if TST_BE
static void bus_error(void)
{
	long *sp = (long*)get_sp();
	BLOG((1,"%s:BUS-ERROR at bus_error:%lx,sp=%lx:%lx", get_curproc()->name, bus_error, sp, *sp));
	KDBG(("BUS-ERROR at bus_error:%lx", bus_error));
	*(long*)-3L = 44;
	KDBG(("BUS-ERROR OK"));
	BLOG((1,"BUS-ERROR at bus_error OK"));
}
#endif

static void close_menu_if_move(struct xa_window *wind)
{
	XA_WIDGET *widg = get_widget( wind, XAW_MENU );
	if( widg && widg->stuff )
		close_window_menu(TAB_LIST_START);
}

static void
do_kbd_win( int lock, struct xa_window *wind, GRECT *r, int md)
{
	if (wind->opts & XAWO_WCOWORK)
		*r = f2w(&wind->delta, r, true);
	if( md == WM_SIZED )
	{
		if( (wind->opts & XAWO_SENDREPOS) )
			send_reposed(lock, wind, AMQ_NORM, r);
		else
			send_sized(lock, wind, AMQ_NORM, r);
	}
	else
		send_moved(lock, wind, AMQ_NORM, r);

	/* resize wdialogs (todo) */
	/*if( (wind->do_message == NULL || (wind->dial & created_for_WDIAL)) )
	{
		short msg[8] = {WM_SIZED, 0, 0, 0, r.x, r.y, r.w, r.h};
		do_formwind_msg(wind, client, 0, 0, msg);
	}*/
}

/******************************************************************************
 from "unofficial XaAES":

+----------------------------------------------------------------------------------------------------------------------------+
|Key combo             |Action                                                                                               |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+CLRHOME      |Redraw screen                                                                                        |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+A            |Terminates all applications (A list of exceptions can be specified)                                  |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+D            |Open the screenshot dialog (XAAESNAP.PRG is required. See 6.6.2)                                     |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+H            |Halt the system                                                                                      |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+L            |Open task manager                                                                                    |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+P            |Restore palette in colour depth of 8-bits or less                                                    |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+Q            |Quit XaAES                                                                                           |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+R            |Change resolution                                                                                    |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+V            |Unhide all applications                                                                              |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+W            |Global window cycling                                                                                |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+X            |Hide all except the currently focused application                                                    |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+Y            |Hide currently focused application                                                                   |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+SPACE        |Open main menu bar                                                                                   |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+SHIFT+SPACE  |Open menu in current window if it has one, else open main menu bar                                   |
|----------------------+-----------------------------------------------------------------------------------------------------|
|CTRL+ALT+TAB          |Cycle open applications                                                                              |
+----------------------------------------------------------------------------------------------------------------------------+
********************************************************************************/
static bool
kernel_key(int lock, struct rawkey *key)
{
#if ALT_CTRL_APP_OPS
	if ((key->raw.conin.state & (K_CTRL|K_ALT)) == (K_CTRL|K_ALT))
	{
		struct xa_client *client;
		struct kernkey_entry *kkey = C.kernkeys;
		short nk, n;
		short sdmd = 0;

		key->norm = nkc_tconv(key->raw.bcon);
		nk = xa_toupper(key->norm & 0x00ff);

		DIAG((D_keybd, NULL,"CTRL+ALT+%04x --> %04x '%c'", key->aes, key->norm, nk ));

		while (kkey)
		{
			if (kkey->key == nk)
				break;
			kkey = kkey->next_key;
		}
		if (kkey)
		{
			if (kkey->act)
			{
				post_cevent(C.Hlp, ceExecfunc, kkey->act, NULL, 1,1, NULL, NULL);
			}
			return true;
		}
#if 0
		/* CTRL|ALT|number key is emulate wheel. */
		if (   nk=='U' || nk=='N'
		    || nk=='H' || nk=='J')
		{
			short  wheel = 0;
			short  click = 0;
			struct moose_data md = mainmd;

			md.x = md.sx;
			md.y = md.sy;

			switch (nk)
			{
			case 'U':
				wheel = cfg.ver_wheel_id, click = -1;
				break;
			case 'N':
				wheel = cfg.ver_wheel_id, click = 1;
				break;
			case 'H':
				wheel = cfg.hor_wheel_id, click = -1;
				break;
			case 'J':
				wheel = cfg.hor_wheel_id, click = 1;
				break;
			}

			md.state = wheel;
			md.clicks = click;

			XA_wheel_event(lock, &md);
			return true;
		}
#endif

		if( (nk == cfg.keyboards.c && (key->raw.conin.state & (K_RSHIFT|K_LSHIFT))) || (tolower(nk) == cfg.keyboards.c) )
		{
			char *tbname;
			int r;

			for( ; ; )
			{
				cfg.keyboards.cur++;
				if( cfg.keyboards.cur == MAX_KEYBOARDS || !*cfg.keyboards.keyboard[cfg.keyboards.cur])
				{
					cfg.keyboards.cur = -1;
					tbname = "keyboard";
				}
				else
					tbname = cfg.keyboards.keyboard[cfg.keyboards.cur];
				if( !(r = switch_keyboard( tbname )) || cfg.keyboards.cur == -1)
				{
					if( cfg.keyboards.cur == -1)
					{
						if( r )
						{
							switch_keyboard(0);
						}
						tbname = xa_strings(SW_DEFAULT);
					}
					add_keybd_switch(tbname);
					break;
				}
			}

			return true;
		}
		n = DoCtrlAlt( Get, nk, key->raw.conin.state );
		if( n == HK_FREE )
			return false;
		if( n )
		{
			nk = n;
			if( nk > HK_SHIFT )
			{
				nk -= HK_SHIFT;
				key->raw.conin.state |= (K_RSHIFT|K_LSHIFT);
			}
			else
				key->raw.conin.state &= ~(K_RSHIFT|K_LSHIFT);
		}

#if WITH_BBL_HELP
		if( nk != 'D' )	/* allow screenshot */
		{
			xa_bubble( lock, bbl_close_bubble2, 0, 0 );
		}
#endif
		switch (nk)
		{
		case NK_TAB:				/* TAB, switch menu bars */
			if (key->raw.conin.state & (K_RSHIFT|K_LSHIFT))
			{
				client = APP_LIST_START;
				for( client = NEXT_APP(client); client && ((client->type & APP_SYSTEM) || client == C.Aes || client == C.Hlp); client = NEXT_APP(client) )
				{

				}
				if( !client )
					client = APP_LIST_START;
			}
			else
				client = next_app(lock, true, false);  /* including the AES for its menu bar. */
			app_or_acc_in_front( lock, client );
			return true;

		case '0':	/* toggle main-menubar */
			toggle_menu(lock, -1);
			return true;

		case '+':
			C.loglvl++;
			return true;

		case '-':
			if( --C.loglvl < 0 )
				C.loglvl = 0;
			return true;
		case ' ':
			{
				struct xa_window *wind;
				struct xa_widget *widg;
				short mb = cfg.menu_bar;
	
#if 0
				if (nk == NK_ESC && !TAB_LIST_START)
					goto otm;
#endif
				client = NULL;
				widg = NULL;
	
				if (key->raw.conin.state & (K_RSHIFT|K_LSHIFT))
				{
					client = find_focus(true, NULL, NULL, &wind);
					if (client)
					{
						if (wind)
						{
							widg = get_widget(wind, XAW_MENU);
							if (!wdg_is_act(widg))
								client = NULL;
						}
						else
							client = NULL;
					}
				}
				else
					toggle_menu(lock, 1);
	
				if (!client)
				{
					if (TAB_LIST_START)
					{
						client = TAB_LIST_START->client;
						widg = TAB_LIST_START->widg;
						wind = TAB_LIST_START->wind;
					}
					else
					{
						widg = get_menu_widg();
						client = menu_owner();
						wind = root_window;
					}
				}
				if (client)
					post_cevent(client, cXA_open_menubykbd, wind,widg, 0,0, NULL,NULL);
				if( !mb )
					cfg.menu_bar = -1;
			}
			return true;
#if GENERATE_DIAGS == 0
		case 'D':	/* screen-dump */
			post_cevent(C.Hlp, ceExecfunc, screen_dump, NULL, 1, 0, NULL,NULL);
			return true;
#endif
		case 'R':				/* attempt to recover a hung system */
			if ((key->raw.conin.state & (K_RSHIFT|K_LSHIFT)))
			{
				if (C.reschange )
					post_cevent(C.Hlp, ceExecfunc, C.reschange, NULL, 1,0, NULL, NULL);
			}
			else
			{
				recover();
			}
			return true;

		case 'L':				/* open the task manager */
#if GENERATE_DIAGS
			D.debug_level = 4;
			return true;
#else
			if( !C.update_lock )
			{
				if( key->raw.conin.state & (K_RSHIFT|K_LSHIFT) )
					post_cevent(C.Hlp, ceExecfunc, open_launcher, NULL, HL_LOAD_CFG, 0, NULL,NULL);
				else
					post_cevent(C.Hlp, ceExecfunc, open_taskmanager,NULL, 1, 0, NULL,NULL);
			}
			return true;
#endif

		case 'I':	/* (un-)iconify window */
			return 	iconify_action(lock, TOP_WINDOW, 0);

		case NK_INS:
			TOP_WINDOW->send_message(lock, TOP_WINDOW, NULL, AMQ_NORM, QMF_CHKDUP,
			   WM_FULLED, 0, 0, TOP_WINDOW->handle, 0, 0, 0, 0);
			return true;

#if TST_BE
		case NK_ENTER:
			if( key->raw.conin.state & (K_RSHIFT|K_LSHIFT) )
				bus_error();
			break;
#endif

#define WGROW	16
		case NK_UP:
		case NK_DOWN:
			if( TOP_WINDOW )
			{
				struct xa_window *wind = TOP_WINDOW;
				short g = WGROW, s = key->raw.conin.state & (K_RSHIFT|K_LSHIFT);
				GRECT r;

				if( !wind || wind == root_window || (wind->dial & created_for_POPUP) )
					return true;
				close_menu_if_move(wind);
				client = wind->owner;
				r = wind->r;
				if( nk == NK_DOWN )
					g = -g;
				g *= 2;
				r.g_y -= g;

				if( !s )
				{
					if( !wind->rect_list.start
						|| (wind->dial & (created_for_ALERT | created_for_FORM_DO | created_for_WDIAL))
						|| (wind->window_status & (XAWS_ICONIFIED | XAWS_HIDDEN)) )
						return true;
					r.g_x -= g;
					r.g_w += g * 2;
					r.g_h += g * 2;

					if( nk == NK_UP )
					{
						if( wind->r.g_w >= wind->max.g_w && wind->r.g_h >= wind->max.g_h )
							return true;
						if (r.g_x < 0)
							r.g_x = 0;
						if (r.g_y < 0)
							r.g_y = 0;

						if( r.g_w > wind->max.g_w )
							r.g_w = wind->max.g_w;

						if( r.g_h > wind->max.g_h )
							r.g_h = wind->max.g_h;

						if( r.g_y < 0 && r.g_x < 0 )
							return true;
					}
					else	/* DOWN */
					{
						if( wind->r.g_w <= wind->min.g_w && wind->r.g_h <= wind->min.g_h )
							return true;

						if( r.g_w < WGROW * 8 )
						{
							r.g_x = wind->r.g_x;
							r.g_w = wind->r.g_w;
						}
						if( r.g_h < WGROW * 10 )
						{
							r.g_y = wind->r.g_y;
							r.g_h = wind->r.g_h;
						}

						if( r.g_x >= screen.r.g_w )
							r.g_x = screen.r.g_w + g;

						if( r.g_w < wind->min.g_w )
						{
							r.g_x = wind->r.g_x;
							r.g_w = wind->min.g_w;
						}
						if( r.g_h < wind->min.g_h )
						{
							r.g_y = wind->r.g_y;
							r.g_h = wind->min.g_h;
						}
					}
				}

				if( r.g_y >= screen.r.g_h )
					r.g_y = screen.r.g_h - WGROW;
				if( !cfg.leave_top_border && r.g_y < root_window->wa.g_y )
					r.g_y = root_window->wa.g_y;

				if( memcmp(&r, &wind->r, sizeof(GRECT)) )
					do_kbd_win( lock, wind, &r, s ? WM_MOVED : WM_SIZED);
			}
			return true;

		case NK_RIGHT:
		case NK_LEFT:
			if( TOP_WINDOW )
			{
				struct xa_window *wind = TOP_WINDOW;
				short s = key->raw.conin.state & (K_RSHIFT|K_LSHIFT);
				GRECT r;

				if( !wind || wind == root_window || (wind->dial & created_for_POPUP) )
					return true;

				close_menu_if_move(wind);
				client = wind->owner;
				r = wind->r;

				if( !s )
				{
					short g = WGROW;

					if( !wind->rect_list.start
						|| (wind->dial & (created_for_ALERT | created_for_FORM_DO | created_for_WDIAL)
						|| (wind->window_status & (XAWS_ICONIFIED | XAWS_HIDDEN))) )
						return true;
					if( nk == NK_LEFT )
						g = -g;
					r.g_x -= g;
					r.g_w += g * 2;
					if( nk == NK_LEFT )
					{
						if( wind->r.g_w <= wind->min.g_w )
							return true;
						if( r.g_w < wind->min.g_w )
							r.g_w = wind->min.g_w;
					}
					else
					{
						if( wind->r.g_w >= wind->max.g_w )
							return true;
						if( inside_root( &r, true) & 2 )
						{
							r.g_w += g;	/* inside_root changes r */
							if( r.g_w > screen.r.g_w )
								r.g_w = screen.r.g_w;
						}
					}

					do_kbd_win( lock, wind, &r, WM_SIZED );
				}
				else	/* shift */
				{
					if( nk == NK_RIGHT )
					{
						r.g_x += WGROW;
						if( r.g_x >= screen.r.g_w - WGROW * 2)
							return true;
					}
					else{
						r.g_x -= WGROW;
						if( r.g_x + wind->r.g_w <= WGROW/2 )
							return true;
					}
					if( inside_root( &r, client->options.noleft ) & 2 )
						return true;

					do_kbd_win( lock, wind, &r, WM_MOVED );

				}
			}
			return true;

		case 'E':	/* open windows-submenu on top-window */
	
			if( TOP_WINDOW && !C.update_lock )
			{
				struct moose_data md;
	
				/* init moose_data */
				memset( &md, 0, sizeof(md) );
				md.x = TOP_WINDOW->r.g_x + TOP_WINDOW->r.g_w/2;
				md.y = TOP_WINDOW->r.g_y + TOP_WINDOW->r.g_h/2;
				md.state = MBS_LEFT;
				post_cevent(C.Hlp, CE_winctxt, TOP_WINDOW, NULL, 0,0, NULL, &md);
			}
			return true;

		case 'B':	/* system-window */
			if( !C.update_lock )
			{
				post_cevent(C.Hlp, ceExecfunc, open_systemalerts,NULL, 1, 0, NULL,NULL);
			}
			return true;
		case NK_HELP:	/* about-window */
			if( !C.update_lock )
			{
				post_cevent(C.Hlp, ceExecfunc, open_about,NULL, 1,0, NULL,NULL);
			}
			return true;
		case 'K':	/* launcher */
			if( !C.update_lock )
			{
				post_cevent(C.Hlp, ceExecfunc, open_launcher, NULL, HL_LAUNCH, 0, NULL, NULL);
			}
			return true;
		case 'S':	/* save inf */
			write_inf();
			return true;
		case NK_HOME:				/*     "    Home       "     */
			if( C.update_lock )
				return true;
			update_windows_below(lock, &screen.r, NULL, window_list, NULL);
			redraw_menu(lock);
			return true;
		case 'M':				/* ctrl+alt+M  recover mouse */
			forcem();
			return true;

#if WITH_GRADIENTS || WITH_BKG_IMG
		case 'N':	/* load gradients */
			if( !C.update_lock )
			{
				post_cevent(C.Hlp, ceExecfunc, open_launcher, NULL, (key->raw.conin.state & (K_RSHIFT|K_LSHIFT)) ? HL_LOAD_IMG : HL_LOAD_GRAD, 0, NULL,NULL);
			}
			return true;
#endif

#if WITH_BKG_IMG
		case ':':	/* .: make background-image */
			/* if( key->raw.conin.state & (K_RSHIFT|K_LSHIFT) ) */
			{
				do_bkg_img( C.Aes, 1, 0 );
			}
			return true;
#endif

		case '.':	/* .: remove widgets and display top-window full-screen, todo: restore */
			remove_window_widgets(lock, 1);
			return true;

		case ',':	/* ,: remove widgets */
			remove_window_widgets(lock, 0);
			return true;

#if HOTKEYQUIT
		case 'A':
			DIAGS(("Quit all clients by CtlAlt A"));
			post_cevent(C.Hlp, ceExecfunc, ce_quit_all_clients, NULL, !(key->raw.conin.state & (K_RSHIFT|K_LSHIFT)),0, NULL, NULL);
			return true;
#endif

		case 'P':
			if( key->raw.conin.state & (K_RSHIFT|K_LSHIFT) )
			{
				if( !C.update_lock )
				{
					post_cevent(C.Hlp, ceExecfunc, open_launcher, NULL, HL_LOAD_PAL, 0, NULL,NULL);
				}
			}
			else
			{
				DIAGS(("Recover palette"));
				/* if (screen.planes <= 8) */
				{
					set_syspalette( C.Aes->vdi_settings->handle, screen.palette);
				}
			}
			return true;

#if HOTKEYQUIT
		case 'J':
			sdmd = RESTART_XAAES;
			/* fall through */
		case 'H':
			if( !sdmd )
				sdmd = HALT_SYSTEM;
			/* fall through */
		case 'Q':
			{
				struct proc *p;
				const char *sdmaster = sdmd == RESTART_XAAES ? 0 : get_env(0, "SDMASTER=");
	
				if (sdmaster)
				{
					int ret = create_process(sdmaster, NULL, NULL, &p, 0, NULL);
					if (ret < 0)
					{
						if( strlen(sdmaster) > 12 )
							sdmaster += (strlen(sdmaster) - 12);	/* ->fix ALERT! */
						ALERT((xa_strings(AL_SDMASTER), sdmaster));
					}
					else
						return true;
				}
				else	/* the ALERT comes after ce_dispatch_shutdown, so the else must stay for now */
				{
					post_cevent(C.Hlp, ceExecfunc, ce_dispatch_shutdown, NULL, sdmd,1, NULL, NULL);
				}
			}
			return true;
#endif
		case 'Y':				/* ctrl+alt+Y, hide current app. */
			hide_app(lock, focus_owner());
			return true;

		case 'X':				/* ctrl+alt+X, hide all other apps. */
			hide_other(lock, focus_owner());
			return true;
		case 'U':
			{
				struct xa_window *wind;
				wind = TOP_WINDOW;
				if( wind )
				{
					if( wind->send_message )
						wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_CHKDUP,
								   WM_CLOSED, 0, 0, wind->handle, 0,0,0,0);
				}
			}
			return true;
		case 'V':				/* ctrl+alt+V, unhide all. */
			unhide_all(lock, focus_owner());
			return true;

		case 'W':
			if (key->raw.conin.state & (K_RSHIFT|K_LSHIFT))
			{
				if( TOP_WINDOW )
				{
					struct xa_window *w = TOP_WINDOW;

					client = w->owner;
					for( w = w->next; w && (client == w->owner || w->owner == C.Aes || w->owner == C.Hlp); w = w->next )
					;
					if( !w )
						w = TOP_WINDOW->next;
					if( w )
					{
						top_window(lock, true, true, w );
						return true;
					}
				}
				return false;
			}
			else
			{
				struct xa_window *wind;
				wind = next_wind(lock);
				if (wind)
				{
					top_window(lock, true, true, wind);
				}
				return true;
			}

#if GENERATE_DIAGS
		case 'O':
			cfg.menu_locking ^= true;
			return true;
#endif
		}
	}
#endif	/* ALT_CTRL_APP_OPS	*/
	return false;
}

void
keyboard_input(int lock)
{
	/* Did we get some keyboard input? */
#if EIFFEL_SUPPORT
	while (1)
#endif
	{
		struct rawkey key;

		key.raw.bcon = f_getchar(C.KBD_dev, RAW);

		/* this produces wheel-events on some F-keys (eg. S-F10) */
#if EIFFEL_SUPPORT
		if ( !key.raw.conin.state && cfg.eiffel_support && eiffel_wheel((unsigned short)key.raw.conin.scan & 0xff))
		{
			if( f_instat(C.KBD_dev) )
				continue;
			else
				break;
		}
#endif
		/* Translate the BIOS raw data into AES format */
		key.aes = (key.raw.conin.scan << 8) | key.raw.conin.code;
		key.norm = 0;

		if (kernel_key(lock, &key) == false )
		{
#if WITH_BBL_HELP
			xa_bubble( lock, bbl_close_bubble2, 0, 0 );
#endif
			XA_keyboard_event(lock, &key);
		}
#if EIFFEL_SUPPORT
		break;
#endif
	}
}
