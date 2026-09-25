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
 * File : nfs3_xdr.c
 *        (de)serialisation of the RFC 1813 data types
 *
 * The xdr_size_*() functions return an upper bound for the number of
 * bytes an encoded request occupies, so that a buffer of the right size
 * can be allocated before encoding starts.
 */

# include "global.h"


/* ---------------------------------------------------------------- */
/* basic types                                                      */
/* ---------------------------------------------------------------- */

bool_t
xdr_nfs_fh3 (xdrs *x, nfs_fh3 *fhp)
{
	return xdr_varopaque (x, fhp->data, &fhp->len, NFS3_FHSIZE);
}

long
xdr_size_nfs_fh3 (nfs_fh3 *fhp)
{
	return XDR_STRSIZE (fhp->len);
}

bool_t
xdr_nfstime3 (xdrs *x, nfstime3 *tp)
{
	if (!xdr_ulong (x, &tp->seconds))
		return FALSE;

	return xdr_ulong (x, &tp->nseconds);
}

bool_t
xdr_specdata3 (xdrs *x, specdata3 *sp)
{
	if (!xdr_ulong (x, &sp->specdata1))
		return FALSE;

	return xdr_ulong (x, &sp->specdata2);
}

bool_t
xdr_fattr3 (xdrs *x, fattr3 *fp)
{
	if (XDR_FREE == x->op)
		return TRUE;

	if (!xdr_enum (x, &fp->type))
		return FALSE;
	if (!xdr_ulong (x, &fp->mode))
		return FALSE;
	if (!xdr_ulong (x, &fp->nlink))
		return FALSE;
	if (!xdr_ulong (x, &fp->uid))
		return FALSE;
	if (!xdr_ulong (x, &fp->gid))
		return FALSE;
	if (!xdr_uint64 (x, &fp->size))
		return FALSE;
	if (!xdr_uint64 (x, &fp->used))
		return FALSE;
	if (!xdr_specdata3 (x, &fp->rdev))
		return FALSE;
	if (!xdr_uint64 (x, &fp->fsid))
		return FALSE;
	if (!xdr_uint64 (x, &fp->fileid))
		return FALSE;
	if (!xdr_nfstime3 (x, &fp->atime))
		return FALSE;
	if (!xdr_nfstime3 (x, &fp->mtime))
		return FALSE;

	return xdr_nfstime3 (x, &fp->ctime);
}

bool_t
xdr_post_op_attr (xdrs *x, post_op_attr *ap)
{
	if (!xdr_bool (x, &ap->attributes_follow))
		return FALSE;

	if (ap->attributes_follow)
		return xdr_fattr3 (x, &ap->attributes);

	return TRUE;
}

static bool_t
xdr_wcc_attr (xdrs *x, wcc_attr *ap)
{
	if (!xdr_uint64 (x, &ap->size))
		return FALSE;
	if (!xdr_nfstime3 (x, &ap->mtime))
		return FALSE;

	return xdr_nfstime3 (x, &ap->ctime);
}

static bool_t
xdr_pre_op_attr (xdrs *x, pre_op_attr *ap)
{
	if (!xdr_bool (x, &ap->attributes_follow))
		return FALSE;

	if (ap->attributes_follow)
		return xdr_wcc_attr (x, &ap->attributes);

	return TRUE;
}

bool_t
xdr_wcc_data (xdrs *x, wcc_data *wp)
{
	if (!xdr_pre_op_attr (x, &wp->before))
		return FALSE;

	return xdr_post_op_attr (x, &wp->after);
}

bool_t
xdr_post_op_fh3 (xdrs *x, post_op_fh3 *fp)
{
	if (!xdr_bool (x, &fp->handle_follows))
		return FALSE;

	if (fp->handle_follows)
		return xdr_nfs_fh3 (x, &fp->handle);

	fp->handle.len = 0;
	return TRUE;
}


/* Prepare a sattr3 structure that changes nothing at all. NFS2 used
 * "all bits set" for this, NFS3 has an explicit flag per attribute.
 */
void
sattr3_init (sattr3 *sp)
{
	sp->set_mode = FALSE;
	sp->mode = 0;
	sp->set_uid = FALSE;
	sp->uid = 0;
	sp->set_gid = FALSE;
	sp->gid = 0;
	sp->set_size = FALSE;
	sp->size = 0;
	sp->set_atime = DONT_CHANGE;
	sp->atime.seconds = 0;
	sp->atime.nseconds = 0;
	sp->set_mtime = DONT_CHANGE;
	sp->mtime.seconds = 0;
	sp->mtime.nseconds = 0;
}

