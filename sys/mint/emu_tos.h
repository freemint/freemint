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
 * begin:	2000-11-09
 * last change:	2000-11-09
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_emu_tos_h
# define _mint_rmu_tos_h

# include "ktypes.h"


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
	char	dta_name [TOS_NAMELEN+1];
};


/* structure used to hold i/o buffers */
typedef struct io_rec
{
	char *bufaddr;
	short buflen;
	volatile short head;
	volatile short tail;
	short low_water;
	short hi_water;
	
} IOREC_T;

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
