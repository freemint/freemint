/*
 * $Id$
 *
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

#include RSCHNAME

#include "k_keybd.h"
#include "c_keybd.h"
#include "xa_global.h"

#include "app_man.h"
#include "c_window.h"
#include "cnf_xaaes.h"
#include "desktop.h"
#include "init.h"
#include "k_main.h"
#include "k_mouse.h"
#include "nkcc.h"
#include "scrlobjc.h"
#include "taskman.h"
#include "widgets.h"
#include "xalloc.h"

#include "xa_form.h"
#include "xa_rsrc.h"
#include "xa_shel.h"

#include "mint/dcntl.h"
#include "mint/fcntl.h"
#include "mint/signal.h"


/* Keystrokes must be put in a queue if no apps are waiting yet.
 * There also may be an error in the timer event code.
 */
struct key_queue pending_keys;

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

/* WDIAL: split off as a function for use in ExitForm functions */
void 
keybd_event(enum locks lock, struct xa_client *client, struct rawkey *key)
{
	AESPB *pb = client->waiting_pb;

	if (client->waiting_for & XAWAIT_MULTI)
	{
		/* If the client is waiting on a multi, the response is
		 * slightly different to the evnt_keybd() response.
		 */
		check_mouse(client, NULL, NULL, NULL);
		mu_button.ks = key->raw.conin.state;
		/* XaAES extension: return normalized keycode for MU_NORM_KEYBD */
		if (client->waiting_for & MU_NORM_KEYBD)
		{
			key->norm = nkc_tconv(key->raw.bcon);
			multi_intout(client, pb->intout, MU_NORM_KEYBD);
			pb->intout[5] = key->norm;
			pb->intout[4] = key->norm; /* for convenience */

			DIAG((D_k, NULL, "evnt_multi normkey to %s: 0x%04x",
				c_owner(client), key->norm));	
		}
		else
		{
			multi_intout(client, pb->intout, MU_KEYBD);
			pb->intout[5] = key->aes;

			DIAG((D_k, NULL, "evnt_multi key to %s: 0x%04x",
				c_owner(client), key->aes));	
		}
	}
	else
	{
		pb->intout[0] = key->aes;
		DIAG((D_k, NULL, "evnt_keybd keyto %s: 0x%04x",
			c_owner(client), key->aes));	
	}
	client->usr_evnt = 1;
	//Unblock(client, XA_OK, 6);
}

void
XA_keyboard_event(enum locks lock, struct rawkey *key)
{
	struct xa_window *top = window_list;
	struct xa_client *locked_client;
	struct xa_client *client;
	struct rawkey *rk;
	bool waiting;

	client = find_focus(&waiting, &locked_client);

	DIAG((D_keybd,client,"XA_keyboard_event: %s; update_lock:%d, focus: %s, window_list: %s",
		waiting ? "waiting" : "", update_locked() ? update_locked()->p->pid : 0,
		c_owner(client), w_owner(top)));

	/* Found either (MU_KEYBD|MU_NORM_KEYBD) or keypress handler. */
	if (waiting)
	{
		struct xa_client *check = update_locked();

		/* See if a (classic) blocked form_do is active */
		if (check && check == client)
		{
			DIAGS(("Classic: fmd.lock %d, via %lx", client->fmd.lock, client->fmd.keypress));

			if (client->fmd.lock && client->fmd.keypress)
			{
				rk = kmalloc(sizeof(struct rawkey));
				if (rk)
				{
					*rk = *key;
					post_cevent(client, cXA_fmdkey, rk, 0, 0, 0, 0, 0);
				}
				//client->fmd.keypress(lock, NULL, &client->wt, key->aes, key->norm, *key);
				return;
			}
		}

		if (is_hidden(top))
		{
			unhide_window(lock, top);
			return;
		}

		/* Does the top & focus window have a keypress handler callback? */
		if (top->keypress)
		{
			rk = kmalloc(sizeof(struct rawkey));
			if (rk)
			{
				*rk = *key;
				post_cevent(top->owner, cXA_keypress, rk, top, 0, 0, 0, 0);
			}	
			//top->keypress(lock, top, NULL, key->aes, key->norm, *key);
			return;
		}
		else if (!client->waiting_pb)
		{
			DIAGS(("XA_keyboard_event: INTERNAL ERROR: No waiting pb."));
			return;
		}

		rk = kmalloc(sizeof(struct rawkey));
		if (rk)
		{
			*rk = *key;
			post_cevent(client, cXA_keybd_event, rk, 0, 0, 0, 0, 0);
		}
#if 0
		Sema_Up(clients);
		keybd_event(lock, client, key);
		Sema_Dn(clients);
#endif
	}
	else
	{
		int c = pending_keys.cur;
		int e = pending_keys.last;

		Sema_Up(pending);

		DIAG((D_keybd,NULL,"pending key cur=%d", c));
		/* If there are pending keys and the top window owner has changed, throw them away. */

		/* FIX! must compare with last queued key!!! */
		if (c != e && client != pending_keys.q[e-1].client)
		{
			DIAG((D_keybd, NULL, " -  clear: cl=%s", c_owner(client)));
			DIAG((D_keybd, NULL, "           qu=%s", c_owner(pending_keys.q[e-1].client)));
			e = c = 0;
		}

		if (e == KEQ_L)
			e = 0;

		DIAG((D_keybd,NULL," -     key %x to queue position %d", key->aes, e));
		pending_keys.q[e].k   = *key;			/* all of key */
		pending_keys.q[e].locked = locked_client;
		pending_keys.q[e].client = client;		/* see find_focus() */
		e++;
		pending_keys.last = e;
		pending_keys.cur = c;

		Sema_Dn(pending);
	}
}

