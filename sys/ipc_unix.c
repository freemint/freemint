/*
 * This file implements the UNIX communication domain.
 * Neither MSG_OOB nor MSG_PEEK flags for send/recv
 * work on UNIX sockets. The getsockname/getpeername calls
 * are not implemented (they wouldn't make sense, because
 * filenames can be relative to the process' current
 * directory).
 *
 * 10/09/93, kay roemer.
 */

# include "ipc_unix.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/signal.h"
# include "mint/stat.h"
# include "mint/emu_tos.h"

# include "dev-null.h"
# include "dosdir.h"
# include "dosfile.h"
# include "ipc_socketutil.h"
# include "ipc_unix_cache.h"
# include "ipc_unix_dgram.h"
# include "ipc_unix_stream.h"
# include "kmemory.h"
# include "proc.h"


# define UN_MINBUF	8192
# define UN_MAXBUF	32000


static long	unix_attach	(struct socket *, short);
static long	unix_dup	(struct socket *, struct socket *);
static long	unix_abort	(struct socket *, enum so_state);
static long	unix_detach	(struct socket *);
static long	unix_bind	(struct socket *, struct sockaddr *, short);
static long	unix_connect	(struct socket *, const struct sockaddr *, short, short);
static long	unix_socketpair	(struct socket *, struct socket *);
static long	unix_accept	(struct socket *, struct socket *, short);
static long	unix_getname	(struct socket *, struct sockaddr *, short *, short);
static long	unix_select	(struct socket *, short, long);
static long	unix_ioctl	(struct socket *, short, void *);
static long	unix_listen	(struct socket *, short);
static long	unix_send	(struct socket *, const struct iovec *, short, short, short, const struct sockaddr *, short);
static long	unix_recv	(struct socket *, const struct iovec *, short, short, short, struct sockaddr *, short *);
static long	unix_shutdown	(struct socket *, short);
static long	unix_setsockopt	(struct socket *, short, short, char *, long);
static long	unix_getsockopt	(struct socket *, short, short, char *, long *);

struct dom_ops unix_ops =
{
	domain:		AF_UNIX,
	next:		NULL,

	attach:		unix_attach,
	dup:		unix_dup,
	abort:		unix_abort,
	detach:		unix_detach,
	bind:		unix_bind,
	connect:	unix_connect,
	socketpair:	unix_socketpair,
	accept:		unix_accept,
	getname:	unix_getname,
	select:		unix_select,
	ioctl:		unix_ioctl,
	listen:		unix_listen,
	send:		unix_send,
	recv:		unix_recv,
	shutdown:	unix_shutdown,
	setsockopt:	unix_setsockopt,
	getsockopt:	unix_getsockopt
};

static long
unix_attach (struct socket *so, short protocol)
{
	static long un_index = 0;
	struct un_data *undata;

	if (so->data)
	{
		DEBUG (("unix: unix_attach: so->data != NULL"));
		return EINTERNAL;
	}

	if (protocol != 0)
	{
		DEBUG (("unix: unix_attach: invalid protocol %d", protocol));
		return EPROTONOSUPPORT;
	}

	if (so->type != SOCK_STREAM && so->type != SOCK_DGRAM)
	{
		DEBUG (("unix: unix_attach: unsupp. socket type %d", so->type));
		return EPROTOTYPE;
	}

	undata = kmalloc (sizeof (struct un_data));
	if (!undata)
	{
		DEBUG (("unix: unix_attach: out of memory"));
		return ENOMEM;
	}

	undata->buf = kmalloc (UN_MINBUF);
	if (!undata->buf)
	{
		DEBUG (("unix: unix_attach: out of memory"));
		kfree (undata);
		return ENOMEM;
	}

	undata->proto =   protocol;
	undata->sock =    so;
	undata->flags =   0;
	undata->index =   un_index++;
	undata->index2 =  0;
	undata->head =    0;
	undata->tail =    0;
	undata->buflen =  UN_MINBUF;
	undata->backlog = 0;
	undata->next =    0;
	undata->addrlen = 0;

	so->data = undata;
	un_put (undata);

	return 0;
}

