/*
 * Filename:     gs_stik.c
 * Project:      GlueSTiK
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann <fnaumann@freemint.de>
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
# include <osbind.h>
# include <mintbind.h>
# include <netinet/in.h>

# include "gs_stik.h"

# include "gs_conf.h"
# include "gs_func.h"
# include "gs_mem.h"
# include "version.h"


static const char *const err_list [E_LASTERROR + 1] =
{
/* E_NORMAL       */	"No error occured",
/* E_OBUFFULL     */	"Output buffer is full",
/* E_NODATA       */	"No data available",
/* E_EOF          */	"EOF from remote",
/* E_RRESET       */	"Reset received from remote",
/* E_UA           */	"Unacceptable packet received, reset",
/* E_NOMEM        */	"Something failed due to lack of memory",
/* E_REFUSE       */	"Connection refused by remote",
/* E_BADSYN       */	"A SYN was received in the window",
/* E_BADHANDLE    */	"Bad connection handle used.",
/* E_LISTEN       */	"The connection is in LISTEN state",
/* E_NOCCB        */	"No free CCB's available",
/* E_NOCONNECTION */	"No connection matches this packet (TCP)",
/* E_CONNECTFAIL  */	"Failure to connect to remote port (TCP)",
/* E_BADCLOSE     */	"Invalid TCP_close() requested",
/* E_USERTIMEOUT  */	"A user function timed out",
/* E_CNTIMEOUT    */	"A connection timed out",
/* E_CANTRESOLVE  */	"Can't resolve the hostname",
/* E_BADDNAME     */	"Domain name or dotted dec. bad format",
/* E_LOSTCARRIER  */	"The modem disconnected",
/* E_NOHOSTNAME   */	"Hostname does not exist",
/* E_DNSWORKLIMIT */	"Resolver Work limit reached",
/* E_NONAMESERVER */	"No nameservers could be found for query",
/* E_DNSBADFORMAT */	"Bad format of DS query",
/* E_UNREACHABLE  */	"Destination unreachable",
/* E_DNSNOADDR    */	"No address records exist for host",
/* E_NOROUTINE    */	"Routine unavailable",
/* E_LOCKED       */	"Locked by another application",
/* E_FRAGMENT     */	"Error during fragmentation",
/* E_TTLEXCEED    */	"Time To Live of an IP packet exceeded",
/* E_PARAMETER    */	"Problem with a parameter",
/* E_BIGBUF       */	"Input buffer is too small for data",
/* E_FNAVAIL      */	"Function is not available"
};

static char const err_unknown [] = "Unrecognized error";

static int flags [64] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


#if TPL_STRUCT_ARGS
#define P(x) p.x
#else
#define P(x) x
#endif

#define UNUSED(x) ((void)(x))


#if TPL_STRUCT_ARGS
static void * __CDECL do_KRmalloc (struct KRmalloc_param p)
#else
static void * __CDECL do_KRmalloc (int32 size)
#endif
{
	return gs_mem_alloc (P(size));
}

#if TPL_STRUCT_ARGS
static void __CDECL do_KRfree (struct KRfree_param p)
#else
static void __CDECL do_KRfree (void *mem)
#endif
{
	gs_mem_free (P(mem));
}

#if TPL_STRUCT_ARGS
static int32 __CDECL do_KRgetfree (struct KRgetfree_param p)
#else
static int32 __CDECL do_KRgetfree (int16 flag)
#endif
{
	return gs_mem_getfree (P(flag));
}

#if TPL_STRUCT_ARGS
static void *__CDECL do_KRrealloc (struct KRrealloc_param p)
#else
static void *__CDECL do_KRrealloc (void *mem, int32 newsize)
#endif
{
	return gs_mem_realloc (P(mem), P(newsize));
}


#if TPL_STRUCT_ARGS
const char *__CDECL do_get_err_text (struct get_err_text_param p)
#else
const char *__CDECL do_get_err_text (int16 code)
#endif
{
#if TPL_STRUCT_ARGS
	int16 code = P(code);
#endif

	if (code < 0)
		code = -code;
	
	if (code > 2000)
		return err_unknown;
	
	if (code > 1000)
	{
		/* Encoded GEMDOS errors */
		return strerror (code - 1000);
	}
	
	if (code > E_LASTERROR || err_list [code] == 0)
		return err_unknown;
	
	return err_list [code];
}

#if TPL_STRUCT_ARGS
static const char *__CDECL do_getvstr (struct getvstr_param p)
#else
static const char *__CDECL do_getvstr (const char *var)
#endif
{
	return gs_getvstr (P(var));
}

/* Incompatibility:  Does nothing.
 * We can't really support this well,
 * since MiNTnet transparently supports multiple modems, as well as
 * non-modem methods of connections, such as local networks
 */
