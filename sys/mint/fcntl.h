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
 * begin:	2001-03-01
 * last change:	2001-03-01
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _mint_fcntl_h
# define _mint_fcntl_h


/* flags for open() modes */
# define O_RDONLY	0x00000000	/* open for reading only */
# define O_WRONLY	0x00000001	/* open for writing only */
# define O_RDWR		0x00000002	/* open for reading and writing */
# define O_ACCMODE	0x00000003	/* mask for above modes */
# ifdef __KERNEL__
# define O_RWMODE  	0x00000003	/* isolates file read/write mode */
# define O_EXEC		0x00000003	/* execute file; used by kernel only */
# endif

# define O_NOATIME	0x00000004	/* Do not set atime.  */
# define O_APPEND	0x00000008	/* all writes go to end of file */

/* file sharing modes (not POSIX) */
# define O_SHMODE	0x00000070	/* isolates file sharing mode */
#  define O_COMPAT	0x00000000	/* old TOS compatibility mode */
#  define O_DENYRW	0x00000010	/* deny both read and write access */
#  define O_DENYW	0x00000020	/* deny write access to others */
#  define O_DENYR	0x00000030	/* deny read access to others */
#  define O_DENYNONE	0x00000040	/* don't deny any access to others */

# define O_NOINHERIT	0x00000080	/* private file (not passed to child) */

# define O_NDELAY	0x00000100	/* don't block for I/O on this file */
# define O_CREAT	0x00000200	/* create new file if needed */
# define O_TRUNC	0x00000400	/* truncate file to 0 bytes if it does exist */
# define O_EXCL		0x00000800	/* error if file exists */

# define O_DIRECTORY	0x00010000	/* a directory */

# if 0
/* XXX missing */
# define O_SHLOCK	0x000000	/* open with shared file lock */
# define O_EXLOCK	0x000000	/* open with exclusive file lock */
# define O_ASYNC	0x000000	/* signal pgrp when data ready */
# define O_SYNC		0x000000	/* synchronous writes */
# define O_DSYNC	0x000000	/* write: I/O data completion */
# define O_RSYNC	0x000000	/* read: I/O completion as for write */
# endif

# ifdef __KERNEL__
# define O_USER		0x00000fff	/* isolates user-settable flag bits */
# define O_GLOBAL	0x00001000	/* OBSOLETE, DONT USE! */
# define O_TTY		0x00002000
# define O_HEAD		0x00004000
# define O_LOCK		0x00008000
# endif

/*
 * Constants used for fcntl(2)
 */

/* command values */
# define F_DUPFD	0		/* duplicate file descriptor */
# define F_GETFD	1		/* get file descriptor flags */
# define F_SETFD	2		/* set file descriptor flags */
# define F_GETFL	3		/* get file status flags */
# define F_SETFL	4		/* set file status flags */
# define F_GETLK	5		/* get record locking information */
# define F_SETLK	6		/* set record locking information */
# define F_SETLKW	7		/* F_SETLK; wait if blocked */

# ifdef __KERNEL__
# define F_GETOPENS		8	/* handled by kernel */
/* structure for F_GETOPENS */
struct listopens
{
# define LO_FILEOPEN		1
# define LO_DIROPEN		2
# define LO_CURDIR		4
# define LO_CURROOT		8
	short	lo_pid;			/* input: first pid to check;
					 * output: who's using it? */
	short	lo_reason;		/* input: bitmask of interesting reasons;
					 * output: why EACCDN? */
	short	lo_flags;		/* file's open flags */
};
# endif

#define F_DUPFD_CLOEXEC		1030

/* file descriptor flags (F_GETFD, F_SETFD) */
# define FD_CLOEXEC	0x01		/* close-on-exec flag */

/* record locking flags (F_GETLK, F_SETLK, F_SETLKW) */
# define F_RDLCK	O_RDONLY	/* shared or read lock */
# define F_WRLCK	O_WRONLY	/* exclusive or write lock */
# define F_UNLCK	3		/* unlock */

# ifdef __KERNEL__
/* XXX todo */
# define F_WAIT		0x010		/* Wait until lock is granted */
# define F_FLOCK	0x020	 	/* Use flock(2) semantics for lock */
# define F_POSIX	0x040	 	/* Use POSIX semantics for lock */
# endif

struct flock
{
	short	l_type;
	short	l_whence;
	long	l_start;		/* XXX off_t */
	long	l_len;			/* XXX off_t */
	short	l_pid;
};

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
/* lock operations for flock(2) */
# define LOCK_SH	0x01		/* shared file lock */
# define LOCK_EX	0x02		/* exclusive file lock */
# define LOCK_NB	0x04		/* don't block when locking */
# define LOCK_UN	0x08		/* unlock file */
# endif

/* lseek() origins */
# define SEEK_SET	0		/* from beginning of file */
# define SEEK_CUR	1		/* from current location */
# define SEEK_END	2		/* from end of file */


# endif /* _mint_fcntl_h */
