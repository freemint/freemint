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

/*
 * Main AES trap handler routine
 * -----------------------------
 * This module replaces the AES trap 2 vector to provide an interface to the
 * XaAES pipe based client/server AES, for normal GEM applications.
 *
 * It works by first creating a pair of XaAES reply pipes for the application
 * in response to the appl_init() call, then using these to communicate with the
 * AES server kernal. When an AES trap occurs, the handler drops the pointer to the
 * parameter block into the XaAES.cmd pipe.
 *
 * There are then 3 modes that the AES could have been called in.
 *
 * If standard GEM emulation mode the handler then drops back into user mode and
 * blocks whilst reading on the current process's reply pipe. This allows other
 * processes to execute whilst XaAES is performing AES functions (the XaAES server
 * runs wholely in user mode so it's more MiNT-friendly than MultiTOS). The server
 * writes back to the clients reply pipe with the reply when it has serviced
 * the command - this unblocks the client which then returns from the exception.
 *
 * If NOREPLY mode is used, the AES doesn't block the calling process to wait
 * for a reply - and indeed, won't generate one.
 *
 * If NOBLOCK mode is used, the AES doesn't block the calling process - but does
 * place the results in the client's reply pipe. The client is then expected to
 * handle it's own reply pipe. This allows multiple AES calls to be made without
 * blocking a process, so an app could make all it's GEM initialisation calls
 * at one go, then go on to do it's internal initialisation before coming back
 * to see if the AES has serviced it's requests (less blocking in the client,
 * and better multitasking).
 *
 * [13/2/96]
 * Included Martin Koehling's patches - this nicely does away with the 'far' data
 * kludges & register patches I had in before.....
 *
 * [18/2/96]
 * New timeout stuff to replace the SIGALRM stuff.
 *
 * [18/12/00]
 * HR: The 4 functions that must always be called direct, and hence run under
 *     the client pid are moved to this file. This way it is easier to detect any
 *     data area's that are shared between the server and the client processes.
 *     These functions are: appl_init, appl_exit, appl_yield and wind_update.
 *
 * [05/12/01]
 * Unclumsified the timeout value passing.
 *
 */


#include <errno.h>
#include <mint/mintbind.h>
#include <fcntl.h>


#include "xa_types.h"
#include "xa_global.h"

#include "handler.h"
#include "xa_codes.h"
#include "xa_evnt.h"
#include "xa_clnt.h"
#include "taskman.h"

#define AES_MAGIC	12345

/* Area's shared between server and client, subject to locking. */
struct shared S;

/*
 * Note: since XA_handler now properly loads the global base register,
 *	 'far' data is no longer needed! <mk>
 * (Except if the 64 K limit is reached, of course...)
 */

/* <beta4>
 * New approach here - the XaAES.cmd pipe is no longer a global handle
 * (there were problems re-opening it after a shutdown), so we have to open it
 * especially - this is in fact the only place it's used. Clients introduce themselves
 * via the XaAES.cmd pipe, then do everything else via their individual pipes.
 * <craig>
 */


/*
 * Application initialise - appl_init()
 * Remember that this executes under the CLIENT pid, not the kernal.
 * (Hence the semaphore locking on the routine)
 */

/* HR 230301:  new_client_contrl
 *             client_exit_contrl
 *             new_client_packet
 *             new_client_pb;
 * moved these areas to the client structure   !!!!!!!
 * Solved many problems with GEM-init of Ulrich Kayser.
 * Solved problems with proper shutdown. (Many apps issue appl_exit concurrently.
 * Sigh of relief. :-)
 */

