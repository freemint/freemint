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

# include "ipc_socket.h"

# include "mint/file.h"
# include "mint/proc.h"

# include "ipc_socketdev.h"
# include "ipc_socketutil.h"
# include "k_fds.h"


/* socket system calls */

long
sys_socket (short domain, enum so_type type, short protocol)
{
	PROC *p = curproc;
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	struct socket *so = NULL;
	long ret;
	
	DEBUG (("sys_socket(%i, %i, %i)", domain, type, protocol));
	
	ret = so_create (&so, domain, type, protocol);
	if (ret) goto error;
	
	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;
	
	fp->flags = O_RDWR;
	fp->devinfo = (long) so;
	fp->dev = &sockdev;
	
	fp_done (p, fp, fd, FD_CLOEXEC);
	
	DEBUG (("sys_socket: fd %i", fd));
	return fd;
	
error:
	if (so) so_free (so);
	if (fp) { fp->links--; fp_free (fp); }
	if (fd >= MIN_OPEN) fd_remove (p, fd);
	
	DEBUG (("sys_socket: failure %li", ret));
	return ret;
}

long
sys_socketpair (short domain, enum so_type type, short protocol, short fds[2])
{
	PROC *p = curproc;
	FILEPTR *fp1 = NULL, *fp2 = NULL;
	short fd1 = MIN_OPEN - 1, fd2 = MIN_OPEN - 1;
	struct socket *so1 = NULL, *so2 = NULL;
	long ret;
	
	DEBUG (("sys_socketpair(%i, %i, %i, %lx)", domain, type, protocol, fds));
	
	ret = so_create (&so1, domain, type, protocol);
	if (ret) goto error;
	ret = so_create (&so2, domain, type, protocol);
	if (ret) goto error;
	
	ret = FD_ALLOC (p, &fd1, MIN_OPEN);
	if (ret) goto error;
	ret = FD_ALLOC (p, &fd2, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &fp1);
	if (ret) goto error;
	ret = FP_ALLOC (p, &fp2);
	if (ret) goto error;
	
	fp1->flags = O_RDWR;
	fp1->devinfo = (long) so1;
	fp1->dev = &sockdev;
	
	fp2->flags = O_RDWR;
	fp2->devinfo = (long) so2;
	fp2->dev = &sockdev;
	
	ret = (*so1->ops->socketpair)(so1, so2);
	if (!ret)
	{
		fp_done (p, fp1, fd1, FD_CLOEXEC);
		fp_done (p, fp1, fd1, FD_CLOEXEC);
		
		fds[0] = fd1;
		fds[1] = fd2;
		
		DEBUG (("sys_socketpair: fds[0] = %i, fds[1] = %i", fds[0], fds[1]));
		return 0;
	}
	
	DEBUG (("sys_socketpair: cannot connect the sockets"));
	
error:
	if (so1) so_free (so1);
	if (so2) so_free (so2);
	if (fp1) { fp1->links--; fp_free (fp1); }
	if (fp2) { fp2->links--; fp_free (fp2); }
	if (fd1 >= MIN_OPEN) fd_remove (p, fd1);
	if (fd2 >= MIN_OPEN) fd_remove (p, fd2);
	
	DEBUG (("sys_socketpair: failure %li", ret));
	return ret;
}

long
sys_bind (short fd, struct sockaddr *addr, short addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_bind(%i, %lx, %i)", fd, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->bind)(so, addr, addrlen);
}

long
sys_listen (short fd, short backlog)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_listen(%i, %i)", fd, backlog));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state != SS_ISUNCONNECTED)
		return EINVAL;
	
	if (backlog < 0)
		backlog = 0;
	
	r = (*so->ops->listen)(so, backlog);
	if (r < 0)
	{
		DEBUG (("sys_listen: failing ..."));
		return r;
	}
	
	so->flags |= SO_ACCEPTCON;
	return 0;
}

long
sys_accept (short fd, struct sockaddr *addr, short *addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp, *newfp = NULL;
	short newfd = MIN_OPEN - 1;
	struct socket *so, *newso = NULL;
	long ret;
	
	DEBUG (("sys_accept(%i, %lx, %lx)", fd, addr, addrlen));
	
	ret = FP_GET1 (p, fd, &fp);
	if (ret) return ret;
	
	so = (struct socket *) fp->devinfo;
	if (so->state != SS_ISUNCONNECTED)
		return EINVAL;
	
	if (!(so->flags & SO_ACCEPTCON))
	{
		DEBUG (("sys_accept: socket not listening"));
		return EINVAL;
	}
	
	ret = FD_ALLOC (p, &newfd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &newfp);
	if (ret) goto error;
	
	ret = so_dup (&newso, so);
	if (ret) goto error;
	
	newfp->flags = O_RDWR;
	newfp->devinfo = (long) newso;
	newfp->dev = &sockdev;
	
	ret = (*so->ops->accept)(so, newso, fp->flags & O_NDELAY);
	if (ret < 0)
	{
		DEBUG (("sys_accept: cannot accept a connection"));
		goto error1;
	}
	
	if (addr)
	{
		ret = (*newso->ops->getname)(newso, addr, addrlen, PEER_ADDR);
		if (ret < 0)
		{
			DEBUG (("sys_accept: getname failed"));
			*addrlen = 0;
		}
	}
	
	fp_done (p, newfp, newfd, FD_CLOEXEC);
	
	DEBUG (("sys_accept: newfd %i", newfd));
	return newfd;
	
error:
	so_drop (so, fp->flags & O_NDELAY);
error1:
	if (newso) so_free (newso);
	if (newfp) fp_free (newfp);
	if (newfd >= MIN_OPEN) fd_remove (p, newfd);
	
	return ret;
}

