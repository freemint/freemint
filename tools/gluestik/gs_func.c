/*
 * Filename:     gs_func.c
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@cs.uni-magdeburg.de>
 * 
 * Portions copyright 1997, 1998, 1999 Scott Bigham <dsb@cs.duke.edu>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include <string.h>
# include <unistd.h>
# include <osbind.h>
# include <mintbind.h>
# include <fcntl.h>
# include <sys/socket.h>
# include <sys/ioctl.h>

# include <netdb.h>
# include <netinet/in.h>


# include <errno.h>
extern int errno;

# include "gs_func.h"

# include "gs_mem.h"
# include "gs_stik.h"


int
gs_xlate_error (int err, const char *funcname)
{
	int ret;
	
	switch (err)
	{
		case ENOSYS:
		case EOPNOTSUPP:
			ret = E_NOROUTINE;
			break;
		
		case EMFILE:
			ret = E_NOCCB;
			break;
		
		case EBADF:
		case ENOTSOCK:
			ret = E_BADHANDLE;
			break;
		
		case ENOMEM:
		case ESBLOCK:
			ret = E_NOMEM;
			break;
		
		case EBADARG:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
			ret = E_PARAMETER;
			break;
		
		case ECONNRESET:
			ret = E_RRESET;
			break;
		
		case ETIMEDOUT:
			ret = E_CNTIMEOUT;
			break;
		
		case ECONNREFUSED:
			ret = E_REFUSE;
			break;
		
		case ENETDOWN:
		case ENETUNREACH:
		case EHOSTDOWN:
		case EHOSTUNREACH:
			ret = E_UNREACHABLE;
			break;
		
		default:
			ret = -1000 + err;
			break;
	}
	
	if (funcname)
		DEBUG (("%s() returns %i (%s)",
			funcname, ret, do_get_err_text (ret)));
	
	return ret;
}


# define TABLE_SIZE	128
static GS *table [TABLE_SIZE];

int
gs_open (void)
{
	int fd;
	
	for (fd = 1; fd < TABLE_SIZE; fd++)
	{
		if (!table [fd])
		{
			GS *gs;
			
			gs = gs_mem_alloc (sizeof (*gs));
			if (!gs)
				return ENOMEM;
			
			bzero (gs, sizeof (*gs));
			gs->flags = GS_NOSOCKET;
			
			table [fd] = gs;
			
			return fd;
		}
	}
	
	return EMFILE;
}

void
gs_close (int fd)
{
	GS *gs = table [fd];
	
	if (gs)
	{
		if (!(gs->flags & GS_NOSOCKET))
			Fclose (gs->sock_fd);
		
		gs_mem_free (gs);
		
		table [fd] = NULL;
	}
}

GS *
gs_get (int fd)
{
	if ((fd >= 0) && (fd < TABLE_SIZE))
		return table [fd];
	
	return NULL;
}

int
gs_accept (int fd)
{
	GS *gs = gs_get (fd);
	struct sockaddr_in addr, addr2;
	socklen_t addr_size = sizeof (struct sockaddr_in);
	int in_fd;
	long fdflags;
	long ret;
	
	DEBUG (("gs_accept(%i)", fd));
	
	if (!gs
		|| gs->flags & GS_NOSOCKET
		|| !(gs->flags & GS_LISTENING))
	{
		DEBUG (("gs_accept: bad handle"));
		return E_BADHANDLE;
	}
	
	/* safe flags */
	fdflags = Fcntl (gs->sock_fd, 0L, F_GETFL);
	if (fdflags < 0)
	{
		DEBUG (("gs_accept: Fcntl(F_GETFL) returns %li", fdflags));
		return gs_xlate_error (-fdflags, "gs_accept");
	}
	
	/* switch to non-blocking mode */
	ret = Fcntl (gs->sock_fd, fdflags | O_NDELAY, F_SETFL);
	if (ret < 0)
	{
		DEBUG (("gs_accept: Fcntl(F_SETFL) returns %li", ret));
		return gs_xlate_error (-ret, "gs_accept");
	}
	
	in_fd = accept (gs->sock_fd, (struct sockaddr *) &addr, &addr_size);
	
	/* restore flags */
	ret = Fcntl (gs->sock_fd, fdflags, F_SETFL);
	if (ret < 0)
	{
		DEBUG (("gs_accept: Fcntl(F_SETFL) returns %li", ret));
		return gs_xlate_error (-ret, "gs_accept");
	}
	
	if (in_fd < 0)
	{
		DEBUG (("gs_accept: accept() returns %i (%i)", in_fd, errno));
		
		if (errno == EWOULDBLOCK)
		{
			DEBUG (("gs_accept: no connections arrived; returning E_LISTEN"));
			return E_LISTEN;
		}
		
		return gs_xlate_error (in_fd, "gs_accept");
	}
	
	/* Fill in the CIB. Part of the data we need came back from accept();
	 * get the rest via getsockname().
	 */
	addr_size = sizeof (struct sockaddr_in);
	if ((ret = getsockname (in_fd, (struct sockaddr *) &addr2, &addr_size)) < 0)
	{
		DEBUG (("gs_accept: getsockname() returns %li (%i)", ret, errno));
		return gs_xlate_error (ret, "gs_accept");
	}
	
	gs->cib.protocol = P_TCP;
	gs->cib.lport = addr2.sin_port;
	gs->cib.rport = addr.sin_port;
	gs->cib.rhost = addr.sin_addr.s_addr;
	gs->cib.lhost = addr2.sin_addr.s_addr;
	
	/* In STiK, an accept() "eats" the listen()'ed socket; we emulate that by
	 * closing it and replacing it with the newly accept()'ed socket.
	 */
	Fclose (gs->sock_fd);
	gs->sock_fd = in_fd;
	gs->flags &= ~GS_LISTENING;
	
	DEBUG (("gs_accept: returns 0"));
	return 0;
}