unsigned long
XA_appl_init(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	int drv,pid = Pgetpid();
	long AES_in;

	CONTROL(0,1,0)

	if unlocked(appl)
	{
#if DEBUG_SEMA
		DIAGS(("For %d:\n", pid));
#endif
		Sema_Up(appl);
	}

	/* must be a foreign client. */
	if (!client)
	{
		if unlocked(clients)
		{
#if DEBUG_SEMA
				DIAGS(("For %d:\n", pid));
#endif
			Sema_Up(clients);
		}

		IFDIAG (if (D.debug_file > 0)
			/* Redirect console output */
		       Fforce(1, D.debug_file);)

		/* New client, but not spawned by XaAES's shell_write */
		client = NewClient(pid);
		if (client)
		{
			strcpy(client->proc_name, "FOREIGN ");
			/* We could get the proc_name I suppose by reading u:\proc. */
			client->parent = Pgetppid();
			client->type = APP_APPLICATION;

			DIAG((D_appl, client, "Alloc client; Foreign %d\n", client->pid));
		}

		if unlocked(clients)
		{
#if DEBUG_SEMA
				DIAGS(("For %d:\n", pid));
#endif
			Sema_Dn(clients);
		}
	}

	DIAG((D_appl, client, "appl_init for %d\n", client->pid));

	/* In XaAES, AESid == MiNT process id : this makes error tracking easier */
	/* HR 150501:
	   At last give in to the fact that it is a struct, NOT an array */
	pb->intout[0] = -1;
	pb->globl->version = 0x0401;		/* Emulate AES4.1 */
	pb->globl->count = -1;			/* Unlimited applications (well, not really) HR: only 32??? */
	pb->globl->id = client->pid;		/* appid==pid */
	pb->globl->pprivate = NULL;
	pb->globl->ptree = NULL;		/* Atari: pointer to pointerarray of trees in rsc. */
	pb->globl->rshdr = NULL;		/* Pointer to resource header. */
	pb->globl->nplanes = screen.planes;
	pb->globl->res1 = 0;
	pb->globl->client_end = 0;
	pb->globl->c_max_h = screen.c_max_h;	/* AES4.0 extensions */
	pb->globl->bvhard = 4;

	client->globl_ptr = pb->globl;		/* Preserve the pointer to the global array */
						/* so we can fill in the resource address later */

	AES_in = Fopen(C.cmd_name, O_RDWR);
	DIAG((D_appl, client, "Open command_pipe %s to %ld\n", C.cmd_name, AES_in));
	if (AES_in > 0)
	{
		pb->intout[0] = client->pid;

		/* Create a new client reply pipe */
		sdisplay(client->app.pipe, "u:\\pipe\\XaClnt.%d", client->pid);

		/* For some reason, a pipe created with mode O_RDONLY does *not* go
			away when all users have closed it (or were terminated) - apparently
			a MiNT bug?!?! */
		/* BTW: if *this* end of the pipe was created with O_RDWR, the *other*
			end cannot be O_WRONLY, or strange things will happen when the
			pipe is closed... */

		/* dynamic client pool: This is normal, end occurs when a program issues
		 * multiple pairs of appl_init/exit, which many old atari programs do.
		 */
		if (!client->client_end)
		{
			/* Client's end of pipe */
			client->client_end = Fopen(client->app.pipe, O_CREAT|O_RDWR);

			DIAG((D_appl, NULL, "pipe '%s' is client_end %d\n", client->app.pipe, client->client_end));
		}

		/* XaAES extension */
		pb->globl->client_end = client->client_end;

		/* Get the client's home directory (where it was started)
		 * - we use this later to load resource files, etc
		 */
		if (!*client->home_path)
		{
			drv = Dgetdrv();
			client->home_path[0] = (char)drv + 'A';
			client->home_path[1] = ':';
			Dgetcwd(client->home_path + 2, drv + 1, sizeof(client->home_path) - 3);

			DIAG((D_appl, client, "[1]Client %d home path = '%s'\n", client->pid, client->home_path));
		}
		else /* already filled out by launch() */
		{
			DIAG((D_appl,client,"[2]Client %d home path = '%s'\n", client->pid, client->home_path));
		}

		/* Reset the AES messages pending list for our new application */
		client->msg = NULL;
		/* Initially, client isn't waiting on any event types */
		cancel_evnt_multi(client,0);
		/* Initial settings for the clients mouse cursor */
		client->mouse = ARROW;		/* Default client mouse cursor is an arrow */
		client->mouse_form = NULL;
		client->save_mouse = client->mouse;
		client->save_mouse_form = client->mouse_form;

		/* Build a 'register new client' packet and send it to the kernal
		 * - The kernal will respond by opening its end of the reply pipe ready for use
		 */
		client->app.ctrl[0] = XA_NEW_CLIENT;
		client->app.pb.contrl = client->app.ctrl;
		client->app.packet.pid = client->pid;		/* Client pid */
		client->app.packet.cmd = AESCMD_STD;		/* No reply */
		client->app.packet.pb = &client->app.pb;	/* Pointer to AES parameter block */

		DIAG((D_appl, client, "Send command %d to %ld\n", client->app.ctrl[0], AES_in));
		/* Send packet */
		Fwrite(AES_in, sizeof(K_CMD_PACKET), &client->app.packet);
		Fclose(AES_in);
		DIAG((D_appl, client, "%ld written & closed\n", AES_in));
	}

	if unlocked(appl)
	{
#if DEBUG_SEMA
		DIAGS(("For %d:\n", pid));
#endif
		Sema_Dn(appl);
	}

	/* Block the client until the server get's it's act together */
	return XAC_BLOCK;
}


