/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

# ifndef _index_h
# define _index_h

# include "global.h"


void init_mount_data (void);
void init_index (void);

void init_mount_attr (XATTR *ap);
/* Diagnostics, reachable through Dcntl(NFS3_MNTDUMP) and NFS3_DUMPALL. */
void do_mountdump (void);
void index_statistics (void);

NFS_INDEX *get_mount_slot (const char *name, NFS_MOUNT_INFO *info);
int release_mount_slot (NFS_INDEX *ni);
NFS_INDEX *get_slot (NFS_INDEX *dir, const char *name, int dom);
void free_slot (NFS_INDEX *ni);
long remove_slot_by_name (NFS_INDEX *dir, char *name);
void init_cluster (INDEX_CLUSTER *icp, int number);
void free_cluster (INDEX_CLUSTER *icp);


# endif /* _index_h */
