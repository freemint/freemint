/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
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

#include "ahcm.h"
#include "xa_defs.h"
#include "xalloc.h"


void *
xmalloc(size_t size, int key)
{
	void *a;

	DIAG((D_x, 0, "XA_alloc %ld k:%d\n", size, key));
	a = XA_alloc(0, size, key, 0);
	DIAG((D_x, 0, "     --> %ld\n", a));

	return a;	
}

void *
xcalloc(size_t items, size_t size, int key)
{
	void *a;

	DIAG((D_x, 0, "XA_calloc %ld*%ld, k:%d\n", items, size, key));
	a = XA_calloc(0, items, size, key, 0);
	DIAG((D_x, 0, "     --> %ld\n", a));

	return a;
}

void
free(void *addr)
{
#if GENERATE_DIAGS
	XA_unit *un;

	un = (XA_unit*)((long)addr - unitprefix);
	DIAG((D_x, 0, "XA_free %ld k:%d\n", addr, un->key));
#endif
	XA_free(0, addr);
}

void
_FreeAll(void)
{
	DIAG((D_x, 0, "XA_free_all\n"));
	XA_free_all(0, -1, -1);
}
