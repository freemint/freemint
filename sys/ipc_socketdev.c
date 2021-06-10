/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1993, 1994, 1995, 1996 Kay Roemer
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-01-12
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# include "ipc_socketdev.h"

# include "libkern/libkern.h"
# include "mint/credentials.h"
# include "mint/net.h"
# include "mint/sockio.h"
# include "mint/stat.h"
# include "sockets/mintsock.h"

# include "dos.h"
# include "dosfile.h"
# include "ipc_socket.h"
# include "ipc_socketutil.h"
# include "k_fds.h"
# include "kmemory.h"
# include "proc.h"
# include "time.h"

#ifndef NO_CONST
#  ifdef __GNUC__
#    define NO_CONST(p) __extension__({ union { const void *cs; void *s; } x; x.cs = p; x.s; })
#  else
#    define NO_CONST(p) ((void *)(p))
#  endif
#endif

/*
 * debugging stuff
 */

# if 0
# ifdef DEBUG_INFO
# define SOCKETDEV_DEBUG 1
# endif
# else
# define SOCKETDEV_DEBUG 1
# endif


# ifndef SOCKETDEV_DEBUG

#  define SOCKDEV_FORCE(x)
#  define SOCKDEV_ALERT(x)	ALERT x
#  define SOCKDEV_DEBUG(x)
#  define SOCKDEV_ASSERT(x)

# else

#  define SOCKDEV_FORCE(x)	FORCE x
#  define SOCKDEV_ALERT(x)	ALERT x
#  define SOCKDEV_DEBUG(x)	DEBUG x
#  define SOCKDEV_ASSERT(x)	assert x

# endif


static long	sock_open	(FILEPTR *);
static long	sock_write	(FILEPTR *, const char *, long);
static long	sock_read	(FILEPTR *, char *, long);
static long	sock_lseek	(FILEPTR *, long, int);
static long	sock_ioctl	(FILEPTR *, int, void *);
static long	sock_datime	(FILEPTR *, ushort *, int);
static long	sock_close	(FILEPTR *, int);
static long	sock_select	(FILEPTR *, long, int);
static void	sock_unselect	(FILEPTR *, long, int);

struct devdrv sockdev =
{
	open:		sock_open,
	write:		sock_write,
	read:		sock_read,
	lseek:		sock_lseek,
	ioctl:		sock_ioctl,
	datime:		sock_datime,
	close:		sock_close,
	select:		sock_select,
	unselect:	sock_unselect,
	writeb:		NULL,
	readb:		NULL
};

static long
sock_open (FILEPTR *f)
{
	SOCKDEV_ASSERT (((struct socket *) f->devinfo));
	SOCKDEV_ALERT (("sock_open ???"));
	return 0;
}

static long
sock_write (FILEPTR *f, const char *buf, long buflen)
{
	union { const char *cc; char *c; } bufptr;
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1];

	bufptr.cc = buf;
	iov[0].iov_base = bufptr.c;
	iov[0].iov_len = buflen;

	SOCKDEV_ASSERT ((so));

	if (so->state == SS_VIRGIN)
		return EINVAL;

	return (*so->ops->send)(so, iov, 1, f->flags & O_NDELAY, 0, 0, 0);
}

static long
sock_read (FILEPTR *f, char *buf, long buflen)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};

	SOCKDEV_ASSERT ((so));

	if (so->state == SS_VIRGIN)
		return EINVAL;

	return (*so->ops->recv)(so, iov, 1, f->flags & O_NDELAY, 0, 0, 0);
}

static long
sock_lseek (FILEPTR *f, long where, int whence)
{
	return ESPIPE;
}

static long
sock_ioctl (FILEPTR *f, int cmd, void *buf)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	switch (cmd)
	{
		case SIOCSPGRP:
		{
			so->pgrp = (short) *(long *) buf;
			return 0;
		}
		case SIOCGPGRP:
		{
			*(long *) buf = (long) so->pgrp;
			return 0;
		}
		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *lk = (struct flock *) buf;

			switch (lk->l_type)
			{
				case F_UNLCK:
				{
					if (so->lockpid != sys_p_getpid ())
						return ELOCKED;

					if (f->flags & O_LOCK)
					{
						f->flags &= ~O_LOCK;
						wake (IO_Q, (long) &so->lockpid);
					}
					else
					{
						DEBUG (("sock_ioctl (F_UNLCK): not locked"));
						return ENSLOCK;
					}

					return 0;
				}
				case F_RDLCK:
				case F_WRLCK:
				{
					while (f->flags & O_LOCK)
					{
						if (so->lockpid == sys_p_getpid ())
							return 0;

						if (cmd == F_SETLK)
							return ELOCKED;

						if (sleep (IO_Q, (long) &so->lockpid))
							return EINTR;
					}

					f->flags |= O_LOCK;
					so->lockpid = sys_p_getpid ();

					return 0;
				}
			}

			return EINVAL;
		}
		case F_GETLK:
		{
			struct flock *lk = (struct flock *) buf;

			if (f->flags & O_LOCK)
			{
				lk->l_start = 0;
				lk->l_len = 0;
				lk->l_pid = so->lockpid;
				lk->l_type = F_WRLCK;
			}
			else
				lk->l_type = F_UNLCK;

			return 0;
		}
	}

	/* fall through to domain ioctl */

	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;

	return (*so->ops->ioctl)(so, cmd, buf);
}

