/*
 * Copyright (c) 2012 David Galvez
 *
 * Parts of this file has been inspired by code typed by:
 * Frank Naumann
 * Petr Stehlik 
 * Vincent Rivière 
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO: Fix XHMiNTInfo
 */

#ifndef TOSONLY
#if 0
# define DEV_DEBUG	1
#endif
#endif

#include "mint/mint.h"
#include "../../global.h"

#include "part.h"
#include "xhdi.h"

/*--- Defines ---*/

#define XHDI_VERSION		0x130
#define MAX_IPL			5
#define XH_TARGET_REMOVABLE	0x02L
#define STRINGLEN		33 /* including termination character */

#ifdef TOSONLY
char *DRIVER_NAME = "TOS USB";
#else
char *DRIVER_NAME = "FreeMiNT USB";
#endif
char *DRIVER_COMPANY = "FreeMiNT list";

#ifndef TOSONLY
# define Sversion()	0x4000		/* this is FreeMiNT's GEMDOS version */
#endif

/*--- External variables ---*/

extern char *drv_version;

/* --- External functions ---*/

extern block_dev_desc_t *usb_stor_get_dev(long);
extern ulong usb_stor_read(long, ulong, ulong, void *);
extern ulong usb_stor_write(long, ulong, ulong, const void *);
extern void usb_stor_eject(long);

/*--- Functions prototypes ---*/

typedef long (*XHDI_HANDLER)(ushort opcode, ...);
extern XHDI_HANDLER usbxhdi;
static XHDI_HANDLER next_handler; /* Next handler installed by XHNewCookie() */

long install_xhdi_driver(void);
long xhdi_handler(ushort stack);

/*--- Global variables ---*/

ulong my_drvbits;
USB_PUN_INFO pun_usb;

/*---Functions ---*/

/*
 * XHDI syscall XHDOSLimits routine
 */
long
sys_XHDOSLimits(ushort which,ulong limit)
{
	static long dl_secsiz, dl_clusts, dl_maxsec, dl_clusts12;
	static int first_time = 1;

	if (first_time)
	{
		ushort version = Sversion();            /* determine GEMDOS version */
		version = (version>>8) | (version<<8);  /* swap to correct order */

		if (version > 0x0040)       /* unknown               */
			version = 0x0000;       /* so force it to lowest */

		if (version < 0x0015)       /* TOS 1.00, 1.02, KAOS TOS */
		{
			dl_secsiz = 8192L;
			dl_clusts = 16383L;
			dl_maxsec = 32767L;     /* max partition size = 256MB approx */
			dl_clusts12 = 2046L;
		}
		else if (version < 0x0030)  /* i.e. TOS 1.04 to TOS 3.06 */
		{
			dl_secsiz = 8192L;
			dl_clusts = 32767L;
			dl_maxsec = 65535L;     /* max partition size = 512MB approx */
			dl_clusts12 = MAX_FAT12_CLUSTERS;
		}
		else                        /* i.e. TOS 4.0x or FreeMiNT */
		{
			dl_secsiz = MAX_LOGSEC_SIZE;
			dl_clusts = 32767L;
			dl_maxsec = 65535L;     /* max partition size = 1024MB approx */
			dl_clusts12 = MAX_FAT12_CLUSTERS;
		}
		first_time = 0;
	}

	if (limit == 0)
	{
		switch (which)
		{
			/* maximal sector size (BIOS level) */
			case XH_DL_SECSIZ:
				return dl_secsiz;

			/* minimal number of FATs */
			case XH_DL_MINFAT:
				return 2L;

			/* maximal number of FATs */
			case XH_DL_MAXFAT:
				return 2L;

			/* sectors per cluster minimal */
			case XH_DL_MINSPC:
				return 2L;

			/* sectors per cluster maximal */
			case XH_DL_MAXSPC:
				return 2L;

			/* maximal number of clusters of a 16 bit FAT */
			case XH_DL_CLUSTS:
				return dl_clusts;

			/* maximal number of sectors */
			case XH_DL_MAXSEC:
				return dl_maxsec;

			/* maximal number of BIOS drives supported by the DOS */
			case XH_DL_DRIVES:
				return MAX_LOGICAL_DRIVE;

			/* maximal clustersize */
			case XH_DL_CLSIZB:
				return dl_secsiz * 2;

			/* maximal (bpb->rdlen * bpb->recsiz / 32) */
			case XH_DL_RDLEN:
				return 1008L;   /* we return the same value as HDDRIVER */

			/* maximal number of clusters of a 12 bit FAT */
			case XH_DL_CLUSTS12:
				return dl_clusts12;

			/* maximal number of clusters of a 32 bit FAT */
			case XH_DL_CLUSTS32:
				return 0L;          /* TOS doesn't support FAT32 */

			/* supported bits in bpb->bflags */
			case XH_DL_BFLAGS:
				return 0x00000001L;
		}
	}

	return ENOSYS;
}

