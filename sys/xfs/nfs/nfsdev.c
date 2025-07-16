/*
 * Copyright 1993, 1994 by Ulrich K�hn. All rights reserved.
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
 * netdev.c networking filesystem driver, device driver functions
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
	
	if ((f->flags & O_TRUNC) && (ni->attr.size != 0))
	{
		/* The file has to be truncated...
		 * NOTE: if this file has been just created, the length field
		 *       is set to 0, so we do not get here.
		 */
		sattr attr;
		
		DEBUG (("nfs_open: truncating file to 0 length"));
		
		attr.mode = (ulong) -1L;
		attr.uid = (ulong) -1L;
		attr.gid = (ulong) -1L;
		attr.size = 0;
		attr.atime.seconds = attr.atime.useconds = (ulong) -1L;
		attr.mtime.seconds = attr.mtime.useconds = (ulong) -1L;
		
		r = do_sattr (&f->fc, &attr);
		if (r != 0)
		{
			DEBUG (("nfs_open : truncation to 0 failed -> EACCES"));
			return EACCES;
		}
	}
	
	if (ni->opt->flags & OPT_RO)
	{
		if (((f->flags & O_RWMODE) == O_RDWR)
			|| ((f->flags & O_RWMODE) == O_WRONLY))
		{
			DEBUG(("nfs_open: mount is read-only ->EACCES"));
			return EACCES;
		}
	}
	
	DEBUG (("nfs_open(%s) -> ok", ni->name));
	return 0;
}

/* BUG: should we really allways return EWRITE? Better might be the number of
 *      already written bytes.
 */
static long _cdecl
nfs_write (FILEPTR *f, const char *buf, long bytes)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	
	long pos;
	long written;
	
	/* TL: If we get an NFSERR_IO we'll reduce the wsize for this request
	 * and retry.
	 * It would be better to adjust wsize dynamically but this is better
	 * than nothing.
	 */
	long wsize = ni->opt->wsize;
	
	/* TL: If somehow mounted with too big wsize reduce it here. */
	if (wsize > MAXDATA)
		wsize = ni->opt->wsize = MAXDATA;
	
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
	
	TRACE(("nfs_write: writing %ld bytes to file '%s'", bytes, ni->name));
	
	written = 0;
	pos = f->pos;
	while (bytes > 0)
	{
		writeargs write_arg;
		attrstat write_res;
		xdrs x;
		
		MESSAGE *mreq;
		MESSAGE *mrep;
		MESSAGE m;
		
		long count = (bytes > wsize) ? wsize : bytes;
		long r;
		
		write_arg.file = ni->handle;
		write_arg.beginoffset = 0;
		write_arg.offset = pos;
		write_arg.totalcount = count;
		write_arg.data_val = buf + written;
		write_arg.data_len = count;
		
		mreq = alloc_message (&m, NULL, 0, xdr_size_writeargs (&write_arg));
		if (!mreq)
		{
			DEBUG(("nfs_write: could not allocate message buffer -> EWRITE"));
			return EWRITE;
		}
		
		xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
		if (!xdr_writeargs (&x, &write_arg))
		{
			DEBUG (("nfs_write: failed to encode arguments -> EWRITE"));
			return EWRITE;
		}
		
		r = rpc_request (&ni->opt->server, mreq, NFSPROC_WRITE, &mrep);
		if (r != 0)
		{
			DEBUG (("nfs_write: could not contact server -> EWRITE"));
			return EWRITE;
		}
		
		xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
		if (!xdr_attrstat (&x, &write_res))
		{
			free_message (mrep);
			
			DEBUG (("nfs_write: failed to decode results -> EWRITE"));
			return EWRITE;
		}
		
		free_message (mrep);
		
		if (write_res.status != NFS_OK)
		{
			/* TL: Reduce the wsize and try again */
			if (write_res.status == NFSERR_IO)
			{
				if (wsize > 1023)
				{
					wsize >>= 1;
					continue;
				}
				else
				{
					DEBUG(("nfs_write: write failed -> EWRITE"));
					return EWRITE;
				}
			}
		}
		
		fattr2xattr (&write_res.attrstat_u.attributes, &ni->attr);
		ni->stamp = get_timestamp ();
		
		written += count;
		pos += count;
		bytes -= count;
	}
	
	f->pos = pos;
	
	TRACE (("nfs_write(%s) -> %ld", ni->name, written));
	return written;
}

/* BUG: should we really allways return EREAD? Better might be the number of
 *      already read bytes.
 */
