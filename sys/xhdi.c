/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1998-02-01
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * changes since last version:
 * 
 * 1998-05-25:
 * 
 * - initial revision
 * 
 * known bugs:
 * 
 * todo:
 * 
 */

# include "xhdi.h"
# include "global.h"

# include "arch/xhdi_emu.h"	/* xhdi_emu */
# include "mint/proc.h"

# include "cookie.h"		/* cookie handling */
# include "info.h"		/* messages */
# include "init.h"		/* boot_printf */
# include "k_prot.h"		/* suser */
# include "block_IO.h"		/* bio_sync_all */
# include "proc.h"


/*
 * internal usage
 */

/* dummy routine */
static long
XHDIfail (ushort opcode, ...)
{
	UNUSED (opcode);
	
	return ENOSYS;
}

/* XHDI handler function */
static long (*XHDI)(ushort opcode, ...) = XHDIfail;

static long sys_XHDOSLimits (ushort which, ulong limit);

ushort XHDI_installed = 0;

#define XHGETVERSION    0
#define XHEJECT         5
#define XHDRVMAP        6
#define XHNEWCOOKIE     9
#define XHMINTINFO      16
#define XHDOSLIMITS     17

long
XHDI_init (void)
{
	long r;
	unsigned long t = 0;
	
	r = get_toscookie (COOKIE_XHDI, &t);
	XHDI = (long(*)(unsigned short, ...))t;
	if (!r && XHDI)
	{
		long *magic_test = (long *) XHDI;
		
		magic_test--;
		
# define XHDIMAGIC	0x27011992L
		
		if (*magic_test == XHDIMAGIC)
		{
			/* check version */
			XHDI_installed = 1;
			XHDI_installed = XHGetVersion ();
		}
	}
	
	/* we need at least XHDI 1.10 */
	if (XHDI_installed >= 0x110)
	{
		long tmp;

		/* Tell the underlying XHDI driver of our limits. */
		tmp = sys_XHDOSLimits(XH_DL_SECSIZ, 0);
		XHDOSLimits(XH_DL_SECSIZ, tmp);
		tmp = sys_XHDOSLimits(XH_DL_MINFAT, 0);
		XHDOSLimits(XH_DL_MINFAT, tmp);
		tmp = sys_XHDOSLimits(XH_DL_MAXFAT, 0);
		XHDOSLimits(XH_DL_MAXFAT, tmp);
		tmp = sys_XHDOSLimits(XH_DL_MINSPC, 0);
		XHDOSLimits(XH_DL_MINSPC, tmp);
		tmp = sys_XHDOSLimits(XH_DL_MAXSPC, 0);
		XHDOSLimits(XH_DL_MAXSPC, tmp);
		tmp = sys_XHDOSLimits(XH_DL_CLUSTS, 0);
		XHDOSLimits(XH_DL_CLUSTS, tmp);
		tmp = sys_XHDOSLimits(XH_DL_MAXSEC, 0);
		XHDOSLimits(XH_DL_MAXSEC, tmp);
		tmp = sys_XHDOSLimits(XH_DL_DRIVES, 0);
		XHDOSLimits(XH_DL_DRIVES, tmp);
		tmp = sys_XHDOSLimits(XH_DL_CLSIZB, 0);
		XHDOSLimits(XH_DL_CLSIZB, tmp);
		tmp = sys_XHDOSLimits(XH_DL_RDLEN, 0);
		XHDOSLimits(XH_DL_RDLEN, tmp);
		tmp = sys_XHDOSLimits(XH_DL_CLUSTS12, 0);
		XHDOSLimits(XH_DL_CLUSTS12, tmp);
		tmp = sys_XHDOSLimits(XH_DL_CLUSTS32, 0);
		XHDOSLimits(XH_DL_CLUSTS32, tmp);
		tmp = sys_XHDOSLimits(XH_DL_BFLAGS, 0);
		XHDOSLimits(XH_DL_BFLAGS, tmp);

# ifdef NONBLOCKING_DMA
		r = XHMiNTInfo (XH_MI_SETKERINFO, &kernelinfo);

		boot_printf (MSG_xhdi_present,
			XHDI_installed >> 8, XHDI_installed & 0xff,
			r ? MSG_kerinfo_rejected : MSG_kerinfo_accepted
		);

# else
		boot_printf (MSG_xhdi_present,
			XHDI_installed >> 8, XHDI_installed & 0xff,
			MSG_kerinfo_unused
		);

# endif
		
		*((long *) 0x4c2L) |= XHDrvMap ();
		
		set_toscookie (COOKIE_XHDI, (long) emu_xhdi);
		r = E_OK;
	}
	else
	{
		boot_print (MSG_xhdi_absent);
		
		XHDI = XHDIfail;
		XHDI_installed = 0;
		
		r = ENOSYS;
	}
	
	return r;
}


/*
 * XHDI syscall XHDOSLimits routine
 */

