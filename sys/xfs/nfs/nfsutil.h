/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _nfsutil_h
# define _nfsutil_h

# include "global.h"


INLINE int	after (ulong u, ulong v);
INLINE long	get_timestamp (void);

int 		nfs_mode (int mode, int attrib);

void fattr2xattr (fattr *fa, XATTR *xa);
# if 0
void xattr2fattr (XATTR *xa, fattr *fa);
# endif


/* Was time stamp u build after timestamp v? Make sure to watch for
 * wrap-arounds!
 */
INLINE int
after (ulong u, ulong v)
{
	return ((long)(u - v) > 0);
}


INLINE long
get_timestamp (void)
{
	return *(volatile long *) 0x4baL;
}


# endif /* _nfsutil_h */
