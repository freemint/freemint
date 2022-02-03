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


# if 1
# define DEBUG(x)
# else
# define DEBUG(x) printf x
# endif

void getfunc_unlock (void);

struct st_gethostbyname_param
{
	char *fnc;
	char *name;
};

struct st_gethostbyaddr_param
{
	char *fnc;
	char *haddr;
	short len;
	short type;
};

struct st_gethostname_param
{
	char *fnc;
	char *name;
	short namelen;
};

struct st_getservbyname_param
{
	char *fnc;
	char *name;
	char *proto;
};

struct st_getservbyport_param
{
	char *fnc;
	short port;
	char *proto;
};


/*
 * synchronization stuff
 * neccessary to avoid reentrancy problems with blocking
 * lib calls
 */

static volatile int lock = 0;
static volatile int getlock = 0;

static void getfunc_lock(void)
{
	while (getlock)
	{
		sleep (1);
	}
	
	getlock = 1;
}

void
getfunc_unlock (void)
{
	getlock = 0;
}

static void
libsocket_lock (void)
{
	while (lock)
	{
		sleep (1);
	}
	
	lock = 1;
}

static void
libsocket_unlock (void)
{
	lock = 0;
}

/*
 * prototypes
 */
struct st_hostent * _cdecl st_gethostbyname (struct st_gethostbyname_param p);
struct st_hostent * _cdecl st_gethostbyaddr (struct st_gethostbyaddr_param p);
struct st_hostent * _cdecl stbl_gethostbyname (struct st_gethostbyname_param p);
struct st_hostent * _cdecl stbl_gethostbyaddr (struct st_gethostbyaddr_param p);
short               _cdecl st_gethostname   (struct st_gethostname_param p);
struct st_servent * _cdecl st_getservbyname (struct st_getservbyname_param p);
struct st_servent * _cdecl st_getservbyport (struct st_getservbyport_param p);
struct st_servent * _cdecl stbl_getservbyname (struct st_getservbyname_param p);
struct st_servent * _cdecl stbl_getservbyport (struct st_getservbyport_param p);

/* --------------------
   | Get host by name |
   -------------------- */
struct st_hostent * _cdecl
st_gethostbyname (struct st_gethostbyname_param p)
{
	static char buf [4096];
	
	PMSG pmsg;
	long r;
	
	libsocket_lock ();
	
	strcpy (buf, p.name);
	pmsg.msg1 = MGW_GETHOSTBYNAME;
	pmsg.msg2 = (long) buf;
	
	DEBUG (("Wait for Pmsg receive on [%s]!\n", buf));
	r = Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
	DEBUG (("Pmsg received = %li, %lx!\n", r, pmsg.msg2));
	
	/* there is a still a race with the static data */
	libsocket_unlock ();
	
	if ((r == 0)
		&& (pmsg.msg1 == MGW_GETHOSTBYNAME)
		&& (pmsg.msg2 != 0))
	{
		static struct st_hostent r_st;
		struct hostent *host = (struct hostent *) pmsg.msg2;
		
		/* struct st_hostent
		 * {
		 * 	char *h_name;
		 * 	char **h_aliases;
		 * 	short h_addrtype;
		 * 	short h_length;
		 * 	char **h_addr_list;
		 * };
		 */
		
		r_st.h_name = host->h_name;
		r_st.h_aliases = host->h_aliases;
		r_st.h_addrtype = host->h_addrtype;
		r_st.h_length = host->h_length;
		r_st.h_addr_list = host->h_addr_list;
		
		return &r_st;
	}
	
	return NULL;
}

/* ----------------------------------------
   | Blocking version of st_gethostbyname |
   ---------------------------------------- */
struct st_hostent * _cdecl
stbl_gethostbyname (struct st_gethostbyname_param p)
{
getfunc_lock (); /* Unlock should be done by the application */
return st_gethostbyname(p);
}

/* --------------------
   | Get host by addr |
   -------------------- */
struct st_hostent * _cdecl
st_gethostbyaddr (struct st_gethostbyaddr_param p)
{
	static long buf[3];
	
	PMSG pmsg;
	long r;
	
	DEBUG (("st_gethostbyaddr: enter (%s)\n", p.haddr));
	
	libsocket_lock ();
	
	buf[0] = (long) p.haddr;
	buf[1] = p.len;
	buf[2] = p.type;
	
	pmsg.msg1 = MGW_GETHOSTBYADDR;
	pmsg.msg2 = (long) &buf[0];
	
	DEBUG (("st_gethostbyaddr: wait for Pmsg receive!\n"));
	r = Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
	DEBUG (("st_gethostbyaddr: Pmsg received = %li!\n", r));
	
	/* there is a still a race with the static data */
	libsocket_unlock ();
	
	if ((r == 0)
		&& (pmsg.msg1 == MGW_GETHOSTBYADDR)
		&& (pmsg.msg2 != 0))
	{
		static struct st_hostent r_st;
		struct hostent *host = (struct hostent *) pmsg.msg2;
		
		DEBUG (("st_gethostbyaddr: ok (%s)\n", host->h_name));
		
		/* struct st_hostent
		 * {
		 * 	char *h_name;
		 * 	char **h_aliases;
		 * 	short h_addrtype;
		 * 	short h_length;
		 * 	char **h_addr_list;
		 * };
		 */
		
		r_st.h_name = host->h_name;
		r_st.h_aliases = host->h_aliases;
		r_st.h_addrtype = host->h_addrtype;
		r_st.h_length = host->h_length;
		r_st.h_addr_list = host->h_addr_list;
		
		return &r_st;
	}
	
	DEBUG (("st_gethostbyaddr: fail!\n"));
	return NULL;
}