static ushort
XHGetVersion(void)
{
	ushort version = XHDI_VERSION;

	if (next_handler) {
		ushort next_version = (ushort)next_handler(XHGETVERSION);
		if (next_version < version)
			version = next_version;
	}

	return version;
}

static ulong
XHDrvMap(void)
{
	ulong drvmap = my_drvbits;

	if (next_handler)
		drvmap |= next_handler(XHDRVMAP);

	return drvmap;
}

static long
XHNewCookie(ulong newcookie)
{
	if (next_handler)
		return next_handler(XHNEWCOOKIE, newcookie);

	next_handler = (XHDI_HANDLER)newcookie;

	return E_OK;
}

static long
XHInqDev2(ushort drv, ushort *major, ushort *minor, ulong *start, BPB *bpb,
	  ulong *blocks, char *partid)
{
	long pstart = pun_usb.partition_start[drv];
	BPB *myBPB;

	DEBUG(("XHInqDev2(%c:) drv=%d pun %x",
		'A' + drv, drv, pun_usb.pun[drv]));

	if (next_handler) {
		long ret = next_handler(XHINQDEV2, drv, major, minor, start,
					bpb, blocks, partid);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if (pun_usb.pun[drv] & PUN_VALID)
		return ENODEV;

	if (major) {
		*major = (PUN_DEV+PUN_USB) & pun_usb.pun[drv];
		DEBUG(("XHInqDev2() major: %d", *major));
	}
	if (minor)
		*minor = 0;
	if (bpb)
		bpb->recsiz = 0;

	if (!pstart)
		return EBUSY;

	if (start) {
		*start = pstart;
		DEBUG(("XHInqDev2() pstart: %lx", *start));
	}

	myBPB = (&pun_usb.bpb[drv]);

	if (bpb) {
		memcpy(bpb, myBPB, sizeof(BPB));

		DEBUG(("XHInqDev2() BPB"));
		DEBUG(("recsiz:	%d", bpb->recsiz));
		DEBUG(("clsiz:	%d", bpb->clsiz));
		DEBUG(("clsizb:	%d", bpb->clsizb));
		DEBUG(("rdlen:	%d", bpb->rdlen));
		DEBUG(("fsiz:	%d", bpb->fsiz));
		DEBUG(("fatrec:	%d", bpb->fatrec));
		DEBUG(("datrec:	%d", bpb->datrec));
		DEBUG(("numcl:	%d", bpb->numcl));
		DEBUG(("bflags:	%x", bpb->bflags));
	}

	if (blocks) {
		*blocks = pun_usb.psize[drv];
		DEBUG(("XHInqDev2(%c:) blocks=%ld",
			'A' + drv, *blocks));
	}

	if (partid) {
		memcpy(partid, &pun_usb.ptype[drv], sizeof(long));

		if (partid[0] == '\0') /* DOS partitiopn */
			DEBUG(("XHInqDev2(%c:) major=%d, partid=%08lx",
				'A' + drv, *major, *((long *)partid),
				pun_usb.ptype[drv]));
		else
			DEBUG(("XHInqDev2(%c:) major=%d, ID=%c%c%c",
				'A' + drv, *major, partid[0], partid[1],
				partid[2]));
	}

	return E_OK;
}

static long
XHInqDev(ushort drv, ushort *major, ushort *minor, ulong *start, BPB *bpb)
{
	if (next_handler) {
		long ret = next_handler(XHINQDEV, drv, major, minor,
					start, bpb);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if (pun_usb.pun[drv] & PUN_VALID)
		return ENODEV;

	return XHInqDev2(drv, major, minor, start, bpb, NULL, NULL);
}

static long
XHReserve(ushort major, ushort minor, ushort do_reserve, ushort key)
{
	if (next_handler) {
		long ret = next_handler(XHRESERVE, major, minor, do_reserve,
					key);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHLock(ushort major, ushort minor, ushort do_lock, ushort key)
{
	if (next_handler) {
		long ret = next_handler(XHLOCK, major, minor, do_lock, key);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHStop(ushort major, ushort minor, ushort do_stop, ushort key)
{
	if (next_handler) {
		long ret = next_handler(XHSTOP, major, minor, do_stop, key);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHEject(ushort major, ushort minor, ushort do_eject, ushort key)
{
	if (next_handler) {
		long ret = next_handler(XHEJECT, major, minor, do_eject, key);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	/* device number in the USB bus */
	short dev = major & PUN_DEV;

	usb_stor_eject(dev);

	return E_OK;
}

static long
XHInqDriver(ushort dev, char *name, char *version, char *company,
		ushort *ahdi_version, ushort *max_IPL)
{
	if (next_handler) {
		long ret = next_handler(XHSTOP, dev, name, version, company,
					ahdi_version, max_IPL);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if (pun_usb.pun[dev] & PUN_VALID)
		return ENODEV;

	name = DRIVER_NAME;
	version = drv_version;
	company = DRIVER_COMPANY;
	*ahdi_version = pun_usb.version_num;
	*max_IPL = MAX_IPL;

	return E_OK;
}

static long
XHDriverSpecial(ulong key1, ulong key2, ushort subopcode, void *data)
{
	if (next_handler) {
		long ret = next_handler(XHDRIVERSPECIAL, key1, key2, subopcode,
					data);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	return ENOSYS;
}

static long
XHMediumChanged(ushort major, ushort minor)
{
	if (next_handler) {
		long ret = next_handler(XHMEDIUMCHANGED, major, minor);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHMiNTInfo(void *data)
{
	if (next_handler) {
		long ret = next_handler(XHMINTINFO, data);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	return ENOSYS;
}

static long
XHDOSLimits(ushort which, ulong limit)
{
	if (next_handler) {
		long ret = next_handler(XHDOSLIMITS, which, limit);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

#ifdef TOSONLY
	return sys_XHDOSLimits(which,limit);
#else
	return ENOSYS;	/* so FreeMiNT kernel will handle this call */
#endif
}

static long
XHLastAccess(ushort major, ushort minor, ulong *ms)
{
	if (next_handler) {
		long ret = next_handler(XHLASTACCESS, major, minor, ms);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHReaccess(ushort major, ushort minor)
{
	if (next_handler) {
		long ret = next_handler(XHREACCESS, major, minor);
		if (ret != ENOSYS && ret != ENODEV && ret != ENXIO)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return ENOSYS;
}

static long
XHInqTarget2(ushort major, ushort minor, ulong *blocksize, ulong *deviceflags,
		 char *productname, ushort stringlen)
{
	DEBUG(("XHInqTarget2(%d.%d)", major, minor));

	if (next_handler) {
		long ret = next_handler(XHINQTARGET2, major, minor, blocksize,
					deviceflags, productname, stringlen);
		if (ret != ENOSYS && ret != ENODEV)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	{
		short dev = major & PUN_DEV;
		block_dev_desc_t *dev_desc = usb_stor_get_dev(dev);
	
		if (blocksize) {
			*blocksize = dev_desc->blksz;
			DEBUG(("XHInqTarget2(%d.%d) blocksize: %ld",
				major, minor, *blocksize));
		}

		if (deviceflags) {
			*deviceflags = XH_TARGET_REMOVABLE;
			DEBUG(("XHInqTarget2(%d.%d) flags: %08lx",
				major, minor, *deviceflags));
		}

		if (productname) {
			char devName[64];

			DEBUG(("XHInqTarget2(%d.%d) %d", major, minor, dev));

			memset(devName, 0, 64);
			strcat(devName, dev_desc->vendor);
			strcat(devName, " ");
			strcat(devName, dev_desc->product);
			strncpy(productname, devName, stringlen);
		}
	}

	return E_OK;
}

static long
XHInqTarget(ushort major, ushort minor, ulong *blocksize, ulong *deviceflags,
		char *productname)
{
	if (next_handler) {
		long ret = next_handler(XHINQTARGET, major, minor, blocksize,
					deviceflags, productname);
		if (ret != ENOSYS && ret != ENODEV)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	return XHInqTarget2(major, minor, blocksize, deviceflags,
				productname, STRINGLEN);
}

static long
XHGetCapacity(ushort major, ushort minor, ulong *blocks,
		ulong *blocksize)
{
	DEBUG(("XHGetCapacity(%d.%d)\n", major, minor));
	
	if (next_handler) {
		long ret = next_handler(XHGETCAPACITY, major, minor, blocks,
					blocksize);
		if (ret != ENOSYS && ret != ENODEV)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	{
		short dev = major & PUN_DEV;
		block_dev_desc_t *dev_desc = usb_stor_get_dev(dev);

		*blocks = dev_desc->lba;
		*blocksize = dev_desc->blksz;
	}

	return E_OK;
}

static long
XHReadWrite(ushort major, ushort minor, ushort rw,
		ulong sector, ushort count, void *buf)
{
	long ret;

	DEBUG(("XH%s(device=%d.%d, sector=%ld, count=%d, buf=%lx)",
		rw ? "Write" : "Read", major, minor, sector, count, buf));

	if (next_handler) {
		ret = next_handler(XHREADWRITE, major, minor, rw, sector,
				   count, buf);
		if (ret != ENOSYS && ret != ENODEV)
			return ret;
	}

	if ((major & PUN_USB) == 0)
		return ENODEV;

	if (minor != 0)
		return ENODEV;

	if (!count)
		return EERROR;

	/* device number in the USB bus */
	short dev = major & PUN_DEV;

	if (rw & 0x0001) {
		ret = usb_stor_write(dev, sector, (long)count, buf);

		DEBUG(("usb_stor_write() returned %ld", ret));
	}
	else {
		ret = usb_stor_read(dev, sector, (long)count, buf);

		DEBUG(("usb_stor_read() returned %ld", ret));
	}

	if (ret < 0)
		return EERROR;

	return E_OK;
}

long
xhdi_handler(ushort stack)
{
	ushort opcode = stack;

	DEBUG(("XHDI handler, opcode: %d", opcode));

	switch (opcode)
	{
		case XHGETVERSION:
		{
			return XHGetVersion();
		}

		case XHINQTARGET:
		{
			struct XHINQTARGET_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ulong *blocksize;
				ulong *deviceflags;
				char *productname;
			} *args = (struct XHINQTARGET_args *)(&stack);

			return XHInqTarget(args->major, args->minor,
					   args->blocksize, args->deviceflags,
					   args->productname);
		}

		case XHRESERVE:
		{
			struct XHRESERVE_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ushort do_reserve;
				ushort key;
			} *args = (struct XHRESERVE_args *)(&stack);

			return XHReserve(args->major, args->minor,
					 args->do_reserve, args->key);
		}

		case XHLOCK:
		{
			struct XHLOCK_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ushort do_lock;
				ushort key;
			} *args = (struct XHLOCK_args *)(&stack);

			return XHLock(args->major, args->minor,
					  args->do_lock, args->key);
		}

		case XHSTOP:
		{
			struct XHSTOP_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ushort do_stop;
				ushort key;
			} *args = (struct XHSTOP_args *)(&stack);

			return XHStop(args->major, args->minor,
					  args->do_stop, args->key);
		}

		case XHEJECT:
		{
			struct XHEJECT_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ushort do_eject;
				ushort key;
			} *args = (struct XHEJECT_args *)(&stack);

			return XHEject(args->major, args->minor, args->do_eject,
					   args->key);
		}

		case XHDRVMAP:
		{
			return XHDrvMap();
		}

		case XHINQDEV:
		{
			struct XHINQDEV_args
			{
				ushort opcode;
				ushort drv;
				ushort *major;
				ushort *minor;
				ulong *start;
				BPB *bpb;
			} *args = (struct XHINQDEV_args *)(&stack);

			return XHInqDev(args->drv, args->major, args->minor,
					args->start, args->bpb);
		}

		case XHINQDRIVER:
		{
			struct XHINQDRIVER_args
			{
				ushort opcode;
				ushort dev;
				char *name;
				char *version;
				char *company;
				ushort *ahdi_version;
				ushort *maxIPL;
			} *args = (struct XHINQDRIVER_args *)(&stack);

			return XHInqDriver(args->dev, args->name, args->version,
					   args->company, args->ahdi_version,
					   args->maxIPL);
		}

		case XHNEWCOOKIE:
		{
			struct XHNEWCOOKIE_args
			{
				ushort opcode;
				ulong newcookie;
			} *args = (struct XHNEWCOOKIE_args *)(&stack);

			return XHNewCookie(args->newcookie);
		}

		case XHREADWRITE:
		{
			struct XHREADWRITE_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ushort rw;
				ulong sector;
				ushort count;
				void *buf;
			} *args = (struct XHREADWRITE_args *)(&stack);

			return XHReadWrite(args->major, args->minor, args->rw,
					   args->sector, args->count,
					   args->buf);
		}

		case XHINQTARGET2:
		{
			struct XHINQTARGET2_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ulong *blocksize;
				ulong *deviceflags;
				char *productname;
				ushort stringlen;
			} *args = (struct XHINQTARGET2_args *)(&stack);

			return XHInqTarget2(args->major, args->minor,
						args->blocksize, args->deviceflags,
						args->productname, args->stringlen);
		}

		case XHINQDEV2:
		{
			struct XHINQDEV2_args
			{
				ushort opcode;
				ushort drv;
				ushort *major;
				ushort *minor;
				ulong *start;
				BPB *bpb;
				ulong *blocks;
				char *partid;
			} *args = (struct XHINQDEV2_args *)(&stack);

			return XHInqDev2(args->drv, args->major, args->minor,
					 args->start, args->bpb, args->blocks,
					 args->partid);
		}

		case XHDRIVERSPECIAL:
		{
			struct XHDRIVERSPECIAL_args
			{
				ushort opcode;
				ulong key1;
				ulong key2;
				ushort subopcode;
				void *data;
			} *args = (struct XHDRIVERSPECIAL_args *)(&stack);

			return XHDriverSpecial(args->key1, args->key2,
						   args->subopcode, args->data);
		}

		case XHGETCAPACITY:
		{
			struct XHGETCAPACITY_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ulong *blocks;
				ulong *blocksize;
			} *args = (struct XHGETCAPACITY_args *)(&stack);

			return XHGetCapacity(args->major, args->minor,
						 args->blocks, args->blocksize);
		}

		case XHMEDIUMCHANGED:
		{
			struct XHMEDIUMCHANGED_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
			} *args = (struct XHMEDIUMCHANGED_args *)(&stack);

			return XHMediumChanged(args->major, args->minor);
		}

		/* FIXME: After we figure out how to handle this call */
		case XHMINTINFO:
		{
			struct XHMINTINFO_args
			{
				ushort opcode;
				void *data;
			} *args = (struct XHMINTINFO_args *)(&stack);

			return XHMiNTInfo(args->data);
		}

		case XHDOSLIMITS:
		{
			struct XHDOSLIMITS_args
			{
				ushort opcode;
				ushort which;
				ulong limit;
			} *args = (struct XHDOSLIMITS_args *)(&stack);

			return XHDOSLimits(args->which, args->limit);
		}

		case XHLASTACCESS:
		{
			struct XHLASTACCESS_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
				ulong *ms;
			} *args = (struct XHLASTACCESS_args *)(&stack);

			return XHLastAccess(args->major, args->minor, args->ms);
		}

		case XHREACCESS:
		{
			 struct XHREACCESS_args
			{
				ushort opcode;
				ushort major;
				ushort minor;
			} *args = (struct XHREACCESS_args *)(&stack);

			return XHReaccess(args->major, args->minor);
		}

		default:
		{
			return ENOSYS;
		}
	}
}

#ifdef TOSONLY
#define XHDIMAGIC 0x27011992L

typedef long (*cookie_fun)(unsigned short opcode,...);

static long
getcookie (long cookie, long *p_value)
{
	long *cookiejar = *((long **)0x5a0);

	if (!cookiejar) return 0;

	do
	{
		if (cookiejar[0] == cookie)
		{
			if (p_value) *p_value = cookiejar[1];
			return 1;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while (cookiejar[-2]);

	return 0;
}

static cookie_fun
get_fun_ptr (void)
{
	static cookie_fun XHDI = NULL;
	long *magic_test;
	
	getcookie (*((long *)"XHDI"), (long *)&XHDI);

	/* check magic */
		
	magic_test = (long *)XHDI;
	if (magic_test && (magic_test[-1] != XHDIMAGIC))
		XHDI = NULL;
	
	return XHDI;
}

static void
set_cookie (void)
{
	struct cookie *cjar = *CJAR;
	long n = 0;

	while (cjar->tag)
	{
		n++;
		cjar++;
	}

	n++;
	if (n < cjar->value)
	{
		n = cjar->value;
		cjar->tag = *((long *)"XHDI");
		cjar->value = (long)&usbxhdi;

		cjar++;
		cjar->tag = 0L;
		cjar->value = n;
	}
}
#endif

long
install_xhdi_driver(void)
{
#ifndef TOSONLY
	return xhnewcookie(*xhdi_handler);
#else
	long r = 0;
	cookie_fun XHDI = get_fun_ptr ();

	if (XHDI) {
		r = XHDI(XHNEWCOOKIE,*xhdi_handler);
	} else {
		set_cookie();
	}

	return r;
#endif
}