/*
 * Application Exit
 * This also executes under the CLIENT pid.
 * This closes the clients end of the reply pipe, and sends a message to the kernal
 * to tell it to close its end as well - the client tidy-up is done at the server
 * end when the XA_CLIENT_EXIT op-code is recieved, not here.
 */
unsigned long
XA_appl_exit(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,0)

	DIAG((D_appl,client,"appl_exit for %d client_end %d\n", client->pid, client->client_end));

	/* Which process are we? It'll be a client pid */
	pb->intout[0] = client->pid;

	if (strnicmp(client->proc_name, "wdialog", 7) == 0)
		return XAC_DONE;

	/* Build a 'client is exiting' packet and send it to the kernal
	 * - The kernal will respond by closing its end of the clients reply pipe.
	 */
	client->app.ctrl[0] = XA_CLIENT_EXIT;
	client->app.pb.contrl = client->app.ctrl;
	client->app.packet.pid = pb->intout[0];		/* Client pid */
	client->app.packet.cmd = AESCMD_STD;		/* No reply */
	client->app.packet.pb = &client->app.pb;	/* Pointer to AES parameter block */

	/* Send packet */
	Fwrite(client->client_end, sizeof(K_CMD_PACKET), &client->app.packet);

#if 0
	/* HR: indeed, this should not be done here, because a Fread on that handle
         *     is still needed to achieve the blocking. ;-)
	 */
	Fclose(client->client_end);
	client->client_end = 0;
#endif

#if 1
 	return XAC_BLOCK;
#else
	return XAC_EXIT;	/* Block the client until the server has closed down it's end of things
				   HR 111201: and then close the clients end.
				              the return is maintained to cleanup the stack. */
#endif
}

/*
 * Free timeslice.
 * Runs (of course) under the clients pid.
 */
unsigned long
XA_appl_yield(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	CONTROL(0,1,0)

	(void) Syield();

	pb->intout[0] = 1;	/* OK */
	return XAC_DONE;

}

/*#define	EACCESS	36*/		/* Access denied */

/*
 * Wind_update handling
 *
 **  XA_wind_update runs under the client pid.
 *
 * This handles locking for the update and mctrl flags.
 * !!!!New version - uses semphores for locking...
 *
 * internal function.
 */
