/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
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
 * Started: 2001-01-13
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _ipc_socketutil_h
# define _ipc_socketutil_h

# include "mint/mint.h"
# include "mint/net.h"


void domaininit (void);

void _cdecl so_register (short dom, struct dom_ops *ops);
void _cdecl so_unregister (short dom);

long _cdecl so_create (struct socket **, short, short, short);
long _cdecl so_dup (struct socket **, struct socket *);
void _cdecl so_free (struct socket *);
long _cdecl so_release (struct socket *);
void _cdecl so_sockpair (struct socket *, struct socket *);
long _cdecl so_connect (struct socket *, struct socket *, short, short, short);
long _cdecl so_accept (struct socket *, struct socket *, short);
void _cdecl so_drop (struct socket *, short);

static inline long so_rselect (struct socket *s, long proc);
static inline long so_wselect (struct socket *s, long proc);
static inline long so_xselect (struct socket *s, long proc);
static inline void so_wakersel (struct socket *s);
static inline void so_wakewsel (struct socket *s);
static inline void so_wakexsel (struct socket *s);


# include "proc.h"

/* so_rselect(), so_wselect(), so_wakersel(), so_wakewsel() handle
 * processes for selecting.
 */
static inline long
so_rselect (struct socket *so, long proc)
{
	if (so->rsel)
		return 2;
	
	so->rsel = proc;
	return 0;
}

static inline long
so_wselect (struct socket *so, long proc)
{
	if (so->wsel)
		return 2;
	
	so->wsel = proc;
	return 0;
}

static inline long
so_xselect (struct socket *so, long proc)
{
	if (so->xsel)
		return 2;
	
	so->xsel = proc;
	return 0;
}

static inline void
so_wakersel (struct socket *so)
{
	if (so->rsel)
		wakeselect ((struct proc *) so->rsel);
}

static inline void
so_wakewsel (struct socket *so)
{
	if (so->wsel)
		wakeselect ((struct proc *) so->wsel);
}

static inline void
so_wakexsel (struct socket *so)
{
	if (so->xsel)
		wakeselect ((struct proc *) so->xsel);
}


# endif	/* _ipc_socketutil_h  */