static int16 __CDECL
do_carrier_detect (void)
{
	return 0;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_open (struct TCP_open_param p)
#else
static int16 __CDECL do_TCP_open (uint32 rhost, uint16 rport, uint16 tos, uint16 obsize)
#endif
{
	uint32 lhost; int16 lport;
	int fd;
	long ret;
	
	UNUSED(P(tos));
	UNUSED(P(obsize));
	DEBUG (("do_TCP_open: rhost = %d.%d.%d.%d, rport = %i", DEBUG_ADDR(P(rhost)), P(rport)));
	
	if (P(rhost) == 0)
	{
		/*
		 * STiK-compatible, passive connection;
		 * 2nd parameter (rport) is used as local port,
		 * connection from any port/host will be accepted
		 */
		P(rhost) = 0;
		lhost = INADDR_ANY;
		lport = P(rport);
		P(rport) = 0;
	}
	else if (P(rport) == TCP_ACTIVE || P(rport) == TCP_PASSIVE)
	{
		CAB *cab = (CAB *) P(rhost);
		
		/*
		 * STinG: rhost gives all parameters
		 */
		P(rhost) = cab->rhost;
		P(rport) = cab->rport;
		lhost = cab->lhost;
		lport = cab->lport;
	}
	else
	{
		/* STiK-compatible, active connection */
		lhost = INADDR_ANY;
		lport = 0;
	}
	
	fd = gs_open ();
	if (fd < 0)
		return fd;
	
	/* The TCP_OPEN_CMD command transmogrifies this descriptor into an
	 * actual connection.
	 */
	ret = gs_connect (fd, P(rhost), P(rport), lhost, lport);
	if (ret < 0)
	{
		gs_close(fd);
		return ret;
	}

	DEBUG (("do_TCP_open: fd = %i", fd));
	return fd;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_close (struct TCP_close_param p)
#else
static int16 __CDECL do_TCP_close (int16 fd, int16 timeout, int16 *result)
#endif
{
	UNUSED(P(timeout));
	UNUSED(P(result));
	gs_close (P(fd));
	return E_NORMAL;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_send (struct TCP_send_param p)
#else
static int16 __CDECL do_TCP_send (int16 fd, const void *buf, int16 len)
#endif
{
	return gs_write (P(fd), P(buf), P(len));
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_wait_state (struct TCP_wait_state_param p)
#else
static int16 __CDECL do_TCP_wait_state (int16 fd, int16 state, int16 timeout)
#endif
{
	UNUSED(P(state));
	return gs_wait (P(fd), P(timeout));
}

/* Incompatibility:  Does nothing.
 * MiNTnet handles this internally.
 */
#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_ack_wait (struct TCP_ack_wait_param p)
#else
static int16 __CDECL do_TCP_ack_wait (int16 fd, int16 timeout)
#endif
{
	UNUSED(P(fd));
	UNUSED(P(timeout));
	return E_NORMAL;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_UDP_open (struct UDP_open_param p)
#else
static int16 __CDECL do_UDP_open (uint32 rhost, uint16 rport)
#endif
{
	int fd;
	int ret;
	
	fd = gs_open ();
	if (fd < 0)
		return fd;
	
	/* The UDP_OPEN_CMD command transmogrifies this descriptor into an
	 * actual connection.
	 */
	ret = (int)gs_udp_open (fd, P(rhost), P(rport));
	if (ret < 0)
	{
		gs_close(fd);
		return ret;
	}

	return fd;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_UDP_close (struct UDP_close_param p)
#else
static int16 __CDECL do_UDP_close (int16 fd)
#endif
{
	gs_close (P(fd));
	return 0;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_UDP_send (struct UDP_send_param p)
#else
static int16 __CDECL do_UDP_send (int16 fd, const void *buf, int16 len)
#endif
{
	return gs_write (P(fd), P(buf), P(len));
}

/* Incompatibility:  Does nothing.
 * MiNTnet handles its own "kicking"
 */
#if TPL_STRUCT_ARGS
static int16 __CDECL do_CNkick (struct CNkick_param p)
#else
static int16 __CDECL do_CNkick (int16 fd)
#endif
{
	UNUSED(P(fd));
	return E_NORMAL;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_CNbyte_count (struct CNbyte_count_param p)
#else
static int16 __CDECL do_CNbyte_count (int16 fd)
#endif
{
	long n = gs_canread (P(fd));
	/*
	 * limit the return value to a signed 16bit value,
	 * to avoid it being misinterpreted as error.
	 */
	if (n <= 32767L)
		return n;
	return 32767;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_CNget_char (struct CNget_char_param p)
#else
static int16 __CDECL do_CNget_char (int16 fd)
#endif
{
	char c;
	long ret;
	
	ret = gs_read (P(fd), &c, 1);
	if (ret < 0)
		return ret;
	
	if (ret == 0)
		return E_NODATA;
	
	return (unsigned char)c;
}

#if TPL_STRUCT_ARGS
static NDB * __CDECL do_CNget_NDB (struct CNget_NDB_param p)
#else
static NDB * __CDECL do_CNget_NDB (int16 fd)
#endif
{
	return gs_readndb (P(fd));
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_CNget_block (struct CNget_block_param p)
#else
static int16 __CDECL do_CNget_block (int16 fd, void *buf, int16 len)
#endif
{
	return gs_read (P(fd), P(buf), P(len));
}

static void __CDECL do_housekeep (void)
{
	/* does nothing */
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_resolve (struct resolve_param p)
#else
static int16 __CDECL do_resolve (const char *dn, char **rdn, uint32 *alist, int16 lsize)
#endif
{
	return gs_resolve (P(dn), P(rdn), P(alist), P(lsize));
}

static void __CDECL do_ser_disable (void)
{
	/* does nothing */
}

static void __CDECL do_ser_enable (void)
{
	/* does nothing */
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_set_flag (struct set_flag_param p)
#else
static int16 __CDECL do_set_flag (int16 flag)
#endif
{
	int flg_val;
	
	if (P(flag) < 0 || P(flag) >= 64)
		return E_PARAMETER;
	
	/* This is probably not necessary, since a MiNT process currently
	 * can't be preempted in supervisor mode, but I'm not taking any
	 * chances...
	 */
	Psemaphore (2, FLG_SEM, -1);
	flg_val = flags [P(flag)];
	flags [P(flag)] = 1;
	Psemaphore (3, FLG_SEM, 0);
	
	return flg_val;
}

#if TPL_STRUCT_ARGS
static void __CDECL do_clear_flag (struct clear_flag_param p)
#else
static void __CDECL do_clear_flag (int16 flag)
#endif
{
	if (P(flag) < 0 || P(flag) >= 64)
		return;
	
	/* This is probably not necessary, since a MiNT process currently
	 * can't be preempted in supervisor mode, but I'm not taking any
	 * chances...
	 */
	Psemaphore (2, FLG_SEM, -1);
	flags [P(flag)] = 0;
	Psemaphore (3, FLG_SEM, 0);
}

#if TPL_STRUCT_ARGS
static CIB * __CDECL do_CNgetinfo (struct CNgetinfo_param p)
#else
static CIB * __CDECL do_CNgetinfo (int16 fd)
#endif
{
	GS *gs = gs_get (P(fd));
	
	if (!gs)
		return (CIB *) E_BADHANDLE;
	
	return &(gs->cib);
}

/* Incompatibility: None of the *_port() commands do anything.
 * Don't use a STiK dialer with GlueSTiK, gods know what will happen!
 */
#if TPL_STRUCT_ARGS
static int16 __CDECL do_on_port (struct on_port_param p)
#else
static int16 __CDECL do_on_port (const char *port)
#endif
{
	UNUSED(P(port));
	return E_NOROUTINE;
}

#if TPL_STRUCT_ARGS
static void __CDECL do_off_port (struct off_port_param p)
#else
static void __CDECL do_off_port (const char *port)
#endif
{
	UNUSED(P(port));
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_setvstr (struct setvstr_param p)
#else
static int16 __CDECL do_setvstr (const char *vs, const char *value)
#endif
{
	return gs_setvstr (P(vs), P(value));
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_query_port (struct query_port_param p)
#else
static int16 __CDECL do_query_port (const char *port)
#endif
{
	UNUSED(P(port));
	return E_NOROUTINE;
}

#if TPL_STRUCT_ARGS
static int16 __CDECL do_CNgets (struct CNgets_param p)
#else
static int16 __CDECL do_CNgets (int16 fd, char *buf, int16 len, char delim)
#endif
{
	return gs_read_delim (P(fd), P(buf), P(len), P(delim));
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_ICMP_send(struct ICMP_send_param p)
#else
static int16 __CDECL do_ICMP_send(uint32 dest_host, uint8 type, uint8 code, const void *data, uint16 length)
#endif
{
	UNUSED(P(dest_host));
	UNUSED(P(type));
	UNUSED(P(code));
	UNUSED(P(data));
	UNUSED(P(length));
	return E_NOROUTINE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_ICMP_handler(struct ICMP_handler_param p)
#else
static int16 __CDECL do_ICMP_handler(int16 cdecl (*handler) (IP_DGRAM *), int16 install_code)
#endif
{
	UNUSED(P(handler));
	UNUSED(P(install_code));
	return FALSE;
}


#if TPL_STRUCT_ARGS
static void __CDECL do_ICMP_discard(struct ICMP_discard_param p)
#else
static void __CDECL do_ICMP_discard(IP_DGRAM *datagram)
#endif
{
	UNUSED(P(datagram));
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_TCP_info(struct TCP_info_param p)
#else
static int16 __CDECL do_TCP_info(int16 handle, TCPIB *buffer)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(buffer));
	return E_BADHANDLE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_cntrl_port(struct cntrl_port_param p)
#else
static int16 __CDECL do_cntrl_port(const char *name, uint32 arg, int16 code)
#endif
{
	UNUSED(P(name));
	UNUSED(P(arg));
	UNUSED(P(code));
	return E_NODATA;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_UDP_info(struct UDP_info_param p)
#else
static int16 __CDECL do_UDP_info(int16 handle, UDPIB *buffer)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(buffer));
	return E_BADHANDLE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_RAW_open(struct RAW_open_param p)
#else
static int16 __CDECL do_RAW_open(uint32 rhost)
#endif
{
	UNUSED(P(rhost));
	return E_NOROUTINE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_RAW_close(struct RAW_close_param p)
#else
static int16 __CDECL do_RAW_close(int16 handle)
#endif
{
	UNUSED(P(handle));
	return E_BADHANDLE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_RAW_out(struct RAW_out_param p)
#else
static int16 __CDECL do_RAW_out(int16 handle, const void *data, int16 dlen, uint32 dest_ip)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(data));
	UNUSED(P(dlen));
	UNUSED(P(dest_ip));
	return E_BADHANDLE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_CN_setopt(struct CN_setopt_param p)
#else
static int16 __CDECL do_CN_setopt(int16 handle, int16 opt_id, const void *optval, int16 optlen)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(opt_id));
	UNUSED(P(optval));
	UNUSED(P(optlen));
	return E_NOROUTINE;
}


#if TPL_STRUCT_ARGS
static int16 __CDECL do_CN_getopt(struct CN_getopt_param p)
#else
static int16 __CDECL do_CN_getopt(int16 handle, int16 opt_id, void *optval, int16 *optlen)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(opt_id));
	UNUSED(P(optval));
	UNUSED(P(optlen));
	return E_NOROUTINE;
}


#if TPL_STRUCT_ARGS
static void __CDECL do_CNfree_NDB(struct CNfree_NDB_param p)
#else
static void __CDECL do_CNfree_NDB(int16 handle, NDB *block)
#endif
{
	UNUSED(P(handle));
	UNUSED(P(block));
}


static long noop(void)
{
	return E_NOROUTINE;
}


static TPL trampoline =
{
	TRANSPORT_DRIVER,
	"Scott Bigham, Frank Naumann (GlueSTiK\277 v" str (VER_MAJOR) "." str (VER_MINOR) ")",
	"01.13",
	do_KRmalloc,
	do_KRfree,
	do_KRgetfree,
	do_KRrealloc,
	do_get_err_text,
	do_getvstr,
	do_carrier_detect,
	do_TCP_open,
	do_TCP_close,
	do_TCP_send,
	do_TCP_wait_state,
	do_TCP_ack_wait,
	do_UDP_open,
	do_UDP_close,
	do_UDP_send,
	do_CNkick,
	do_CNbyte_count,
	do_CNget_char,
	do_CNget_NDB,
	do_CNget_block,
	do_housekeep,
	do_resolve,
	do_ser_disable,
	do_ser_enable,
	do_set_flag,
	do_clear_flag,
	do_CNgetinfo,
	do_on_port,
	do_off_port,
	do_setvstr,
	do_query_port,
	do_CNgets,
	do_ICMP_send,
	do_ICMP_handler,
	do_ICMP_discard,
	do_TCP_info,
	do_cntrl_port,
	do_UDP_info,
	do_RAW_open,
	do_RAW_close,
	do_RAW_out,
	do_CN_setopt,
	do_CN_getopt,
	do_CNfree_NDB,
	noop,
	noop,
	noop,
	noop
};

static DRV_HDR *__CDECL
do_get_dftab (const char *tpl_name)
{
	/* we only have the one, so this is pretty easy... ;)
	 */
	if (strcmp (tpl_name, trampoline.module) != 0)
		return 0;
	
	return (DRV_HDR *) &trampoline;
}

static int16 __CDECL
do_ETM_exec (const char *tpl_name)
{
	UNUSED(tpl_name);
	/* even easier... ;) */
	return 0;
}

static STING_CONFIG sting_cfg;
DRV_LIST stik_driver =
{
	STIK_DRVR_MAGIC,
	do_get_dftab,
	do_ETM_exec,
	{ &sting_cfg },
	NULL
};

int
init_stik_if (void)
{
	if (Psemaphore (0, FLG_SEM, 0) < 0)
	{
		(void) Cconws ("Unable to obtain STiK flag semaphore\r\n");
		return FALSE;
	}
	
	sting_cfg.client_ip = INADDR_LOOPBACK;
	return TRUE;
}

void
cleanup_stik_if (void)
{
	Psemaphore (1, FLG_SEM, 0);
}
