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

# include "ipc_socket.h"

# include "mint/file.h"
# include "mint/proc.h"

# include "ipc_socketdev.h"
# include "ipc_socketutil.h"
# include "k_fds.h"


/* new style pipe */

# if 0
/* old function */

long _cdecl
f_pipe (short *usrh)
{
	PROC *p = get_curproc();
	FILEPTR *in, *out;
	short infd, outfd;
	long r;
	
	/* MAGIC: 32 >= strlen "u:\pipe\sys$pipe.000\0" */
	char pipename[32];
	
	TRACE (("Fpipe(%lx)", usrh));
	
	r = FP_ALLOC (p, &out);
	if (r) return r;
	
	/* BUG: more than 999 open pipes hangs the system
	 */
	do {
		static int pipeno = 0;
		
		ksprintf (pipename, sizeof (pipename), "u:\\pipe\\sys$pipe.%03d", pipeno);
		
		pipeno++;
		if (pipeno > 999)
			pipeno = 0;
		
		/* read-only attribute means unidirectional fifo
		 * hidden attribute means check for broken pipes
		 * changed attribute means act like Unix fifos
		 */
		r = do_open (&out, pipename, O_WRONLY|O_CREAT|O_EXCL, FA_RDONLY|FA_HIDDEN|FA_CHANGED, NULL);
	}
	while (r != 0 && r == EACCES);
	
	if (r)
	{
		out->links--;
		FP_FREE (out);
		
		DEBUG (("Fpipe: error %ld", r));
		return r;
	}
	
	r = FP_ALLOC (p, &in);
	if (r)
	{
		do_close (p, out);
		return r;
	}
	
	r = do_open (&in, pipename, O_RDONLY, 0, NULL);
	if (r)
	{
		do_close (p, out);
		in->links--;
		FP_FREE (in);
		
		DEBUG (("Fpipe: in side of pipe not opened (error %ld)", r));
		return r;
	}
	
	r = FD_ALLOC (p, &infd, MIN_OPEN);
	if (r)
	{
		do_close (p, in);
		do_close (p, out);
		
		return r;
	}
	
	r = FD_ALLOC (p, &outfd, infd+1);
	if (r)
	{
		FD_REMOVE (p, infd);
		
		do_close (p, in);
		do_close (p, out);
		
		return r;
	}
	
	/* activate the fps; default is to leave pipes open across Pexec */
	FP_DONE (p, in, infd, 0);
	FP_DONE (p, out, outfd, 0);
	
	usrh[0] = infd;
	usrh[1] = outfd;
	
	TRACE (("Fpipe: returning E_OK: infd %i outfd %i", infd, outfd));
	return E_OK;
}
# endif

/*
 * sys_pipe(short fds[2]): opens a pipe. if successful, returns 0, and
 * sets fds[0] to a file descriptor for the read end of the pipe
 * and fds[1] to one for the write end.
 */
long _cdecl
sys_pipe (short fds[2])
{
	struct proc *p = get_curproc();
	struct socket *rso = NULL, *wso = NULL;
	FILEPTR *rf = NULL, *wf = NULL;
	short rfd = MIN_OPEN - 1, wfd = MIN_OPEN - 1;
	long ret;
	
	
	ret = so_create (&rso, AF_UNIX, SOCK_STREAM, 0);
	if (ret) goto error;
	
	ret = so_create (&wso, AF_UNIX, SOCK_STREAM, 0);
	if (ret) goto error;
	
	
	ret = FP_ALLOC (p, &rf);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &wf);
	if (ret) goto error;
	
	
	ret = FD_ALLOC (p, &rfd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FD_ALLOC (p, &wfd, rfd+1);
	if (ret) goto error;
	
	
	rf->flags = O_RDONLY;
	rf->devinfo = (long) rso;
	rf->dev = &sockdev;
	
	wf->flags = O_WRONLY;
	wf->devinfo = (long) wso;
	wf->dev = &sockdev;
	
	
	ret = (*rso->ops->socketpair)(rso, wso);
	if (!ret)
	{
		/* activate the fps
		 * default is to leave pipes open across exec()
		 */
		FP_DONE (p, rf, rfd, 0);
		FP_DONE (p, wf, wfd, 0);
		
		fds[0] = rfd; /* infd */
		fds[1] = wfd; /* outfd */
		
		DEBUG (("sys_pipe: fds[0] = %i, fds[1] = %i", fds[0], fds[1]));
		return 0;
	}
	
	DEBUG (("sys_pipe: cannot connect the sockets!"));
	
error:
	if (rso) so_free (rso);
	if (wso) so_free (wso);
	
	if (rf) { rf->links--; FP_FREE (rf); }
	if (wf) { wf->links--; FP_FREE (wf); }
	
	if (rfd >= MIN_OPEN) FD_REMOVE (p, rfd);
	if (wfd >= MIN_OPEN) FD_REMOVE (p, wfd);
	
	DEBUG (("sys_pipe: failure %li", ret));
	return ret;
}


