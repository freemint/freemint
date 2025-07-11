/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2001 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-04-24
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 */

# include "kerinfo.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "block_IO.h"		/* bio */
# include "cookie.h"		/* add_rsvfentry, del_rsvfentry, get_toscookie */
# include "delay.h"		/* loops_per_sec */
# include "dma.h"		/* dma */
# include "filesys.h"		/* changedrv, denyshare, denylock */
# include "ipc_socketutil.h"	/* so_* */
# include "k_kthread.h"		/* kthread_create, kthread_exit */
# include "kmemory.h"		/* kmalloc, kfree */
# include "module.h"		/* load_modules */
# include "proc.h"		/* sleep, wake, wakeselect, iwake */
# include "signal.h"		/* ikill */
# include "syscall_vectors.h"	/* bios_tab, dos_tab */
# include "time.h"		/* xtime */
# include "timeout.h"		/* nap, addtimeout, canceltimeout, addroottimeout, cancelroottimeout */
# include "umemory.h"		/* umalloc, ufree */


# undef DEFAULT_MODE
# define DEFAULT_MODE	(0644)

/* wrapper for the kerinterface */

static void   _cdecl m_changedrv (ushort drv)  { return _changedrv (drv, "xfs/xdd"); }
static void * _cdecl m_kmalloc   (ulong size)  { return _kmalloc (size, "xfs/xdd"); }
static void   _cdecl m_kfree     (void *place) { _kfree (place, "xfs/xdd"); }
static void * _cdecl m_umalloc   (ulong size)  { return _umalloc (size, "xfs/xdd"); }
static void   _cdecl m_ufree     (void *place) { _ufree (place, "xfs/xdd"); }

#ifdef NONBLOACKING_DMA
static void * _cdecl m_dmabuf_alloc(ulong size, short cm)
{ return _dmabuf_alloc (size, cm, "xfs/xdd"); }
#endif

static long _cdecl
old_kthread_create(void _cdecl (*func)(void *), void *arg,
		   struct proc **np, const char *fmt, ...)
{
	va_list args;
	long r;

	va_start(args, fmt);
	r = kthread_create_v(NULL, func, arg, np, fmt, args);
	va_end(args);

	return r;
}

/*
 * kernel info that is passed to loaded file systems and device drivers
 */
struct kerinfo kernelinfo =
{
	MINT_MAJ_VERSION,
	MINT_MIN_VERSION,
	DEFAULT_MODE,
	2, /* MINT_KVERSION */
	&bios_tab, &dos_tab,
	m_changedrv,
	Trace, Debug, ALERT, FATAL,
	m_kmalloc,
	m_kfree,
	m_umalloc,
	m_ufree,
	_mint_o_strnicmp,
	_mint_o_stricmp,
	_mint_strlwr,
	_mint_strupr,
	ksprintf_old,
	ms_time, unixtime, dostime,
	nap, sleep, wake, (void _cdecl (*)(long)) wakeselect,
	denyshare, denylock,
	addtimeout_curproc, canceltimeout,
	addroottimeout, cancelroottimeout,
	ikill, iwake,
	&bio,
	&xtime,		/* version 1 extension */
	0,

	/* version 2
	 */

	add_rsvfentry,
	del_rsvfentry,
	killgroup,
	&dma,
	&loops_per_sec,
	get_toscookie,

	so_register,
	so_unregister,
	so_release,
	so_sockpair,
	so_connect,
	so_accept,
	so_create,
	so_dup,
	so_free,

	load_modules_old,
	old_kthread_create,
	kthread_exit,

	NULL, /* m_dmabuf_alloc, */
	NULL, /* nf_ops */

	remaining_proc_time,

	{
		0
	}
};
