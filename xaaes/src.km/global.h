/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
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
#include "xa_defs.h"
#include "mint/module.h"
#include "mint/config.h"
#include "libkern/libkern.h"


/* XXX -> kassert */
# define FATAL KERNEL_FATAL

/* XXX -> dynamic mapping from kernel */
# define XAAES_MAGIC    0x58614145L
# define XAAES_MAGIC_SH 0x58614146L

/*
 * debug section
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

#define FORCE(x)       (*KENTRY->vec_debug.force) x

/*
 * hk:
 * STATIC used for all previously global symbols used (and defined) in one file
 */
#define STATIC static
/*
 * hk:
 * define 1 to inlude symbols never used anywhere
 */
#define INCLUDE_UNUSED 0

static inline void *
ptr_from_shorts(short hi, short lo)
{
	union { short word[2]; void *ptr; } p;

	p.word[0] = hi;
	p.word[1] = lo;

	return p.ptr;
}

static inline void
ptr_to_shorts(void *p, short *a)
{
	union { short s[2]; void *ptr; } ptoa;

	ptoa.ptr = p;
	a[0] = ptoa.s[0];
	a[1] = ptoa.s[1];
}


/*
 * memory management
 */
#undef kmalloc
#undef kfree
#undef umalloc
#undef ufree

void *xaaes_kmalloc(size_t size, const char *);
void  xaaes_kfree  (void *addr, const char *);
void *xaaes_umalloc(size_t size, const char *);
void  xaaes_ufree(void *addr, const char *);

#define kmalloc(size)	xaaes_kmalloc(size, FUNCTION)
#define kfree(addr)	xaaes_kfree(addr, FUNCTION)
#define umalloc(size)	xaaes_umalloc(size, FUNCTION)
#define ufree(addr)	xaaes_ufree(addr, FUNCTION)

void xaaes_kmalloc_leaks(void);


typedef char Path[PATH_MAX];


/*
 * creates the well known ping
 */
INLINE void ping(void) { b_ubconout(2, 7); }


/* XXX move to a better place */
#define AND_MEMORY , "memory"
#undef trap_14_w
#define trap_14_w(n)							\
__extension__								\
({									\
	register long retvalue __asm__("d0");				\
	    								\
	__asm__ volatile						\
	("\
		movw    %1,sp@-; \
		trap    #14;	\
		addql   #2,sp "						\
	: "=r"(retvalue)			/* outputs */		\
	: "g"(n)				/* inputs  */		\
	: __CLOBBER_RETURN("d0")					\
	  "d1", "d2", "a0", "a1", "a2"		/* clobbered regs */	\
	  AND_MEMORY							\
	);								\
	retvalue;							\
})

#define xbios_getrez()	(short)trap_14_w((short)(0x04))
#define _f_sync			(*KENTRY->vec_dos[0x103]) 	/* f_sync */
#define GL_AES_SZ	16
extern short my_global_aes[GL_AES_SZ];

extern struct xa_window *root_window, *menu_window;
#if WITH_BBL_HELP
extern struct xa_window *bgem_window;
#endif

#endif /* _global_h */
