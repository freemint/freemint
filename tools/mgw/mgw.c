/*
 * Filename:
 * Version:
 * Author:       Frank Naumann, Jens Heitmann
 * Started:      1999-05
 * Last Updated: 2001-06-09
 * Target O/S:	 MiNT
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@cs.uni-magdeburg.de>
 *               Copyright 1999 Jens Heitmann
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
 */
		
# include "global.h"
# include "options.h"


# ifdef SHOWDEBUG
# define DEBUG(x) printf x
# else
# define DEBUG(x)
# endif


# define TCP_VERSION	0x00010007L

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
 * get/set_dns		- not implemented, but normally unimportant, because in MiNTnet the
 * 			  standard calls for name server resolvement are used.
 * 
 * get_loginparams	- not implemented. It's only interesting to determine the current user,
 * 			  but it may only be useful to provide more comfort to the user, i.e. a
 * 			  FTP application may prompt for the user, and displays the current one as
 * 			  the default.
 * 
 * set_loginparams	- not implemented. Login will be done directly in Mint.
 * 
 * get_options		- returns always -1L. Not neccessary under MiNT
 * set_options		- ignores parameter. Not neccessary under MiNT 	
 * 
 * get/sethostip	- not implemented.
 * get/sethostid	- not implemented.
 * 
 * get_connected	-> Non standard, we always return 2, which means network connection alive.
 *
 */

/* Some comments about the standarization of the calls */

/* bind 		-> Standard Berkeley
 * closesocket		-> Standard Berkeley
 * connect		-> Standard Berkeley
 * get_connected	-> Non standard
 * get_dns		-> Non standard
 * gethostbyname	-> Standard Berkeley
 * gethostbyaddr	-> Standard Berkeley
 * gethostname		-> Standard Berkeley
 * gethostid		-> Standard Berkeley
 * gethostip		-> Standard Berkeley
 * gethostname		-> Standard Berkeley
 * get_loginparams	-> Non standard
 * getpeername		-> Standard Berkeley
 * getservbyname	-> Standard Berkeley
 * getservbyport	-> Standard Berkeley
 * getsockname		-> Standard Berkeley
 * getsockopt 		-> Standard Berkeley
 * read 		-> Standard Berkeley
 * recv 		-> Standard Berkeley
 * recvfrom 		-> Standard Berkeley
 * recvmsg		-> Standard Berkeley (and not implemented in the Draconis drivers)
 * seek 		-> Standard Berkeley
 * send 		-> Standard Berkeley
 * sendto 		-> Standard Berkeley
 * sendmsg		-> Standard Berkeley (and not implemented in the Draconis drivers)
 * set_dns		-> Non standard
 * set_loginparams	-> Non standard
 * setsockopt 		-> Standard Berkeley
 * sethostid		-> Standard Berkeley
 * sethostip		-> Standard Berkeley
 * shutdown 		-> Standard Berkeley
 * sock_accept		-> Standard Berkeley
 * sock_listen		-> Standard Berkeley
 * socket 		-> Standard Berkeley
 * socket_select	-> Standard Berkeley
 * write		-> Standard Berkeley
 */

/*
 * prototypes
 */
