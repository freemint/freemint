/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-05-02
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
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
# include <mint/cookie.h>


/*
 * internal usage
 */

/* dummy routine */
static long __CDECL
XHDIfail (void)
{
	return -ENOSYS;
}

/* XHDI handler function */
typedef long __CDECL (*xhdi_t)(void);
static xhdi_t XHDI = XHDIfail;

ushort XHDI_installed = 0;


# define C_XHDI		0x58484449L
# define XHDIMAGIC	0x27011992L

/* initalize flag */
static ushort init = 1;

long
init_XHDI (void)
{
	long *val;
	long r;
	
	init = 0;
	
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
				XHDI = (xhdi_t)val;
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

# define CALL \
	long oldstack = 0;		\
	long r;				\
					\
	if (init) init_XHDI ();		\
					\
	if (!Super (1L))		\
		oldstack = Super (0L);	\
					\
	r = (*((long __CDECL (*)(struct args))XHDI)) (args);		\
	if (r < 0)			\
	{				\
		__set_errno (-r);	\
		r = -1;			\
	}				\
					\
	if (oldstack)			\
		Super (oldstack);	\
					\
	return r

long
XHGetVersion (void)
{
	struct args
	{
		ushort	opcode;
	}
	args = 
	{
		0
	};
	
	CALL;
}

long
XHInqTarget (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ulong	*block_size;
		ulong	*device_flags;
		char	*product_name;
	}
	args =
	{
		1,
		major,
		minor,
		block_size,
		device_flags,
		product_name
	};
	
	CALL;
}

long
XHReserve (ushort major, ushort minor, ushort do_reserve, ushort key)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ushort	do_reserve;
		ushort	key;
	}
	args =
	{
		2,
		major,
		minor,
		do_reserve,
		key
	};
	
	CALL;
}

long
XHLock (ushort major, ushort minor, ushort do_lock, ushort key)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ushort	do_lock;
		ushort	key;
	}
	args =
	{
		3,
		major,
		minor,
		do_lock,
		key
	};
	
	CALL;
}

long
XHStop (ushort major, ushort minor, ushort do_stop, ushort key)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ushort	do_stop;
		ushort	key;
	}
	args =
	{
		4,
		major,
		minor,
		do_stop,
		key
	};
	
	CALL;
}

long
XHEject (ushort major, ushort minor, ushort do_eject, ushort key)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ushort	do_eject;
		ushort	key;
	}
	args =
	{
		5,
		major,
		minor,
		do_eject,
		key
	};
	
	CALL;
}

long
XHDrvMap (void)
{
	struct args
	{
		ushort	opcode;
	}
	args =
	{
		6
	};
	
	CALL;
}

long
XHInqDev (ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb)
{
	struct args
	{
		ushort	opcode;
		ushort	bios;
		ushort	*major;
		ushort	*minor;
		ulong	*start;
		__BPB	*bpb;
	}
	args =
	{
		7,
		bios,
		major,
		minor,
		start,
		bpb
	};
	
	CALL;
}

long
XHInqDriver (ushort bios, char *name, char *version, char *company, ushort *ahdi_version, ushort *maxIPL)
{
	struct args
	{
		ushort	opcode;
		ushort	bios;
		char	*name;
		char	*version;
		char	*company;
		ushort	*ahdi_version;
		ushort	*maxIPL;
	}
	args =
	{
		8,
		bios,
		name,
		version,
		company,
		ahdi_version,
		maxIPL
	};
	
	CALL;
}

long
XHNewCookie (void *newcookie)
{
	struct args
	{
		ushort	opcode;
		void	*newcookie;
	}
	args =
	{
		9,
		newcookie
	};
	
	CALL;
}

long
XHReadWrite (ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ushort	rwflag;
		ulong	recno;
		ushort	count;
		void	*buf;
	}
	args =
	{
		10,
		major,
		minor,
		rwflag,
		recno,
		count,
		buf
	};
	
	CALL;
}

long
XHInqTarget2 (ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ulong	*block_size;
		ulong	*device_flags;
		char	*product_name;
		ushort	stringlen;
	}
	args =
	{
		11,
		major,
		minor,
		block_size,
		device_flags,
		product_name,
		stringlen
	};
	
	CALL;
}

long
XHInqDev2 (ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb, ulong *blocks, char *partid)
{
	struct args
	{
		ushort	opcode;
		ushort	bios;
		ushort	*major;
		ushort	*minor;
		ulong	*start;
		__BPB	*bpb;
		ulong	*blocks;
		char	*partid;
	}
	args =
	{
		12,
		bios,
		major,
		minor,
		start,
		bpb,
		blocks,
		partid
	};
	
	CALL;
}

long
XHDriverSpecial (ulong key1, ulong key2, ushort subopcode, void *data)
{
	struct args
	{
		ushort	opcode;
		ulong	key1;
		ulong	key2;
		ushort	subopcode;
		void 	*data;
	}
	args =
	{
		13,
		key1,
		key2,
		subopcode,
		data
	};
	
	CALL;
}

long
XHGetCapacity (ushort major, ushort minor, ulong *blocks, ulong *bs)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ulong	*blocks;
		ulong	*bs;
	}
	args =
	{
		14,
		major,
		minor,
		blocks,
		bs
	};
	
	CALL;
}

long
XHMediumChanged (ushort major, ushort minor)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
	}
	args =
	{
		15,
		major,
		minor
	};
	
	CALL;
}

long
XHMiNTInfo (ushort op, void *data)
{
	struct args
	{
		ushort	opcode;
		ushort	op;
		void	*data;
	}
	args =
	{
		16,
		op,
		data
	};
	
	CALL;
}

long
XHDOSLimits (ushort which, ulong limit)
{
	struct args
	{
		ushort	opcode;
		ushort	which;
		ulong	limit;
	}
	args =
	{
		17,
		which,
		limit
	};
	
	CALL;
}

long
XHLastAccess (ushort major, ushort minor, ulong *ms)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
		ulong	*ms;
	}
	args =
	{
		18,
		major,
		minor,
		ms
	};
	
	CALL;
}

long
XHReaccess (ushort major, ushort minor)
{
	struct args
	{
		ushort	opcode;
		ushort	major;
		ushort	minor;
	}
	args =
	{
		19,
		major,
		minor
	};
	
	CALL;
}
