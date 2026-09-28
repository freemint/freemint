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
 * nfsdev.c networking filesystem driver, device driver functions
 */


# include "nfsdev.h"

# include "mint/ioctl.h"

# include "nfssys.h"
# include "nfsutil.h"
# include "sock_ipc.h"


static long	_cdecl nfs_open		(FILEPTR *f);
static long	_cdecl nfs_write	(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl nfs_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl nfs_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl nfs_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl nfs_datime	(FILEPTR *f, ushort *time, int flag);
static long	_cdecl nfs_close	(FILEPTR *f, int pid);
static long	_cdecl nfs_select	(FILEPTR *f, long proc, int mode);
static void	_cdecl nfs_unselect	(FILEPTR *f, long proc, int mode);

DEVDRV nfs_device =
{
	nfs_open, nfs_write, nfs_read, nfs_lseek, nfs_ioctl, nfs_datime,
	nfs_close, nfs_select, nfs_unselect,
};


static long _cdecl
nfs_open (FILEPTR *f)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	long r;

	DEBUG (("nfs_open(%s, 0x%x)", ni->name, f->flags));

	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_open: root dir is not a file, -> EACCES"));
		return EACCES;
	}

	/* A file that was just created may not have a handle yet: NFS3
	 * servers are allowed to answer CREATE3 without one. Fetch it now,
	 * before anybody tries to read or write through this FILEPTR.
	 */
	if (nfs_get_handle (ni) != 0)
	{
		DEBUG (("nfs_open(%s): no file handle -> ENOENT", ni->name));
		return ENOENT;
	}

	if (ni->opt->flags & OPT_RO)
	{
		if (((f->flags & O_RWMODE) == O_RDWR)
			|| ((f->flags & O_RWMODE) == O_WRONLY))
		{
			DEBUG (("nfs_open: mount is read-only ->EACCES"));
			return EACCES;
		}
	}

	if ((f->flags & O_TRUNC) && (ni->size != 0))
	{
		/* The file has to be truncated...
		 * NOTE: if this file has been just created, the length field
		 *       is set to 0, so we do not get here.
		 */
		sattr3 attr;

		DEBUG (("nfs_open: truncating file to 0 length"));

		sattr3_init (&attr);
		attr.set_size = TRUE;
		attr.size = 0;

		r = do_sattr (&f->fc, &attr);
		if (r != 0)
		{
			DEBUG (("nfs_open : truncation to 0 failed -> %ld", r));
			return r;
		}
	}

	DEBUG (("nfs_open(%s) -> ok", ni->name));
	return 0;
}


/* Write `bytes' bytes at file offset `pos'.
 *
 * `stable' selects the NFS3 write mode; with UNSTABLE the server may
 * keep the data in volatile memory until a COMMIT3 arrives, which is
 * what makes NFS3 writes so much faster than NFS2's, where every single
 * request had to hit the disk.
 *
 * If the caller passes a `verf' buffer, the write verifier of the first
 * reply is stored there; it has to be compared with the one COMMIT3
 * returns to notice a server reboot.
 *
 * Returns the number of bytes written, or a negative error code.
 */