static int		_cdecl st_bind			(long fnc, int sfd, struct sockaddr *address, int addrlen);
static int		_cdecl st_closesocket 		(long fnc, int sfd);
static int		_cdecl st_connect 		(long fnc, int sfd, struct sockaddr *servaddr, int addrlen);
static ulong		_cdecl st_get_dns 		(long fnc, int no);
static void 		_cdecl st_get_loginparams	(long fnc, char *user, char *pass);
static int		_cdecl st_get_connected 	(long fnc);
static CFG_OPT *	_cdecl st_get_options 		(long fnc);
static struct hostent * _cdecl st_gethostbyname 	(long fnc, char *name);
static struct hostent * _cdecl st_gethostbyaddr 	(long fnc, char *haddr, int len, int type);
static int		_cdecl st_gethostname 		(long fnc, char *name, int namelen);
static long 		_cdecl st_gethostid 		(long fnc);
static long 		_cdecl st_gethostip 		(long fnc);
static struct servent * _cdecl st_getservbyname 	(long fnc, char *name, char *proto);
static struct servent * _cdecl st_getservbyport 	(long fnc, int port, char *proto);
static int		_cdecl st_getsockname 		(long fnc, int sfd, struct sockaddr *addr, int *namelen);
static int		_cdecl st_getpeername 		(long fnc, int sfd, struct sockaddr *addr, int *namelen);
static size_t 		_cdecl st_read			(char *fnc, int sfd, void *buf, size_t len);
static size_t 		_cdecl st_recv			(char *fnc, int sfd, void * buf, size_t len, unsigned flags);
static size_t 		_cdecl st_recvfrom		(char *fnc, int sfd, void * buf, size_t len, unsigned flags, struct sockaddr *addr, int *addr_len);
static size_t 		_cdecl st_recvmsg 		(char *fnc, int sfd, struct msghdr *msg, unsigned int flags); 				 
static int		_cdecl st_seek			(char *fnc, int sfd, size_t offset, int whence);
static size_t 		_cdecl st_send			(char *fnc, int sfd, void *buf, size_t len, unsigned flags);
static size_t 		_cdecl st_sendto		(char *fnc, int sfd, void *buf, size_t len, unsigned flags, struct sockaddr *addr, int addr_len);
static size_t 		_cdecl st_sendmsg 		(char *fnc, int sfd, struct msghdr *msg, unsigned int flags);
static void 		_cdecl st_set_loginparams	(long fnc, char *user, char *pass);
static void 		_cdecl st_set_options 		(long fnc, CFG_OPT *opt_ptr);
static int		_cdecl st_sethostid 		(long fnc, long new_id);
static int		_cdecl st_sethostip 		(long fnc, long new_id);
static int		_cdecl st_shutdown		(long fnc, int sfd, int how);
static int		_cdecl st_sock_accept 		(long fnc, int sfd, struct sockaddr *addr, int *addr_len);
static int		_cdecl st_sock_listen 		(long fnc, int sfd, int backlog);
static int		_cdecl st_socket		(long fnc, int domain, int type, int protocol);
static int		_cdecl st_socket_select 	(long fnc, int nfds, st_fd_set *readfds, st_fd_set *writefds, st_fd_set *exceptfds, struct timeval *timeout); 								
static size_t 		_cdecl st_write 					(long fnc, int sfd, void *buf, size_t len);

/* 1.7 */
static int		_cdecl st_getsockopt		(int sfd, int level, int optname, char *optval, int *optlen);
static int		_cdecl st_setsockopt		(int sfd, int level, int optname, char *optval, int optlen);
static long 		_cdecl st_sockfcntl 		(int sfd, int cmd, long args);


/*
 * cookie entry table
 */
long tcp_cookie [] =
{
	1L,
	0L,
	0L,
	0L,
	0L,
	0L, 											/*	5 */
	0L,
	0L,
	0L,
	0L,
	0L, 											/* 10 */
	0L,
	0L,
	0L,
	0L,
	(long) st_socket, 		/* 15 */
	(long) st_closesocket,
	(long) st_connect,
	(long) st_bind,
	(long) st_write,
	(long) st_send, 		/* 20 */
	(long) st_sendto,
	(long) st_sendmsg,
	(long) st_seek,
	(long) st_read,
	(long) st_recv, 		/* 25 */
	(long) st_recvfrom,
	(long) st_recvmsg,
	0L,
	0L,
	(long) st_gethostid,		/* 30 */
	(long) st_sethostid,
	(long) st_getsockname,
	(long) st_getpeername,
	(long) st_gethostip,
	(long) st_sethostip,		/* 35 */
	(long) st_shutdown,
	0L,
	0L,
	0L,
	0L, 											/* 40 */
	0L,
	0L,
	(long) st_socket_select,
	(long) st_set_options,
	(long) st_get_options,		/* 45 */
	(long) st_sock_accept,
	(long) st_sock_listen,
	(long) st_set_loginparams,
	(long) st_get_loginparams,
	0L, 											/* 50 */
	0L,
	0L,
	0L,
	0L,
	0L, 											/* 55 */
	0L,  
	0L,
	0L,
	0L,
	0L, 											/* 60 */
	0L,
	0L,
	0L,
	(long) st_get_connected,
	0L, 											/* 65 */
	0L,
	0L,
	0L,
	0L,
	0L, 											/* 70 */
	(long) st_get_dns,
	0L,
	0L,
	0L,
	(long) st_gethostbyname,	/* 75 */
	(long) st_gethostbyaddr,
	(long) st_gethostname,
	(long) st_getservbyname,
	(long) st_getservbyport,
	(long) st_setsockopt, 		/* 80 */	/* 1.7 */
	(long) st_getsockopt,				/* 1.7 */
	(long) 0L,
	(long) 0L,
	(long) st_sockfcntl,				/* 1.7 */
	(long) TCP_VERSION, 		/* 85 */
	0L
};

