/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 1998-04-22
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * This file defines the pun_info structure used in several kinds of hard disk
 * driver software. Basically it gives info for the first 16 bios drives.
 * Since it gives physical partition starts it's rather useful. Only snag is
 * the 'first 16 bios drives' bit.
 * 
 */

# ifndef _pun_h
# define _pun_h

# include "mint/mint.h"


typedef struct pun_info PUN_INFO;

struct pun_info
{	
	ushort		puns;			/* Number of HD's */
	uchar		pun [16];		/* AND with masks below: */
	
# define PUN_DEV	0x1f			/* device number of HD */
# define PUN_UNIT	0x7			/* Unit number */
# define PUN_SCSI	0x8			/* 1=SCSI 0=ACSI */
# define PUN_IDE	0x10			/* Falcon IDE */
# define PUN_REMOVABLE	0x40			/* Removable media */
# define PUN_VALID	0x80			/* zero if valid */
	
	long		partition_start [16];
	long		cookie;			/* 'AHDI' if following valid */
	long		*cookie_ptr;		/* Points to 'cookie' */
	ushort		version_num;		/* AHDI version */
	ushort		max_sect_siz;		/* Max logical sec size */
	long		reserved[16];		/* Reserved */	
};


/*
 * returns a pointer to the pun_info_struct
 *  or:
 * returns NULL if no AHDI 3.0 or higher is installed
 */

PUN_INFO * get_pun (void);


# endif /* _pun_h */