bool_t
xdr_sattr3 (xdrs *x, sattr3 *sp)
{
	if (XDR_FREE == x->op)
		return TRUE;

	if (!xdr_bool (x, &sp->set_mode))
		return FALSE;
	if (sp->set_mode && !xdr_ulong (x, &sp->mode))
		return FALSE;

	if (!xdr_bool (x, &sp->set_uid))
		return FALSE;
	if (sp->set_uid && !xdr_ulong (x, &sp->uid))
		return FALSE;

	if (!xdr_bool (x, &sp->set_gid))
		return FALSE;
	if (sp->set_gid && !xdr_ulong (x, &sp->gid))
		return FALSE;

	if (!xdr_bool (x, &sp->set_size))
		return FALSE;
	if (sp->set_size && !xdr_uint64 (x, &sp->size))
		return FALSE;

	if (!xdr_enum (x, &sp->set_atime))
		return FALSE;
	if (sp->set_atime == SET_TO_CLIENT_TIME && !xdr_nfstime3 (x, &sp->atime))
		return FALSE;

	if (!xdr_enum (x, &sp->set_mtime))
		return FALSE;
	if (sp->set_mtime == SET_TO_CLIENT_TIME && !xdr_nfstime3 (x, &sp->mtime))
		return FALSE;

	return TRUE;
}

long
xdr_size_sattr3 (sattr3 *sp)
{
	long r = 6L * sizeof (ulong);	/* the six discriminators */

	if (sp->set_mode)
		r += sizeof (ulong);
	if (sp->set_uid)
		r += sizeof (ulong);
	if (sp->set_gid)
		r += sizeof (ulong);
	if (sp->set_size)
		r += 2L * sizeof (ulong);
	if (sp->set_atime == SET_TO_CLIENT_TIME)
		r += 2L * sizeof (ulong);
	if (sp->set_mtime == SET_TO_CLIENT_TIME)
		r += 2L * sizeof (ulong);

	return r;
}

bool_t
xdr_diropargs3 (xdrs *x, diropargs3 *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->dir))
		return FALSE;

	return xdr_string (x, &ap->name, MAXNAMLEN);
}

long
xdr_size_diropargs3 (diropargs3 *ap)
{
	return xdr_size_nfs_fh3 (&ap->dir) + XDR_STRSIZE (strlen (ap->name));
}


/* ---------------------------------------------------------------- */
/* GETATTR / SETATTR                                                */
/* ---------------------------------------------------------------- */

bool_t
xdr_getattr3res (xdrs *x, getattr3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;

	if (NFS3_OK == rp->status)
		return xdr_fattr3 (x, &rp->obj_attributes);

	return TRUE;
}

bool_t
xdr_setattr3args (xdrs *x, setattr3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->object))
		return FALSE;
	if (!xdr_sattr3 (x, &ap->new_attributes))
		return FALSE;
	if (!xdr_bool (x, &ap->check))
		return FALSE;

	if (ap->check)
		return xdr_nfstime3 (x, &ap->obj_ctime);

	return TRUE;
}

long
xdr_size_setattr3args (setattr3args *ap)
{
	long r = xdr_size_nfs_fh3 (&ap->object);

	r += xdr_size_sattr3 (&ap->new_attributes);
	r += sizeof (ulong);
	if (ap->check)
		r += 2L * sizeof (ulong);

	return r;
}

/* Result of SETATTR, REMOVE and RMDIR: status plus wcc_data, which is
 * present in the failure case as well.
 */
bool_t
xdr_wcc3res (xdrs *x, wcc3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;

	return xdr_wcc_data (x, &rp->wcc);
}


/* ---------------------------------------------------------------- */
/* LOOKUP / ACCESS                                                  */
/* ---------------------------------------------------------------- */

bool_t
xdr_lookup3res (xdrs *x, lookup3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;

	if (NFS3_OK == rp->status)
	{
		if (!xdr_nfs_fh3 (x, &rp->object))
			return FALSE;
		if (!xdr_post_op_attr (x, &rp->obj_attributes))
			return FALSE;
	}
	else
		rp->obj_attributes.attributes_follow = FALSE;

	return xdr_post_op_attr (x, &rp->dir_attributes);
}

bool_t
xdr_access3args (xdrs *x, access3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->object))
		return FALSE;

	return xdr_ulong (x, &ap->access);
}

long
xdr_size_access3args (access3args *ap)
{
	return xdr_size_nfs_fh3 (&ap->object) + sizeof (ulong);
}

