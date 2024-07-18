/*
 * Daynaport SCSI/Link driver for FreeMiNT.
 * GNU C conversion by Miro Kropacek, <miro.kropacek@gmail.com>
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Roger Burrows.
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
 */

/*
 *	MiNTnet driver
 *	for Dayna Communications' DaynaPORT SCSI/Link (SCSI-to-ethernet adapter)
 *
 *
 *	Key features of driver
 *	----------------------
 *	1. Supports up to 16 devices
 *	2. Allows retrieval of internal statistics
 *
 *
 *	Version history
 *	---------------
 *	version 0.10	february/2007 (roger burrows, anodyne software)
 *	  .	based on v0.52 of defunct MagxNet driver
 *
 *	version 0.15	february/2007 (roger burrows, anodyne software)
 *	  .	the i/o wrapper method does not work with the current MiNT system,
 *		so change to use addroottimeout(), thereby avoiding doing I/O out
 *		of an interrupt handler.
 *		as a precaution, we avoid actual i/o for now ...
 *
 *	version 0.20	february/2007 (roger burrows, anodyne software)
 *	  .	enable open, close, ioctl (no i/o);input, output are still disabled ...
 *
 *	version 0.25	february/2007 (roger burrows, anodyne software)
 *	  .	enable ioctl i/o & output;input is still disabled ...
 *
 *	version 0.30	february/2007 (roger burrows, anodyne software)
 *	  .	enable input ...
 *
 *	version 0.34	february/2007 (roger burrows, anodyne software)
 *	  .	enable mode switching in SCSIDRV library
 *
 *	version 0.35	february/2007 (roger burrows, anodyne software)
 *	  .	add trace
 *
 *	version 0.40	march/2007 (roger burrows, anodyne software)
 *	  .	add code to bypass ftpd wart: ftpd runs under a non-root euid while
 *		issuing login messages, causing calls to scsidrv to fail with EPERM
 *
 *	version 0.50	march/2007 (roger burrows, anodyne software)
 *	  .	rework trace to allow it to be started dynamically (via SLINKCTL)
 *	  .	set maxpackets to zero 
 *
 *	version 0.60	november/2007 (roger burrows, anodyne software)
 *	  .	add ioctl to return MAC address of device
 *	  .	update trace recording
 *	  .	relink with updated SCSILINK.LIB to fix ARP problem with Dayna
 *		firmware 1.4a - broadcast mode has to be enabled explicitly
 *
 *
 *	future: consider removing option -y and adding __saveds to:
 *				driver_init(), sl_open(), sl_close(), sl_output(), sl_ioctl(), interrupt_handler()
 */

#include <osbind.h>
#include <macros.h>

#include "libkern/libkern.h"
#include "arch/timer.h"
#include "cookie.h"
#include "mint/ssystem.h"

#include "buf.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"
#include "netinfo.h"

#include "mint/sockio.h"
#include "errno.h"
#include "config.h"

#include "scsilink_func.h"
#include "scsilink.h"


#define MAJOR_VERSION	0
#define MINOR_VERSION	60

#define DRIVER_DESC		"DaynaPORT SCSI/Link driver"
#define DRIVER			"SCSILINK.XIF"

#define ETHERNET_NAME	"en"

typedef volatile char FASTLOCK;

struct sl_device {
	char interface_up;
	FASTLOCK active;
	short devnum;
	struct scsilink_counts count;
	long trace_entries;					/* total number of entries */
	long current_entry;					/* current entry number */
	SLINK_TRACE *trace_table;			/* trace table */
};


/*
 *	globals
 */
static long num_devices = 0L;			/* number successfully opened */

static struct netif *niftab[MAX_SCSILINK_DEVICES];

static SCSI_PACKET scsiPacket;			/* buffer for input packets */


