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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-04-30
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _k_sysctl_h
# define _k_sysctl_h

# include "mint/mint.h"

struct proc;


long _cdecl sys_p_sysctl (long *, ulong, void *, ulong *, const void *, ulong);

/*
 * Internal sysctl function calling convention:
 *
 *	(*sysctlfn)(name, namelen, oldval, oldlenp, newval, newlen);
 *
 * The name parameter points at the next component of the name to be
 * interpreted.  The namelen parameter is the number of integers in
 * the name.
 */
typedef long (sysctlfn)(long *, ulong, void *, ulong *, const void *, ulong, struct proc *);

long sysctl_long (void *, ulong *, const void *, ulong, long *);
long sysctl_rdlong (void *, ulong *, const void *, long);
long sysctl_quad (void *, ulong *, const void *, ulong, llong *);
long sysctl_rdquad (void *, ulong *, const void *, llong);
long sysctl_string (void *, ulong *, const void *, ulong, char *, long);
long sysctl_rdstring (void *, ulong *, const void *, const char *);
long sysctl_struct (void *, ulong *, const void *, ulong, void *, long);
long sysctl_rdstruct (void *, ulong *, const void *, const void *, long);
long sysctl_rdminstruct (void *, ulong *, const void *, const void *, long);


# endif /* _k_sysctl_h */