static long
write_range (FILEPTR *f, const char *buf, long bytes, long pos,
	     long stable, char *verf, int *all_stable)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	long wsize = ni->opt->wsize;
	long written = 0;
	int have_verf = 0;

	/* If somehow mounted with too big wsize reduce it here. */
	if (wsize > ni->opt->wtmax)
		wsize = ni->opt->wsize = ni->opt->wtmax;
	if (wsize <= 0)
		wsize = ni->opt->wsize = DEFAULT_WSIZE;

	*all_stable = 1;

	while (bytes > 0)
	{
		write3args write_arg;
		write3res write_res;
		xdrs x;

		MESSAGE *mreq;
		MESSAGE *mrep;
		MESSAGE m;

		long count = (bytes > wsize) ? wsize : bytes;
		long r;

		write_arg.file = ni->handle;
		write_arg.offset = (uint64) (ulong) pos;
		write_arg.count = count;
		write_arg.stable = stable;
		write_arg.data_val = buf + written;
		write_arg.data_len = count;

		mreq = alloc_message (&m, NULL, 0, xdr_size_write3args (&write_arg));
		if (!mreq)
		{
			DEBUG (("nfs_write: could not allocate message buffer"));
			return written ? written : EWRITE;
		}

		xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
		if (!xdr_write3args (&x, &write_arg))
		{
			DEBUG (("nfs_write: failed to encode arguments -> EWRITE"));
			free_message (mreq);
			return written ? written : EWRITE;
		}

		r = rpc_request (&ni->opt->server, mreq, NFSPROC3_WRITE, &mrep);
		if (r != 0)
		{
			DEBUG (("nfs_write: could not contact server -> %ld", r));
			if (written)
				return written;
			return IS_TRANSPORT_ERROR (r) ? r : EWRITE;
		}

		xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
		if (!xdr_write3res (&x, &write_res))
		{
			free_message (mrep);

			DEBUG (("nfs_write: failed to decode results -> EWRITE"));
			return written ? written : EWRITE;
		}

		free_message (mrep);

		update_index_attr (ni, &write_res.file_wcc.after);

		if (write_res.status != NFS3_OK)
		{
			/* Reduce the wsize and try again; some servers
			 * answer with NFS3ERR_IO when a request is too
			 * big for their liking.
			 */
			if (write_res.status == NFS3ERR_IO && wsize > 1024)
			{
				wsize >>= 1;
				continue;
			}

			DEBUG (("nfs_write: write failed rpc->%ld", write_res.status));
			return written ? written : nfs3_error (write_res.status, EWRITE);
		}

		if (write_res.count == 0)
		{
			DEBUG (("nfs_write: server wrote nothing -> EWRITE"));
			return written ? written : EWRITE;
		}

		/* the server is free to write less than we asked for */
		if (write_res.count < (ulong) count)
			count = (long) write_res.count;

		if (write_res.committed == UNSTABLE)
			*all_stable = 0;

		if (verf && !have_verf)
		{
			memcpy (verf, write_res.verf, NFS3_WRITEVERFSIZE);
			have_verf = 1;
		}

		written += count;
		pos += count;
		bytes -= count;
	}

	return written;
}

static long _cdecl
nfs_write (FILEPTR *f, const char *buf, long bytes)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	char verf[NFS3_WRITEVERFSIZE];
	long start = f->pos;
	long written;
	int all_stable;

	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_write: attempt to write root dir! -> 0"));
		return 0;
	}

	if (ni->opt->flags & OPT_RO)
	{
		DEBUG (("nfs_write: mount is read-only -> EACCES"));
		return EACCES;
	}

	if (bytes <= 0)
		return 0;

	if (nfs_get_handle (ni) != 0)
		return EWRITE;

	TRACE (("nfs_write: writing %ld bytes to file '%s'", bytes, ni->name));

	written = write_range (f, buf, bytes, start, UNSTABLE, verf, &all_stable);
	if (written < 0)
		return written;

	if (!all_stable && written > 0)
	{
		/* The data is only in the server's memory so far. Do NOT
		 * commit here: applications write in small pieces, and one
		 * COMMIT3 per write() call doubles the number of round trips
		 * -- which is exactly what made writing four times slower
		 * than reading. Remember the state instead and flush once in
		 * nfs_close(), the way RFC 1813 intends it.
		 */
		if (!ni->wdirty)
		{
			memcpy (ni->wverf, verf, NFS3_WRITEVERFSIZE);
			ni->wdirty = 1;
		}
	}

	f->pos = start + written;

	if (ni->size < (uint64) (ulong) f->pos)
		ni->size = (uint64) (ulong) f->pos;

	TRACE (("nfs_write(%s) -> %ld", ni->name, written));
	return written;
}

