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
 *	scsilink.h: header for SCSI/Link library routines
 *
 *	the following functions are provided:
 *
 *		long scsilink_probe(void)
 *			probes for valid SCSI/Link devices accessible via the SCSIDRV
 *			interface.
 *			returns	the number of devices found (the bus/id of each detected
 *						device is stored in an internal table)
 *					zero if no devices were found
 *					negative if an error was detected (no SCSIDRV support)
 *
 *		long scsilink_open(long devnum,char *macaddr)
 *			opens a previously-discovered device (devnum is 0 thru n-1, where
 *			n is the positive value returned by scsilink_probe().
 *			returns 0 if OK (the character array pointed to by macaddr is
 *					updated to contain the device's MAC address)
 *					negative if an error was detected
 *
 *		long scsilink_read(long devnum,char *buf)
 *			reads a packet from the device 'devnum' into the specified buffer;
 *			if the return code from SCSIDRV is greater than zero, the device
 *			is reset and enabled.
 *			returns the return code from SCSIDRV
 *
 *		long scsilink_write(long devnum,char *buf,long length)
 *			sends a packet of the specified length from the buffer to the
 *			device 'devnum'; if the return code from SCSIDRV is greater than
 *			zero, the device is reset and enabled.
 *			returns the return code from SCSIDRV
 *
 *		long scsilink_reset(long devnum,long type)
 *			issues a device reset command to the device 'devnum': iff type==0,
 *			the	device is left disabled, otherwise it is enabled.  when the
 *			device is enabled, the external variable scsilink_next_io[devnum]
 *			is updated with the earliest time that an I/O should be issued to
 *			the device. 
 *			returns the return code from SCSIDRV
 *
 *		long scsilink_busid(long devnum)
 *			retrieves the SCSIDRV bus and id associated with the device
 *			'devnum'.
 *			returns bus number in high-order word of return code, scsi id in
 *					low-order word
 *					negative if the device numberdata could not be retrieved
 *
 *		long scsilink_modetest(long required)
 *			if 'required' is TRUE, requests SCSIDRV.LIB to test for supervisor
 *			mode, and switch to it if necessary, before calling the SCSIDRV
 *			routines.  if 'required' is FALSE, informs SCSIDRV.LIB that it is
 *			already in supervisor mode, so no switch will be made.  by default
 *			a mode test *will* always be made.
 *			returns negative if error, otherwise zero.
 *
 *		long scsilink_statistics(long devnum,long *counter)
 *			retrieves two sets of counts into the array of 6 longs pointed
 *			to by 'counter':
 *				. three internal counts maintained by this library: reads,
 *				  writes, resets
 *				. three hardware error statistics maintained by the device
 *				  'devnum'.
 *			returns negative if 'devnum' is invalid, otherwise returns 0;
 *			if the hardware error statistics cannot be read, the corresponding
 *			counts are set to -1L
 *
 *		long scsilink_status(long devnum)
 *			returns status of device 'devnum'; valid values are:
 *				NORMAL / RESET_IN_PROGRESS / NEED_BROADCAST
 *
 *		long scsilink_set_macaddr(long devnum,char *macaddr)
 *			sets MAC address for device 'devnum' to 'macaddr'
 *			returns negative if 'devnum' is invalid, otherwise the return
 *			code from SCSIDRV
 *
 *		long scsilink_set_broadcast(long devnum)
 *			enables broadcast mode for device 'devnum'
 *			returns negative if 'devnum' is invalid, otherwise the return
 *			code from SCSIDRV
 *
 *
 *	version 1.0	rfb, march/2002
 *		initial version.  note that all arguments are longs or pointers, to
 *		circumvent complications with default short ints and/or type-based
 *		stack alignment.
 *
 *	version 1.2	rfb, may/2002
 *		added scsilink_modetest() function.  this version of the header is
 *		numbered to correspond with scsilink.c.
 *
 *	version 2.0	rfb, april/2005
 *		added functions scsilink_status(), scsilink_set_macaddr(),
 *		scsilink_set_broadcast(), scsilink_set_promiscuous().
 *		dropped scsilink_next_io[] array.
 *
 *	version 2.1	rfb, july/2005
 *		removed function scsilink_set_promiscuous().
 *
 *	version 2.2	rfb, august/2007
 *		cleanup for release.
 */
#ifndef SCSILINK_H
#define SCSILINK_H

#include "portab.h"
#include "inet4/ifeth.h"

 /*
  *	Missing ethernet definitions
  *	(ENET_HDR is eth_dgram from ifeth.h in fact
  *	but eth_dgram adds one pointer there and I'm
  *	not sure if I can just change it)
  */
 
typedef struct {				/* packet header */
	 char destination[ETH_ALEN];	/* Destination hardware address */
	 char source[ETH_ALEN];			/* Source hardware address */
	 UWORD type;					/* Ethernet protocol type */
} ENET_HDR;

typedef struct {				/* generic IP ethernet packet */
	ENET_HDR eh;
	char ed[ETH_MAX_DLEN];
} ENET_PACKET;
#define ETH_MIN_LEN		(ETH_HLEN+ETH_MIN_DLEN)
#define ETH_MAX_LEN		sizeof(ENET_PACKET)

/*
 *	SCSI hardware stuff
 */
typedef struct {
	WORD length;
	ULONG flags;
	ENET_PACKET enet;
	ULONG crc;
} SCSI_PACKET;
#define MAX_SCSI_READ	sizeof(SCSI_PACKET)	/* in bytes */

#define MAX_SCSILINK_DEVICES	16		/* max devices supported (!) */


/*
 *	values returned by scsilink_status()
 */
#define NORMAL				0
#define RESET_IN_PROGRESS	1
#define NEED_BROADCAST		2


/*
 *	function prototypes
 */
long scsilink_busid(long devnum);
long scsilink_modeswitch(long required);
long scsilink_open(long devnum,char *macaddr);
long scsilink_probe(void);
long scsilink_read(long devnum,char *buf);
long scsilink_reset(long devnum,long type);
long scsilink_set_broadcast(long devnum);
long scsilink_set_macaddr(long devnum,char *macaddr);
long scsilink_statistics(long devnum,long *counter);
long scsilink_status(long devnum);
long scsilink_write(long devnum,char *buf,long length);

#endif
