/*
 * $Id$
 * 
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
# include "libkern/libkern.h"

# include "cookie.h"		/* cookie handling */
# include "init.h"		/* boot_printf */
# include "k_prot.h"		/* suser */


# ifdef DEBUG_INFO
# define XHDI_DEBUG(x)		DEBUG (x)
# else
# define XHDI_DEBUG(x)	
# endif


/*
 * internal usage
 */

/* dummy routine */
static long _cdecl
XHDIfail (ushort opcode, ...)
{
	UNUSED (opcode);
	
	return ENOSYS;
}

/* XHDI handler function */
static long _cdecl (*XHDI)(ushort opcode, ...) = XHDIfail;

ushort XHDI_installed = 0;


long
XHDI_init (void)
{
	long r;
	
# ifdef XHDI_MON
	static void init_xhdi_mon (void);
	init_xhdi_mon();
# endif
	
	r = get_toscookie (COOKIE_XHDI, (long *) &XHDI);
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
# ifdef NONBLOCKING_DMA
		r = XHMiNTInfo (XH_MI_SETKERINFO, &kernelinfo);
# else
		r = 1;
# endif
		
		boot_printf ("This system features XHDI level %x.%x (kerninfo %s)\r\n\r\n",
			XHDI_installed >> 8, XHDI_installed & 0xff,
			r ? "rejected" : "accepted"
		);
		
		*((long *) 0x4c2L) |= XHDrvMap ();
		
		set_cookie (COOKIE_XHDI, (long) emu_xhdi);
		r = E_OK;
	}
	else
	{
		boot_print ("This system does not feature XHDI.\r\n\r\n");
		
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
				return 65536L;
			
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
				return XHDOSLimits (XH_DL_BFLAGS, 0UL);
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
	if (op == 0)
		return XHDI_installed;
	
	/* XHDrvMap */
	if (op == 6)
		return XHDI (6);
	
	/* all other functions are restricted to root processes */
	if (!suser (curproc->p_cred->ucr))
		return EPERM;
	
	/* XHNewCookie and XHMiNTInfo are never allowed */
	if (op == 9 || op == 16)
		return ENOSYS;
	
	/* applications see our own XHDOSLimits;
	 * mainly to make Uwe happy
	 */
	if (op == 17)
		return sys_XHDOSLimits ((a1 >> 16), (a1 << 16) | (a2 >> 16));
	
	return XHDI (op, a1, a2, a3, a4, a5, a6, a7);
}


/*
 * XHDI wrapper routines
 */