/* help function */

static long
getsock (struct proc *p, short fd, struct file **fp)
{
	long r;
	
	r = FP_GET1 (p, fd, fp);
	if (r) return r;
	
# ifdef OLDSOCKDEVEMU
	if ((*fp)->dev != &sockdev && (*fp)->dev != &sockdevemu)
# else
	if ((*fp)->dev != &sockdev)
# endif
		return ENOTSOCK;
	
	
	return 0;
}


/* socket system calls */

long _cdecl
sys_socket (long domain, long type, long protocol)
{
	PROC *p = get_curproc();
	FILEPTR *fp = NULL;
	short fd = MIN_OPEN - 1;
	struct socket *so = NULL;
	long ret;
	
	DEBUG (("sys_socket(%li, %li, %li)", domain, type, protocol));
	
	ret = so_create (&so, domain, type, protocol);
	if (ret) goto error;
	
	ret = FD_ALLOC (p, &fd, MIN_OPEN);
	if (ret) goto error;
	
	ret = FP_ALLOC (p, &fp);
	if (ret) goto error;
	
	fp->flags = O_RDWR;
	fp->devinfo = (long) so;
	fp->dev = &sockdev;
	
	FP_DONE (p, fp, fd, FD_CLOEXEC);
	
	DEBUG (("sys_socket: fd %i", fd));
	return fd;
	
error:
	if (so) so_free (so);
	if (fp) { fp->links--; FP_FREE (fp); }
	if (fd >= MIN_OPEN) FD_REMOVE (p, fd);
	
	DEBUG (("sys_socket: failure %li", ret));
	return ret;
}

long _cdecl
sys_socketpair (long domain, long type, long protocol, short fds[2])
{
	PROC *p = get_curproc();
	FILEPTR *fp1 = NULL, *fp2 = NULL;
	short fd1 = MIN_OPEN - 1, fd2 = MIN_OPEN - 1;
	struct socket *so1 = NULL, *so2 = NULL;
	long ret;
	
	DEBUG (("sys_socketpair(%li, %li, %li, %p)", domain, type, protocol, fds));
	
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
		FP_DONE (p, fp1, fd1, FD_CLOEXEC);
		FP_DONE (p, fp2, fd2, FD_CLOEXEC);
		
		fds[0] = fd1;
		fds[1] = fd2;
		
		DEBUG (("sys_socketpair: fds[0] = %i, fds[1] = %i", fds[0], fds[1]));
		return 0;
	}
	
	DEBUG (("sys_socketpair: cannot connect the sockets"));
	
error:
	if (so1) so_free (so1);
	if (so2) so_free (so2);
	
	if (fp1) { fp1->links--; FP_FREE (fp1); }
	if (fp2) { fp2->links--; FP_FREE (fp2); }
	
	if (fd1 >= MIN_OPEN) FD_REMOVE (p, fd1);
	if (fd2 >= MIN_OPEN) FD_REMOVE (p, fd2);
	
	DEBUG (("sys_socketpair: failure %li", ret));
	return ret;
}

long _cdecl
sys_bind (short fd, struct sockaddr *addr, long addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_bind(%i, 0x%p, %li)", fd, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	r = (*so->ops->bind)(so, addr, addrlen);
	if (r) DEBUG (("sys_bind() failed with %li", r));
	return r;
}

long _cdecl
sys_listen (short fd, long backlog)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_listen(%i, %li)", fd, backlog));
	
	r = getsock (p, fd, &fp);
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

long _cdecl
sys_accept (short fd, struct sockaddr *addr, long *addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp, *newfp = NULL;
	short newfd = MIN_OPEN - 1;
	struct socket *so, *newso = NULL;
	long ret;
	
	DEBUG (("sys_accept(%i, 0x%p, 0x%p)", fd, addr, addrlen));
	
	ret = getsock (p, fd, &fp);
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
		short addrlen16 = *addrlen;
		ret = (*newso->ops->getname)(newso, addr, &addrlen16, PEER_ADDR);
		if (ret < 0)
		{
			DEBUG (("sys_accept: getname failed"));
			*addrlen = 0;
		}
		else
			*addrlen = addrlen16;
	}
	
	FP_DONE (p, newfp, newfd, FD_CLOEXEC);
	
	DEBUG (("sys_accept: newfd %i", newfd));
	return newfd;
	
error:
	so_drop (so, fp->flags & O_NDELAY);
error1:
	if (newso) so_free (newso);
	if (newfp) { newfp->links--; FP_FREE (newfp); }
	if (newfd >= MIN_OPEN) FD_REMOVE (p, newfd);
	
	return ret;
}

