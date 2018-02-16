/*
 * The Host OS filesystem access driver - filesystem definitions.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa of ARAnyM dev team.
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Originally taken from the STonX CVS repository.
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
 */

#ifndef _hostfs_xfs_h_
#define _hostfs_xfs_h_

# include "mint/mint.h"
# include "mint/file.h"

extern ulong    fs_drive_bits(void);
extern long     fs_native_init(int fs_devnum, char *mountpoint, char *hostroot,
				int halfsensitive, void *fs, void *fs_dev);

extern FILESYS hostfs_filesys;
extern FILESYS *hostfs_init(void);

#endif /* _hostfs_xfs_h_ */