bool_t
xdr_access3res (xdrs *x, access3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->obj_attributes))
		return FALSE;

	if (NFS3_OK == rp->status)
		return xdr_ulong (x, &rp->access);

	rp->access = 0;
	return TRUE;
}


/* ---------------------------------------------------------------- */
/* READLINK / READ / WRITE                                          */
/* ---------------------------------------------------------------- */

bool_t
xdr_readlink3res (xdrs *x, readlink3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->symlink_attributes))
		return FALSE;

	if (NFS3_OK == rp->status)
	{
		const char *path = rp->data;

		return xdr_string (x, &path, MAXPATHLEN);
	}

	return TRUE;
}

bool_t
xdr_read3args (xdrs *x, read3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->file))
		return FALSE;
	if (!xdr_uint64 (x, &ap->offset))
		return FALSE;

	return xdr_ulong (x, &ap->count);
}

long
xdr_size_read3args (read3args *ap)
{
	return xdr_size_nfs_fh3 (&ap->file) + 3L * sizeof (ulong);
}

bool_t
xdr_read3res (xdrs *x, read3res *rp)
{
	ulong len;

	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->file_attributes))
		return FALSE;

	rp->data_len = 0;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_ulong (x, &rp->count))
		return FALSE;
	if (!xdr_bool (x, &rp->eof))
		return FALSE;

	/* the opaque data carries its own length, which has to agree
	 * with `count' (RFC 1813, 3.3.6)
	 */
	if (x->length < (long) sizeof (ulong))
		return FALSE;

	len = *(ulong *) x->current;
	if (len != rp->count || len > rp->data_max)
		return FALSE;

	x->current += sizeof (ulong);
	x->length -= sizeof (ulong);

	if (x->length < (long) XDR_ROUNDUP (len))
		return FALSE;

	memcpy (rp->data_val, x->current, len);
	rp->data_len = len;

	x->current += XDR_ROUNDUP (len);
	x->length -= XDR_ROUNDUP (len);

	return TRUE;
}

bool_t
xdr_write3args (xdrs *x, write3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->file))
		return FALSE;
	if (!xdr_uint64 (x, &ap->offset))
		return FALSE;
	if (!xdr_ulong (x, &ap->count))
		return FALSE;
	if (!xdr_enum (x, &ap->stable))
		return FALSE;

	{
		/* xdr_opaque() wants a long, and ap->data_len is a
		 * count3; do not alias the two
		 */
		long len = (long) ap->data_len;
		bool_t ok;

		ok = xdr_opaque (x, (const opaque **) &ap->data_val, &len, MAXDATA);
		ap->data_len = (ulong) len;

		return ok;
	}
}

long
xdr_size_write3args (write3args *ap)
{
	long r = xdr_size_nfs_fh3 (&ap->file);

	r += 2L * sizeof (ulong);		/* offset */
	r += sizeof (ulong);			/* count */
	r += sizeof (ulong);			/* stable */
	r += XDR_STRSIZE (ap->data_len);

	return r;
}

bool_t
xdr_write3res (xdrs *x, write3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_wcc_data (x, &rp->file_wcc))
		return FALSE;

	rp->count = 0;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_ulong (x, &rp->count))
		return FALSE;
	if (!xdr_enum (x, &rp->committed))
		return FALSE;

	return xdr_fixedopaq (x, rp->verf, NFS3_WRITEVERFSIZE);
}


/* ---------------------------------------------------------------- */
/* CREATE / MKDIR / SYMLINK                                         */
/* ---------------------------------------------------------------- */

bool_t
xdr_create3res (xdrs *x, create3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;

	if (NFS3_OK == rp->status)
	{
		if (!xdr_post_op_fh3 (x, &rp->obj))
			return FALSE;
		if (!xdr_post_op_attr (x, &rp->obj_attributes))
			return FALSE;
	}
	else
	{
		rp->obj.handle_follows = FALSE;
		rp->obj.handle.len = 0;
		rp->obj_attributes.attributes_follow = FALSE;
	}

	return xdr_wcc_data (x, &rp->dir_wcc);
}

bool_t
xdr_create3args (xdrs *x, create3args *ap)
{
	if (!xdr_diropargs3 (x, &ap->where))
		return FALSE;
	if (!xdr_enum (x, &ap->how))
		return FALSE;

	if (EXCLUSIVE == ap->how)
		return xdr_fixedopaq (x, ap->verf, NFS3_CREATEVERFSIZE);

	return xdr_sattr3 (x, &ap->obj_attributes);
}

