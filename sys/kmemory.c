/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * implementation aspects:
 * =======================
 *
 * kmr_get/kmr_free:
 * - low answer latency
 * - avoid cyclic recursion problems with MEMREGION descriptors
 *
 * kmalloc/kfree:
 * - low answer latency
 * - strict 16 byte alignment
 *
 * -> small blocks (size <= (S1_SIZE - S1_HEAD)) are handled in O(1)
 *
 *
 * I analysed a little bit kmalloc/kfree requests:
 * -----------------------------------------------
 *
 * - 2/3 of all requests are smaller than 32 byte (without TIMEOUT kmallocs!)
 *   - there are 1000 TIMEOUT kmallocs per minute (22 byte)
 *   - all these requests are temporary and freed short time later!
 *
 * --> special small block handler for blocks <= (S1_SIZE - S1_HEAD)
 *     works in O(1) (that means all requests are answered in constant time)
 *
 *
 * - the other requests are greater and mostly temporary
 *   there are also a number of kmallocs that are resident
 *
 * -> special small block handler for blocks > S1_SIZE and <= PAGESIZE
 *    fast answer (reduce searching overhead)
 *
 */

# include "kmemory.h"

# include "libkern/libkern.h"
# include "arch/mprot.h" /* no_mem_prot */

# include "memory.h"

# define KMEMORY_VERS	1		/* internal version */

# define PAGESIZE	8192		/* pagesize */
# define PAGEBITS	13		/* relevant bits */

# define HASHBITS	8
# define HASHSIZE	(1UL << HASHBITS)
# define HASHMASK	(HASHSIZE - 1)

# define MR_MAGIC	(0x5330)
# define S1_MAGIC	(0x5331)
# define S2_MAGIC	(0x5332)
# define LB_MAGIC	(0x5333)
# define ST_MAGIC	(0x5334)

# define MR_SIZE	((sizeof (MEMREGION) + MR_HEAD + 3) & ~3)
# define S1_SIZE	(16UL * 3)


/*
 * debugging stuff
 */

# if 1
# ifdef DEBUG_INFO
# define KMEMORY_DEBUG 1
# endif
# else
# define KMEMORY_DEBUG 1
# endif


# ifndef KMEMORY_DEBUG

#  define KM_FORCE(x)
#  define KM_ALERT(x)	ALERT x
#  define KM_DEBUG(x)
#  define KM_ASSERT(x)
#  define KM_ALIGN(x,s)

# else

#  undef S1_SIZE
#  define S1_SIZE	(16UL * 4)

#  define KM_FORCE(x)	FORCE x
#  define KM_ALERT(x)	ALERT x
#  define KM_DEBUG(x)	km_debug x
#  define KM_ASSERT(x)	assert x
#  define KM_ALIGN(x,s)	{ if ((long) x & 15) \
			{ DEBUG (("%s, %ld: not aligned (%s, %lx)!", \
				__FILE__, (long) __LINE__, s, (long) x)); } }
#if 0
static void km_debug (const char *fmt, ...);
#endif

#  if 1
#  define KM_USAGE
#  endif

#  if 1
#  define KM_STAT		/* need 65kb extra RAM (at runtime) */
#  define KM_STAT_FILE		"u:\\ram\\kmstat.c"
#  define KM_STAT_SMAGIC	(0x6373)
#  define KM_STAT_LMAGIC	(0x7363)
#  define KM_STAT_OFFSET	(4)
#  ifndef KM_USAGE
#  define KM_USAGE
#  endif
#  endif

#  ifndef NO_RAMFS
#  define KM_TRACE
#  define KM_TRACE_FILE		"u:\\ram\\kmtrace.txt"
#  endif

#  ifndef NO_RAMFS
#  define KM_DEBUG_FILE		"u:\\ram\\kmemdebug.txt"
#  endif

#  if 0
#  define MR_DEBUG
#  endif

#  if 0
#  define S1_DEBUG
#  endif

#  if 0
#  define S2_DEBUG
#  endif

#  if 0
#  define LB_DEBUG
#  endif

# endif

/****************************************************************************/
/* BEGIN definition part */

typedef struct list LIST;
typedef struct km_p KM_P;
typedef struct km_s KM_S;

typedef struct km_mr KM_MR;
typedef struct km_s1 KM_S1;
typedef struct km_s2 KM_S2;
typedef struct km_lb KM_LB;


struct list
{
	KM_P	*head;
	KM_P	*tail;
};

struct km_p
{
/*1*/	KM_P		*hnext;	/* next in hash list */
/*2*/	KM_P		*next;	/* next in linked list */
/*3*/	KM_P		*prev;	/* prev in linked list */
/*4*/	MEMREGION	*self;	/* descriptor for this block */
	ushort		size;	/* size in 16 byte blocks */
/*5*/	ushort		magic;	/* magic number */
# define PS_HEAD	(5 * 4)	/* page small head size */

	union
	{
		struct
		{
			KM_MR	*free;
			ulong	pos;
			ushort	used;
		} mr;

		struct
		{
			KM_S1	*free;
			ulong	pos;
			ushort	used;
		} s1;

		struct
		{
# ifdef S2_DEBUG
			KM_S2	*list;
			ulong	size;
# endif
		} s2;
	} s;
# define P__HEAD	((sizeof (KM_P) + 15) & ~15)
};

struct km_s
{
	KM_P	*page;
# define S__HEAD	(sizeof (KM_S))
};


# ifndef KMEMORY_DEBUG
/* align structures size optimized */

struct km_mr
{
	KM_S	s;
# define MR_HEAD	(S__HEAD)
	KM_MR	*next;
};

struct km_s1
{
	KM_S	s;
# define S1_HEAD	(S__HEAD)
	KM_S1	*next;
};

struct km_s2
{
	ushort	magic;
	ushort	size;
	ushort	free;
# define INUSE		0
# define FREE		1
	ushort	res;
	KM_S2	*next;
	KM_S2	*prev;
	KM_S	s;
# define S2_HEAD	(16UL + S__HEAD)
# if 1
# define S2_SPLIT_SIZE	(((S2_HEAD + S1_SIZE) + 15) & ~15)
# else
# define S2_SPLIT_SIZE	80
# endif
	KM_S2	*fnext;
	KM_S2	*fprev;
};

# else /* KMEMORY_DEBUG */
/* align structures debug optimized */

struct km_mr
{
	KM_MR	*next;
	KM_S	s;
# define MR_HEAD	(sizeof (KM_MR))
};