static long
unix_dup (struct socket *newso, struct socket *oldso)
{
	struct un_data *undata = oldso->data;
	return unix_attach (newso, undata->proto);
}

static long
unix_abort (struct socket *so, enum so_state ostate)
{
	/* wake anyone waiting on the socket */
	wake (IO_Q, (long) so);
	so_wakersel (so);
	so_wakewsel (so);
	so_wakexsel (so);

	return 0;
}

static long
unix_detach (struct socket *so)
{
	struct un_data *undata = so->data;

	if (!undata)
	{
		DEBUG (("unix: unix_detach: no data attached to socket"));
		return 0;
	}

	un_remove (undata);
	so->data = 0;
	kfree (undata->buf);
	kfree (undata);

	return 0;
}

static long
unix_bind (struct socket *so, struct sockaddr *addr, short addrlen)
{
	struct un_data *undata = so->data;
	long r, fd, index;
	XATTR attr;

	if (!addr)
		return EDESTADDRREQ;

	if (addrlen <= UN_PATH_OFFSET || addrlen > UN_ADDR_SIZE)
	{
		DEBUG (("unix: unix_bind: invalid addrlen %d", addrlen));
		return EINVAL;
	}

	if (undata->flags & UN_ISBOUND)
	{
		DEBUG (("unix: unix_bind: already bound"));
		return EINVAL;
	}

	memcpy (&undata->addr, addr, addrlen);
	undata->addr.sun_path[
		MIN(sizeof(struct sockaddr_un) - 1 - UN_PATH_OFFSET, addrlen - UN_PATH_OFFSET)] = '\0';

	if (undata->addr.sun_family != AF_UNIX)
	{
		DEBUG (("unix: unix_bind: domain not AF_UNIX"));
		return EAFNOSUPPORT;
	}

	r = sys_f_xattr (1, undata->addr.sun_path, &attr);
	if (r == 0)
	{
		DEBUG (("unix_bind: Fxattr(%s), file exists", undata->addr.sun_path));
		return EADDRINUSE;
	}
	/* Invalidate cache entries referring to the same file. */
	r = un_cache_remove (undata->addr.sun_path);
	if (r < 0) {
		return r;
	}

	/* To do the creat(), the user must have write access for the
	 * directory containing the file.
	 */
	fd = sys_f_create (undata->addr.sun_path, FA_HIDDEN);
	if (fd < 0)
	{
		DEBUG (("unix: unix_bind: could not create file %s",
			undata->addr.sun_path));
		if (fd == EEXIST) 
			return EADDRINUSE;
		return fd;
	}

	/* We write the index of the socket to the socket file, because
	 * Fxattr() is broken on Tosfs (the index field is junk).
	 */
	index = UN_INDEX (undata);
	r = sys_f_write (fd, sizeof (index), (char *) &index);
	sys_f_close (fd);

	if (r >= 0 && r != sizeof (index))
		r = EACCES;

	if (r < 0)
	{
		DEBUG (("unix: unix_bind: could not write idx to file %s",
			undata->addr.sun_path));
		return r;
	}

	/* If the filesystem cannot change mode to 0777, the bits that cannot
	 * be changed are silently ignored. Note that there is no S_IFSOCK and
	 * the like, the sockets file type is S_IFREG.
	 */
	r = sys_f_chmod (undata->addr.sun_path, 0777);
	if (r < 0)
	{
		DEBUG (("unix: unix_bind: could not chmod %s to 0777",
			undata->addr.sun_path));
		return r;
	}

	undata->addrlen = addrlen;
	undata->flags |= UN_ISBOUND;

	return 0;
}

static long
unix_connect (struct socket *so, const struct sockaddr *addr, short addrlen, short nonblock)
{
	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_connect (so, addr, addrlen, nonblock);

		case SOCK_DGRAM:
			return unix_dgram_connect (so, addr, addrlen, nonblock);

		default:
			break;
	}

	return EPROTOTYPE;
}

static long
unix_socketpair (struct socket *so1, struct socket *so2)
{
	switch (so1->type)
	{
		case SOCK_STREAM:
			return unix_stream_socketpair (so1, so2);

		case SOCK_DGRAM:
			return unix_dgram_socketpair (so1, so2);

		default:
			break;
	}

	return EPROTOTYPE;
}