# if 0
int
gs_establish (int fd)
{
	GS *gs = gs_get (fd);
	ulong wfs;
	long n;
	
	DEBUG (("gs_establish(%i)", fd));
	
	if (!gs || gs->flags & GS_NOSOCKET || !(gs->flags & GS_PEND_OPEN))
	{
		DEBUG (("gs_establish: bad handle"));
		return E_BADHANDLE;
	}
	
	wfs = 1UL << gs->sock_fd;
	n = Fselect (1, 0L, &wfs, 0L);
	
	if (n > 0)
	{
		gs->flags &= ~GS_PEND_OPEN;
		
		DEBUG (("gs_establish: returns 0"));
		return 0;
	}
	else if (n < 0)
	{
		DEBUG (("gs_establish: Fselect() returns %li", n));
		return gs_xlate_error(n, "gs_establish");
	}
	
	DEBUG (("gs_establish: timeout"));
	return E_USERTIMEOUT;
}
# endif

long
gs_connect (int fd, uint32 rhost, int16 rport, uint32 lhost, int16 lport)
{
	GS *gs = gs_get (fd);
	struct sockaddr_in laddr;
	struct sockaddr_in raddr;
	socklen_t addr_size = sizeof (struct sockaddr_in);
	int sock_fd;
	int retval;
# if 0
	long fdflags;
	long r;
	int pending = 0;
# endif
	
	DEBUG (("gs_connect(%i, {%lu, %i}, {%lu, %i})",
		fd, lhost, lport, rhost, rport));
	
	if (!gs || !(gs->flags & GS_NOSOCKET))
	{
		DEBUG (("gs_connect: bad handle"));
		return E_BADHANDLE;
	}
	
	laddr.sin_family = AF_INET;
	laddr.sin_addr.s_addr = lhost;
	laddr.sin_port = lport;
	raddr.sin_family = AF_INET;
	raddr.sin_addr.s_addr = rhost;
	raddr.sin_port = rport;
	
	sock_fd = socket (AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0)
	{
		DEBUG (("gs_connect: socket() returns %i (%i)", sock_fd, errno));
		return gs_xlate_error (sock_fd, "gs_connect");
	}
# if 0
	/* save flags */
	fdflags = Fcntl (sock_fd, 0L, F_GETFL);
	if (fdflags < 0)
	{
		DEBUG (("gs_connect: Fcntl(F_GETFL) returns %li", fdflags));
		return gs_xlate_error (-fdflags, "gs_connect");
	}
	
	/* switch to non-blocking mode */
	r = Fcntl (sock_fd, fdflags | O_NDELAY, F_SETFL);
	if (r < 0)
	{
		DEBUG (("gs_connect: Fcntl(F_SETFL) returns %li", r));
		return gs_xlate_error (-r, "gs_connect");
	}
# endif
	
	retval = bind (sock_fd, (struct sockaddr *) &laddr, addr_size);
	if (retval < 0)
	{
		DEBUG (("gs_connect: bind() returns %i (%i)", retval, errno));
		return gs_xlate_error (retval, "gs_connect");
	}
	
	if (rhost == 0)
	{
		retval = listen (sock_fd, 5);
		if (retval < 0)
		{
			DEBUG (("gs_connect: listen() returns %i (%i)", retval, errno));
			return gs_xlate_error (retval, "gs_connect");
		}
	}
	else
	{
		retval = connect (sock_fd, (struct sockaddr *) &raddr, addr_size);
# if 0
		if (retval == EINPROGRESS)
		{
			pending = 1;
		}
		else
# endif
		if (retval < 0)
		{
			DEBUG (("gs_connect: connect() returns %i (%i)", retval, errno));
			return gs_xlate_error (retval, "gs_connect");
		}
	}
	
	/* Fill in the CIB. Data for the remote end was provided by the parms
	 * in the connect() case, and filled in as zero by the stub in the
	 * listen() case; data for the local end was probably changed by the
	 * bind(), so update it via getsockname().
	 */
	retval = getsockname (sock_fd, (struct sockaddr *) &laddr, &addr_size);
	if (retval < 0)
	{
		DEBUG (("gs_connect: getsockname() returns %i (%i)", retval, errno));
		return gs_xlate_error (retval, "gs_connect");
	}
	
	gs->cib.protocol = P_TCP;
	gs->cib.rhost = raddr.sin_addr.s_addr;
	gs->cib.rport = raddr.sin_port;
	gs->cib.lhost = laddr.sin_addr.s_addr;
	gs->cib.lport = laddr.sin_port;
	
	gs->sock_fd = sock_fd;
	if (rhost == 0)
	{
		gs->flags = GS_LISTENING;
	}
	else
	{
# if 0
		gs->flags = pending ? GS_PEND_OPEN : 0;
# else
		gs->flags = 0;
# endif
	}
	
	DEBUG (("gs_connect: returns 0"));
	return 0;
}