static long _cdecl
nfs_read (FILEPTR *f, char *buf, long bytes)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	long pos;
	long total;
	long rsize;

	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_read: attempt to read root dir! -> 0"));
		return 0;
	}

	if (nfs_get_handle (ni) != 0)
	{
		ALERT (("nfs3: nfs_read(%s): no file handle", ni->name));
		return EREAD;
	}

	rsize = ni->opt->rsize;
	if (rsize > ni->opt->rtmax)
		rsize = ni->opt->rsize = ni->opt->rtmax;
	if (rsize <= 0)
		rsize = ni->opt->rsize = DEFAULT_RSIZE;

	TRACE (("nfs_read: reading %ld bytes for file '%s'", bytes, ni->name));

	total = 0;
	pos = f->pos;
	while (bytes > 0)
	{
		long req_buf [READBUFSIZE / sizeof (long)];

		read3args read_arg;
		read3res read_res;
		xdrs x;

		MESSAGE *mreq;
		MESSAGE *mrep;
		MESSAGE m;

		long count = (bytes > rsize) ? rsize : bytes;
		long r;

		read_arg.file = ni->handle;
		read_arg.offset = (uint64) (ulong) pos;
		read_arg.count = count;

		mreq = alloc_message (&m, (char *) req_buf, READBUFSIZE,
				      xdr_size_read3args (&read_arg));
		if (!mreq)
		{
			ALERT (("nfs3: nfs_read(%s): out of memory for a %ld "
				"byte request", ni->name,
				xdr_size_read3args (&read_arg)));
			return total ? total : EREAD;
		}

		xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
		if (!xdr_read3args (&x, &read_arg))
		{
			ALERT (("nfs3: nfs_read(%s): cannot encode request "
				"(fh %ld bytes, buffer %ld) - please report",
				ni->name, ni->handle.len, mreq->data_len));
			free_message (mreq);
			return total ? total : EREAD;
		}

		r = rpc_request (&ni->opt->server, mreq, NFSPROC3_READ, &mrep);
		if (r != 0)
		{
			DEBUG (("nfs_read: failed to contact server, -> %ld", r));
			if (total)
				return total;
			return IS_TRANSPORT_ERROR (r) ? r : EREAD;
		}

		read_res.data_val = buf + total;
		read_res.data_max = count;

		xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
		if (!xdr_read3res (&x, &read_res))
		{
			free_message (mrep);

			DEBUG (("nfs_read: could not decode results, -> EREAD"));
			return total ? total : EREAD;
		}

		free_message (mrep);

		update_index_attr (ni, &read_res.file_attributes);

		if (read_res.status != NFS3_OK)
		{
			/* Try to reduce the rsize; see nfs_write() */
			if (read_res.status == NFS3ERR_IO && rsize > 1024)
			{
				rsize >>= 1;
				continue;
			}

			DEBUG (("nfs_read: request failed rpc->%ld", read_res.status));
			return total ? total : nfs3_error (read_res.status, EREAD);
		}

		r = (long) read_res.data_len;
		total += r;
		pos += r;
		bytes -= r;

		/* NFS3 tells us explicitly whether we reached the end of
		 * the file; NFS2 could only be guessed at by a short read
		 */
		if (read_res.eof || r == 0)
		{
			DEBUG (("nfs_read: eof after %ld bytes", total));
			break;
		}
	}

	f->pos = pos;
	return total;
}

static long _cdecl
nfs_lseek (FILEPTR *f, long where, int whence)
{
	TRACE (("nfs_lseek(.., %ld, %d)", where, whence));

	switch (whence)
	{
		case SEEK_SET:
		{
			if (where < 0)
				return EBADARG;

			f->pos = where;
			return f->pos;
		}
		case SEEK_CUR:
		{
			if (f->pos + where < 0)
				return EBADARG;

			f->pos += where;
			return f->pos;
		}
		case SEEK_END:
		{
			NFS_INDEX *ni;
			long size;

			long r = nfs_getxattr (&f->fc, NULL);
			if (r)
			{
				DEBUG (("nfs_lseek: nfs_getxattr failed while SEEK_END, -> %ld", r));
				return r;
			}

			ni = (NFS_INDEX *) f->fc.index;

			/* files may well be bigger than 2 GB on an NFS3
			 * server, but MiNT's file position is a signed long
			 */
			size = clamp64 (ni->size);

			if (where < -size)  /* seek before beginning of file */
				return EBADARG;

			return (f->pos = size + where);
		}
	}

	return EBADARG;
}

