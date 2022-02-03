/*
 * Copyright 1993 by Ulrich K�hn. All rights reserved.
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

/*
 * File : nfs_xdr.c
 *        some utility functions for dealing with the nfs xdr types
 */

# include "global.h"


bool_t
xdr_nfsstat (xdrs *x, nfsstat *sp)
{
	return xdr_enum (x, (enum_t*)sp);
}

long
xdr_size_nfsstat (nfsstat *sp)
{
	return sizeof (ulong);
}

bool_t
xdr_ftype (xdrs *x, ftype *fp)
{
	return xdr_enum (x, (int*)fp);
}

bool_t
xdr_nfscookie (xdrs *x, nfscookie cookie)
{
	return xdr_fixedopaq (x, cookie, COOKIESIZE);
}

bool_t
xdr_nfsfh (xdrs *x, nfs_fh *fp)
{
	return xdr_fixedopaq (x, fp->data, FHSIZE);
}


bool_t
xdr_nfstime (xdrs *x, nfstime *tp)
{
	if (!xdr_ulong (x, &tp->seconds))
		return FALSE;
	
	return xdr_ulong (x, &tp->useconds);
}

bool_t
xdr_fattr (xdrs *x, fattr *fp)
{
	long *buf;
	
	if (XDR_FREE == x->op)
		return TRUE;
	
	xdr_ftype (x, &fp->type);
# ifndef NO_INLINE
	buf = xdr_inline (x, 10 * BYTES_PER_XDR_UNIT);
# else
	buf = NULL;
# endif
	if (buf)
	{
		if (x->op == XDR_DECODE)
		{
			fp->mode	= IXDR_GET_ULONG (buf);
			fp->nlink	= IXDR_GET_ULONG (buf);
			fp->uid		= IXDR_GET_ULONG (buf);
			fp->gid		= IXDR_GET_ULONG (buf);
			fp->size	= IXDR_GET_ULONG (buf);
			fp->blocksize	= IXDR_GET_ULONG (buf);
			fp->rdev	= IXDR_GET_ULONG (buf);
			fp->blocks	= IXDR_GET_ULONG (buf);
			fp->fsid	= IXDR_GET_ULONG (buf);
			fp->fileid	= IXDR_GET_ULONG (buf);
		}
		else if (x->op == XDR_ENCODE)
		{
			IXDR_PUT_ULONG (buf, fp->mode);
			IXDR_PUT_ULONG (buf, fp->nlink);
			IXDR_PUT_ULONG (buf, fp->uid);
			IXDR_PUT_ULONG (buf, fp->gid);
			IXDR_PUT_ULONG (buf, fp->size);
			IXDR_PUT_ULONG (buf, fp->blocksize);
			IXDR_PUT_ULONG (buf, fp->rdev);
			IXDR_PUT_ULONG (buf, fp->blocks);
			IXDR_PUT_ULONG (buf, fp->fsid);
			IXDR_PUT_ULONG (buf, fp->fileid);
		}
	}
	else
	{
		if (!xdr_ulong (x, &fp->mode))
			return FALSE;
		if (!xdr_ulong (x, &fp->nlink))
			return FALSE;
		if (!xdr_ulong (x, &fp->uid))
			return FALSE;
		if (!xdr_ulong (x, &fp->gid))
			return FALSE;
		if (!xdr_ulong (x, &fp->size))
			return FALSE;
		if (!xdr_ulong (x, &fp->blocksize))
			return FALSE;
		if (!xdr_ulong (x, &fp->rdev))
			return FALSE;
		if (!xdr_ulong (x, &fp->blocks))
			return FALSE;
		if (!xdr_ulong (x, &fp->fsid))
			return FALSE;
		if (!xdr_ulong (x, &fp->fileid))
			return FALSE;
	}
	
	if (!xdr_nfstime (x, &fp->atime))
		return FALSE;
	if (!xdr_nfstime (x, &fp->mtime))
		return FALSE;
	if (!xdr_nfstime (x, &fp->ctime))
		return FALSE;
	
	return TRUE;
}

bool_t
xdr_sattr (xdrs *x, sattr *sp)
{
	long *buf;
	
	if (XDR_FREE == x->op)
		return TRUE;
	
# ifndef NO_INLINE
	buf = xdr_inline (x, 4 * BYTES_PER_XDR_UNIT);
# else
	buf = NULL;
# endif
	if (buf)
	{
		if (XDR_DECODE == x->op)
		{
			sp->mode = IXDR_GET_ULONG (buf);
			sp->uid  = IXDR_GET_ULONG (buf);
			sp->gid  = IXDR_GET_ULONG (buf);
			sp->size = IXDR_GET_ULONG (buf);
		}
		else if (XDR_ENCODE == x->op)
		{
			IXDR_PUT_ULONG (buf, sp->mode);
			IXDR_PUT_ULONG (buf, sp->uid);
			IXDR_PUT_ULONG (buf, sp->gid);
			IXDR_PUT_ULONG (buf, sp->size);
		}
	}
	else
	{
		if (!xdr_ulong (x, &sp->mode))
			return FALSE;
		if (!xdr_ulong (x, &sp->uid))
			return FALSE;
		if (!xdr_ulong (x, &sp->gid))
			return FALSE;
		if (!xdr_ulong (x, &sp->size))
			return FALSE;
	}
	
	if (!xdr_nfstime (x, &sp->atime))
			return FALSE;
	
	return xdr_nfstime (x, &sp->mtime);
}