long
XHGetVersion (void)
{
	ushort own_version = 0x130;
	ushort installed;
	
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
XHInqDriver (ushort bios_device, char *name, char *version, char *company, ushort *ahdi_version, ushort *maxIPL)
{
	return XHDI (8, bios_device, name, version, company, ahdi_version, maxIPL);
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


# ifdef XHDI_MON

static long _cdecl xhdi_mon_XHGetVersion(void);
static long _cdecl xhdi_mon_XHInqTarget(ushort, ushort, ulong *, ulong *, char *);
static long _cdecl xhdi_mon_XHReserve(ushort, ushort, ushort, ushort);
static long _cdecl xhdi_mon_XHLock(ushort, ushort, ushort, ushort);
static long _cdecl xhdi_mon_XHStop(ushort, ushort, ushort, ushort);
static long _cdecl xhdi_mon_XHEject(ushort, ushort, ushort, ushort);
static long _cdecl xhdi_mon_XHDrvMap(void);
static long _cdecl xhdi_mon_XHInqDev(ushort, ushort *, ushort *, ulong *, __BPB *);
static long _cdecl xhdi_mon_XHInqDriver(ushort, char *, char *, char *, ushort *, ushort *);
static long _cdecl xhdi_mon_XHNewCookie(void *);
static long _cdecl xhdi_mon_XHReadWrite(ushort, ushort, ushort, ulong, ushort, void *);
static long _cdecl xhdi_mon_XHInqTarget2(ushort, ushort, ulong *, ulong *, char *, ushort);
static long _cdecl xhdi_mon_XHInqDev2(ushort, ushort *, ushort *, ulong *, __BPB *, ulong *, char *);
static long _cdecl xhdi_mon_XHDriverSpecial(ulong, ulong, ushort, void *);
static long _cdecl xhdi_mon_XHGetCapacity(ushort, ushort, ulong *, ulong *);
static long _cdecl xhdi_mon_XHMediumChanged(ushort, ushort);
static long _cdecl xhdi_mon_XHMiNTInfo(ushort, void *);
static long _cdecl xhdi_mon_XHDOSLimits(ushort, ulong);
static long _cdecl xhdi_mon_XHLastAccess(ushort, ushort, ulong *);
static long _cdecl xhdi_mon_XHReaccess(ushort, ushort);

struct xhdi_mon_dispatcher
{
	void *func;
	ulong len;
}
xhdi_mon_dispatcher_table[] =
{
	{ xhdi_mon_XHGetVersion,	 0 },
	{ xhdi_mon_XHInqTarget,		16 },
	{ xhdi_mon_XHReserve,		 8 },
	{ xhdi_mon_XHLock,		 8 },
	{ xhdi_mon_XHStop,		 8 },
	{ xhdi_mon_XHEject,		 8 },
	{ xhdi_mon_XHDrvMap,		 0 },
	{ xhdi_mon_XHInqDev,		18 },
	{ xhdi_mon_XHInqDriver,		22 },
	{ xhdi_mon_XHNewCookie,		 4 },
	{ xhdi_mon_XHReadWrite,		16 },
	{ xhdi_mon_XHInqTarget2,	18 },
	{ xhdi_mon_XHInqDev2,		26 },
	{ xhdi_mon_XHDriverSpecial,	14 },
	{ xhdi_mon_XHGetCapacity,	12 },
	{ xhdi_mon_XHMediumChanged,	 4 },
	{ xhdi_mon_XHMiNTInfo,		 6 },
	{ xhdi_mon_XHDOSLimits,		 6 },
	{ xhdi_mon_XHLastAccess,	 8 },
	{ xhdi_mon_XHReaccess,		 4 }
};

long _cdecl (*xhdi_mon)(ushort opcode, ...) = XHDIfail;

static void
init_xhdi_mon (void)
{
	long r;
	
	r = get_toscookie (COOKIE_XHDI, (long *) &xhdi_mon);
	if (!r && xhdi_mon)
	{
		long *magic_test = (long *) xhdi_mon;
		
		magic_test--;
		
		if (*magic_test == XHDIMAGIC)
			set_toscookie (COOKIE_XHDI, (long) xhdi_mon_dispatcher);
	}
}

static long xhdi_mon_result (long res);
static void xhdi_mon_prres (long res, char *buf, long size);
static void xhdi_mon_prflags (ulong flags, char *buf, long size);
static void xhdi_mon_prbpb (__BPB *bpb);

static ushort _cdecl
xhdi_mon_XHGetVersion (void)
{
	ushort ret;
	
	XHDI_DEBUG(("XHGetVersion"));
	
	ret = xhdi_mon (0);
	
	XHDI_DEBUG(("-> %x.%02x", ret >> 8, ret & 0xff));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHInqTarget (ushort major, ushort minor, ulong *block_size,
		      ulong *device_flags, char *product_name)
{
	char buf[256];
	long ret;
	
	XHDI_DEBUG(("XHInqTarget    major %d  minor %d", major, minor));
	
	ret = xhdi_mon (1, major, minor, block_size, device_flags, product_name);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (block_size)
		{
			ksprintf (s, sizeof(s), "  block_size %ld", *block_size);
			strcat (buf, s);
		}
		
		if (device_flags)
			xhdi_mon_prflags (*device_flags, buf, sizeof(buf));
		
		if (product_name)
		{
			ksprintf (s, sizeof(s), "  product_name \"%s\"", product_name);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHReserve (ushort major, ushort minor, ushort do_reserve, ushort key)
{
	XHDI_DEBUG(("XHReserve    major %d  minor %d  do_reserve %d"
		    "  key  %d", major, minor, do_reserve, key));
	
	return xhdi_mon_result (xhdi_mon (2, major, minor, do_reserve, key));
}

static long _cdecl
xhdi_mon_XHLock (ushort major, ushort minor, ushort do_lock, ushort key)
{
	XHDI_DEBUG(("XHLock    major %d  minor %d  do_lock %d"
		    "  key %d", major, minor, do_lock, key));
	
	return xhdi_mon_result (xhdi_mon (3, major, minor, do_lock, key));
}

static long _cdecl
xhdi_mon_XHStop (ushort major, ushort minor, ushort do_stop, ushort key)
{
	XHDI_DEBUG(("XHStop    major %d  minor %d  do_stop %d"
		    "  key %d", major, minor, do_stop, key));
	
	return xhdi_mon_result (xhdi_mon (4, major, minor, do_stop, key));
}

static long _cdecl
xhdi_mon_XHEject (ushort major, ushort minor, ushort do_eject, ushort key)
{
	XHDI_DEBUG(("XHEject    major %d  minor %d  do_eject %d"
		    "  key %d", major, minor, do_eject, key));
	
	return xhdi_mon_result (xhdi_mon (5, major, minor, do_eject, key));
}

static ulong _cdecl
xhdi_mon_XHDrvMap (void)
{
	char buf[128];
	ulong ret;
	int i;
	
	XHDI_DEBUG(("XHDrvMap"));
	
	ret = xhdi_mon (6);
	
	strcat (buf, "-> ");
	for (i = 0; i < 32; i++)
	{
		if (ret & (1L << i))
		{
			char s[2];
			
			s[1] = '\0';
			
			if (i < 26) { s[0] = 'A'+i; }
			else { s[0] = i - 26 + '0'; }
			
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHInqDev (ushort bios_device, ushort *major, ushort *minor,
		   ulong *start_sector, __BPB *bpb)
{
	char buf[256];
	long ret;
	
	XHDI_DEBUG(("XHInqDev    bios_device %d", bios_device));
	
	ret = xhdi_mon (7, bios_device, major, minor, start_sector, bpb);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (major)
		{
			ksprintf (s, sizeof(s), "  major %d", *major);
			strcat (buf, s);
		}
		
		if (minor)
		{
			ksprintf (s, sizeof(s), "  minor %d", *minor);
			strcat (buf, s);
		}
		
		if (start_sector)
		{
			ksprintf (s, sizeof(s), "  start_sector %ld", *start_sector);
			strcat (buf, s);
		}
		
		if (bpb)
		{
			ksprintf (s, sizeof(s), "  bpb $%lx", bpb);
			strcat (buf, s);
		}
	}
	else if (ret == -2)
	{
		char s[32];
		
		if (major)
		{
			ksprintf (s, sizeof(s), "  major %d", *major);
			strcat (buf, s);
		}
		
		if (minor)
		{
			ksprintf (s, sizeof(s), "  minor %d", *minor);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	if (bpb && bpb->recsiz)
		xhdi_mon_prbpb (bpb);
	
	return ret;
}

static long _cdecl
xhdi_mon_XHInqDriver (ushort bios_device, char *name, char *version,
		      char *company, ushort *ahdi_version, ushort *maxIPL)
{
	char buf[256];
	long ret;

	XHDI_DEBUG(("XHInqDriver    bios_device %d", bios_device));
	
	ret = xhdi_mon (8, bios_device, name, version, company, ahdi_version, maxIPL);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (name)
		{
			ksprintf (s, sizeof(s), "  name \"%s\"", name);
			strcat (buf, s);
		}
		
		if (version)
		{
			ksprintf (s, sizeof(s), "  version \"%s\"", version);
			strcat (buf, s);
		}
		
		if (company)
		{
			ksprintf (s, sizeof(s), "  company \"%s\"", company);
			strcat (buf, s);
		}
		
		if (ahdi_version)
		{
			ksprintf (s, sizeof(s), "  ahdi_version \"%s\"", ahdi_version);
			strcat (buf, s);
		}
		
		if (maxIPL)
		{
			ksprintf (s, sizeof(s), "  maxIPL %d", maxIPL);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHNewCookie (void *newcookie)
{
	XHDI_DEBUG(("XHNewCookie    $%lx", newcookie));
	
	xhdi_mon = newcookie;
	return 0;
}

static long _cdecl
xhdi_mon_XHReadWrite (ushort major, ushort minor, ushort rwflag, ulong recno,
		      ushort count, void *buf)
{
	XHDI_DEBUG(("XHReadWrite    major %d  minor %d  rwflag %d  "
		    "recno %ld  count %d  buf $%lx", major, minor, rwflag, recno, count, buf));
	
	return xhdi_mon_result (xhdi_mon (10, major, minor, rwflag, recno, count, buf));
}

static long _cdecl
xhdi_mon_XHInqTarget2 (ushort major, ushort minor, ulong *block_size,
		       ulong *device_flags, char *product_name, ushort stringlen)
{
	char buf[256];
	long ret;
	
	XHDI_DEBUG(("XHInqTarget2    major %d  minor %d  stringlen %d", major, minor, stringlen));
	
	ret = xhdi_mon (11, major, minor, block_size, device_flags, product_name, stringlen);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (block_size)
		{
			ksprintf (s, sizeof(s), "  block_size %ld", *block_size);
			strcat (buf, s);
		}
		
		if (device_flags)
			xhdi_mon_prflags (*device_flags, buf, sizeof(buf));
		
		if (product_name)
		{
			ksprintf (s, sizeof(s), "  product_name \"%s\"", product_name);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHInqDev2 (ushort bios_device, ushort *major, ushort *minor,
		    ulong *start_sector, __BPB *bpb, ulong *blocks, char *partid)
{
	char buf[256];
	long ret;
	
	XHDI_DEBUG(("XHInqDev2    bios_device %d", bios_device));
	
	ret = xhdi_mon (12, bios_device, major, minor, start_sector, bpb, blocks, partid);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (major)
		{
			ksprintf (s, sizeof(s), "  major %d", *major);
			strcat (buf, s);
		}
		
		if (minor)
		{
			ksprintf (s, sizeof(s), "  minor %d", *minor);
			strcat (buf, s);
		}
		
		if (start_sector)
		{
			ksprintf (s, sizeof(s), "  start_sector %ld", *start_sector);
			strcat (buf, s);
		}
		
		if (bpb)
		{
			ksprintf (s, sizeof(s), "  bpb $%lx", bpb);
			strcat (buf, s);
		}
		
		if (partid)
		{
			ksprintf (s, sizeof(s), "  partid \"%s\"", partid);
			strcat (buf, s);
		}
	}
	else if (ret == -2)
	{
		char s[32];
		
		if (major)
		{
			ksprintf (s, sizeof(s), "  major %d", *major);
			strcat (buf, s);
		}
		
		if (minor)
		{
			ksprintf (s, sizeof(s), "  minor %d", *minor);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	if (bpb && bpb->recsiz)
		xhdi_mon_prbpb (bpb);
	
	return ret;
}

static long _cdecl
xhdi_mon_XHDriverSpecial (ulong key1, ulong key2, ushort subopcode, void *data)
{
	/* 0x55534844L = USHD
	 */
	if (key1 == 0x55534844L && (key2 == 0x13497800L || key2 == 0x2690454L))
		XHDI_DEBUG(("XHDriverSpecial    [HDDRIVER]  subopcode %d"
			    "  data $%lx", subopcode, data));
	else
		XHDI_DEBUG(("XHDriverSpecial    key1 %ld  key2 %ld  subopcode %d"
			    "  data $%lx", key1, key2, subopcode, data));
	
	return xhdi_mon_result (xhdi_mon (13, key1, key2, subopcode, data));
}

static long _cdecl
xhdi_mon_XHGetCapacity (ushort major, ushort minor, ulong *blocks, ulong *bs)
{
	char buf[256];
	long ret;
	
	XHDI_DEBUG(("XHGetCapacity    major %d  minor %d", major, minor));
	
	ret = xhdi_mon (14, major, minor, blocks, bs);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	if (!ret)
	{
		char s[32];
		
		if (blocks)
		{
			ksprintf (s, sizeof(s), "  blocks %ld", *blocks);
			strcat (buf, s);
		}
		
		if (bs)
		{
			ksprintf (s, sizeof(s), "  blocksize %ld", *bs);
			strcat (buf, s);
		}
	}
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHMediumChanged(ushort major, ushort minor)
{
	XHDI_DEBUG(("XHMediumChanged    major %d  minor %d", major, minor));
	
	return xhdi_mon_result (xhdi_mon (15, major, minor));
}

static long _cdecl
xhdi_mon_XHMiNTInfo(ushort opcode, void *data)
{
	XHDI_DEBUG(("XHMiNTInfo    opcode %d  data $%lx", opcode, data));
	
	return xhdi_mon_result (xhdi_mon (16, opcode, data));
}

static long _cdecl
xhdi_mon_XHDOSLimits(ushort which, ulong limit)
{
	char buf[256];
	
	strcpy (buf, "XHDOSLimits    ");
	
	switch (which)
	{
		case 0:  strcat (buf, "XH_DL_SECSIZ");		break;
		case 1:  strcat (buf, "XH_DL_MINFAT");		break;
		case 2:  strcat (buf, "XH_DL_MAXFAT");		break;
		case 3:  strcat (buf, "XH_DL_MINSPC");		break;
		case 4:  strcat (buf, "XH_DL_MAXSPC");		break;
		case 5:  strcat (buf, "XH_DL_CLUSTS");		break;
		case 6:  strcat (buf, "XH_DL_MAXSEC");		break;
		case 7:  strcat (buf, "XH_DL_DRIVES");		break;
		case 8:  strcat (buf, "XH_DL_CLSIZB");		break;
		case 9:  strcat (buf, "XH_DL_RDLEN");		break;
		case 12: strcat (buf, "XH_DL_CLUSTS12");	break;
		case 13: strcat (buf, "XH_DL_CLUSTS32");	break;
		case 14: strcat (buf, "XH_DL_BFLAGS");		break;
		default:
		{
			char s[32];
			ksprintf (s, sizeof(s), "%d", which);
			strcat (buf, s);
			break;
		}
	}
	
	if (limit)
	{
		char s[32];
		
		ksprintf (s, sizeof(s), "  limit %ld", limit);
		strcat (buf, s);
	}
	
	XHDI_DEBUG(("%s", buf));
	
	return xhdi_mon_result (xhdi_mon (17, which, limit));
}

static long _cdecl
xhdi_mon_XHLastAccess (ushort major, ushort minor, ulong *ms)
{
	char buf[256];
	char s[32];
	long ret;
	
	XHDI_DEBUG(("XHLastAccess    major %d  minor %d", major, minor));
	
	ret = xhdi_mon (18, major, minor, ms);
	xhdi_mon_prres (ret, buf, sizeof(buf));
	
	ksprintf (s, sizeof(s), "  ms %ld", ret, ms);
	strcat (buf, s);
	XHDI_DEBUG(("%s", buf));
	
	return ret;
}

static long _cdecl
xhdi_mon_XHReaccess (ushort major, ushort minor)
{
	XHDI_DEBUG(("XHReaccess    major %d  minor %d", major, minor));
	
	return xhdi_mon_result (xhdi_mon (19, major, minor));
}


static long
xhdi_mon_result (long res)
{
	char buf[128];
	
	xhdi_mon_prres (res, buf, sizeof(buf));
	XHDI_DEBUG(("%s", buf));
	
	return res;
}

static void
xhdi_mon_prres (long res, char *buf, long size)
{
	char s[32];
	
	ksprintf (s, sizeof(s), "%ld", res);
	if (res < 0)
	{
		switch (res)
		{
			case -1:  strcpy (s, "ERROR");	break;
			case -2:  strcpy (s, "EDRVNR"); break;
			case -13: strcpy (s, "EWRPRO"); break;
			case -14: strcpy (s, "E_CHNG"); break;
			case -32: strcpy (s, "EINVFN"); break;
			case -36: strcpy (s, "EACCDN"); break;
			case -46: strcpy (s, "EDRIVE"); break;
			default:
				break;
		}
	}
	
	strcpy (buf, "-> ");
	strcat (buf, s);
}

static void
xhdi_mon_prflags (ulong flags, char *buf, long size)
{
	strcat (buf, "  device_flags");
	
	if (flags & 0xe000000fL)
	{
		if (flags & 0x00000001L) strcat (buf, "  XH_TARGET_STOPPABLE");
		if (flags & 0x00000002L) strcat (buf, "  XH_TARGET_REMOVABLE");
		if (flags & 0x00000004L) strcat (buf, "  XH_TARGET_LOCKABLE");
		if (flags & 0x00000008L) strcat (buf, "  XH_TARGET_EJECTABLE");
		if (flags & 0x20000000L) strcat (buf, "  XH_TARGET_LOCKED");
		if (flags & 0x40000000L) strcat (buf, "  XH_TARGET_STOPPED");
		if (flags & 0x80000000L) strcat (buf, "  XH_TARGET_RESERVED");
	}
	else
	{
		char s[32];
		
		ksprintf (s, sizeof(s), "  %ld", flags);
		strcat (buf, s);
	}
}

static void
xhdi_mon_prbpb (__BPB *bpb)
{
	XHDI_DEBUG(("  recsiz %u  clsiz %u  clsizb %u  rdlen %u", bpb->recsiz, bpb->clsiz, bpb->clsizb, bpb->rdlen));
	XHDI_DEBUG(("  fsiz %u  fatrec %u  datrec %u  numcl %u", bpb->fsiz, bpb->fatrec, bpb->datrec, bpb->numcl));
	XHDI_DEBUG(("  bflags %u", bpb->bflags));
}

# endif