static long _cdecl
nfs_read (FILEPTR *f, char *buf, long bytes)
{
	NFS_INDEX *ni = (NFS_INDEX *) f->fc.index;
	long pos;
	long read;
	
	/* TL: If we get an NFSERR_IO try to reduce the rsize for this request
	 * and retry.
	 * It would be better to adjust rsize dynamically but this is better
	 * than nothing.
	 */
	long rsize = ni->opt->rsize;
	
	/* TL: If somehow mounted with too big rsize reduce it here */
	if (rsize > MAXDATA)
		rsize = ni->opt->rsize = MAXDATA;
	
	if (ROOT_INDEX == ni)
	{
		DEBUG (("nfs_read: attempt to read root dir! -> 0"));
		return 0;
	}
	
	TRACE (("nfs_read: reading %ld bytes for file '%s'", bytes, ni->name));
	
	read = 0;
	pos = f->pos;
	while (bytes > 0)
	{
		char req_buf[READBUFSIZE];
		
		readargs read_arg;
		readres read_res;
		xdrs x;
		
		MESSAGE *mreq;
		MESSAGE *mrep;
		MESSAGE m;
		
		long count = (bytes > ni->opt->rsize) ? ni->opt->rsize : bytes;
		long r;
		
		read_arg.file = ni->handle;
		read_arg.offset = pos;
		read_arg.count = count;
		read_arg.totalcount = count;
		
		mreq = alloc_message (&m, req_buf, READBUFSIZE, xdr_size_readargs(&read_arg));
		if (!mreq)
		{
			DEBUG (("nfs_read: failed to allocate message buffer, -> EREAD"));
			return EREAD;
		}
		
		xdr_init (&x, mreq->data, mreq->data_len, XDR_ENCODE, NULL);
		if (!xdr_readargs (&x, &read_arg))
		{
			DEBUG (("nfs_read: failed to encode arguments, -> EREAD"));
			return EREAD;
		}
		
		r = rpc_request (&ni->opt->server, mreq, NFSPROC_READ, &mrep);
		if (r != 0)
		{
			DEBUG (("nfs_read: failed to contact server, -> EREAD"));
			return EREAD;
		}

		read_res.readres_u.read_ok.data_val = buf + read;
		
		xdr_init (&x, mrep->data, mrep->data_len, XDR_DECODE, NULL);
		if (!xdr_readres (&x, &read_res))
		{
			free_message (mrep);
			
			DEBUG (("nfs_read: could not decode results, -> EREAD"));
			return EREAD;
		}
		
		free_message (mrep);
		
		if (read_res.status != NFS_OK)
		{
			/* TL: Try to reduce the rsize */
			if (read_res.status == NFSERR_IO)
			{
				if (rsize > 1023)
				{
					rsize >>= 1;
					continue;
				}
				else
				{
					/* read failed for some reason */
					DEBUG (("nfs_read: request failed, -> EREAD"));
					return EREAD;
				}
			}
		}
		
		r = read_res.readres_u.read_ok.data_len;
		read += r;
		pos += r;
		bytes -= r;
		
		fattr2xattr (&read_res.readres_u.read_ok.attributes, &ni->attr);
		ni->stamp = get_timestamp ();
		
		if (r < count)
		{
			/* no more data */
			
			DEBUG (("nfs_read: read only %ld bytes, -> ok", r));
			break;
		}
	}
	
	f->pos = pos;
	return read;
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
			
			long r = nfs_getxattr (&f->fc, NULL);
			if (r)
			{
				DEBUG (("nfs_lseek: nfs_getxattr failed while SEEK_END, -> %ld", r));
				return r;
			}
			
			ni = (NFS_INDEX *) f->fc.index;
			if (where <  - ni->attr.size)  /* seek before beginning of file */
				return EBADARG;
			
			return (f->pos = ni->attr.size + where);
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
			long r;
			
			r = nfs_getxattr (&f->fc, NULL);
			if (r)
			{
				DEBUG (("nf_ioctl: cant get file attributes, -> %ld", r));
				return r;
			}
			
			*(long *) arg = MIN (ni->attr.size-f->pos, 0);
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
			sattr attr;
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
			
			attr.uid = (ulong) -1L;
			attr.gid = (ulong) -1L;
			attr.mode = (ulong) -1L;
			attr.size = (ulong) -1L;
			
			if (arg)
			{
				if (mode == FUTIME_UTC)
				{
					long *timeptr = arg;
					
					attr.atime.seconds = timeptr[0];
					attr.atime.useconds = 0;
					attr.mtime.seconds = timeptr[1];
					attr.mtime.useconds = 0;
				}
				else
				{
					MUTIMBUF *buf = arg;
					
					attr.atime.seconds = unixtime (buf->actime, buf->acdate);
					attr.atime.useconds = 0;
					attr.mtime.seconds = unixtime (buf->modtime, buf->moddate);
					attr.mtime.useconds = 0;
				}
			}
			else
			{
				attr.atime.seconds = CURRENT_TIME;
				attr.atime.useconds = 0;
				attr.mtime.seconds = CURRENT_TIME;
				attr.mtime.useconds = 0;
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
				DEBUG (("nfs_datime: nfs_getattr failed, -> %ld", r));
				return r;
			}
			
			timeptr[0] = ni->attr.atime.time;
			timeptr[1] = ni->attr.atime.date;
			
			break;
		}
		case 1:
		{
			/* set access and modification time of the file; we
			 * dont have any chance to change the creation time,
			 * as the nfs protcol does not specify this
			 */
			sattr attr;
			
			attr.uid = (ulong) -1L;
			attr.gid = (ulong) -1L;
			attr.mode = (ulong) -1L;
			attr.size = (ulong) -1L;
			
			attr.atime.seconds = *((ulong *) timeptr);
			attr.atime.useconds = 0;
			
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
	TRACE (("nfs_close -> ok"));
	
	/* nothing to do */
	return 0;
}

static long _cdecl
nfs_select (FILEPTR *f, long proc, int mode)
{
	TRACE (("nfs_select"));
	return 1;
}

static void _cdecl
nfs_unselect (FILEPTR *f, long proc, int mode)
{
	TRACE (("nfs_unselect"));
	/* do nothing */
}
