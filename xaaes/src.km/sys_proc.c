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

#include "sys_proc.h"

#include "xa_appl.h"
#include "semaphores.h"


static void _cdecl xaaes_share(void *);
static void _cdecl xaaes_release(void *);

static void _cdecl xaaes_on_exit(void *, struct proc *p, int code);
static void _cdecl xaaes_on_exec(void *, struct proc *p);
static void _cdecl xaaes_on_fork(void *, struct proc *p, long flags, struct proc *child);
static void _cdecl xaaes_on_stop(void *, struct proc *p, unsigned short nr);
static void _cdecl xaaes_on_signal(void *, struct proc *p, unsigned short nr);


struct module_callback xaaes_cb_vector =
{
	xaaes_share,
	xaaes_release,

	xaaes_on_exit,
	xaaes_on_exec,
	xaaes_on_fork,
	xaaes_on_stop,
	xaaes_on_signal,
};


static void _cdecl
xaaes_share(void *_client)
{
	DIAGS(("xaaes_share: %lx", _client));
}

static void _cdecl
xaaes_release(void *_client)
{
	DIAGS(("xaaes_release: %lx", _client));
}


/*
 * called on process termination
 */
static void _cdecl
xaaes_on_exit(void *_client, struct proc *p, int code)
{
	struct xa_client *client = _client;

	if (client->p->pid == p->pid)
	{
		enum locks lock = NOLOCKS;

		DIAGS(("xaaes_on_exit event for %u (%i)", p->pid, code));
		exit_client(lock, _client, code);
	}
	else
		DIAGS(("xaaes_on_exit - thread terminate"));
}

/*
 * called on exec syscall
 */
static void _cdecl
xaaes_on_exec(void *_client, struct proc *p)
{
}

/*
 * called if the process context is forked
 */
static void _cdecl
xaaes_on_fork(void *_client, struct proc *p, long flags, struct proc *child)
{
}

/*
 * called if the process is going to be stopped due to a signal
 */
static void _cdecl
xaaes_on_stop(void *_client, struct proc *p, unsigned short nr)
{
}

/*
 * called if the process receive an signal (before the signal is dispatched)
 */
static void _cdecl
xaaes_on_signal(void *_client, struct proc *p, unsigned short nr)
{
}
