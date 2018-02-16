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

#ifndef _xa_list_h
#define _xa_list_h

#undef LIST_HEAD
#undef LIST_ENTRY
#undef LIST_NEXT
#undef LIST_INIT
#undef LIST_REMOVE
#undef LIST_FOREACH

/* list types */

#define LIST_HEAD(type) \
	struct { struct type *first; int mflag; }

#define LIST_ENTRY(type) \
	struct { struct type *next; struct type *prev; }

/* list access */

#define LIST_START(head) \
	((head)->first)

#define LIST_MFLAG(head) \
	((head)->mflag)

#define LIST_NEXT(element,field) \
	((element)->field.next)

#define LIST_PREV(element,field) \
	((element)->field.prev)

/* list operations */

#define LIST_INIT(head) do { \
	\
	LIST_START(head) = NULL; \
	\
} while (0)

#define LIST_INSERT_START(head,element,field) do { \
	\
	LIST_MFLAG(head) = 1; \
	\
	LIST_PREV(element,field) = NULL; \
	LIST_NEXT(element,field) = LIST_START(head); \
	\
	if (LIST_START(head)) \
		LIST_PREV(LIST_START(head),field) = element; \
	\
	LIST_START(head) = element; \
	\
} while (0)

#define LIST_INSERT_END(head,element,field,type) do { \
	\
	struct type **end = &(LIST_START(head)); \
	struct type *prev = NULL; \
	\
	LIST_MFLAG(head) = 1; \
	\
	while (*end) \
	{ \
		prev = *end; \
		end = &(LIST_NEXT(*end,field)); \
	} \
	\
	*end = element; \
	LIST_PREV(element,field) = prev; \
	LIST_NEXT(element,field) = NULL; \
	\
} while (0)

#define LIST_REMOVE(head,element,field) do { \
	\
	LIST_MFLAG(head) = 1; \
	\
	if ((element) == LIST_START(head)) \
		LIST_START(head) = LIST_NEXT(element,field); \
	\
	if (LIST_PREV(element,field)) \
		LIST_NEXT(LIST_PREV(element,field),field) = LIST_NEXT(element,field); \
	\
	if (LIST_NEXT(element,field)) \
		LIST_PREV(LIST_NEXT(element,field),field) = LIST_PREV(element,field); \
	\
} while (0)

#define LIST_FOREACH(head,run,field) \
	for (LIST_MFLAG(head) = 0, (run) = LIST_START(head); \
	     assert(LIST_MFLAG(head) == 0), (run); \
	     assert(LIST_MFLAG(head) == 0), (run) = LIST_NEXT(run,field))

#endif /* _xa_list_h */
