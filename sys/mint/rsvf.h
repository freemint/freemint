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
 * Started: 1999-07-29
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *  
 */

# ifndef _rsvf_h
# define _rsvf_h


typedef struct rsvf RSVF;
struct rsvf
{
	char	*data;		/* name or next pointer */
	char	type;		/* type specifies the data pointer */
# define RSVF_PORT	0x80	/* if set, RSFV is a port object */
# define RSVF_GEMDOS	0x40	/* port: GEMDOS know about it */
# define RSVF_BIOS	0x20	/* port: BIOS know about it */
# define RSVF_FTABLE	0x01	/* port: ptr to function table is before data */
	char	res1;		/* reserved */
	char	bdev;		/* BIOS device number */
	char	res2;		/* reserved */
};


# endif /* _rsvf_h */
