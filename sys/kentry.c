/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
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
 */

# include "kentry.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "mint/arch/mfp.h"
# include "arch/cpu.h"
# include "arch/syscall.h"

# include "block_IO.h"		/* bio */
# include "cookie.h"		/* add_rsvfentry, del_rsvfentry, get_toscookie */
# include "cnf.h"
# include "delay.h"		/* loops_per_sec */
# include "dma.h"		/* dma */
# include "filesys.h"		/* changedrv, denyshare, denylock */
# include "global.h"		/* global */
# include "ipc_socketutil.h"	/* so_* */
# include "k_exec.h"		/* create_process */
# include "k_kthread.h"		/* kthread_create, kthread_exit */
# include "k_prot.h"		/* proc_setuid/proc_setgid */
# include "kmemory.h"		/* kmalloc, kfree */
# include "memory.h"		/* addr2mem, attach_region, detach_region */
# include "module.h"		/* load_modules */
# include "pcibios.h"
# include "proc.h"		/* sleep, wake, wakeselect, iwake */
# include "proc_help.h"		/* proc_extensions */
# include "proc_wakeup.h"	/* addprocwakeup */
# include "semaphores.h"	/* semaphore_* */
# include "signal.h"		/* ikill */
# include "syscall_vectors.h"	/* bios_tab, dos_tab */
# include "time.h"		/* xtime */
# include "timeout.h"		/* nap, addtimeout, canceltimeout, addroottimeout, cancelroottimeout */
# include "umemory.h"		/* umalloc, ufree */
# include "util.h"		/* pid2proc */
# include "xfs_xdd.h"
# include "xhdi.h"


# undef DEFAULT_MODE
# undef DEFAULT_DIRMODE

# define DEFAULT_MODE		(0644)
# define DEFAULT_DIRMODE	(0755)


struct kentry kentry = DEFAULTS_kentry;