static long
sock_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	if (rwflag)
	{
		so->time = timeptr[0];
		so->date = timeptr[1];
	}
	else
	{
		timeptr[0] = so->time;
		timeptr[1] = so->date;
	}

	return 0;
}

static long
sock_close (FILEPTR *f, int pid)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	/* Wake anyone waiting on the socket. */
	wake (IO_Q, (long) so);
	so_wakersel (so);
	so_wakewsel (so);
	so_wakexsel (so);

	if ((f->flags & O_LOCK) && ((so->lockpid == pid) || (f->links <= 0)))
	{
		f->flags &= ~O_LOCK;
		wake (IO_Q, (long) &so->lockpid);
	}

	if (f->links <= 0)
		so_free (so);

	return 0;
}

static long
sock_select (FILEPTR *f, long proc, int mode)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return 1;

	return (*so->ops->select)(so, mode, proc);
}

static void
sock_unselect (FILEPTR *f, long proc, int mode)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	switch (mode)
	{
		case O_RDONLY:
		{
			if (so->rsel == proc)
				so->rsel = 0;
			break;
		}
		case O_WRONLY:
		{
			if (so->wsel == proc)
				so->wsel = 0;
			break;
		}
		case O_RDWR:
		{
			if (so->xsel == proc)
				so->xsel = 0;
			break;
		}
	}
}


long
so_fstat_old (FILEPTR *f, XATTR *xattr)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct ucred *ucr = get_curproc()->p_cred->ucr;

	xattr->mode	= S_IFSOCK;
	xattr->index	= f->devinfo;
	xattr->dev	= 0;
	xattr->rdev	= 0;
	xattr->nlink	= 1;
	xattr->uid	= ucr->euid;
	xattr->gid	= ucr->egid;
	xattr->size	= 0;
	xattr->blksize	= 1024;
	xattr->nblocks	= 0;
	xattr->mtime	= so->time;
	xattr->mdate	= so->date;
	xattr->atime	= timestamp;
	xattr->adate	= datestamp;
	xattr->ctime	= so->time;
	xattr->cdate	= so->date;
	xattr->attr	= 0;

	xattr->reserved2 = 0;
	xattr->reserved3[0] = 0;
	xattr->reserved3[1] = 0;

	return 0;
}

long
so_fstat (FILEPTR *f, struct stat *st)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct ucred *ucr = get_curproc()->p_cred->ucr;

	st->dev		= 0;		/* inode's device */
	st->ino		= f->devinfo;	/* inode's number */
	st->mode	= S_IFSOCK;	/* inode protection mode */
	st->nlink	= 1;		/* number of hard links */
	st->uid		= ucr->euid;	/* user ID of the file's owner */
	st->gid		= ucr->egid;	/* group ID of the file's group */
	st->rdev	= 0;		/* device type */

	st->atime.high_time = 0;
	st->atime.time	= xtime.tv_sec;
	st->atime.nanoseconds = 0;

	st->mtime.high_time = 0;
	st->mtime.time	= unixtime (so->time, so->date) + timezone;;
	st->mtime.nanoseconds = 0;

	st->ctime.high_time = 0;
	st->ctime.time	= st->mtime.time;
	st->ctime.nanoseconds = 0;

	st->size	= 0;		/* file size, in bytes */
	st->blocks	= 0;		/* blocks allocated for file */
	st->blksize	= 1024;		/* optimal blocksize for I/O */
	st->flags	= 0;		/* user defined flags for file */
	st->gen		= 0;		/* file generation number */

	mint_bzero (st->res, sizeof (st->res));

	return 0;
}


# ifdef OLDSOCKDEVEMU

static long	sockemu_open	(FILEPTR *);
static long	sockemu_ioctl	(FILEPTR *, int, void *);

struct devdrv sockdevemu =
{
	open:		sockemu_open,
	write:		sock_write,
	read:		sock_read,
	lseek:		sock_lseek,
	ioctl:		sockemu_ioctl,
	datime:		sock_datime,
	close:		sock_close,
	select:		sock_select,
	unselect:	sock_unselect,
	writeb:		NULL,
	readb:		NULL
};


