/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000, 2001 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1999 Jens Heitmann
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
 * 
 * 
 * Author: Frank Naumann, Jens Heitmann
 * Started: 1999-05
 * 
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 * 
 */
		
# include "global.h"
# include "options.h"

# include "syscalls.h"


# if 1
# define DEBUG(x)
# else
# define DEBUG(x) printf x
# endif


/*
 * Comments:
 * 
 * read()
 * write()		- are missing! Maybe not important. Applications should not use this calls, 
 * 			  an application should better use calls to recv/send.
 * 			  (Adamas/Marathon are not using these calls)
 * 
 * get/setsockopt	- Implemented since version 1.7 (supports only SO_FIONBIO).
 *
 */

/*
 * map the system error codes to the propritaer DRACONIS codes
 */
static short
r_map (long r)
{
	switch (r)
	{
		case -EAGAIN:
			return -ST_EAGAIN;
		case -EPROTO:
			return -ST_EPROTO;
		case -ENOTSOCK:
			return -ST_ENOTSOCK;
		case -EOPNOTSUPP:
			return -ST_EOPNOTSUPP;
		case -EADDRINUSE:
			return -ST_EADDRINUSE;
		case -ENOBUFS:
			return -ST_ENOBUFS;
		case -EISCONN:
			return -ST_EISCONN;
		case -ENOTCONN:
			return -ST_ENOTCONN;
		case -EALREADY:
			return -ST_EALREADY;
		case -EINPROGRESS:
			return -ST_EINPROGRESS;
	}
	
	return (short) r;
}


/*
 * Mapper data part
 */

/* -----------------------------------------
  | Map into a Draconis compatible handle |
  ----------------------------------------- */
static inline short
map_it (short mint_sfd)
{
	if (mint_sfd < 0)
		return mint_sfd;
	else
		return mint_sfd + 0x1000 + 1;
}

/* ------------------------------------
  | Map Draconis into MintNet handle |
  ------------------------------------ */
static inline short
map_sfd (short sfd)
{
	return sfd - 0x1000 - 1;
}


/* -------------------
  | Create a socket |
  ------------------- */
short _cdecl
st_socket (struct st_socket_param p)
{
	long sockfd;
	short r;
	
	DEBUG (("st_socket: enter (%i, %i, %i)\n", p.domain, p.type, p.proto));
	
	sockfd = Fsocket (p.domain, p.type, p.proto);
	if (sockfd < 0)
	{
		DEBUG (("st_socket: socket fail (%li)\n", sockfd));
		return r_map (sockfd);
	}
	
	r = map_it (sockfd);
	if (r < 0)
	{
		Fclose (sockfd);
		
		DEBUG (("st_socket: map_it fail -> r = %i\n", r));
		return r_map (r);
	}
	
	DEBUG (("st_socket: return r = %i\n", r));
	return r;
}

/* ----------------
   | Close socket |
   ---------------- */