long
gs_udp_open (int fd, uint32 rhost, int16 rport)
{
	GS *gs = gs_get (fd);
	struct sockaddr_in addr, addr2;
	socklen_t addr_size = sizeof (struct sockaddr_in);
	int sock_fd;
	int retval;
# if 0
	long fdflags;
	long r;
	int pending = 0;
# endif
	
	DEBUG (("gs_udp_open(%i, %lu, %i)", fd, rhost, rport));
	
	if (!gs || !(gs->flags & GS_NOSOCKET))
	{
		DEBUG (("gs_udp_open: bad handle"));
		return E_BADHANDLE;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = rhost;
	addr.sin_port = rport;
	
	sock_fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0)
	{
		DEBUG (("gs_udp_open: socket() returns %i (%i)", sock_fd, errno));
		return gs_xlate_error (sock_fd, "gs_udp_open");
	}
	
# if 0
	fdflags = Fcntl (sock_fd, 0L, F_GETFL);
	if (fdflags < 0)
	{
		DEBUG (("gs_udp_open: Fcntl(F_GETFL) returns %li", fdflags));
		return gs_xlate_error (-fdflags, "gs_udp_open");
	}
	
	r = Fcntl (sock_fd, fdflags | O_NDELAY, F_SETFL);
	if (r < 0)
	{
		DEBUG (("gs_udp_open: Fcntl(F_SETFL) returns %li", r));
		return gs_xlate_error (-r, "gs_udp_open");
	}
# endif

	retval = connect (sock_fd, (struct sockaddr *) &addr, addr_size);
# if 0
	if (retval == EINPROGRESS)
	{
		pending = 1;
	}
	else
# endif
	if (retval < 0)
	{
		DEBUG (("gs_udp_open: connect() returns %i (%i)", retval, errno));
		return gs_xlate_error (retval, "gs_udp_open");
	}
	
	/* Fill in the CIB. Part of the data we need came from our
	 * parameters; get the rest via getsockname().
	 */
	addr_size = sizeof (struct sockaddr_in);
	retval = getsockname (sock_fd, (struct sockaddr *) &addr2, &addr_size);
	if (retval < 0)
	{
		DEBUG (("gs_udp_open: getsockame() returns %i (%i)", retval, errno));
		return gs_xlate_error (retval, "gs_udp_open");
	}
	
	gs->cib.protocol = P_TCP;
	gs->cib.lport = addr2.sin_port;
	gs->cib.rport = addr.sin_port;
	gs->cib.rhost = addr.sin_addr.s_addr;
	gs->cib.lhost = addr2.sin_addr.s_addr;
	
	gs->sock_fd = sock_fd;
# if 0
	gs->flags = pending ? GS_PEND_OPEN : 0;
# else
	gs->flags = 0;
# endif
	
	DEBUG (("gs_udp_open: returns 0"));
	return 0;
}

