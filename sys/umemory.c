/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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
 * "user" memory allocation routines; the kernel can use these to
 * allocate/free memory that will be attached in some way to a process
 * (and freed automatically when the process exits)
 */

# include "umemory.h"

# include "libkern/libkern.h"
# include "mint/proc.h"
# include "mint/signal.h"

# include "dosmem.h"
# include "info.h"
# include "memory.h"
# include "signal.h"

# include "proc.h"


/****************************************************************************/
/* BEGIN definition part */

struct umem_descriptor
{
	long head_magic;
	struct umem_descriptor *next;
	struct umem_descriptor *prev;
	short free;
	short tail_magic;

};

#define UMEM_HEAD_MAGIC		0x4d694e54L
#define UMEM_TAIL_MAGIC		0x4d69

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN user memory alloc */

static int
umem_check_inside(MEMREGION *m, void *ptr)
{
	unsigned long place = (unsigned long)ptr;

	if (place >= m->loc && place < (m->loc + m->len))
		return 1;

	return 0;
}

static int
umem_verify1(MEMREGION *m, struct umem_descriptor *descr)
{
	if (descr->head_magic != UMEM_HEAD_MAGIC)
		return 0;
	if (descr->tail_magic != UMEM_TAIL_MAGIC)
		return 0;

	if (descr->next && !umem_check_inside(m, descr->next))
		return 0;
	if (descr->prev && !umem_check_inside(m, descr->prev))
		return 0;

	/* magics and pointer ok */
	return 1;
}

static int
umem_verify(MEMREGION *m, struct umem_descriptor *descr)
{
	struct umem_descriptor *next;
	struct umem_descriptor *prev;

	if (!umem_verify1(m, descr))
		return 0;

	/* check list forward */
	prev = descr;
	next = descr->next;
	while (next)
	{
		if (!umem_verify1(m, next))
			return 0;

		if (next->prev != prev)
			return 0;

		prev = next;
		next = next->next;
	}

	/* check list backward */
	next = descr;
	prev = descr->prev;
	while (prev)
	{
		if (!umem_verify1(m, prev))
			return 0;

		if (prev->next != next)
			return 0;

		next = prev;
		prev = prev->prev;
	}

	/* now 'next' must be first descriptor of m */
	if ((long)next != m->loc)
		return 0;

	/* double linked list ok */
	return 1;
}

static struct umem_descriptor *
umem_split(struct umem_descriptor *descr, unsigned long n, unsigned long size)
{
	union { char *c; struct umem_descriptor *d; } ptr;
	struct umem_descriptor *descr1;

	ptr.d = descr;

	descr1 = (void *)(ptr.c + (n - size));

	descr1->head_magic = UMEM_HEAD_MAGIC;
	descr1->next = ptr.d->next;
	descr1->prev = ptr.d;
	descr1->free = 0;
	descr1->tail_magic = UMEM_TAIL_MAGIC;

	if (descr1->next)
		descr1->next->prev = descr1;

	ptr.d->next = descr1;

	return descr1;
}

static int
umem_lookup(struct proc *p, MEMREGION *m, unsigned long size, void **result)
{
	struct umem_descriptor *descr;
	union { char **c; void **v; } resultptr; 
	
	resultptr.v = result;

	/* default is nothing found */
	*resultptr.v = NULL;

	descr = (struct umem_descriptor *)m->loc;

	if (!umem_verify(m, descr))
	{
		ALERT(MSG_umem_mem_corrupted);

		ikill(p->pid, SIGKILL);
		return 0;
	}

	while (descr)
	{
		if (descr->free)
		{
			long n;

			if (descr->next)
				n = (long)descr->next - (long)descr;
			else
				n = (m->loc + m->len) - (long)descr;

			if (n >= size)
			{
				if ((n - size) > (2 * sizeof(*descr)))
				{
					*resultptr.v = umem_split(descr, n, size);
					*resultptr.c += sizeof(*descr);
				}
				else
				{
					descr->free = 0;

					*resultptr.v = descr;
					*resultptr.c += sizeof(*descr);
				}

				break;
			}
		}

		descr = descr->next;
	}

	return 1;
}

static void
umem_setup(MEMREGION *m)
{
	struct umem_descriptor *descr;

	m->mflags |= M_UMALLOC;

	descr = (struct umem_descriptor *)m->loc;
	descr->head_magic = UMEM_HEAD_MAGIC;
	descr->next = NULL;
	descr->prev = NULL;
	descr->free = 1;
	descr->tail_magic = UMEM_TAIL_MAGIC;
}

void * _cdecl
_umalloc(unsigned long size, const char *func)
{
	struct proc *p = get_curproc();
	MEMREGION *m;
	int i;

	DEBUG(("umalloc(%lu, %s)", size, func));

	/* add space for the header */
	size += sizeof(struct umem_descriptor);
	/* align */
	size = (size + 3) & ~3;

	for (i = 0; i < p->p_mem->num_reg; i++)
	{
		m = p->p_mem->mem[i];
		if (m && (m->mflags & M_UMALLOC))
		{
			void *ptr;

			if (!umem_lookup(p, m, size, &ptr))
				return NULL;

			if (ptr)
			{
				DEBUG(("umalloc: found %p in managed region %i", ptr, i));
				return ptr;
			}
		}
	}

	m = get_region(alt, size, PROT_P);
	if (!m) m = get_region(core, size, PROT_P);

	if (m)
	{
		if (attach_region(p, m))
		{
			void *ptr;

			/* NOTE: get_region returns a region with link count 1;
			 * since attach_region increments the link count, we have
			 * to remember to decrement the count to correct for this.
			 */
			m->links--;

			/* initialize new umalloc block */
			umem_setup(m);

			if (!umem_lookup(p, m, size, &ptr))
				return NULL;

			if (ptr)
			{
				DEBUG(("umalloc: allocated new region %i, return %p", i, ptr));
				return ptr;
			}
		}

		m->links = 0;
		free_region(m);
	}

	return NULL;
}

void _cdecl
_ufree(void *plac, const char *func)
{
	union { char *c; void *v; long l; struct umem_descriptor *descr; } placeptr; 
	struct proc *p = get_curproc();
	MEMREGION *m;

	placeptr.v = plac;

	DEBUG(("ufree(0x%p, %s)", plac, func));

	m = proc_addr2region(p, placeptr.l);
	if (m)
	{
		placeptr.c -= sizeof(*placeptr.descr);

		if (placeptr.l >= m->loc)
		{
			if (!umem_verify(m, placeptr.descr))
			{
				ALERT(MSG_umem_mem_corrupted);

				ikill(p->pid, SIGKILL);
				return;
			}

			/* mark as free */
			placeptr.descr->free = 1;

			if (placeptr.descr->prev && placeptr.descr->prev->free)
				placeptr.descr = placeptr.descr->prev;

			while (placeptr.descr->next && placeptr.descr->next->free)
			{
				placeptr.descr->next = placeptr.descr->next->next;
				if (placeptr.descr->next)
					placeptr.descr->next->prev = placeptr.descr;
			}

			/* todo: free m if totally free */
		}
		else
			goto invalid_address;
	}
	else
	{
invalid_address:
		FATAL("ufree: invalid address (0x%lx) for pid %i", placeptr.l, p->pid);
	}
}

/* END user memory alloc */
/****************************************************************************/
