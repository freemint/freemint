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
	long (*share)(void *);
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
	short links;		/* number of references */
	short pad;		/* padding */

	void *data;		/* module data (private kernel memory),
				 * allocation managed by the kernel */
	struct proc_ext *next;	/* extensions are chained */

	/* module callback vector */
	struct module_callback *cb_vector;

	long reserved[6];	/* sizeof() => 44 bytes */
};

# endif /* _mint_proc_extensions_h */