long
gs_wait (int fd, int timeout)
{
	GS *gs = gs_get (fd);
	long rfs, wfs;
	long n;
	
	DEBUG (("gs_wait(%i, %i)", fd, timeout));
	
	if (!gs || gs->flags & GS_NOSOCKET)
	{
		DEBUG (("gs_wait: bad handle"));
		return E_BADHANDLE;
	}
	
	/* TCP_wait_state() expects its timeout in seconds, Fselect() in
	 * milliseconds...
	 */
	timeout *= 1000;
	
	rfs = wfs = 1L << gs->sock_fd;
	
	if ((gs->flags & (GS_LISTENING
# if 0
				| GS_PEND_OPEN
# endif
				)) == 0)
	{
		DEBUG (("gs_wait: returns E_NORMAL (LISTENING)"));
		return E_NORMAL;
	}
	
	if (gs->flags & GS_LISTENING)
	{
		n = Fselect ((short) timeout, &rfs, 0L, 0L);
	}
	else
	{
		n = Fselect (timeout, 0L, &wfs, 0L);
	}
	
	if (n > 0)
	{
		/* Finish establishing the connection.
		 */
		if (gs->flags & GS_LISTENING)
			n = gs_accept (fd);
# if 0
		else
			n = gs_establish (fd);
# endif
		if (n == 0)
		{
			DEBUG (("gs_wait: returns 0"));
			return 0;
		}
		else
		{
			DEBUG (("gs_wait:: gs_%s() returns %li",
				((gs->flags & GS_LISTENING) ? "accept" : "establish"), n));
			return n;
		}
	}
	else if (n == 0)
	{
		DEBUG (("gs_wait: timeout"));
		return E_USERTIMEOUT;
	}
	
	DEBUG (("gs_wait: Fselect() returns %li", n));
	return gs_xlate_error (n, "gs_wait");
}

long
gs_canread (int fd)
{
	GS *gs = gs_get (fd);
	long r, n;
	
	/* DEBUG (("gs_canread(%i)", fd)); */
	
	if (gs->flags & GS_NOSOCKET)
	{
		DEBUG (("gs_canread: bad handle"));
		return E_BADHANDLE;
	}
	
	if ((gs->flags & GS_LISTENING) && gs_accept (fd) != 0)
	{
		DEBUG (("gs_canread: no connection arrived"));
		return E_LISTEN;
	}
	
# if 0
	if ((gs->flags & GS_PEND_OPEN) && gs_establish (fd) != 0)
	{
		DEBUG (("gs_canread: open in progress"));
		return 0;
	}
# endif
	
	r = Fcntl (gs->sock_fd, (long) &n, FIONREAD);
# if 0
	if (r == -ENOTCONN)
		return 0;
# endif
	if (r < 0)
	{
		DEBUG (("gs_canread: Fcntl(FIONREAD) returns %li", r));
		return gs_xlate_error (-r, "gs_canread");
	}
	
	if (n == 0x7FFFFFFFUL)
	{
		DEBUG (("gs_canread: EOF reached"));
		return E_EOF;
	}
	
	/* DEBUG (("gs_canread: returns %li", n)); */
	return n;
}

