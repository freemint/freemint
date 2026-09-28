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

# ifndef _cache_h
# define _cache_h

# include "global.h"


NFS_INDEX *nfs_cache_lookup (NFS_INDEX *dir, const char *name, int dom);
void nfs_cache_expire (void);
int nfs_cache_add (NFS_INDEX *dir, NFS_INDEX *index);
int nfs_cache_remove (NFS_INDEX *ni);
int nfs_cache_removebyname (NFS_INDEX *parent, const char *name);


# endif /* _cache_h */
