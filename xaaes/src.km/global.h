/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _global_h
#define _global_h

#include <stddef.h>

#include "mint/mint.h"
#include "mint/config.h"
#include "libkern/libkern.h"

/* XXX -> kassert */
# define FATAL KERNEL_FATAL

/* XXX -> dynamic mapping from kernel */
# define XAAES_MAGIC 0x58614145L 

/* debug section
 */

#if GENERATE_DIAGS

#define ALERT(x)       KERNEL_ALERT x
#define DEBUG(x)       KERNEL_DEBUG x
#define TRACE(x)       KERNEL_TRACE x
#define ASSERT(x)      assert x

#else

#define ALERT(x)       KERNEL_ALERT x
#define DEBUG(x)
#define TRACE(x)
#define ASSERT(x)      assert x

#endif


typedef char Path[PATH_MAX];


/*
 * creates the well known ping
 */
INLINE void ping(void) { b_ubconout(2, 7); }


/* XXX move to a better place */
#define AND_MEMORY , "memory"
#define trap_14_w(n)							\
__extension__								\
({									\
	register long retvalue __asm__("d0");				\
	    								\
	__asm__ volatile						\
	("\
		movw    %1,sp@-; \
		trap    #14;	\
		addqw   #2,sp "						\
	: "=r"(retvalue)			/* outputs */		\
	: "g"(n)				/* inputs  */		\
	: "d0", "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */	\
	  AND_MEMORY							\
	);								\
	retvalue;							\
})
#define xbios_getrez()	(short)trap_14_w((short)(0x04))


#endif /* _global_h */
