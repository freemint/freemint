/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_file_h
# define _mint_file_h

# include "kcompiler.h"
# include "ktypes.h"

# include "fcntl.h"
# include "fsops.h"


struct file
{
	short	links;		/* number of copies of this descriptor */
	ushort	flags;		/* file open mode and other file flags */
	long	pos;		/* position in file */
	long	devinfo;	/* device driver specific info */
	fcookie	fc;		/* file system cookie for this file */
	DEVDRV	*dev;		/* device driver that knows how to deal with this */
	FILEPTR	*next;		/* link to next fileptr for this file */
};


/* structure for internal kernel locks */
struct ilock
{
	struct flock l;		/* the actual lock */
	struct ilock *next;	/* next lock in the list */
	long reserved [4];	/* reserved for future expansion */
};


/* macros to be applied to FILEPTRS to determine their type */
# define is_terminal(f) ((f)->flags & O_TTY)

/* drives are A:->Z:, 1:->6: */
#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)


# endif /* _mint_file_h */