struct km_s1
{
	KM_S1	*next;
	KM_S	s;
# define S1_HEAD	(sizeof (KM_S1))
};

struct km_s2
{
	ushort	magic;
	ushort	size;
	ushort	free;
# define INUSE		0
# define FREE		1
	ushort	res;
	KM_S2	*next;
	KM_S2	*prev;
# if 1
# define S2_SPLIT_SIZE	(((S2_HEAD + S1_SIZE) + 15) & ~15)
# else
# define S2_SPLIT_SIZE	80
# endif
	KM_S2	*fnext;
	KM_S2	*fprev;
	KM_S	s;
# define S2_HEAD	(sizeof (KM_S2))
};

# endif  /* KMEMORY_DEBUG */


/*
 * internal prototypes
 */


/* large block hash table functions */

INLINE ulong	km_hash		(const void *ptr);
INLINE KM_P *	km_hash_lookup	(const void *ptr);
INLINE void	km_hash_insert	(KM_P *item);
INLINE void	km_hash_remove	(KM_P *item);


/* help functions */

INLINE MEMREGION *km_get_region	(MMAP map, ulong size, MEMREGION *m, short cmode);
INLINE void	km_free_region	(MEMREGION *m);
INLINE KM_P *	km_malloc	(ulong size, MEMREGION *descr, KM_P *page);
INLINE void	km_free		(MEMREGION *m);


/* double linked list functions */

INLINE void	km_list_init	(LIST *list);
INLINE long	km_list_empty	(LIST *list);
INLINE void	km_list_insert	(LIST *list, KM_P *item);
INLINE void	km_list_remove	(LIST *list, KM_P *item);


/* MEMREGION alloc */

static void	km_mr_setup	(KM_P *page);
# ifdef MR_DEBUG
static void	km_mr_dump	(void);
# endif


/* small 1 alloc */

static void	km_s1_setup	(KM_P *page);
INLINE void *	km_s1_malloc	(void);
INLINE void	km_s1_free	(KM_S1 *ptr, KM_P *page);
# ifdef S1_DEBUG
static void	km_s1_dump	(void);
# endif


/* small 2 alloc */

INLINE void	km_s2_flist_ins	(KM_S2 *item);
INLINE void	km_s2_flist_rem	(KM_S2 *item);
INLINE KM_S2 *	km_s2_split	(KM_S2 *item, ushort size);
INLINE void	km_s2_concat	(KM_S2 *item1, KM_S2 *item2);
static KM_S2 *	km_s2_setup	(KM_P *page);
INLINE void *	km_s2_malloc	(ushort size);
INLINE void	km_s2_free	(KM_S2 *ptr, KM_P *page);
# ifdef S2_DEBUG
static void	km_s2_dump	(void);
# endif


/* large block alloc */

INLINE void *	km_lb_malloc	(ulong size);
INLINE void	km_lb_free	(KM_P *kmp);
# ifdef LB_DEBUG
static void	km_lb_dump	(void);
# endif


/* statistic functions */

# ifdef KM_STAT
static void	km_stat_dump	(void);
# endif

# ifdef KM_TRACE
static void	km_trace_register(void *ptr, unsigned long size, const char *func);
static void	km_trace_unregister(void *ptr);
static void	km_trace_dump(void);
# endif

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN large block hash table */

static KM_P *table [HASHSIZE];

INLINE ulong
km_hash (const void *ptr)
{
# if 0
	register ulong hash;

	hash = (ulong) ptr;
	hash = hash + (hash >> HASHBITS) + (hash >> (HASHBITS << 1));

	return hash & HASHMASK;
# else
	return (ulong) ptr & HASHMASK;
# endif
}

INLINE KM_P *
km_hash_lookup (const void *ptr)
{
	register KM_P *n;

	KM_ASSERT ((ptr));

	for (n = table [km_hash (ptr)]; n; n = n->hnext)
	{
		if (ptr == (void *) n->self->loc)
		{
			break;
		}
	}

	return n;
}

INLINE void
km_hash_insert (KM_P *item)
{
	register KM_P **n = &(table [km_hash ((void *) item->self->loc)]);

	item->hnext = *n;
	*n = item;
}

INLINE void
km_hash_remove (KM_P *item)
{
	register KM_P **n = &(table [km_hash ((void *) item->self->loc)]);

	while (*n)
	{
		if (item == *n)
		{
			/* remove from table */
			*n = (*n)->hnext;

			return;
		}
		n = &((*n)->hnext);
	}
}

# if 0
void
km_hash_dump (void)
{
	int i;

	printf ("----------------------------------------\n");

	for (i = 0; i < HASHSIZE; i++)
	{
		KM_P *n;

		printf ("slot %i -> %lx\n\n", i, table [i]);

		for (n = table [i]; n; n = n->hnext)
		{
			printf ("\tn = %lx\n", n);			fflush (stdout);
			printf ("\t\tn->hnext = %lx\n", n->hnext);	fflush (stdout);
			printf ("\t\tn->next = %lx\n", n->next);	fflush (stdout);
			printf ("\t\tn->prev = %lx\n", n->prev);	fflush (stdout);
			printf ("\t\tn->self = %lx\n", n->self);	fflush (stdout);
			printf ("\t\tn->size = %li\n", n->size);	fflush (stdout);
			printf ("\t\tn->magic = %lx\n", n->magic);	fflush (stdout);

			printf ("\n");

			KM_ASSERT ((n->self));
			KM_ASSERT ((n->self->loc));
		}

		printf ("\n");
	}

	printf ("----------------------------------------\n");
	fflush (stdout);
}
# endif

/* END large block hash table */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data */

# ifdef KM_USAGE
static ulong	used_mem = 0;
# endif

# ifdef KM_STAT
static struct
{
	ulong	req_alloc;
	ulong	req_free;

} stat [PAGESIZE];

static struct
{
	ulong	req_alloc;
	ulong	req_free;

} kmr_stat;

static struct
{
	ulong	req_alloc;
	ulong	req_free;

} km_s1_stat;
# endif

/* END global data */
/****************************************************************************/

/****************************************************************************/
/* BEGIN help functions */

/*
 * get/free region primitives
 */

INLINE MEMREGION *
km_get_region (MMAP map, ulong size, MEMREGION *m, short cmode)
{
	register MEMREGION *new = _get_region (map, size, PROT_S, cmode, m, 1);

	if (new)
	{
		new->mflags |= M_KMALLOC;
# ifdef KM_USAGE
		used_mem += new->len;

		KM_ALIGN (new->loc, "km_get_region");
# endif
	}
	return new;
}