long _cdecl
sys_connect (short fd, const struct sockaddr *addr, long addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_connect(%i, 0x%p, %li)", fd, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r)
	{
		DEBUG (("sys_connect(%i): not a socket (%li)", fd, r));
		return r;
	}
	
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
			r = (*so->ops->connect)(so, addr, addrlen, fp->flags & O_NDELAY);
			if (r) DEBUG (("sys_connect(%i): error %li", fd, r));
			return r;
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

long _cdecl
sys_getsockname (short fd, struct sockaddr *addr, long *addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	short addrlen16;
	
	DEBUG (("sys_getsockname(%i, 0x%p, 0x%p)", fd, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	if (addrlen)
		addrlen16 = *addrlen;
	
	r = (*so->ops->getname)(so, addr, &addrlen16, SOCK_ADDR);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return r;
}

long _cdecl
sys_getpeername (short fd, struct sockaddr *addr, long *addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	short addrlen16;
	
	DEBUG (("sys_getpeername(%i, 0x%p, 0x%p)", fd, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
		return EINVAL;
	
	if (addrlen)
		addrlen16 = *addrlen;
	
	r =  (*so->ops->getname)(so, addr, &addrlen16, PEER_ADDR);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return r;
}

long _cdecl
sys_sendto (short fd, char *buf, long buflen, long flags, const struct sockaddr *addr, long addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	struct iovec iov[1] = {{ buf, buflen }};
	
	DEBUG (("sys_sendto(%i, %p, %li, %li, 0x%p, %li)", fd, buf, buflen, flags, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	return (*so->ops->send)(so, iov, 1, fp->flags & O_NDELAY, flags, addr, addrlen);
}

long _cdecl
sys_sendmsg (short fd, const struct msghdr *msg, long flags)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	
	DEBUG (("sys_sendmsg(%i, 0x%p, %li)", fd, msg, flags));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
#if 0
	if (msg->msg_accrights || msg->msg_accrightslen) {
		msg->msg_accrights = NULL;
		msg->msg_accrightslen = 0;
	}
#endif
	
	return (*so->ops->send)(so, msg->msg_iov, msg->msg_iovlen,
				fp->flags & O_NDELAY, flags,
				msg->msg_name, msg->msg_namelen);
}

long _cdecl
sys_recvfrom (short fd, char *buf, long buflen, long flags, struct sockaddr *addr, long *addrlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	struct iovec iov[1] = {{ buf, buflen }};
	short addrlen16;
	
	DEBUG (("sys_recvfrom(%i, %p, %li, %li, 0x%p, 0x%p)", fd, buf, buflen, flags, addr, addrlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	so = (struct socket *) fp->devinfo;
	if (so->state == SS_VIRGIN)
		return EINVAL;
	
	if (addrlen)
		addrlen16 = *addrlen;
	
	r = (*so->ops->recv)(so, iov, 1, fp->flags & O_NDELAY, flags, addr, &addrlen16);
	
	if (addrlen)
		*addrlen = addrlen16;
	
	return r;
}

long _cdecl
sys_recvmsg (short fd, struct msghdr *msg, long flags)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	struct socket *so;
	short namelen = msg->msg_namelen;
	
	DEBUG (("sys_recvmsg(%i, 0x%p, %li)", fd, msg, flags));
	
	r = getsock (p, fd, &fp);
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

long _cdecl
sys_setsockopt (short fd, long level, long optname, void *optval, long optlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	
	DEBUG (("sys_setsockopt(%i, %li, %li, 0x%p, %li)", fd, level, optname, optval, optlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	r = so_setsockopt ((struct socket *) fp->devinfo, level, optname, optval, optlen);
	if (r) DEBUG (("sys_setsockopt() -> %li", r));
	return r;
}

long _cdecl
sys_getsockopt (short fd, long level, long optname, void *optval, long *optlen)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	
	DEBUG (("sys_getsockopt(%i, %li, %li, 0x%p, 0x%p)", fd, level, optname, optval, optlen));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	r = so_getsockopt ((struct socket *) fp->devinfo, level, optname, optval, optlen);
	if (r) DEBUG (("sys_getsockopt() -> %li", r));
	return r;
}

long _cdecl
sys_shutdown (short fd, long how)
{
	PROC *p = get_curproc();
	FILEPTR *fp;
	long r;
	
	DEBUG (("sys_shutdown(%i, %li)", fd, how));
	
	r = getsock (p, fd, &fp);
	if (r) return r;
	
	return so_shutdown ((struct socket *) fp->devinfo, how);
}


/* internal socket functions */

long
so_setsockopt (struct socket *so, long level, long optname, void *optval, long optlen)
{
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
so_getsockopt (struct socket *so, long level, long optname, void *optval, long *optlen)
{
	if (so->state == SS_VIRGIN || so->state == SS_ISDISCONNECTED)
	{
		DEBUG (("so_getsockopt: virgin state -> EINVAL"));
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
so_shutdown (struct socket *so, long how)
{
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
		default:
			return EINVAL;
	}
	
	return (*so->ops->shutdown)(so, how);
}
