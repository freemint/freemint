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

# ifndef _inode_h
# define _inode_h

# include "global.h"


int	read_inode	(unsigned num, d_inode *rip, ushort drv);
int	write_inode	(unsigned num, d_inode *rip, ushort drv);

void	trunc_inode	(d_inode *rip, ushort drive, long count, int zap);
long	itruncate	(unsigned inum, ushort drive, long length);


# endif /* _inode_h */