INLINE void
km_free_region (MEMREGION *m)
{
# ifdef KMEMORY_DEBUG
	mint_bzero ((void *) m->loc, m->len);
# endif
# ifdef KM_USAGE
	used_mem -= m->len;
# endif

	m->links--;
	free_region (m);
}

/*
 * get/free service routines
 */

INLINE KM_P *
km_malloc (ulong size, MEMREGION *descr, KM_P *page)
{
	MEMREGION *m;

	/* try first TT-RAM */
	m = km_get_region (alt, size, descr, 0);

	if (!m)
		/* fall back to ST-RAM */
		m = km_get_region (core, size, descr, 0);

	if (m) {
		if (!page)
			page = (KM_P *) m->loc;

		page->self = m;
		page->size = size >> 4;
	} else
		page = NULL;

	return page;
}

INLINE void
km_free (MEMREGION *m)
{
	km_free_region (m);
}

/* END help functions */
/****************************************************************************/

/****************************************************************************/
/* BEGIN double linked list functions */

INLINE void
km_list_init (LIST *list)
{
	list->head = NULL;
	list->tail = NULL;
}

INLINE long
km_list_empty (LIST *list)
{
	return (list->head == NULL);
}

INLINE void
km_list_insert (LIST *list, KM_P *item)
{
	if (list->tail)
		list->tail->next = item;
	else
	{
		KM_ASSERT ((!list->head));
		list->head = item;
	}

	item->prev = list->tail;
	item->next = NULL;

	list->tail = item;
}

INLINE void
km_list_remove (LIST *list, KM_P *item)
{
	if (item->prev)
		/* not first element */
		item->prev->next = item->next;
	else
		/* correct head ptr */
		list->head = item->next;

	if (item->next)
		/* not last element */
		item->next->prev = item->prev;
	else
		/* correct tail ptr */
		list->tail = item->prev;
}

/* END double linked list functions */
/****************************************************************************/

/****************************************************************************/
/* BEGIN MEMREGION alloc */

static char mr_initial [PAGESIZE + 16];
static LIST mr_list = { NULL, NULL };

static KM_P *mr_free;

# define km_mr_list_init()	km_list_init	(&mr_list)
# define km_mr_list_empty()	km_list_empty	(&mr_list)
# define km_mr_list_insert(n)	km_list_insert	(&mr_list, n)
# define km_mr_list_remove(n)	km_list_remove	(&mr_list, n)

static void
km_mr_setup (KM_P *page)
{
	register char *ptr = (char *) page + P__HEAD;

	km_mr_list_insert (page);

	page->magic = MR_MAGIC;
	page->s.mr.free = (KM_MR *) ptr;
	page->s.mr.pos = page->prev ? page->prev->s.mr.pos + 1 : 0;
	page->s.mr.used = 0;

	mr_free = page;

	{
		register KM_MR *temp;
		register long i;

		for (i = (PAGESIZE - P__HEAD) / MR_SIZE; i; i--)
		{
			temp = (KM_MR *) ptr;
			temp->s.page = page;

			ptr += MR_SIZE;

			temp->next = (KM_MR *) ptr;
		}

		temp->next = NULL;
	}
}

MEMREGION *
_kmr_get (void)
{
	register MEMREGION *m;
	register KM_MR *new;

retry:
	new = mr_free->s.mr.free;
	mr_free->s.mr.free = mr_free->s.mr.free->next;
	mr_free->s.mr.used++;

	KM_ASSERT ((new->s.page == mr_free));

# ifdef KM_STAT
	kmr_stat.req_alloc++;
# endif

	m = (MEMREGION *) ((char *) new + MR_HEAD);
	mint_bzero(m, sizeof(*m));
#if 0
	m->loc		= 0;
	m->len		= 0;
	m->links	= 0;
	m->mflags	= 0;
	m->save		= NULL;
	m->shadow	= NULL;
	m->next		= NULL;
#endif
	if (!mr_free->s.mr.free)
	{
		while (mr_free && !mr_free->s.mr.free)
		{
			mr_free = mr_free->next;
		}

		if (!mr_free)
		{
			register KM_P *new_page = km_malloc (PAGESIZE, m, NULL);

			if (new_page)
			{
				km_mr_setup (new_page);
				goto retry;
			}

			new->next = mr_free->s.mr.free;
			mr_free->s.mr.free = new;
			mr_free->s.mr.used--;

			KM_ALERT ((__FILE__ ": kmr_get fail, out of memory?"));

# ifdef KM_STAT
			kmr_stat.req_alloc--;
# endif

			return NULL;
		}
	}

	return m;
}

void
_kmr_free (MEMREGION *place)
{
	register KM_MR *ptr = (KM_MR *) ((char *) place - MR_HEAD);
	register KM_P *page = ptr->s.page;

# ifdef KMEMORY_DEBUG
	KM_ASSERT ((place->links != 65000));
	place->links = 65000;
# endif

	KM_ASSERT ((page && page->magic == MR_MAGIC));

# ifdef KM_STAT
	kmr_stat.req_free++;
# endif

	ptr->next = page->s.mr.free;
	page->s.mr.free = ptr;
	page->s.mr.used--;

	if (page->s.mr.used == 0 && page->self && mr_free != page)
	{
		km_mr_list_remove (page);

		/* free page */
		km_free (page->self);
	}
	else if (mr_free->s.mr.pos > page->s.mr.pos)
	{
		mr_free = page;
	}
}

# ifdef MR_DEBUG
static void
km_mr_dump (void)
{
# error km_mr_dump not defined
}
# endif

# undef km_mr_list_init
# undef km_mr_list_empty
# undef km_mr_list_insert
# undef km_mr_list_remove

/* END MEMREGION alloc */
/****************************************************************************/

/****************************************************************************/
/* BEGIN small 1 alloc */

static char s1_initial [PAGESIZE + 16];
static LIST s1_list = { NULL, NULL };

static KM_P *s1_free;

# define km_s1_list_init()	km_list_init	(&s1_list)
# define km_s1_list_empty()	km_list_empty	(&s1_list)
# define km_s1_list_insert(n)	km_list_insert	(&s1_list, n)
# define km_s1_list_remove(n)	km_list_remove	(&s1_list, n)

