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
 * begin:	2001-01-12
 * last change:	2001-01-12
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "ipc_socketdev.h"

# include "mint/net.h"
# include "mint/proc.h"
# include "sockets/mintsock.h"

# include "dos.h"
# include "dosfile.h"
# include "ipc_socket.h"
# include "ipc_socketutil.h"
# include "k_fds.h"
# include "kmemory.h"
# include "proc.h"


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
	ALERT ("sock_open not supported, internal failure!");
	return 0;
}

static long
sock_write (FILEPTR *f, const char *buf, long buflen)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ (char *) buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->send) (so, iov, 1, f->flags & O_NDELAY, 0, 0, 0);
}

static long
sock_read (FILEPTR *f, char *buf, long buflen)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->recv) (so, iov, 1, f->flags & O_NDELAY, 0, 0, 0);
}

static long
sock_lseek (FILEPTR *f, long where, int whence)
{
	return EACCES;
}

static long
sock_ioctl (FILEPTR *f, int cmd, void *buf)
{
	switch (cmd)
	{
		case SIOCSPGRP:
		{
			struct socket *so = (struct socket *) f->devinfo;
			
			so->pgrp = (short) *(long *) buf;
			return 0;
		}
		case SIOCGPGRP:
		{
			struct socket *so = (struct socket *) f->devinfo;
			
			*(long *) buf = (long) so->pgrp;
			return 0;
		}
		case F_SETLK:
		case F_SETLKW:
		{
			struct socket *so = (struct socket *) f->devinfo;
			struct flock *lk = (struct flock *) buf;
			
			switch (lk->l_type)
			{
				case F_UNLCK:
				{
					if (so->lockpid != p_getpid ())
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
						if (so->lockpid == p_getpid ())
							return 0;
						
						if (cmd == F_SETLK)
							return ELOCKED;
						
						if (sleep (IO_Q, (long) &so->lockpid))
							return EINTR;
					}
					
					f->flags |= O_LOCK;
					so->lockpid = p_getpid ();
					
					return 0;
				}
			}
			
			return EINVAL;
		}
		case F_GETLK:
		{
			struct socket *so = (struct socket *) f->devinfo;
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
	
	/* fall through to socket ioctl */
	{
		struct socket *so = (struct socket *) f->devinfo;
		
		if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
			return EINVAL;
		
		return (*so->ops->ioctl) (so, cmd, buf);
	}
}

static long
sock_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	struct socket *so = (struct socket *) f->devinfo;
	
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
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return 1;
	
	return (*so->ops->select) (so, mode, proc);
}

