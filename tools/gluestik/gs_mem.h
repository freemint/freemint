/*
 * Filename:     gs_mem.h
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@freemint.de>
 * 
 * Portions copyright 1997, 1998, 1999 Scott Bigham <dsb@cs.duke.edu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _gs_mem_h
# define _gs_mem_h

# include "gs.h"


int	init_mem	(void);
void	cleanup_mem	(void);

void *	gs_mem_alloc	(ulong);
void	gs_mem_free	(void *);
int32	gs_mem_getfree	(int16);
void *	gs_mem_realloc	(void *, ulong);


# endif /* _gs_mem_h */