static void
km_s1_setup (KM_P *page)
{
	register char *ptr = (char *) page + P__HEAD;

	km_s1_list_insert (page);

	page->magic = S1_MAGIC;
	page->s.s1.free = (KM_S1 *) ptr;
	page->s.s1.pos = page->prev ? page->prev->s.s1.pos + 1 : 0;
	page->s.s1.used = 0;

	s1_free = page;

	{
		register KM_S1 *temp;
		register long i;

		for (i = (PAGESIZE - P__HEAD) / S1_SIZE; i; i--)
		{
			temp = (KM_S1 *) ptr;
			temp->s.page = page;

			ptr += S1_SIZE;

			temp->next = (KM_S1 *) ptr;
		}

		temp->next = NULL;
	}
}

INLINE void *
km_s1_malloc (void)
{
	register KM_S1 *new;

retry:
	new = s1_free->s.s1.free;

	KM_ALIGN (new, "s1");

	s1_free->s.s1.free = s1_free->s.s1.free->next;
	s1_free->s.s1.used++;

	KM_ASSERT ((new->s.page == s1_free));

# ifdef KM_STAT
	km_s1_stat.req_alloc++;
# endif

	if (!s1_free->s.s1.free)
	{
		while (s1_free && !s1_free->s.s1.free)
		{
			s1_free = s1_free->next;
		}

		if (!s1_free)
		{
			register MEMREGION *m = kmr_get ();

			if (m)
			{
				register KM_P *new_page = km_malloc (PAGESIZE, m, NULL);

				if (new_page)
				{
					km_s1_setup (new_page);
					goto retry;
				}

				kmr_free (m);
			}

			new->next = s1_free->s.s1.free;
			s1_free->s.s1.free = new;
			s1_free->s.s1.used--;

# ifdef KM_STAT
			km_s1_stat.req_alloc--;
# endif

			return NULL;
		}
	}

	return ((char *) new + S1_HEAD);
}

INLINE void
km_s1_free (KM_S1 *ptr, KM_P *page)
{
# ifdef KM_STAT
	km_s1_stat.req_free++;
# endif

	KM_ASSERT ((page && page->magic == S1_MAGIC));

	ptr->next = page->s.s1.free;
	page->s.s1.free = ptr;
	page->s.s1.used--;

# ifdef KMEMORY_DEBUG
	mint_bzero ((char *) ptr + S1_HEAD, S1_SIZE - S1_HEAD);
# endif

	if (page->s.s1.used == 0 && page->self && s1_free != page)
	{
		km_s1_list_remove (page);

		/* free page */
		km_free (page->self);
	}
	else if (s1_free->s.s1.pos > page->s.s1.pos)
	{
		s1_free = page;
	}
}

# ifdef S1_DEBUG
static void
km_s1_dump (void)
{
# error km_s1_dump not defined
}
# endif

# undef km_s1_list_init
# undef km_s1_list_empty
# undef km_s1_list_insert
# undef km_s1_list_remove

/* END small 1 alloc */
/****************************************************************************/

/****************************************************************************/
/* BEGIN small 2 alloc */

# ifdef S2_DEBUG
static LIST s2_list = { NULL, NULL };

# define km_s2_list_init()	km_list_init	(&s2_list)
# define km_s2_list_empty()	km_list_empty	(&s2_list)
# define km_s2_list_insert(n)	km_list_insert	(&s2_list, n)
# define km_s2_list_remove(n)	km_list_remove	(&s2_list, n)
# endif

/*
 * insert KM_S2 block in free list
 *
 * - set free flag to FREE
 * - update double linked list
 */

static KM_S2 *s2_free_head = NULL;
static KM_S2 *s2_free_tail = NULL;

INLINE void
km_s2_flist_ins (KM_S2 *item)
{
	if (s2_free_tail)
		s2_free_tail->fnext = item;
	else
	{
		KM_ASSERT ((!s2_free_head));
		s2_free_head = item;
	}

	item->free = FREE;
	item->fprev = s2_free_tail;
	item->fnext = NULL;

	s2_free_tail = item;
}

/*
 * remove KM_S2 block from free list
 *
 * - simple list operation (double linked list)
 */

INLINE void
km_s2_flist_rem (KM_S2 *item)
{
	item->free = INUSE;

	if (item->fnext)
		item->fnext->fprev = item->fprev;
	else
		s2_free_tail = item->fprev;

	if (item->fprev)
		item->fprev->fnext = item->fnext;
	else
		/* item is first element -> correct start */
		s2_free_head = item->fnext;
}

/*
 * split a KM_S2 block
 *
 * - new block is positionated at the end of KM_S2 block
 * - double linked list is updated
 * - free is initialized as INUSE
 * - return pointer to new KM_S2 block
 *
 * conditions:
 * - size include system area
 * - block must be large enough
 * -> no checks
 */

INLINE KM_S2 *
km_s2_split (KM_S2 *item, ushort size)
{
	register KM_S2 *new;

	item->size -= size;

	new = (KM_S2 *) ((char *) item + item->size);

	KM_ALIGN (new, "s2");

	new->magic = S2_MAGIC;
	new->size = size;
	new->free = INUSE;

	new->next = item->next;
	new->prev = item;

	if (item->next)
		item->next->prev = new;
	item->next = new;

	new->s.page = item->s.page;

	return new;
}

/*
 * concat 2 KM_S2 blocks
 *
 * - item2 must be next element after item1
 */

INLINE void
km_s2_concat (KM_S2 *item1, KM_S2 *item2)
{
	item1->size += item2->size;

	item1->next = item2->next;

	if (item2->next)
		item2->next->prev = item1;
}

static KM_S2 *
km_s2_setup (KM_P *page)
{
	register KM_S2 *temp = (KM_S2 *) ((char *) page + P__HEAD);

	page->magic = S2_MAGIC;

# ifdef S2_DEBUG
	page->s.s2.list = temp;
	km_s2_list_insert (page);
# endif

	temp->magic = S2_MAGIC;
	temp->size = PAGESIZE - P__HEAD;
	temp->free = INUSE;
	temp->next = NULL;
	temp->prev = NULL;
	temp->s.page = page;

	km_s2_flist_ins (temp);

	return temp;
}

