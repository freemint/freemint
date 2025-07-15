/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _pipefs_h
# define _pipefs_h

# include "mint/mint.h"
# include "mint/fsops.h"
# include "mint/time.h"


struct file;
struct pipe;
struct tty;


struct fifo
{
# define NAME_MAX 14
	char	name[NAME_MAX+1]; /* FIFO's name */
	struct timeval mtime;	/* mtime */
	struct timeval ctime;	/* ctime */
	short	dosflags;	/* DOS flags, e.g. FA_RDONLY, FA_HIDDEN */
	ushort	mode;		/* file access mode, for XATTR */
	ushort	uid, gid;	/* file owner; uid and gid */
	fs_ino_t ino;
	short	flags;		/* various other flags (e.g. O_TTY) */
	short	lockpid;	/* pid of locking process */
	short	cursrate;	/* cursor flash rate for pseudo TTY's */
	struct tty *tty;	/* tty struct for pseudo TTY's */
	struct pipe *inp;	/* pipe for reads */
	struct pipe *outp;	/* pipe for writes (0 if unidirectional) */
	struct fifo *next;	/* link to next FIFO in list */
	struct file *open;	/* open file pointers for this fifo */
};

extern FILESYS pipe_filesys;

void pipefs_warp_clock(long diff);

# endif /* _pipefs_h */
