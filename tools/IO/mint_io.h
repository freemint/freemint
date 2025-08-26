/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 * Started: 200-06-14
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_io_h
# define _mint_io_h


# define BLKGETSIZE		(('b'<< 8) | 1)
# define BLOCKSIZE		(('b'<< 8) | 2)

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)


# endif /* _mint_io_h */
