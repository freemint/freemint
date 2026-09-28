/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

# ifndef _nfssys_h
# define _nfssys_h

# include "global.h"


extern FILESYS nfs_filesys;

void	init_fs	(void);

long	do_sattr (fcookie *fc, sattr3 *attr);

/* Make sure an index really has a file handle. Indices coming out of
 * nfs_readdir() have none (READDIR3 does not deliver handles), and
 * CREATE3/MKDIR3/SYMLINK3 return the new handle only optionally, so the
 * handle may have to be fetched with a LOOKUP3 first.
 */
long	nfs_get_handle (NFS_INDEX *ni);

/* COMMIT3 the given range; new in NFS3, needed to make unstable writes
 * durable. Returns E_OK and copies the server's write verifier to
 * `verf' (NFS3_WRITEVERFSIZE bytes) on success.
 */
long	do_commit (fcookie *fc, uint64 offset, ulong count, char *verf);

long	_cdecl nfs_getxattr	(fcookie *fc, XATTR *xattr);


# endif /* _nfssys_h */