INLINE void *
km_s2_malloc (ushort size)
{
	register KM_S2 *new = s2_free_head;
	register void *ptr = NULL;

	size += S2_HEAD;
	size = (size + 15) & ~15;

	while (new)
	{
		if (size <= new->size)
		{
			break;
		}

		new = new->fnext;
	}

	if (!new)
	{
		register MEMREGION *m = kmr_get ();

		if (m)
		{
			register KM_P *page = km_malloc (PAGESIZE, m, NULL);

			if (page)
			{
				new = km_s2_setup (page);

				KM_ALIGN (new, "s2");
			}
			else
			{
				kmr_free (m);
			}
		}
	}
	else
	{
		KM_ALIGN (new, "s2");
	}

	if (new)
	{
		if ((new->size - size) > S2_SPLIT_SIZE)
		{
			new = km_s2_split (new, size);
		}
		else
		{
			km_s2_flist_rem (new);
		}

		ptr = (char *) new + S2_HEAD;

		KM_ALIGN (new, "s2");
	}

	return ptr;
}
#include "signal.h"
#include "mint/signal.h"
INLINE void
km_s2_free (KM_S2 *ptr, KM_P *page)
{
	register KM_S2 *next = ptr->next;
	register KM_S2 *prev = ptr->prev;

	if(ptr->magic != S2_MAGIC)
	{
		KM_FORCE(("********magic: %x != %x:%lx->%lx**********(%ld,%s)", ptr->magic, S2_MAGIC, ptr, (char*)ptr + S2_HEAD, (long) __LINE__, __FILE__));
		KM_ALERT(("ERROR:0x%x != 0x%x:%lx->%lx**********(%ld,%s)", ptr->magic, S2_MAGIC, ptr, (char*)ptr + S2_HEAD, (long) __LINE__, __FILE__));
		raise (SIGTERM);
		return;
	}

	KM_ASSERT ((ptr->magic == S2_MAGIC));

	if (next && next->free == FREE)
	{
		km_s2_flist_rem (next);

		/* concat ptr and next */
		km_s2_concat (ptr, next);
	}

	if (prev && prev->free == FREE)
	{
		km_s2_flist_rem (prev);

		/* concat prev and ptr */
		km_s2_concat (prev, ptr);

		ptr = prev;
	}

# ifdef KMEMORY_DEBUG
	mint_bzero ((char *) ptr + S2_HEAD, ptr->size - S2_HEAD);
# endif

	if (ptr->size == PAGESIZE - P__HEAD)
	{
# ifdef S2_DEBUG
		km_s2_list_remove (page);
# endif
		km_free (page->self);
	}
	else
		km_s2_flist_ins (ptr);
}

# ifdef S2_DEBUG
static void
km_s2_dump (void)
{
	KM_P *page = s2_list.head;
	KM_S2 *t;
	long i = 0;

	while (page)
	{
		printf ("page: %li\n", i++);
		printf ("page = %lx\n", (ulong) page);
		printf ("page->next = %lx\n", (ulong) page->next);
		printf ("page->prev = %lx\n", (ulong) page->prev);

		KM_ASSERT ((page->magic == S2_MAGIC));

		printf ("Blockliste:\n");

		t = page->s.s2.list;
		while (t)
		{
			printf ("\tself = %lx\n", (ulong) t);
			printf ("\tprev = %lx\n", (ulong) t->prev);
			printf ("\tnext = %lx\n", (ulong) t->next);
			printf ("\tfree = %i\n", t->free);
			printf ("\tsize = %i\n", t->size);

			KM_ASSERT ((t->magic == S2_MAGIC));

			t = t->next;
		}

		printf ("\n");

		page = page->next;
	}

	printf ("Freeliste:\n");

	t = s2_free;
	while (t)
	{
		printf ("self = %lx\n", (ulong) t);
		printf ("page = %lx\n", (ulong) t->s.page);
		printf ("fprev = %lx\n", (ulong) t->fprev);
		printf ("fnext = %lx\n", (ulong) t->fnext);
		printf ("free = %i\n", t->free);
		printf ("size = %i\n", t->size);
		printf ("\n");

		KM_ASSERT ((t->magic == S2_MAGIC));

		t = t->fnext;
	}

	printf ("--------------------------------------------------------\n");
}

# undef km_s2_list_init
# undef km_s2_list_empty
# undef km_s2_list_insert
# undef km_s2_list_remove
# endif

/* END small 2 alloc */
/****************************************************************************/

/****************************************************************************/
/* BEGIN large block alloc */

# ifdef LB_DEBUG
static LIST lb_list = { NULL, NULL };

# define km_lb_list_init()	km_list_init	(&lb_list)
# define km_lb_list_empty()	km_list_empty	(&lb_list)
# define km_lb_list_insert(n)	km_list_insert	(&lb_list, n)
# define km_lb_list_remove(n)	km_list_remove	(&lb_list, n)
# endif

INLINE void *
km_lb_malloc (ulong size)
{
	MEMREGION *m = kmr_get ();
	KM_P *kmp = (KM_P *) kmr_get ();

	if (m && kmp)
	{
		KM_P *new;

		if (size < PAGESIZE)
			size = PAGESIZE;
		else
			size = (size + 15) & ~15;

		new = km_malloc (size, m, kmp);

		if (new)
		{
			new->magic = LB_MAGIC;
# ifdef LB_DEBUG
			km_lb_insert (new);
# endif
			km_hash_insert (new);

			return (void *) new->self->loc;
		}
	}

	if (m) kmr_free (m);
	if (kmp) kmr_free ((void *) kmp);

	return NULL;
}

INLINE void
km_lb_free (KM_P *kmp)
{
	KM_ASSERT ((kmp->magic == LB_MAGIC));

# ifdef LB_DEBUG
	km_lb_remove (kmp);
# endif

	km_hash_remove (kmp);

	km_free (kmp->self);
	kmr_free ((void *) kmp);
}

# ifdef LB_DEBUG
static void
km_lb_dump (void)
{
# error km_lb_dump not defined
}

# undef km_lb_list_init
# undef km_lb_list_empty
# undef km_lb_list_insert
# undef km_lb_list_remove
# endif

/* END large block alloc */
/****************************************************************************/

/****************************************************************************/
/* BEGIN kernel memory alloc */