static long
sys_XHDOSLimits (ushort which, ulong limit)
{
	if (limit == 0)
	{
		switch (which)
		{
			/* maximal sector size (BIOS level) */
			case XH_DL_SECSIZ:
				return 32768L;
			
			/* minimal number of FATs */
			case XH_DL_MINFAT:
				return 1L;
			
			/* maximal number of FATs */
			case XH_DL_MAXFAT:
				return 4L;
			
			/* sectors per cluster minimal */
			case XH_DL_MINSPC:
				return 1L;
			
			/* sectors per cluster maximal */
			case XH_DL_MAXSPC:
				return 128L;
			
			/* maximal number of clusters of a 16 bit FAT */
			case XH_DL_CLUSTS:
				return 65518L; /* 0xffee */
			
			/* maximal number of sectors */
			case XH_DL_MAXSEC:
				return 2147483647L; /* LONG_MAX */
			
			/* maximal number of BIOS drives supported by the DOS */
			case XH_DL_DRIVES:
				return 32L;
			
			/* maximal clustersize */
			case XH_DL_CLSIZB:
				return 32768L; /* This should be the value MIN_BLOCK in block_IO.c */
			
			/* maximal (bpb->rdlen * bpb->recsiz / 32) */
			case XH_DL_RDLEN:
				return 2048; /* ??? */
			
			/* maximal number of clusters of a 12 bit FAT */
			case XH_DL_CLUSTS12:
				return 4078L; /* 0xfee */
			
			/* maximal number of clusters of a 32 bit FAT */
			case XH_DL_CLUSTS32:
				return 268435455L; /* 0x0fffffff */
			
			/* supported bits in bpb->bflags */
			case XH_DL_BFLAGS:
				return XHDI (XHDOSLIMITS, XH_DL_BFLAGS, 0UL);
		}
	}
	
	return ENOSYS;
}

/*
 * XHDI syscall wrapper routine
 */

long
sys_xhdi (ushort op,
		long a1, long a2, long a3, long a4,
		long a5, long a6, long a7)
{
	/* version information */
	if (op == XHGETVERSION)
		return XHDI_installed;

	/* XHEject */
	/* a2 contains do_eject parameter */
	if (op == XHEJECT && (a2 >> 16) == 1)
	{
		bio_sync_all ();
	}

	/* XHDrvMap */
	if (op == XHDRVMAP)
		return XHDI (XHDRVMAP);
	
	/* all other functions are restricted to root processes */
	if (!suser (get_curproc()->p_cred->ucr))
		return EPERM;
	
	/* XHNewCookie and XHMiNTInfo are never allowed */
	if (op == XHNEWCOOKIE || op == XHMINTINFO)
		return ENOSYS;
	
	/* applications see our own XHDOSLimits;
	 * mainly to make Uwe happy
	 */
	if (op == XHDOSLIMITS)
		return sys_XHDOSLimits ((a1 >> 16), (a1 << 16) | (a2 >> 16));
	
	return XHDI (op, a1, a2, a3, a4, a5, a6, a7);
}


/*
 * XHDI wrapper routines
 */

long
XHGetVersion (void)
{
	long own_version = 0x130;
	long installed;
	
	installed = XHDI (0);
	if (XHDI_installed)
		return MIN (own_version, installed);
	
	return 0;
}

long
XHInqTarget (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name)
{
	return XHDI (1, major, minor, block_size, device_flags, product_name);
}

long
XHReserve (ushort major, ushort minor, ushort do_reserve, ushort key)
{
	return XHDI (2, major, minor, do_reserve, key);
}

long
XHLock (ushort major, ushort minor, ushort do_lock, ushort key)
{
	return XHDI (3, major, minor, do_lock, key);
}

long
XHStop (ushort major, ushort minor, ushort do_stop, ushort key)
{
	return XHDI (4, major, minor, do_stop, key);
}

long
XHEject (ushort major, ushort minor, ushort do_eject, ushort key)
{
	if (do_eject == 1)
		bio_sync_all ();

	return XHDI (5, major, minor, do_eject, key);
}

long
XHDrvMap (void)
{
	return XHDI (6);
}

long
XHInqDev (ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, __BPB *bpb)
{
	return XHDI (7, bios_device, major, minor, start_sector, bpb);
}

long
XHInqDriver (ushort bios_device, char *name, char *ver, char *company, ushort *ahdi_version, ushort *maxIPL)
{
	return XHDI (8, bios_device, name, ver, company, ahdi_version, maxIPL);
}

long
XHNewCookie (void *newcookie)
{
	return XHDI (9, newcookie);
}

long
XHReadWrite (ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf)
{
	return XHDI (10, major, minor, rwflag, recno, count, buf);
}

long
XHInqTarget2 (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen)
{
	return XHDI (11, major, minor, block_size, device_flags, product_name, stringlen);
}

long
XHInqDev2 (ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, __BPB *bpb, ulong *blocks, char *partid)
{
	return XHDI (12, bios_device, major, minor, start_sector, bpb, blocks, partid);
}

long
XHDriverSpecial (ulong key1, ulong key2, ushort subopcode, void *data)
{
	return XHDI (13, key1, key2, subopcode, data);
}

long
XHGetCapacity (ushort major, ushort minor, ulong *blocks, ulong *bs)
{
	return XHDI (14, major, minor, blocks, bs);
}

long
XHMediumChanged (ushort major, ushort minor)
{
	return XHDI (15, major, minor);
}

long
XHMiNTInfo (ushort opcode, struct kerinfo *data)
{
	return XHDI (16, opcode, data);
}

long
XHDOSLimits (ushort which, ulong limit)
{
	if (limit == 0)
		return sys_XHDOSLimits(which, limit);

	return XHDI (17, which, limit);
}

long
XHLastAccess (ushort major, ushort minor, ulong *ms)
{
	return XHDI (18, major, minor, ms);
}

long
XHReaccess (ushort major, ushort minor)
{
	return XHDI (19, major, minor);
}