long
xdr_size_create3args (create3args *ap)
{
	long r = xdr_size_diropargs3 (&ap->where);

	r += sizeof (ulong);
	if (EXCLUSIVE == ap->how)
		r += NFS3_CREATEVERFSIZE;
	else
		r += xdr_size_sattr3 (&ap->obj_attributes);

	return r;
}

bool_t
xdr_mkdir3args (xdrs *x, mkdir3args *ap)
{
	if (!xdr_diropargs3 (x, &ap->where))
		return FALSE;

	return xdr_sattr3 (x, &ap->attributes);
}

long
xdr_size_mkdir3args (mkdir3args *ap)
{
	return xdr_size_diropargs3 (&ap->where)
		+ xdr_size_sattr3 (&ap->attributes);
}

bool_t
xdr_symlink3args (xdrs *x, symlink3args *ap)
{
	if (!xdr_diropargs3 (x, &ap->where))
		return FALSE;

	/* NFS2 sent the target path before the attributes, NFS3 sends
	 * the attributes first
	 */
	if (!xdr_sattr3 (x, &ap->symlink_attributes))
		return FALSE;

	return xdr_string (x, &ap->symlink_data, MAXPATHLEN);
}

long
xdr_size_symlink3args (symlink3args *ap)
{
	long r = xdr_size_diropargs3 (&ap->where);

	r += xdr_size_sattr3 (&ap->symlink_attributes);
	r += XDR_STRSIZE (strlen (ap->symlink_data));

	return r;
}


/* ---------------------------------------------------------------- */
/* RENAME / LINK                                                    */
/* ---------------------------------------------------------------- */

bool_t
xdr_rename3args (xdrs *x, rename3args *ap)
{
	if (!xdr_diropargs3 (x, &ap->from))
		return FALSE;

	return xdr_diropargs3 (x, &ap->to);
}

long
xdr_size_rename3args (rename3args *ap)
{
	return xdr_size_diropargs3 (&ap->from) + xdr_size_diropargs3 (&ap->to);
}

bool_t
xdr_rename3res (xdrs *x, rename3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_wcc_data (x, &rp->fromdir_wcc))
		return FALSE;

	return xdr_wcc_data (x, &rp->todir_wcc);
}

bool_t
xdr_link3args (xdrs *x, link3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->file))
		return FALSE;

	return xdr_diropargs3 (x, &ap->link);
}

long
xdr_size_link3args (link3args *ap)
{
	return xdr_size_nfs_fh3 (&ap->file) + xdr_size_diropargs3 (&ap->link);
}

bool_t
xdr_link3res (xdrs *x, link3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->file_attributes))
		return FALSE;

	return xdr_wcc_data (x, &rp->linkdir_wcc);
}


/* ---------------------------------------------------------------- */
/* READDIR                                                          */
/* ---------------------------------------------------------------- */

bool_t
xdr_readdir3args (xdrs *x, readdir3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->dir))
		return FALSE;
	if (!xdr_uint64 (x, &ap->cookie))
		return FALSE;
	if (!xdr_fixedopaq (x, ap->cookieverf, NFS3_COOKIEVERFSIZE))
		return FALSE;

	return xdr_ulong (x, &ap->count);
}

long
xdr_size_readdir3args (readdir3args *ap)
{
	long r = xdr_size_nfs_fh3 (&ap->dir);

	r += 2L * sizeof (ulong);		/* cookie */
	r += NFS3_COOKIEVERFSIZE;
	r += sizeof (ulong);			/* count */

	return r;
}

/* Decode the entry list of a READDIR3 reply into the caller's buffer.
 *
 * On the wire the list is a chain of "value follows" booleans, each one
 * followed by an entry3. We build a linked list of entry3 structures in
 * `buffer', with the (zero terminated) name stored directly behind each
 * structure. Everything is bounded by `buflen', so a hostile or just
 * unexpectedly large reply cannot run past the end of the buffer.
 */
