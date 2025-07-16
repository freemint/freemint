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


extern FILESYS pipe_filesys;

void pipefs_warp_clock(long diff);

# endif /* _pipefs_h */