static void
sock_unselect (FILEPTR *f, long proc, int mode)
{
	struct socket *so = (struct socket *) f->devinfo;
	
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


# ifdef OLDSOCKDEVEMU

static long	sockemu_open	(FILEPTR *);
static long	sockemu_write	(FILEPTR *, const char *, long);
static long	sockemu_read	(FILEPTR *, char *, long);
static long	sockemu_lseek	(FILEPTR *, long, int);
static long	sockemu_ioctl	(FILEPTR *, int, void *);
static long	sockemu_datime	(FILEPTR *, ushort *, int);
static long	sockemu_close	(FILEPTR *, int);
static long	sockemu_select	(FILEPTR *, long, int);
static void	sockemu_unselect(FILEPTR *, long, int);

struct devdrv sockdevemu =
{
	open:		sockemu_open,
	write:		sockemu_write,
	read:		sockemu_read,
	lseek:		sockemu_lseek,
	ioctl:		sockemu_ioctl,
	datime:		sockemu_datime,
	close:		sockemu_close,
	select:		sockemu_select,
	unselect:	sockemu_unselect,
	writeb:		NULL,
	readb:		NULL
};

static long
sockemu_open (FILEPTR *f)
{
	f->devinfo = -1;		
	return 0;	 
}

static long
sockemu_write (FILEPTR *f, const char *buf, long buflen)
{
	short fd = f->devinfo;
	long ret;
	
	if (fd < 0)
		return EINVAL;
	
	ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
	if (ret)
		return ret;
	
	return f->dev->write (f, buf, buflen);
}

static long
sockemu_read (FILEPTR *f, char *buf, long buflen)
{
	short fd = f->devinfo;
	long ret;
	
	if (fd < 0)
		return EINVAL;
	
	ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
	if (ret)
		return ret;
	
	return f->dev->read (f, buf, buflen);
}

static long
sockemu_lseek (FILEPTR *f, long where, int whence)
{
	return EACCES;
}

static long
sockemu_ioctl (FILEPTR *f, int cmd, void *buf)
{
	if (cmd == SOCKETCALL)
	{
		struct generic_cmd *gc = buf;
		short fd = f->devinfo;
		
		if (gc->cmd == SOCKET_CMD || gc->cmd == SOCKETPAIR_CMD)
		{
			if (fd >= 0)
				return EINVAL;
		}
		else
		{
			if (fd < 0)
				return EINVAL;
		}
		
		switch (gc->cmd)
		{
/* special (internal use only) ioctl() to get a handle's fileptr */
# define FD2FP (('S' << 8) | 101)
			case FD2FP:
			{
				*(FILEPTR **) buf = f;
				return 0;
			}
			case SOCKET_CMD:
			{
				struct socket_cmd *c = buf;
				
				fd = sys_socket (c->domain, c->type, c->protocol);
				if (fd >= 0)
					f->devinfo = fd;
				
				return fd;
			}
			case SOCKETPAIR_CMD:
			{
				struct socketpair_cmd *c = buf;
				short newfd;
				
				newfd = f_open ("u:\\dev\\socket", O_RDWR);
				if (newfd >= 0)
				{
					FILEPTR *newf;
					short fds[2];
					long ret;
					
					ret = f_cntl (newfd, (long) &newf, FD2FP);
					if (ret)
					{
						f_close (newfd);
						return ret;
					}
					
					ret = sys_socketpair (c->domain, c->type, c->protocol, fds);
					if (ret)
					{
						f_close (newfd);
						return ret;
					}
					
					f->devinfo = fds[0];
					newf->devinfo = fds[1];
				}
				
				return newfd;
			}
			case BIND_CMD:
			{
				struct bind_cmd *c = buf;
				return sys_bind (fd, c->addr, c->addrlen);
			}
			case LISTEN_CMD:
			{
				struct listen_cmd *c = buf;
				return sys_listen (fd, c->backlog);
			}
			case ACCEPT_CMD:
			{
				struct accept_cmd *c = buf;
				return sys_accept (fd, c->addr, c->addrlen);
			}
			case CONNECT_CMD:
			{
				struct connect_cmd *c = buf;
				return sys_connect (fd, c->addr, c->addrlen);
			} 
			case GETSOCKNAME_CMD:
			{
				struct getsockname_cmd *c = buf;
				return sys_getsockname (fd, c->addr, c->addrlen);
			}
			case GETPEERNAME_CMD:
			{
				struct getpeername_cmd *c = buf;
				return sys_getpeername (fd, c->addr, c->addrlen);
			}
			case SEND_CMD:
			{
				struct send_cmd *c = buf;
				return sys_sendto (fd, c->buf, c->buflen, c->flags, NULL, 0);
			}
			case SENDTO_CMD:
			{
				struct sendto_cmd *c = buf;
				return sys_sendto (fd, c->buf, c->buflen, c->flags, c->addr, c->addrlen);
			}
			case SENDMSG_CMD:
			{
				struct sendmsg_cmd *c = buf;
				return sys_sendmsg (fd, c->msg, c->flags);
			}
			case RECV_CMD:
			{
				struct recv_cmd *c = buf;
				return sys_recvfrom (fd, c->buf, c->buflen, c->flags, NULL, NULL);
			} 
			case RECVFROM_CMD:
			{
				struct recvfrom_cmd *c = buf;
				return sys_recvfrom (fd, c->buf, c->buflen, c->flags, c->addr, c->addrlen);
			}
			case RECVMSG_CMD:
			{
				struct recvmsg_cmd *c = buf;
				return sys_recvmsg (fd, c->msg, c->flags);
			}
			case SETSOCKOPT_CMD:
			{
				struct setsockopt_cmd *c = buf;
				return sys_setsockopt (fd, c->level, c->optname, c->optval, c->optlen);
			}
			case GETSOCKOPT_CMD:
			{
				struct getsockopt_cmd *c = buf;
				return sys_getsockopt (fd, c->level, c->optname, c->optval, c->optlen);
			}
			case SHUTDOWN_CMD:
			{
				struct shutdown_cmd *c = buf;
				return sys_shutdown (fd, c->how);
			}
		}
		
		/* XXX */
		return ENOSYS;
	}
	
	/* else fallthrough */
	{
		short fd = f->devinfo;
		long ret;
		
		if (fd < 0)
			return EINVAL;
		
		ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
		if (ret)
			return ret;
		
		return f->dev->ioctl (f, cmd, buf);
	}
}

static long
sockemu_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	short fd = f->devinfo;
	long ret;
	
	if (fd < 0)
		return EINVAL;
	
	ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
	if (ret)
		return ret;
	
	return f->dev->datime (f, timeptr, rwflag);
}

static long
sockemu_close (FILEPTR *f, int pid)
{
	short fd = f->devinfo;
	long ret = 0;
	
	if (fd < 0)
		return EINVAL;
	
	if (f->links <= 0)
	{
		ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
		if (ret)
			return ret;
		
		/* remove fd */
		fd_remove (curproc, fd);
		
		/* XXX attention, reentrancy */
		ret = do_close (curproc, f);
	}
	
	return ret;
}

static long
sockemu_select (FILEPTR *f, long proc, int mode)
{
	short fd = f->devinfo;
	long ret;
	
	if (fd < 0)
		return EINVAL;
	
	ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
	if (ret)
		return ret;
	
	return f->dev->select (f, proc, mode);
}

static void
sockemu_unselect (FILEPTR *f, long proc, int mode)
{
	short fd = f->devinfo;
	long ret;
	
	if (fd < 0)
		return;
	
	ret = fp_get1 (curproc, fd, &f, __FUNCTION__);
	if (ret)
		return;
	
	f->dev->unselect (f, proc, mode);
}

# endif /* OLDSOCKDEVEMU */