bool
lock_screen(XA_CLIENT *client, long time_out, short *ret, int which)
{
	long r;

	DIAG((D_sema, NULL, "[%d]lock_screen for %s, state: %d for %d\n",
		which, c_owner(client), S.update_cnt, S.update_lock));

	if (S.update_lock == client->pid)
	{
		S.update_cnt++;
		return true;
	}

	r = Psemaphore(2, UPDATE_LOCK, time_out);

	if (r == 0)
	{
		S.update_lock = client->pid;
		S.update_cnt = 1;
		return true;
	}
	if (r == -EACCES)
	{
		if (ret)
			*ret = 0;

		return false;
	}
	if (r == -1)
	{
		S.update_cnt++;
		return true;
	}
	if (r == -ERANGE)
	{
		return false;
	}
	return false;
}

#if 0

	/* Already owning it? */
	if (S.update_lock == client->pid)
	{
		S.update_cnt++;
	}
	else
	{
		long r;

		DIAG((D_sema, NULL,"Sema U up\n"));

		r = Psemaphore(2, UPDATE_LOCK, time_out);
		if (r == -EACCES)
		{
			if (ret)
				/* Screen locked by different process */
				*ret = 0;

			DIAG((D_sema, NULL,"    -36\n"));
			return false;
		}
		else if (r < -1)
		{
			DIAG((D_sema, NULL,"    -1\n"));
			return false;
		}
		else
		{
			S.update_lock = client->pid,
			S.update_cnt = 1;

			DIAG((D_sema, NULL, "    %ld\n", r));
		}
	}

	return true;
}
#endif

/* internal function. */
long
unlock_screen(XA_CLIENT *client, int which)
{
	long r = 0;

	DIAG((D_sema, NULL, "[%d]unlock_screen for %s, state: %d for %d\n",
		which, c_owner(client), S.update_cnt, S.update_lock));

	if (S.update_lock == client->pid)
	{
		S.update_cnt--;
		if (S.update_cnt == 0)
		{
			r = Psemaphore(3, UPDATE_LOCK, 0);
			S.update_lock = 0;
			r = 1;
			DIAG((D_sema, NULL, "Sema U down\n"));
		}
	}

	return r;
}

/* internal function. */
bool
lock_mouse(XA_CLIENT *client, long time_out, short *ret, int which)
{
	DIAG((D_sema, NULL, "[%d]lock_mouse for %s, state: %d for %d\n",
		which, c_owner(client), S.mouse_cnt, S.mouse_lock));

	/* Already owning it? */
	if (S.mouse_lock == client->pid)
	{
		S.mouse_cnt++;
	}
	else
	{
		long r;

		DIAG((D_sema, NULL, "Sema M up\n"));

		r = Psemaphore(2, MOUSE_LOCK, 0);
		if (r == -EACCES)
		{
			if (ret)
				/* Mouse locked by different process */
				*ret = 0;

			DIAG((D_sema, NULL, "    -36\n"));
			return false;
		}
		else if (r < 0)
		{
			DIAG((D_sema, NULL, "    -1\n"));
			return false;
		}
		else
		{
			S.mouse_lock = client->pid;
			S.mouse_cnt = 1 ;
			DIAG((D_sema, NULL, "    %ld\n", r));
		}
	}
	return true;
}

/* internal function. */
long
unlock_mouse(XA_CLIENT *client, int which)
{
	long r = 0;


	DIAG((D_sema, NULL, "[%d]unlock_mouse for %s, state: %d for %d\n",
		which, c_owner(client), S.mouse_cnt, S.mouse_lock));

	if (S.mouse_lock == client->pid)
	{
		S.mouse_cnt--;
		if (S.mouse_cnt == 0)
		{
			S.mouse_lock = 0;
			r = Psemaphore(3, MOUSE_LOCK, 0);
			r = 1;
			DIAG((D_sema, NULL,"Sema M down\n"));
		}
	}

	return r;
}

/* The fmd.lock may only be updated in the real wind_update,
 * otherwise screen writing AES functions that use the internal
 * lock/unlock_screen functions spoil the state.
 * This repairs zBench dialogues.
 * Also changed fmd.lock usage to additive flags.
 */

#define SCREEN_UPD	1
#define MOUSE_UPD	2

