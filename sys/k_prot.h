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

# ifndef _k_prot_h
# define _k_prot_h

# include "mint/mint.h"
# include "mint/credentials.h"


long _cdecl sys_pgetuid		(void);
long _cdecl sys_pgetgid		(void);
long _cdecl sys_pgeteuid	(void);
long _cdecl sys_pgetegid	(void);
long _cdecl sys_psetuid		(int id);
long _cdecl sys_psetgid		(int id);
long _cdecl sys_pseteuid	(int id);
long _cdecl sys_psetegid	(int id);
long _cdecl sys_psetreuid	(int rid, int eid);
long _cdecl sys_psetregid	(int rid, int eid);
long _cdecl sys_pgetgroups	(int gidsetlen, int gidset[]);
long _cdecl sys_psetgroups	(int ngroups, int gidset[]);

int groupmember (struct ucred *ucr, ushort group);

INLINE int
suser (struct ucred *cred)
{
	if (cred->euid == 0)
		return 1;
	
	return 0;
}

/* p_cred */

# define hold_cred(cred) \
		(cred)->links++

struct ucred *copy_cred (struct ucred *ucr);
void free_cred (struct ucred *ucr);


# endif /* _k_prot_h */