static long
unix_accept (struct socket *server, struct socket *newso, short nonblock)
{
	long r;

	/* BSD seems to allow accept() on DGRAM sockets ... */
	if (server->type != SOCK_STREAM)
	{
		DEBUG (("unix: unix_accept: connectionless socket"));
		return EOPNOTSUPP;
	}

	r = so_accept (server, newso, nonblock);
	if (r < 0)
	{
		DEBUG (("unix: unix_accept: no client accepted"));
		return r;
	}

	return 0;
}

static long
unix_getname (struct socket *so, struct sockaddr *addr, short *addrlen, short peer)
{
	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_getname (so, addr, addrlen, peer);

		case SOCK_DGRAM:
			return unix_dgram_getname (so, addr, addrlen, peer);

		default:
			break;
	}

	return EOPNOTSUPP;
}

static long
unix_select (struct socket *so, short how, long proc)
{
	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_select (so, how, proc);

		case SOCK_DGRAM:
			return unix_dgram_select (so, how, proc);

		default:
			break;
	}

	return 1;
}

static long
unix_ioctl (struct socket *so, short cmd, void *buf)
{
	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_ioctl (so, cmd, buf);

		case SOCK_DGRAM:
			return unix_dgram_ioctl (so, cmd, buf);

		default:
			break;
	}

	return EPROTOTYPE;
}

static long
unix_listen (struct socket *so, short backlog)
{
	struct un_data *undata = so->data;

	/* BSD seems to allow listen() on DGRAM sockets ... */
	if (so->type != SOCK_STREAM)
	{
		DEBUG (("unix: unix_listen: connectionless socket"));
		return 0;
	}

	if (!(undata->flags & UN_ISBOUND))
	{
		DEBUG (("unix: unix_listen: listening socket is not bound"));
		return EDESTADDRREQ;
	}

	undata->backlog = backlog;
	return 0;
}

static long
unix_send (struct socket *so, const struct iovec *iov, short niov, short nonblock,
		short flags, const struct sockaddr *addr, short addrlen)
{
	if (flags)
	{
		DEBUG (("unix: unix_send: flags not zero"));
		return EOPNOTSUPP;
	}

	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_send (so, iov, niov, nonblock, flags,
					addr, addrlen);

		case SOCK_DGRAM:
			return unix_dgram_send (so, iov, niov, nonblock, flags,
					addr, addrlen);

		default:
			break;
	}

	return EPROTOTYPE;
}

static long
unix_recv (struct socket *so, const struct iovec *iov, short niov, short nonblock,
		short flags, struct sockaddr *addr, short *addrlen)
{
	if (flags)
	{
		DEBUG (("unix: unix_recv: flags not zero"));
		return EOPNOTSUPP;
	}

	switch (so->type)
	{
		case SOCK_STREAM:
			return unix_stream_recv (so, iov, niov, nonblock, flags,
				     addr, addrlen);

		case SOCK_DGRAM:
			return unix_dgram_recv (so, iov, niov, nonblock, flags,
				    addr, addrlen);

		default:
			break;
	}

	return EPROTOTYPE;
}

static long
unix_shutdown (struct socket *so, short how)
{
# define SO_CANTDOMORE	(SO_CANTSNDMORE|SO_CANTRCVMORE)

	/* Note that sock_shutdown() has already set so->flags for us. */
	if ((so->flags & SO_CANTDOMORE) == SO_CANTDOMORE)
	{
		DEBUG (("unix_shutdown: releasing socket"));
		so_release (so);
		return 0;
	}

	switch (how)
	{
		case 0:
			{
				struct un_data *undata = so->data;
				undata->head = undata->tail = 0;

				if (so->conn)
					so->conn->flags |= SO_CANTSNDMORE;
			}
			break;
		case 1:
			if (so->conn)
				so->conn->flags |= SO_CANTSNDMORE;
			break;
		case 2:
			if (so->conn)
				so->conn->flags |= SO_CANTDOMORE;
			break;
	}

	wake (IO_Q, (long) so);
	so_wakersel (so);
	so_wakewsel (so);
	so_wakexsel (so);
	if (so->conn)
	{
		wake (IO_Q, (long) so->conn);
		so_wakersel (so->conn);
		so_wakewsel (so->conn);
		so_wakexsel (so->conn);
	}

	return 0;
}

