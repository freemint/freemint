/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _dir_h
# define _dir_h

# include "global.h"


long	search_dir	(const char *name, unsigned inum, ushort drive, int flag);
long	is_parent	(unsigned dir1, unsigned dir2, ushort drive);


# endif /* _dir_h */