# if 0
/* this don't work */
void *
_kcore (ulong size, const char *func)
{
	MEMREGION *m = kmr_get ();
	char *ptr = NULL;

	KM_DEBUG (("%s: kcore (%li)!", func, size));

	if (m)
	{
		register MEMREGION *new;

# ifdef KM_STAT
		size += KM_STAT_OFFSET;
# endif
		size += P__HEAD + S__HEAD;
		size = (size + 15) & ~15;

		new = km_get_region (core, size, m, 0);
		if (new)
		{
			register KM_P *page = (KM_P *) new->loc;

			if (new->links != 1)
				FATAL ("%s: kcore: new-links != 1", func);

			page->self = new;
			page->size = size >> 4;
			page->magic = ST_MAGIC;

			{
				register KM_S *head;

				head = (KM_S *) ((char *) page + P__HEAD);
				head->page = page;
			}

			ptr = (char *) page;
			ptr += P__HEAD + S__HEAD;
# ifdef KM_STAT
			{
				register ushort *p = (ushort *) ptr;

# if KM_STAT_OFFSET != 4
# error KM_STAT_OFFSET != 4
# endif
				*p++ = size >> 4;
				*p = KM_STAT_LMAGIC;

				ptr += KM_STAT_OFFSET;
			}
# endif
		}
		else
		{
			KM_ALERT (("%s: kcore (%li) fail, out of memory?", func, size));
		}
	}

	return ptr;
}
# endif

void * _cdecl
_kmalloc(unsigned long size, const char *func)
{
	register char *ptr = NULL;

# ifdef KM_STAT
	register long flag = 1;
	size += KM_STAT_OFFSET;
# endif

	if (!size)
		FATAL("%s: kmalloc(%lu) - invalid size request", func, size);

	if (size <= (S1_SIZE - S1_HEAD))
		/* very small blocks */
		ptr = km_s1_malloc();

	if (!ptr)
	{
		/* lock if there is something left that is bigger than s1 strategy */
		if (size < (PAGESIZE - P__HEAD - S2_HEAD - (S1_SIZE - S1_HEAD)))
		{
			/* medium sized blocks */
			ptr = km_s2_malloc(size);
		}
		else
		{
# ifdef KM_STAT
			flag = 0;
			size -= KM_STAT_OFFSET;
# endif
			ptr = km_lb_malloc(size);
		}
	}

	if (!ptr)
	{
		KM_ALERT(("%s: kmalloc (%li) fail, out of memory?", func, size));
	}
# ifdef KM_STAT
	else if (flag)
	{
		register ushort *p = (ushort *) ptr;

# if KM_STAT_OFFSET != 4
# error KM_STAT_OFFSET != 4
# endif
		size -= KM_STAT_OFFSET;

		stat[size].req_alloc++;

		*p++ = size;
		*p = KM_STAT_SMAGIC;

		ptr += KM_STAT_OFFSET;
	}
# endif

# ifdef KM_TRACE
	if (ptr)
		km_trace_register(ptr, size, func);
# endif

	return ptr;
}

void _cdecl
_kfree(void *place, const char *func)
{
	KM_P *page;

	if (!place)
	{
		KM_ALERT(("%s: kfree on NULL ptr!", func));
		return;
	}

# ifdef KM_TRACE
	km_trace_unregister(place);
# endif

	page = km_hash_lookup(place);
	if (page)
	{
		km_lb_free(page);
	}
	else
	{
# ifdef KM_STAT
		register ushort *p = place;

		p--;
		if (*p == KM_STAT_SMAGIC)
		{
			register ushort size;

			p--;
			size = *p;

			if (size < PAGESIZE)
				stat[size].req_free++;
		}
		else if (*p == KM_STAT_LMAGIC)
		{
		}
		else
		{
			//FATAL ("%s: kfree on 0x%lx, invalid STAT_MAGIC!", func, (ulong) place);
			KM_FORCE (("%s: kfree on 0x%lx, invalid STAT_MAGIC:%x!", func, (ulong) place, *p));
			KM_ALERT (("%s: kfree on 0x%lx, invalid STAT_MAGIC:%x!", func, (ulong) place, *p));
		}

		place = (char *) place - KM_STAT_OFFSET;
# endif
		page = *(KM_P **)((KM_S *) place - 1);

		/* not maskable */
		assert (page);

		switch (page->magic)
		{
			case S1_MAGIC:
			{
				km_s1_free((KM_S1 *)((char *) place - S1_HEAD), page);
				break;
			}
			case S2_MAGIC:
			{
				km_s2_free((KM_S2 *)((char *) place - S2_HEAD), page);
				break;
			}
			case ST_MAGIC:
			{
				km_free_region(page->self);
				break;
			}
			default:
			{
				//FATAL("%s: kfree on 0x%lx: invalid S?_MAGIC!", func, (ulong) place);
				KM_FORCE(("%s: kfree on 0x%lx: invalid S?_MAGIC:%x!", func, (ulong) place, page->magic));
				KM_ALERT(("%s: kfree on 0x%lx: invalid S?_MAGIC:%x!", func, (ulong) place, page->magic));
				raise (SIGTERM);
			}
		}
	}
}

/* extended kmalloc
 *
 */
void * _cdecl
_dmabuf_alloc(ulong size, short cmode, const char *func)
{
	MEMREGION *m;

	/* we can't support cmode if memory protection is disabled */
	if (cmode
# ifdef WITH_MMU_SUPPORT
	    && no_mem_prot
# endif
	    )
		return NULL;

	m = kmr_get();
	if (m)
	{
		MEMREGION *new;

		if (size < PAGESIZE)
			size = PAGESIZE;
		else
			size = (size + 15) & ~15;

		new = km_get_region(core, size, m, cmode);
		if (new)
			return (void *)new->loc;

		kmr_free(m);
	}

	return NULL;
}

/* END kernel memory alloc */
/****************************************************************************/

/****************************************************************************/
/* BEGIN init & configuration part */

void
init_kmemory (void)
{
	void *_mr_initial = (void *) (((long) mr_initial + 15) & ~15);
	void *_s1_initial = (void *) (((long) s1_initial + 15) & ~15);

	if (PS_HEAD > sizeof (MEMREGION))
	{
		FATAL (__FILE__ ", %ld: sizeof (KM_P) > sizeof (MEMREGION) "
			"-> internal failure", (long) __LINE__);
	}

	km_mr_setup (_mr_initial);
	km_s1_setup (_s1_initial);

# ifdef KM_STAT
	{
		long i;

		for (i = 0; i < PAGESIZE; i++)
		{
			stat [i].req_alloc = 0;
			stat [i].req_free = 0;
		}

		kmr_stat.req_alloc = 0;
		kmr_stat.req_free = 0;

		km_s1_stat.req_alloc = 0;
		km_s1_stat.req_free = 0;
	}
# endif
}

long
km_config (long mode, long arg)
{
	switch (mode)
	{
# ifdef KM_STAT
		case KM_STAT_DUMP:
		{
			km_stat_dump ();
			return E_OK;
		}
# endif
# ifdef KM_TRACE
		case KM_TRACE_DUMP:
		{
			km_trace_dump();
			return E_OK;
		}
# endif
	}

	return ENOSYS;
}