static long _cdecl
nfs_ioctl (FILEPTR *f, int mode, void *arg)
{
	switch (mode)
	{
		case FIONREAD:
		{
			NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
			long r, avail;

			r = nfs_getxattr (&f->fc, NULL);
			if (r)
			{
				DEBUG (("nfs_ioctl: cant get file attributes, -> %ld", r));
				return r;
			}

			avail = clamp64 (ni->size) - f->pos;
			*(long *) arg = (avail > 0) ? avail : 0;
			break;
		}
		case FIONWRITE:
		{
			*(long *) arg = ((NFS_INDEX *) f->fc.index)->opt->wsize;
			break;
		}
		case FUTIME:
		case FUTIME_UTC:
		{
			NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
			sattr3 attr;
			int uid;

			/* The owner or super-user can always touch, others only
			 * if timeptr == 0 and open for writing
			 */
			uid = p_geteuid ();
			if (uid && (uid != ni->attr.uid)
				&& (arg || ((f->flags & O_RWMODE) == O_RDONLY)))
			{
				return EACCES;
			}

			sattr3_init (&attr);

			if (arg)
			{
				attr.set_atime = SET_TO_CLIENT_TIME;
				attr.set_mtime = SET_TO_CLIENT_TIME;

				if (native_utc || (mode == FUTIME_UTC))
				{
					long *timeptr = arg;

					attr.atime.seconds = timeptr[0];
					attr.mtime.seconds = timeptr[1];
				}
				else
				{
					MUTIMBUF *buf = arg;

					attr.atime.seconds = unixtime (buf->actime, buf->acdate);
					attr.mtime.seconds = unixtime (buf->modtime, buf->moddate);
				}
			}
			else
			{
				/* NFS3 can ask the server to use its own clock,
				 * which avoids a skewed time stamp when the
				 * Atari's clock is off
				 */
				attr.set_atime = SET_TO_SERVER_TIME;
				attr.set_mtime = SET_TO_SERVER_TIME;
			}

			return do_sattr (&f->fc, &attr);
		}
		default:
		{
			return ENOSYS;
		}
	}

	return E_OK;
}

static long _cdecl
nfs_datime (FILEPTR *f, ushort *timeptr, int flag)
{
	switch (flag)
	{
		case 0:
		{
			NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
			long r;

			/* update cache if necessary */
			r = nfs_getxattr (&f->fc, NULL);
			if (r != 0)
			{
				DEBUG (("nfs_datime: nfs_getxattr failed, -> %ld", r));
				return r;
			}

			timeptr[0] = ni->attr.atime;
			timeptr[1] = ni->attr.adate;

			break;
		}
		case 1:
		{
			/* set access and modification time of the file; we
			 * dont have any chance to change the creation time,
			 * as the nfs protcol does not specify this
			 */
			sattr3 attr;

			sattr3_init (&attr);
			attr.set_atime = SET_TO_CLIENT_TIME;
			attr.set_mtime = SET_TO_CLIENT_TIME;

			if (native_utc)
				attr.atime.seconds = *((long *) timeptr);
			else
				attr.atime.seconds = unixtime (timeptr[0], timeptr[1]);

			attr.mtime = attr.atime;

			return do_sattr (&f->fc, &attr);
		}
		default:
		{
			return EBADARG;
		}
	}

	return E_OK;
}

static long _cdecl
nfs_close (FILEPTR *f, int pid)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	char cverf[NFS3_WRITEVERFSIZE];
	long r;

	(void) pid;

	TRACE (("nfs_close"));

	if (ROOT_INDEX == ni || !ni->wdirty)
		return 0;

	/* Flush the unstable data of this file. A count of zero means
	 * "from offset to the end of file" (RFC 1813, 3.3.21).
	 */
	r = do_commit (&f->fc, 0, 0, cverf);
	if (r != E_OK)
	{
		ALERT (("nfs3: COMMIT3 of '%s' failed -> %ld", ni->name, r));
		return r;
	}

	ni->wdirty = 0;

	if (memcmp (ni->wverf, cverf, NFS3_WRITEVERFSIZE) != 0)
	{
		/* The server rebooted between our writes and this commit,
		 * so the data is gone. We have no page cache to write it
		 * again from, so all we can do is report the loss -- which
		 * is why write errors surface at close() on NFS.
		 */
		ALERT (("nfs3: server lost unstable data of '%s'", ni->name));
		return EIO;
	}

	TRACE (("nfs_close -> ok"));
	return 0;
}

static long _cdecl
nfs_select (FILEPTR *f, long proc, int mode)
{
	(void) f;
	(void) proc;
	(void) mode;

	TRACE (("nfs_select"));
	return 1;
}

static void _cdecl
nfs_unselect (FILEPTR *f, long proc, int mode)
{
	(void) f;
	(void) proc;
	(void) mode;

	TRACE (("nfs_unselect"));
	/* do nothing */
}
