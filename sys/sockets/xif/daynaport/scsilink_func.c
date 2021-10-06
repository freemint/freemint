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
 *	scsilink.c: device access routines for SCSI/Link device
 *
 *	v1.0	march/2002	rfb
 *		first library version of SCSI/Link routines
 *
 *	v1.1	may/2002	rfb
 *		use new function SCSI_Options() to inform SCSIDRV library
 *		that no mode switch is required
 *
 *	v1.2	may/2002	rfb
 *		add new function scsilink_modetest() to specify whether
 *		SCSIDRV.LIB should test the current user/supervisor mode
 *		(and switch to supervisor if necessary) before calling the
 *		low-level SCSIDRV functions.
 *
 *	v2.0	april/2005	rfb
 *		. support sending 'enable broadcast' after reset (required
 *		  for firmware version 1.4a):
 *			. the next_io array is no longer exposed, instead we
 *			  provide the scsilink_status() function to tell us
 *			  where we are
 *			. a new function scsilink_set_broadcast() enables
 *			  reception of broadcast messages
 *		. add scsilink_set_promiscuous() to support promiscuous
 *		  mode (only works for firmware 1.4a)
 *		. add scsilink_set_macaddr() to set MAC address
 *
 *	v2.1	july/2005	rfb
 *		. remove scsilink_set_promiscuous() [my understanding of
 *		  the command set was wrong; I don't think promiscuous mode
 *		  is supported]
 *
 *	v2.2	august/2007	rfb
 *		. code cleanup for release
 *
 *	v3.0	november/2007	rfb
 *		. add scsilink_get_macaddr() to retrieve MAC address
 *		. remove scsilink_set_broadcast(): this function is now
 *		  performed automagically after each reset completes
 *		. rename scsilink_modetest() to scsilink_modeswitch()
 *		  for clarity; it now uses the new SCSIDRV library call
 *		  SCSI_Modeswitch()
 */
#include <osbind.h>

#include "libkern/libkern.h"
#include "cookie.h"

#include "portab.h"
#include "scsidefs.h"
#include "scsidriver.h"
#include "scsilink_func.h"

#include "mint/time.h"

#define VERSION			"v3.0"	/* of my code */

#define MANUFACTURER	"Dayna   "
#define PRODUCT			"SCSI/Link       "

#define SCSI_TIMEOUT	5		/* in ticks */
#define ENABLE_DELAY	100

typedef struct {
	unsigned char byte[4];
} ILONG;

struct cmd09_data {		/* returned by 0x09 SCSI command */
	char macaddr[ETH_ALEN];
	ILONG counter1;			/* these 3 counters are possibly (in no order): crc_errors, frame_errors, missed_errors, */
	ILONG counter2;			/* overruns, collisions, carrier, heartbeat, window ... */
	ILONG counter3;
};

/*
 *	globals
 */
static long num_devices = 0L;			/* count of entries in following table */
static struct {
	long bus_id;						/* actually 16-bit bus number || 16-bit id number */
	tHandle handle;						/* corresponding scsidrv handle */
	ULONG next_io;						/* earliest time for next i/o (after reset) */
	long status;						/* NORMAL or NEED_BROADCAST */
	long reads;							/* values returned by scsilink_statistics() */
	long writes;
	long resets;
} devtab[MAX_SCSILINK_DEVICES];

static BYTE sense[18];					/* global to reduce stack requirements (precautionary) */

/*
 *	function prototypes
 */
static long check_device(long bus,long id);
static long get_device_data(long devnum,struct cmd09_data *data);
static long get_ticks(void);
static unsigned long getulong(ILONG *p);
static long set_broadcast_mode(long devnum);

long scsilink_modeswitch(long required);
long scsilink_get_macaddr(long devnum,char *macaddr);


/********************************************
*											*
*	E X T E R N A L   I N T E R F A C E		*
*											*
********************************************/

/*
 *	probe for valid SCSI/LINK devices
 *		returns	number of devices found (bus/id stored in internal table)
 *				-ve: error
 */
