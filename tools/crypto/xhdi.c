/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 * Started:      2000-05-02
 * 
 * Changes:
 * 
 * 0.1:
 * 
 * fix: Cookie handling stuff; use Getcookie from MiNT-Lib now
 *      requires an actual MiNT-Lib (>= PL49)
 * 
 * 0.0:
 * 
 * - inital version
 * 
 */

# include "xhdi.h"

# include <stdio.h>
# include <stdlib.h>
# include <errno.h>
# include <mintbind.h>
# include <sys/cookie.h>


/*
 * internal usage
 */

/* dummy routine */
static long
XHDIfail (ushort opcode, ...)
{
	return -ENOSYS;
}

/* XHDI handler function */
static long (*XHDI)(ushort opcode, ...) = XHDIfail;

ushort XHDI_installed = 0;


# define C_XHDI		0x58484449L
# define XHDIMAGIC	0x27011992L

long
XHDI_init (void)
{
	long *val;
	long r;
	
	r = Getcookie (C_XHDI, (long *) &val);
	if (r == C_FOUND)
	{
		long *magic_test = val;
		
		/* check magic */
		if (magic_test)
		{
			magic_test--;
			if (*magic_test == XHDIMAGIC)
			{
				(long *) XHDI = val;
			}
		}
	}
	
	r = XHGetVersion ();
	if (r < 0)
	{
		perror ("XHGetVersion");
		
		XHDI = XHDIfail;
		return r;
	}
	
	/* we need at least XHDI 1.10 */
	if (r >= 0x110)
	{
		XHDI_installed = r;
		return 0;
	}
	
	XHDI = XHDIfail;
	return -1;
}


/*
 * XHDI wrapper routines
 */

long
XHGetVersion (void)
{
	long oldstack = 0;
	long r = 0;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (0);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqTarget (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (1, major, minor, block_size, device_flags, product_name);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHReserve (ushort major, ushort minor, ushort do_reserve, ushort key)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (2, major, minor, do_reserve, key);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHLock (ushort major, ushort minor, ushort do_lock, ushort key)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (3, major, minor, do_lock, key);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHStop (ushort major, ushort minor, ushort do_stop, ushort key)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (4, major, minor, do_stop, key);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHEject (ushort major, ushort minor, ushort do_eject, ushort key)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (5, major, minor, do_eject, key);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHDrvMap (void)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (6);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqDev (ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (7, bios, major, minor, start, bpb);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqDriver (ushort bios, char *name, char *version, char *company, ushort *ahdi_version, ushort *maxIPL)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (8, bios, name, version, company, ahdi_version, maxIPL);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHNewCookie (void *newcookie)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (9, newcookie);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHReadWrite (ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (10, major, minor, rwflag, recno, count, buf);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqTarget2 (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (11, major, minor, block_size, device_flags, product_name, stringlen);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqDev2 (ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb, ulong *blocks, char *partid)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (12, bios, major, minor, start, bpb, blocks, partid);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHDriverSpecial (ulong key1, ulong key2, ushort subopcode, void *data)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (13, key1, key2, subopcode, data);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHGetCapacity (ushort major, ushort minor, ulong *blocks, ulong *bs)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (14, major, minor, blocks, bs);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHMediumChanged (ushort major, ushort minor)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (15, major, minor);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHMiNTInfo (ushort opcode, void *data)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (16, opcode, data);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHDOSLimits (ushort which, ulong limit)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (17, which, limit);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHLastAccess (ushort major, ushort minor, ulong *ms)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (18, major, minor, ms);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}

long
XHReaccess (ushort major, ushort minor)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (19, major, minor);
	if (r < 0)
	{
		__set_errno (-r);
		r = -1;
	}
	
	if (oldstack) Super (oldstack);
	return r;
}
