/*
 * Filename:     gs_stik.c
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
# include <osbind.h>
# include <mintbind.h>
# include <netinet/in.h>

# include "gs_stik.h"

# include "gs_config.h"
# include "gs_func.h"
# include "gs_mem.h"
# include "version.h"


/* STIK global configuration structure.
 * 
 * STinG's <transprt.h> doesn't
 * define this, so we define it here.
 */

typedef struct config CONFIG;
struct config
{
	uint32	client_ip;	/* IP address of client (local) machine */
	uint32	provider;	/* IP address of provider, or 0L */
	uint16	ttl;		/* Default TTL for normal packets */
	uint16	ping_ttl;	/* Default TTL for 'ping'ing */
	uint16	mtu;		/* Default MTU (Maximum Transmission Unit) */
	uint16	mss;		/* Default MSS (Maximum Segment Size) */
	uint16	df_bufsize; 	/* Size of defragmentation buffer to use */
	uint16	rcv_window; 	/* TCP receive window */
	uint16	def_rtt;	/* Initial RTT time in ms */
	int16 	time_wait_time;	/* How long to wait in 'TIME_WAIT' state */
	int16 	unreach_resp;	/* Response to unreachable local ports */
	int32 	cn_time;	/* Time connection was made */
	int16 	cd_valid;	/* Is Modem CD a valid signal ?? */
	int16 	line_protocol;	/* What type of connection is this */
	void	(*old_vec)(void);	/* Old vector address */
	struct	slip *slp;	/* Slip structure for happiness */
	char	*cv[101];	/* Space for extra config variables */
	int16 	reports;	/* Problem reports printed to screen ?? */
	int16 	max_num_ports;	/* Maximum number of ports supported */
	uint32	received_data;	/* Counter for data being received */
	uint32	sent_data;	/* Counter for data being sent */
};

static char *err_list [E_LASTERROR + 1] =
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
/* E_BIGBUF       */	"Input buffer is too small for data"
};

char err_unknown [] = "Unrecognized error";

static int flags [64] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static void *
do_KRmalloc (int32 size)
{
	return gs_mem_alloc (size);
}

static void
do_KRfree (void *mem)
{
	gs_mem_free (mem);
}

static int32
do_KRgetfree (int16 flag)
{
	return gs_mem_getfree (flag);
}

static void *
do_KRrealloc (void *mem, int32 newsize)
{
	return gs_mem_realloc (mem, newsize);
}

char *
do_get_err_text (int16 code)
{
	if (code < 0)
		code = -code;
	
	if (code > 2000)
		return err_unknown;
	
	if (code > 1000)
	{
		/* Encoded GEMDOS errors */
		return strerror (code - 1000);
	}
	
	if (code > E_LASTERROR)
		return err_unknown;
	
	return err_list [code];
}

static char *
do_getvstr (char *var)
{
	return gs_getvstr (var);
}

/* Incompatibility:  Does nothing.
 * We can't really support this well,
 * since MiNTnet transparently supports multiple modems, as well as
 * non-modem methods of connections, such as local networks
 */
static int16
do_carrier_detect (void)
{
	return 0;
}

static int16
do_TCP_open (uint32 rhost, int16 rport, int16 tos, uint16 obsize)
{
	uint32 lhost; int16 lport;
	int fd;
	long ret;
	
	if (rhost == 0)
	{
		rhost = 0;
		lhost = INADDR_ANY;
		lport = rport;
		rport = 0;
	}
	else if (rport == 0)
	{
		CIB *cib = (CIB *) rhost;
		
		rhost = cib->rhost;
		rport = cib->rport;
		lhost = cib->lhost;
		lport = cib->lport;
	}
	else
	{
		rhost = rhost;
		rport = rport;
		lhost = INADDR_ANY;
		lport = 0;
	}
	
	fd = gs_open ();
	if (fd < 0)
		return fd;
	
	/* The TCP_OPEN_CMD command transmogrifies this descriptor into an
	 * actual connection.
	 */
	ret = gs_connect (fd, rhost, rport, lhost, lport);
	if (ret < 0)
		return ret;
	
	return fd;
}

static int16
do_TCP_close (int16 fd, int16 timeout)
{
	gs_close (fd);
	return 0;
}

static int16
do_TCP_send (int16 fd, void *buf, int16 len)
{
	return gs_write (fd, buf, len);
}

static int16
do_TCP_wait_state (int16 fd, int16 state, int16 timeout)
{
	return gs_wait (fd, timeout);
}

/* Incompatibility:  Does nothing.
 * MiNTnet handles this internally.
 */
static int16
do_TCP_ack_wait (int16 fd, int16 timeout)
{
	return E_NORMAL;
}