/*
 * Mapper data part
 */

/* -----------------------------------------
  | Map into a Draconis compatible handle |
  ----------------------------------------- */
static inline int
map_it (int mint_sfd)
{
	if (mint_sfd < 0)
	{
		return mint_sfd;
	}
	else
	{
		return mint_sfd + 0x1000 + 1;
	}
}

/* ------------------------------------
  | Map Draconis into MintNet handle |
  ------------------------------------ */
static inline int
map_sfd (int sfd)
{
	return sfd - 0x1000 - 1;
}

/* -------------------
  | Create a socket |
  ------------------- */
static int _cdecl
st_socket (long fnc, int domain, int type, int proto)
{
	struct socket_cmd cmd;
	int sockfd, r;
	
	DEBUG (("st_socket: enter (%i, %i, %i)\n", domain, type, proto));
	
	sockfd = Fopen (SOCKDEV, O_RDWR);
	if (sockfd < 0)
	{
		DEBUG (("st_socket: Fopen fail -> r = %i\n", sockfd));
		return sockfd;
	}
	
	DEBUG (("st_socket: Fopen ok (%i)\n", sockfd));
	
	cmd.cmd 	= SOCKET_CMD;
	cmd.domain	= domain;
	cmd.type	= type;
	cmd.protocol	= proto;
	
	r = Fcntl (sockfd, (long) &cmd, SOCKETCALL);
	if (r)
	{
		Fclose (sockfd);
		DEBUG (("st_socket: Fcntl fail -> r = %i\n", r));
		return r;
	}
	
	r = map_it (sockfd);
	if (r < 0)
	{
		Fclose (sockfd);
		DEBUG (("st_socket: map_it fail -> r = %i\n", r));
		return r;
	}
	
	DEBUG (("st_socket: return r = %i\n", r));
	return r;
}

/* -----------------------------------------------
   | Bind a name to a socket. Nothing much       |
   | to do here since it's the protocol's        |
   | responsibility to handle the local address. |
   ----------------------------------------------- */
