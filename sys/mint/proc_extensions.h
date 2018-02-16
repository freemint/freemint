/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
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

# ifndef _mint_proc_extensions_h
# define _mint_proc_extensions_h

# include "ktypes.h"

#define PEXT_NOSHARE		1	/* Never share this control block */
#define PEXT_COPYONSHARE	2	/* Make a new copy when sharing this block */
#define PEXT_SHAREONCE		4	/* Setting this will set PEXT_NOSHARE when PEXT_COPYONSHARE is set */
/*
 * module callback vector
 *
 * These are the entry points into a module to notify a module
 * about important process lifetime events.
 *
 * The function pointers can be NULL. In this case no callback
 * is performed :-)
 */
struct module_callback
{
	void (*share)(void *, struct proc *, struct proc *);
	void (*release)(void *);

	void (*on_exit  )(void *, struct proc *, int);
	void (*on_exec  )(void *, struct proc *);
	void (*on_fork  )(void *, struct proc *, long, struct proc *);
	void (*on_stop  )(void *, struct proc *, unsigned short);
	void (*on_signal)(void *, struct proc *, unsigned short);
};

/*
 * process control block extensions for modules
 *
 * This is a way to allow modules to attach process related control data
 * to the process structure. Together with the callback vector a module
 * can enhance the base functionality of the kernel.
 */
struct proc_ext
{
	long ident;		/* module identification */
	unsigned long flags;	/* flags */
	unsigned long size;	/* size */
	short links;		/* number of references */
	short pad;		/* padding */

	void *data;		/* module data (private kernel memory),
				 * allocation managed by the kernel */

	/* module callback vector */
	struct module_callback *cb_vector;

	long reserved[4];	/* sizeof() => 44 bytes */
};

struct p_ext
{
	unsigned short size;
	unsigned short used;
	struct proc_ext *ext[10];
};

# endif /* _mint_proc_extensions_h */
