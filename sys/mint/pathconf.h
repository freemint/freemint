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
 * Started: 2001-03-01
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_pathconf_h
# define _mint_pathconf_h


/* The requests for Dpathconf() */

# define DP_INQUIRE	-1
# define DP_IOPEN	0		/* internal limit on # of open files */
# define DP_MAXLINKS	1		/* max number of hard links to a file */
# define DP_PATHMAX	2		/* max path name length */
# define DP_NAMEMAX	3		/* max length of an individual file name */
# define DP_ATOMIC	4		/* # of bytes that can be written atomically */

# define DP_TRUNC	5		/* file name truncation behavior */
#  define DP_NOTRUNC	0		/* long filenames give an error */
#  define DP_AUTOTRUNC	1		/* long filenames truncated */
#  define DP_DOSTRUNC	2		/* DOS truncation rules in effect */

# define DP_CASE	6
#  define DP_CASESENS	0		/* case sensitive */
#  define DP_CASECONV	1		/* case always converted */
#  define DP_CASEINSENS	2		/* case insensitive, preserved */

# define DP_MODEATTR	7
#  define DP_ATTRBITS	0x000000ffL	/* mask for valid TOS attribs */
#  define DP_MODEBITS	0x000fff00L	/* mask for valid Unix file modes */
#  define DP_FILETYPS	0xfff00000L	/* mask for valid file types */
#  define DP_FT_DIR	0x00100000L	/* directories (always if . is there) */
#  define DP_FT_CHR	0x00200000L	/* character special files */
#  define DP_FT_BLK	0x00400000L	/* block special files, currently unused */
#  define DP_FT_REG	0x00800000L	/* regular files */
#  define DP_FT_LNK	0x01000000L	/* symbolic links */
#  define DP_FT_SOCK	0x02000000L	/* sockets, currently unused */
#  define DP_FT_FIFO	0x04000000L	/* pipes */
#  define DP_FT_MEM	0x08000000L	/* shared memory or proc files */

# define DP_XATTRFIELDS	8
#  define DP_INDEX	0x0001
#  define DP_DEV	0x0002
#  define DP_RDEV	0x0004
#  define DP_NLINK	0x0008
#  define DP_UID	0x0010
#  define DP_GID	0x0020
#  define DP_BLKSIZE	0x0040
#  define DP_SIZE	0x0080
#  define DP_NBLOCKS	0x0100
#  define DP_ATIME	0x0200
#  define DP_CTIME	0x0400
#  define DP_MTIME	0x0800

# define DP_VOLNAMEMAX	9		/* max length of a volume name
					 * (0 if volume names not supported)
					 */

/* Dpathconf and Sysconf return this when a value is not limited
 * (or is limited only by available memory)
 */
# define UNLIMITED	0x7fffffffL


# endif /* _mint_pathconf_h */