/* END init & configuration part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN stat part */

# ifdef KM_STAT

# include "k_fds.h"
# include "util.h"

static void
km_stat_dump(void)
{
	static char line[SPRINTF_MAX];

	FILEPTR *fp;
	long ret;

	KM_FORCE((__FILE__ ": used_mem = %li", used_mem));
	KM_FORCE((__FILE__ ": kmr: alloc = %li, free = %li -> %li",
		kmr_stat.req_alloc,
		kmr_stat.req_free,
		kmr_stat.req_alloc - kmr_stat.req_free
	));
	KM_FORCE ((__FILE__ ": km_s1: alloc = %li, free = %li -> %li",
		km_s1_stat.req_alloc,
		km_s1_stat.req_free,
		km_s1_stat.req_alloc - km_s1_stat.req_free
	));

	ret = do_open(&fp, rootproc, KM_STAT_FILE, (O_RDWR | O_CREAT | O_TRUNC), 0, NULL);
	if (!ret)
	{
		long i;

		ksprintf(line, sizeof(line), "/*\r\n * " __FILE__ ": used_mem = %li\r\n *\r\n"
			 "kmr: alloc = %li, free = %li -> %li\r\n",
			 used_mem,
			 kmr_stat.req_alloc,
			 kmr_stat.req_free,
			 kmr_stat.req_alloc - kmr_stat.req_free
		);
								(*fp->dev->write)(fp, line, strlen(line));

		ksprintf(line, sizeof(line), "km_s1: alloc = %li, free = %li -> %li\r\n\rn",
			 km_s1_stat.req_alloc,
			 km_s1_stat.req_free,
			 km_s1_stat.req_alloc - km_s1_stat.req_free
		);
		(*fp->dev->write)(fp, line, strlen(line));

		ksprintf(line, sizeof(line), "size:\talloc:\tfree:\tdifference:\r\n");
		(*fp->dev->write)(fp, line, strlen(line));

		for (i = 0; i < PAGESIZE; i++)
		{
			if (stat [i].req_alloc)
			{
				ksprintf(line, sizeof(line),
					 "%ld\t%ld\t%ld\t%ld\r\n",
					 i,
					 stat[i].req_alloc,
					 stat[i].req_free,
					 stat[i].req_alloc - stat[i].req_free);
				(*fp->dev->write)(fp, line, strlen(line));
			}
		}

		ksprintf(line, sizeof(line), "\r\n *\r\n */\r\n\r\n");
								(*fp->dev->write)(fp, line, strlen(line));

		ksprintf(line, sizeof(line),
			 "# include \"kmstat.h\"\r\n\r\n"
			 "void\r\narray_init (void)\r\n{\r\n");
								(*fp->dev->write)(fp, line, strlen(line));

		for (i = 0; i < PAGESIZE; i++)
		{
			if (stat [i].req_alloc)
			{
				ksprintf(line, sizeof(line),
					 "\tstat [%ld].req_alloc = %ld;\r\n"
					 "\tstat [%ld].req_free = %ld;\r\n",
					 i,
					 stat[i].req_alloc,
					 i,
					 stat[i].req_free);
				(*fp->dev->write)(fp, line, strlen(line));
			}
		}

		ksprintf(line, sizeof(line), "}\r\n\r\n");
		(*fp->dev->write)(fp, line, strlen(line));

		do_close(rootproc, fp);
	}
}
# else

# ifdef KM_USAGE
static void
km_stat_dump (void)
{
	KM_FORCE((__FILE__ ": used_mem = %li", used_mem));
	KM_FORCE((__FILE__ ": requests: kmr_get = %li, kmr_free = %li",
		kmr_stat.req_alloc,
		kmr_stat.req_free
	));
	KM_FORCE((__FILE__ ": requests: km_s1_malloc = %li, km_s1_free = %li",
		km_s1_stat.req_alloc,
		km_s1_stat.req_free
	));
}
# endif

# endif

/* END start part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN allocation tracer infos */

# ifdef KM_TRACE

struct km_trace
{
	void *ptr;
	unsigned long size;
	const char *func;
};

#define KM_TRACE_LEN 30000
static struct km_trace km_trace[KM_TRACE_LEN];
static long km_trace_used = 0;
static long km_trace_first_free = 0;

static void
km_trace_register(void *ptr, unsigned long size, const char *func)
{
	long i;

	for (i = km_trace_first_free; i < KM_TRACE_LEN; i++)
	{
		if (km_trace[i].ptr == NULL)
		{
			km_trace_used++;

			km_trace[i].ptr = ptr;
			km_trace[i].size = size;
			km_trace[i].func = func;

			return;
		}
	}

	/* no more space to remember allocation */
	{
		static int reported = 10;

		if (reported)
		{
			reported--;
			KM_FORCE(("KM_TRACE buffer to small (or memory leaks?!)"));
		}
	}
}

static void
km_trace_unregister(void *ptr)
{
	long i;

	for (i = 0; i < KM_TRACE_LEN; i++)
	{
		if (km_trace[i].ptr == ptr)
		{
			km_trace_used--;

			km_trace[i].ptr = NULL;

			if (km_trace_first_free > i)
				km_trace_first_free = i;

			break;
		}
	}

	if (i == KM_TRACE_LEN)
	{
		KM_FORCE(("KM_TRACE: unregister failed?!"));
	}
}

static void
km_trace_dump(void)
{
	static char line[SPRINTF_MAX];

	FILEPTR *fp;
	long ret;

  ret = do_open(&fp, rootproc, KM_TRACE_FILE, (O_RDWR | O_CREAT | O_TRUNC), 0, NULL);
  if (!ret)
  {
		long i;

		ksprintf(line, sizeof(line),
			 "P__HEAD = %lu\n"
			 "S1_HEAD = %lu\n"
			 "S2_HEAD = %lu\n"
			 "S1_SIZE = %lu\n\n",
			 P__HEAD, S1_HEAD, S2_HEAD, S1_SIZE);
		(*fp->dev->write)(fp, line, strlen(line));

		ksprintf(line, sizeof(line), "s1_malloc if size <= %li\n", (S1_SIZE - S1_HEAD));
		(*fp->dev->write)(fp, line, strlen(line));

		ksprintf(line, sizeof(line), "s2_malloc if size < %li\n\n",
			 (PAGESIZE - P__HEAD - S2_HEAD - (S1_SIZE - S1_HEAD)));
		(*fp->dev->write)(fp, line, strlen(line));

		for (i = 0; i < KM_TRACE_LEN; i++)
		{
			if (km_trace[i].ptr)
			{
				ksprintf(line, sizeof(line),
					 "[%4li]: %6lu bytes at 0x%08lx from %s\n",
					 i,
					 km_trace[i].size,
					 km_trace[i].ptr,
					 km_trace[i].func);
				(*fp->dev->write)(fp, line, strlen(line));
			}
		}

		do_close(rootproc, fp);
	}
}

