/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 * Modified for Atari by Didier Mequignon 2009
 *
 * Most of this source has been derived from the Linux USB
 * project:
 *   (c) 1999-2002 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *   (c) 2000 David L. Brown, Jr. (usb-storage@davidb.org)
 *   (c) 1999 Michael Gee (michael@linuxspecific.com)
 *   (c) 2000 Yggdrasil Computing, Inc.
 *
 *
 * Adapted for U-Boot:
 *   (C) Copyright 2001 Denis Peter, MPL AG Switzerland
 *
 * For BBB support (C) Copyright 2003
 * Gary Jennejohn, DENX Software Engineering <garyj@denx.de>
 *
 * BBB support based on /sys/dev/usb/umass.c from
 * FreeBSD.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/* Note:
 * Currently only the CBI transport protocoll has been implemented, and it
 * is only tested with a TEAC USB Floppy. Other Massstorages with CBI or CB
 * transport protocoll may work as well.
 */
/*
 * New Note:
 * Support for USB Mass Storage Devices (BBB) has been added. It has
 * only been tested with USB memory sticks.
 */

#include "../../global.h"

#include "scsi.h"

#include "../../config.h"
#include "../../usb.h"
#include "../../ucd/ucd_defs.h"
#include "../udd_defs.h"
#include "../../usb_api.h"

#define MSG_VERSION "0.3.0"
char *drv_version = MSG_VERSION;

#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT	\
	"\033p USB mass storage class driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"Ported, mixed and shaken by David Galvez.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"

/*
#if defined(CONFIG_USB_UHCI) || defined(CONFIG_USB_OHCI) || defined(CONFIG_USB_EHCI) \
			|| defined(CONFIG_USB_ISP116X_HCD) || defined(CONFIG_USB_ARANYM_HCD)
#ifdef CONFIG_USB_STORAGE
*/

/*
 * Debug section
 */
#ifndef TOSONLY

#if 0
# define DEV_DEBUG	1
#endif

#ifdef DEV_DEBUG

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x
/*
 * Extra debug messages
 * uncomment them if you want them on
 * BE AWARE, IF YOU TURN THEM ON IN SLOW SYSTEMS
 * THE OVERHAEAD CAN PRODUCE THE BUS MALFUNCTION 
 */
//#define USB_STOR_DEBUG
//#define BBB_COMDAT_TRACE
//#define BBB_XPORT_TRACE

#else

# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x

#endif

#endif

/****************************************************************************/
/* BEGIN kernel interface */

#ifndef TOSONLY
struct kentry	*kentry;
#else
extern unsigned long _PgmSize;
#endif
struct usb_module_api	*api;

/* END kernel interface */
/****************************************************************************/

/*
 * USB device interface
 */

static long _cdecl	storage_open		(struct uddif *);
static long _cdecl	storage_close		(struct uddif *);
static long _cdecl	storage_ioctl		(struct uddif *, short, long);

static char lname[] = "USB mass storage class driver\0";

static struct uddif storage_uif = 
{
	0,			/* *next */
	USB_DEVICE,		/* class */
	lname,			/* lname */
	"storage",		/* name */
	0,			/* unit */
	0,			/* flags */
	storage_open,		/* open */
	storage_close,		/* close */
	0,			/* resrvd1 */
	storage_ioctl,		/* ioctl */
	0,			/* resrvd2 */
};


/*
 * External prototypes
 */
extern void ltoa		(char *buf, long n, unsigned long base);
extern long install_usb_stor	(long dev_num, unsigned long part_type, 
			     	 unsigned long part_offset, unsigned long part_size, 
			     	 char *vendor, char *revision, char *product);
extern long uninstall_usb_stor	(long dev_num);

/* Both these used by bios.S */
#ifdef TOSONLY
unsigned long max_logical_drive = 16;
#else
unsigned long max_logical_drive = 32;
#endif
unsigned long usb_1st_disk_drive = 0;

/* direction table -- this indicates the direction of the data
 * transfer for each command code -- a 1 indicates input
 */
unsigned char us_direction[256/8] = {
	0x28, 0x81, 0x14, 0x14, 0x20, 0x01, 0x90, 0x77,
	0x0C, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#define US_DIRECTION(x) ((us_direction[x>>3] >> (x & 7)) & 1)


/*
 * CBI style
 */

#define US_CBI_ADSC		0

/*
 * BULK only
 */
#define US_BBB_RESET		0xff
#define US_BBB_GET_MAX_LUN	0xfe

/* Command Block Wrapper */
typedef struct
{
	__u32		dCBWSignature;
#	define CBWSIGNATURE	0x43425355
	__u32		dCBWTag;
	__u32		dCBWDataTransferLength;
	__u8		bCBWFlags;
#	define CBWFLAGS_OUT	0x00
#	define CBWFLAGS_IN	0x80
	__u8		bCBWLUN;
	__u8		bCDBLength;
#	define CBWCDBLENGTH	16
	__u8		CBWCDB[CBWCDBLENGTH];
} umass_bbb_cbw_t;
#define UMASS_BBB_CBW_SIZE	31
static __u32 CBWTag;

/* Command Status Wrapper */
typedef struct
{
	__u32		dCSWSignature;
#	define CSWSIGNATURE	0x53425355
	__u32		dCSWTag;
	__u32		dCSWDataResidue;
	__u8		bCSWStatus;
#	define CSWSTATUS_GOOD	0x0
#	define CSWSTATUS_FAILED 0x1
#	define CSWSTATUS_PHASE	0x2
} umass_bbb_csw_t;
#define UMASS_BBB_CSW_SIZE	13

#define USB_MAX_STOR_DEV 	5	/* DEFAULT 5 */

static block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];

struct us_data;
typedef long (*trans_cmnd)(ccb *cb, struct us_data *data);
typedef long (*trans_reset)(struct us_data *data);

/* Galvez: For this struct we should take care and cast to long the unsigned 
 *         char elements before calling some of the MACROs defined in usb.h
 *         If we don't do that, when compiling with mshort, some values are 
 *	   transformed wrongly during the operations done in those MACROs
 */
struct us_data
{
	struct usb_device *pusb_dev;		 /* this usb_device */
	unsigned long	flags;			/* from filter initially */
# define USB_READY (1<<0)
	unsigned char	ifnum;			/* interface number */
	unsigned char	ep_in;			/* in endpoint */
	unsigned char	ep_out;			/* out ....... */
	unsigned char	ep_int;			/* interrupt . */
	unsigned char	subclass;		/* as in overview */
	unsigned char	protocol;		/* .............. */
	unsigned char	attention_done;		/* force attn on first cmd */
	unsigned short	ip_data;		/* interrupt data */
	long		action;			/* what to do */
	long		ip_wanted;		/* needed */
	long		*irq_handle;		/* for USB int requests */
	unsigned long	irqpipe;	 	/* pipe for release_irq */
	unsigned char	irqmaxp;		/* max packed for irq Pipe */
	unsigned char	irqinterval;		/* Intervall for IRQ Pipe */
	ccb		*srb;			/* current srb */
	trans_reset	transport_reset;	/* reset routine */
	trans_cmnd	transport;		/* transport routine */
};

struct bios_partitions
{
	unsigned long biosnum[32];		/* BIOS device number belonging this USB device */
	short partnum;				/* Total number of partitions this device has */
};

static struct us_data usb_stor[USB_MAX_STOR_DEV];
static struct bios_partitions bios_part[USB_MAX_STOR_DEV];

#define USB_STOR_TRANSPORT_GOOD	   0
#define USB_STOR_TRANSPORT_FAILED -1
#define USB_STOR_TRANSPORT_ERROR  -2

#define DEFAULT_SECTOR_SIZE 512

#define DOS_PART_TBL_OFFSET	0x1be
#define DOS_PART_MAGIC_OFFSET	0x1fe
#define DOS_PBR_FSTYPE_OFFSET	0x36
#define DOS_PBR_MEDIA_TYPE_OFFSET	0x15
#define DOS_MBR	0
#define DOS_PBR	1
#define DOS_FS_TYPE_OFFSET 0x36

//typedef union 
//{				/* Galvez: To avoid strict aliasing warnings */
//	unsigned long u32;
//	unsigned char u8[4];
//} U32;

typedef struct dos_partition
{
	unsigned char boot_ind;		/* 0x80 - active			*/
	unsigned char head;		/* starting head			*/
	unsigned char sector;		/* starting sector			*/
	unsigned char cyl;		/* starting cylinder			*/
	unsigned char sys_ind;		/* What partition type			*/
	unsigned char end_head;		/* end head				*/
	unsigned char end_sector;	/* end sector				*/
	unsigned char end_cyl;		/* end cylinder				*/
	unsigned long start4;		/* starting sector counting from 0	*/
	unsigned long size4;		/* nr of sectors in partition		*/
} dos_partition_t;

