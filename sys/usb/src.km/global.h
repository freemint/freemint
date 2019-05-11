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

/* DavidGZ: changedrv doesn't seem equivalent to Mediach BIOS function.
 * I don't know why this was done, just in case I'm missing something
 * I've commented the define instead of removing it.
 */
#undef changedrv
#define changedrv(x) /* (void)Mediach */
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

#ifdef DEV_DEBUG

/* Debug console output for TOS */
static char tos_debugbuffer[512];
long _cdecl kvsprintf(char *buf, long buflen, const char *fmt, va_list args) __attribute__((format(printf, 3, 0)));
static void tos_printmsg(const char *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	kvsprintf (tos_debugbuffer, sizeof(tos_debugbuffer)-1, fmt, args);
	va_end (args);
	(void)Cconws(tos_debugbuffer);
	(void)Cconws("\r\n");
}

# define ALERT(x)       tos_printmsg x
# define DEBUG(x)       tos_printmsg x
# define TRACE(x)
# define ASSERT(x)

#else /* !DEV_DEBUG */

# define ALERT(x)
# define DEBUG(x)
# define TRACE(x)
# define ASSERT(x)
#endif

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

static inline int getcookie(long target,long *p_value)
{
	long oldssp;
	struct cookie *cookie_ptr;

	if (Super((void *)1L) == 0L)
		oldssp = Super(0L);
	else
		oldssp = 0;

	cookie_ptr = *CJAR;

	if (oldssp)
		SuperToUser((void *)oldssp);

	if (cookie_ptr) {
		do {
			if (cookie_ptr->tag == target) {
				if (p_value)
					*p_value = cookie_ptr->value;
				return 1;
			}
		} while ((cookie_ptr++)->tag != 0L);
	}

	return 0;
}

static inline int setcookie (long tag, long value)
{
	long oldssp;
	struct cookie *cjar;
	long n = 0;

	if (Super((void *)1L) == 0L)
		oldssp = Super(0L);
	else
		oldssp = 0;

	cjar = * CJAR;

	if (oldssp)
		SuperToUser((void *)oldssp);

	while (cjar->tag)
	{
		n++;
		if (cjar->tag == tag)
		{
			cjar->value = value;
			return 1;
		}
		cjar++;
	}

	n++;
	if (n < cjar->value)
	{
		n = cjar->value;
		cjar->tag = tag;
		cjar->value = value;

		cjar++;
		cjar->tag = 0L;
		cjar->value = n;
		return 1;
	}

	return 0;
}

/* Precise delays functions for TOS USB drivers */
#include "tosdelay.c"

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