long
gs_read_delim (int fd, char *buf, int len, char delim)
{
	int n = 0;
	long r;
	
	DEBUG (("gs_read_delim(%i, %p, %i, %c)", fd, (void *) buf, len, delim));
	
	while (n < len - 1)
	{
		r = gs_read (fd, buf + n, 1);
		if (r < 0)
			return r;
		
		if (r == 0)
		{
			DEBUG (("gs_read_delim: end of data"));
			
			return E_NODATA;
		}
		
		if (buf[n] == delim)
			break;
		
		n++;
		continue;
	}
	
	buf [n] = '\0';
	
	DEBUG (("gs_read_delim: returns %i", n));
	return n;
}

NDB *
gs_readndb (int fd)
{
	GS *gs = gs_get (fd);
	NDB *ndb;
	long n;
	long ret;
	
	DEBUG (("gs_readndb(%i)", fd));
	
	if (gs->flags & GS_NOSOCKET)
	{
		DEBUG (("gs_readndb: bad handle"));
		return NULL;
	}
	
	if ((gs->flags & GS_LISTENING) && gs_accept (fd) != 0)
	{
		DEBUG (("gs_readndb: no connections arrived"));
		return NULL;
	}
	
# if 0
	if ((gs->flags & GS_PEND_OPEN) && gs_establish (fd) != 0)
	{
		DEBUG (("gs_readndb: open in progress"));
		return NULL;
	}
# endif
	
	n = gs_canread (fd);
	if (n <= 0)
	{
		DEBUG (("gs_readndb: no data available"));
		return NULL;
	}
	
	if (n > 65535)
		n = 65535;
	
	ndb = gs_mem_alloc (sizeof (*ndb));
	if (!ndb)
	{
		DEBUG (("gs_readndb: no memory for NDB"));
		return NULL;
	}
	
	ndb->ptr = gs_mem_alloc (n);
	if (!ndb->ptr)
	{
		gs_mem_free (ndb);
		
		DEBUG (("gs_readndb: no memory for data"));
		return NULL;
	}
	
	ret = Fread (gs->sock_fd, n, ndb->ptr);
	if (ret < 0)
	{
		gs_mem_free (ndb->ptr);
		gs_mem_free (ndb);
		
		DEBUG (("gs_readndb: Fread() returned %li", ret));
		return NULL;
	}
	
	ndb->ndata = ndb->ptr;
	ndb->len = n;
	ndb->next = 0;
	
	DEBUG (("gs_readndb: read %li bytes, returns %p", n, (void *) ndb));
	return ndb;
}

long
gs_write (int fd, char *buf, long buflen)
{
	GS *gs = gs_get (fd);
	long r, n;
	
	DEBUG (("gs_write(%i, %p, %li)", fd, (void *) buf, buflen));
	
	if (gs->flags & GS_NOSOCKET)
	{
		DEBUG (("gs_write: bad handle"));
		
		return E_BADHANDLE;
	}
	
	if ((gs->flags & GS_LISTENING) && gs_accept (fd) != 0)
	{
		DEBUG (("gs_write: no connections arrived"));
		
		return E_LISTEN;
	}
	
# if 0
	/* This is kind of a lie, but at least it's an error code a STiK app
	 * will be prepared to deal with.
	 */
	if ((gs->flags & GS_PEND_OPEN) && gs_establish (fd) != 0)
	{
		DEBUG (("gs_write: open in progress"));
		
		return E_OBUFFULL;
	}
# endif
	
	/* STiK has no provisions for writing part of a buffer and saving the
	 * rest for later; so make sure we can write the whole thing.
	 */
	r = Fcntl (gs->sock_fd, (long) &n, FIONWRITE);
	if (r < 0)
	{
		DEBUG (("gs_write: Fcntl(FIONWRITE) returned %li", r));
		return gs_xlate_error (-r, "gs_write");
	}
	
	if (n < buflen)
	{
		DEBUG (("gs_write: can only write %li bytes; returning E_OBUFFULL", n));
		return E_OBUFFULL;
	}
	
	/* Okay, we can safely write.
	 */
	r = Fwrite (gs->sock_fd, buflen, buf);
	if (r < 0)
	{
		DEBUG (("gs_write: Fwrite() returned %li", r));
		return gs_xlate_error (-r, "gs_write");
	}
	
	/* Okay, according to the Fcntl(), we should have been able to
	 * write everything; warn if we didn't.
	 */
	if (r < buflen)
		DEBUG (("gs_write: only got %li of %li bytes", r, buflen));
	
	DEBUG (("gs_write: returns E_NORMAL"));
	return E_NORMAL;
}