typedef struct disk_partition
{
	unsigned long type;
	unsigned long	start; /* # of first block in partition	*/
	unsigned long	size;  /* number of blocks in partition	*/
	unsigned long	blksz; /* block size in bytes			*/
} disk_partition_t;


//extern unsigned long swap_long(unsigned long val);
//#define le32_to_int(a) swap_32(*(unsigned long *)a)

/* Functions prototypes */
long 		usb_stor_get_info	(struct usb_device *, struct us_data *, block_dev_desc_t *);
long 		usb_stor_probe		(struct usb_device *, unsigned long, struct us_data *);
unsigned long 	usb_stor_read		(long, unsigned long, unsigned long, void *);
unsigned long 	usb_stor_write		(long, unsigned long, unsigned long, const void *);
void		usb_stor_eject		(long);
//struct usb_device * 	usb_get_dev_index	(long index);
block_dev_desc_t *	usb_stor_get_dev	(long);
void 		uhci_show_temp_int_td	(void);
long 		usb_stor_BBB_comdat	(ccb *, struct us_data *);
long 		usb_stor_CB_comdat	(ccb *, struct us_data *);
long 		usb_stor_CBI_get_status	(ccb *, struct us_data *);
long 		usb_stor_BBB_clear_endpt_stall	(struct us_data *, unsigned char);
long 		usb_stor_BBB_transport	(ccb *, struct us_data *);
long 		usb_stor_CB_transport	(ccb *, struct us_data *);
void 		usb_storage_init	(void);
long		usb_storage_probe	(struct usb_device *);
void		usb_storage_disconnect	(struct usb_device *);
void 		cconws_from_S		(char *);
void 		dsetdrv_from_S		(short);
void 		bconout_from_S		(short);
long 		XHDINewCookie_from_S	(void *);



static struct usb_driver storage_driver =
{
	name:		"usb-storage",
	probe:		usb_storage_probe,
	disconnect:	usb_storage_disconnect,
	next:		NULL,
};


/*
 * Galvez: I don't know how to call OS functions
 * from assembler(bios.S), this is a workaround.
 */
void
cconws_from_S(char *str)
{
	c_conws(str);
}

void
dsetdrv_from_S(short drv)
{
#ifdef TOSONLY
	Dsetdrv(drv);
#else
	d_setdrv(drv);
#endif
}

void
bconout_from_S(short c)
{
#ifdef TOSONLY
	Bconout(2, c);
#else
	b_ubconout(2, c); /* Console */
#endif
}


/* --- Inteface functions -------------------------------------------------- */

static long _cdecl
storage_open (struct uddif *u)
{
	return E_OK;
}

static long _cdecl
storage_close (struct uddif *u)
{
	return E_OK;
}

static long _cdecl
storage_ioctl (struct uddif *u, short cmd, long arg)
{
	return E_OK;
}
/* ------------------------------------------------------------------------- */

#if 0
static unsigned long usb_get_max_lun(struct us_data *us)
{
	int len;
	unsigned char result[1];
	len = usb_control_msg(us->pusb_dev,
			      usb_rcvctrlpipe(us->pusb_dev, 0),
			      US_BBB_GET_MAX_LUN,
			      USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
			      0, us->ifnum,
			      result, sizeof(char),
		      USB_CNTL_TIMEOUT * 5);
	DEBUG(("Get Max LUN -> len = %i, result = %i", len, (long) *result));
	return (len > 0) ? *result : 0;
}
#endif

block_dev_desc_t *usb_stor_get_dev(long idx)
{
	return(idx < USB_MAX_STOR_DEV) ? &usb_dev_desc[idx] : NULL;
}

void
init_part(block_dev_desc_t *dev_desc)
{
#ifdef TOSONLY
	unsigned char *buffer = (unsigned char *)Malloc(DEFAULT_SECTOR_SIZE);
#else
	unsigned char *buffer = (unsigned char *)kmalloc(DEFAULT_SECTOR_SIZE);
#endif
        if(buffer == NULL)
                return;
	if((dev_desc->block_read(dev_desc->dev, 0, 1, (unsigned long *)buffer) != 1)
	 || (buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) || (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa))
	{
#ifdef TOSONLY
		Mfree(buffer);
#else
		kfree(buffer);
#endif
		return;
	}
	dev_desc->part_type = PART_TYPE_DOS;
	DEBUG(("DOS partition table found"));
#ifdef USB_STOR_DEBUG			
	{
		char buf[128];
		char build_str[64];
		long j;
		
		sprintf(buf, sizeof(buf), "\0");

		for(j = 0; j < 512; j++)
		{
			if((j & 15) == 0)
			{
				sprintf(build_str, sizeof(build_str), "%04x ", j);
				strcat(buf, build_str);
			}
			sprintf(build_str, sizeof(build_str), "%02x ", buffer[j]);
			strcat(buf, build_str);
			if((j & 15) == 15)
			{
				long k;
				for(k = j-15; k <= j; k++)
				{
					if(buffer[k] < ' ' || buffer[k] >= 127)
						strcat(buf, ".");
					else 
					{
						sprintf(build_str, sizeof(build_str), "%c", buffer[k]);
						strcat(buf, build_str);
					}
				}
				DEBUG((buf));
				sprintf(buf, sizeof(buf), "\0");
			}
			
		}
	}			
#endif	
#ifdef TOSONLY
	Mfree(buffer);
#else
	kfree(buffer);
#endif
}

static inline long
is_extended(long part_type)
{
	return(part_type == 0x5 || part_type == 0xf || part_type == 0x85);
}

/*  Print a partition that is relative to its Extended partition table
 */
static long
get_partition_info_extended(block_dev_desc_t *dev_desc, long ext_part_sector, long relative, long part_num, long which_part, disk_partition_t *info)
{
	dos_partition_t *pt;
	long i;
	unsigned char buffer[DEFAULT_SECTOR_SIZE];

	if(dev_desc->block_read(dev_desc->dev, ext_part_sector, 1, (unsigned long *)buffer) != 1)
	{
		DEBUG(("Can't read partition table on %ld:%ld", dev_desc->dev, ext_part_sector));
		return -1;
	}
	
	if(buffer[DOS_PART_MAGIC_OFFSET] != 0x55 || buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa)
	{
		DEBUG(("bad MBR sector signature 0x%02x%02x", buffer[DOS_PART_MAGIC_OFFSET], buffer[DOS_PART_MAGIC_OFFSET + 1]));
		return -1;
	}
	/* Print all primary/logical partitions */
	pt = (dos_partition_t *)(buffer + DOS_PART_TBL_OFFSET);
	for(i = 0; i < 4; i++, pt++)
	{
		/* fdisk does not show the extended partitions that are not in the MBR */
		if((pt->sys_ind != 0) && (part_num == which_part) && (is_extended(pt->sys_ind) == 0))
		{
			info->type = (unsigned long)pt->sys_ind;
			info->blksz = 512;
			info->start = ext_part_sector + le2cpu32(pt->start4);
			info->size = le2cpu32(pt->size4);
			DEBUG(("DOS partition at offset 0x%lx, size 0x%lx, type 0x%x %s", 
					info->start, info->size, pt->sys_ind, 
					(is_extended(pt->sys_ind) ? " Extd" : "")));
			return 0;
		}
		/* Reverse engr the fdisk part# assignment rule! */
		if((ext_part_sector == 0) || (pt->sys_ind != 0 && !is_extended (pt->sys_ind)))
			part_num++;
	}
	/* Follows the extended partitions */
	pt = (dos_partition_t *)(buffer + DOS_PART_TBL_OFFSET);
	for(i = 0; i < 4; i++, pt++)
	{
		if(is_extended(pt->sys_ind))
		{
			long lba_start = le2cpu32(pt->start4) + relative;
			return get_partition_info_extended(dev_desc, lba_start, ext_part_sector == 0 ? lba_start : relative, part_num, which_part, info);
		}
	}
	return -1;
}

long
fat_register_device(block_dev_desc_t *dev_desc, long part_no, unsigned long *part_type, unsigned long *part_offset, unsigned long *part_size)
{
	unsigned char buffer[DEFAULT_SECTOR_SIZE];
	disk_partition_t info;

	if(!dev_desc->block_read)
		return -1;
		
	/* check if we have a MBR (on floppies we have only a PBR) */
	if(dev_desc->block_read(dev_desc->dev, 0, 1, (unsigned long *)buffer) != 1)
	{
		DEBUG(("Can't read from device %ld", dev_desc->dev));
		return -1;
	}
	
	if(buffer[DOS_PART_MAGIC_OFFSET] != 0x55 || buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa)
	{
		/* no signature found */
		return -1;
	}
	
	/* First we assume, there is a MBR */
	if(!get_partition_info_extended(dev_desc, 0, 0, 1, part_no, &info))
	{
		*part_type = info.type;
		*part_offset = info.start;
		*part_size = info.size;
	}
	else if(!strncmp((char *)&buffer[DOS_FS_TYPE_OFFSET], "FAT", 3))
	{
		/* ok, we assume we are on a PBR only */
		*part_type = 0;
		*part_offset = 0;
		*part_size = 0;
	}
	else
	{
		DEBUG(("Partition %ld not valid on device %ld", part_no, dev_desc->dev));
		return -1;
	}
	return 0;
}