unsigned long
XA_wind_update(LOCK lock, XA_CLIENT *client, AESPB *pb)
{
	short op = pb->intin[0];
	long time_out = (op & 0x100) ? 0L : -1L; /* Test for check-and-set mode */
	long r;

	CONTROL(1,1,0)

	DIAG((D_sema, NULL, "XA_wind_update for %s %s\n",
		c_owner(client), time_out ? "" : "RESPONSE"));

	pb->intout[0] = 1;

	switch (op & 0xff)
	{
	/* Grab the update lock */
	case BEG_UPDATE:
	{
		DIAG((D_sema, NULL, ">> Sema U: lock %d, cnt %d\n", S.update_lock, S.update_cnt));

		if (lock_screen(client, time_out, pb->intout, 1))
			client->fmd.lock |= SCREEN_UPD;

		break;
	}
	case END_UPDATE:
	{
		r = unlock_screen(client, 1);
		if (r)
			client->fmd.lock &= ~SCREEN_UPD;

		DIAG((D_sema, NULL, "<< Sema U: lock %d, cnt %d r:%ld\n", S.update_lock, S.update_cnt, r));
		break;
	}
	/* Grab the mouse lock */
	case BEG_MCTRL:
	{
		DIAG((D_sema, NULL, ">> Sema M: lock %d, cnt %d\n", S.mouse_lock, S.mouse_cnt));

		if (lock_mouse(client, time_out, pb->intout, 1))
			client->fmd.lock |= MOUSE_UPD;

		break;
	}
	case END_MCTRL:
	{
		
		r = unlock_mouse(client, 1);
		if (r)
			client->fmd.lock &= ~MOUSE_UPD;

		DIAG((D_sema, NULL, "<< Sema M: lock %d, cnt %d r:%ld\n", S.mouse_lock, S.mouse_cnt, r));
		break;
	}

	default:
		DIAG((D_sema, NULL, "WARNING! Invalid opcode for wind_update: 0x%04x\n", op));
	}

	return XAC_DONE;
}

static void
timer_intout(short *o)
{
	if ( !(o[0] & MU_BUTTON) )
	{
		vq_mouse(C.vh, o+3, o+1, o+2);
		vq_key_s(C.vh, o+4);
	}

	o[0] |= MU_TIMER;
	o[5] = 0;
	o[6] = 0;
}

/* The main AES kernal command jump table */
extern XA_FTAB Ktab[KtableSize];

static XA_CLIENT *
Sema_Pid2Client(LOCK lock, int clnt_pid)
{
	XA_CLIENT *client;
#if DEBUG_SEMA
	DIAGS(("For %d:\n", clnt_pid));
#endif
	/* wait until the client_pool is consistently updated */
	Sema_Up(clients);
	client = Pid2Client(clnt_pid);
#if DEBUG_SEMA
	DIAGS(("For %d:\n", clnt_pid));
#endif
	Sema_Dn(clients);
	return client;
}

/*
 * Trap exception handler
 * - This routine executes under the client application's pid
 * - I've semaphore locked any sensitive bits
 */
