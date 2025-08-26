/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2014 Thorsten Otto <admin @ tho-otto.de>
 * All rights reserved.
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
 *
 */

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/stat.h"
# include "mint/arch/nf_ops.h"


enum {
	GET_VERSION = 0,	/* no parameters, return NFAPI_VERSION in d0 */
	NFCD_OPEN,
	NFCD_CLOSE,
	NFCD_READ,
	NFCD_WRITE,
	NFCD_SEEK,
	NFCD_STATUS,
	NFCD_IOCTL,
	NFCD_UNDEF8,
	NFCD_UNDEF9,
	NFCD_UNDEF10,
	NFCD_STARTAUDIO,
	NFCD_STOPAUDIO,
	NFCD_SETSONGTIME,
	NFCD_GETTOC,
	NFCD_DISCINFO,

	NFCD_DRIVESMASK
};

#define METADOS_BOSDEVICE_NAMELEN	32

typedef struct {
	void *next;
	unsigned long attrib;
	unsigned short phys_letter;
	unsigned short dma_channel;
	unsigned short sub_device;
	void /* metados_bosfunctions_t */ *functions;
	unsigned short status;
	unsigned long reserved[2];
	char name[METADOS_BOSDEVICE_NAMELEN];
} metados_bosheader_t;

typedef struct {
	char *name;
	unsigned long reserved[3];
} metaopen_t;

/*
 * debugging stuff
 */

# if 0
# define DEV_DEBUG	1
# endif


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	1
# define VER_STATUS

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p CDROM native feature output device version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"½ May   7 2014 by Thorsten Otto <admin at tho-otto.de>\r\n" \
	"½                 build date " MSG_BUILDDATE ".\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"


/*
 * default settings
 */

# define DEVICE_NAME	"u:\\dev\\nfcdrom%d"

# define CD_FRAMESIZE 2048

/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * device driver
 */