long
gs_read (int fd, char *buf, long buflen)
{
	GS *gs = gs_get (fd);
	long r, n;
	
	DEBUG (("gs_read(%i, %p, %li)", fd, (void *) buf, buflen));
	
	if (gs->flags & GS_NOSOCKET)
	{
		DEBUG (("gs_read: bad handle"));
		
		return E_BADHANDLE;
	}
	
	if ((gs->flags & GS_LISTENING) && gs_accept (fd) != 0)
	{
		DEBUG (("gs_read: no connections arrived"));
		
		return E_LISTEN;
	}
	
# if 0
	if ((gs->flags & GS_PEND_OPEN) && gs_establish (fd) != 0)
	{
		DEBUG (("gs_read: open in progress"));
		
		return E_NODATA;
	}
# endif
	
	/* STiK has no provisions for reading less than a full buffer's worth
	 * of data; make sure we can fill the buffer.
	 */
	n = gs_canread (fd);
	if (n < 0)
		return n;
	
	if (n < buflen)
	{
		DEBUG (("gs_read: only %li bytes available", n));
		
		return E_NODATA;
	}
	
	/* Okay, we can safely read.
	 */
	r = Fread (gs->sock_fd, buflen, buf);
	if (r < 0)
	{
		DEBUG (("gs_read: Fread() returns %li", r));
		return gs_xlate_error (-r, "gs_read");
	}
	
	/* Okay, according to the Fcntl(), we should have been able to
	 * fill the entire buffer; warn if we didn't.
	 */
	if (r < buflen)
		DEBUG (("gs_read: unable to read all %li bytes", buflen));
	
	DEBUG (("gs_read: returns %li (%li)", buflen, r));
	return buflen;
}

int
gs_resolve (char *dn, char **rdn, uint32 *alist, int16 lsize)
{
	static char buf [4096];
	static char lock = 0;
	
	struct hostent *host = NULL;
	PMSG pmsg;
	char **raddr;
	long r;
	int ret;
	
	
	DEBUG (("gs_resolve(\"%s\", %p, %p, %i)", dn, rdn, alist, lsize));
	
	
	while (lock)
		sleep (1);
	
	lock = 1;
	
	
	strcpy (buf, dn);
	pmsg.msg1 = 1;
	pmsg.msg2 = (long) buf;
	
	DEBUG (("gs_resolve: Wait for Pmsg receive on [%s]!", buf));
	r = Pmsg (2, GS_GETHOSTBYNAME, &pmsg);
	DEBUG (("gs_resolve: Pmsg received = %li, %lx!", r, pmsg.msg2));
	
	if (r != 0)
	{
		DEBUG (("gs_resolve: Pmsg() returns %li", r));
		
		ret = gs_xlate_error (-r, "gs_resolve");
		goto out;
	}
	
	DEBUG (("gs_resolve: gethostbyname() returns %lx", pmsg.msg2));
	host = (struct hostent *) pmsg.msg2;
	if (!host)
	{
		extern int h_errno;
		
		switch (h_errno)
		{
			case HOST_NOT_FOUND:
				ret = E_NOHOSTNAME;
				break;
			case NO_DATA:
				ret = E_DNSNOADDR;
				break;
			case TRY_AGAIN:
			case NO_RECOVERY:
			default:
				ret = E_CANTRESOLVE;
				break;
		}
		
		DEBUG (("gs_resolve: h_errno = %d -> %i", h_errno, ret));
		goto out;
	}
	
	if (rdn)
	{
		DEBUG (("gs_resolve: Copying name: \"%s\"", host->h_name));
		
		*rdn = gs_mem_alloc (strlen (host->h_name) + 1);
		strcpy (*rdn, host->h_name);
	}
	
	/* BUG:  assumes addresses returned have type struct in_addr
	 */
	for (ret = 0, raddr = host->h_addr_list; *raddr && ret < lsize; ret++, raddr++)
	{
		DEBUG (("gs_resolve: Copying address %p to array element %i",
			(void *) ((struct in_addr *) (*raddr))->s_addr, ret));
		
		alist [ret] = ((struct in_addr *) (*raddr))->s_addr;
	}
	
out:
	lock = 0;
	
	DEBUG (("gs_resolve: returns %i", ret));
	return ret;
}