/*
 *	locking stuff
 *
 *		int set_lock(FASTLOCK *lockptr)
 *			attempts to set the 1-byte lock at 'lockptr'.
 *			returns 0 if error (lock already set), else non-zero
 *
 *		int release_lock(FASTLOCK *lockptr)
 *			attempts to release the 1-byte lock at 'lockptr'.
 *			returns 0 if error (lock not set), else non-zero
 *
 *		int test_lock(FASTLOCK *lockptr)
 *			checks the 1-byte lock at 'lockptr'.
 *			returns 0 if lock not set, else non-zero
 */

int set_lock(FASTLOCK *lockptr);
int release_lock(FASTLOCK *lockptr);
int test_lock(FASTLOCK *lockptr);

#define trace(dev,type,rc,len,data)	if ((dev)->trace_entries) add_trace_entry(dev,type,rc,len,data)

/*
 *	function prototypes
 */
static long	_cdecl sl_open(struct netif *);
static long	_cdecl sl_close(struct netif *);
static long _cdecl sl_output(struct netif *,BUF *,const char *,short,short);
static long	_cdecl sl_ioctl(struct netif *,short,long);

static void add_trace_entry(struct sl_device *dev,char type,long rc,short length,char *data);
static void get_waiting_packets(struct netif *nif);
static void handle_all_input(PROC *proc, long arg);
static void handle_input(struct netif *nif);
static long queue_output(struct netif *nif,BUF *nbuf);
static long read_packet(struct sl_device *dev,char *packet);
static int receive_packet(struct netif *nif);
static long write_packet(struct sl_device *dev,BUF *nbuf,int len);

void install_interrupts(void);		/* in lowlevel.s */
void interrupt_handler(void);		/* referenced by lowlevel.s */

long driver_init(void);

/********************************************************
*														*
*	I N I T I A L I S A T I O N   R O U T I N E S		*
*														*
********************************************************/

long driver_init(void)
{
char message[200], macaddr[ETH_ALEN];
struct netif *nif;
struct sl_device *dev;
long n, rc;
int i, bus, id;

	ksprintf(message,"%s v%d.%02d initialising ...\r\n",DRIVER_DESC,MAJOR_VERSION,MINOR_VERSION);
	c_conws(message);

	/*
	 *	validate system
	 */
	if (s_system (S_GETCOOKIE, COOKIE_MiNT, 0L) == 1) {
		ksprintf(message,"%s: must run under MiNT\r\n",DRIVER);
		c_conws(message);
		return 1L;
	}

	if (scsilink_modeswitch(FALSE) < 0L) {
		ksprintf(message,"%s: can't disable mode switch\r\n",DRIVER);
		c_conws(message);
		return 1L;
	}

	rc = scsilink_probe();					/* look for devices */
	if (rc <= 0L) {
		if (rc < 0L)
			ksprintf(message,"%s: SCSIDRV not available\r\n",DRIVER);
		else ksprintf(message,"%s: no SCSI/Link devices detected\r\n",DRIVER);
		c_conws(message);
		return 1L;
	}
	num_devices = rc;

	for (i = 0, n = 0; (i < num_devices) && (n < MAX_SCSILINK_DEVICES); i++) {
		if (scsilink_open(i,macaddr) < 0L) {
			ksprintf(message,"%s: can't get MAC address for device %d\r\n",DRIVER,i);
			c_conws(message);
			continue;
		}

		nif = kmalloc(sizeof(struct netif));
		dev = kmalloc(sizeof(struct sl_device));
		if (!nif || !dev) {
			ksprintf(message,"%s: insufficient memory for device %d\r\n",DRIVER,i);
			c_conws(message);
			break;
		} 

		memset(nif,0,sizeof(struct netif));			/* must do this! */
		memset(dev,0,sizeof(struct sl_device));
		strcpy(nif->name,ETHERNET_NAME);
		nif->unit = if_getfreeunit((char *)ETHERNET_NAME);
		nif->flags = IFF_BROADCAST;
		nif->mtu = ETH_MAX_DLEN;
		nif->hwtype = HWTYPE_ETH;
		nif->hwlocal.len = ETH_ALEN;
		memcpy(nif->hwlocal.adr.bytes,macaddr,ETH_ALEN);
		nif->hwbrcst.len = ETH_ALEN;
		memset(nif->hwbrcst.adr.bytes,0xff,ETH_ALEN);
		nif->snd.maxqlen = IF_MAXQ;
		nif->rcv.maxqlen = IF_MAXQ;

		nif->open = sl_open;
		nif->close = sl_close;
		nif->output = sl_output;
		nif->ioctl = sl_ioctl;

		nif->data = dev;

		/*
		 * Tell upper layers the max. number of packets we are able to
		 * receive in fast succession.
		 */
		nif->maxpackets = 0;
			
		niftab[n++] = nif;
		dev->devnum = i;
	}

	num_devices = n;					/* successfully opened */
	if (num_devices == 0) {
		ksprintf(message,"%s: no SCSI/Link devices opened successfully\r\n",DRIVER);
		c_conws(message);
		return 1L;
	}

	/*
	 *	register the interfaces, displaying devices as we go
	 */
	for (i = 0; i < num_devices; i++) {
		nif = niftab[i];
		dev = nif->data;
		rc = scsilink_busid(dev->devnum);
		if (rc >= 0L) {
			bus = (int)(rc >> 16);
			id = (int)(rc & 0xffff);
		} else bus = id = -1;			/* error, but not a show-stopper */

		if_register(nif);

		ksprintf(message,"%s v%d.%02d (%s%d) (%02x:%02x:%02x:%02x:%02x:%02x) on bus/id %d/%d\r\n",
				DRIVER_DESC,MAJOR_VERSION,MINOR_VERSION,ETHERNET_NAME,nif->unit,
				nif->hwlocal.adr.bytes[0],nif->hwlocal.adr.bytes[1],nif->hwlocal.adr.bytes[2],
				nif->hwlocal.adr.bytes[3],nif->hwlocal.adr.bytes[4],nif->hwlocal.adr.bytes[5],bus,id);
		c_conws(message);
	}

	install_interrupts();				/* VBL at the moment */

	return 0L;
}


