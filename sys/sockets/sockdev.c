/*
 *	Implementation of the socket device driver and the generic part
 *	of the socket layer. Most of the work is done in the domain
 *	specific parts of the socket layer.
 *
 *	09/30/93, kay roemer.
 */

# include "sockdev.h"

# include "mintsock.h"
# include "net.h"
# include "iov.h"
# include "sockutil.h"

# include <mint/dcntl.h>
# include <mint/file.h>


/* special (internal use only) ioctl() to get a handle's fileptr */
# define FD2FP		(('S' << 8) | 101)

static long	socketcall	(FILEPTR *, struct generic_cmd *);

static long	sock_open	(FILEPTR *);
static long	sock_write	(FILEPTR *, const char *, long);
static long	sock_read	(FILEPTR *, char *, long);
static long	sock_lseek	(FILEPTR *, long, int);
static long	sock_ioctl	(FILEPTR *, int, void *);
static long	sock_datime	(FILEPTR *, ushort *, int);
static long	sock_close	(FILEPTR *, int);
static long	sock_select	(FILEPTR *, long, int);
static void	sock_unselect	(FILEPTR *, long, int);

static long	sock_socket	(FILEPTR *, short, enum so_type, short);
static long	sock_socketpair	(FILEPTR *, short, enum so_type, short);
static long	sock_bind	(FILEPTR *, struct sockaddr *, short);
static long	sock_listen	(FILEPTR *, short);
static long	sock_accept	(FILEPTR *, struct sockaddr *, short *);
static long	sock_connect	(FILEPTR *, struct sockaddr *, short);
static long	sock_getsockname(FILEPTR *, struct sockaddr *, short *);
static long	sock_getpeername(FILEPTR *, struct sockaddr *, short *);
static long	sock_send	(FILEPTR *, char *, long, short);
static long	sock_sendto	(FILEPTR *, char *, long, short, struct sockaddr *, short);
static long	sock_recv	(FILEPTR *, char *, long, short);
static long	sock_recvfrom	(FILEPTR *, char *, long, short, struct sockaddr *, short *);
static long	sock_setsockopt	(FILEPTR *, short, short, void *, long);
static long	sock_getsockopt	(FILEPTR *, short, short, void *, long *);
static long	sock_shutdown	(FILEPTR *, short);
static long	sock_sendmsg	(FILEPTR *, struct msghdr *, short);
static long	sock_recvmsg	(FILEPTR *, struct msghdr *, short);

static void	so_drop		(FILEPTR *);
static long	so_getnewsock	(FILEPTR **);


static struct devdrv sockdev =
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

static struct dev_descr sockdev_descr =
{
	driver:		&sockdev
};

static char sockdev_name[] = "u:\\dev\\socket";

int
init_sockdev (void)
{
	long r;
	
	r = d_cntl (DEV_INSTALL, sockdev_name, (long) &sockdev_descr);
	
	return (!r || r == ENOSYS);
}


/* list of registered domains */
struct dom_ops *alldomains = NULL;

static long
sock_open (FILEPTR *f)
{
	struct socket *so;
	
	so = so_create ();
	if (!so)
	{
		DEBUG (("sockdev: sock_open: out of memory"));
		return ENOMEM;
	}
	
	f->devinfo = (long) so;		
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
	return ESPIPE;
}