static long
unix_setsockopt (struct socket *so, short level, short optname, char *optval, long optlen)
{
	struct un_data *undata;
	long newsize;

	if (level != SOL_SOCKET)
		return ENOPROTOOPT;

	switch (optname)
	{
		case SO_SNDBUF:
		{
			if (!optval || optlen < sizeof (long) ||
			    so->state != SS_ISCONNECTED)
				return EINVAL;

			undata = so->conn->data;
			newsize = *(long *) optval;

			if (newsize < UN_MINBUF) newsize = UN_MINBUF;
			else if (newsize > UN_MAXBUF) newsize = UN_MAXBUF;

			return un_resize (undata, newsize);
		}
		case SO_RCVBUF:
		{
			if (!optval || optlen < sizeof (long))
				return EINVAL;

			undata = so->data;
			newsize = *(long *) optval;

			if (newsize < UN_MINBUF) newsize = UN_MINBUF;
			else if (newsize > UN_MAXBUF) newsize = UN_MAXBUF;

			return un_resize (undata, newsize);
		}
		case SO_DEBUG:
		default:
			break;
	}

	return EOPNOTSUPP;
}

static long
unix_getsockopt (struct socket *so, short level, short optname, char *optval, long *optlen)
{
	long ret = 0;

	if (level != SOL_SOCKET)
		return EOPNOTSUPP;

	switch (optname)
	{
		case SO_DEBUG:
			return ENOPROTOOPT;

		case SO_ERROR:
			ret = 0;
			break;

		case SO_TYPE:
			ret = so->type;
			break;

		case SO_SNDBUF:
			if (so->state != SS_ISCONNECTED)
			{
				DEBUG (("unix: unix_getsockopt: not connected"));
				return EINVAL;
			}
			ret = ((struct un_data *) so->conn->data)->buflen;
			break;

		case SO_RCVBUF:
			ret = ((struct un_data *) so->data)->buflen;
			break;

		default:
			return EOPNOTSUPP;
	}

	if (!optval || !optlen || *optlen < sizeof (long))
	{
		DEBUG (("unix: unix_getsockopt: invalid optval or optlen"));
		return EINVAL;
	}

	*(long *) optval = ret;
	*optlen = sizeof (long);

	return 0;
}

/* unix utility functions
 *
 * Find the un_data which has type `type' that is bound to the file
 * given by `index'.
 * Returns a pointer to the un_data of the socket if the search is
 * successful, else 0.
 */

static struct un_data *allundatas[UN_HASH_SIZE];

struct un_data *
un_lookup (long index, enum so_type type)
{
	struct un_data *undata;

	for (undata = allundatas[UN_HASH (index)]; undata; undata = undata->next)
	{
		if (undata->sock
			&& undata->sock->type == type
			&& UN_INDEX (undata) == index)
		{
			break;
		}
	}

	return undata;
}

void
un_put (struct un_data *undata)
{
	short idx;

	idx = UN_HASH (UN_INDEX (undata));
	undata->next = allundatas[idx];
	allundatas[idx] = undata;
}

void
un_remove (struct un_data *undata)
{
	ushort idx;
	struct un_data *prev;

	idx = UN_HASH (UN_INDEX (undata));
	prev = allundatas[idx];
	if (prev == undata)
		allundatas[idx] = undata->next;
	else
	{
		while (prev && (prev->next != undata))
			prev = prev->next;
		if (prev)
			prev->next = undata->next;
		else
			DEBUG (("unix: unix_remove: not found"));
	}
}

/* Resize `un's buffer to `newsize' bytes. Before calling this
 * you must validate `newsize' and `un'.
 */
