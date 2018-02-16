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

#include "sys_proc.h"
#include "xa_global.h"

#include "xa_appl.h"
#include "semaphores.h"


static void _cdecl xaaes_share(void *, struct proc *, struct proc *);
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
xaaes_share(void *_client, struct proc *from, struct proc *to)
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
	extern int ferr;

	if (!ferr && client->p->pid == p->pid)
	{
		enum locks lock = NOLOCKS;

		DIAGS(("xaaes_on_exit event for %u (%i)", p->pid, code));
		exit_client(lock, _client, code, true, false);
	}
	else
	{
		DIAGS(("xaaes_on_exit - thread terminate"));
	}
}

/*
 * called on exec syscall
 */
static void _cdecl
xaaes_on_exec(void *_client, struct proc *p)
{
	DIAGS(("xaaes_on_exec for %u", p->pid));
}

/*
 * called if the process context is forked
 */
static void _cdecl
xaaes_on_fork(void *_client, struct proc *p, long flags, struct proc *child)
{
	DIAGS(("xaaes_on_fork for %u, child %u", p->pid, child->pid));
}

/*
 * called if the process is going to be stopped due to a signal
 */
static void _cdecl
xaaes_on_stop(void *_client, struct proc *p, unsigned short nr)
{
	DIAGS(("xaaes_on_stop for %u (%u)", p->pid, nr));
}

/*
 * called if the process receive an signal (before the signal is dispatched)
 */
static void _cdecl
xaaes_on_signal(void *_client, struct proc *p, unsigned short nr)
{
	DIAGS(("xaaes_on_signal for %u (signal %u)", p->pid, nr));
}

static void _cdecl info_share(void *, struct proc *, struct proc *);
static void _cdecl info_release(void *);

static void _cdecl info_on_exit(void *, struct proc *p, int code);
static void _cdecl info_on_exec(void *, struct proc *p);
static void _cdecl info_on_fork(void *, struct proc *p, long flags, struct proc *child);
static void _cdecl info_on_stop(void *, struct proc *p, unsigned short nr);
static void _cdecl info_on_signal(void *, struct proc *p, unsigned short nr);

struct module_callback info_cb = //xaaes_cb_vector_sh_info =
{
	info_share,		/* share */
	info_release,		/* release */

	info_on_exit, 	//NULL,	/* on_exit */
	info_on_exec,		/* on_exec */
	info_on_fork,		/* on_fork */
	info_on_stop,		/* on_stop */
	info_on_signal,		/* on_signal */
};

static void _cdecl
info_share(void *_info, struct proc *from, struct proc *to)
{
	struct shel_info *info = _info;
	struct xa_client *client = lookup_extension(from, XAAES_MAGIC);

	if (client)
	{
		if (info->tail_is_heap)
		{
			info->cmd_tail = NULL;
			info->tail_is_heap = false;
		}
		info->rppid = from->pid;
		info->shel_write = false;
	}
// 	display("info_share %lx", _info);
}


#define MAX_CMDLEN 65535	/* ?? */
static void _cdecl
info_release(void *_info)
{
	struct shel_info *info = _info;
	struct proc *p = get_curproc();

	DIAGS(("info_release: releasing 0x%lx", info));
// 	display("info_release: releasing 0x%lx", info);

	if (info->tail_is_heap && info->cmd_tail && !(p->p_flag & P_FLAG_SLB << 8))
	{
		unsigned long *x = (unsigned long *)info->cmd_tail;

		//struct proc *ip = pid2proc(info->reserved);

		/* Problem: The kernel duplicates *info including cmt_tail, which is malloced,
			and releases the copy and the original, so cmd_tail is freed twice.
			TRY to check this.
	 */

		if( !(*x == 0x7f || *(x-1) < 130 ) || *(x-1) > MAX_CMDLEN || !p || p->pid != info->reserved )
		{
			BLOG((0,"info_release:kfree(%lx)->%lx:%ld): pid=%d:%d: error", x, *x, *(x-1), info->reserved, p ? p->pid : -1 ));
			return;
		}
		kfree(info->cmd_tail);
	}
}

static void _cdecl
info_on_exit(void *_info, struct proc *p, int nr)
{
 	struct xa_client *c = lookup_extension(p, XAAES_MAGIC);

// 	display("info_on_exit: nr=%d, c=%lx, %s", nr, c, p ? p->name : "What?");
	DIAGS(("info_on_exit for %u (signal %u)", p->pid, nr));

	/* Ozk:
	 * If process got client structure, we do not call exit_proc() here,
	 * else CH_EXIT wont get sent correctly
	 */
	if (!c)
	{
		exit_proc(0, p, nr);
	}
}

static void _cdecl
info_on_exec(void *_info, struct proc *p)
{
// 	display("info_on_exec: (info = %lx) %d", _info, p->pid);
}

static void _cdecl
info_on_fork(void *_info, struct proc *p, long flags, struct proc *child)
{
// 	display("info_on_fork: (info = %lx) %d forks to %d (new = %lx)", _info, p->pid, child->pid, new);
}

static void _cdecl
info_on_stop(void *_info, struct proc *p, unsigned short nr)
{
// 	display("info_on_stop: (info = %lx) pid: %d", _info, p->pid);
}
static void _cdecl
info_on_signal(void *_info, struct proc *p, unsigned short nr)
{
// 	display("info_on_signal: (info = %lx) pid: %d", _info, p->pid);
}