/* This comes first, because of the `inline' */
INLINE long
socketcall (FILEPTR *f, struct generic_cmd *call)
{
	switch (call->cmd)
	{
		case SOCKET_CMD:
		{
			struct socket_cmd *c = (struct socket_cmd *) call;
			return sock_socket (f, c->domain, c->type, c->protocol);
		}
		case SOCKETPAIR_CMD:
		{
			struct socketpair_cmd *c = (struct socketpair_cmd *) call;
			return sock_socketpair (f, c->domain, c->type, c->protocol);
		}
		case BIND_CMD:
		{
			struct bind_cmd *c = (struct bind_cmd *) call;
			return sock_bind (f, c->addr, c->addrlen);
		}
		case LISTEN_CMD:
		{
			struct listen_cmd *c = (struct listen_cmd *) call;
			return sock_listen (f, c->backlog);
		}
		case ACCEPT_CMD:
		{
			struct accept_cmd *c = (struct accept_cmd *) call;
			return sock_accept (f, c->addr, c->addrlen);
		}
		case CONNECT_CMD:
		{
			struct connect_cmd *c = (struct connect_cmd *) call;
			return sock_connect (f, c->addr, c->addrlen);
		} 
		case GETSOCKNAME_CMD:
		{
			struct getsockname_cmd *c = (struct getsockname_cmd *) call;
			return sock_getsockname (f, c->addr, c->addrlen);
		}
		case GETPEERNAME_CMD:
		{
			struct getpeername_cmd *c = (struct getpeername_cmd *) call;
			return sock_getpeername (f, c->addr, c->addrlen);
		}
		case SEND_CMD:
		{
			struct send_cmd *c = (struct send_cmd *) call;
			return sock_send (f, c->buf, c->buflen, c->flags);
		}
		case SENDTO_CMD:
		{
			struct sendto_cmd *c = (struct sendto_cmd *) call;
			return sock_sendto (f, c->buf, c->buflen, c->flags, c->addr, c->addrlen);
		}
		case RECV_CMD:
		{
			struct recv_cmd *c = (struct recv_cmd *) call;
			return sock_recv (f, c->buf, c->buflen, c->flags);
		} 
		case RECVFROM_CMD:
		{
			struct recvfrom_cmd *c = (struct recvfrom_cmd *) call;
			return sock_recvfrom (f, c->buf, c->buflen, c->flags, c->addr, c->addrlen);
		}
		case SETSOCKOPT_CMD:
		{
			struct setsockopt_cmd *c = (struct setsockopt_cmd *) call;
			return sock_setsockopt (f, c->level, c->optname, c->optval, c->optlen);
		}
		case GETSOCKOPT_CMD:
		{
			struct getsockopt_cmd *c = (struct getsockopt_cmd *) call;
			return sock_getsockopt (f, c->level, c->optname, c->optval, c->optlen);
		}
		case SHUTDOWN_CMD:
		{
			struct shutdown_cmd *c = (struct shutdown_cmd *) call;
			return sock_shutdown (f, c->how);
		}
		case SENDMSG_CMD:
		{
			struct sendmsg_cmd *c = (struct sendmsg_cmd *) call;
			return sock_sendmsg (f, c->msg, c->flags);
		}
		case RECVMSG_CMD:
		{
			struct recvmsg_cmd *c = (struct recvmsg_cmd *) call;
			return sock_recvmsg (f, c->msg, c->flags);
		}
	}
	
	return ENOSYS;
}

static long
sock_ioctl (FILEPTR *f, int cmd, void *buf)
{
	if (cmd == SOCKETCALL)
		return socketcall (f, (struct generic_cmd *) buf);
	
	switch (cmd)
	{
		case FD2FP:
		{
			*(FILEPTR **) buf = f;
			return 0;
		}
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
						
						if (isleep (IO_Q, (long) &so->lockpid))
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
	so_wakersel (so);
	so_wakewsel (so);
	so_wakexsel (so);
	wake (IO_Q, (long)so);
	
	if (f->flags & O_LOCK && so->lockpid == pid)
	{
		f->flags &= ~O_LOCK;
		wake (IO_Q, (long) &so->lockpid);
	}
	
	if (f->links <= 0)
	{
		short oflags = so->flags;
		if (so_release (so) == 0)
		{
			if (oflags & SO_CLOSING && f->links < 0)
			{
				/*
				 * This is hackish, but keeps Mints
				 * non reentrant close() from panic.
				 */
				f->links = 0;
			}
			kfree (so);
		}
		else
			FATAL ("sock_close: close() reentrancy problem!");
	}
	
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

static long
sock_socket (FILEPTR *f, short domain, enum so_type type, short protocol)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct dom_ops *ops;
	long r;
	
	if (so->state != SS_VIRGIN)
		return EINVAL;
	
	for (ops = alldomains; ops; ops = ops->next)
		if (ops->domain == domain)
			break;
	
	if (!ops)
	{
		DEBUG (("sockdev: sock_socket: domain %d not supported", domain));
		return EAFNOSUPPORT;
	}
	
	switch (type)
	{
		case SOCK_DGRAM:
		case SOCK_STREAM:
		case SOCK_RAW:
		case SOCK_RDM:
		case SOCK_SEQPACKET:
		{
			so->ops = ops;
			so->type = type;
			
			r = (*so->ops->attach) (so, protocol);
			if (r < 0)
			{
				DEBUG (("sockdev: sock_socket: failed to attach protocol data"));
				return r;
			}
			
			so->state = SS_ISUNCONNECTED;
			return 0;
		}
	}
	
	DEBUG (("sockdev: sock_socket: no such socket type %d", type));
	return ESOCKTNOSUPPORT;
}

static long
sock_socketpair (FILEPTR *fp1, short domain, enum so_type type, short protocol)
{
	struct socket *so2, *so1 = (struct socket *) fp1->devinfo;
	FILEPTR *fp2;
	long fd2, r;
	
	r = sock_socket (fp1, domain, type, protocol);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_socketpair: cannot create 1st socket"));
		return r;
	}
	
	fd2 = so_getnewsock (&fp2);
	if (fd2 < 0)
	{
		DEBUG (("sockdev: sock_socketpair: cannot alloc 2nd socket"));
		so_release (so1);
		return fd2;
	}
	
	r = sock_socket (fp2, domain, type, protocol);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_socketpair: cannot create 2nd socket"));
		so_release (so1);
		f_close (fd2);
		return r;
	}	
	so2 = (struct socket *)fp2->devinfo;
	
	r = (*so1->ops->socketpair) (so1, so2);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_socketpair: cannot connect the sockets"));
		so_release (so1);
		f_close (fd2);
		return r;
	}
	
	return fd2;	
}

