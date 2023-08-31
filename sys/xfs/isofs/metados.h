/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
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

#ifndef _metados_h
#define _metados_h

#include <mint/mintbind.h>

struct meta_info_1
{
	unsigned long drivemap; 	/* bits for MetaDOS drives; bit 0 -> drive 'A' */
	char *version;			/* name and version of MetaDOS  */
	long reserved;
	struct meta_info_2 *info;
};

struct meta_info_2
{
	unsigned short mi_version;	/* version number (0x230 = '02.30') */
	long mi_magic;			/* magic number '_MET' */
	const char *mi_log2phys;	/* mapping GEMDOS -> MetaDOS */
};

struct meta_drvinfo
{
	char *mdr_name;
	long reserved[3];
};

#ifndef trap_14_wwllw
#define trap_14_wwllw(n, a, b, c, d)					\
__extension__								\
({									\
	register long retvalue __asm__("d0");				\
	short _a = (short)(a);						\
	long  _b = (long) (b);						\
	long  _c = (long) (c);						\
	short _d = (short)(d);						\
	    								\
	__asm__ volatile						\
	("\
		movw    %5,%%sp@-; \
		movl    %4,%%sp@-; \
		movl    %3,%%sp@-; \
		movw    %2,%%sp@-; \
		movw    %1,%%sp@-; \
		trap    #14;	\
		lea	%%sp@(14),%%sp "					\
	: "=r"(retvalue)			/* outputs */		\
	: "g"(n),							\
	  "r"(_a), "r"(_b), "r"(_c), "r"(_d)    /* inputs  */		\
	: __CLOBBER_RETURN("d0")					\
	  "d1", "d2", "a0", "a1", "a2", "memory"			\
	);								\
	retvalue;							\
})
#endif

#ifndef trap_14_wwlwl
#define trap_14_wwlwl(n, a, b, c, d)					\
__extension__								\
({									\
	register long retvalue __asm__("d0");				\
	short _a = (short)(a);						\
	long  _b = (long) (b);						\
	short _c = (short)(c);						\
	long  _d = (long) (d);						\
	    								\
	__asm__ volatile						\
	("\
		movl    %5,%%sp@-; \
		movw    %4,%%sp@-; \
		movl    %3,%%sp@-; \
		movw    %2,%%sp@-; \
		movw    %1,%%sp@-; \
		trap    #14;	\
		lea	%%sp@(14),%%sp "					\
	: "=r"(retvalue)			/* outputs */		\
	: "g"(n),							\
	  "r"(_a), "r"(_b), "r"(_c), "r"(_d)    /* inputs  */		\
	: __CLOBBER_RETURN("d0")					\
	  "d1", "d2", "a0", "a1", "a2", "memory"			\
	);								\
	retvalue;							\
})
#endif

#ifndef Metainit
#define Metainit(ptr) \
	trap_14_wl((short)0x30,(long)(ptr))
#endif

#ifndef Metaopen
#define Metaopen(drv,ptr) \
	trap_14_wwl((short)0x31,(short)(drv),(long)(ptr))
#endif

#ifndef Metaclose
#define Metaclose(drv) \
	trap_14_ww((short)0x32,(short)(drv))
#endif

#ifndef Metaread
#define Metaread(drv,buf,first,count) \
	trap_14_wwllw((short)0x33,(short)(drv),(long)(buf),(long)(first),\
			 (short)(count))
#endif

#ifndef Metaioctl
#define Metaioctl(drv,magic,opcode,buf) \
	trap_14_wwlwl((short)0x37,(short)(drv),(long)(magic),(short)(opcode),\
			 (long)(buf))
#endif

#endif /* _metados_h */
