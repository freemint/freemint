/*
 * $Id$
 * 
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

# ifndef _m68k_slb_util_h
# define _m68k_slb_util_h

# include "mint/mint.h"


long slb_open (void);
long slb_close (void);
long slb_close_and_pterm (void);
long _cdecl slb_exec (SHARED_LIB *sl, long fn, short nargs, ...);
long _cdecl slb_fast (SHARED_LIB *sl, long fn, short nargs, ...);
long _cdecl P_kill (short, short);
long _cdecl P_setpgrp (short, short);
long _cdecl P_sigsetmask (long);
long _cdecl P_domain (short);
char * _cdecl getslbpath(BASEPAGE *base);


# endif /* _m68k_slb_util_h */