/********************************************
*											*
*	D R I V E R   R O U T I N E S			*
*											*
********************************************/

static long _cdecl sl_open(struct netif *nif)
{
struct sl_device *dev = nif->data;
long rc;

	if (dev->interface_up)
		return 0L;

	if (set_lock(&dev->active) == 0L)
		return ENOENT;

	rc = scsilink_reset(dev->devnum,1);
	trace(dev,TRACE_OPEN,rc,0,NULL);

	release_lock(&dev->active);

	if (rc)
		return ENOENT;

	dev->interface_up = TRUE;
	return 0L;
}

static long _cdecl sl_close(struct netif *nif)
{
struct sl_device *dev = nif->data;
long rc;

	if (!dev->interface_up)
		return 0L;

	if (set_lock(&dev->active) == 0L)
		return ENOENT;

	rc = scsilink_reset(dev->devnum,0);
	trace(dev,TRACE_CLOSE,rc,0,NULL);

	release_lock(&dev->active);

	if (rc)
		return ENOENT;

	dev->interface_up = FALSE;
	return 0L;
}

static long _cdecl sl_output(struct netif *nif,BUF *buf,const char *hwaddr,short hwlen,short pktype)
{
struct sl_device *dev = nif->data;
short len;
BUF *nbuf;
long rc;

	//FIXME: should we attempt a read first??  or after the write ??

	dev->count.output_calls++;
	nbuf = eth_build_hdr(buf,nif,hwaddr,pktype);
	if (!nbuf) {
		nif->out_errors++;
		return ENOMEM;
	}
	if (nif->bpf)
		bpf_input(nif,nbuf);

	/*
	 *	we drop packet here if interface is down (like PAMSDMA.C)
	 *	IS THIS CORRECT?
	 */
	if (!dev->interface_up) {
		dev->count.output_ifdown++;
		buf_deref(nbuf,BUF_NORMAL);
		return ENOMEM;
	}

	/*
	 *	check if device is busy or resetting
	 *	if so, we must queue the I/O
	 */
	if (set_lock(&dev->active) == 0L) {
		dev->count.output_dev_active++;
		return queue_output(nif,nbuf);
	}
	if (scsilink_status(dev->devnum) == RESET_IN_PROGRESS) {
		release_lock(&dev->active);
		dev->count.output_dev_resetting++;
		return queue_output(nif,nbuf);
	}

	/*
	 *	all OK, we can do the I/O
	 *
	 * Enet packets must at least have 60 bytes ...
	 */
	len = (short) max(nbuf->dend-nbuf->dstart,ETH_MIN_LEN);
	rc = write_packet(dev,nbuf,len);

	if (rc != 0L) {
		dev->count.write_fails++;
		nif->out_errors++;
	} else {
		dev->count.packets_sent++;
		nif->out_packets++;
	}

	buf_deref(nbuf,BUF_NORMAL);		/* we're done with it ... */
	release_lock(&dev->active);

	return rc;						//FIXME: should this be 0 ???
}