static bool
kernel_key(enum locks lock, struct rawkey *key)
{
#if ALT_CTRL_APP_OPS	
	if ((key->raw.conin.state&(K_CTRL|K_ALT)) == (K_CTRL|K_ALT))
	{
		struct xa_client *client;
		short nk;

		key->norm = nkc_tconv(key->raw.bcon);

		nk = key->norm & 0xff;

		DIAG((D_keybd, NULL,"CTRL+ALT+%04x --> %04x '%c'", key->aes, key->norm, nk));

#if GENERATE_DIAGS
		/* CTRL|ALT|number key is emulate wheel. */
		if (   nk=='U' || nk=='N'
		    || nk=='H' || nk=='J')
		{
			short  wheel = 0;
			short  click = 0;
			struct moose_data md = { 0, 0, 0, 0, 0, 0, 0, 0 };

			cfg.wheel_amount = 1;

			switch (nk)
			{
			case 'U':
				wheel = 0, click = -cfg.wheel_amount;
				break;
			case 'N':
				wheel = 0, click = cfg.wheel_amount;
				break;
			case 'H':
				wheel = 1, click = -cfg.wheel_amount;
				break;
			case 'J':
				wheel = 1, click = cfg.wheel_amount;
				break;
			}

			md.state = wheel;
			md.clicks = click;

			XA_wheel_event(lock, &md);
			return true;
		}
#endif

		switch (nk)
		{
		case NK_TAB:				/* TAB, switch menu bars */
			client = next_app(lock); 	 /* including the AES for its menu bar. */
			if (client)
			{
				DIAGS(("next_app() :: %s", c_owner(client)));
				app_in_front(lock, client);
			}
			return true;
		case 'R':				/* attempt to recover a hung system */
			recover();
			return true;
		case 'L':				/* open the task manager */
		case NK_ESC:
			open_taskmanager(lock);
			return true;
		case 'T':				/* ctrl+alt+T    Tidy screen */
		case NK_HOME:				/*     "    Home       "     */
			display_windows_below(lock, &screen.r, window_list);
		case 'M':				/* ctrl+alt+M  recover mouse */
			forcem();
			return true;
		case 'Q':
		case 'A':
			DIAGS(("shutdown by CtlAlt Q/A"));
			shutdown(lock);
			C.shutdown = QUIT_XAAES;
			return true;
		case 'Y':				/* ctrl+alt+Y, hide current app. */
			hide_app(lock, focus_owner());
			return true;
		case 'X':				/* ctrl+alt+X, hide all other apps. */
			hide_other(lock, focus_owner());
			return true;
		case 'V':				/* ctrl+alt+V, unhide all. */
			unhide_all(lock, focus_owner());
			return true;
	#if GENERATE_DIAGS
		case 'D':				/* ctrl+alt+D, turn on debugging output */
			D.debug_level = 4;
			return true;
	#endif
#if GENERATE_DIAGS
		case 'O':
			cfg.menu_locking ^= true;
			return true;
#endif
		}
	}
#endif
	return false;
}

void
keyboard_input(enum locks lock)
{
	/* Did we get some keyboard input? */
	struct rawkey key;

	key.raw.bcon = b_ubconin(2); /* Crawcin(); */

	/* Translate the BIOS raw data into AES format */
	key.aes = (key.raw.conin.scan<<8) | key.raw.conin.code;
	key.norm = 0;

	DIAGS(("Bconin: 0x%08lx", key.raw.bcon));

	if (!kernel_key(lock, &key))
		XA_keyboard_event(lock, &key);
}
