/*
 * Filename:     xhdi.c
 * Version:      0.1
 * Author:       Frank Naumann
 * Started:      1998-08-01
 * Last Updated: 1999-03-26
 * Target O/S:   TOS
 * Description:  
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@).
 * 
 * Copying:      Copyright (C) 1998 Frank Naumann
 *                                  (fnaumann@cs.uni-magdeburg.de)
 *                                  (Frank Naumann @ L2)
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
 * 
 * 
 * changes since last version:
 * 
 * 0.1:
 * 
 * fix: Cookie handling stuff; use Getcookie from MiNT-Lib now
 *      requires an actual MiNT-Lib (>= PL49)
 * 
 * 0.0:
 * 
 * - first version
 */

# include "xhdi.h"

# include <stdlib.h>
# include <osbind.h>
# include <mintbind.h>

# include <sys/cookie.h>

# define __KERNEL__
# include <errno.h>


/*
 * internal usage
 */

/* dummy routine */
static long
XHDIfail (ushort opcode, ...)
{
	return ENOSYS;
}

/* XHDI handler function */
static long (*XHDI)(ushort opcode, ...) = XHDIfail;


ushort XHDI_installed = 0;

# define C_XHDI		0x58484449L
# define XHDIMAGIC	0x27011992L

long
XHDI_init (void)
{
	long i;
	
	i = Getcookie (C_XHDI, (long *) &XHDI);
	if (i == E_OK)
	{
		long *magic_test;
		
		/* check magic */
		magic_test = (long *) XHDI;
		if (magic_test)
		{
			magic_test--;
			if (*magic_test != XHDIMAGIC)
			{
				/* wrong magic */
				XHDI_installed = 0;
			}
			else
				/* check version */
				XHDI_installed = 1;
		}
		else
		{
			/* not installed */
			XHDI_installed = 0;			
		}
	}
	
	XHDI_installed = XHGetVersion ();
	
	/* we need at least XHDI 1.10 */
	if (XHDI_installed >= 0x110)
	{
		return 0;
	}
	
	XHDI = XHDIfail;
	XHDI_installed = 0;
	
	return 1;
}


/*
 * XHDI wrapper routines
 */

ushort
XHGetVersion (void)
{
	long oldstack = 0;
	ushort r = 0;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	if (XHDI_installed) r = XHDI (0);
	
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
		
	if (oldstack) Super (oldstack);
	return r;
}

long
XHEject (ushort major, ushort minor, ushort do_eject, ushort key)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	return XHDI (5, major, minor, do_eject, key);
		
	if (oldstack) Super (oldstack);
	return r;
}

ulong
XHDrvMap (void)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (6);
		
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
		
	if (oldstack) Super (oldstack);
	return r;
}

long
XHInqDev3 (ushort bios, ushort *major, ushort *minor, ulong *start, __xhdi_BPB *bpb, ulong *blocks, u_char *partid, ushort stringlen)
{
	long oldstack = 0;
	long r;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	r = XHDI (20, bios, major, minor, start, bpb, blocks, partid, stringlen);
		
	if (oldstack) Super (oldstack);
	return r;
}

void
XHMakeName (ushort major, ushort minor, ulong start, char *name)
{
	long oldstack = 0;
	
	if (!Super (1L)) oldstack = Super (0L);
	
	(void) XHDI (21, major, minor, start, name);
		
	if (oldstack) Super (oldstack);
}