static int _cdecl
st_bind (long fnc, int sfd, struct sockaddr *address, int addrlen)
{
	struct bind_cmd cmd;
	
	DEBUG (("st_bind [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.addr	= address;
	cmd.addrlen 	= (short) addrlen;
	cmd.cmd 	= BIND_CMD;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* -------------------------------
   | Accept connection on socket |
   ------------------------------- */
static int _cdecl
st_sock_accept (long fnc, int sfd, struct sockaddr *addr, int *addrlen)
{
	struct accept_cmd cmd;
	int newsock;
	
	DEBUG (("st_sock_accept [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd 	= ACCEPT_CMD;
	cmd.addr	= addr;
	cmd.addrlen 	= (short *) addrlen;
	
	newsock = Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
	
	return map_it (newsock);
}

/* ----------------
   | Close socket |
   ---------------- */
static int _cdecl
st_closesocket (long fnc, int sfd)
{
	int r;
	
	DEBUG (("st_closesocket [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	r = Fclose (map_sfd (sfd));
	
	DEBUG (("st_closesocket [%i, %i]: Fclose = %i\n", sfd, map_sfd (sfd), r));
	return r;
}

/* --------------------
   | Connect a socket |
   -------------------- */
static int _cdecl
st_connect (long fnc, int sfd, struct sockaddr *servaddr, int addrlen)
{
	struct connect_cmd cmd;
	int r;
	
	DEBUG (("st_connect [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.addr	= servaddr;
	cmd.addrlen 	= (short) addrlen;
	cmd.cmd 	= CONNECT_CMD;
	
	r = Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
	DEBUG (("st_connect [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	return r;
}

/* --------------------------
   | Connection available ? |
   -------------------------- */
static int _cdecl
st_get_connected (long fnc)
{
	DEBUG (("st_get_connected: -> 2\n"));
	
	/* 0 = No connection;
	 * 1 = Dial phase;
	 * 2 = Connection alive
	 */
	return 2; 	/* We always assume that MintNet is connected */
}

/* ----------------------------------
   | Get domain name server address |
   ---------------------------------- */
static ulong _cdecl
st_get_dns (long fnc, int no)
{
	DEBUG (("st_get_dns: -> 0\n"));
	
	/* no = 1: DNS-address 1
	 * no = 2: DNS-address 2
	 *
	 * return dns-ipaddr;
	 * return 0; (= Undefined)
	 */
	return 0;
}

/* --------------------
   | Get host by name |
   -------------------- */
static struct hostent * _cdecl
st_gethostbyname (long fnc, char *name)
{
# if 1
	static char buf [4096];
	
	struct hostent *host = NULL;
	PMSG pmsg;
	long r;
	
	strcpy (buf, name);
	pmsg.msg1 = (long) buf;
	pmsg.msg2 = 0;
	
	DEBUG (("Wait for Pmsg receive on [%s]!\n", buf));
	r = Pmsg (2, MGW_GETHOSTBYNAME, &pmsg);
	DEBUG (("Pmsg received = %li, %lx!\n", r, pmsg.msg2));
	
	if (r == 0)
		host = (struct hostent *) pmsg.msg2;
	
# else
	struct hostent *host = NULL;
	int i = 10;
	
	DEBUG (("st_gethostbyname: enter (%s) [%li]\n", name, Pdomain (-1)));
	Pdomain (1);
	
retry:
	host = gethostbyname (name);
	if (host)
		DEBUG (("st_gethostbyname: ok (%s)\n", host->h_name));
	else
	{
		DEBUG (("st_gethostbyname: fail!\n"));
		if (i--)
			goto retry;
	}
	
# endif
	return host;
}

/* --------------------
   | Get host by addr |
   -------------------- */
static struct hostent * _cdecl
st_gethostbyaddr (long fnc, char *haddr, int len, int type)
{
	struct hostent *r;
	
	DEBUG (("st_gethostbyaddr: enter (%s)\n", haddr));
	
	r = gethostbyaddr (haddr, len, type);
	if (r)
		DEBUG (("st_gethostbyaddr: ok (%s)\n", r->h_name));
	else
		DEBUG (("st_gethostbyaddr: fail!\n"));
	
	return r;
}

/* -----------------------
   | Get local host name |
   ----------------------- */
static int _cdecl
st_gethostname (long fnc, char *name, int namelen)
{
	int r;
	
	DEBUG (("st_gethostname: enter\n"));
	
	r = gethostname (name, namelen);
	if (!r)
		DEBUG (("st_gethostname: ok (%s)\n", name));
	else
		DEBUG (("st_gethostname: fail (%i, errno = %i)\n", r, errno));
	
	return r;
}

/* ---------------------------
   | Get login name/password |
   --------------------------- */
static void _cdecl
st_get_loginparams (long fnc, char *user, char *pass)
{
	DEBUG (("st_get_loginparams: -> nothing\n"));
}

/* -----------------------------------
   | Get pointer of option structure |
   ----------------------------------- */
static CFG_OPT * _cdecl
st_get_options (long fnc)
{
	DEBUG (("st_get_options: -> ok\n"));
	
	return (CFG_OPT *) -1;
}

static long _cdecl
st_gethostid (long fnc)
{
	DEBUG (("st_gethostid: -> 0\n"));
	
	return 0;
}

static long _cdecl
st_gethostip (long fnc)
{
	DEBUG (("st_gethostip: -> 0\n"));
	
	return 0;
}

/* -----------------------------------------
   | This does both peername and sockname. |
   ----------------------------------------- */
static int _cdecl
st_getpeername (long fnc, int sfd, struct sockaddr *addr, int *addrlen)
{
	struct getpeername_cmd cmd;
	
	DEBUG (("st_getpeername [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd 	= GETPEERNAME_CMD;
	cmd.addr	= addr;
	cmd.addrlen 	= (short *) addrlen;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* -----------------------
   | Get service by name |
   ----------------------- */
static struct servent * _cdecl
st_getservbyname(long fnc, char *name, char *proto)
{
	struct servent *r;
	
	DEBUG (("st_getservbyname: enter (%s, %s)\n", name, proto));
	
	r = getservbyname (name, proto);
	if (!r)
		DEBUG (("st_getservbyname: fail\n"));
	else
		DEBUG (("st_getservbyname: enter (%s, %s)\n", r->s_name, r->s_proto));
	
	return r;
}

/* -----------------------
   | Get service by port |
  ----------------------- */
static struct servent * _cdecl
st_getservbyport(long fnc, int port, char *proto)
{
	DEBUG (("st_getservbyport: enter (%i, %s)\n", port, proto));
	
	return getservbyport (port, proto);
}

/* -----------------------------------------
   | This does both peername and sockname. |
   ----------------------------------------- */
static int _cdecl
st_getsockname (long fnc, int sfd, struct sockaddr *addr, int *addrlen)
{
	struct getsockname_cmd cmd;
	
	DEBUG (("st_getsockname [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd 	= GETSOCKNAME_CMD;
	cmd.addr	= addr;
	cmd.addrlen 	= (short *) addrlen;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* --------------------
   | Read from socket |
   -------------------- */
static size_t _cdecl
st_read (char *fnc, int sfd, void *buf, size_t len)
{
	DEBUG (("st_getsockname [%i, %i]: -> -1\n", sfd, map_sfd (sfd)));
	
	/* return read (map_sfd (sfd), buf, len); */
	return -1;
}

/* --------------------------------
   | Receive datagram from socket |
   -------------------------------- */
static size_t _cdecl
st_recv (char *fnc, int sfd, void *buf, size_t buflen, unsigned flags)
{
	struct recv_cmd cmd;
	void *zwbuf = NULL;
	int r;
	
	DEBUG (("st_recv [%i, %i]: enter (buf %lx, len %li, flags %x)\n", sfd, map_sfd (sfd), buf, buflen, flags));
	
	cmd.cmd 	= RECV_CMD;
	cmd.buflen	= buflen;
	cmd.flags 	= flags;
	
	if (!buf)
		cmd.buf = zwbuf = malloc (buflen);
	else
		cmd.buf = buf;
	
	if (!cmd.buf)
		return -ENOMEM;
	
	r = Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
	
	if (zwbuf)
		free (zwbuf);
	
	DEBUG (("st_recv [%i, %i]: return %i\n", sfd, map_sfd (sfd), r));
	
	return r;
}

/* -------------------------------------------------------------
   | Receive a frame from the socket and optionally record the |
   | address of the sender. 																	 |
   ------------------------------------------------------------- */
static size_t _cdecl
st_recvfrom (char *fnc, int sfd, void *buf, size_t buflen, unsigned flags, struct sockaddr *addr, int *addrlen)
{
	struct recvfrom_cmd cmd;
	
	DEBUG (("st_recvfrom [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd = RECVFROM_CMD;
	cmd.buf = buf;
	cmd.buflen =	buflen;
	cmd.flags = flags;
	cmd.addr =	addr;
	cmd.addrlen = (short *) addrlen;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* -------------------------
   | BSD recvmsg interface |
   ------------------------- */
static size_t _cdecl
st_recvmsg (char *fnc, int sfd, struct msghdr *msg, unsigned int flags)
{
	struct recvmsg_cmd cmd;
	
	DEBUG (("st_recvmsg [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd 	= RECVMSG_CMD;
	cmd.msg 	= msg;
	cmd.flags 	= flags;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* -----------------------------
   | Sockets are not seekable. |
   ----------------------------- */
static int _cdecl
st_seek (char *fnc, int sfd, size_t offset, int whence)
{
	DEBUG (("st_seek [%i, %i]: -> ESPIPE\n", sfd, map_sfd (sfd)));
	
# ifndef ESPIPE
# define ESPIPE 6
# endif
	return -ESPIPE;
}

/* ----------------------------------
   | Send a datagram down a socket. |
   ---------------------------------- */
static size_t _cdecl
st_send (char *fnc, int sfd, void *buf, size_t buflen, unsigned flags)
{
	struct send_cmd cmd;
	
	DEBUG (("st_send [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd 	= SEND_CMD;
	cmd.buf 	= buf;
	cmd.buflen	= buflen;
	cmd.flags 	= flags;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* ---------------------------------------
   | Send a datagram to a given address. |
   --------------------------------------- */
static size_t _cdecl
st_sendto (char *fnc, int sfd, void *buf, size_t buflen, unsigned flags, struct sockaddr *addr, int addrlen)
{
	struct sendto_cmd cmd;
	
	DEBUG (("st_sendto [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.addr	= addr;
	cmd.addrlen 	= (short) addrlen;
	
	cmd.cmd 	= SENDTO_CMD;
	cmd.buf 	= buf;
	cmd.buflen	= buflen;
	cmd.flags 	= flags;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* -------------------------
   | BSD sendmsg interface |
   ------------------------- */
static size_t _cdecl
st_sendmsg (char *fnc, int sfd, struct msghdr *msg, unsigned int flags)
{
	struct sendmsg_cmd cmd;
	
	DEBUG (("st_sendmsg [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.msg 	= msg;
	cmd.cmd 	= SENDMSG_CMD;
	cmd.flags 	= flags;
	
	return Fcntl (map_sfd (sfd), (long)&cmd, SOCKETCALL);
}

/* ---------------------------
   | Set login name/password |
   --------------------------- */
static void _cdecl
st_set_loginparams (long fnc, char *user, char *pass)
{
	DEBUG (("st_set_loginparams: (%s, %s)\n", user, pass));
}

/* -----------------------------------
	 | Set pointer to option structure |
	 ----------------------------------- */
static void _cdecl
st_set_options (long fnc, CFG_OPT *opt_ptr)
{
	DEBUG (("st_set_options: -> nothing\n"));
}

static int _cdecl
st_sethostid (long fnc, long new_id)
{
	DEBUG (("st_sethostid: (%li)\n", new_id));
	
	return 0;
}

static int _cdecl
st_sethostip (long fnc, long new_ip)
{
	DEBUG (("st_sethostip: (%li)\n", new_ip));
	
	return 0;
}

/* -------------------
   | Shutdown socket |
   ------------------- */
static int _cdecl
st_shutdown (long fnc, int sfd, int how)
{
	struct shutdown_cmd cmd;
	
	DEBUG (("st_shutdown [%i, %i]: enter\n", sfd, map_sfd (sfd)));
		
	cmd.cmd = SHUTDOWN_CMD;
	cmd.how = how;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* --------------------
   | Set listen state |
   -------------------- */
static int _cdecl
st_sock_listen (long fnc, int sfd, int backlog)
{
	struct listen_cmd cmd;
	
	DEBUG (("st_sock_listen [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	cmd.cmd = LISTEN_CMD;
	cmd.backlog = backlog;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

/* ------------------------
   | PrÅfen auf exception |
   ------------------------ */
static inline int
check_exception (fd_set *mint_fds, st_fd_set *st_fds)
{
	int i, m;
	
	ST_FD_ZERO (st_fds);
	
	DEBUG (("check_exception "));
	
	m = 0;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (FD_ISSET (i, mint_fds))
		{
			struct recv_cmd cmd;
			int r;
			char c;
			
			cmd.cmd 	= RECV_CMD;
			cmd.buflen	= 1;
			cmd.flags 	= MSG_PEEK /* MSG_PEEK = 2 -> nicht warten */;
			cmd.buf 	= &c;
			
			r = Fcntl (i, (long) &cmd, SOCKETCALL);
			DEBUG (("[Fcntl = %i] ", r));
			
			if (r == 0)
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
static inline int
remap_fdset (fd_set *mint_fds, st_fd_set *st_fds, char *type, int is_read)
{
	int i, m;
	
	ST_FD_ZERO (st_fds);
	
	DEBUG (("remap %s ", type));
	
	m = 0;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (FD_ISSET (i, mint_fds))
		{
			if (is_read)
			{
				struct recv_cmd cmd;
				int r;
				char c;
			
				cmd.cmd 	= RECV_CMD;
				cmd.buflen	= 1;
				cmd.flags 	= MSG_PEEK /* MSG_PEEK = 2 -> nicht warten */;
				cmd.buf 	= &c;
			
				r = Fcntl (i, (long) &cmd, SOCKETCALL);
				DEBUG (("[Fcntl = %i] ", r));
			
				if (r != 0)
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
static inline void
map_fdset (st_fd_set *st_fds, fd_set *mint_fds, char *type)
{
	int i;
	
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
static int _cdecl
st_socket_select (long fnc, int nfds, st_fd_set *readfds, st_fd_set *writefds, st_fd_set *exceptfds, struct timeval *timeout)
{
	fd_set rfds, wfds, efds;
	fd_set *rptr = NULL, *wptr = NULL, *eptr = NULL;
	int r, j;
	unsigned long mtime = 0;
	
	if (timeout)
	{
		mtime = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
		if (mtime == 0)
			mtime = 1;
	}
	
	DEBUG (("st_socket_select: enter (nfds %i, %lx, %lx, %lx, %lu)\n", nfds, readfds, writefds, exceptfds, mtime));
	
	if (readfds)
	{
		rptr = &rfds;
		map_fdset (readfds, rptr, "readfds");
	}
	
	if (writefds)
	{
		wptr = &wfds;
		map_fdset (writefds, wptr, "writefds");
	}
	
	if (exceptfds)
	{
		eptr = &efds;
		map_fdset (exceptfds, eptr, "exceptfds");
	}
	
	r = select (nfds - 0x1000 - 1, rptr, wptr, NULL, timeout);
	/* nfds nur der Sauberkeit halber
	 * wieder eingebaut.
	 * eptr hier rausgenommen,
	 * wird unten erledigt.
	 */
	
	DEBUG (("select returned %i\n", r));
	
	if (!r)
	{
	/* if 0 is returned the structures are not cleared.
	   This will make us problems, during remapping it. 
	   So we clear it before... */
	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	}

	r = 0;
	if (rptr)
	{
		/* Remap read bits */
		j = remap_fdset (rptr, readfds, "readfds", 1);
		if (j > r)
			r = j;
	}
	
	if (wptr)
	{
		/* Remap write bits */
		j = remap_fdset (wptr, writefds, "writefds", 0);
		if (j > r)
			r = j;
	}
	
	if (eptr)
	{
		j = check_exception (eptr, exceptfds);
		if (j > r)
			r = j;
	}
	
	DEBUG (("st_socket_select: select return %i (%i)\n", r, r + 0x1000 + 1));
	if (r > 0)
		return r + 0x1000 + 1;
	
	return r;
}

/* ---------------------------
   | Write data to a socket. |
   --------------------------- */
static size_t _cdecl
st_write (long fnc, int sfd, void *buf, size_t len)
{
	DEBUG (("st_write [%i, %i]: enter\n", sfd, map_sfd (sfd)));
	
	/* return write (map_sfd (sfd), buf, len); */
	return -1;
}

/* New version 1.7 */

/* !Internal message to frank! :-(( */
/* Hier fehlte mir jetzt die Docu fÅr die Mint-Calls */

static int _cdecl 
st_getsockopt (int sfd, int level, int optname, char *optval, int *optlen)
{
	struct getsockopt_cmd cmd;
	long optlen32;
	int r;
	
	if (optlen)
		optlen32 = *optlen;
	
	cmd.cmd		= GETSOCKOPT_CMD;
	cmd.level	= level;
	cmd.optname	= optname;
	cmd.optval	= optval;
	cmd.optlen	= &optlen32;
	
	r = Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
	
	if (optlen)
		*optlen = optlen32;
	
	return r;
}

static int _cdecl 
st_setsockopt (int sfd, int level, int optname, char *optval, int optlen)
{
	struct setsockopt_cmd cmd;
	
	cmd.cmd		= SETSOCKOPT_CMD;
	cmd.level	= level;
	cmd.optname	= optname;
	cmd.optval	= optval;
	cmd.optlen	= optlen;
	
	return Fcntl (map_sfd (sfd), (long) &cmd, SOCKETCALL);
}

static long _cdecl 
st_sockfcntl (int sfd, int cmd, long args)
{
	return Fcntl (map_sfd (sfd), args, cmd);
}