long
sys_connect (short fd, struct sockaddr *addr, short addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_connect(%i, %lx, %i)", fd, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	switch (so->state)
	{
		case SS_ISUNCONNECTED:
		case SS_ISCONNECTING:
		{
			if (so->flags & SO_ACCEPTCON)
			{
				DEBUG (("sys_connect: attempt to connect a listening socket"));
				return EINVAL;
			}
			return (*so->ops->connect)(so, addr, addrlen, fp->flags & O_NDELAY);
		}
		case SS_ISCONNECTED:
		{
			/* Connectionless sockets can be connected serveral
			 * times. So their state must always be
			 * SS_ISUNCONNECTED.
			 */
			DEBUG (("sys_connect: already connected"));
			return EISCONN;
   		}
		case SS_ISDISCONNECTING:
		case SS_ISDISCONNECTED:
		case SS_VIRGIN:
		{
			DEBUG (("sys_connect: socket cannot connect"));
			return EINVAL;
		}
	}
	
	DEBUG (("sys_connect: invalid socket state %d", so->state));
	return EINTERNAL;
}

long
sys_getsockname (short fd, struct sockaddr *addr, short *addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_getsockname(%i, %lx, %lx)", fd, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->getname)(so, addr, addrlen, SOCK_ADDR);
}

long
sys_getpeername (short fd, struct sockaddr *addr, short *addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_getpeername(%i, %lx, %lx)", fd, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	return (*so->ops->getname)(so, addr, addrlen, PEER_ADDR);
}

long
sys_sendto (short fd, char *buf, long buflen, short flags, struct sockaddr *addr, short addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	struct iovec iov[1] = {{ buf, buflen }};
	
	DEBUG (("sys_sendto(%i, %lx, %li, %i, %lx, %i)", fd, buf, buflen, flags, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->send)(so, iov, 1, fp->flags & O_NDELAY, flags, addr, addrlen);
}

long
sys_sendmsg (short fd, struct msghdr *msg, short flags)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_sendmsg(%i, %lx, %i)", fd, msg, flags));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	if (msg->msg_accrights && msg->msg_accrightslen)
		return EINVAL;
	
	return (*so->ops->send)(so, msg->msg_iov, msg->msg_iovlen,
				fp->flags & O_NDELAY, flags,
				msg->msg_name, msg->msg_namelen);
}

long
sys_recvfrom (short fd, char *buf, long buflen, short flags, struct sockaddr *addr, short *addrlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	struct iovec iov[1] = {{ buf, buflen }};
	
	DEBUG (("sys_recvfrom(%i, %lx, %li, %i, %lx, %lx)", fd, buf, buflen, flags, addr, addrlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->recv)(so, iov, 1, fp->flags & O_NDELAY, flags, addr, addrlen);
}

long
sys_recvmsg (short fd, struct msghdr *msg, short flags)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	short namelen = msg->msg_namelen;
	
	DEBUG (("sys_recvmsg(%i, %lx, %i)", fd, msg, flags));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	if (msg->msg_accrights && msg->msg_accrightslen)
		msg->msg_accrightslen = 0;
	
	r = (*so->ops->recv)(so, msg->msg_iov, msg->msg_iovlen,
				fp->flags & O_NDELAY, flags,
				msg->msg_name, &namelen);
	msg->msg_namelen = namelen;
	return r;
}

long
sys_setsockopt (short fd, short level, short optname, void *optval, long optlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_setsockopt(%i, %i, %i, %lx, %li)", fd, level, optname, optval, optlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
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
	
	return (*so->ops->setsockopt)(so, level, optname, optval, optlen);
}

long
sys_getsockopt (short fd, short level, short optname, void *optval, long *optlen)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_getsockopt(%i, %i, %i, %lx, %lx)", fd, level, optname, optval, optlen));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
	{
		DEBUG (("sys_getsockopt: virgin state -> EINVAL"));
		return EINVAL;
	}
	
	if (level == SOL_SOCKET) switch (optname)
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
	
	return (*so->ops->getsockopt)(so, level, optname, optval, optlen);
}

long
sys_shutdown (short fd, short how)
{
	PROC *p = curproc;
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_shutdown(%i, %i)", fd, how));
	
	r = FP_GET1 (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
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
	
	return (*so->ops->shutdown)(so, how);
}
