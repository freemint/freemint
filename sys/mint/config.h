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
 * undefine VM_EXTENSION if CPU030 is not defined
 */
# ifndef CPU030
# undef VM_EXTENSION
# endif

/*
 * make real processor exceptions (bus error, etc.) raise a signal
 */
# define EXCEPTION_SIGS

/*
 * Fcreate() on pipes should not fail
 */
# define CREATE_PIPES

/*
 * try to gather the MiNT executable's name from the parent's DTA
 */
# define AUTO_FIX

/*
 * define if you want a daemon process for sync'ing (otherwise uses timeout)
 */
# define SYSUPDATE_DAEMON

/*
 * use GEMDOS FS instead of real FAT XFS
 */
# undef OLDTOSFS

/*
 * include Linux style /kern pseudo filesystem
 * define to 0 to exclude
 */
# define WITH_KERNFS 1

/*
 * activating non blocking xfs extension
 * highly alpha at the moment!!!
 */
# undef NONBLOCKING_DMA

/*
 * add /dev/random and /dev/urandom
 */
# define DEV_RANDOM

/*
 * other options best set in the makefile
 */
# if 0
# define MULTITOS	/* make a MultiTOS kernel */
# define ONLY030	/* make a 68030 only version */
# endif

/*
 * Ensure that MMU040 is always set together with ONLY030. This must be an
 * #error, as we can't influence the defines passed to asmtrans from here.
 */
# ifdef MMU040
# ifndef ONLY030
# error MMU040 requires ONLY030
# endif
# endif

# ifndef MULTITOS
# define FASTTEXT	/* include the u:\dev\fasttext device */
# endif

/* MAXPID is the maxium PID MiNT will generate */
# define MAXPID 1000

/* PATH_MAX is the maximum path allowed. The kernel uses this in lots of
 * places, so there isn't much point in file systems allowing longer
 * paths (they can restrict paths to being shorter if they want).
 * (This is slowly changing, actually... fewer and fewer places use
 *  PATH_MAX, and eventually we should get rid of it)
 */
# define PATH_MAX 128

/* maximum length of a string passed to ksprintf_old
 */
# define SPRINTF_MAX 128

/* NOTE: NAME_MAX is a "suggested" maximum name length only. Individual
 * file systems may choose a longer or shorter NAME_MAX, so do _not_
 * use this in the kernel for anything!
 */
# define NAME_MAX 14


# endif /* _mint_config_h */
