/*
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
 * Started: 2000-11-07
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _k_fork_h
# define _k_fork_h

# include "mint/mint.h"


/* flags for internal fork */
# define FORK_SHAREVM		0x01	/* share vmspace with parent */
# define FORK_SHARECWD		0x02	/* share cdir/rdir/cmask */
# define FORK_SHAREFILES	0x04	/* share file descriptors */
# define FORK_SHARESIGS		0x08	/* share signal actions */
# define FORK_SHARELIMITS	0x10	/* share process limits */
# define FORK_SHAREEXT		0x20	/* share proc extensions */

struct proc *	fork_proc1	(struct proc *p1, long flags, long *err);
struct proc *	fork_proc	(long flags, long *err);

long _cdecl	sys_pvfork	(void);
long _cdecl	sys_pfork	(void);


# endif /* _k_fork_h */
