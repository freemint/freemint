/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

# ifndef _nfsutil_h
# define _nfsutil_h

# include "global.h"


INLINE int	after (ulong u, ulong v);
INLINE long	get_timestamp (void);

/* protection bits to be put into a sattr3. Unlike NFS2 the file type
 * must NOT be encoded here.
 */
ulong	nfs3_mode (ushort mode);

/* Map an nfsstat3 to a MiNT error code. NFS2 knew so few errors that the
 * old driver just returned EACCES for everything; NFS3 is precise enough
 * to be worth translating.
 */
long	nfs3_error (enum_t status, long dflt);

void	fattr2xattr (fattr3 *fa, XATTR *xa);

/* store a freshly received fattr3 in an index and restamp it */
void	set_index_attr (NFS_INDEX *ni, fattr3 *fa);

/* the same for the optional attributes NFS3 attaches to nearly every
 * reply; does nothing when the server did not send them
 */
void	update_index_attr (NFS_INDEX *ni, post_op_attr *ap);

/* clamp a 64 bit size to what MiNT's 32 bit XATTR/FILEPTR can express */
long	clamp64 (uint64 v);


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