static long
sock_bind (FILEPTR *f, struct sockaddr *addr, short addrlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->bind) (so, addr, addrlen);
}

static long
sock_listen (FILEPTR *f, short backlog)
{
	struct socket *so = (struct socket *) f->devinfo;
	long r;
	
	if (so->state != SS_ISUNCONNECTED)
		return EINVAL;
	
	if (backlog < 0)
		backlog = 0;
	
	r = (*so->ops->listen) (so, backlog);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_listen: failing ..."));
		return r;
	}
	
	so->flags |= SO_ACCEPTCON;
	return 0;
}

static void
so_drop (FILEPTR *f)
{
	struct socket *newso, *so = (struct socket *) f->devinfo;
	
	if (!(so->flags & SO_DROP))
		return;
	
	DEBUG (("so_drop: dropping incoming connection"));
	if (!(newso = so_create ()))
		return;
	
	newso->type = so->type;
	newso->ops = so->ops;
	if ((*so->ops->dup) (newso, so) < 0)
	{
		kfree (newso);
		return;
	}
	
	newso->state = SS_ISUNCONNECTED;
	(*so->ops->accept) (so, newso, f->flags & O_NDELAY);
	so_release (newso);
	kfree (newso);
}

static long
sock_accept (FILEPTR *fp, struct sockaddr *addr, short *addrlen)
{
	struct socket *newso, *so = (struct socket *) fp->devinfo;
	FILEPTR *newfp;
	long newfd, r;
	
	if (so->state != SS_ISUNCONNECTED)
		return EINVAL;
	
	if (!(so->flags & SO_ACCEPTCON))
	{
		DEBUG (("sockdev: sock_accept: socket not listening"));
		return EINVAL;
	}
		
	newfd = so_getnewsock (&newfp);
	if (newfd < 0)
	{
		DEBUG (("sockdev: sock_accept: cannot alloc fresh socket"));
		so_drop (fp);
		return newfd;
	}
	newso = (struct socket *) newfp->devinfo;
	newso->type = so->type;
	newso->ops = so->ops;
	
	r = (*so->ops->dup) (newso, so);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_accept: failed to dup protocol data"));
		f_close (newfd);
		so_drop (fp);
		return r;
	}
	newso->state = SS_ISUNCONNECTED;
	
	r = (*so->ops->accept) (so, newso, fp->flags & O_NDELAY);
	if (r < 0)
	{
		DEBUG (("sockdev: sock_accept: cannot accept a connection"));
		f_close (newfd);
		return r;
	}
	
	if (addr)
	{
		r = (*newso->ops->getname) (newso, addr, addrlen, PEER_ADDR);
		if (r < 0)
		{
			DEBUG (("sockdev: sock_accept: getname failed"));
			*addrlen = 0;
		}
	}
	
	return newfd;
}

static long
sock_connect (FILEPTR *f, struct sockaddr *addr, short addrlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	switch (so->state)
	{
		case SS_ISUNCONNECTED:
		case SS_ISCONNECTING:
		{
			if (so->flags & SO_ACCEPTCON)
			{
				DEBUG (("sockdev: sock_connect: attempt to connect a listening socket"));
				return EINVAL;
			}
			return (*so->ops->connect) (so, addr, addrlen, f->flags & O_NDELAY);
		}
		case SS_ISCONNECTED:
		{
			/* Connectionless sockets can be connected serveral
			 * times. So their state must always be
			 * SS_ISUNCONNECTED.
			 */
			DEBUG (("sockdev: sock_connect: already connected"));
			return EISCONN;
   		}
		case SS_ISDISCONNECTING:
		case SS_ISDISCONNECTED:
		case SS_VIRGIN:
		{
			DEBUG (("sockdev: sock_connect: socket cannot connect"));
			return EINVAL;
		}
	}
	
	DEBUG (("socketdev: so_connect: invalid socket state %d", so->state));
	return EINTERNAL;
}