static long queue_output(struct netif *nif,BUF *nbuf)
{
struct sl_device *dev = nif->data;
long rc;

	rc = if_enqueue(&nif->snd,nbuf,nbuf->info);
	if (rc) {
		nif->out_errors++;
		return rc;
	}
	dev->count.write_enqueues++;

	return 0L;
}

static long _cdecl sl_ioctl(struct netif *nif,short cmd,long argument)
{
struct sl_device *dev = nif->data;
struct ifreq *ifr;
SCSILINK_STATS *user;
char *p;
long n, rc = 0L;
unsigned long option;

	switch(cmd) {
	case SIOCSIFNETMASK:
	case SIOCSIFFLAGS:
	case SIOCSIFADDR:
		break;
	case SIOCSIFMTU:
		/*
		 * Limit MTU to 1500 bytes. MintNet has already set nif->mtu
		 * to the new value, we only limit it here.
		 */
		if (nif->mtu > ETH_MAX_DLEN)
			nif->mtu = ETH_MAX_DLEN;
		break;

	case SIOCGLNKSTATS:				/* returns statistics (plus trace if active) */
		/* first we retrieve the latest hardware statistics */
		if (set_lock(&dev->active)) {									/* precautionary */
			rc = scsilink_statistics(dev->devnum,dev->count.sl_count);	/* update struct with hardware stats */
			release_lock(&dev->active);
			trace(dev,TRACE_STATS,rc,6*sizeof(long),(char *)(dev->count.sl_count));
		}
		ifr = (struct ifreq *)argument;
		user = (SCSILINK_STATS *)ifr->ifru.data;			/* point to user area */
		user->magic = STATS_MAGIC;
		user->major_version = MAJOR_VERSION;
		user->minor_version = MINOR_VERSION;
		user->reserved = 0;
		user->count = dev->count;							/* copy statistics */
		user->trace_entries = dev->trace_entries;
		user->current_entry = dev->current_entry;
		if (dev->trace_entries && dev->trace_table)
			memcpy(user->trace_table,dev->trace_table,dev->trace_entries*sizeof(SLINK_TRACE));
		break;
	case SIOCGLNKFLAGS:				/* misuse, but should work ... */
		ifr = (struct ifreq *)argument;
		option = (unsigned long)ifr->ifru.data;

		if ((option&0xff) == SL_SET_TRACE) {	/*** set number of trace entries ***/
			n = option >> 8;						/* new size of trace table */
			if (dev->trace_table) {					/* free any existing trace */
				dev->trace_entries = 0L;
				dev->current_entry = 0L;
				kfree(dev->trace_table);
				dev->trace_table = NULL;
			}
			if (n) {								/* allocate any requested new trace */
				dev->trace_table = kmalloc(n*sizeof(SLINK_TRACE));
				if (!dev->trace_table)
					return ENOMEM;
				memset(dev->trace_table,0,n*sizeof(SLINK_TRACE));
				dev->trace_entries = n;
				dev->current_entry = 0L;
			}
			break;
		}

		if (option == SL_GET_VERSION)			/*** return current driver version ***/
			return (MAJOR_VERSION<<8) | MINOR_VERSION;

		if (option == SL_STATS_LENGTH)			/*** return length of statistics data ***/
			return sizeof(SCSILINK_STATS) + dev->trace_entries*sizeof(SLINK_TRACE);
			
		if (option == SL_CLEAR_COUNTS) {		/*** clear counts ***/
			/*
			 *	FIXME: here we should reset the hardware statistics, when we figure out how
			 *	(it may not be possible, or it may happen every time we fetch them)
			 */
			memset((char *)&dev->count,0,sizeof(struct scsilink_counts));
			break;
		}

		if (option == SL_GET_MACADDR) {			/*** get MAC address */
			p = (char *)&ifr->ifru.data;
			memcpy(p,nif->hwlocal.adr.bytes,ETH_ALEN);
			break;
		}

		if (option == SL_RESET_DEVICE) {		/*** reset device ***/
			if (set_lock(&dev->active) == 0L)
				return ELOCKED;
			rc = scsilink_reset(dev->devnum,dev->interface_up?1:0);
			trace(dev,TRACE_RESET,rc,sizeof(long),(char *)(dev->interface_up?1L:0L));
			release_lock(&dev->active);
			if (rc)
				return EINTERNAL;
			break;
		}
		/* else drop through: invalid arg (reserved for future use) */
	default:
		return EINVAL;
	}

	return 0L;
}

