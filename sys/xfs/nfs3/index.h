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

# ifndef _index_h
# define _index_h

# include "global.h"


void init_mount_data (void);
void init_index (void);

void init_mount_attr (XATTR *ap);
NFS_INDEX *get_mount_slot (const char *name, NFS_MOUNT_INFO *info);
int release_mount_slot (NFS_INDEX *ni);
NFS_INDEX *get_slot (NFS_INDEX *dir, const char *name, int dom);
void free_slot (NFS_INDEX *ni);
long remove_slot_by_name (NFS_INDEX *dir, char *name);
void init_cluster (INDEX_CLUSTER *icp, int number);
void free_cluster (INDEX_CLUSTER *icp);


# endif /* _index_h */