short _cdecl
st_close (struct st_close_param p)
{
	long r;
	
	r = Fclose (map_sfd (p.sfd));
	
	DEBUG (("st_close [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* -----------------------------
   | Sockets are not seekable. |
   ----------------------------- */
short _cdecl
st_seek (struct st_seek_param p)
{
	DEBUG (("st_seek [%i, %i]: -> ESPIPE\n", p.sfd, map_sfd (p.sfd)));
	
# ifndef ESPIPE
# define ESPIPE 6
# endif
	return -ESPIPE;
}

/* -----------------
   | socket fnctl. |
   ----------------- */
short _cdecl 
st_fcntl (struct st_fcntl_param p)
{
	long r;
	
	DEBUG (("st_fcntl [%i, %i]: cmd %i, args 0x%lx\n", p.sfd, map_sfd (p.sfd), p.cmd, p.args));
	
	switch (p.cmd)
	{
		case ST_SETFL:
		{
			long args = p.args;
			
			if (args & ST_O_NONBLOCK)
			{
				args &= ~ST_O_NONBLOCK;
				args |= O_NONBLOCK;
			}
			
			DEBUG (("Fcntl (%i, 0x%lx, F_SETFL)\n", map_sfd (p.sfd), args));
			r = Fcntl (map_sfd (p.sfd), args, F_SETFL);
			
			break;
		}
		case ST_GETFL:
		{
			r = Fcntl (map_sfd (p.sfd), 0, F_GETFL);
			DEBUG (("Fcntl (%i, 0, F_GETFL) -> 0x%lx\n", map_sfd (p.sfd), r));
			
			if (r > 0 && r & O_NONBLOCK)
			{
				r &= ~O_NONBLOCK;
				r |= ST_O_NONBLOCK;
			}
			
			break;
		}
		case ST_FIONBIO:
		{
			r = Fcntl (map_sfd (p.sfd), 0, F_GETFL);
			
			if (p.args)
				r |= O_NONBLOCK;
			else
				r &= ~O_NONBLOCK;
			
			Fcntl (map_sfd (p.sfd), r, F_SETFL);
			
			r = 0;
			DEBUG (("Fcntl (ST_FIONBIO) -> %s\n", p.args ? "on" : "off"));
			
			break;
		}
		default:
		{
			r = Fcntl (map_sfd (p.sfd), p.args, p.cmd);
			
			break;
		}
	}
	
	DEBUG (("st_fcntl [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}


/* --------------------
   | Read from socket |
   -------------------- */
size_t _cdecl
st_read (struct st_read_param p)
{
	DEBUG (("st_read [%i, %i]: -> -1\n", p.sfd, map_sfd (p.sfd)));
	
	/* return read (map_sfd (sfd), buf, len); */
	return -1;
}

/* ---------------------------
   | Write data to a socket. |
   --------------------------- */
size_t _cdecl
st_write (struct st_write_param p)
{
	DEBUG (("st_write [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	/* return write (map_sfd (p.sfd), p.buf, p.len); */
	return -1;
}


/* --------------------------------
   | Receive datagram from socket |
   -------------------------------- */
size_t _cdecl
st_recv (struct st_recv_param p)
{
	void *buf = NULL;
	void *zwbuf = NULL;
	long r;
	
	DEBUG (("st_recv [%i, %i]: enter (buf %p, len %li, flags %x)\n", p.sfd, map_sfd (p.sfd), p.buf, p.buflen, p.flags));
	
	buf = p.buf;
	
	if (!buf)
		buf = zwbuf = malloc (p.buflen);
	
	if (!buf)
		return -ENOMEM;
	
	r = Frecvfrom (map_sfd (p.sfd), buf, p.buflen, p.flags, 0, 0);
	
	if (zwbuf)
		free (zwbuf);
	
	DEBUG (("st_recv [%i, %i]: return %li\n", p.sfd, map_sfd (p.sfd), r));
	
	return r_map (r);
}

/* -------------------------------------------------------------
   | Receive a frame from the socket and optionally record the |
   | address of the sender. 																	 |
   ------------------------------------------------------------- */
size_t _cdecl
st_recvfrom (struct st_recvfrom_param p)
{
	long addrlen32;
	long r;
	
	DEBUG (("st_recvfrom [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	if (p.addrlen)
		addrlen32 = *(p.addrlen);
	
	r = Frecvfrom (map_sfd (p.sfd), p.buf, p.buflen, p.flags, p.addr, &addrlen32);
	
	if (p.addrlen)
		*(p.addrlen) = addrlen32;
	
	return r_map (r);
}

/* -------------------------
   | BSD recvmsg interface |
   ------------------------- */
size_t _cdecl
st_recvmsg (struct st_recvmsg_param p)
{
	struct msghdr msg;
	long r;
	
	DEBUG (("st_recvmsg [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	/* struct st_msghdr
	 * {
	 * 	void *msg_name;
	 * 	short msg_namelen;
	 * 	struct iovec *msg_iov;
	 * 	long msg_iovlen;
	 * 	void *msg_accrights;
	 * 	long msg_accrightslen;
	 * };
	 */
	
	msg.msg_name = p.msg->msg_name;
	msg.msg_namelen = p.msg->msg_namelen;
	msg.msg_iov = p.msg->msg_iov;
	msg.msg_iovlen = p.msg->msg_iovlen;
	msg.msg_control = p.msg->msg_accrights;
	msg.msg_controllen = p.msg->msg_accrightslen;
	
	r = Frecvmsg (map_sfd (p.sfd), &msg, p.flags);
	
	p.msg->msg_namelen = msg.msg_namelen;
	
	return r_map (r);
}

/* ----------------------------------
   | Send a datagram down a socket. |
   ---------------------------------- */
size_t _cdecl
st_send (struct st_send_param p)
{
	long r;
	
	DEBUG (("st_send [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	r = Fsendto (map_sfd (p.sfd), p.buf, p.buflen, p.flags, 0, 0);
	
	return r_map (r);
}

/* ---------------------------------------
   | Send a datagram to a given address. |
   --------------------------------------- */
size_t _cdecl
st_sendto (struct st_sendto_param p)
{
	long r;
	
	DEBUG (("st_sendto [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	r = Fsendto (map_sfd (p.sfd), p.buf, p.buflen, p.flags, p.addr, p.addrlen);
	
	return r_map (r);
}

/* -------------------------
   | BSD sendmsg interface |
   ------------------------- */
size_t _cdecl
st_sendmsg (struct st_sendmsg_param p)
{
	struct msghdr msg;
	long r;
	
	DEBUG (("st_sendmsg [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	/* struct st_msghdr
	 * {
	 * 	void *msg_name;
	 * 	short msg_namelen;
	 * 	struct iovec *msg_iov;
	 * 	long msg_iovlen;
	 * 	void *msg_accrights;
	 * 	long msg_accrightslen;
	 * };
	 */
	
	msg.msg_name = p.msg->msg_name;
	msg.msg_namelen = p.msg->msg_namelen;
	msg.msg_iov = p.msg->msg_iov;
	msg.msg_iovlen = p.msg->msg_iovlen;
	msg.msg_control = p.msg->msg_accrights;
	msg.msg_controllen = p.msg->msg_accrightslen;
	
	r = Fsendmsg (map_sfd (p.sfd), &msg, p.flags);
	
	return r_map (r);
}


/* -------------------------------
   | Accept connection on socket |
   ------------------------------- */
short _cdecl
st_accept (struct st_accept_param p)
{
	long newsock;
	long addrlen;
	
	DEBUG (("st_accept [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	if (p.addrlen)
		addrlen = *(p.addrlen);
	
	newsock = Faccept (map_sfd (p.sfd), p.addr, &addrlen);
	
	if (p.addrlen)
		*(p.addrlen) = addrlen;
	
	return map_it (newsock);
}

/* -----------------------------------------------
   | Bind a name to a socket. Nothing much       |
   | to do here since it's the protocol's        |
   | responsibility to handle the local address. |
   ----------------------------------------------- */
short _cdecl
st_bind (struct st_bind_param p)
{
	long r;
	
	DEBUG (("st_bind [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	r = Fbind (map_sfd (p.sfd), p.address, p.addrlen);
	
	DEBUG (("st_bind [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* --------------------
   | Connect a socket |
   -------------------- */
short _cdecl
st_connect (struct st_connect_param p)
{
	long r;
	extern long compatibility;
	
	DEBUG (("st_connect [%i, %i]: addr %p addrlen %i\n", p.sfd, map_sfd (p.sfd), p.addr, p.addrlen));
	
	r = Fconnect (map_sfd (p.sfd), p.addr, p.addrlen);

	DEBUG (("st_connect [%i, %i]: -> %li (%i)\n", p.sfd, map_sfd (p.sfd), r, r_map (r)));

	if (compatibility)
	{
		r = r_map(r);
		if (compatibility <= 0x00010007)
		{
			if (r == -ST_EISCONN)
				r = 0;
			else
				if (r == -ST_EALREADY)
					r = -ST_EINPROGRESS;
		}
		
		return r;
	}
	else	
		if (r == -EALREADY)
		  return -ST_EINPROGRESS;
		else
   		  return r_map (r);
}

/* --------------------
   | Set listen state |
   -------------------- */
short _cdecl
st_listen (struct st_listen_param p)
{
	long r;
	
	DEBUG (("st_listen [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	r = Flisten (map_sfd (p.sfd), p.backlog);
	
	DEBUG (("st_listen [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* -------------------
   | Shutdown socket |
   ------------------- */
short _cdecl
st_shutdown (struct st_shutdown_param p)
{
	long r;
	
	DEBUG (("st_shutdown [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	r = Fshutdown (map_sfd (p.sfd), p.how);
	
	DEBUG (("st_shutdown [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}


/* -----------------------------------------
   | This does both peername and sockname. |
   ----------------------------------------- */
short _cdecl
st_getpeername (struct st_getpeername_param p)
{
	long addrlen32;
	long r;
	
	DEBUG (("st_getpeername [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	if (p.addrlen)
		addrlen32 = *(p.addrlen);
	
	r = Fgetpeername (map_sfd (p.sfd), p.addr, &addrlen32);
	
	if (p.addrlen)
		*(p.addrlen) = addrlen32;
	
	DEBUG (("st_getpeername [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* -----------------------------------------
   | This does both peername and sockname. |
   ----------------------------------------- */
short _cdecl
st_getsockname (struct st_getsockname_param p)
{
	long addrlen32;
	long r;
	
	DEBUG (("st_getsockname [%i, %i]: enter\n", p.sfd, map_sfd (p.sfd)));
	
	if (p.addrlen)
		addrlen32 = *(p.addrlen);
	
	r = Fgetsockname (map_sfd (p.sfd), p.addr, &addrlen32);
	
	if (p.addrlen)
		*(p.addrlen) = addrlen32;
	
	DEBUG (("st_getsockname [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* -----------------------
   | get socket options. |
   ----------------------- */
short _cdecl 
st_getsockopt (struct st_getsockopt_param p)
{
	long optlen32;
	long r;
	
	if (p.optlen)
		optlen32 = *(p.optlen);
	
	r = Fgetsockopt (map_sfd (p.sfd), p.level, p.optname, p.optval, &optlen32);
	
	if (p.optlen)
		*(p.optlen) = optlen32;
	
	DEBUG (("st_getsockopt [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}

/* -----------------------
   | set socket options. |
   ----------------------- */
short _cdecl 
st_setsockopt (struct st_setsockopt_param p)
{
	long r;
	
	r = Fsetsockopt (map_sfd (p.sfd), p.level, p.optname, p.optval, p.optlen);
	
	DEBUG (("st_setsockopt [%i, %i]: -> %li\n", p.sfd, map_sfd (p.sfd), r));
	return r_map (r);
}


/* --------------------------
   | check for an exception |
   -------------------------- */
static short
check_exception (fd_set *mint_fds, st_fd_set *st_fds)
{
	short i, m;
	
	ST_FD_ZERO (st_fds);
	
	DEBUG (("check_exception "));
	
	m = 0;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (FD_ISSET (i, mint_fds))
		{
			long r;
			char c;
			
			r = Frecvfrom (i, &c, 1, MSG_PEEK, 0, 0);
			DEBUG (("[Frecvfrom = %li] ", r));
			
			if (r <= 0 && r != -EAGAIN) /* Exception on error or EOF ; 0 = EOF */
			{
				/* Wenn EOF dann exception, sollte zwar eigentlich nur
				 * einmal gesetzt werden, und ist auch etwas unelegant,
				 * sollte aber funktionieren.
				 */
				DEBUG (("# %i -> %i ", i, i));
				
				ST_FD_SET (i, st_fds);
				m = i;
			}
		}
	}
	
	DEBUG (("- return %i\n", m));
	return m;
}

/* --------------------------
   | Map FDSET file handles |
   -------------------------- */
static short
remap_fdset (fd_set *mint_fds, st_fd_set *st_fds, char *type, int is_read)
{
	short i, m;
	
	ST_FD_ZERO (st_fds);
	
	DEBUG (("remap %s ", type));
	
	m = 0;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (FD_ISSET (i, mint_fds))
		{
			if (is_read)
			{
				long r;
				char c;
				
				r = Frecvfrom (i, &c, 1, MSG_PEEK, 0, 0);
				DEBUG (("[Frecvfrom = %li] ", r));
				
				if (r > 0)	/* only return codes > 0 are ok */
				{
					DEBUG (("# %i -> %i ", i, i));
				
					ST_FD_SET (i, st_fds);
					m = i;
				}
			}
			else
			{
			ST_FD_SET (i, st_fds);
			m = i;
			}
		}
	}
	
	DEBUG (("- return %i\n", m));
	return m;
}

/* --------------------------
   | Map FDSET file handles |
   -------------------------- */
static void
map_fdset (st_fd_set *st_fds, fd_set *mint_fds, char *type)
{
	short i;
	
	FD_ZERO (mint_fds);
	
	DEBUG (("map %s ", type));
	
	for (i = 0; i < ST_FD_SETSIZE; i++)
	{
		if (ST_FD_ISSET (i, st_fds))
		{
			DEBUG (("# %i -> %i ", i, i));
			
			FD_SET (i, mint_fds);
		}
	}
	
	DEBUG (("\n"));
}

/* -------------------------
   | Execute socket select |
   ------------------------- */
short _cdecl
st_select (struct st_select_param p)
{
	fd_set rfds, wfds, efds;
	fd_set *rptr = NULL, *wptr = NULL, *eptr = NULL;
	unsigned long mtime = 0;
	short j;
	long r;
	
	if (p.timeout)
	{
		mtime = p.timeout->tv_sec * 1000 + p.timeout->tv_usec / 1000;
		if (mtime == 0)
			mtime = 1;
	}
	
	DEBUG (("st_select: enter (nfds %i, %p, %p, %p, %lu)\n", p.nfds, p.readfds, p.writefds, p.exceptfds, mtime));
	
	if (p.readfds)
	{
		rptr = &rfds;
		map_fdset (p.readfds, rptr, "readfds");
	}
	
	if (p.writefds)
	{
		wptr = &wfds;
		map_fdset (p.writefds, wptr, "writefds");
	}
	
	if (p.exceptfds)
	{
		eptr = &efds;
		map_fdset (p.exceptfds, eptr, "exceptfds");
	}
	
	r = select (p.nfds - 0x1000 - 1, rptr, wptr, NULL, p.timeout);
	/* nfds nur der Sauberkeit halber
	 * wieder eingebaut.
	 * eptr hier rausgenommen,
	 * wird unten erledigt.
	 */
	
	DEBUG (("select returned %li\n", r));
	
	if (!r)
	{
	/* if 0 is returned the structures are not cleared.
	   This will make us problems, during remapping it. 
	   So we clear it before... */
//	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	}

	r = 0;
	if (rptr)
	{
		j = remap_fdset (rptr, p.readfds, "readfds", 1);
		if (j > r)
			r = j;
	}
	
	if (wptr)
	{
		j = remap_fdset (wptr, p.writefds, "writefds", 0);
		if (j > r)
			r = j;
	}
	
	if (eptr)
	{
		j = check_exception (eptr, p.exceptfds);
		if (j > r)
			r = j;
	}
	
	DEBUG (("st_select: select return %li (%li)\n", r, r + 0x1000 + 1));
	if (r > 0)
		return r + 0x1000 + 1;
	
	return r_map (r);
}
