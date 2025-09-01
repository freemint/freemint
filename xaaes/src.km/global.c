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

#include "global.h"
#include "xa_types.h"

/*
 * memory allocation for private XaAES memory
 *
 * include simple statistic analysis to detect
 * memory leaks
 */

static unsigned long memory;

void *
xaaes_kmalloc(size_t size, const char *func)
{
	register unsigned long *tmp;

	size += sizeof(*tmp);

	tmp = _kmalloc(size, func);
	if (tmp)
	{
		bzero(tmp, size);

		*tmp++ = size;
		memory += size;
	}

	return tmp;
}

void
xaaes_kfree(void *addr, const char *func)
{
	register unsigned long *tmp = addr;

	if (!addr)
		FATAL("xaaes_kfree(NULL) from %s", func);

	tmp--;
	if( *tmp == 0 || *tmp > memory )
	{
		BLOG((0,"%s:freeing invalid memory: %lx(%ld/%ld)", func, (unsigned long)(tmp+1), *tmp, memory));
		return;
	}
	memory -= *tmp;

	_kfree(tmp, func);
}

void
xaaes_kmalloc_leaks(void)
{
	DIAGS(("XaAES memory leaks: %lu bytes", memory));
	KERNEL_DEBUG("XaAES memory leaks: %lu bytes", memory);
}


/*
 * memory allocation for process private XaAES memory
 */

void *
xaaes_umalloc(size_t size, const char *func)
{
	register void *ptr;

	ptr = _umalloc(size, func);
	if (ptr)
		bzero(ptr, size);

	return ptr;
}

void
xaaes_ufree(void *addr, const char *func)
{
	_ufree(addr, func);
}

/* for old gemlib */
short my_global_aes[GL_AES_SZ];

/*
 * global data
 */
struct xa_window *root_window = 0, *menu_window = 0;