static bool_t
decode_dirlist3 (xdrs *x, readdir3res *rp)
{
	char *p = rp->buffer;
	char *end = rp->buffer + rp->buflen;
	entry3 *last = NULL;
	bool_t follows;

	rp->entries = NULL;

	for (;;)
	{
		entry3 *ep;
		const char *name;
		ulong namelen;
		char *q;

		if (!xdr_bool (x, &follows))
			return FALSE;

		if (!follows)
			break;

		/* place the entry structure, 4 byte aligned */
		q = (char *) ((((long) p) + 3L) & ~3L);
		if (q + sizeof (entry3) > end)
			return FALSE;

		ep = (entry3 *) q;
		p = q + sizeof (entry3);

		if (!xdr_uint64 (x, &ep->fileid))
			return FALSE;

		/* the name goes right behind the structure; check that
		 * the announced length still fits before copying
		 */
		if (x->length < (long) sizeof (ulong))
			return FALSE;

		namelen = *(ulong *) x->current;
		if (namelen > MAXNAMLEN)
			return FALSE;
		if (p + namelen + 1 > end)
			return FALSE;

		ep->name = p;
		name = p;
		if (!xdr_string (x, &name, MAXNAMLEN))
			return FALSE;

		p += namelen + 1;

		if (!xdr_uint64 (x, &ep->cookie))
			return FALSE;

		ep->nextentry = NULL;

		if (last)
			last->nextentry = ep;
		else
			rp->entries = ep;

		last = ep;
	}

	return xdr_bool (x, &rp->eof);
}

bool_t
xdr_readdir3res (xdrs *x, readdir3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->dir_attributes))
		return FALSE;

	rp->entries = NULL;
	rp->eof = FALSE;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_fixedopaq (x, rp->cookieverf, NFS3_COOKIEVERFSIZE))
		return FALSE;

	return decode_dirlist3 (x, rp);
}


/* ---------------------------------------------------------------- */
/* FSSTAT / FSINFO / PATHCONF / COMMIT                              */
/* ---------------------------------------------------------------- */

bool_t
xdr_fsstat3res (xdrs *x, fsstat3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->obj_attributes))
		return FALSE;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_uint64 (x, &rp->tbytes))
		return FALSE;
	if (!xdr_uint64 (x, &rp->fbytes))
		return FALSE;
	if (!xdr_uint64 (x, &rp->abytes))
		return FALSE;
	if (!xdr_uint64 (x, &rp->tfiles))
		return FALSE;
	if (!xdr_uint64 (x, &rp->ffiles))
		return FALSE;
	if (!xdr_uint64 (x, &rp->afiles))
		return FALSE;

	return xdr_ulong (x, &rp->invarsec);
}

bool_t
xdr_fsinfo3res (xdrs *x, fsinfo3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->obj_attributes))
		return FALSE;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_ulong (x, &rp->rtmax))
		return FALSE;
	if (!xdr_ulong (x, &rp->rtpref))
		return FALSE;
	if (!xdr_ulong (x, &rp->rtmult))
		return FALSE;
	if (!xdr_ulong (x, &rp->wtmax))
		return FALSE;
	if (!xdr_ulong (x, &rp->wtpref))
		return FALSE;
	if (!xdr_ulong (x, &rp->wtmult))
		return FALSE;
	if (!xdr_ulong (x, &rp->dtpref))
		return FALSE;
	if (!xdr_uint64 (x, &rp->maxfilesize))
		return FALSE;
	if (!xdr_nfstime3 (x, &rp->time_delta))
		return FALSE;

	return xdr_ulong (x, &rp->properties);
}

bool_t
xdr_pathconf3res (xdrs *x, pathconf3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_post_op_attr (x, &rp->obj_attributes))
		return FALSE;

	if (NFS3_OK != rp->status)
		return TRUE;

	if (!xdr_ulong (x, &rp->linkmax))
		return FALSE;
	if (!xdr_ulong (x, &rp->name_max))
		return FALSE;
	if (!xdr_bool (x, &rp->no_trunc))
		return FALSE;
	if (!xdr_bool (x, &rp->chown_restricted))
		return FALSE;
	if (!xdr_bool (x, &rp->case_insensitive))
		return FALSE;

	return xdr_bool (x, &rp->case_preserving);
}

bool_t
xdr_commit3args (xdrs *x, commit3args *ap)
{
	if (!xdr_nfs_fh3 (x, &ap->file))
		return FALSE;
	if (!xdr_uint64 (x, &ap->offset))
		return FALSE;

	return xdr_ulong (x, &ap->count);
}

long
xdr_size_commit3args (commit3args *ap)
{
	return xdr_size_nfs_fh3 (&ap->file) + 3L * sizeof (ulong);
}

bool_t
xdr_commit3res (xdrs *x, commit3res *rp)
{
	if (!xdr_enum (x, &rp->status))
		return FALSE;
	if (!xdr_wcc_data (x, &rp->file_wcc))
		return FALSE;

	if (NFS3_OK != rp->status)
		return TRUE;

	return xdr_fixedopaq (x, rp->verf, NFS3_WRITEVERFSIZE);
}