/********************************************
*											*
*	I N T E R R U P T   H A N D L E R		*
*											*
********************************************/

/*
 *	interrupt handler
 */
void interrupt_handler(void)
{
	addroottimeout(0,handle_all_input,1);
}

/********************************************
*											*
*	I N P U T   H A N D L E R				*
*											*
********************************************/

static void handle_all_input(PROC *proc, long arg)
{
int i;

	for (i = 0; i < num_devices; i++)
		handle_input(niftab[i]);
}

static void handle_input(struct netif *nif)
{
struct sl_device *dev = nif->data;
int sent = 0;
int len;
long rc;
BUF *nbuf;

	dev->count.input_calls++;

	if (!dev->interface_up) {
		dev->count.input_ifdown++;
		return;
	}

	if (set_lock(&dev->active) == 0L) {		/* in use (by output or ioctl routine) */
		dev->count.input_dev_active++;
		return;
	}

	if (scsilink_status(dev->devnum) == RESET_IN_PROGRESS) {
		release_lock(&dev->active);
		dev->count.input_dev_resetting++;
		return;
	}

	get_waiting_packets(nif);				/* receive one or more waiting packets */

	while((nbuf=if_dequeue(&nif->snd))) {	/* send any queued packets */
		dev->count.write_dequeues++;
		/*
		 * Enet packets must at least have 60 bytes ...
		 */
		len = (short) max(nbuf->dend-nbuf->dstart,ETH_MIN_LEN);
		rc = write_packet(dev,nbuf,len);

		if (rc != 0L) {
			dev->count.write_fails++;
			nif->out_errors++;
		} else {
			dev->count.packets_sent++;
			nif->out_packets++;
		}

		buf_deref(nbuf,BUF_ATOMIC);
		sent++;
	}

	//FIXME maybe we should read again if any were sent?
	release_lock(&dev->active);				/* let output go again ... */
}

/*
 *	get any waiting packets in the adapter and pass to upper layers
 */