void
dev_print(block_dev_desc_t *dev_desc)
{
#ifdef CONFIG_LBA48
	uint64_t lba512; /* number of blocks if 512bytes block size */
#else
	lbaint_t lba512;
#endif
	if(dev_desc->type == DEV_TYPE_UNKNOWN)
	{
		DEBUG(("not available"));
		return;
	}
	DEBUG(("Vendor: %s Rev: %s Prod: %s", dev_desc->vendor, dev_desc->revision, dev_desc->product));
	DEBUG((""));
	if((dev_desc->lba * dev_desc->blksz) > 0L)
	{
		unsigned long mb, mb_quot, mb_rem, gb, gb_quot, gb_rem;
		lbaint_t lba = dev_desc->lba;
		lba512 = (lba * (dev_desc->blksz / 512));
		mb = (10 * lba512) / 2048;	/* 2048 = (1024 * 1024) / 512 MB */
		/* round to 1 digit */
		mb_quot	= mb / 10;
		mb_rem	= mb - (10 * mb_quot);
		UNUSED(mb_rem);
		gb = mb / 1024;
		gb_quot	= gb / 10;
		gb_rem	= gb - (10 * gb_quot);
		UNUSED(gb_rem);
#ifdef CONFIG_LBA48
		if(dev_desc->lba48)
			DEBUG(("Supports 48-bit addressing"));
#endif
		DEBUG(("Capacity: %ld.%ld MB = %ld.%ld GB (%ld x %ld)", mb_quot, mb_rem, gb_quot, gb_rem, (unsigned long)lba, dev_desc->blksz));
//		ALERT(("Capacity: %ld.%ld MB = %ld.%ld GB (%ld x %ld)", mb_quot, mb_rem, gb_quot, gb_rem, (unsigned long)lba, dev_desc->blksz));
	}
	else
	{
		DEBUG(("Capacity: not available"));
		ALERT(("Capacity: not available"));
	}
}

//# endif /* CONFIG_USB_OHCI */


static long
usb_stor_irq(struct usb_device *dev)
{
	struct us_data *us;
	us = (struct us_data *)dev->privptr;
	c_conws("IRQ\r\n");
	if(us->ip_wanted)
		us->ip_wanted = 0;
	return 0;
}

#ifdef USB_STOR_DEBUG

static void
usb_show_srb(ccb *pccb)
{
	long i;
	DEBUG(("SRB: len %d datalen 0x%lx ", pccb->cmdlen, pccb->datalen));
	for (i = 0; i < 12; i++)
		DEBUG(("%02x ", pccb->cmd[i]));
	DEBUG((""));
}

static void
display_int_status(unsigned long tmp)
{
	DEBUG(("Status: %s %s %s %s %s %s %s",
		(tmp & USB_ST_ACTIVE) ? "Active" : "",
		(tmp & USB_ST_STALLED) ? "Stalled" : "",
		(tmp & USB_ST_BUF_ERR) ? "Buffer Error" : "",
		(tmp & USB_ST_BABBLE_DET) ? "Babble Det" : "",
		(tmp & USB_ST_NAK_REC) ? "NAKed" : "",
		(tmp & USB_ST_CRC_ERR) ? "CRC Error" : "",
		(tmp & USB_ST_BIT_ERR) ? "Bitstuff Error" : ""));
}

#endif

/***********************************************************************
 * Data transfer routines
 ***********************************************************************/

static long
us_one_transfer(struct us_data *us, long pipe, char *buf, long length)
{
	long max_size;
	long this_xfer;
	long result;
	long partial;
	long maxtry;
	long stat;
	/* determine the maximum packet size for these transfers */
	max_size = usb_maxpacket(us->pusb_dev, pipe) * 16;
	/* while we have data left to transfer */
	while(length)
	{
		/* calculate how long this will be -- maximum or a remainder */
		this_xfer = length > max_size ? max_size : length;
		length -= this_xfer;
		/* setup the retry counter */
		maxtry = 10;
		/* set up the transfer loop */
		do
		{
			/* transfer the data */
			DEBUG(("Bulk xfer 0x%xl(%ld) try #%ld", (unsigned long)buf, this_xfer, 11 - maxtry));
			result = usb_bulk_msg(us->pusb_dev, pipe, buf, this_xfer, &partial, USB_CNTL_TIMEOUT * 5, 0);
			DEBUG(("bulk_msg returned %ld xferred %ld/%ld", result, partial, this_xfer));
			if(us->pusb_dev->status != 0)
			{
				/* if we stall, we need to clear it before we go on */
#ifdef USB_STOR_DEBUG
				display_int_status(us->pusb_dev->status);
#endif
				if(us->pusb_dev->status & USB_ST_STALLED)
				{
					DEBUG(("stalled ->clearing endpoint halt for pipe 0x%lx", pipe));
					stat = us->pusb_dev->status;
					usb_clear_halt(us->pusb_dev, pipe);
					us->pusb_dev->status = stat;
					if(this_xfer == partial)
					{
						DEBUG(("bulk transferred with error %lx, but data ok", us->pusb_dev->status));
						return 0;
					}
					else
						return result;
				}
				if(us->pusb_dev->status & USB_ST_NAK_REC)
				{
					DEBUG(("Device NAKed bulk_msg"));
					return result;
				}
				DEBUG(("bulk transferred with error"));
				if(this_xfer == partial)
				{
					DEBUG((" %ld, but data ok", us->pusb_dev->status));
					return 0;
				}
				/* if our try counter reaches 0, bail out */
				DEBUG((" %ld, data %d", us->pusb_dev->status, partial));
				if(!maxtry--)
					return result;
			}
			/* update to show what data was transferred */
			this_xfer -= partial;
			buf += partial;
			/* continue until this transfer is done */
		}
		while (this_xfer);
	}
	/* if we get here, we're done and successful */
	return 0;
}

static long
usb_stor_BBB_reset(struct us_data *us)
{
	long result;
	long pipe;
	/*
	 * Reset recovery (5.3.4 in Universal Serial Bus Mass Storage Class)
	 *
	 * For Reset Recovery the host shall issue in the following order:
	 * a) a Bulk-Only Mass Storage Reset
	 * b) a Clear Feature HALT to the Bulk-In endpoint
	 * c) a Clear Feature HALT to the Bulk-Out endpoint
	 *
	 * This is done in 3 steps.
	 *
	 * If the reset doesn't succeed, the device should be port reset.
	 *
	 * This comment stolen from FreeBSD's /sys/dev/usb/umass.c.
	 */
	DEBUG(("BBB_reset"));
	result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0),
	 US_BBB_RESET, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, us->ifnum, 0, 0, USB_CNTL_TIMEOUT * 5);
	if((result < 0) && (us->pusb_dev->status & USB_ST_STALLED))
	{
		DEBUG(("RESET:stall"));
		return -1;
	}
	/* long wait for reset */
	mdelay(150);
	DEBUG(("BBB_reset result %ld: status %lx reset", result, us->pusb_dev->status));
	pipe = usb_rcvbulkpipe(us->pusb_dev, (long)us->ep_in);
	result = usb_clear_halt(us->pusb_dev, pipe);
	/* long wait for reset */
	mdelay(150);
	DEBUG(("BBB_reset result %d: status %lx clearing IN endpoint", result, us->pusb_dev->status));
	/* long wait for reset */
	pipe = usb_sndbulkpipe(us->pusb_dev, (long)us->ep_out);
	result = usb_clear_halt(us->pusb_dev, pipe);
	mdelay(150);
	DEBUG(("BBB_reset result %ld: status %lx clearing OUT endpoint", result, us->pusb_dev->status));
	DEBUG(("BBB_reset done"));
	return 0;
}

/* FIXME: this reset function doesn't really reset the port, and it
 * should. Actually it should probably do what it's doing here, and
 * reset the port physically
 */
static long
usb_stor_CB_reset(struct us_data *us)
{
	unsigned char cmd[12];
	long result;
	DEBUG(("CB_reset"));
	memset(cmd, 0xff, sizeof(cmd));
	cmd[0] = SCSI_SEND_DIAG;
	cmd[1] = 4;
	result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0), 
	 US_CBI_ADSC, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, us->ifnum, cmd, sizeof(cmd), USB_CNTL_TIMEOUT * 5);
	UNUSED(result);
	/* long wait for reset */
	mdelay(1500);
	DEBUG(("CB_reset result %ld: status %lx clearing endpoint halt", result, us->pusb_dev->status));
	usb_clear_halt(us->pusb_dev, usb_rcvbulkpipe(us->pusb_dev, (long)us->ep_in));
	usb_clear_halt(us->pusb_dev, usb_rcvbulkpipe(us->pusb_dev, (long)us->ep_out));
	DEBUG(("CB_reset done"));
	return 0;
}

