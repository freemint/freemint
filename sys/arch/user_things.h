/*
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * Copyright 2003 Konrad M. Kokoszkiewicz <draco@atari.org>
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
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 1999-08-17
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _m68k_user_things_h
# define _m68k_user_things_h

# include "mint/mint.h"
# include "mint/slb.h"

extern long user_header[];

void terminateme(int code);

void sig_return (void);
void *pc_valid_return;

long slb_open (void);
long slb_close (void);
long slb_close_and_pterm (void);
void _cdecl slb_init_and_exit(BASEPAGE *b);

long _cdecl slb_exec (SHARED_LIB *sl, long fn, short nargs, ...);

# endif /* _m68k_user_things_h */