short _cdecl
XA_handler(ushort c, AESPB *pb)
{
	LOCK lock = NOLOCKS;
	int clnt_pid;
	XA_CLIENT *client;
	unsigned long cmd_rtn;
	unsigned long reply_s;
	int cmd;

	DIAGS(("XA_handler (c=0x%x, pb=%lx)\n", c, pb));

	/* We must know who we are so we can get our client structure. */
	clnt_pid = Pgetpid();

	/* let the client bomb out */
	if (!pb)
	{
		DIAGS(("No Parameter Block!!!! pid %d\n", clnt_pid));
		return AES_MAGIC;
	}

	if (clnt_pid == C.AESpid)
	{
		DIAGS(("XA_handler for XaAES!!?? pid %d\n", C.AESpid));
		pb->intout[0] = 0;
		return AES_MAGIC;
	}

	cmd = pb->contrl[0];

	if (   cmd == XA_NEW_CLIENT
	    || cmd == XA_APPL_INIT)
		/* Only need this semaphore immediately after NewClient() */
		client = Sema_Pid2Client(lock, clnt_pid);
	else
		client = Pid2Client(clnt_pid);

	/* default paths are kept per process by MiNT ??
	 * so we need to get them here when we run under the process id.

	 */
/*
..25...51, 258, 3,   0,      1, 753,226, 1, 1, 0 - evnt_multi( ... )

 106. 835, 178, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_find(x, y)
 107,   3, 178, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( beg_mctrl )
 24 , 100,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - evnt_timer( 100, 0 )
 79 , 100,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - graf_mkstate()
 107,   2,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( end_mctrl )
 107,   3,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( beg_mctrl )
 78 ,   4,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - graf_mouse( flathand(4), 0)

..79..  4,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - graf_mkstate()

 78,    0,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - graf_mouse(ARROW, 0)
 107,   2,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( end_mctrl)
 107,   1,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( beg_update)
 107,   3,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - wind_update( beg_mctrl)
 30 ,  -1,   0, 4, 444, -14948, 441, 64, 1, 0, 0 - menu_bar( MENU_INQUIRE )
 105,   4,  10, 4, 444, -14949, 441, 64, 1, 0, 0 - win_set( hand, WF_TOP)
 107,   2,  10, 4, 444, -14949, 441, 64, 1, 0, 0 - wind_update( end_mctrl )
 107,   0,  10, 4, 444, -14949, 441, 64, 1, 0, 0 - wind_update( end_update )

..25...51, 258, 3,   0,      1, 753,226, 1, 1, 0 - evnt_multi( ... )

*/
	if (client)
	{

#if 0
		if ( /*(!strcmp("  Thing Desktop", client->name)) || */(!strcmp("  FontList 1.11 ", client->name)) )
			display("%s opcod %d - %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", client->name, cmd,
			pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3], pb->intin[4],
			pb->intin[5], pb->intin[6], pb->intin[7], pb->intin[8], pb->intin[9] );
#endif

		if (   cmd == XA_RSRC_LOAD
		    || cmd == XA_SHELL_FIND)
		{
			client->xdrive = Dgetdrv();
			Dgetpath(client->xpath,0);
		}
	}

	/* better check this first */
	if ((cmd >= 0) && (cmd < KtableSize))
	{
		if (   (cfg.fsel_cookie || cfg.no_xa_fsel)
		    && (   cmd == XA_FSEL_INPUT
		        || cmd == XA_FSEL_EXINPUT))
		{
			DIAG((D_fsel, client, "Redirected fsel call: %d\n", clnt_pid));

			/* This causes call via the old vector
			 * see p_handlr.s
			 */
			return -1;
		}

		/* Call direct? */
		if (   Ktab[cmd].d
#if NOSEMA_CALL_DIRECT
		    || (Ktab[cmd].p & NO_SEMA)
#endif
		   )
		{
			AES_function *cmd_routine = Ktab[cmd].f;

			IFDIAG (extern char *op_code_names[];)
			DIAG((D_trap,client,">>DIRECT: pid=%d, %s[%d] @%lx\n",
				clnt_pid, op_code_names[cmd], cmd, &Ktab[cmd].d));

			/* If opcode was implemented, call it */
			if (cmd_routine)
			{
				/* This is now done only once per AES call */

				/* HR: The root of all locking under client pid. */
#if USE_CALL_DIRECT
				LOCK lock = cfg.direct_call ? NOLOCKS : (winlist|envstr);

				/* The pending sema is only needed if the
				 * evnt_... functions are called direct.
				 */
				if (!Ktab[XA_EVNT_MULTI].d)
					lock |= pending;
#else
				/* HR: how about this? It means that these
	                         * semaphores are not needed and are effectively skipped.
				 */
				LOCK lock = winlist|envstr|pending;
#endif
				vq_mouse(C.vh, &button.b, &button.x, &button.y);
				vq_key_s(C.vh, &button.ks);

				if (Ktab[cmd].p & LOCKSCREEN)
					lock_screen(client, -1, NULL, 2);

				/* callout the AES function */
				cmd_rtn = (*cmd_routine)(lock, client, pb);

				if (Ktab[cmd].p & LOCKSCREEN)
					unlock_screen(client, 2);

				if (!client)
				{
					if (cmd == XA_APPL_INIT)
						client = Sema_Pid2Client(lock, clnt_pid);

					if (!client)
						return AES_MAGIC;
				}

				switch (cmd_rtn)
				{
				/* Command completed, do nothing & return */
				case XAC_DONE:
					break;
				/* Block indefinitely (like for evnt_mesag) */
				case XAC_BLOCK:
					Fread(client->client_end, sizeof(cmd_rtn), &cmd_rtn);
					break;
				/* Block and the close the pipe (for appl_exit) */
				case XAC_EXIT:
					DIAG((D_trap,NULL,"Direct blocking exit command; client_end is %d\n",
						client ? client->client_end : -999));
					Fread(client->client_end, sizeof(cmd_rtn), &cmd_rtn);
					Fclose(client->client_end);
					client->client_end = 0;
					break;
#if USE_CALL_DIRECT
				case XAC_TIMER:
					reply_s = 1L << client->client_end;

					if (!client->timer_val)
						/* Immediate timeout */
						cmd_rtn = 0;
					else
						cmd_rtn = Fselect(client->timer_val, (long *)&reply_s, NULL, NULL);

					Sema_Up(clients);

					if (!cmd_rtn)		/* Timed out */
					{
						if (client->waiting_for & XAWAIT_MULTI)
							/* HR: fill out mouse data!!! */
							timer_intout(client->waiting_pb->intout);
						else
							/* evnt_timer() always returns 1 */
							client->waiting_pb->intout[0] = 1;

						cancel_evnt_multi(client,4);
					}

					Sema_Dn(clients);

					break;
#endif
				}

				return AES_MAGIC;
			}
			/* else pb->intout[0] = 0;  HR: proceed to error exit */
		}
		else if (client)
		{
			/* Nope, go through the pipes messaging system instead... */

			bool isfsel =    cmd == XA_FSEL_INPUT
				      || cmd == XA_FSEL_EXINPUT;

			if (isfsel)
				Sema_Up(fsel);	/* Wait for access to the fileselector */

			if (Ktab[cmd].p & LOCKSCREEN)
				lock_screen(client, -1, NULL, 3);

			/* HR 220501: Use client structures */
			/* Build command packet */
			client->app.packet.pid = clnt_pid;
			client->app.packet.cmd = c;
			client->app.packet.pb = pb;

#if GENERATE_DIAGS
			if (cmd == 107)
			{
				/* strange spurious wind_update	call. */
				DIAGS((" ???? wind_update @%lx\n", &Ktab[cmd].d));
			}
#endif

			/* Send command packet */
			Fwrite(client->client_end, sizeof(K_CMD_PACKET), &client->app.packet);

			/* Unless we are doing standard GEM style AES calls, */
			if (c != AESCMD_STD)
			{
				DIAGS(("non standard command: %d, client->end %d\n", c, client->client_end));
				if (Ktab[cmd].p & LOCKSCREEN)
					unlock_screen(client, 3);

				return client->client_end;
			}

			/* OK, here we are in blocking AES call mode (standard GEM)
			 * - so we handle the reply pipe on behalf of the client.
			 */

			Fread(client->client_end, sizeof(cmd_rtn), &cmd_rtn);

			if (Ktab[cmd].p & LOCKSCREEN)
				unlock_screen(client, 3);

			if (isfsel)
				/* Release the file selector */
				Sema_Dn(fsel);

			/* New timeout stuff */
			switch (cmd_rtn)
			{
			/* Standard stuff, operation completed, etc */
			case XA_OK:
			case XA_ILLEGAL:
			case XA_UNIMPLEMENTED:
				break;
			/* Read again. */
			case XA_UNLOCK:
				Fread(client->client_end, sizeof(unsigned long), &cmd_rtn);
				break;
			/* Unclumsify the timer value passing.
			 * Ahh - block again, with a timeout
			 */
			case XA_TIMER:
			{
				reply_s = 1L << client->client_end;

				if (!client->timer_val)
					/* Immediate timeout */
					cmd_rtn = 0;
				else
					cmd_rtn = Fselect(client->timer_val, (long *)&reply_s, NULL, NULL);

/*				#if DEBUG_SEMA
					DIAGS(("For %d:\n", clnt_pid));
				#endif
				Sema_Up(CLIENTS_SEMA);
*/
				if (!cmd_rtn)
				{
					/* Timed out */

					if (client->waiting_for & XAWAIT_MULTI)
						/* HR: fill out mouse data!!! */
						timer_intout(client->waiting_pb->intout);
					else
						/* evnt_timer() always returns 1 */
						client->waiting_pb->intout[0] = 1;

					cancel_evnt_multi(client,3);
					DIAG((D_kern,client,"[20]Unblocked timer for %s\n",c_owner(client)));
				}
				else
				{
					/* Second dummy read until unblock */

					DIAG((D_kern,client,"Timer block for %s\n",c_owner(client)));
					Fread(client->client_end, sizeof(unsigned long), &cmd_rtn);
				}

/*				#if DEBUG_SEMA
					DIAGS(("For %d:\n", clnt_pid));
				#endif
				Sema_Dn(CLIENTS_SEMA);
*/
				break;
			}
			}
			return AES_MAGIC;
		}
		else
		{
			DIAGS(("No XaAES! Client!!   pid %d, cmd %d\n", clnt_pid, cmd));
			pb->intout[0] = 0;
			return AES_MAGIC;
		}
	}

	/* error exit */
	DIAGS(("Unimplemented AES code: %d\n", cmd));
	pb->intout[0] = 0;

	return AES_MAGIC;
}