bool_t
xdr_attrstat (xdrs *x, attrstat *ap)
{
	if (!xdr_enum (x, &ap->status))
		return FALSE;
	
	if (NFS_OK == ap->status)
		return xdr_fattr (x, &ap->attrstat_u.attributes);
	
	return TRUE;
}

long
xdr_size_attrstat (attrstat *ap)
{
	ulong r = sizeof (ulong);

	if (NFS_OK == ap->status)
		r += sizeof (fattr);
	
	return r;
}

bool_t
xdr_sattrargs (xdrs *x, sattrargs *argp)
{
	if (!xdr_nfsfh (x, &argp->file))
		return FALSE;
	
	return xdr_sattr (x, &argp->attributes);
}

long
xdr_size_sattrargs (sattrargs *ap)
{
	return xdr_size_nfsfh (&ap->file) + xdr_size_sattr (&ap->attributes);
}

bool_t
xdr_diropres (xdrs *x, diropres *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	
	if (NFS_OK == rp->status)
	{
		if (!xdr_nfsfh (x, &rp->diropres_u.diropok.file))
			return FALSE;
		
		return xdr_fattr (x, &rp->diropres_u.diropok.attributes);
	}
	
	return TRUE;
}

long
xdr_size_diropres (diropres *rp)
{
	long r = sizeof (ulong);
	
	if (NFS_OK == rp->status)
	{
		r += xdr_size_nfsfh (&rp->diropres_u.diropok.file);
		r += xdr_size_fattr (&rp->diropres_u.diropok.attributes);
	}
	
	return r;
}

bool_t
xdr_diropargs (xdrs *x, diropargs *ap)
{
	if (!xdr_nfsfh (x, &ap->dir))
		return FALSE;
	
	return xdr_string (x, (const char **)&ap->name, MAXNAMLEN);
}

long
xdr_size_diropargs (diropargs *ap)
{
	long r = xdr_size_nfsfh (&ap->dir);
	
	r += sizeof (ulong);
	r += (strlen (ap->name)+3) & (~3L);  /* round up to four bytes */
	
	return r;
}


bool_t
xdr_readlinkres (xdrs *x, readlinkres *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	
	if (NFS_OK == rp->status) {
		const char *name = rp->readlinkres_u.data;

		return xdr_string (x, &name, MAXPATHLEN);
	}
	
	return TRUE;
}

long
xdr_size_readlinkres (readlinkres *rp)
{
	long r = sizeof (ulong);
	
	if (NFS_OK == rp->status)
	{
		r += sizeof (ulong);
		r += (strlen (rp->readlinkres_u.data) + 3) & (~3L);
	}
	
	return r;
}

bool_t
xdr_readres (xdrs *x, readres *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	
	if (NFS_OK == rp->status)
	{
		const char *name;

		if (!xdr_fattr (x, &rp->readres_u.read_ok.attributes))
			return FALSE;

		name = rp->readres_u.read_ok.data_val;
		
		if (!xdr_opaque (x, &name,
				    (long *)&rp->readres_u.read_ok.data_len, MAXDATA))
			return FALSE;
	}
	
	return TRUE;
}

long
xdr_size_readres (readres *rp)
{
	long r = sizeof (ulong);

	if (NFS_OK == rp->status)
	{
		r += xdr_size_fattr (&rp->readres_u.read_ok.attributes);
		r += sizeof (ulong);
		r += (rp->readres_u.read_ok.data_len + 3) & (~3L);
	}
	
	return r;
}

bool_t
xdr_readargs (xdrs *x, readargs *ap)
{
	if (!xdr_nfsfh (x, &ap->file))
		return FALSE;
	
	xdr_ulong (x, &ap->offset);
	xdr_ulong (x, &ap->count);
	
	return xdr_ulong (x, &ap->totalcount);
}

long
xdr_size_readargs (readargs *ap)
{
	return xdr_size_nfsfh (&ap->file) + 3 * sizeof (ulong);
}

bool_t
xdr_writeargs (xdrs *x, writeargs *ap)
{
	if (!xdr_nfsfh (x, &ap->file))
		return FALSE;
	
	xdr_ulong (x, &ap->beginoffset);
	xdr_ulong (x, &ap->offset);
	xdr_ulong (x, &ap->totalcount);
	
	return xdr_opaque (x, (const opaque **)&ap->data_val, (long *)&ap->data_len, MAXDATA);
}

long
xdr_size_writeargs (writeargs *ap)
{
	long r;
	
	r  = sizeof (nfs_fh);
	r += 4 * sizeof (ulong);
	r += (ap->totalcount + 3) & (~3L);
	
	return r;
}

