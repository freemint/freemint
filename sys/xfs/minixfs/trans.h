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

# ifndef _trans_h
# define _trans_h

# include "global.h"


int	do_trans	(long flag, int drive);
char *	tosify		(const char *name, int flag, char *first, int mnamlength);


# endif /* _trans_h */