static long _cdecl	xopen		(FILEPTR *f);
static long _cdecl	xwrite		(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	xread		(FILEPTR *f, char *buf, long bytes);
static long _cdecl	xlseek		(FILEPTR *f, long where, int whence);
static long _cdecl	xioctl		(FILEPTR *f, int mode, void *buf);
static long _cdecl	xdatime		(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	xclose		(FILEPTR *f, int pid);
static long _cdecl	xselect		(FILEPTR *f, long proc, int mode);
static void _cdecl	xunselect	(FILEPTR *f, long proc, int mode);

static DEVDRV devtab =
{
	xopen,
	xwrite, xread, xlseek, xioctl, xdatime,
	xclose,
	xselect, xunselect,
	xwrite, xread
};


/*
 * debugging stuff
 */

# ifdef DEV_DEBUG
#  define DEBUG(x)	KERNEL_DEBUG x
#  define TRACE(x)	KERNEL_TRACE x
#  define ALERT(x)	KERNEL_ALERT x
# else
#  define DEBUG(x)
#  define TRACE(x)
#  define ALERT(x)	KERNEL_ALERT x
# endif

/* CDROM feature ID value fetched by nf_ops->get_id() */
static long nf_cdrom_id = 0;
/* Cache for the nf_ops->call() function pointer */
static long (*nf_call)(long id, ...);

#define NFCDROM(a)      (nf_cdrom_id + a)

static unsigned long drives_mask;

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

/* END global data & access implementation */
/****************************************************************************/


#if KERNEL != 1
#define boot_printf(fmt, ...) \
	ksprintf(buf, fmt, __VA_ARGS__); \
	c_conws(buf)
#define boot_print(str) c_conws(str)
#endif


DEVDRV * _cdecl init_xdd(struct kerinfo *k)
{
	struct dev_descr dev_descriptor =
	{
		&devtab,
		0,
		0,
		NULL,
		sizeof(DEVDRV), /* drvsize */
		S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
		NULL,
		0,
		0
	};
	long r;
	int i;
	int drv;
	int keep = 0;
	char device_name[sizeof(DEVICE_NAME) + 4];
	kernel = k;
#if KERNEL != 1
	char buf[512];
#endif

	boot_print(MSG_BOOT);
	boot_print(MSG_GREET);

	DEBUG(("%s: enter init", __FILE__));

	if (!kernel->nf_ops)
	{
		boot_print("\r\nNative features not supported!\r\n");
	} else
	{
		nf_cdrom_id = kernel->nf_ops->get_id( "CDROM" );
		if (nf_cdrom_id == 0)
		{
			boot_print("\r\nCDROM native feature not supported!\r\n");
		} else
		{
			nf_call = kernel->nf_ops->call;
		
			drives_mask = nf_call(NFCDROM(NFCD_DRIVESMASK));
			boot_print(" Host drives present:\r\n");
			drv = 0;
			for (i = 0; i < NUM_DRIVES; i++)
			{
				if (drives_mask & (1L << i))
				{
					boot_printf(" - %c: ", DriveToLetter(i));
					ksprintf(device_name, DEVICE_NAME, drv);
					/*
					 * we encode the logical drive number in the upper byte,
					 * and the physical drive number in the lower byte.
					 * The dinfo field is copied to the private field of the bios
					 * device driver, which in turn is copied to the aux field
					 * of the file cookie upon opening such a device.
					 */
					dev_descriptor.dinfo = (drv << 8) | i;
					
					r = d_cntl(DEV_INSTALL, device_name, (long) &dev_descriptor);
					/*
					 * Dcntl(DEV_INSTALL) returns kerinfo ptr on success,
					 * and zero or error code for failure
					 */
					if (r <= 0)
					{
						boot_printf("(Dcntl(DEV_INSTALL, %s) failed)!\r\n", device_name);
					} else
					{
						keep = 1;
						drv++;
						boot_printf("%s\r\n", device_name);
					}
				}
			}
			boot_print("\r\n");
			if (drives_mask == 0)
			{
				boot_print("\r\nno CDROM drives found!\r\n");
			}
		}
	}
		
	if (keep == 0)
	{
		boot_print("\7\r\nSorry, nfcdrom NOT installed!\r\n\r\n");
		return NULL;
	}

	DEBUG(("%s: init ok", __FILE__));
	
	/*
	 * Tell kernel success, and that we already installed the device(s)
	 */
	return (DEVDRV *) 1;
}


static void setup_metadev(FILEPTR *f, metados_bosheader_t *device)
{
	int drv;
	
	/*
	 * fetch the (physical) drive number. This is the only information
	 * needed by the host driver part to identify the device
	 */
	drv = f->fc.aux & 0xff;
	memset(device, 0, sizeof(*device));
	device->phys_letter = DriveToLetter(drv);
}

/*
 *	the device driver routines
 */

static long _cdecl xopen(FILEPTR *f)
{
	metados_bosheader_t device;
	metaopen_t metaopen;

	setup_metadev(f, &device);
	memset(&metaopen, 0, sizeof(metaopen));
	return nf_call(NFCDROM(NFCD_OPEN), &device, &metaopen);
}

static long _cdecl xwrite(FILEPTR *f, const char *buf, long bytes)
{
	return EROFS;
}


static long _cdecl xread(FILEPTR *f, char *buf, long bytes)
{
	char secbuf[CD_FRAMESIZE];
	long nread = 0;
	long first;
	long ret;
	long off;
	metados_bosheader_t device;

	setup_metadev(f, &device);

	if ((off = (f->pos % CD_FRAMESIZE)) != 0 && bytes != 0)
	{
		first = f->pos / CD_FRAMESIZE;
		ret = nf_call(NFCDROM(NFCD_READ), &device, secbuf, first, 1l);
		if (ret < 0)
			return ret;
		ret = CD_FRAMESIZE - off;
		if (ret > bytes)
			ret = bytes;
		memcpy(buf, &secbuf[off], ret);
		nread += ret;
		buf += ret;
		f->pos += ret;
		bytes -= ret;
	}
	if (bytes >= CD_FRAMESIZE)
	{
		first = f->pos / CD_FRAMESIZE;
		ret = nf_call(NFCDROM(NFCD_READ), &device, buf, first, bytes / CD_FRAMESIZE);
		if (ret < 0)
			return ret;
		ret = (bytes / CD_FRAMESIZE) * CD_FRAMESIZE;
		if (ret > bytes)
			ret = bytes;
		nread += ret;
		buf += ret;
		f->pos += ret;
		bytes -= ret;
	}
	if (bytes >= 0)
	{
		first = f->pos / CD_FRAMESIZE;
		ret = nf_call(NFCDROM(NFCD_READ), &device, secbuf, first, 1L);
		if (ret < 0)
			return ret;
		ret = bytes;
		memcpy(buf, secbuf, ret);
		nread += ret;
		buf += ret;
		f->pos += ret;
		bytes -= ret;
	}
	return nread;
}


static long _cdecl xlseek(FILEPTR *f, long where, int whence)
{
	switch (whence)
	{
		case SEEK_SET:				break;
		case SEEK_CUR:	where += f->pos;	break;
		/* case SEEK_END:	where += c->i_size;	break; */
		default:	return EINVAL;
	}

	if ((where < 0) /* || (where > c->i_size) */)
	{
		return EBADARG;
	}

	f->pos = where;

	return where;
}


static long _cdecl xioctl(FILEPTR *f, int mode, void *buf)
{
	metados_bosheader_t device;

	setup_metadev(f, &device);

	switch (mode)
	{
	case FIONREAD:
	case FIONWRITE:
		*((long *) buf) = CD_FRAMESIZE;
		return E_OK;
	}
#if 0
	{
		char cbuf[512];
		ksprintf(cbuf, "xioctl: %lx %4x %lx\n", (long)&device, mode, (long)buf);
		kernel->nf_ops->call(kernel->nf_ops->get_id("NF_STDERR"), cbuf);
	}
#endif
	return nf_call(NFCDROM(NFCD_IOCTL), &device, (long)mode, (long)buf);
}


static long _cdecl xdatime(FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (rwflag)
		return EACCES;

	*timeptr++ = timestamp;
	*timeptr = datestamp;

	return E_OK;
}


static long _cdecl xclose(FILEPTR *f, int pid)
{
	metados_bosheader_t device;

	setup_metadev(f, &device);

	return nf_call(NFCDROM(NFCD_CLOSE), &device);
}


static long _cdecl xselect(FILEPTR *f, long proc, int mode)
{
	/* we're always ready for I/O */
	return 1;
}


static void _cdecl xunselect(FILEPTR *f, long proc, int mode)
{
}
