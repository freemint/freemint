/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

#ifndef _xalloc_h
#define _xalloc_h

#include "global.h"

/* private memory */
void *_xmalloc(size_t size, int key, const char *);
void *_xcalloc(size_t items, size_t size, int key, const char *);
void  _xfree  (void *addr, const char *);

#define xmalloc(size, key)		_xmalloc(size, key, FUNCTION)
#define xcalloc(items, size, key)	_xcalloc(items, size, key, FUNCTION)
#define free(addr)			_xfree(addr, FUNCTION)

void free_all(void);

/* proc private memory */
void *_proc_malloc(size_t size, const char *);
void  _proc_free(void *addr, const char *);
void   proc_free_all(void); /* XXX todo */

#define proc_malloc(size)		_proc_malloc(size, FUNCTION)
#define proc_free(addr)			_proc_free(addr, FUNCTION)

#endif /* _xalloc_h */
