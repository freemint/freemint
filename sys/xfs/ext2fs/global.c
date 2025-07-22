/*
 * Filename:     global.c
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include "global.h"


SI *super [NUM_DRIVES];

ulong event = 0;
#ifdef EXT2FS_DEBUG
ulong memory = 0;
#endif

ushort native_utc = 0;
