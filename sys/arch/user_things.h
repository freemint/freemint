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

typedef struct
{
	const long	magic;
	const long	len;
	BASEPAGE	*bp;		/* user basepage ptr, internal use */
	const long	jmpno;

	long terminateme_p;
	long sig_return_p;
	long pc_valid_return_p;
	long slb_init_and_exit_p;
	long slb_open_p;
	long slb_close_p;
	long slb_close_and_pterm_p;
	long slb_exec_p;
} USER_THINGS;

extern USER_THINGS user_things;
extern USER_THINGS kernel_things;

# endif /* _m68k_user_things_h */