# endif

long
km_trace_lookup(void *ptr, char *buf, unsigned long buflen)
{
# ifdef KM_TRACE
	const unsigned long loc = (long)ptr;
	long i;

	KM_FORCE(("km_trace_used: %li", km_trace_used));
	KM_FORCE(("km_trace_first_free: %li", km_trace_first_free));

	for (i = 0; i < KM_TRACE_LEN; i++)
	{
		const unsigned long block = (long)km_trace[i].ptr;

		if (loc >= block && loc < (block + km_trace[i].size))
		{
			ksprintf(buf, buflen,
				 "[%4li]: %6lu bytes at 0x%08lx from %s",
				 i,
				 km_trace[i].size,
				 km_trace[i].ptr,
				 km_trace[i].func);

			return 0;
		}
	}
# endif

	*buf = '\0';
	return ENOENT;
}

/* END allocation tracer part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN debug infos */

# ifdef KMEMDEBUG

# include "global.h"

static FILEPTR *kmemdebug_fp = NULL;
static int kmemdebug_initialized = 0;

int kmemdebug_can_init = 0;

static void
init_kmemdebug (void)
{
	if (kmemdebug_can_init)
	{
		FILEPTR *fp;
		long ret;

		kmemdebug_initialized = 1;

		ret = do_open (&fp, rootproc, KM_DEBUG_FILE, (O_RDWR | O_CREAT | O_TRUNC), 0, NULL);
		if (ret)
		{
			kmemdebug_initialized = 0;
		}
		else
			kmemdebug_fp = fp;
	}
}

static void
print_hex (ulong hex)
{
	static char *hex_digits = "0123456789ABCDEF";
	static char outbuf[10];
	char *cursor = outbuf + 2;
	int shift;

	outbuf[0] = '0';
	outbuf[1] = 'x';

	for (shift = 28; shift >= 0; shift -= 4)
		*cursor++ = hex_digits[0xf & (hex >> shift)];

	(*kmemdebug_fp->dev->write)(kmemdebug_fp, outbuf, 10);
}

# define FPUTS(s)	((*kmemdebug_fp->dev->write)(kmemdebug_fp, (s), strlen (s)))
# define PRINT_HEX(n)	(print_hex ((ulong) n))
# define PRINT_CALLER	(PRINT_HEX ((ulong) __builtin_return_address (0) - \
				((ulong) _base + 0x100UL)))

void *	_cdecl	__real__kcore	(ulong size);
void *	_cdecl	__real__kmalloc	(ulong size);
void	_cdecl	__real__kfree	(void *place);
void *	_cdecl	__real__umalloc	(ulong size);
void	_cdecl	__real__ufree	(void *place);

void *	_cdecl	__wrap__kcore	(ulong size);
void *	_cdecl	__wrap__kmalloc	(ulong size);
void	_cdecl	__wrap__kfree	(void *place);
void *	_cdecl	__wrap__umalloc	(ulong size);
void	_cdecl	__wrap__ufree	(void *place);

static int lock = 0;

void * _cdecl
__wrap__kcore (ulong size)
{
	void *retval;

	lock++;

	retval = __real__kcore (size);

	if (lock == 1)
	{
		if (!kmemdebug_initialized)
			init_kmemdebug ();

		if (kmemdebug_fp)
		{
			FPUTS ("kcore: ");
			PRINT_CALLER;
			FPUTS (": ");
			PRINT_HEX (size);
			FPUTS (" bytes at ");
			PRINT_HEX (retval);
			FPUTS ("\n");
		}
	}

	lock--;
	return retval;
}

void * _cdecl
__wrap__kmalloc (ulong size)
{
	void *retval;

	lock++;

	retval = __real__kmalloc (size);

	if (lock == 1)
	{
		if (!kmemdebug_initialized)
			init_kmemdebug ();

		if (kmemdebug_fp)
		{
			FPUTS ("kmalloc: ");
			PRINT_CALLER;
			FPUTS (": ");
			PRINT_HEX (size);
			FPUTS (" bytes at ");
			PRINT_HEX (retval);
			FPUTS ("\n");
		}
	}

	lock--;
	return retval;
}

void _cdecl
__wrap__kfree (void *place)
{
	lock++;

	if (lock == 1)
	{
		if (!kmemdebug_initialized)
			init_kmemdebug ();

		if (kmemdebug_fp)
		{
			FPUTS ("kfree: ");
			PRINT_CALLER;
			FPUTS (": ");
			FPUTS ("at ");
			PRINT_HEX (place);
			FPUTS ("\n");
		}
	}

	__real__kfree (place);

	lock--;
}

void * _cdecl
__wrap__umalloc (ulong size)
{
	void *retval;

	lock++;

	retval = __real__umalloc (size);

	if (lock == 1)
	{
		if (!kmemdebug_initialized)
			init_kmemdebug ();

		if (kmemdebug_fp)
		{
			FPUTS ("umalloc: ");
			PRINT_CALLER;
			FPUTS (": ");
			PRINT_HEX (size);
			FPUTS (" bytes at ");
			PRINT_HEX (retval);
			FPUTS ("\n");
		}
	}

	lock--;
	return retval;
}

void _cdecl
__wrap__ufree (void *place)
{
	lock++;

	if (lock == 1)
	{
		if (!kmemdebug_initialized)
			init_kmemdebug ();

		if (kmemdebug_fp)
		{
			FPUTS ("ufree: ");
			PRINT_CALLER;
			FPUTS (": ");
			FPUTS ("at ");
			PRINT_HEX (place);
			FPUTS ("\n");
		}
	}

	__real__ufree (place);

	lock--;
}

# endif

# ifdef KMEMORY_DEBUG

# include <stdarg.h>
#if 0
static void
km_debug (const char *fmt, ...)
{
	static char line [SPRINTF_MAX];
	va_list args;

	va_start (args, fmt);

	kvsprintf (line, sizeof (line), fmt, args);
	KM_FORCE ((line));

	va_end (args);
}
#endif

# endif

/* END debug infos */
/****************************************************************************/
