/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2003 Odd Skancke <ozk@atari.org>
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
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 */

/*
 * Mouse device structures and definitions
 */

#ifndef _whlmoose_h
#define _whlmoose_h

struct moose_data
{
	unsigned short l;	/* record length */
	unsigned short ty;	/* button & movement */
	short x;
	short y;
	short state;
	short cstate;
	short clicks;
	short dbg1;
	short dbg2;
};

struct mooses_data
{
	short   state;
	short   x;
	short   y;
};

#define MOOSE_BUTTON_PREFIX	0x5842	/* 'XB' */
#define MOOSE_MOVEMENT_PREFIX	0x584d	/* 'XM' */
#define MOOSE_WHEEL_PREFIX	0x5857	/* 'XW' */

/*
 * Fcntl() opcodes
 * (move to mint/ioctl.h if finished)
 */
#define MOOSE_READVECS	(('M'<< 8) | 0)	/* buf is ptr to struct moose_vecsbuf */
#define MOOSE_DCLICK	(('M'<< 8) | 1)	/* buf is ptr to unsigned short */

typedef short vdi_vec(void *, ...);

/* structure for MOOSE_READVECS */
struct moose_vecsbuf
{
	vdi_vec *motv;
	vdi_vec *butv;
	vdi_vec *timv;
	vdi_vec *whlv;
};

#endif /* _whlmoose_h */
