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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-10-30
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_credentials_h
# define _mint_credentials_h


# define NGROUPS_MAX	8
# define NGROUPS	NGROUPS_MAX

# ifdef __KERNEL__

struct ucred
{
	unsigned short	euid;			/* effective user id */
	unsigned short	egid;			/* effective group id */
	unsigned short	groups [NGROUPS_MAX];	/* groups */
	unsigned short	ngroups;		/* number of groups */
	
	short		links;			/* number of references */
};


struct pcred
{
	struct ucred	*ucr;			/*  */
	unsigned short	ruid;			/* real user id */
	unsigned short	rgid;			/* real group id */
	unsigned short	suid;			/* saved effective user id */
	unsigned short	sgid;			/* saved effective group id */
	
	short		links;			/* number of references */
	short		pad;
};

# endif

# endif /* _mint_credentials_h */
