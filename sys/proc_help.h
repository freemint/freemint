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

# ifndef _proc_help_h
# define _proc_help_h

# include "mint/mint.h"

# include "mint/filedesc.h"
# include "mint/proc.h"
# include "mint/signal.h"


/* p_mem */

void init_page_table_ptr (struct memspace *m);

# define hold_mem(mem) \
		(mem)->links++

# define share_mem(p) \
		({ hold_mem((p)->p_mem); (p)->p_mem; })

struct memspace *copy_mem (struct proc *p);
void free_mem (struct proc *p);

long increase_mem(struct memspace *mem);


/* p_fd */

# define hold_fd(fd) \
		(fd)->links++

# define share_fd(p) \
		({ hold_fd((p)->p_fd); (p)->p_fd; })

struct filedesc *copy_fd (struct proc *p);
void free_fd (struct proc *p);


/* p_cwd */

# define hold_cwd(cwd) \
		(cwd)->links++

# define share_cwd(p) \
		({ hold_cwd((p)->p_cwd); (p)->p_cwd; })

struct cwd *copy_cwd (struct proc *p);
void free_cwd (struct proc *p);


/* p_sigacts */

# define hold_sigacts(sigacts) \
		(sigacts)->links++

# define share_sigacts(p) \
		({ hold_sigacts((p)->p_sigacts); (p)->p_sigacts; })

struct sigacts *copy_sigacts (struct proc *p);
void free_sigacts (struct proc *p);


/* p_limits */

struct plimit *copy_limits (struct proc *p);
void free_limits (struct proc *p);


/* P_ext */

struct p_ext *share_ext(struct proc *p1, struct proc *p2);
void free_ext(struct proc *p);

void *_cdecl proc_lookup_extension(struct proc *, long ident);
void *_cdecl proc_attach_extension(struct proc *, long ident, unsigned long flags, unsigned long size, struct module_callback *);
void  _cdecl proc_detach_extension(struct proc *, long ident);

void proc_ext_on_exit(struct proc *p, int code);
void proc_ext_on_exec(struct proc *p);
void proc_ext_on_fork(struct proc *p, long flags, struct proc *child);
void proc_ext_on_stop(struct proc *p, unsigned short nr);
void proc_ext_on_signal(struct proc *p, unsigned short nr);

# endif /* _proc_help_h */
