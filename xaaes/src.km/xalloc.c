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

#include "xalloc.h"


/* memory allocation for private XaAES memory
 * 
 * include statistic analysis to detect
 * memory leaks
 */

static unsigned long memory;

void *
xmalloc(size_t size, int key)
{
	register unsigned long *tmp;

	size += sizeof(*tmp);

	tmp = kmalloc(size);
	if (tmp)
	{
		bzero(tmp, size);

		*tmp++ = size;
		memory += size;
	}

	return tmp;
}

void *
xcalloc(size_t items, size_t size, int key)
{
	return xmalloc(items * size, key);
}

void
free(void *addr)
{
	register unsigned long *tmp = addr;

	tmp--;
	memory -= *tmp;

	kfree(tmp);
}

void
free_all(void)
{
	DEBUG(("XaAES memory leaks: %lu bytes", memory));
}


/* memory allocation for process private XaAES memory
 */

void *
proc_malloc(size_t size)
{
	/* XXX todo
	 * very inefficient
	 * as the kernel allocate at least a chunk of 8kb
	 */
	register void *ptr;

	ptr = umalloc(size);
	if (ptr)
		bzero(ptr, size);

	return ptr;
}

void
proc_free(void *addr)
{
	ufree(addr);
}

void
proc_free_all(void)
{
}
