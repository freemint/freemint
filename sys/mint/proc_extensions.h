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
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_proc_extensions_h
# define _mint_proc_extensions_h

# include "ktypes.h"


struct proc_ext
{
	void	*data;		/* allocation managed by the kernel */

	short	links;		/* number of references */
	short	pad;		/* unused */

	long	(*share)(struct proc_ext *);
	long	(*release)(struct proc_ext *);

	long	(*on_exit  )(struct proc_ext *, struct proc *);
	long	(*on_exec  )(struct proc_ext *, struct proc *);
	long	(*on_fork  )(struct proc_ext *, long, struct proc *, struct proc *);
	long	(*on_signal)(struct proc_ext *, unsigned short, struct proc *);

	long	reserved[3];	/* sizeof() => 44 bytes */
};

# endif /* _mint_proc_extensions_h */
