/*
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
 * begin:	2000-10-30
 * last change:	2000-10-30
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_credentials_h
# define _mint_credentials_h

# include "ktypes.h"


# define NGROUPS	8

struct ucred
{
	short		euid;			/* effective user id */
	short		egid;			/* effective group id */
	short		groups [NGROUPS];	/* groups */
	ushort		ngroups;		/* number of groups */
	ushort		pad;
	
	long		links;			/* number of references */
};


struct pcred
{
	struct ucred	*ucr;			/*  */
	short		ruid;			/* real user id */
	short		rgid;			/* real group id */
	short		suid;			/* saved effective user id */
	short		sgid;			/* saved effective group id */
	
	long		links;			/* number of references */
};


# endif /* _mint_credentials_h */