long scsilink_probe()
{
unsigned long busses, b;
long i, j;

	busses = SCSI_Init(NULL);
	if (!busses)
		return -1L;

	for (i = 0, b = busses, num_devices = 0; i < NUMBUSES; i++, b >>= 1) {
		if (b & 0x01)
			for (j = 0; j < 8; j++)
				if (check_device(i,j) == 0L)
					if (num_devices < MAX_SCSILINK_DEVICES)		/* if too many SCSI/LINKs, ignore silently */
						devtab[num_devices++].bus_id = (i<<16) | j;
	}

	return num_devices;
}

/*
 *	open one of the previously-discovered devices
 *		returns 0 if OK (MAC addr is filled in)
 *				-ve if error
 */
long scsilink_open(long devnum,char *macaddr)
{
struct cmd09_data temp;
DLONG scsiid = { 0L, 0L };
ULONG maxlen, rc;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;
	if (devtab[devnum].handle)		/* already open? */
		return -1L;

	scsiid.lo = devtab[devnum].bus_id & 0xffff;
	if ((rc=SCSI_Open(devtab[devnum].bus_id>>16,&scsiid,&maxlen)) <= 0L)
		return -1L;

	if (maxlen < MAX_SCSI_READ) {	/* in case something is screwy ... */
		SCSI_Close((tHandle)rc);
		return -1L;
	}

	devtab[devnum].handle = (tHandle)rc;

	if (get_device_data(devnum,&temp) < 0L) {
		SCSI_Close((tHandle)rc);
		devtab[devnum].handle = NULL;
		return -1L;
	}
	memcpy(macaddr,temp.macaddr,ETH_ALEN);

	set_broadcast_mode(devnum);

	return 0L;
}

/*
 *	return bus and id corresponding to devnum
 */
long scsilink_busid(long devnum)
{
	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	return devtab[devnum].bus_id;
}

/*
 *	inform SCSIDRV if mode switch is required
 */
long scsilink_modeswitch(long required)
{
	return SCSI_Modeswitch(required);
}

/*
 *	return device status
 */
long scsilink_status(long devnum)
{
	if (*(_hz_200) < devtab[devnum].next_io)
		return RESET_IN_PROGRESS;

	return NORMAL;
}

/*
 *	get MAC address
 */
long scsilink_get_macaddr(long devnum,char *macaddr)
{
struct cmd09_data temp;
long rc;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	if ((rc=get_device_data(devnum,&temp)) != 0L)
		return rc;

	memcpy(macaddr,temp.macaddr,ETH_ALEN);
	return 0L;
}

/*
 *	return internal counts plus error statistics captured by hardware
 *
 *	COMMENT: if we find that retrieving the hardware counts resets them,
 *	we probably should do the same for the internal counts ...
 */
long scsilink_statistics(long devnum,long *counter)
{
struct cmd09_data temp;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	*counter++ = devtab[devnum].reads;		/* return internal counts */
	*counter++ = devtab[devnum].writes;
	*counter++ = devtab[devnum].resets;

	if (get_device_data(devnum,&temp) == 0L) {
		*counter++ = getulong(&temp.counter1);
		*counter++ = getulong(&temp.counter2);
		*counter = getulong(&temp.counter3);
	} else {
		*counter++ = -1L;		/* show stats not available */
		*counter++ = -1L;
		*counter = -1L;
	}

	return 0L;
}

/*
 *	read packet from device
 */
long scsilink_read(long devnum,char *buf)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x08, 0, 0, MAX_SCSI_READ>>8, MAX_SCSI_READ&0xff, 0x80 };
long rc;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	if (devtab[devnum].status == NEED_BROADCAST)
		set_broadcast_mode(devnum);

	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = buf;
	scsi.TransferLen = MAX_SCSI_READ;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	rc = SCSI_In(&scsi);
	if (rc > 0)
		scsilink_reset(devnum,1);

	devtab[devnum].reads++;

	return rc;
}

/*
 *	reset device: leave it disabled iff type=0
 */