/* ----------------------------------------
   | Blocking version of st_gethostbyaddr |
   ---------------------------------------- */
struct st_hostent * _cdecl
stbl_gethostbyaddr (struct st_gethostbyaddr_param p)
{
getfunc_lock (); /* Unlock should be done by the application */
return st_gethostbyaddr(p);
}

/* -----------------------
   | Get local host name |
   ----------------------- */
short _cdecl
st_gethostname (struct st_gethostname_param p)
{
	static long buf[2];
	
	PMSG pmsg;
	long r;
	
	DEBUG (("st_gethostname: enter\n"));
	
	libsocket_lock ();
	
	buf[0] = (long) p.name;
	buf[1] = p.namelen;
	
	pmsg.msg1 = MGW_GETHOSTNAME;
	pmsg.msg2 = (long) &buf[0];
	
	DEBUG (("st_gethostname: wait for Pmsg receive!\n"));
	r = Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
	DEBUG (("st_gethostname: Pmsg received = %li!\n", r));
	
	/* there is a still a race with the static data */
	libsocket_unlock ();
	
	if ((r == 0)
		&& (pmsg.msg1 == MGW_GETHOSTNAME))
	{
		r = pmsg.msg2;
	}
	
	if (!r)
		DEBUG (("st_gethostname: ok (%s)\n", p.name));
	else
		DEBUG (("st_gethostname: fail (%li, errno = %i)\n", r, errno));
	
	return r;
}

/* -----------------------
   | Get service by name |
   ----------------------- */
struct st_servent * _cdecl
st_getservbyname (struct st_getservbyname_param p)
{
	static long buf[2];
	
	PMSG pmsg;
	long r;
	
	DEBUG (("st_getservbyname: enter (%s, %s)\n", p.name, p.proto));
	
	libsocket_lock ();
	
	buf[0] = (long) p.name;
	buf[1] = (long) p.proto;
	
	pmsg.msg1 = MGW_GETSERVBYNAME;
	pmsg.msg2 = (long) &buf[0];
	
	DEBUG (("st_getservbyname: wait for Pmsg receive!\n"));
	r = Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
	DEBUG (("st_getservbyname: Pmsg received = %li!\n", r));
	
	/* there is a still a race with the static data */
	libsocket_unlock ();
	
	if ((r == 0)
		&& (pmsg.msg1 == MGW_GETSERVBYNAME)
		&& (pmsg.msg2 != 0))
	{
		static struct st_servent st_r;
		struct servent *host = (struct servent *) pmsg.msg2;
		
		DEBUG (("st_getservbyname: -> %s, %s\n", host->s_name, host->s_proto));
		
		/* struct st_servent
		 * {
		 * 	char *s_name;
		 * 	char **s_aliases;
		 * 	short s_port;
		 * 	char *s_proto;
		 * };
		 */
		
		st_r.s_name    = host->s_name;
		st_r.s_aliases = host->s_aliases;
		st_r.s_port    = host->s_port;
		st_r.s_proto   = host->s_proto;
		
		return &st_r;
	}
	
	DEBUG (("st_getservbyname: fail\n"));
	return NULL;
}

/* -----------------------
   | Get service by name |
   | (blocking version)  |
   ----------------------- */
struct st_servent * _cdecl
stbl_getservbyname (struct st_getservbyname_param p)
{
getfunc_lock (); /* Unlock should be done by the application */
return st_getservbyname(p);
}

/* -----------------------
   | Get service by port |
  ----------------------- */
struct st_servent * _cdecl
st_getservbyport (struct st_getservbyport_param p)
{
	static long buf[2];
	
	PMSG pmsg;
	long r;
	
	DEBUG (("st_getservbyport: enter (%i, %s)\n", p.port, p.proto));
	
	libsocket_lock ();
	
	buf[0] = p.port;
	buf[1] = (long) p.proto;
	
	pmsg.msg1 = MGW_GETSERVBYPORT;
	pmsg.msg2 = (long) &buf[0];
	
	DEBUG (("st_getservbyport: wait for Pmsg receive!\n"));
	r = Pmsg (2, MGW_LIBSOCKETCALL, &pmsg);
	DEBUG (("st_getservbyport: Pmsg received = %li!\n", r));
	
	/* there is a still a race with the static data */
	libsocket_unlock ();
	
	if ((r == 0)
		&& (pmsg.msg1 == MGW_GETSERVBYPORT)
		&& (pmsg.msg2 != 0))
	{
		static struct st_servent st_r;
		struct servent *host = (struct servent *) pmsg.msg2;
		
		DEBUG (("st_getservbyport: -> %s, %s\n", host->s_name, host->s_proto));
		
		/*
		 * struct st_servent
		 * {
		 * 	char *s_name;
		 * 	char **s_aliases;
		 * 	short s_port;
		 * 	char *s_proto;
		 * };
		 */
		
		st_r.s_name    = host->s_name;
		st_r.s_aliases = host->s_aliases;
		st_r.s_port    = host->s_port;
		st_r.s_proto   = host->s_proto;
		
		return &st_r;
	}
	
	DEBUG (("st_getservbyport: fail\n"));
	return NULL;
}

/* -----------------------
   | Get service by port |
  ----------------------- */
struct st_servent * _cdecl
stbl_getservbyport (struct st_getservbyport_param p)
{
getfunc_lock (); /* Unlock should be done by the application */
return st_getservbyport(p);
}
