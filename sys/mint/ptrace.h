/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2000-10-08
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_ptrace_h
# define _mint_ptrace_h

# include "kcompiler.h"


# define PT_TRACE_ME	0	/* child declares it's being traced */
# define PT_READ_I	1	/* read word in child's I space */
# define PT_READ_D	2	/* read word in child's D space */
# define PT_WRITE_I	4	/* write word in child's I space */
# define PT_WRITE_D	5	/* write word in child's D space */
# define PT_CONTINUE	7	/* continue the child */
# define PT_KILL	8	/* kill the child process */
# define PT_ATTACH	9	/* attach to running process */
# define PT_DETACH	10	/* detach from running process */
# define PT_SYSCALL	11	/* continue and stop at next return from syscall */

# define PT_BASEPAGE	999

# define PT_FIRSTMACH	32	/* for machine-specific requests */
# include "arch/ptrace.h"


# ifndef __KERNEL__
# ifdef __SYSCALL__
# include "syscall-bind.h
# define Ptrace(SYS_ptrace, pid, arg, data) \
	trap_1_wwwll (XXX, request, pid, arg, data)
# endif
# endif


# endif /* _mint_ptrace_h */
