/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *
 * A multitasking AES replacement for MiNT
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

#include <mint/mintbind.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "xa_types.h"
#include "xa_global.h"
#include "xa_codes.h"
#include "nkcc.h"

#include "xalloc.h"
#include "rectlist.h"
#include "c_window.h"
#if GENERATE_DIAGS
#include "op_names.h"
#endif
#include "taskman.h"
#include "app_man.h"
#include "xa_form.h"
#include "xa_fsel.h"
#include "xa_evnt.h"
#include "xa_rsrc.h"
#include "scrlobjc.h"
#include "xa_clnt.h"

/* Direct call interface enable */
#if USE_CALL_DIRECT
#define CALL_DIRECT(x) Ktab[(x)].d = true
#else
#define CALL_DIRECT(x)		/* HR make empty define */
#endif

XA_FTAB Ktab[KtableSize];	/* The main AES kernel command jump table */

/*
 * Kernel Message Handler
 * 
 * This is responsible for accepting requests via the XaAES.cmd pipe and
 * sending (most) replies via the clients' reply pipes.
 * 	
 * We also get keyboard & mouse input data here.
 */

static unsigned long client_handle_mask  = 0;
static unsigned long client_waiting_mask = 0;

#if VECTOR_VALIDATION
bool
xa_invalid(int which, int pid, void *addr, long size, bool allowNULL)
{
	if (!C.mvalidate)
		return false;

	if (allowNULL && addr == NULL)
		return false;

	{
		long rep;

		rep = Pvalidate(pid, addr, size, NULL);
		DIAGS(("[%d]xa_invalid::%ld for %d: %ld(%lx) + %ld(%lx) --> %ld(%lx)\n",
			which,
			rep,
			pid,
			addr,addr,
			size,size,
			(long)addr+size,
			(long)addr+size ));

		return rep != 0;
	}
}
#endif

void
get_mouse(short which)
{


	/* Ozk: This stuff below reads current mouse from mooses, which
	 * is a new device (of moose). I wonder if this is the way to go,
	 * or if we should stay with calling the VDI. It works great, tho.
	 */
#if 0
	long	n, h;
	struct mooses_data msd;

//	if (!button.have)
//	{
		h = Fopen("u:\\dev\\mooses", O_RDONLY);
		if (h > 0)
		{
			n = Fread(h, sizeof(msd), &msd);
			if (n == sizeof(msd))
			{
				button.cb	= msd.state;
				button.x	= msd.x;
				button.y	= msd.y;
			}
			Fclose(h);
		}
//		else
//		{
//			vq_mouse(C.P_handle, &button.cb, &button.x, &button.y);
//		}

		vq_key_s( C.vh, &button.ks );
		button.client	= NULL;
		button.have	= true;
		button.b	= button.cb;
//	}

#endif

#if 1
	if (!button.have)
	{
		short b;
		
		vq_mouse(C.P_handle, &b, &button.x, &button.y);
		vq_key_s(C.vh, &button.ks);
		button.client = NULL;
		DIAG((D_v, NULL, "[%d]get_mouse %d,%d/%d\n", which, b, button.x, button.y));
		button.have = true;
		/* if (b != button.b)
			button.skip = true; */
		button.b = b;
		button.cb = b;
	}
#endif
}

static void
no_mouse(void)
{
	DIAG((D_v, NULL, "no_mouse\n"));
	button.have = false;
/*	button.got = false; */		/* This is for still button handling.
					 * Ozk: yes, but it should not be cleared until we
					 * have a new button event!!!!
					 */
}

static void
have_mouse(struct moose_data *md)
{
	button.b	= md->state;
	button.cb	= md->cstate;
	button.x	= md->x;
	button.y	= md->y;
	vq_key_s(C.vh, &button.ks);
	DIAG((D_v, NULL, "have_mouse %d,%d/%d\n", button.b, button.x, button.y));
	button.have 	= true;
	button.got	= false;
}

static void
new_mu_mouse(struct moose_data *md)
{
	mu_button.b	= md->state;
	mu_button.cb	= md->cstate;
	mu_button.x	= md->x;
	mu_button.y	= md->y;
	mu_button.clicks = md->clicks;
	vq_key_s(C.vh, &mu_button.ks);
}

void
remove_client_handle(XA_CLIENT *client)
{	
	client_handle_mask &= ~(1UL << client->kernel_end);
}

void
insert_client_handle(XA_CLIENT *client)
{
	client_handle_mask |=  (1UL << client->kernel_end);
}

void
Block(XA_CLIENT *client, int which)
{
	client_waiting_mask |=  (1UL << client->kernel_end);
	DIAG((D_kern,client,"[%d]Blocked %s\n", which, c_owner(client)));
}

void
Unblock(XA_CLIENT *client, unsigned long value, int which)
{
	client->tinybuf = value;
		/* Write a value to clients reply pipe to unblock the process */
	Fwrite(client->kernel_end, sizeof(unsigned long), &client->tinybuf);
	client_waiting_mask &= ~(1UL << client->kernel_end);

	/* HR 041201: the following served as a excellent safeguard on the
	 *            internal consistency of the event handling mechanisms.
	 */
 	if (value == XA_OK)
 	{
		DIAG((D_kern,client,"[%d]Unblocked %s 0x%lx\n", which, c_owner(client), value));
		cancel_evnt_multi(client,1);
	}
}