long scsilink_reset(long devnum,long type)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x0e, 0, 0, 0, 0, 0 };
long rc, t;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	cmd[5] = type?0x80:0x00;
	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = NULL;
	scsi.TransferLen = 0;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	rc = SCSI_Out(&scsi);

	/*
	 *	Note: the Mac driver delays at this point (when enabling
	 *	the device), for 30 Mac ticks (I believe), equivalent to
	 *	half a second.  Extensive tests of live code show that this
	 *	may be A GOOD IDEA.
	 */
	if (type) {
		devtab[devnum].status = NEED_BROADCAST;	/* really only needed for firmware v1.4a ... */
		t = Supexec(get_ticks) + ENABLE_DELAY;	/* we don't use max() because it needs ints, */
		if (t > devtab[devnum].next_io)			/*  which are 16 bits for this module ...    */
			devtab[devnum].next_io = t;
	}

	devtab[devnum].resets++;

	return rc;
}

/*
 *	set MAC address
 */
long scsilink_set_macaddr(long devnum,char *macaddr)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x0c, 0, 0, 0, 0x04, 0x40 };

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;
	devtab[devnum].status = NORMAL;

	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = macaddr;
	scsi.TransferLen = ETH_ALEN;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	return SCSI_Out(&scsi);
}

/*
 *	send packet to device
 */
long scsilink_write(long devnum,char *buf,long length)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x0a, 0, 0, 0, 0, 0 };
long rc;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	if (devtab[devnum].status == NEED_BROADCAST)
		set_broadcast_mode(devnum);

	cmd[3] = length >> 8;
	cmd[4] = length & 0xff;
	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = buf;
	scsi.TransferLen = length;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	rc = SCSI_Out(&scsi);
	if (rc > 0)
		scsilink_reset(devnum,1);

	devtab[devnum].writes++;

	return rc;
}


/********************************************
*											*
*	I N T E R N A L   R O U T I N E S		*
*											*
********************************************/

/*
 *	check if device exists and is a SCSI/Link
 */
static long check_device(long bus,long id)
{
char inquiry[32];
DLONG scsiid = { 0L, 0L };

	scsiid.lo = id;
	if (SCSI_Inquiry(bus,&scsiid,0,inquiry))
		return -1L;

	if (strncmp(inquiry+8,MANUFACTURER,8) != 0)
		return -1L;
	if (strncmp(inquiry+16,PRODUCT,16) != 0)
		return -1L;

	return 0L;
}

/*
 *	get MAC address & error statistics from device
 */
static long get_device_data(long devnum,struct cmd09_data *data)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x09, 0, 0, 0, 18, 0 };
long rc;

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;

	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = (char *)data;
	scsi.TransferLen = 18;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	rc = SCSI_In(&scsi);
	if (rc > 0) {
		scsilink_reset(devnum,1);
		rc = SCSI_In(&scsi);
	}

	return rc;
}

/* the following routine must be called from supervisor mode! */
static long get_ticks()
{
	return *(_hz_200);
}

static unsigned long getulong(ILONG *p)
{
	return ((ULONG)p->byte[3]<<24) + ((ULONG)p->byte[2]<<16) + ((ULONG)p->byte[1]<<8) + (ULONG)p->byte[0];
}

/*
 *	enable broadcast messages
 */
static long set_broadcast_mode(long devnum)
{
tSCSICmd scsi;
static BYTE cmd[6] = { 0x0c, 0, 0, 0, 0x04, 0x80 };

	if ((devnum < 0L) || (devnum >= num_devices))
		return -1L;
	devtab[devnum].status = NORMAL;

	scsi.Handle = devtab[devnum].handle;
	scsi.Cmd = cmd;
	scsi.CmdLen = 6;
	scsi.Buffer = NULL;
	scsi.TransferLen = 0;
	scsi.SenseBuffer = sense;
	scsi.Timeout = SCSI_TIMEOUT;
	scsi.Flags = 0;

	return SCSI_Out(&scsi);
}
