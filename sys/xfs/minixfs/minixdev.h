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

# ifndef _minixdev_h
# define _minixdev_h

# include "global.h"


extern DEVDRV minix_dev;

long	m_close		(FILEPTR *f, int pid);


# endif /* _minixdev_h */