void
pending_client_exit(XA_CLIENT *client)
{
	int i;

	for (i = 0; i < MAX_CLIENT; i++)
	{
		if (C.pending_exit[i] == NULL)
		{
			DIAGS(("Pending client exit on position %d\n", i));
			C.pending_exit[i] = client;
			break;
		}
	}
}

#if USE_CALL_DIRECT	/* HR: This is really not necessary anymore.
			       Good engineered C is the obvious solution. */
static void
enable_call_direct(void)
{
	/* HR 230501: event functions are NEVER called direct!!!
	 * They are part of the event queuing subsystem.
	 * This subsystem is a integral part of the main kernel loop.
	 */

	CALL_DIRECT(XA_GRAF_HANDLE);
	CALL_DIRECT(XA_GRAF_MOUSE);
	CALL_DIRECT(XA_GRAF_MKSTATE);
	CALL_DIRECT(XA_GRAF_MOVEBOX);
	CALL_DIRECT(XA_GRAF_GROWBOX);
	CALL_DIRECT(XA_GRAF_SHRINKBOX);
	CALL_DIRECT(XA_WIND_FIND);
#if 1
	CALL_DIRECT(XA_WIND_GET);
	CALL_DIRECT(XA_WIND_SET);
#endif
#if 1
	CALL_DIRECT(XA_WIND_CREATE);
	CALL_DIRECT(XA_WIND_DELETE);
	CALL_DIRECT(XA_WIND_OPEN);
	CALL_DIRECT(XA_WIND_CLOSE);
#endif
	CALL_DIRECT(XA_OBJC_ADD);
	CALL_DIRECT(XA_OBJC_DELETE);
	CALL_DIRECT(XA_OBJC_FIND);
	CALL_DIRECT(XA_OBJC_OFFSET);
	CALL_DIRECT(XA_OBJC_ORDER);
#if 1
	CALL_DIRECT(XA_OBJC_CHANGE);
	CALL_DIRECT(XA_OBJC_DRAW);
#endif	
	CALL_DIRECT(XA_RSRC_LOAD);
	CALL_DIRECT(XA_RSRC_FREE);
	CALL_DIRECT(XA_RSRC_GADDR);
	CALL_DIRECT(XA_RSRC_OBFIX);
#if 1
	CALL_DIRECT(XA_MENU_TNORMAL);
	CALL_DIRECT(XA_MENU_ICHECK);
	CALL_DIRECT(XA_MENU_IENABLE);
#endif
}
#endif

void
close_client(LOCK lock, XA_CLIENT *client)
{
	if (client->kernel_end)
	{
		Fclose(client->kernel_end);
		remove_client_handle(client);
		DIAGS(("Closed kernel_end %d\n", client->kernel_end));
	}
	DIAGS(("Free client: %s\n", c_owner(client)));
	FreeClient(lock, client);
	update_tasklist(lock);
}