/*
 * Set up the command for a BBB device. Note that the actual SCSI
 * command is copied into cbw.CBWCDB.
 */
long
usb_stor_BBB_comdat(ccb *srb, struct us_data *us)
{
	long result;
	long actlen;
	long dir_in;
	unsigned long pipe;
	umass_bbb_cbw_t cbw[sizeof(umass_bbb_cbw_t)];
	dir_in = US_DIRECTION(srb->cmd[0]);
	DEBUG(("usb_stor_BBB_comdat: dir_in: %ld",dir_in));
#ifdef BBB_COMDAT_TRACE

	char buf[srb->cmdlen * 64];
	char build_str[64];

	sprintf(buf, sizeof(buf), "\0");

	DEBUG(("dir %ld lun %d cmdlen %d cmd %lx datalen %ld pdata %lx", dir_in, srb->lun, srb->cmdlen, srb->cmd, srb->datalen, srb->pdata));
	if(srb->cmdlen)
	{
		for(result = 0; result < srb->cmdlen; result++) 
		{
			sprintf(build_str, sizeof(build_str), "cmd[%ld] 0x%x ", result, srb->cmd[result]);
			strcat(buf, build_str);
		}
		DEBUG((buf));
	}
#endif
	/* sanity checks */
	if(!(srb->cmdlen <= CBWCDBLENGTH))
	{
		DEBUG(("usb_stor_BBB_comdat: cmdlen too large"));
		return -1;
	}
	/* always OUT to the ep */
	pipe = usb_sndbulkpipe(us->pusb_dev, (long)us->ep_out);
	cbw->dCBWSignature = cpu2le32(CBWSIGNATURE);
	cbw->dCBWTag = cpu2le32(CBWTag++);
	cbw->dCBWDataTransferLength = cpu2le32(srb->datalen);
	cbw->bCBWFlags = (dir_in ? CBWFLAGS_IN : CBWFLAGS_OUT);
	cbw->bCBWLUN = srb->lun;
	cbw->bCDBLength = srb->cmdlen;
	/* copy the command data into the CBW command data buffer */
	/* DST SRC LEN!!! */
	memcpy(&cbw->CBWCDB, srb->cmd, srb->cmdlen);

	result = usb_bulk_msg(us->pusb_dev, pipe, cbw, UMASS_BBB_CBW_SIZE, &actlen, USB_CNTL_TIMEOUT * 5, 0);
	if(result < 0)
	{
		DEBUG(("usb_stor_BBB_comdat:usb_bulk_msg error"));
	}
	return result;
}

/* FIXME: we also need a CBI_command which sets up the completion
 * interrupt, and waits for it
 */
long
usb_stor_CB_comdat(ccb *srb, struct us_data *us)
{
	long result = 0;
	long dir_in, retry;
	unsigned long pipe;
	unsigned long status;
	retry = 5;
	dir_in = US_DIRECTION(srb->cmd[0]);
	if(dir_in)
		pipe = usb_rcvbulkpipe(us->pusb_dev, (long)us->ep_in);
	else
		pipe = usb_sndbulkpipe(us->pusb_dev, (long)us->ep_out);
	while(retry--)
	{
		DEBUG(("CBI gets a command: Try %ld", 5 - retry));
#ifdef USB_STOR_DEBUG
		usb_show_srb(srb);
#endif
		/* let's send the command via the control pipe */
		result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev , 0),
		 US_CBI_ADSC, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, us->ifnum, srb->cmd, srb->cmdlen, USB_CNTL_TIMEOUT * 5);
		DEBUG(("CB_transport: control msg returned %ld, status %lx", result, us->pusb_dev->status));
		/* check the return code for the command */
		if(result < 0)
		{
			if(us->pusb_dev->status & USB_ST_STALLED)
			{
				status = us->pusb_dev->status;
				DEBUG((" stall during command found, clear pipe"));
				usb_clear_halt(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0));
				us->pusb_dev->status = status;
			}
			DEBUG((" error during command %02x Stat = %lx", srb->cmd[0], us->pusb_dev->status));
			return result;
		}
		/* transfer the data payload for this command, if one exists*/
		DEBUG(("CB_transport: control msg returned %ld, direction is %s to go 0x%lx", result, dir_in ? "IN" : "OUT", srb->datalen));
		if(srb->datalen)
		{
			result = us_one_transfer(us, pipe, (char *)srb->pdata, srb->datalen);
			DEBUG(("CBI attempted to transfer data, result is %ld status %lx, len %d", result, us->pusb_dev->status, us->pusb_dev->act_len));
			if(!(us->pusb_dev->status & USB_ST_NAK_REC))
				break;
		} /* if(srb->datalen) */
		else
			break;
	}
	/* return result */
	return result;
}

long
usb_stor_CBI_get_status(ccb *srb, struct us_data *us)
{
	long timeout;
	us->ip_wanted = 1;
	usb_submit_int_msg(us->pusb_dev, us->irqpipe, (void *) &us->ip_data, us->irqmaxp, us->irqinterval);
	timeout = 1000;
	while(timeout--)
	{
		if((volatile long *) us->ip_wanted == 0)
			break;
		mdelay(10);
	}
	if(us->ip_wanted)
	{
		DEBUG(("Did not get interrupt on CBI"));
		us->ip_wanted = 0;
		return USB_STOR_TRANSPORT_ERROR;
	}
	DEBUG(("Got interrupt data 0x%x, transfered %ld status 0x%lx", us->ip_data, us->pusb_dev->irq_act_len, us->pusb_dev->irq_status));
	/* UFI gives us ASC and ASCQ, like a request sense */
	if(us->subclass == US_SC_UFI)
	{
		if(srb->cmd[0] == SCSI_REQ_SENSE || srb->cmd[0] == SCSI_INQUIRY)
			return USB_STOR_TRANSPORT_GOOD; /* Good */
		else if(us->ip_data)
			return USB_STOR_TRANSPORT_FAILED;
		else
			return USB_STOR_TRANSPORT_GOOD;
	}
	/* otherwise, we interpret the data normally */
	switch(us->ip_data)
	{
		case 0x0001: return USB_STOR_TRANSPORT_GOOD;
		case 0x0002: return USB_STOR_TRANSPORT_FAILED;
		default: return USB_STOR_TRANSPORT_ERROR;
	}
	return USB_STOR_TRANSPORT_ERROR;
}

#define USB_TRANSPORT_UNKNOWN_RETRY 5
#define USB_TRANSPORT_NOT_READY_RETRY 10

/* clear a stall on an endpoint - special for BBB devices */
long
usb_stor_BBB_clear_endpt_stall(struct us_data *us, __u8 endpt)
{
	long result;
	/* ENDPOINT_HALT = 0, so set value to 0 */
	result = usb_control_msg(us->pusb_dev, usb_sndctrlpipe(us->pusb_dev, 0),
	 			USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, endpt, 0, 0, USB_CNTL_TIMEOUT * 5);
	return result;
}

