/*
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
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "global.h"
# include "version.h"


ushort native_utc = 0;

UNIT error;

SI *super_ptr[NUM_DRIVES];
FILEPTR *firstptr;
struct kerinfo *kernel;

long mfs_magic = MFS_MAGIC;

int mfs_maj = MFS_MAJOR;
int mfs_min = MFS_MINOR;
int mfs_plev = 0;

long fs_mode[NUM_DRIVES] =
{
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
	TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT, TRANS_DEFAULT,
};