static bool
kernel_key(LOCK lock, KEY *key)
{
#if ALT_CTRL_APP_OPS	
	if ((key->raw.conin.state&(K_CTRL|K_ALT)) == (K_CTRL|K_ALT))
	{
		XA_CLIENT *client;
		short nk;

		key->norm = nkc_tconv(key->raw.bcon);

		nk = key->norm & 0xff;

		DIAG((D_keybd, NULL,"CTRL+ALT+%04x --> %04x '%c'\n", key->aes, key->norm, nk));

#if GENERATE_DIAGS
		/* CTRL|ALT|number key is emulate wheel. */
		if (   nk=='U' || nk=='N'
		    || nk=='H' || nk=='J')
		{
			short wheel, click;
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

		switch(nk)
		{
		case NK_TAB:						/* TAB, switch menu bars */
			client = next_app(lock); 	 /* including the AES for its menu bar. */
			if (client)
			{
				DIAGS(("next_app() :: %s\n", c_owner(client)));
				app_in_front(lock, client);
			}
			return true;
		case 'R':						/* attempt to recover a hung system */
			recover();
			return true;
		case 'L':						/* open the task manager */
			open_taskmanager(lock, false);
			return true;
		case 'K':						/* tidy up after any clients that have died without calling appl_exit() */
			find_dead_clients(lock);
			return true;
		case 'T':						/* ctrl+alt+T    Tidy screen */
		case NK_HOME:						/*     "    Home       "     */
			display_windows_below(lock, &screen.r, window_list);		/* HR */
		case 'M':						/* ctrl+alt+M  recover mouse */
			forcem();
			return true;
		case 'Q':
		case 'A':
			DIAGS(("shutdown by CtlAlt Q/A\n"));
			shutdown(lock);
	#if 0 /* GENERATE_DIAGS */
			D.debug_level = 4;
	#endif
			return true;
		case 'Y':						/* ctrl+alt+Y, hide current app. */
			hide_app(lock, focus_owner());
			return true;
		case 'X':						/* ctrl+alt+X, hide all other apps. */
			hide_other(lock, focus_owner());
			return true;
		case 'V':						/* ctrl+alt+V, unhide all. */
			unhide_all(lock, focus_owner());
			return true;
	#if GENERATE_DIAGS
		case 'D':						/* ctrl+alt+D, turn on debugging output */
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

BUTTON button = {NULL};
BUTTON mu_button = {NULL};

static int alert_pending = 0;

void
XaAES(void)
{
	unsigned long rtn = XA_OK;
	int client_handle;
	AES_function *cmd_routine;
	unsigned long repl;
	unsigned long input_channels;
	int fs_rtn, r, evnt_count = 0;

#if USE_CALL_DIRECT
	if (cfg.direct_call)
		enable_call_direct();
#endif

	/* Main kernel loop */

	DIAGS(("Handles: MOUSE %ld, KBD %ld, AES %ld, ALERT %ld\n",
		C.MOUSE_dev, C.KBD_dev, C.AES_in_pipe, C.Salert_pipe));

//	display(" mdev %d, kbd %d, aesin %d, alrt %d", C.MOUSE_dev, C.KBD_dev, C.AES_in_pipe, C.Salert_pipe);
	display(" int is %d, short is %d", sizeof(int), sizeof(short));

	pending_button[0].client = 0;
	pending_button[1].client = 0;
	pending_button[2].client = 0;
	pending_button[3].client = 0;

	button.b = 0;
	button.cb = 0;
	button.clicks = 0;
	button.got = true;

	mu_button.b = 0;
	mu_button.cb = 0;
	mu_button.clicks = 0;
	mu_button.x = 0;
	mu_button.y = 0;


	do {
		/* HR: The root of all locking under AES pid. */
#if USE_CALL_DIRECT
		LOCK lock = cfg.direct_call ? NOLOCKS : (winlist|envstr);

		/* The pending sema is only needed if the evnt_... functions
		 * are called direct.
		 */
		if (!Ktab[XA_EVNT_MULTI].d)
			lock |= pending;
#else

		/* HR: how about this? It means that these
	         *     semaphores are not needed and are effectively skipped.
		 */
		LOCK lock = winlist|envstr|pending;
#endif
#if GENERATE_DIAGS
		/* For if no keys can be received anymore. */
		{
			short sta;

			vq_key_s(C.vh, &sta);
			if ((sta & 14) == 14)	/* CTRL|ALT|LSHF (break_in) */
			{
				DIAGS(("shutdown by CtlAltShift\n"));
				shutdown(lock);
				break;
			}
		}
#endif
		/* HR: Due to the nature of the signal handlers in MiNT (The
		 *     handler interrupts currently running XaAES and runs as
		 *     if a seperate thread, but alas! under XaAES's pid,
		 *     which makes the use of a semaphore inappropriate.
		 *     But well, if the handler is running, this code is not!
		 *     So it looks like the easiest way is to just set and
		 *     check a global variable that indicates a XA_client_exit
		 *     action is pending.
		 *     
		 *     100701: And do this before building input_channels,
		 *             otherwise Fselect goes horribly wrong.
		 */
		if (S.update_lock == 0 && S.mouse_lock == 0)
		{
			int i;

			for (i = 0; i < MAX_CLIENT; i++)
			{
				XA_CLIENT *c;

				c = C.pending_exit[i];
				if (c)
				{
					DIAGS(("Pending_exit %d for %s\n", i, c_owner(c)));
					XA_client_exit(lock, c, NULL);
					close_client(lock, c);
					C.pending_exit[i] = NULL;
				}
			}
		}


		input_channels  = 1UL << C.MOUSE_dev;
		input_channels |= 1UL << C.KBD_dev;	/* We are waiting on all these channels */
		input_channels |= 1UL << C.AES_in_pipe;	/* This is only used for appl_init() now */
		input_channels |= 1UL << C.Salert_pipe;	/* Monitor the system alert pipe */
		input_channels |= client_handle_mask;	/* Clients send general requests via their own pipes now */
		/* Feedback: */
		input_channels &= ~client_waiting_mask;	/* Dont select clients that are supposed to be waiting. */

		no_mouse();				/* Indicate mouse data not sampled. */ 

		DIAG((D_kern, NULL, "Fselect mask: 0x%08lx\n", input_channels));

		/* The pivoting point of XaAES!
		 * Wait via Fselect() for keyboard, mouse and the AES command pipe(s).
		 */
		fs_rtn = Fselect(C.active_timeout.timeout, (long *) &input_channels, 0L, 0L);	

//		display("fs_rtn = %d %d %d %d %d %d\n", fs_rtn, C.MOUSE_dev, C.KBD_dev, C.AES_in_pipe, C.Salert_pipe, client_handle_mask);
		DIAG((D_kern, NULL,">>Fselect(t%d) :: %d, channels: 0x%08lx, U%dM%d\n",
			C.active_timeout.timeout, fs_rtn, input_channels, S.update_lock, S.mouse_lock));

		if (C.shutdown)
			break;

		/* get_mouse(0); */

		if (evnt_count == 5000)
		{
			/* Housekeeping */
			evnt_count = 0;
			find_dead_clients(lock);
		}

		if (fs_rtn > 0)
		{
			/* HR Fselect returns no of bits set in input_channels */
			evnt_count++;

			if (input_channels & (1UL << C.Salert_pipe))
			{
				/* System alert? Alert and add it to the log */
				OBJECT *icon;
				char *buf, c;
				long n;

				// display("alrt\n");

				n = Finstat(C.Salert_pipe);
				/* HR MiNT's Fselect bug still there? */
				if (n > 0)
				{
					SCROLL_ENTRY_TYPE flag = FLAG_MAL;
					OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
					buf = xmalloc(sizeof(char) * n + 4,2);
					Fread(C.Salert_pipe, n, buf);

					/* Pretty up the log entry with a nice
					 * icon to match the one in the alert
					 */
					c = buf[1];
					switch(c)
					{
					case '1':
						icon = form + SALERT_IC1;
						break;
					case '2':
						icon = form + SALERT_IC2;
						break;
					case '3':
						icon = form + SALERT_IC3;
						break;
					case '4':
						icon = form + SALERT_IC4;
						break;
					default:
						icon = NULL;
						break;
					}
					
					if (S.update_lock)
						flag |= FLAG_PENDING;

					/* HR! not buf+4; you cant free that! */
					/* Add the log entry */
					add_scroll_entry(form, SYSALERT_LIST, icon, buf, flag);

					 /* HR: Now you can always lookup the error in the log. */
					DIAGS(("ALERT PIPE: '%s' %s\n",buf, S.update_lock ? "pending" : "displayed"));
					if (flag & FLAG_PENDING)
					{
						/* ping */
						Bconout(2,7);
						alert_pending |= 1;
					}
					else
					{
						/* HR: for if a app left the mouse off */
						forcem();
						/* Bring up an alert */
						do_form_alert(lock, C.Aes, 1, buf);
					}
				}
				else
				{
					DIAGS(("No bytes in alert_pipe\n"));
				}
			}
			else
			{
				if (S.update_lock == 0 && alert_pending)
				{
					OBJECT *form = ResourceTree(C.Aes_rsc, SYS_ERROR);
					char *buf;

					do {
						buf = pendig_alerts(form, SYSALERT_LIST);
						if (buf)
						{
							/* HR: for if a app left the mouse off */
							forcem();
							/* Bring up an alert */
							do_form_alert(lock, C.Aes, 1, buf);
						}
					}
					while (buf);

					alert_pending = 0;
				}
			}

			/* Did we get a mouse message? */
			if (input_channels & (1UL << C.MOUSE_dev))
			{
				struct moose_data mdata;
				long n;

				// display("mouse\n");

			#if 0
				mdata.l = 0;
				mdata.ty = 0;
				mdata.x = 0;
				mdata.y = 0;
				mdata.state = 0;
				mdata.cstate = 0;
				mdata.clicks = 0;
				mdata.dbg1 = 0;
				mdata.dbg2 = 0;
			#endif

				/* Read always whole packets, otherwise loose
				 * sync. For now ALL packets are same size,
				 * faster anyhow.
				 * BTW. The record length can still be used
				 * to decide on the amount to read.
				 */
				/* Ozk:
				 * Added 'cstate', in addition to 'state' to the moose_data structure.
				 *'state' indicates which button(s) triggered the event,
				 *'cstate' contains the button state at the time when
				 * double-click timer expires.
				 * Makes it much easier, yah know ;-)
				 */
				n = Fread(C.MOUSE_dev, sizeof(mdata), &mdata);
				if (n == sizeof(mdata))
				{
					/* if (mdata.dbg1 != mdata.dbg2)
					{ DIAG((D_v, NULL,"rptr: %d, wptr:%d\n",mdata.dbg1,mdata.dbg2)); } */

					/* Mouse data packet type */
					switch (mdata.ty)
					{
					case MOOSE_BUTTON_PREFIX:
					{
						DIAG((D_button, NULL, "Button %d on: %d/%d\n",
							mdata.state, mdata.x, mdata.y));
						
						//display("butv pkt: Len = %d, - x,y = %d,%d - state,cstate %d,%d - clicks %d",
						//	mdata.l, mdata.x, mdata.y, mdata.state, mdata.cstate, mdata.clicks);

						have_mouse(&mdata);
						new_mu_mouse(&mdata);
						XA_button_event(lock, &mdata, true);

						/* Ozk: If we get a button packet in which the button
						 * state at the time moose sent it is 'released', we
						 * need to fake a 'button-released' event.
						 * If state == cstate != 0, moose will send a 'button-released'
						 * packet.
						*/ 
						if ( mdata.state && !mdata.cstate )
						{
							mdata.state	= 0;
							mdata.cstate	= 0;
							mdata.clicks	= 0;
							have_mouse(&mdata);
							new_mu_mouse(&mdata);
							XA_button_event(lock, &mdata, true);
						}

						/* Ozk: button.got is now used as a flag indicating
						 * whether or not a mouse-event packet has been
						 * delivered, queued as pending or not.
						 */
						button.got = true;
						break;

#if 0 /* Ozk: Lets skip all this shit.. what is this anyway? */
						if (button.skip)
						{
							/* HR 121102: already processed */
							DIAG((D_button, NULL, "Button skipped\n"));
							button.skip = false;
						}
						/* else */
						{
							have_mouse(&mdata);
							/* Call the mouse button event handler */
							XA_button_event(lock, &mdata, true);
							/* Mark event processed. */
							button.got = true;
						}
						break;
#endif
					}

					case MOOSE_MOVEMENT_PREFIX:
					{
						/* HR: new mouse rectangle events */
						DIAG((D_v, NULL,"mouse move to: %d/%d\n", mdata.x, mdata.y));

						/* no_mouse(); 
						 * HR 061201: needed here, because the button
						 * state cannot be up to date in this case.
						 * This leaves the button info to multi_intout()
						 */
						
						/* Call the mouse movement event handler (doesnt use md->state) */
						XA_move_event(lock, &mdata);
						break;
					}

					case MOOSE_WHEEL_PREFIX:
					{
						XA_wheel_event(lock, &mdata);
						break;
					}
					default:
					{
						DIAG((D_mouse, NULL, "Unknown mouse datatype (0x%x)\n", mdata.ty));
						DIAG((D_mouse, NULL, " l=0x%x, ty=%d, x=%d, y=%d, state=0x%x, clicks=%d\n",
							mdata.l, mdata.ty, mdata.x, mdata.y, mdata.state, mdata.clicks));
						DIAG((D_mouse, NULL, " dbg1=0x%x, dbg2=0x%x\n",
							mdata.dbg1, mdata.dbg2));
						break;
					}
					}
				}
				else
				{
					DIAG((D_mouse, NULL,"Moose channel yielded %ld\n", n));
				}
			}

			if (input_channels & (1UL << C.KBD_dev))
			{
				/* Did we get some keyboard input? */
				KEY key;
				key.raw.bcon = Bconin(2); /* Crawcin(); */
				
				// display("kbd\n");

				/* Translate the BIOS raw data into AES format */
				key.aes = (key.raw.conin.scan<<8) | key.raw.conin.code;
				key.norm = 0;
#if 1
				DIAGS(("Bconin: 0x%08lx\n",key.raw.bcon));
#else
				DIAG((D_k, NULL, "XA_keyboard_event: 0x%08lx\n",key.raw.bcon));
#endif
				if (!kernel_key(lock, &key))
					XA_keyboard_event(lock, &key);
			}

			input_channels &= (client_handle_mask | (1UL << C.AES_in_pipe));
			client_handle = 0;

			/* HR: Removed stupid inner loop which was the cause of
			 * always reading non existing channel 32 !!!!
			 */
			while (input_channels)
			{
				if (input_channels & 1UL)
				{
					K_CMD_PACKET b;

					r = Fread(client_handle, sizeof(K_CMD_PACKET), &b);
					if (r == sizeof(K_CMD_PACKET) && b.pb)
					{
						XA_CLIENT *client = Pid2Client(b.pid);
						int cmd = b.pb->contrl[0];
						repl = XAC_DONE;
						
						// display("h %d, cmd %d, client %s\n", client_handle, cmd, client->name);

#if GENERATE_DIAGS
						if (cmd >= 0 && cmd <= MAX_NAMED_DIAG)	/* HR >= 0 */
						{
							DIAG((D_trap,client,">>CMD_PIPE: pid=%d, %s[%d] pb:%lx, intin:%lx, [0]=%d\n",
								b.pid, op_code_names[cmd], cmd, b.pb, b.pb->intin, b.pb->intin[0]));
						}
						else
							DIAG((D_trap,client,">>CMD_PIPE: pid=%d, op-code=%d\n", b.pid, cmd));

						/* strange spurious wind_update call. */
						if (cmd == 107)
						{
							DIAG((D_trap,client," **** wind_update @%lx\n", &Ktab[cmd].d));
							goto cont;
						}
#endif
						/* Call AES routine via jump table */
			
						if (client)
						{
							if (cmd >= 0 && cmd < KtableSize)
							{
								cmd_routine = Ktab[cmd].f;
								/* Do we support this op-code yet? */
								if (cmd_routine != NULL)
								{
									/* !! Call the Aes function proper !! */
									repl =(*cmd_routine)(lock, client, b.pb);
									rtn = XA_OK;
								}
								else
								{
									DIAG((D_trap,client,"WARNING: cmd_pipe: pid:%d, Opcode %d not implemented\n", b.pid, cmd));
									/* Unimplemented functions :-( */
									rtn = XA_UNIMPLEMENTED;
								}
							}
							else
							{
								DIAG((D_trap,client,"WARNING: illegal AES opcode=%d\n", cmd));
								/* Illegal op-code - these may be caused by bugs in the client program */
								rtn = XA_ILLEGAL;
							}

							/* If client wants a reply, send it one -
							 * standard GEM programs will always do this, 
							 * but XaAES aware programs don't always need
							 * to (depends if they are going to use the
							 * reply I suppose). Some op-codes (evnt_multi
							 * for instance) will want to leave the client
							 * blocked until an event occurs.
							 * I've added some extra blocking modes to
							 * support better timeouts...
							 * HR 051201: Unclumsified the timer handling.
							 *            Just pass the timer value in client
							 *            structure. that leaves only 1 mode
							 *            extra throughout.
							 */

							if (b.cmd == AESCMD_NOREPLY || repl == XAC_BLOCK)
							{
								if (Ktab[cmd].p & LOCKSCREEN)
									/* HR 250202:
									 * screen locking AES functions:
									 * return to the handler which will
									 * then unlock and reread.
									 */
									Unblock(client, XA_UNLOCK, 10);
								else
									Block(client, 1);
							}
							else
							{
								if (repl == XAC_TIMER)
									rtn = repl;
#if GENERATE_DIAGS
								else
								{
									/* must be XAC_DONE. */
									if (cmd == XA_EVNT_MULTI)
										evnt_diag_output(client->waiting_pb, client, "kernel ");
								}
#endif
								Unblock(client, rtn, 1);
							}
						} /* if client */
					} /* if r && b.pb */
				} /* if input_channels & 1 */
cont:
				/* HR: Think simple please!! */
				input_channels >>= 1;
				client_handle++;
			} /* while channels */
		} /* if fs_rtn > 0 */
#if GENERATE_DIAGS
		else if (fs_rtn < 0)
		{
			DIAG((D_kern, NULL, ">>Fselect :: %d\n",fs_rtn));
		}
#endif
		else
		{
			/* Call the active function if we need to */
			if (C.active_timeout.task)
			{
//				display(" timeout!\n");
				if (C.active_timeout.timeout != 0
				     && (fs_rtn == 0 || (evnt_count & 0xff) == 1))
					(*C.active_timeout.task)(&C.active_timeout);
			}
		}
	}
	while (!C.shutdown);

	DIAGS(("**** Leave kernel for shutdown\n"));
}


/*
 * Setup the AES kernal jump table
 * 
 * HR 210202: LOCKSCREEN flag for AES functions that are writing the screen
 *            and are supposed to be locking
 * HR 110802: NO_SEMA flag for Call direct for small AES functions that do
 *            not need the call_direct semaphores.
 */
void
setup_k_function_table(void)
{
	bzero(Ktab, sizeof(Ktab));

	/* appl_ class functions */
	
	Ktab[XA_APPL_INIT   ].f = XA_appl_init;
	Ktab[XA_APPL_INIT   ].d = true;		/* Must always call appl_init/exit/yield directly */

	Ktab[XA_APPL_EXIT   ].f = XA_appl_exit;
	Ktab[XA_APPL_EXIT   ].d = true;		/* because they must run under the client pid */

	Ktab[XA_APPL_GETINFO].f = XA_appl_getinfo;
	Ktab[XA_APPL_GETINFO].p = NO_SEMA;

	Ktab[XA_APPL_FIND   ].f = XA_appl_find;
	Ktab[XA_APPL_FIND   ].p = NO_SEMA;

	Ktab[XA_APPL_WRITE  ].f = XA_appl_write;
/*	Ktab[XA_APPL_WRITE  ].p = NO_SEMA; */

	Ktab[XA_APPL_YIELD  ].f = XA_appl_yield;
	Ktab[XA_APPL_YIELD  ].d = true;

	Ktab[XA_APPL_SEARCH ].f = XA_appl_search;
	Ktab[XA_APPL_CONTROL].f = XA_appl_control;

	/* Form handlers (form_ xxxx) */

	Ktab[XA_FORM_ALERT ].f = XA_form_alert;
	Ktab[XA_FORM_ALERT ].p = LOCKSCREEN;
	Ktab[XA_FORM_ERROR ].f = XA_form_error;
	Ktab[XA_FORM_ERROR ].p = LOCKSCREEN;

	Ktab[XA_FORM_CENTER].f = XA_form_center;
	Ktab[XA_FORM_CENTER].p = NO_SEMA;

	Ktab[XA_FORM_DIAL  ].f = XA_form_dial;
	Ktab[XA_FORM_DIAL  ].p = LOCKSCREEN;
	Ktab[XA_FORM_BUTTON].f = XA_form_button;
	Ktab[XA_FORM_DO    ].f = XA_form_do;
	Ktab[XA_FORM_DO    ].p = LOCKSCREEN;
	Ktab[XA_FORM_KEYBD ].f = XA_form_keybd;

	/* File select (fsel_ xxx) */
#if FILESELECTOR
	Ktab[XA_FSEL_INPUT  ].f = XA_fsel_input;
	Ktab[XA_FSEL_INPUT  ].p = LOCKSCREEN;
	Ktab[XA_FSEL_EXINPUT].f = XA_fsel_exinput;
	Ktab[XA_FSEL_EXINPUT].p = LOCKSCREEN;
#if 0
	/* HR: semaphore moved to handler (under client pid).
	 * Now the fsel itself runs in user mode under the AES pid.
	 */
	
	/* Must always call fsel_xxx direct as they use a global semaphore lock */
	Ktab[XA_FSEL_INPUT  ].d = true;
	Ktab[XA_FSEL_EXINPUT].d = true;
#endif
#endif

	/* Event handlers (evnt_ xxx) */

	Ktab[XA_EVNT_BUTTON].f = XA_evnt_button;
	Ktab[XA_EVNT_KEYBD ].f = XA_evnt_keybd;
	Ktab[XA_EVNT_MESAG ].f = XA_evnt_mesag;
	Ktab[XA_EVNT_MULTI ].f = XA_evnt_multi;
	Ktab[XA_EVNT_TIMER ].f = XA_evnt_timer;
	Ktab[XA_EVNT_MOUSE ].f = XA_evnt_mouse;
/*	Ktab[XA_EVNT_DCLICK].f = XA_evnt_dclick; */


	/* graf_ class functions */

	Ktab[XA_GRAF_RUBBERBOX].f = XA_graf_rubberbox;
	Ktab[XA_GRAF_DRAGBOX  ].f = XA_graf_dragbox;

	Ktab[XA_GRAF_HANDLE   ].f = XA_graf_handle;
	Ktab[XA_GRAF_HANDLE   ].p = NO_SEMA;

	Ktab[XA_GRAF_MOUSE    ].f = XA_graf_mouse;
	Ktab[XA_GRAF_MOUSE    ].p = NO_SEMA;

	Ktab[XA_GRAF_MKSTATE  ].f = XA_graf_mkstate;
/*	Ktab[XA_GRAF_MKSTATE  ].p = NO_SEMA; */

/*	Ktab[XA_GRAF_GROWBOX  ].f = XA_graf_growbox; */
/*	Ktab[XA_GRAF_SHRINKBOX].f = XA_graf_growbox; */
/*	Ktab[XA_GRAF_MOVEBOX  ].f = XA_graf_movebox; */
	Ktab[XA_GRAF_SLIDEBOX ].f = XA_graf_slidebox;
	Ktab[XA_GRAF_WATCHBOX ].f = XA_graf_watchbox;

	/* Window Handling (wind_ xxxx) */

	Ktab[XA_WIND_CREATE].f = XA_wind_create;

	Ktab[XA_WIND_OPEN  ].f = XA_wind_open;
	Ktab[XA_WIND_OPEN  ].p = LOCKSCREEN;

	Ktab[XA_WIND_CLOSE ].f = XA_wind_close;
	Ktab[XA_WIND_CLOSE ].p = LOCKSCREEN;

	Ktab[XA_WIND_SET   ].f = XA_wind_set;
	Ktab[XA_WIND_SET   ].p = LOCKSCREEN;

	Ktab[XA_WIND_GET   ].f = XA_wind_get;
	Ktab[XA_WIND_FIND  ].f = XA_wind_find;

	Ktab[XA_WIND_UPDATE].f = XA_wind_update;
	/* wind_update must ALWAYS be call direct as it uses semaphore locking */
	Ktab[XA_WIND_UPDATE].d = true;

	Ktab[XA_WIND_DELETE].f = XA_wind_delete;

	Ktab[XA_WIND_NEW   ].f = XA_wind_new;
	Ktab[XA_WIND_NEW   ].p = LOCKSCREEN;

	Ktab[XA_WIND_CALC  ].f = XA_wind_calc;

	/* Object Tree Handling (objc_ xxxx) */

	Ktab[XA_OBJC_ADD   ].f = XA_objc_add;
	Ktab[XA_OBJC_DELETE].f = XA_objc_delete;

	Ktab[XA_OBJC_DRAW  ].f = XA_objc_draw;
/*	Ktab[XA_OBJC_DRAW  ].p = LOCKSCREEN; */

	Ktab[XA_OBJC_FIND  ].f = XA_objc_find;
	Ktab[XA_OBJC_FIND  ].p = NO_SEMA;

	Ktab[XA_OBJC_OFFSET].f = XA_objc_offset;
	Ktab[XA_OBJC_OFFSET].p = NO_SEMA;

	Ktab[XA_OBJC_ORDER ].f = XA_objc_order;
	Ktab[XA_OBJC_CHANGE].f = XA_objc_change;
	Ktab[XA_OBJC_EDIT  ].f = XA_objc_edit;

	Ktab[XA_OBJC_SYSVAR].f = XA_objc_sysvar;
	Ktab[XA_OBJC_SYSVAR].p = NO_SEMA;

	/* Resource Handling */

	Ktab[XA_RSRC_LOAD ].f = XA_rsrc_load;
	Ktab[XA_RSRC_FREE ].f = XA_rsrc_free;

	Ktab[XA_RSRC_GADDR].f = XA_rsrc_gaddr;
	Ktab[XA_RSRC_GADDR].p = NO_SEMA;

	Ktab[XA_RSRC_OBFIX].f = XA_rsrc_obfix;
	Ktab[XA_RSRC_OBFIX].p = NO_SEMA;

	Ktab[XA_RSRC_RCFIX].f = XA_rsrc_rcfix;
#if MEM_PROT
	/* rsrc_load must ALWAYS be call direct as it uses semaphore locking */
	Ktab[XA_RSRC_LOAD].d = true;
	/* rsrc_free must ALWAYS be call direct as it uses semaphore locking */
	Ktab[XA_RSRC_FREE].d = true;
#endif
	/* Menu Handling */

	Ktab[XA_MENU_BAR     ].f = XA_menu_bar;
	Ktab[XA_MENU_BAR     ].p = LOCKSCREEN;
	Ktab[XA_MENU_TNORMAL ].f = XA_menu_tnormal;
	Ktab[XA_MENU_TNORMAL ].p = LOCKSCREEN;
	Ktab[XA_MENU_ICHECK  ].f = XA_menu_icheck;
	Ktab[XA_MENU_ICHECK  ].p = LOCKSCREEN;
	Ktab[XA_MENU_IENABLE ].f = XA_menu_ienable;
	Ktab[XA_MENU_IENABLE ].p = LOCKSCREEN;
	Ktab[XA_MENU_TEXT    ].f = XA_menu_text;
	Ktab[XA_MENU_REGISTER].f = XA_menu_register;
	Ktab[XA_MENU_POPUP   ].f = XA_menu_popup;
/*	Ktab[XA_MENU_POPUP   ].p = LOCKSCREEN; */	/* Since we cannot lock with client PID
							 * and unlock under kernel PID
							 * we gotta let the function itself (under kernel PID)
							 * do the lock_screen() call
							*/
	Ktab[XA_MENU_ATTACH  ].f = XA_menu_attach;
	Ktab[XA_MENU_ISTART  ].f = XA_menu_istart;
	Ktab[XA_MENU_SETTINGS].f = XA_menu_settings;
	
	/* Shell */

	Ktab[XA_SHELL_WRITE].f = XA_shell_write;
	Ktab[XA_SHELL_READ ].f = XA_shell_read;
	Ktab[XA_SHELL_FIND ].f = XA_shell_find;
	Ktab[XA_SHELL_ENVRN].f = XA_shell_envrn;
#if MEM_PROT
	Ktab[XA_SHELL_ENVRN].d = true;
#endif
	/* Scrap / Clipboard */

	Ktab[XA_SCRAP_READ ].f = XA_scrap_read;
	Ktab[XA_SCRAP_WRITE].f = XA_scrap_write;

	Ktab[XA_FORM_POPUP ].f = XA_form_popup;
//	Ktab[XA_FORM_POPUP ].p = LOCKSCREEN;		/* Same situation as for XA_menu_popup */

#if WDIAL
	/* HR 261101: started implementation of WDIALOG functions. */
	Ktab[XA_WDIAL_CREATE].f = XA_wdlg_create;
	Ktab[XA_WDIAL_OPEN  ].f	= XA_wdlg_open;
	Ktab[XA_WDIAL_CLOSE ].f	= XA_wdlg_close;
	Ktab[XA_WDIAL_DELETE].f	= XA_wdlg_delete;
	Ktab[XA_WDIAL_GET   ].f = XA_wdlg_get;
	Ktab[XA_WDIAL_SET   ].f = XA_wdlg_set;
	Ktab[XA_WDIAL_EVENT ].f = XA_wdlg_event;
	Ktab[XA_WDIAL_REDRAW].f = XA_wdlg_redraw;
#endif

#if LBOX
	Ktab[XA_LBOX_CREATE ].f = XA_lbox_create;
	Ktab[XA_LBOX_UPDATE ].f = XA_lbox_update;
	Ktab[XA_LBOX_DO     ].f = XA_lbox_do;
	Ktab[XA_LBOX_DELETE ].f = XA_lbox_delete;
	Ktab[XA_LBOX_GET    ].f = XA_lbox_get;
	Ktab[XA_LBOX_SET    ].f = XA_lbox_set;
#endif

	/* XaAES specific AES calls */

	Ktab[XA_APPL_PIPE  ].f = XA_appl_pipe;

	/*
	 * XaAES kernel internal messages - applications should NEVER send
	 * these to the kernel, they are used internally to pass crucial info
	 * from the client pid trap handler to the kernel.
	 */

	Ktab[XA_NEW_CLIENT ].f = XA_new_client;
	Ktab[XA_CLIENT_EXIT].f = XA_client_exit;
}

/* 
 * Set the "active" function  
 */
/* HR: extended & generalized */
/* preliminary versions (only 1 such task at a time),
   but it is easy expandable. */
Tab *
new_task(Tab *new)
{
	bzero(new, sizeof(*new));
	return new;
}

void
free_task(Tab *t, int *nester)
{
	if (t)
	{
		if (t->nest && *nester)
		{
			nester--;
			free_task(t->nest, nester);
		}

		t->nest = 0;
		t->ty = NO_TASK;
		t->pr = 0;
		t->nx = 0;
		t->task = 0;
		t->timeout = 0;
	}
}
