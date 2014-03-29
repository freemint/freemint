/*
 * $Id$
 *
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
 * begin:	2000-04-18
 * last change: 1998-04-18
 *
 * Author: Frank Naumann <fnaumann@freemint.de>
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 */

# ifndef _mint_config_h
# define _mint_config_h

/*
 * define if you want a daemon process for sync'ing (otherwise uses timeout)
 */
# define SYSUPDATE_DAEMON

/*
 * use GEMDOS FS instead of real FAT XFS
 */
//# undef OLDTOSFS

/*
 * include old style socket device emulation
 */
# define OLDSOCKDEVEMU

/*
 * include Linux style /kern pseudo filesystem
 * define to 0 to exclude
 */
# define WITH_KERNFS 1

/*
 * store file-names in file-struct.
 * define to 0 to exclude
 */
#if WITH_KERNFS
# ifndef STORE_FILENAMES
#  define STORE_FILENAMES 1
# endif
#endif

/*
 * activating non blocking xfs extension
 * highly alpha at the moment!!!
 */
# undef NONBLOCKING_DMA

/*
 * add /dev/random and /dev/urandom
 */

#ifdef NO_DEV_RANDOM
# undef DEV_RANDOM
#else
# define DEV_RANDOM
#endif

/*
 * MAXPID is the maxium PID MiNT will generate
 *
 * ATTENTION: Do not change at the moment!
 */
# define MAXPID 1000

/*
 * PATH_MAX is the maximum path allowed. The kernel uses this in lots of
 * places, so there isn't much point in file systems allowing longer
 * paths (they can restrict paths to being shorter if they want).
 * (This is slowly changing, actually... fewer and fewer places use
 *  PATH_MAX, and eventually we should get rid of it)
 */
# define PATH_MAX 255

/*
 * maximum length of a string passed to ksprintf_old
 */
# define SPRINTF_MAX 128


# endif /* _mint_config_h */
