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

# ifndef _bitmap_h
# define _bitmap_h

# include "global.h"


long	count_bits	(ushort *buf, long num);

long	alloc_zone	(ushort drive);
long	free_zone	(ushort drive, long zone);

ushort	alloc_inode	(ushort drive);
long	free_inode	(ushort drive, ushort inum);


# endif /* _bitmap_h */