static long
sock_getsockname (FILEPTR *f, struct sockaddr *addr, short *addrlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->getname) (so, addr, addrlen, SOCK_ADDR);
}

static long
sock_getpeername (FILEPTR *f, struct sockaddr *addr, short *addrlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->getname) (so, addr, addrlen, PEER_ADDR);
}

static long
sock_send (FILEPTR *f, char *buf, long buflen, short flags)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->send) (so, iov, 1, f->flags & O_NDELAY, flags, 0, 0);
}

static long
sock_sendto (FILEPTR *f, char *buf, long buflen, short flags, struct sockaddr *addr, short addrlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->send) (so, iov, 1, f->flags & O_NDELAY, flags, addr, addrlen);
}

static long
sock_recv (FILEPTR *f, char *buf, long buflen, short flags)
{
	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->recv) (so, iov, 1, f->flags & O_NDELAY, flags, 0, 0);
}

static long
sock_recvfrom (FILEPTR *f, char *buf, long buflen, short flags, struct sockaddr *addr, short *addrlen)
{
 	struct socket *so = (struct socket *) f->devinfo;
	struct iovec iov[1] = {{ buf, buflen }};
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->recv) (so, iov, 1, f->flags & O_NDELAY, flags, addr, addrlen);
}

static long
sock_setsockopt (FILEPTR *f, short level, short optname, void *optval, long optlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	if (level == SOL_SOCKET) switch (optname)
	{
		case SO_DROPCONN:
		{
			if (!optval || optlen < sizeof (long))
				return EINVAL;
			
			if (*(long *) optval)
				so->flags |= SO_DROP;
			else
				so->flags &= ~SO_DROP;
			
			return 0;
		}
		default:
			break;
	}
	
	return (*so->ops->setsockopt) (so, level, optname, optval, optlen);
}

static long
sock_getsockopt (FILEPTR *f, short level, short optname, void *optval, long *optlen)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
	{
		DEBUG (("sockdev: sock_getsockopt: virgin state"));
		return EINVAL;
	}
	
	if (level == SOL_SOCKET)
	{
		switch (optname)
		{
			case SO_DROPCONN:
			{
				if (!optval || !optlen || *optlen < sizeof (long))
					return EINVAL;
				
				*(long *) optval = !!(so->flags & SO_DROP);
				*optlen = sizeof (long);
				
				return 0;
			}
			default:
				break;
		}
	}
	
	return (*so->ops->getsockopt) (so, level, optname, optval, optlen);
}

static long
sock_shutdown (FILEPTR *f, short how)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	switch (how)
	{
		case 0:
			so->flags |= SO_CANTRCVMORE;
			break;
		case 1:
			so->flags |= SO_CANTSNDMORE;
			break;
		case 2:
			so->flags |= (SO_CANTRCVMORE|SO_CANTSNDMORE);
			break;
	}
	
	return (*so->ops->shutdown) (so, how);
}

static long
sock_sendmsg (FILEPTR *f, struct msghdr *msg, short flags)
{
	struct socket *so = (struct socket *) f->devinfo;
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	if (msg->msg_accrights && msg->msg_accrightslen)
		return EINVAL;
	
	return (*so->ops->send) (so, msg->msg_iov, msg->msg_iovlen,
				f->flags & O_NDELAY, flags,
				msg->msg_name, msg->msg_namelen);
}

static long
sock_recvmsg (FILEPTR *f, struct msghdr *msg, short flags)
{
	struct socket *so = (struct socket *) f->devinfo;
	short namelen = msg->msg_namelen;
	long r;
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	if (msg->msg_accrights && msg->msg_accrightslen)
		msg->msg_accrightslen = 0;
	
	r = (*so->ops->recv) (so, msg->msg_iov, msg->msg_iovlen,
				f->flags & O_NDELAY, flags,
				msg->msg_name, &namelen);
	msg->msg_namelen = namelen;
	return r;
}

/* socket utility functions */

/* Get a new filehandle with an associated fileptr and virgin socket.
 * Return the filehandle or an negative error code.
 * Return the fileptr in *fp.
 */
static long
so_getnewsock (FILEPTR **fp)
{
	long fd, r;
	
	fd = f_open (sockdev_name, O_RDWR);
	if (fd < 0)
	{
		DEBUG (("sockdev: so_getnewsock: cannot open %s", sockdev_name));
		return fd;
	}
	
	r = f_cntl (fd, (long) fp, FD2FP);
	if (r < 0)
	{
		DEBUG (("sockdev: so_getnewsock: f_cntl failed"));
		f_close (fd);
		return r;
	}
	
	return fd;
}
