/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

/*
 * File : mount_xdr3.h
 *        version 3 of the MOUNT protocol, RFC 1813 appendix I
 *
 * Differences to version 1 (the one mount_nfs speaks):
 *  - the file handle is variable sized (up to 64 bytes) instead of a
 *    fixed 32 byte blob,
 *  - the reply carries the list of authentication flavours the server
 *    is willing to accept,
 *  - the status codes are the nfsstat3 ones, not errnos.
 */

# ifndef _mount_xdr3_h
# define _mount_xdr3_h

# include <sys/types.h>
# include <rpc/xdr.h>


/* request numbers for the nfs mount service */
#define MOUNTPROC3_NULL     0
#define MOUNTPROC3_MNT      1
#define MOUNTPROC3_DUMP     2
#define MOUNTPROC3_UMNT     3
#define MOUNTPROC3_UMNTALL  4
#define MOUNTPROC3_EXPORT   5

#define MOUNT_PROGRAM   100005
#define MOUNT_V3        3
#define MOUNT_MAXPROC   5


#define MNTPATHLEN   1024
#define MNTNAMLEN     255
#define FHSIZE3        64

/* mountstat3 */
#define MNT3_OK                 0
#define MNT3ERR_PERM            1
#define MNT3ERR_NOENT           2
#define MNT3ERR_IO              5
#define MNT3ERR_ACCES          13
#define MNT3ERR_NOTDIR         20
#define MNT3ERR_INVAL          22
#define MNT3ERR_NAMETOOLONG    63
#define MNT3ERR_NOTSUPP     10004
#define MNT3ERR_SERVERFAULT 10006

#define MAX_AUTH_FLAVORS       8


bool_t xdr_dirpath (XDR *x, char *s);
bool_t xdr_name (XDR *x, char *s);


/* fhandle3: opaque data<FHSIZE3> */
typedef struct fhandle3
{
	u_int	len;
	char	data[FHSIZE3];
} fhandle3;

bool_t xdr_fhandle3 (XDR *x, fhandle3 *fhp);


typedef struct mountres3
{
	u_int		status;			/* mountstat3 */
	fhandle3	fhandle;
	u_int		nauth;
	int		auth_flavors[MAX_AUTH_FLAVORS];
} mountres3;

bool_t xdr_mountres3 (XDR *x, mountres3 *mrp);

const char *mountstat3_str (u_int status);


# endif
