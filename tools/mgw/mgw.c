/*
 * Filename:
 * Version:
 * Author:       Frank Naumann, Jens Heitmann
 * Started:      1999-05
 * Last Updated: 1999-12-02
 * Last Updated: 2001-04-17
 * Target O/S:   FreeMiNT 1.16
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list <mint@fishpool.com>.
 * 
 * Copying:      Copyright 1999, 200, 2001 Frank Naumann <fnaumann@freemint.de>
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

# include "libsocket.h"
# include "syscalls.h"


# if 1
# define DEBUG(x)
# else
# define DEBUG(x) printf x
# endif


# if 0
# define TCP_VERSION	0x00010007L
# else
# define TCP_VERSION	0x00010000L
# endif

/*
 * Comments:
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

struct st_get_connected_param
{
	char *fnc;
};

struct st_get_dns_param
{
	char *fnc;
	short no;
};

struct st_get_loginparams_param
{
	char *fnc;
	char *user;
	char *pass;
};

struct st_get_options_param
{
	char *fnc;
};

struct st_gethostid_param
{
	char *fnc;
};

struct st_gethostip_param
{
	char *fnc;
};

struct st_set_loginparams_param
{
	char *fnc;
	char *user;
	char *pass;
};

struct st_set_options_param
{
	char *fnc;
	CFG_OPT *opt_ptr;
};

struct st_sethostid_param
{
	char *fnc;
	long new_id;
};

struct st_sethostip_param
{
	char *fnc;
	long new_ip;
};


/*
 * prototypes
 */
static ulong		_cdecl st_get_dns 		(struct st_get_dns_param p);
static void 		_cdecl st_get_loginparams	(struct st_get_loginparams_param p);
static short		_cdecl st_get_connected 	(struct st_get_connected_param p);
static CFG_OPT *	_cdecl st_get_options 		(struct st_get_options_param p);
static long 		_cdecl st_gethostid 		(struct st_gethostid_param p);
static long 		_cdecl st_gethostip 		(struct st_gethostip_param p);
static void 		_cdecl st_set_loginparams	(struct st_set_loginparams_param p);
static void 		_cdecl st_set_options 		(struct st_set_options_param p);
static short		_cdecl st_sethostid 		(struct st_sethostid_param p);
static short		_cdecl st_sethostip 		(struct st_sethostip_param p);


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
	0L, 				/*  5 */
	0L,
	0L,
	0L,
	0L,
	0L,				/* 10 */
	0L,
	0L,
	0L,
	0L,
	(long) st_socket,		/* 15 */
	(long) st_close,
	(long) st_connect,
	(long) st_bind,
	(long) st_write,
	(long) st_send,			/* 20 */
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
	0L, 				/* 40 */
	0L,
	0L,
	(long) st_select,
	(long) st_set_options,
	(long) st_get_options,		/* 45 */
	(long) st_accept,
	(long) st_listen,
	(long) st_set_loginparams,
	(long) st_get_loginparams,
	0L, 				/* 50 */
	0L,
	0L,
	0L,
	0L,
	0L, 				/* 55 */
	0L,  
	0L,
	0L,
	0L,
	0L, 				/* 60 */
	0L,
	0L,
	0L,
	(long) st_get_connected,
	0L, 				/* 65 */
	0L,
	0L,
	0L,
	0L,
	0L, 				/* 70 */
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
	(long) st_fcntl,				/* 1.7 */
	(long) TCP_VERSION, 		/* 85 */
	0L
};


/* --------------------------
   | Connection available ? |
   -------------------------- */
static short _cdecl
st_get_connected (struct st_get_connected_param p)
{
	DEBUG (("st_get_connected: -> 2\n"));
	
	/* 0 = No connection;
	 * 1 = Dial phase;
	 * 2 = Connection alive
	 */
	
	/* We always assume that MintNet is connected */
	return 2;
}

/* ----------------------------------
   | Get domain name server address |
   ---------------------------------- */
static ulong _cdecl
st_get_dns (struct st_get_dns_param p)
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

/* ---------------------------
   | Get login name/password |
   --------------------------- */
static void _cdecl
st_get_loginparams (struct st_get_loginparams_param p)
{
	DEBUG (("st_get_loginparams: -> nothing\n"));
}

/* -----------------------------------
   | Get pointer of option structure |
   ----------------------------------- */
static CFG_OPT * _cdecl
st_get_options (struct st_get_options_param p)
{
	DEBUG (("st_get_options: -> ok\n"));
	
	return (CFG_OPT *) -1;
}

static long _cdecl
st_gethostid (struct st_gethostid_param p)
{
	DEBUG (("st_gethostid: -> 0\n"));
	
	return 0;
}

static long _cdecl
st_gethostip (struct st_gethostip_param p)
{
	DEBUG (("st_gethostip: -> 0\n"));
	
	return 0;
}

/* ---------------------------
   | Set login name/password |
   --------------------------- */
static void _cdecl
st_set_loginparams (struct st_set_loginparams_param p)
{
	DEBUG (("st_set_loginparams: (%s, %s)\n", p.user, p.pass));
}

/* -----------------------------------
   | Set pointer to option structure |
   ----------------------------------- */
static void _cdecl
st_set_options (struct st_set_options_param p)
{
	DEBUG (("st_set_options: -> nothing\n"));
}

static short _cdecl
st_sethostid (struct st_sethostid_param p)
{
	DEBUG (("st_sethostid: (%li)\n", p.new_id));
	
	return 0;
}

static short _cdecl
st_sethostip (struct st_sethostip_param p)
{
	DEBUG (("st_sethostip: (%li)\n", p.new_ip));
	
	return 0;
}
