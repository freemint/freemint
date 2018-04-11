/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
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

#include "cookie.h"
#include "mint/module.h"
#ifndef TOSONLY
#include "libkern/libkern.h"

typedef char Path[PATH_MAX];

/* XXX -> kassert */
#define FATAL KERNEL_FATAL

/* XXX -> dynamic mapping from kernel */
//#define USB_MAGIC    0x5553425FL
//#define USB_MAGIC_SH 0x55534253L

/*
 * debug section
 */

#ifdef DEV_DEBUG

# define FORCE(x)       KERNEL_FORCE x
# define ALERT(x)       KERNEL_ALERT x
# define DEBUG(x)       KERNEL_DEBUG x
# define TRACE(x)       KERNEL_TRACE x
# define ASSERT(x)      assert x

#else

# define FORCE(x)       KERNEL_FORCE x
# define ALERT(x)       KERNEL_ALERT x
# define DEBUG(x)
# define TRACE(x)
# define ASSERT(x)      assert x

#endif

#else /* TOSONLY */
#include <mint/osbind.h> /* Setexc */
#include <stdarg.h>

#undef changedrv
#define changedrv (void)Mediach
#undef c_conws
#define c_conws (void)Cconws
#undef c_conout
#define c_conout (void)Cconout
#undef d_setdrv
#define d_setdrv (void)Dsetdrv
#undef kmalloc
#define kmalloc Malloc
#undef kfree
#define kfree Mfree

# define ALERT(x)
# define DEBUG(x)
# define TRACE(x)
# define ASSERT(x)

/* library replacements */
static inline void *memset(void *s, int c, long n)
{
	char *ptr = (char *)s;

	while (n--) {
		*ptr++ = c;
	}

	return s;
}

static inline void *memcpy(void *dest, const void *source, long n)
{
	char *dst = (char*)dest;
	const char *src = (const char*)source;

	while (n--) {
		*dst++ = *src++;
	}

	return dest;
}

static inline long strncmp (const char *str1, const char *str2, long len)
{
	register char c1, c2;

	do {
		c1 = *str1++;
		c2 = *str2++;
	}
	while (--len >= 0 && c1 && c1 == c2);
	
	if (len < 0)
		return 0L;
	
	return (long) (c1 - c2);
}

static inline char *strncpy (char *dst, const char *src, long len)
{
	register char *_dst = dst;
	
	while (--len >= 0 && (*_dst++ = *src++) != 0)
		continue;
	
	while (--len >= 0)
		*_dst++ = 0;
	
	return dst;
}

static inline char *strcat (char *dst, const char *src)
{
	register char *_dscan;
	
	for (_dscan = dst; *_dscan; _dscan++) ;
	while ((*_dscan++ = *src++) != 0) ;
	
	return dst;
}

static inline long strlen (const char *scan)
{
	register const char *start;
	
	start = scan + 1;
	while (*scan++)
		;
	
	return (long) (scan - start);
}

/* cookie jar definition
 */

#define _USB 0x5f555342L

static inline struct usb_module_api *
get_usb_cookie (void)
{
	struct usb_module_api *api;
	long ret;
	struct cookie *cjar;

	ret = Super(0L);

	api = NULL;
	cjar = *CJAR;

	while (cjar->tag)
	{
		if (cjar->tag == _USB)
		{
			api = (struct usb_module_api *)cjar->value;
	
			SuperToUser(ret);

			return api;
		}

		cjar++;
	}

	SuperToUser(ret);

	return NULL;
}

/*
 * delay functions, not accurate at all,
 * for slow CPUs (ex: 68000 on the ST)
 */
static inline void udelay (register ulong loops)
{
	__asm__ __volatile__
	(
		"1: subql #1,%0; jcc 1b"
		: "=d" (loops)
		: "0" (loops)
	);
}

static inline void mdelay (register ulong msecs)
{
	while (msecs--)
		udelay (1000);
}
#endif

static inline void hex_nybble(int n)
{
	char c;

	c = (n > 9) ? 'A'+n-10 : '0'+n;
	c_conout(c);
}

static inline void hex_byte(uchar n)
{
	hex_nybble(n>>4);
	hex_nybble(n&0x0f);
}

static inline void hex_word(ushort n)
{
	hex_byte(n>>8);
	hex_byte(n&0xff);
}

static inline void hex_long(ulong n)
{
	hex_word(n>>16);
	hex_word(n&0xffff);
};

#endif /* _global_h */
