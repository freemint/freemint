/*
 * XaAES - XaAES Ain't the AES (c) 1999 - 2003 H.Robbers
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

#ifndef _ahcm_h
#define _ahcm_h

#include <sys/types.h>
#include "global.h"


#define XA_lib_replace 1

typedef short XA_key;
typedef struct xa_unit XA_unit;
typedef struct xa_list XA_list;
typedef struct xa_block XA_block;
typedef struct xa_memory XA_memory;

struct xa_unit
{
	long size;
	struct xa_unit *next,*prior;
	XA_key key, type;
	char area[0];
};

struct xa_list
{
	XA_unit *first, *cur, *last;
};

/* These are the big ones, at least 8K */
struct xa_block
{
	long size;
	struct xa_block *next, *prior;

	XA_list used, free;
	short mode;

	/* non ANSI, but gcc support it */
	XA_unit area[0];
};

struct xa_memory
{
	/* No free pointer here, blocks that
	 * become free are returned to GEMDOS
	 */
	XA_block *first, *last, *cur;

	long chunk;
	short head;
	short mode;
};

extern XA_memory XA_default_base;

typedef void XA_report(XA_memory *base, XA_block *blk, XA_unit *unit, char *txt);

void 	XA_set_base	(XA_memory *base, size_t chunk, short head, short flags);
void *	XA_alloc	(XA_memory *base, size_t amount, XA_key key, XA_key type);
void *	XA_calloc	(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type);
void 	XA_free		(XA_memory *base, void *area);
void 	XA_free_all	(XA_memory *base, XA_key key, XA_key type);
bool	XA_leaked	(XA_memory *base, XA_key key, XA_key type, XA_report *report);
void	XA_sanity	(XA_memory *base, XA_report *report);


#define unitprefix sizeof(XA_unit)
#define blockprefix sizeof(XA_block)


#endif /* _ahcm_h */
