/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2003 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

/* XXX clean me up */
# ifndef _cdecl
# define _cdecl __CDECL
# endif

/* PureC specifics to replace the mint kernel's libkern */
#ifdef __PUREC__

#include <compiler.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* memset */
#define bzero( buffer, count )  memset( buffer, 0, count )

typedef unsigned char uchar;

#else

/* common FreeMiNT kernel library used */
#include "libkern.h"

#endif


/* these are otherwise built in */
typedef enum boolean
{
	false = 0,
	true
} bool;


#ifndef PATH_MAX
#define PATH_MAX 128
#define NAME_MAX 128
#endif

typedef char Path[PATH_MAX];

/* XXX */
#define via(x) if (x) (*x)


#endif /* _global_h */