static long
sockemu_open (FILEPTR *f)
{
	struct socket *so;

	so = kmalloc (sizeof (*so));
	if (so)
	{
		mint_bzero (so, sizeof (*so));

		so->state = SS_VIRGIN;

		so->date = datestamp;
		so->time = timestamp;

		f->devinfo = (long) so;

		return 0;
	}

	return ENOMEM;
}

static long
sockemu_ioctl (FILEPTR *f, int cmd, void *buf)
{
	struct socket *so = (struct socket *) f->devinfo;

	SOCKDEV_ASSERT ((so));

	if (cmd != SOCKETCALL)
		/* fall through to normal ioctl */
		return sock_ioctl (f, cmd, buf);

	/* handle old sockdev commands */
	switch (((struct generic_cmd *) buf)->cmd)
	{
		/* attention, these CMDs create new sockets */

		case SOCKET_CMD:
		{
			struct socket_cmd *c = buf;
			struct socket *newso;
			long ret;

			ret = so_create (&newso, c->domain, c->type, c->protocol);
			if (ret) return ret;

			kfree (so); /* XXX copy date/time/pgrp/lockpid ??? */
			f->devinfo = (long) newso;
			return 0;
		}
		case SOCKETPAIR_CMD:
		case ACCEPT_CMD:
		{
			PROC *p = get_curproc();
			FILEPTR *newfp = NULL;
			short newfd = MIN_OPEN - 1;
			long ret;

			if (((struct generic_cmd *) buf)->cmd == ACCEPT_CMD)
			{
				if (so->state != SS_ISUNCONNECTED)
					return EINVAL;

				if (!(so->flags & SO_ACCEPTCON))
				{
					DEBUG (("sys_accept: socket not listening"));
					return EINVAL;
				}
			}

			ret = FD_ALLOC (p, &newfd, MIN_OPEN);
			if (ret) goto error;

			ret = FP_ALLOC (p, &newfp);
			if (ret) goto error;

			newfp->flags = O_RDWR;
			newfp->dev = &sockdevemu;

			if (((struct generic_cmd *) buf)->cmd == SOCKETPAIR_CMD)
			{
				struct socketpair_cmd *c = buf;
				struct socket *so1 = NULL, *so2 = NULL;

				ret = so_create (&so1, c->domain, c->type, c->protocol);
				if (ret) goto error;
				ret = so_create (&so2, c->domain, c->type, c->protocol);
				if (ret) { so_free (so1); goto error; }

				kfree (so); /* XXX copy date/time/pgrp/lockpid ??? */
				f->devinfo = (long) so1;
				newfp->devinfo = (long) so2;

				ret = (*so1->ops->socketpair)(so1, so2);
				if (ret)
				{
					so_free (so1);
					so_free (so2);
					goto error;
				}
			}
			else
			{
				struct accept_cmd *c = buf;
				struct socket *newso = NULL;

				ret = so_dup (&newso, so);
				if (ret) goto error;

				newfp->devinfo = (long) newso;

				ret = (*so->ops->accept)(so, newso, f->flags & O_NDELAY);
				if (ret < 0)
				{
					DEBUG (("sockemu_accept: cannot accept a connection"));
					so_free (newso);
					goto error1;
				}

				if (c->addr)
				{
					ret = (*newso->ops->getname)(newso, c->addr, c->addrlen, PEER_ADDR);
					if (ret < 0)
					{
						DEBUG (("sockemu_accept: getname failed"));
						*(c->addrlen) = 0;
					}
				}
			}

			FP_DONE (p, newfp, newfd, FD_CLOEXEC);
			return newfd;

		error:
			if (((struct generic_cmd *) buf)->cmd == ACCEPT_CMD) so_drop (so, f->flags & O_NDELAY);
		error1:
			if (newfp) { newfp->links--; FP_FREE (newfp); }
			if (newfd >= MIN_OPEN) FD_REMOVE (p, newfd);

			DEBUG (("%s: error %li", ((cmd == ACCEPT_CMD) ? "ACCEPT" : "SOCKETPAIR"), ret));
			return ret;
		}

		/* all other CMDs don't create new sockets */

		case BIND_CMD:
		{
			struct bind_cmd *c = buf;

			if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
				return EINVAL;

			return (*so->ops->bind)(so, c->addr, c->addrlen);
		}
		case LISTEN_CMD:
		{
			struct listen_cmd *c = buf;
			short backlog = c->backlog;
			long ret;

			if (so->state != SS_ISUNCONNECTED)
				return EINVAL;

			if (backlog < 0)
				backlog = 0;

			ret = (*so->ops->listen)(so, backlog);
			if (ret < 0)
				return ret;

			so->flags |= SO_ACCEPTCON;
			return 0;
		}
		case CONNECT_CMD:
		{
			struct connect_cmd *c = buf;

			switch (so->state)
			{
				case SS_ISUNCONNECTED:
				case SS_ISCONNECTING:
				{
					if (so->flags & SO_ACCEPTCON)
					{
						DEBUG (("sockemu_connect: attempt to connect a listening socket"));
						return EINVAL;
					}
					return (*so->ops->connect)(so, c->addr, c->addrlen, f->flags & O_NDELAY);
				}
				case SS_ISCONNECTED:
				{
					/* Connectionless sockets can be connected several
					 * times. So their state must always be
					 * SS_ISUNCONNECTED.
					 */
					DEBUG (("sockemu_connect: already connected"));
					return EISCONN;
   				}
				case SS_ISDISCONNECTING:
				case SS_ISDISCONNECTED:
				case SS_VIRGIN:
				{
					DEBUG (("sockemu_connect: socket cannot connect"));
					return EINVAL;
				}
			}

			DEBUG (("sockemu_connect: invalid socket state %d", so->state));
			return EINTERNAL;
		}
		case GETSOCKNAME_CMD:
		{
			struct getsockname_cmd *c = buf;

			if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
				return EINVAL;

			return (*so->ops->getname)(so, c->addr, c->addrlen, SOCK_ADDR);
		}
		case GETPEERNAME_CMD:
		{
			struct getpeername_cmd *c = buf;

			if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
				return EINVAL;

			return (*so->ops->getname)(so, c->addr, c->addrlen, PEER_ADDR);
		}
		case SEND_CMD:
		{
			struct send_cmd *c = buf;
			struct iovec iov[1] = {{ NO_CONST(c->buf), c->buflen }};

			if (so->state == SS_VIRGIN)
				return EINVAL;

			return (*so->ops->send)(so, iov, 1, f->flags & O_NDELAY, c->flags, NULL, 0);
		}
		case SENDTO_CMD:
		{
			struct sendto_cmd *c = buf;
			struct iovec iov[1] = {{ NO_CONST(c->buf), c->buflen }};

			if (so->state == SS_VIRGIN)
				return EINVAL;

			return (*so->ops->send)(so, iov, 1, f->flags & O_NDELAY, c->flags, c->addr, c->addrlen);
		}
		case SENDMSG_CMD:
		{
			struct sendmsg_cmd *c = buf;
			const struct msghdr *msg = c->msg;

				if (so->state == SS_VIRGIN)
			return EINVAL;

#if 0
			if (msg->msg_accrights || msg->msg_accrightslen) {
				msg->msg_accrights = NULL;
				msg->msg_accrightslen = 0;
			}
#endif

			return (*so->ops->send)(so, msg->msg_iov, msg->msg_iovlen,
						f->flags & O_NDELAY, c->flags,
						msg->msg_name, msg->msg_namelen);
		}
		case RECV_CMD:
		{
			struct recv_cmd *c = buf;
			struct iovec iov[1] = {{ c->buf, c->buflen }};

			if (so->state == SS_VIRGIN)
				return EINVAL;

			return (*so->ops->recv)(so, iov, 1, f->flags & O_NDELAY, c->flags, NULL, NULL);
		}
		case RECVFROM_CMD:
		{
			struct recvfrom_cmd *c = buf;
			struct iovec iov[1] = {{ c->buf, c->buflen }};

			if (so->state == SS_VIRGIN)
				return EINVAL;

			return (*so->ops->recv)(so, iov, 1, f->flags & O_NDELAY, c->flags, c->addr, c->addrlen);
		}
		case RECVMSG_CMD:
		{
			struct recvmsg_cmd *c = buf;
			struct msghdr *msg = c->msg;
			short namelen = msg->msg_namelen;
			long ret;

			if (so->state == SS_VIRGIN)
				return EINVAL;

			if (msg->msg_accrights && msg->msg_accrightslen)
				msg->msg_accrightslen = 0;

			ret = (*so->ops->recv)(so, msg->msg_iov, msg->msg_iovlen,
						f->flags & O_NDELAY, c->flags,
						msg->msg_name, &namelen);
			msg->msg_namelen = namelen;
			return ret;
		}
		case SETSOCKOPT_CMD:
		{
			struct setsockopt_cmd *c = buf;
			return so_setsockopt (so, c->level, c->optname, c->optval, c->optlen);
		}
		case GETSOCKOPT_CMD:
		{
			struct getsockopt_cmd *c = buf;
			return so_getsockopt (so, c->level, c->optname, c->optval, c->optlen);
		}
		case SHUTDOWN_CMD:
		{
			struct shutdown_cmd *c = buf;
			return so_shutdown (so, c->how);
		}
	}

	/* XXX - I think EINVAL is correct here */
	return ENOSYS;
}

# endif /* OLDSOCKDEVEMU */