long
un_resize (struct un_data *un, long newsize)
{
	short head = un->head, tail = un->tail;
	char *newbuf;

	newsize = (((newsize) + 1) & ~1);
	if (newsize < UN_USED (un))
		return EINVAL;

	if (newsize == un->buflen)
		return 0;

	newbuf = kmalloc (newsize);
	if (!newbuf)
	{
		DEBUG (("unix: un_resize: out of mem"));
		return ENOMEM;
	}

	if (tail >= head)
	{
		memcpy (newbuf, &un->buf[head], tail - head);
		un->head = 0;
		un->tail = tail - head;
	}
	else
	{
		short done = un->buflen - head;
		memcpy (newbuf, &un->buf[head], done);
		memcpy (newbuf + done, un->buf, tail);
		un->head = 0;
		un->tail = done + tail;
	}

	if (newsize > un->buflen)
	{
		wake (IO_Q, (long)un->sock);
		so_wakewsel (un->sock);
	}

	kfree (un->buf);
	un->buf = newbuf;
	un->buflen = newsize;

	return 0;
}

/* Convert a file name to a index. */
long
un_namei (const struct sockaddr *addr, short addrlen, long *index)
{
	struct sockaddr_un un;
	XATTR attr;
	long r;

	if (!addr)
		return EDESTADDRREQ;

	if (addrlen <= UN_PATH_OFFSET || addrlen > UN_ADDR_SIZE)
	{
		DEBUG (("unix: un_namei: invalid addrlen %d", addrlen));
		return EINVAL;
	}

	memcpy (&un, addr, addrlen);
	un.sun_path[
		MIN(sizeof(struct sockaddr_un) - 1 - UN_PATH_OFFSET, addrlen - UN_PATH_OFFSET)] = '\0';

	if (un.sun_family != AF_UNIX)
	{
		DEBUG (("unix: un_namei: domain not AF_UNIX"));
		return EAFNOSUPPORT;
	}

	r = sys_f_xattr (0, un.sun_path, &attr);
	if (r)
	{
		DEBUG (("un_namei: Fxattr(%s) -> %ld", un.sun_path, r));
		return r;
	}

	if (S_ISFIFO(attr.mode))
	{
		DEBUG (("un_namei: %s is a FIFO not a socket", un.sun_path));
		return EFTYPE;
	}

	return un_cache_lookup (un.sun_path, index, &attr);
}


/*
 *	This file implements /dev/unix. It is intended for controlling
 *	the behavior of the unix domain layer and getting information
 *	about it. netstat(8) is implemented using this device.
 *
 *	12/15/93, kay roemer.
 */

/* read() obtains this structure for every unix domain socket */
struct unix_info
{
	short		proto;	 /* protcol numer, always 0 */
	short		flags;	 /* socket flags, SO_* */
	short		type;	 /* socket type, SOCK_DGRAM or SOCK_STREAM */
	short		state;	 /* socket state, SS_* */
	short		qlen;	 /* bytes in read buffer */
	short		addrlen; /* addrlen, 0 if no address */
	struct sockaddr_un addr; /* addr, only meaningful if addrlen > 0 */
};

static long
unixdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	struct un_data *unp = 0;
	struct unix_info info, *infop = (struct unix_info *) buf;
	long space;

	for (space = nbytes; space >= sizeof (info); ++fp->pos)
	{
		int i, j;

		for (i = fp->pos, j = 0; j < UN_HASH_SIZE && i >= 0; ++j)
		{
			unp = allundatas[j];
			for (; unp && --i >= 0; unp = unp->next)
				;
		}

		if (j >= UN_HASH_SIZE)
			break;

		info.proto	= unp->proto;
		info.flags	= unp->sock->flags;
		info.type	= unp->sock->type;
		info.state	= unp->sock->state;
		info.addrlen	= unp->addrlen;
		info.addr	= unp->addr;

		if (info.type == SOCK_DGRAM)
		{
			struct dgram_hdr header = { 0, 0 };

			if (UN_USED (unp))
				un_read_header (unp, &header, 0);

			info.qlen = header.nbytes;
		}
		else
			info.qlen = UN_USED (unp);

		*infop++ = info;
		space -= sizeof (info);
	}

	return (nbytes - space);
}

DEVDRV unixdev =
{
	open:		null_open,
	write:		null_write,
	read:		unixdev_read,
	lseek:		null_lseek,
	ioctl:		zero_ioctl,
	datime:		null_datime,
	close:		null_close,
	select:		null_select,
	unselect:	null_unselect,
	writeb:		NULL,
	readb:		NULL
};
