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

# ifndef _zone_h
# define _zone_h

# include "global.h"


void	read_zone	(long num, void *buf, int drive);
void	write_zone	(long num, void *buf, int drive);

UNIT *	cget_zone	(long num, int drive);
UNIT *	cput_zone	(long num, int drive);

long	find_zone	(d_inode *rip, long numr, int drive, int flag);
int	next_zone	(d_inode *rip, long numr);

long	l_write		(ushort inum, long pos, long len, const void *buf, int drive);


INLINE void read_zones	(long num, long count, void *buf, int drive);
INLINE void write_zones	(long num, long count, void *buf, int drive);

INLINE void
read_zones (long num, long count, void *buf, int drive)
{
	(void) bio.l_read (super_ptr[drive]->di, num, count, BLOCK_SIZE, buf);
}

INLINE void
write_zones (long num, long count, void *buf, int drive)
{
	(void) bio.l_write (super_ptr[drive]->di, num, count, BLOCK_SIZE, buf);
}


# endif /* _zone_h */
