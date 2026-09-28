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

# ifndef _nfssys_h
# define _nfssys_h

# include "global.h"


extern FILESYS nfs_filesys;

void	init_fs	(void);

long	do_sattr (fcookie *fc, sattr *attr);

long	_cdecl nfs_getxattr	(fcookie *fc, XATTR *xattr);


# endif /* _nfssys_h */
