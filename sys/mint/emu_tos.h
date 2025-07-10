/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _mint_emu_tos_h
# define _mint_emu_tos_h

# ifdef __KERNEL__
# include "ktypes.h"
# else
# include <sys/types.h>
# endif


# define TOS_NAMELEN 13

struct dtabuf
{
	ushort	index;		/* index into arrays in the PROC struct */
	long	magic;
# define SVALID	0x1234fedcL	/* magic for a valid search */
# define EVALID	0x5678ba90L	/* magic for an exhausted search */
	
	char	dta_pat[TOS_NAMELEN+1]; /* pointer to pattern, if necessary */
	char	dta_sattrib;	/* attributes being searched for */
	
	/* this stuff is returned to the user */
	char	dta_attrib;
	ushort	dta_time;
	ushort	dta_date;
	ulong	dta_size;
	char	dta_name[TOS_NAMELEN+1];
};

/* defines for TOS attribute bytes */
# define FA_RDONLY	0x01
# define FA_HIDDEN	0x02
# define FA_SYSTEM	0x04
# define FA_LABEL	0x08
# define FA_DIR		0x10
# define FA_CHANGED	0x20
# ifdef __KERNEL__
# define FA_VFAT	0x0f	/* VFAT entry */
# define FA_SYMLINK	0x40	/* symbolic link */
# endif

/* Codes used with Cursconf() */
# define CURS_HIDE	0
# define CURS_SHOW	1
# define CURS_BLINK	2
# define CURS_NOBLINK	3
# define CURS_SETRATE	4
# define CURS_GETRATE	5

#include "struct_iorec.h"
#include "struct_kbdvbase.h"

/* Bconmap struct, * returned by Bconmap (-2) */
typedef struct
{
	struct
	{
		long bconstat;
		long bconin;
		long bcostat;
		long bconout;
		long rsconf;
		IOREC_T	*iorec;
		
	} *maptab;
	short	maptabsize;
	
} BCONMAP2_T;

# endif /* _mint_emu_tos_h */