static int16
do_UDP_open (uint32 rhost, int16 rport)
{
	int fd;
	int ret;
	
	fd = gs_open ();
	if (fd < 0)
		return fd;
	
	/* The UDP_OPEN_CMD command transmogrifies this descriptor into an
	 * actual connection.
	 */
	ret = gs_udp_open (fd, rhost, rport);
	if (ret < 0)
		return ret;
	
	return fd;
}

static int16
do_UDP_close (int16 fd)
{
	gs_close (fd);
	return 0;
}

static int16
do_UDP_send (int16 fd, void *buf, int16 len)
{
	return gs_write (fd, buf, len);
}

/* Incompatibility:  Does nothing.
 * MiNTnet handles its own "kicking"
 */
static int16
do_CNkick (int16 fd)
{
	return E_NORMAL;
}

static int16
do_CNbyte_count (int16 fd)
{
	return gs_canread (fd);
}

static int16
do_CNget_char (int16 fd)
{
	char c;
	long ret;
	
	ret = gs_read (fd, &c, 1L);
	if (ret < 0)
		return ret;
	
	if (ret == 0)
		return E_NODATA;
	
	return c;
}

static NDB *
do_CNget_NDB (int16 fd)
{
	return gs_readndb (fd);
}

static int16
do_CNget_block (int16 fd, void *buf, int16 len)
{
	return gs_read (fd, buf, len);
}

static void
do_housekeep (void)
{
	/* does nothing */
}

static int16
do_resolve (char *dn, char **rdn, uint32 *alist, int16 lsize)
{
	return gs_resolve (dn, rdn, alist, lsize);
}

static void
do_ser_disable (void)
{
	/* does nothing */
}

static void
do_ser_enable (void)
{
	/* does nothing */
}

static int16
do_set_flag (int16 flag)
{
	int flg_val;
	
	if (flag < 0 || flag >= 64)
		return E_PARAMETER;
	
	/* This is probably not necessary, since a MiNT process currently
	 * can't be preempted in supervisor mode, but I'm not taking any
	 * chances...
	 */
	Psemaphore (2, FLG_SEM, -1);
	flg_val = flags [flag];
	flags [flag] = 1;
	Psemaphore (3, FLG_SEM, 0);
	
	return flg_val;
}

static void
do_clear_flag (int16 flag)
{
	if (flag < 0 || flag >= 64)
		return;
	
	/* This is probably not necessary, since a MiNT process currently
	 * can't be preempted in supervisor mode, but I'm not taking any
	 * chances...
	 */
	Psemaphore (2, FLG_SEM, -1);
	flags [flag] = 0;
	Psemaphore (3, FLG_SEM, 0);
}

static CIB *
do_CNgetinfo (int16 fd)
{
	GS *gs = gs_get (fd);
	
	if (!gs)
		return (CIB *) E_BADHANDLE;
	
	return &(gs->cib);
}

/* Incompatibility: None of the *_port() commands do anything.
 * Don't use a STiK dialer with GlueSTiK, gods know what will happen!
 */
static int16
do_on_port (char *port)
{
	return E_NOROUTINE;
}

static void
do_off_port (char *port)
{
}

static int16
do_setvstr (char *vs, char *value)
{
	return gs_setvstr (vs, value);
}

static int16
do_query_port (char *port)
{
	return E_NOROUTINE;
}

static int16
do_CNgets (int16 fd, char *buf, int16 len, char delim)
{
	return gs_read_delim (fd, buf, len, delim);
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
	NULL /* (int16 (*)(uint32, uint8, uint8, void *, uint16)) */,
	NULL /* (int16 (*)(int16 (*)(IP_DGRAM *), int16)) */,
	NULL /* (void (*)(IP_DGRAM *)) */
};

static DRV_HDR *
do_get_dftab (char *tpl_name)
{
	/* we only have the one, so this is pretty easy... ;)
	 */
	if (strcmp (tpl_name, trampoline.module) != 0)
		return 0;
	
	return (DRV_HDR *) &trampoline;
}

static int16
do_ETM_exec (char *tpl_name)
{
	/* even easier... ;) */
	return 0;
}

static CONFIG stik_cfg;
DRV_LIST stik_driver =
{
	MAGIC,
	do_get_dftab,
	do_ETM_exec,
	&stik_cfg,
	NULL
};

int
init_stik_if (void)
{
	if (Psemaphore (0, FLG_SEM, 0) < 0)
	{
		Cconws ("Unable to obtain STiK flag semaphore\r\n");
		return 0;
	}
	
	stik_cfg.client_ip = INADDR_LOOPBACK;
	return 1;
}

void
cleanup_stik_if (void)
{
	Psemaphore (1, FLG_SEM, 0);
}