typedef void (ExchangeVec)(void);

void
hook_into_vector(void)
{
	extern long old_trap2_vector;
	void handler(void);

	old_trap2_vector = (long) Setexc(0x22, handler);
	DIAGS(("hook_into_vector: old = %lx\n", old_trap2_vector));

	/* We want to do this with task switching disabled in order
	 * to prevent a possible race condition...
	 *
	 * Dummy access to the critical error handler
	 * (make Selectric, FSELECT and other AES extenders happy...)
	 */
	(void) Setexc(0x101, (ExchangeVec *)-1L);
}

struct xbra
{
	long xbra_id;
	long app_id;
	ExchangeVec *oldvec;
};

typedef struct xbra XBRA;

#define XBRA_ID 0x58425241L /* 'XBRA' */
#define XAPP_ID 0x58614145L /* 'XaAE' */

/*
 * New unhook, pays attention to XBRA unhook procedure
 */
void
unhook_from_vector(void)
{
	long old_ssp;
	struct xbra *rx;
	long vecadr, *stepadr;

 	vecadr = (long) Setexc(0x20 + AES_TRAP, (ExchangeVec *)-1L);
 	rx = (XBRA *)(vecadr - sizeof(XBRA));

	old_ssp = Super(0);

	if ((rx->xbra_id == XBRA_ID) && (rx->app_id == XAPP_ID))
	{
		(void) Setexc(0x20 + AES_TRAP, rx->oldvec);
		return;
	}

	stepadr = (long *)&rx->oldvec;
	rx = (XBRA *)((long)rx->oldvec - sizeof(XBRA));
	while (rx->xbra_id == XBRA_ID)
	{
		if (rx->app_id == XAPP_ID)
		{
			*stepadr = (long)rx->oldvec;
			break;
		}
		stepadr = (long*)&rx->oldvec;
		rx = (XBRA *)((long)rx->oldvec - sizeof(XBRA));
	}

	Super(old_ssp);
}
