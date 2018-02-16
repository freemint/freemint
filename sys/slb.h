/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000 Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
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
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * Started: 1999-08-17
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * Purpose:
 * Prototypes necessary for MagiC-style "share libraries".
 * 
 * History:
 * 99/07/04-
 * 99/08/17: - Creation, with pauses (Gryf)
 * 
 */

# ifndef _slb_h
# define _slb_h

# include "mint/mint.h"
# include "mint/slb.h"

long _cdecl sys_s_lbopen (char *name, char *path, long min_ver, SHARED_LIB **sl, SLB_EXEC *fn);
long _cdecl sys_s_lbclose (SHARED_LIB *sl);
int slb_close_on_exit (int term);
void remove_slb (void);

# endif /* _slb_h */