bool_t
xdr_createargs (xdrs *x, createargs *ap)
{
	if (!xdr_diropargs (x, &ap->where))
		return FALSE;
	
	return xdr_sattr (x, &ap->attributes);
}

long
xdr_size_createargs (createargs *ap)
{
	return xdr_size_diropargs (&ap->where) + xdr_size_sattr (&ap->attributes);
}


bool_t
xdr_renameargs (xdrs *x, renameargs *ap)
{
	if (!xdr_diropargs (x, &ap->from))
		return FALSE;
	
	return xdr_diropargs (x, &ap->to);
}

long
xdr_size_renameargs (renameargs *ap)
{
	return xdr_size_diropargs (&ap->from) + xdr_size_diropargs (&ap->to);
}

bool_t
xdr_linkargs (xdrs *x, linkargs *ap)
{
	if (!xdr_nfsfh (x, &ap->from))
		return FALSE;
	
	return xdr_diropargs (x, &ap->to);
}

long
xdr_size_linkargs (linkargs *ap)
{
	return xdr_size_nfsfh (&ap->from) + xdr_size_diropargs (&ap->to);
}

bool_t
xdr_symlinkargs (xdrs *x, symlinkargs *ap)
{
	if (!xdr_diropargs (x, &ap->from))
		return FALSE;
	
	if (!xdr_string (x, &ap->to, MAXPATHLEN))
		return FALSE;
	
	return xdr_sattr (x, &ap->attributes);
}

long
xdr_size_symlinkargs (symlinkargs *ap)
{
	long r = xdr_size_diropargs (&ap->from);
	
	r += sizeof (ulong);
	r += (strlen (ap->to) + 3) & (~3L);
	r += xdr_size_sattr (&ap->attributes);
	
	return r;
}

bool_t
xdr_entry (xdrs *x, entry *ep)
{
	const char *name;

	if (!xdr_ulong (x, &ep->fileid))
		return FALSE;
	
	if (XDR_DECODE == x->op)
		ep->name = (char *) ep + sizeof (entry);
	
	name = ep->name;

	if (!xdr_string (x, &name, MAXNAMLEN))
		return FALSE;
	
	if (!xdr_nfscookie (x, ep->cookie))
		return FALSE;
	
	if (XDR_DECODE == x->op)
	{
		char *p = (char*)ep;

		p += sizeof (entry) + strlen (ep->name) + 1;
		p = (char *)(((long) p + 1) & (~1L));  /* round up */
		
		ep->nextentry = (entry *) p;
	}
	
	return xdr_pointer (x, (char **)((entry *)&ep->nextentry), sizeof(entry), (xdrproc_t) xdr_entry);
}

long
xdr_size_entry (entry *ep)
{
	long r = 0;
	
	while (ep)
	{
		r += sizeof (ulong);
		r += sizeof (ulong);
		r += (strlen (ep->name) + 3) & (~3L);
		r += xdr_size_nfscookie (ep->cookie);
		r += sizeof (ulong);
		
		ep = ep->nextentry;
	}
	
	return r;
}

bool_t
xdr_readdirres (xdrs *x, readdirres *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	
	if (NFS_OK == rp->status)
	{
		if (!xdr_pointer (x, (char**)((entry *)&rp->readdirres_u.readdirok.entries),
			sizeof(entry), (xdrproc_t) xdr_entry))
		{
			return FALSE;
		}
		
		return xdr_bool (x, &rp->readdirres_u.readdirok.eof);
	}
	
	return TRUE;
}

long
xdr_size_readdirres (readdirres *rp)
{
	long r = sizeof (ulong);
	
	if (NFS_OK == rp->status)
	{
		r += sizeof (ulong);   /* the first boolean "pointer" */
		r += xdr_size_entry (rp->readdirres_u.readdirok.entries);
		r += sizeof (ulong);
	}
	
	return r;
}

bool_t
xdr_readdirargs (xdrs *x, readdirargs *ap)
{
	if (!xdr_nfsfh (x, &ap->dir))
		return FALSE;
	
	if (!xdr_nfscookie (x, ap->cookie))
		return FALSE;
	
	return xdr_ulong (x, &ap->count);
}

long
xdr_size_readdirargs (readdirargs *ap)
{
	long r;
	
	r  = xdr_size_nfsfh (&ap->dir);
	r += xdr_size_nfscookie (ap->cookie);
	r += sizeof (ulong);
	
	return r;
}

bool_t
xdr_statfsres (xdrs *x, statfsres *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	
	if (NFS_OK == rp->status)
	{
		xdr_ulong (x, &rp->statfsres_u.info.tsize);
		xdr_ulong (x, &rp->statfsres_u.info.bsize);
		xdr_ulong (x, &rp->statfsres_u.info.blocks);
		xdr_ulong (x, &rp->statfsres_u.info.bfree);
		
		return xdr_ulong (x, &rp->statfsres_u.info.bavail);
	}
	
	return TRUE;
}

long
xdr_size_statfsres (statfsres *rp)
{
	long r = sizeof (ulong);
	
	if (NFS_OK == rp->status)
		r += 5 * sizeof (ulong);
	
	return r;
}
