/*
 * Filename:
 * Version:
 * Author:       Frank Naumann, Jens Heitmann
 * Started:      1999-05
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


# if 1
# define DEBUG(x)
# else
# define DEBUG(x) printf x
# endif


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
 * prototypes
 */
struct st_hostent * _cdecl st_gethostbyname (struct st_gethostbyname_param p);
struct st_hostent * _cdecl st_gethostbyaddr (struct st_gethostbyaddr_param p);
short               _cdecl st_gethostname   (struct st_gethostname_param p);
struct st_servent * _cdecl st_getservbyname (struct st_getservbyname_param p);
struct st_servent * _cdecl st_getservbyport (struct st_getservbyport_param p);


/* --------------------
   | Get host by name |
   -------------------- */
struct st_hostent * _cdecl
st_gethostbyname (struct st_gethostbyname_param p)
{
	static char buf [4096];
	
	struct hostent *host = NULL;
	PMSG pmsg;
	long r;
	
	strcpy (buf, p.name);
	pmsg.msg1 = (long) buf;
	pmsg.msg2 = 0;
	
	DEBUG (("Wait for Pmsg receive on [%s]!\n", buf));
	r = Pmsg (2, MGW_GETHOSTBYNAME, &pmsg);
	DEBUG (("Pmsg received = %li, %lx!\n", r, pmsg.msg2));
	
	if (r == 0)
	{
		static struct st_hostent r_st;
		
		host = (struct hostent *) pmsg.msg2;
		
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

/* --------------------
   | Get host by addr |
   -------------------- */
struct st_hostent * _cdecl
st_gethostbyaddr (struct st_gethostbyaddr_param p)
{
	struct hostent *r;
	
	DEBUG (("st_gethostbyaddr: enter (%s)\n", p.haddr));
	
	r = gethostbyaddr (p.haddr, p.len, p.type);
	if (r)
	{
		static struct st_hostent r_st;
		
		DEBUG (("st_gethostbyaddr: ok (%s)\n", r->h_name));
		
		/* struct st_hostent
		 * {
		 * 	char *h_name;
		 * 	char **h_aliases;
		 * 	short h_addrtype;
		 * 	short h_length;
		 * 	char **h_addr_list;
		 * };
		 */
		
		r_st.h_name = r->h_name;
		r_st.h_aliases = r->h_aliases;
		r_st.h_addrtype = r->h_addrtype;
		r_st.h_length = r->h_length;
		r_st.h_addr_list = r->h_addr_list;
		
		return &r_st;
	}
	
	DEBUG (("st_gethostbyaddr: fail!\n"));
	return NULL;
}

/* -----------------------
   | Get local host name |
   ----------------------- */
short _cdecl
st_gethostname (struct st_gethostname_param p)
{
	short r;
	
	DEBUG (("st_gethostname: enter\n"));
	
	r = gethostname (p.name, p.namelen);
	if (!r)
		DEBUG (("st_gethostname: ok (%s)\n", p.name));
	else
		DEBUG (("st_gethostname: fail (%i, errno = %i)\n", r, errno));
	
	return r;
}

/* -----------------------
   | Get service by name |
   ----------------------- */
struct st_servent * _cdecl
st_getservbyname (struct st_getservbyname_param p)
{
	struct servent *r;
	
	DEBUG (("st_getservbyname: enter (%s, %s)\n", p.name, p.proto));
	
	r = getservbyname (p.name, p.proto);
	if (r)
	{
		static struct st_servent st_r;
		
		DEBUG (("st_getservbyname: -> %s, %s\n", r->s_name, r->s_proto));
		
		/* struct st_servent
		 * {
		 * 	char *s_name;
		 * 	char **s_aliases;
		 * 	short s_port;
		 * 	char *s_proto;
		 * };
		 */
		
		st_r.s_name    = r->s_name;
		st_r.s_aliases = r->s_aliases;
		st_r.s_port    = r->s_port;
		st_r.s_proto   = r->s_proto;
		
		return &st_r;
	}
	
	DEBUG (("st_getservbyname: fail\n"));
	return NULL;
}

/* -----------------------
   | Get service by port |
  ----------------------- */
struct st_servent * _cdecl
st_getservbyport (struct st_getservbyport_param p)
{
	struct servent *r;
	
	DEBUG (("st_getservbyport: enter (%i, %s)\n", p.port, p.proto));
	
	r = getservbyport (p.port, p.proto);
	if (r)
	{
		static struct st_servent st_r;
		
		DEBUG (("st_getservbyport: -> %s, %s\n", r->s_name, r->s_proto));
		
		/*
		 * struct st_servent
		 * {
		 * 	char *s_name;
		 * 	char **s_aliases;
		 * 	short s_port;
		 * 	char *s_proto;
		 * };
		 */
		
		st_r.s_name    = r->s_name;
		st_r.s_aliases = r->s_aliases;
		st_r.s_port    = r->s_port;
		st_r.s_proto   = r->s_proto;
		
		return &st_r;
	}
	
	DEBUG (("st_getservbyport: fail\n"));
	return NULL;
}