static void get_waiting_packets(struct netif *nif)
{
struct sl_device *dev = nif->data;
long rc;

	do {									/* receive any waiting packets */
		rc = read_packet(dev,(char *)&scsiPacket);

		if (rc != 0L) {
			dev->count.read_fails++;
			break;
		}
		if (scsiPacket.length == 0) {
			dev->count.zero_reads++;
			break;
		}
		if (scsiPacket.flags < 0L)
			dev->count.negative_flag++;		/* informational */

		scsiPacket.length -= 4;					/* adjust for length of crc */

		//FIXME should we cross-check length vs type in frame ???

		if ((scsiPacket.length < ETH_MIN_LEN)
		 || (scsiPacket.length > ETH_MAX_LEN))
			dev->count.invalid_length++;
		else {
			if (!receive_packet(nif))
				dev->count.receive_errors++;
			else dev->count.packets_received++;
		}
	} while(scsiPacket.flags > 0L);
}

/*
 *	get packet from memory and pass it to the upper layers
 *	return 1 if OK, 0 if error
 */
static int receive_packet(struct netif *nif)
{
BUF *b;

	/*
	 *	copy data to local buffer
	 */
	b = buf_alloc(scsiPacket.length+100,50,BUF_ATOMIC);
	if (!b) {
		nif->in_errors++;
		return 0;
	}
	b->dend += scsiPacket.length;
	memcpy(b->dstart,(char *)&scsiPacket.enet,scsiPacket.length);

	/*
	 *	pass packet to upper layers
	 */
	if (nif->bpf)
		bpf_input(nif,b);
	
	if (if_input(nif,b,0,eth_remove_hdr(b))) {
		nif->in_errors++;
		return 0;
	}

	nif->in_packets++;
	return 1;
}


/********************************************
*											*
*	U T I L I T Y   R O U T I N E S			*
*											*
********************************************/

/*
 *	read packet, with retry if appropriate
 */
static long read_packet(struct sl_device *dev,char *packet)
{
SCSI_PACKET *sp = (SCSI_PACKET *)packet;
long rc;
WORD euid;

	rc = scsilink_read(dev->devnum,packet);
	dev->count.scsi_reads++;

	if ((sp->length != 0) || (rc != 0L))	/* avoid useless tracing */
		trace(dev,TRACE_READ_1,rc,sp->length-4,(char *)&sp->enet);

	if (rc == EPERM) {		/* try to bypass problem with ftpd ... */
		euid = p_geteuid();		/* remember current euid */
		if (euid) {							/* if non-zero, */
			p_seteuid(0);		/* try to set to super-user */
			rc = scsilink_read(dev->devnum,packet);	/* retry */
			p_setuid(euid);
			trace(dev,TRACE_READ_2,rc,sp->length-4,(char *)&sp->enet);
			if (rc == 0L)					/* ok, so */
				dev->count.read_fixups++;	/* count it */
		}
	}

	return rc;
}

/*
 *	write packet, with retry if appropriate
 */
static long write_packet(struct sl_device *dev,BUF *nbuf,int len)
{
long rc;
WORD euid;

	rc = scsilink_write(dev->devnum,nbuf->dstart,len);
	trace(dev,TRACE_WRITE_1,rc,len,nbuf->dstart);
	dev->count.scsi_writes++;

	if (rc == EPERM) {		/* try to bypass problem with ftpd ... */
		euid = p_geteuid();		/* remember current euid */
		if (euid) {							/* if non-zero, */
			p_seteuid(0);		/* try to set to super-user */
			rc = scsilink_write(dev->devnum,nbuf->dstart,len); /* retry */
			p_seteuid(euid);
			trace(dev,TRACE_WRITE_2,rc,len,nbuf->dstart);
			if (rc == 0L)					/* ok, so */
				dev->count.write_fixups++;	/* count it */
		}
	}

	return rc;
}

/*
 *	make entry in internal trace
 */
static void add_trace_entry(struct sl_device *dev,char type,long rc,short length,char *data)
{
SLINK_TRACE *t;

	if (!dev->trace_table)		/* paranoia */
		return;

	t = &dev->trace_table[dev->current_entry];
	t->time = *_hz_200;
	t->rc = rc;
	t->type = type;
	t->length = length;
	if (length > 0)
		memcpy(t->data,data,min(SLINK_TRACE_LEN,length));

	if (++dev->current_entry >= dev->trace_entries)
		dev->current_entry = 0;
}
