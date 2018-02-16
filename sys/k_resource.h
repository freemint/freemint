/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998 Guido Flohr <guido@freemint.de>
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

# ifndef _k_resource_h
# define _k_resource_h

# include "mint/mint.h"

/* Return the highest priority of any process specified by WHICH and WHO
 * (see above); if WHO is zero, the current process, process group, or user
 * (as specified by WHO) is used. A lower priority number means higher
 * priority. Priorities range from PRIO_MIN to PRIO_MAX (above).
 */
long _cdecl sys_pgetpriority(short which, short who);

/* Set the priority of all processes specified by WHICH and WHO (see above)
 * to PRIO.  Returns 0 on success, -1 on errors.
 */
long _cdecl sys_psetpriority(short which, short who, short prio);

/* Provided for backward compatibility */
long _cdecl sys_prenice(short pid, short increment);
long _cdecl sys_pnice(short increment);

long _cdecl sys_prusage(long *r);
long _cdecl sys_psetlimit(short i, long v);

# endif	/* _k_resource_h  */
