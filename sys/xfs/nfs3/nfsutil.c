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
 * File : nfsutil.c
 *        utility functions for the NFS3 XFS
 */

# include "nfsutil.h"

# include "mint/emu_tos.h"


/* NFS2 packed the file type into the mode word, so the driver had to
 * juggle with the N_IF* constants of the protocol. NFS3 has a separate
 * ftype3 field, which maps one to one onto MiNT's file types.
 */
static ushort
mint_type (enum_t type)
{
	switch (type)
	{
		case NF3REG:	return S_IFREG;
		case NF3DIR:	return S_IFDIR;
		case NF3BLK:	return S_IFBLK;
		case NF3CHR:	return S_IFCHR;
		case NF3LNK:	return S_IFLNK;
		case NF3SOCK:	return S_IFSOCK;
		case NF3FIFO:	return S_IFIFO;
	}

	return S_IFREG;
}

/* Only the protection bits go into an NFS3 mode3, see RFC 1813, 2.5.
 * MiNT uses the same values for suid/sgid/sticky and rwx, so masking
 * is all that is needed.
 */
ulong
nfs3_mode (ushort mode)
{
	return (ulong) (mode & N3MODE_PERM);
}

long
nfs3_error (enum_t status, long dflt)
{
	switch (status)
	{
		case NFS3_OK:			return E_OK;
		case NFS3ERR_PERM:		return EPERM;
		case NFS3ERR_NOENT:		return ENOENT;
		case NFS3ERR_IO:		return EIO;
		case NFS3ERR_NXIO:		return ENXIO;
		case NFS3ERR_ACCES:		return EACCES;
		case NFS3ERR_EXIST:		return EEXIST;
		case NFS3ERR_XDEV:		return EXDEV;
		case NFS3ERR_NODEV:		return ENODEV;
		case NFS3ERR_NOTDIR:		return ENOTDIR;
		case NFS3ERR_ISDIR:		return EISDIR;
		case NFS3ERR_INVAL:		return EINVAL;
		case NFS3ERR_FBIG:		return EFBIG;
		case NFS3ERR_NOSPC:		return ENOSPC;
		case NFS3ERR_ROFS:		return EROFS;
		case NFS3ERR_MLINK:		return EMLINK;
		case NFS3ERR_NAMETOOLONG:	return ENAMETOOLONG;
		case NFS3ERR_NOTEMPTY:		return ENOTEMPTY;
		case NFS3ERR_DQUOT:		return EDQUOT;
		case NFS3ERR_STALE:		return ESTALE;
		case NFS3ERR_REMOTE:		return EREMOTE;
		case NFS3ERR_BADHANDLE:		return ESTALE;
		case NFS3ERR_NOT_SYNC:		return EINVAL;
		case NFS3ERR_BAD_COOKIE:	return EINVAL;
		case NFS3ERR_NOTSUPP:		return ENOSYS;
		case NFS3ERR_TOOSMALL:		return EBADARG;
		case NFS3ERR_SERVERFAULT:	return EIO;
		case NFS3ERR_BADTYPE:		return EINVAL;
		case NFS3ERR_JUKEBOX:		return EAGAIN;
	}

	return dflt;
}

long
clamp64 (uint64 v)
{
	if (v > 0x7fffffffULL)
		return 0x7fffffffL;

	return (long) v;
}


/* convert an NFS3 fattr3 structure into a MiNT xattr structure
 */
void
fattr2xattr (fattr3 *fa, XATTR *xa)
{
	xa->mode = mint_type (fa->type) | (ushort) (fa->mode & N3MODE_PERM);
	xa->attr = 0;

	if ((xa->mode & S_IFMT) == S_IFDIR)
		xa->attr |= FA_DIR;
	if ((xa->mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		xa->attr |= FA_RDONLY;

	xa->index	= (long) (fa->fileid & 0xffffffffULL);
	xa->dev		= (ushort) (fa->fsid & 0xffffUL);
	xa->rdev	= (ushort) (((fa->rdev.specdata1 & 0xffUL) << 8)
				    | (fa->rdev.specdata2 & 0xffUL));
	xa->nlink	= (ushort) fa->nlink;
	xa->uid		= (ushort) fa->uid;
	xa->gid		= (ushort) fa->gid;
	xa->size	= clamp64 (fa->size);

	/* NFS3 reports the number of bytes really allocated instead of
	 * NFS2's count of 512 byte blocks
	 */
	xa->blksize	= 512;
	xa->nblocks	= clamp64 ((fa->used + 511ULL) >> 9);

	if (native_utc)
	{
		SET_XATTR_TD(xa,m,fa->mtime.seconds);
		SET_XATTR_TD(xa,a,fa->atime.seconds);
		SET_XATTR_TD(xa,c,fa->ctime.seconds);
	}
	else
	{
		SET_XATTR_TD(xa,m,dostime(fa->mtime.seconds));
		SET_XATTR_TD(xa,a,dostime(fa->atime.seconds));
		SET_XATTR_TD(xa,c,dostime(fa->ctime.seconds));
	}

	xa->reserved2 = 0;
	xa->reserved3 [0] = 0;
	xa->reserved3 [1] = 0;
}

void
set_index_attr (NFS_INDEX *ni, fattr3 *fa)
{
	fattr2xattr (fa, &ni->attr);
	ni->size = fa->size;
	ni->stamp = get_timestamp ();
}

void
update_index_attr (NFS_INDEX *ni, post_op_attr *ap)
{
	if (ni && ap->attributes_follow)
		set_index_attr (ni, &ap->attributes);
}