long
usb_stor_BBB_transport(ccb *srb, struct us_data *us)
{
	long result, retry;
	long dir_in;
	long actlen, data_actlen;
	unsigned long pipe, pipein, pipeout;
#ifdef BBB_XPORT_TRACE
	unsigned char *ptr;
	long idx;
#endif
	umass_bbb_csw_t csw[sizeof(umass_bbb_csw_t)];
	dir_in = US_DIRECTION(srb->cmd[0]);
	/* COMMAND phase */
	DEBUG(("COMMAND phase"));
	result = usb_stor_BBB_comdat(srb, us);
	if(result < 0)
	{
		c_conws("failed to send CBW\r\n");
		DEBUG(("failed to send CBW status %ld", us->pusb_dev->status));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
        if (!(us->flags & USB_READY))
                mdelay(20);
	pipein = usb_rcvbulkpipe(us->pusb_dev, (long)us->ep_in);
	pipeout = usb_sndbulkpipe(us->pusb_dev, (long)us->ep_out);
	/* DATA phase + error handling */
	data_actlen = 0;
	/* no data, go immediately to the STATUS phase */
	if(srb->datalen == 0)
		goto st;
	DEBUG(("DATA phase"));
	if(dir_in)
		pipe = pipein;
	else
		pipe = pipeout;
	
	result = usb_bulk_msg(us->pusb_dev, pipe, srb->pdata, srb->datalen, &data_actlen, USB_CNTL_TIMEOUT * 5, 0);
	
	/* special handling of STALL in DATA phase */
	if((result < 0) && (us->pusb_dev->status & USB_ST_STALLED))
	{
		DEBUG(("DATA:stall"));
		/* clear the STALL on the endpoint */
		result = usb_stor_BBB_clear_endpt_stall(us, dir_in ? us->ep_in : us->ep_out);
		if(result >= 0)
			/* continue on to STATUS phase */
			goto st;
	}
	if(result < 0)
	{
		c_conws("usb_bulk_msg error status\r\n");
		DEBUG(("usb_bulk_msg error status %ld", us->pusb_dev->status));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
#ifdef BBB_XPORT_TRACE
	char build_str[32];
	char buf[data_actlen * 16];

	sprintf(buf, sizeof(buf),"\0");
	for(idx = 0; idx < data_actlen; idx++)
	{
		sprintf(build_str, sizeof(build_str), "pdata[%ld] 0x%x ", idx, srb->pdata[idx]);
		strcat(buf, build_str);
	}	
	DEBUG((buf));
#endif
	/* STATUS phase + error handling */
st:
	retry = 0;
again:
	DEBUG(("STATUS phase"));
	result = usb_bulk_msg(us->pusb_dev, pipein, csw, UMASS_BBB_CSW_SIZE, &actlen, USB_CNTL_TIMEOUT*5, 0);
	/* special handling of STALL in STATUS phase */

	if((result < 0) && (retry < 1) && (us->pusb_dev->status & USB_ST_STALLED))
	{
		DEBUG(("STATUS:stall"));
		/* clear the STALL on the endpoint */
		result = usb_stor_BBB_clear_endpt_stall(us, us->ep_in);
		if(result >= 0 && (retry++ < 1))
			/* do a retry */
			goto again;
	}

	if(result < 0)
	{
		c_conws("usb_bulk_msg error status2\r\n");
		DEBUG(("usb_bulk_msg error status %ld", us->pusb_dev->status));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
#ifdef BBB_XPORT_TRACE
	unsigned char buf2[UMASS_BBB_CSW_SIZE * 16];

	sprintf(buf2, sizeof(buf2),"\0");
	ptr = (unsigned char *)csw;
	for(idx = 0; idx < UMASS_BBB_CSW_SIZE; idx++)
	{
		sprintf(build_str, sizeof(build_str), "ptr[%ld] 0x%x ", idx, ptr[idx]);
		strcat(buf2, build_str);
	}
	DEBUG((buf2));
#endif
	/* misuse pipe to get the residue */
	pipe = le2cpu32(csw->dCSWDataResidue);
	if(pipe == 0 && srb->datalen != 0 && srb->datalen - data_actlen != 0)
		pipe = srb->datalen - data_actlen;
	if(CSWSIGNATURE != le2cpu32(csw->dCSWSignature))
	{
		c_conws("bad signature\r\n");
		DEBUG(("!CSWSIGNATURE"));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
	else if((CBWTag - 1) != le2cpu32(csw->dCSWTag))
	{
		c_conws("bad tag\r\n");
		DEBUG(("!Tag"));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
	else if(csw->bCSWStatus > CSWSTATUS_PHASE)
	{
		c_conws("bad phase\r\n");
		DEBUG((">PHASE"));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
	else if(csw->bCSWStatus == CSWSTATUS_PHASE)
	{
		c_conws("bad phase2\r\n");
		DEBUG(("=PHASE"));
		usb_stor_BBB_reset(us);
		return USB_STOR_TRANSPORT_FAILED;
	}
	else if(data_actlen > srb->datalen)
	{
		c_conws("bad transfer\r\n");
		DEBUG(("transferred %dB instead of %ldB", data_actlen, srb->datalen));
		return USB_STOR_TRANSPORT_FAILED;
	}
	else if(csw->bCSWStatus == CSWSTATUS_FAILED)
	{
		c_conws("failure\r\n");
		DEBUG(("FAILED"));
		return USB_STOR_TRANSPORT_FAILED;
	}
	return result;
}

long usb_stor_CB_transport(ccb *srb, struct us_data *us)
{
	long result, status;
	ccb *psrb;
	ccb reqsrb;
	long retry, notready;
	psrb = &reqsrb;
	status = USB_STOR_TRANSPORT_GOOD;
	retry = 0;
	notready = 0;
	/* issue the command */
do_retry:
	result = usb_stor_CB_comdat(srb, us);
	DEBUG(("command / Data returned %ld, status %lx", result, us->pusb_dev->status));
	/* if this is an CBI Protocol, get IRQ */
	if(us->protocol == US_PR_CBI)
	{
		c_conws("CBI GET IRQ\r\n");
		status = usb_stor_CBI_get_status(srb, us);
		/* if the status is error, report it */
		if(status == USB_STOR_TRANSPORT_ERROR)
		{
			c_conws("CBI ERROR\r\n");
			DEBUG((" USB CBI Command Error"));
			return status;
		}
		srb->sense_buf[12] = (unsigned char)(us->ip_data >> 8);
		srb->sense_buf[13] = (unsigned char)(us->ip_data & 0xff);
		if(!us->ip_data)
		{
			/* if the status is good, report it */
			if(status == USB_STOR_TRANSPORT_GOOD)
			{
				DEBUG((" USB CBI Command Good"));
				return status;
			}
		}
	}
	/* do we have to issue an auto request? */
	/* HERE we have to check the result */
	if((result < 0) && !(us->pusb_dev->status & USB_ST_STALLED))
	{
		c_conws("CBI STALLED\r\n");
		DEBUG(("ERROR %lx", us->pusb_dev->status));
		us->transport_reset(us);
		return USB_STOR_TRANSPORT_ERROR;
	}
	if((us->protocol == US_PR_CBI) && ((srb->cmd[0] == SCSI_REQ_SENSE) || (srb->cmd[0] == SCSI_INQUIRY)))
	{
		c_conws("CBI NO REQ\r\n");
		/* do not issue an autorequest after request sense */
		DEBUG(("No auto request and good"));
		return USB_STOR_TRANSPORT_GOOD;
	}
	/* issue an request_sense */
	memset(&psrb->cmd[0], 0, 12);
	psrb->cmd[0] = SCSI_REQ_SENSE;
	psrb->cmd[1] = srb->lun << 5;
	psrb->cmd[4] = 18;
	psrb->datalen = 18;
	psrb->pdata = &srb->sense_buf[0];
	psrb->cmdlen = 12;
	/* issue the command */
	result = usb_stor_CB_comdat(psrb, us);
	DEBUG(("auto request returned %ld", result));
	/* if this is an CBI Protocol, get IRQ */
	if(us->protocol == US_PR_CBI)
		status = usb_stor_CBI_get_status(psrb, us);
	if((result < 0) && !(us->pusb_dev->status & USB_ST_STALLED))
	{
		DEBUG((" AUTO REQUEST ERROR %ld", us->pusb_dev->status));
		c_conws("CBI AUTO REQ ERROR\r\n");
		return USB_STOR_TRANSPORT_ERROR;
	}
	DEBUG(("autorequest returned 0x%02x 0x%02x 0x%02x 0x%02x", srb->sense_buf[0], srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]));
	/* Check the auto request result */
	if((srb->sense_buf[2] == 0) && (srb->sense_buf[12] == 0) && (srb->sense_buf[13] == 0))
	{
		/* ok, no sense */
		return USB_STOR_TRANSPORT_GOOD;
	}
	/* Check the auto request result */
	switch(srb->sense_buf[2])
	{
		case 0x01:
			/* Recovered Error */
			return USB_STOR_TRANSPORT_GOOD;
		case 0x02: /* Not Ready */
			if(notready++ > USB_TRANSPORT_NOT_READY_RETRY)
			{
				DEBUG(("cmd 0x%02x returned 0x%02x 0x%02x 0x%02x 0x%02x (NOT READY)", srb->cmd[0], srb->sense_buf[0], srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]));
				c_conws("CBI NOT READY ERROR\r\n");
				return USB_STOR_TRANSPORT_FAILED;
			}
			else
			{
				mdelay(100);
				goto do_retry;
			}
			break;
		default:
			if(retry++ > USB_TRANSPORT_UNKNOWN_RETRY)
			{
				DEBUG(("cmd 0x%02x returned 0x%02x 0x%02x 0x%02x 0x%02x", 
					       srb->cmd[0], srb->sense_buf[0], srb->sense_buf[2],
					       srb->sense_buf[12], srb->sense_buf[13]));
				c_conws("CBI NOT UNKNOWN ERROR\r\n");
				return USB_STOR_TRANSPORT_FAILED;
			}
			else
				goto do_retry;
			break;
	}
	c_conws("CBI TRANSPORT FAILED\r\n");
	return USB_STOR_TRANSPORT_FAILED;
}

static long
usb_inquiry(ccb *srb, struct us_data *ss)
{
	DEBUG(("usb_inquiry()"));  /* GALVEZ: DEBUG */

	long retry, i;
	retry = 5;
	while (retry--)
	{
		memset(&srb->cmd[0], 0, 12);
		srb->cmd[0] = SCSI_INQUIRY;
		srb->cmd[4] = 36;
		srb->datalen = 36;
		srb->cmdlen = 12;
		i = ss->transport(srb, ss);
		DEBUG(("inquiry returns %ld", i));
		if(i == 0)
			break;
	}
	if(!retry)
	{
		DEBUG(("error in inquiry"));
		return -1;
	}
	return 0;
}

static long
usb_request_sense(ccb *srb, struct us_data *ss)
{
	DEBUG(("usb_request_sense()"));  /* GALVEZ: DEBUG */
	char *ptr;
	ptr = (char *)srb->pdata;
	memset(&srb->cmd[0], 0, /*12*/6);	/* GALVEZ: DEBUG: DEFAULT 12 */
	srb->cmd[0] = SCSI_REQ_SENSE;
	srb->cmd[4] = 18;
	srb->datalen = 18;
	srb->pdata = &srb->sense_buf[0];
	srb->cmdlen = /*12*/6;		/* GALVEZ: DEBUG: DEFAULT 12 */
	ss->transport(srb, ss);
	DEBUG(("Request Sense returned %02x %02x %02x", srb->sense_buf[2], srb->sense_buf[12], srb->sense_buf[13]));
	srb->pdata = (unsigned char *)ptr;
	return 0;
}

static long
usb_test_unit_ready(ccb *srb, struct us_data *ss)
{
	long retries = 10;
	DEBUG(("usb_test_unit_ready()"));  /* GALVEZ: DEBUG */
	do
	{
		memset(&srb->cmd[0], 0, 6);	/* GALVEZ: DEBUG: DEFAULT 12 */ 
		srb->cmd[0] = SCSI_TST_U_RDY;
		srb->datalen = 0;
		srb->cmdlen = 6;		/* GALVEZ: DEBUG: DEFAULT 12 */
		if(ss->transport(srb, ss) == USB_STOR_TRANSPORT_GOOD) {
			ss->flags |= USB_READY;
			return 0;
		}
		usb_request_sense(srb, ss);
		if ((srb->sense_buf[2] == 0x02) &&
                    (srb->sense_buf[12] == 0x3a))
                        return -1;
		mdelay(100);
	}
	while(retries--);
	return -1;
}

static long
usb_read_capacity(ccb *srb, struct us_data *ss)
{
	long retry;
	/* XXX retries */
	retry = 3;
	DEBUG(("usb_read_capacity()"));  /* GALVEZ: DEBUG */
	do
	{
		memset(&srb->cmd[0], 0, 12);
		srb->cmd[0] = SCSI_RD_CAPAC;
		srb->datalen = 8;
		srb->cmdlen = 12;
		if(ss->transport(srb, ss) == USB_STOR_TRANSPORT_GOOD)
			return 0;
	}
	while(retry--);
	return -1;
}

static inline long
usb_read_10(ccb *srb, struct us_data *ss, unsigned long start, unsigned short blocks)
{
	memset(&srb->cmd[0], 0, 12);
	srb->cmd[0] = SCSI_READ10;
	srb->cmd[2] = ((unsigned char) (start >> 24)) & 0xff;
	srb->cmd[3] = ((unsigned char) (start >> 16)) & 0xff;
	srb->cmd[4] = ((unsigned char) (start >> 8)) & 0xff;
	srb->cmd[5] = ((unsigned char) (start)) & 0xff;
	srb->cmd[7] = ((unsigned char) (blocks >> 8)) & 0xff;
	srb->cmd[8] = (unsigned char) blocks & 0xff;
	srb->cmdlen = 12;
	DEBUG(("read10: start %lx blocks %x", start, blocks));
	return ss->transport(srb, ss);
}

static inline long
usb_write_10(ccb *srb, struct us_data *ss, unsigned long start, unsigned short blocks)
{
	memset(&srb->cmd[0], 0, 12);
	srb->cmd[0] = SCSI_WRITE10;
	srb->cmd[2] = ((unsigned char) (start >> 24)) & 0xff;
	srb->cmd[3] = ((unsigned char) (start >> 16)) & 0xff;
	srb->cmd[4] = ((unsigned char) (start >> 8)) & 0xff;
	srb->cmd[5] = ((unsigned char) (start)) & 0xff;
	srb->cmd[7] = ((unsigned char) (blocks >> 8)) & 0xff;
	srb->cmd[8] = (unsigned char) blocks & 0xff;
	srb->cmdlen = 12;
	DEBUG(("write10: start %lx blocks %x", start, blocks));
	return ss->transport(srb, ss);
}

#ifdef CONFIG_USB_BIN_FIXUP
/*
 * Some USB storage devices queried for SCSI identification data respond with
 * binary strings, which if output to the console freeze the terminal. The
 * workaround is to modify the vendor and product strings read from such
 * device with proper values (as reported by 'usb info').
 *
 * Vendor and product length limits are taken from the definition of
 * block_dev_desc_t in include/part.h.
 */
static void
usb_bin_fixup(struct usb_device_descriptor descriptor, unsigned char vendor[], unsigned char product[])
{
	const unsigned char max_vendor_len = 40;
	const unsigned char max_product_len = 20;
	if(descriptor.idVendor == 0x0424 && descriptor.idProduct == 0x223a)
	{
		strncpy((char *)vendor, "SMSC", max_vendor_len);
		strncpy((char *)product, "Flash Media Cntrller", max_product_len);
	}
}
#endif /* CONFIG_USB_BIN_FIXUP */

#define USB_MAX_READ_BLK 32	/* Galvez: default 20 */

unsigned long
usb_stor_read(long device, unsigned long blknr, unsigned long blkcnt, void *buffer)
{
	unsigned long start, blks, buf_addr;
	unsigned short smallblks;
	struct us_data *ss;
	struct usb_device *dev;
	long retry;
	ccb srb;

	c_conws("READING\r\n");

	if(blkcnt == 0)
		return 0;

	device &= 0xff;	
	/* Setup  device */
	DEBUG(("usb_read: dev %ld ", device));
        dev = usb_dev_desc[device].priv;
	ss = (struct us_data *)dev->privptr;
	usb_disable_asynch(1); /* asynch transfer not allowed */
	srb.lun = usb_dev_desc[device].lun;
	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;

	DEBUG(("usb_read: dev %ld startblk %lx, blccnt %lx buffer %lx", device, start, blks, buf_addr));

	if (blks * usb_dev_desc[device].blksz > DEFAULT_SECTOR_SIZE) {
		c_conws("OUCH!\r\n");
	}

	do
	{
		c_conws("NOWREADING\r\n");
		/* XXX need some comment here */
		retry = 2;
		srb.pdata = (unsigned char *)buf_addr;
		if(blks > USB_MAX_READ_BLK)
			smallblks = USB_MAX_READ_BLK;
		else
			smallblks = (unsigned short) blks;
retry_it:
		srb.datalen = usb_dev_desc[device].blksz * smallblks;
		srb.pdata = (unsigned char *)buf_addr;
		if(usb_read_10(&srb, ss, start, smallblks))
		{
			c_conws("READ ERROR\r\n");
			DEBUG(("Read ERROR"));
			usb_request_sense(&srb, ss);
			if(retry--)
				goto retry_it;
			blkcnt -= blks;
			break;
		}
		start += smallblks;
		blks -= smallblks;
		buf_addr += srb.datalen;
	}
	while(blks != 0);
	ss->flags &= ~USB_READY;
	DEBUG(("usb_read: end startblk %lx, blccnt %x buffer %lx", start, smallblks, buf_addr));
	usb_disable_asynch(0); /* asynch transfer allowed */
	c_conws("FINREAD\r\n");
	return blkcnt;
}

unsigned long
usb_stor_write(long device, unsigned long blknr, unsigned long blkcnt, const void *buffer)
{
	unsigned long start, blks, buf_addr;
	unsigned short smallblks;
	struct us_data *ss;
	struct usb_device *dev;
	long retry;
	ccb srb;
	
	c_conws("WRITING\r\n");
	if(blkcnt == 0)
		return 0;
	device &= 0xff;
	/* Setup  device */
	DEBUG(("usb_write: dev %ld ", device));
        dev = usb_dev_desc[device].priv;
	ss = (struct us_data *)dev->privptr;
	usb_disable_asynch(1); /* asynch transfer not allowed */
	srb.lun = usb_dev_desc[device].lun;
	buf_addr = (unsigned long)buffer;
	start = blknr;
	blks = blkcnt;

	DEBUG(("usb_write: dev %ld startblk %lx, blccnt %lx buffer %lx", device, start, blks, buf_addr));

	do
	{
		/* XXX need some comment here */
		retry = 2;
		srb.pdata = (unsigned char *)buf_addr;
		if(blks > USB_MAX_READ_BLK)
			smallblks = USB_MAX_READ_BLK;
		else
			smallblks = (unsigned short)blks;
retry_it:
		srb.datalen = usb_dev_desc[device].blksz * smallblks;
		srb.pdata = (unsigned char *)buf_addr;
		
		if(usb_write_10(&srb, ss, start, smallblks))
		{
			DEBUG(("Write ERROR"));
			usb_request_sense(&srb, ss);
			if(retry--)
				goto retry_it;
			blkcnt -= blks;
			break;
		}
		start += smallblks;
		blks -= smallblks;
		buf_addr += srb.datalen;
	}
	while(blks != 0);
	ss->flags &= ~USB_READY;
	DEBUG(("usb_write: end startblk %lx, blccnt %x buffer %lx", start, smallblks, buf_addr));
	usb_disable_asynch(0); /* asynch transfer allowed */

	return blkcnt;
}

/* Probe to see if a new device is actually a Storage device */
long
usb_stor_probe(struct usb_device *dev, unsigned long ifnum, struct us_data *ss)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep_desc;
	long i;
	unsigned long flags = 0;
	long protocol = 0;
	long subclass = 0;

	/* let's examine the device now */
	iface = &dev->config.if_desc[ifnum];
# if 0
	DEBUG(("iVendor 0x%x iProduct 0x%x", dev->descriptor.idVendor, dev->descriptor.idProduct);
	/* this is the place to patch some storage devices */
	if((dev->descriptor.idVendor) == 0x066b && (dev->descriptor.idProduct) == 0x0103)
	{
		DEBUG(("patched for E-USB"));
		protocol = US_PR_CB;
		subclass = US_SC_UFI;	    /* an assumption */
	}
# endif
	if (dev->descriptor.bDeviceClass != 0 ||
			iface->desc.bInterfaceClass != USB_CLASS_MASS_STORAGE ||
			iface->desc.bInterfaceSubClass < US_SC_MIN ||
			iface->desc.bInterfaceSubClass > US_SC_MAX) {
		/* if it's not a mass storage, we go no further */
		return 0;
	}
	memset(ss, 0, sizeof(struct us_data));
	/* At this point, we know we've got a live one */
	DEBUG(("USB Mass Storage device detected"));
	DEBUG(("Protocol: %x SubClass: %x", iface->desc.bInterfaceProtocol,       /* GALVEZ: DEBUG */
					    iface->desc.bInterfaceSubClass ));
	/* Initialize the us_data structure with some useful info */
	ss->flags = flags;
	ss->ifnum = ifnum;
	ss->pusb_dev = dev;
	ss->attention_done = 0;
	/* If the device has subclass and protocol, then use that.  Otherwise,
	 * take data from the specific interface.
	 */
	if(subclass) {
		ss->subclass = subclass;
		ss->protocol = protocol;
	} else {
		ss->subclass = iface->desc.bInterfaceSubClass;
		ss->protocol = iface->desc.bInterfaceProtocol;
	}
	/* set the handler pointers based on the protocol */
	DEBUG(("Transport: "));
	switch(ss->protocol)
	{
		case US_PR_CB:
			DEBUG(("Control/Bulk"));
			ss->transport = usb_stor_CB_transport;
			ss->transport_reset = usb_stor_CB_reset;
			break;
		case US_PR_CBI:
			DEBUG(("Control/Bulk/Interrupt"));
			ss->transport = usb_stor_CB_transport;
			ss->transport_reset = usb_stor_CB_reset;
			break;
		case US_PR_BULK:
			DEBUG(("Bulk/Bulk/Bulk"));
			ss->transport = usb_stor_BBB_transport;
			ss->transport_reset = usb_stor_BBB_reset;
			break;
		default:
			ALERT(("USB Storage Transport unknown / not yet implemented"));
			return 0;
			break;
	}
	/*
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	DEBUG(("Number of endpoints: %d", iface->desc.bNumEndpoints));
	for (i = 0; i < iface->desc.bNumEndpoints; i++) {
		ep_desc = &iface->ep_desc[i];
		/* is it an BULK endpoint? */
		if ((ep_desc->bmAttributes &
		     USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
			if (ep_desc->bEndpointAddress & USB_DIR_IN)
				ss->ep_in = ep_desc->bEndpointAddress &
						USB_ENDPOINT_NUMBER_MASK;
			else
				ss->ep_out =
					ep_desc->bEndpointAddress &
					USB_ENDPOINT_NUMBER_MASK;
		}
		/* is it an interrupt endpoint? */
		if ((ep_desc->bmAttributes &
		     USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
			ss->ep_int = ep_desc->bEndpointAddress &
						USB_ENDPOINT_NUMBER_MASK;
			ss->irqinterval = ep_desc->bInterval;
		}
	}
	DEBUG(("Endpoints In %d Out %d Int %d", ss->ep_in, ss->ep_out, ss->ep_int));
	/* Do some basic sanity checks, and bail if we find a problem */
	if (usb_set_interface(dev, iface->desc.bInterfaceNumber, 0) ||
	    !ss->ep_in || !ss->ep_out ||
	    (ss->protocol == US_PR_CBI && ss->ep_int == 0)) {
		DEBUG(("Problems with device"));
		return 0;
	}
	/* set class specific stuff */
	/* We only handle certain protocols.  Currently, these are
	 * the only ones.
	 * The SFF8070 accepts the requests used in u-boot
	 */
	if(ss->subclass != US_SC_UFI && ss->subclass != US_SC_SCSI && ss->subclass != US_SC_8070)
	{
		DEBUG(("Sorry, protocol %d not yet supported.", ss->subclass));
		ALERT(("Sorry, protocol %d not yet supported.", ss->subclass));
		return 0;
	}
	if(ss->ep_int)
	{
		/* we had found an interrupt endpoint, prepare irq pipe
		 * set up the IRQ pipe and handler
		 */
		ss->irqinterval = (ss->irqinterval > 0) ? ss->irqinterval : 255;
		ss->irqpipe = usb_rcvintpipe(ss->pusb_dev, (long)ss->ep_int);
		ss->irqmaxp = usb_maxpacket(dev, ss->irqpipe);
		dev->irq_handle = usb_stor_irq;
	}
	dev->privptr = (void *)ss;
	return 1;
}

long
usb_stor_get_info(struct usb_device *dev, struct us_data *ss, block_dev_desc_t *dev_desc)
{
	unsigned char perq, modi;
	unsigned long cap[2];
	unsigned char usb_stor_buf[36];
	unsigned long *capacity, *blksz;
	ccb pccb;
	DEBUG(("usb_stor_get_info()"));
	/* for some reasons a couple of devices would not survive this reset */
	if(
	 /* Sony USM256E */
	 (dev->descriptor.idVendor == 0x054c && dev->descriptor.idProduct == 0x019e)
	 /* USB007 Mini-USB2 Flash Drive */
	 || (dev->descriptor.idVendor == 0x066f && dev->descriptor.idProduct == 0x2010)
	 /* SanDisk Corporation Cruzer Micro 20044318410546613953 */
	 || (dev->descriptor.idVendor == 0x0781 && dev->descriptor.idProduct == 0x5151)
	 /* SanDisk Corporation U3 Cruzer Micro 1/4GB Flash Drive 000016244373FFB4 */
	 || (dev->descriptor.idVendor == 0x0781 && dev->descriptor.idProduct == 0x5406)
	)
	{
		DEBUG(("usb_stor_get_info: skipping RESET.."));
	}
	else
		ss->transport_reset(ss);

	pccb.pdata = usb_stor_buf;
	dev_desc->priv = dev;
	dev_desc->target = dev->devnum;
	pccb.lun = dev_desc->lun;
	DEBUG(("address %d", dev_desc->target));
	if(usb_inquiry(&pccb, ss)) {
		c_conws("INQUIRY FAILED\n\r");
		return -1;
	}
	perq = usb_stor_buf[0];
	modi = usb_stor_buf[1];
	if((perq & 0x1f) == 0x1f) {
		c_conws("UNKNOWN DEVICE\n\r");
		/* skip unknown devices */
		return 0;
	}
	if((modi&0x80) == 0x80)
		/* drive is removable */
		dev_desc->removable = 1;
	memcpy(&dev_desc->vendor[0], &usb_stor_buf[8], 8);
	memcpy(&dev_desc->product[0], &usb_stor_buf[16], 16);
	memcpy(&dev_desc->revision[0], &usb_stor_buf[32], 4);
	dev_desc->vendor[8] = 0;
	dev_desc->product[16] = 0;
	dev_desc->revision[4] = 0;
	DEBUG(("Vendor: %s Rev: %s Prod: %s", dev_desc->vendor, dev_desc->revision, dev_desc->product)); /* Galvez debug temp */
#ifdef CONFIG_USB_BIN_FIXUP
	usb_bin_fixup(dev->descriptor, (uchar *)dev_desc->vendor, (uchar *)dev_desc->product);
#endif /* CONFIG_USB_BIN_FIXUP */
	DEBUG(("ISO Vers %x, Response Data %x", usb_stor_buf[2], usb_stor_buf[3]));
	if(usb_test_unit_ready(&pccb, ss))
	{
		DEBUG(("Device NOT ready\r\n   Request Sense returned %02x %02x %02x", pccb.sense_buf[2], pccb.sense_buf[12], pccb.sense_buf[13]));
		c_conws("DEVICE NOT READY\n\r");
		if(dev_desc->removable == 1)
		{
			c_conws("DEVICE REMOVEABLE\n\r");
			dev_desc->type = perq;
			return 1;
		}
		return 0;
	}
	pccb.pdata = (unsigned char *)&cap[0];
	memset(pccb.pdata, 0, 8);
	if(usb_read_capacity(&pccb, ss) != 0)
	{
		DEBUG(("READ_CAP ERROR"));
		cap[0] = 2880;
		cap[1] = 0x200;
	}
	ss->flags &= ~USB_READY;
	DEBUG(("Read Capacity returns: 0x%lx, 0x%lx", cap[0], cap[1]));
# if 0
	if(cap[0] > (0x200000 * 10)) /* greater than 10 GByte */
		cap[0] >>= 16;
# endif
	cap[0] = cpu2be32(cap[0]);
	cap[1] = cpu2be32(cap[1]);
	/* this assumes bigendian! */
	cap[0] += 1;
	capacity = &cap[0];
	blksz = &cap[1];
	DEBUG(("Capacity = 0x%lx, blocksz = 0x%lx", *capacity, *blksz));
	dev_desc->lba = *capacity;
	dev_desc->blksz = *blksz;
	dev_desc->type = perq;
	DEBUG((" address %d", dev_desc->target));
	DEBUG(("partype: %d", dev_desc->part_type));
	//init_part(dev_desc);
	DEBUG(("partype: %d", dev_desc->part_type));
	return 1;
}


void
usb_stor_eject(long device)
{
	if (usb_stor[device].pusb_dev->devnum == usb_dev_desc[device].target)
	{
		memset(&usb_dev_desc[device], 0, sizeof(block_dev_desc_t));
		usb_dev_desc[device].target = 0xff;
		usb_dev_desc[device].if_type = IF_TYPE_USB;
		usb_dev_desc[device].dev = device;
		usb_dev_desc[device].part_type = PART_TYPE_UNKNOWN;
		usb_dev_desc[device].block_read = usb_stor_read;
		usb_dev_desc[device].block_write = usb_stor_write;
	}
	else
	{
		DEBUG(("USB mass storage device was already disconnected"));
		return;
	}

	long idx;
	for (idx = 1; idx <= bios_part[device].partnum; idx++)
	{	
		uninstall_usb_stor(bios_part[device].biosnum[idx - 1]);
	}
	bios_part[device].partnum = 0;

	ALERT(("USB Storage Device disconnected: (%ld) %s", usb_stor[device].pusb_dev->devnum,
							    usb_stor[device].pusb_dev->prod));
}


/*******************************************************************************
 * 
 * 
 */
void
usb_storage_disconnect(struct usb_device *dev)
{
	long i;
	for (i = 0; i < USB_MAX_STOR_DEV; i++)
	{
		if (dev->devnum == usb_dev_desc[i].target)
		{
			memset(&usb_dev_desc[i], 0, sizeof(block_dev_desc_t));
			usb_dev_desc[i].target = 0xff;
			usb_dev_desc[i].if_type = IF_TYPE_USB;
			usb_dev_desc[i].dev = i;
			usb_dev_desc[i].part_type = PART_TYPE_UNKNOWN;
			usb_dev_desc[i].block_read = usb_stor_read;
			usb_dev_desc[i].block_write = usb_stor_write;
			break;
		}
	}

	if (i == USB_MAX_STOR_DEV) 
	{
		/* Probably the device has been already disconnected by software (XHEject) */
		DEBUG(("USB mass storage device was already disconnected"));
		return;
	}

	long idx;
	for (idx = 1; idx <= bios_part[i].partnum; idx++)
	{	
		uninstall_usb_stor(bios_part[i].biosnum[idx - 1]);
	}
	bios_part[i].partnum = 0;

	ALERT(("USB Storage Device disconnected: (%ld) %s", dev->devnum, dev->prod));

}


/*******************************************************************************
 * 
 * 
 */
long
usb_storage_probe(struct usb_device *dev)
{
	long r;	
	long i;
	
	if(dev == NULL)
		return -1;
	
	c_conws("PROBING STORAGE DEVICE\r\n");
	for (i = 0; i < USB_MAX_STOR_DEV; i++)
	{
		if (usb_dev_desc[i].target == 0xff)
		{
			break;
		}
	}
	c_conws("PROBING STORAGE DEVICE2\r\n");

	/* if storage device */
	if(i == USB_MAX_STOR_DEV)
	{
			ALERT(("Max USB Storage Device reached: %ld stopping", USB_MAX_STOR_DEV));
			return -1;
	}

	usb_disable_asynch(1); /* asynch transfer not allowed */
	c_conws("PROBING STORAGE DEVICE3\r\n");

	if(!usb_stor_probe(dev, 0, &usb_stor[i])) {
		usb_disable_asynch(0); /* asynch transfer allowed */
		c_conws("NOT A STORAGE DEVICE\r\n");
		return -1; /* It's not a storage device */
	}
	c_conws("PROBING STORAGE DEVICE4\r\n");

	/* ok, it is a storage devices
	 * get info and fill it in
	 */
	if(!usb_stor_get_info(dev, &usb_stor[i], &usb_dev_desc[i])) {
		usb_disable_asynch(0); /* asynch transfer allowed */
		c_conws("FAILED TO GET STORAGE INFO\r\n");
		return -1;
	}
	c_conws("PROBING STORAGE DEVICE5\r\n");
	
	usb_disable_asynch(0); /* asynch transfer allowed */

	long dev_num = i;
	block_dev_desc_t *stor_dev;

	stor_dev = usb_stor_get_dev(dev_num);
	long part_num = 1;
	unsigned long part_type, part_offset, part_size;
	
	c_conws("REGISTERING FAT DEVICE\r\n");
	/* Now find partitions in this storage device */	
	while (!fat_register_device(stor_dev, part_num, &part_type, 
				    &part_offset, &part_size))
	{
		c_conws("INSTALLING PARTITION\r\n");
		/* install partition */
		r = install_usb_stor(dev_num, part_type, part_offset, 
				     part_size, stor_dev->vendor, 
				     stor_dev->revision, stor_dev->product);
		if (r == -1)
			c_conws("unable to install storage device\r\n");
		else
		{
			/* inform the kernel about media change */
			c_conws("PARTITION INSTALL SUCCESS!\r\n");
#ifdef TOSONLY
			(void)Mediach(r);
#else
			changedrv(r);
#endif
			bios_part[dev_num].biosnum[part_num - 1] = r;
			bios_part[dev_num].partnum = part_num;
		}	
		part_num++;
	}
	return 0;
}

void
usb_storage_init(void)
{
	unsigned char i;

	for(i = 0; i < USB_MAX_STOR_DEV; i++)
	{
		memset(&usb_dev_desc[i], 0, sizeof(block_dev_desc_t));
		usb_dev_desc[i].target = 0xff;
		usb_dev_desc[i].if_type = IF_TYPE_USB;
		usb_dev_desc[i].dev = i;
		usb_dev_desc[i].part_type = PART_TYPE_UNKNOWN;
		usb_dev_desc[i].block_read = usb_stor_read;
		usb_dev_desc[i].block_write = usb_stor_write;
	}
}

#ifdef TOSONLY
/* cookie jar definition
 */

struct cookie
{
        long tag;
        long value;
};

#define _USB 0x5f555342L
#define CJAR ((struct cookie **) 0x5a0)

static long
get_cookie (void)
{
	struct cookie *cjar = *CJAR;
	api = NULL;

	while (cjar->tag)
	{
		if (cjar->tag == _USB)
		{
			api = (struct usb_module_api *)cjar->value;

			return 0;
		}

		cjar++;
	}

	return -1;
}
#endif

#ifdef TOSONLY
int init(void);
int
init (void)
#else
long _cdecl	init			(struct kentry *, struct usb_module_api *, long, long);
long _cdecl
init (struct kentry *k, struct usb_module_api *uapi, long arg, long reason)
#endif
{
	long ret;

#ifndef TOSONLY
	kentry	= k;
	api	= uapi;

	if (check_kentry_version())
		return -1;
#endif

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

#ifdef TOSONLY
	/* GET _USB COOKIE to REGISTER */
	Supexec(get_cookie);
	if (!api) {
		(void)Cconws("STORAGE failed to get _USB cookie\r\n");
		return -1;
	}
#endif

	usb_storage_init();

	ret = udd_register(&storage_uif, &storage_driver);
	
	if (ret)
	{
		DEBUG (("%s: udd register failed!", __FILE__));
		return 1;
	}

	DEBUG (("%s: udd register ok", __FILE__));

#ifdef TOSONLY
	Ptermres(_PgmSize + 65536,0);
#endif

	return 0;
}

