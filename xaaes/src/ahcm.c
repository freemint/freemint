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

/*
 * Thread safe and debug friendly memory allocator
 */

#include <mint/mintbind.h>
#include "ahcm.h"


void display(char *t, ...);

#define DEBUG 0
#define IFIRST 0

#if DEBUG
#define DBG(x) display x
#else
#define DBG(x)
#endif


XA_memory XA_default_base = { NULL, NULL, NULL, 16384, 16, 3 }; /* pref TT-ram */


static void *
new_block(XA_memory *base, long size, bool svcur)
{
	XA_block *new;

	if (base->mode)
		new = (void *) Mxalloc(size, base->mode);
	else
		new = (void *) Malloc(size);

 	if (new)
	{
#if IFIRST
		XA_block *first;
#else
		XA_block *last;
#endif
		XA_unit *fr;

		DBG((" - new_block %ld :: %ld\n", new, size));

#if IFIRST
		first = base->first;
		if (first)
		{
			first->prior = new;
			base->first = new;
			new->prior = NULL;
			new->next = first;
#else
		last = base->last;
		if (last)
		{
			last->next = new;
			base->last = new;
			new->next = NULL;
			new->prior = last;
#endif
		}
		else
		{
			base->last = base->first = new;
			new->next = new->prior = NULL;
		}
		new->used.first = new->used.last = new->used.cur = NULL;
		fr = new->area;
		new->free.first = new->free.last = new->free.cur = fr;
		fr->next = fr->prior = NULL;
		fr->size = size - blockprefix;
		new->size = size;
		if (svcur)
			base->cur = new;
		return new;
	}
	return NULL;
}

static void
free_block(XA_memory *base, XA_block *blk)
{
	XA_block *next = blk->next;
	XA_block *prior = blk->prior;

#if DEBUG
	XA_list *free = &blk->free;
	XA_unit *this = free->first;

	if (!free->first || !free->last)
		DBG(("free_block; wrong free list f:%ld, l:%ld\n", free->first, free->last));
	else if (this->next || this->prior)
		DBG(("free_block; wrong free unit p:%ld, n:%ld\n", this->prior, this->next));
	else if (this->size != blk->size - blockprefix)
		DBG(("free_block; wrong free size: %ld, b:%ld, diff:%ld\n",
			this->size, blk->size, blk->size - (this->size + blockprefix)));
#endif

	if (next)
		next->prior = prior;
	else
		base->last = prior;

	if (prior)
		prior->next = next;
	else
		base->first = next;

	DBG((" - Free block %ld :: %ld\n", blk, blk->size));

	base->cur = prior ? prior : next;
	Mfree(blk);
}

static void
free_unit(XA_list *list, XA_unit *this)
{
	XA_unit *next = this->next;
	XA_unit *prior = this->prior;

	if (next)
		next->prior = prior;
	else
		list->last = prior;

	if (prior)
		prior->next = next;
	else
		list->first = next;

	list->cur = prior ? prior : next;
}

static XA_block *
find_free(XA_memory *base, long s)
{
	XA_block *blk;

	blk = base->cur;
	if (blk)
	{
		if (blk->free.cur)
		{
			if (blk->free.cur->size >= s)
			{
				DBG((" - f0 %ld %ld:: %ld\n",blk, blk->free.cur, blk->free.cur->size));
				return blk;
			}
		}
	}

	blk = base->first;
	while (blk)
	{
		XA_unit *fr = blk->free.first;
		while (fr)
		{
			DBG((" - f1 %ld :: %ld\n", fr, fr->size));
			if (fr->size >= s)
			{
				base->cur = blk;
				blk->free.cur = fr;
				return blk;
			}
			fr = fr->next;
		}
		blk = blk->next;
	}
	return NULL;
}

static XA_unit *
split(XA_list *list, long s)
{
	XA_unit *cur = list->cur;
	long l = cur->size - s;
	if (l > 2*unitprefix)
	{
		XA_unit *new = cur,
	            *next = cur->next;
		(long)new += s;
		new->next = next;
		new->prior = cur;
		if (next)
			next->prior = new;
		else
			list->last = new;
		cur->next = new;
	
		new->size = l;
		cur->size = s;
		new->key = -1;
		new->type = -1;
		list->cur = new;		/* last split off becomes cur. */
	}
#if DEBUG
	else
		DBG(("Not split %ld :: %ld\n", cur, cur->size));
#endif
	return cur;
}

static void
insfirst(XA_list *list, XA_unit *this)
{
	DBG(("insfirst %ld :: %ld\n", this, this->size));

	list->cur = this;
	this->prior = NULL;
	this->next = list->first;

	if (list->first)
		list->first->prior = this;
	else					/* if last is NULL, first also NULL */
		list->last = this;

	list->first = this;
}

static void
insbefore(XA_list *list, XA_unit *this, XA_unit *before)
{
	DBG(("insbefore %ld\n", before));

	this->next = before;

	if (before->prior)
		before->prior->next = this;
	else
		list->first = this;

	this->prior = before->prior;
	before->prior = this;
}

static void
inslast(XA_list *list, XA_unit *this)
{
	DBG(("inslast %ld :: %ld\n", this, this->size));

	list->cur = this;
	this->next = NULL;
	this->prior = list->last;

	if (list->last)
		list->last->next = this;
	else					/* if first is NULL, last also NULL */
		list->first = this;

	list->last = this;
}

static void
insafter(XA_list *list, XA_unit *this, XA_unit *after)
{
	DBG(("insafter %ld\n", after));

	this->prior = after;

	if (after->next)
		after->next->prior = this;
	else
		list->last = this;

	this->next = after->next;
	after->next = this;
}

static void
amalgam(XA_list *list, XA_unit *this)
{
	if (this->next)
	{
		XA_unit *next = this->next;
		if ((long) this + this->size == (long) next)
		{
			DBG((" - combine %ld :: %ld + %ld :: %ld = %ld :: %ld\n",
				this, this->size, next, next->size, this, this->size + next->size));
			this->size += next->size;
			next = next->next;
			if (next)
				next->prior = this;
			else
				list->last = this;
			this->next = next;
		}
	}
	list->cur = this;
}

static void
sortins(XA_list *list, XA_unit *this)
{
	XA_unit *have;
	DBG(("sortins %ld\n", this));

	have = list->cur;
	if (!have)
		have = list->first;

	if (!have)
	{
		DBG((" - c0\n"));
		insfirst(list, this);
	}
	else if (have->area < this->area)
	{
		DBG((" - c1\n"));
		while (have && have->area < this->area)
			have = have->next;
		if (!have)
			inslast(list, this);
		else
			insbefore(list, this, have);
	}
	else
	{
		DBG((" - c2\n"));
		while (have && have->area > this->area)
			have = have->prior;
		if (!have)
			insfirst(list, this);
		else
			insafter(list, this, have);
	}
}

static void
combine(XA_list *list, XA_unit *this)
{
	DBG((" - a1\n"));
	amalgam(list, this);
	if (this->prior)
	{
		DBG((" - a2\n"));
		amalgam(list, this->prior);
	}
}

/* The smaller the requested area, the less likely it is
 * allocated & freed only once in a while.
 */
void *
XA_alloc(XA_memory *base, size_t size, XA_key key, XA_key type)
{
	XA_block *blk = NULL;
	long s = ((size + 3) & -4) + unitprefix;

	if (!base)
		base = &XA_default_base;

	if (s > (base->chunk - base->head - blockprefix))
	{
		long bls = s + blockprefix;
		long ns = 0;

		while (ns < bls)
			ns+= base->chunk;

		blk = new_block(base, ns - base->head, false);
		if (!blk)
			return NULL;
	}
	else
		blk = find_free(base, s);

	if (!blk)
		blk = new_block(base, base->chunk - base->head, true);

	if (blk)
	{
		XA_unit *free = split(&blk->free, s);
		free_unit(&blk->free, free);
#if IFIRST
		insfirst(&blk->used, free);
#else
		inslast(&blk->used, free);
#endif
		free->key = key;
		free->type = type;
		return free->area;
	}
	return NULL;
}

void *
XA_calloc(XA_memory *base, size_t items, size_t chunk, XA_key key, XA_key type)
{
	void *new = NULL;
	long l;

	l = items*chunk;
	if (l)
	{
		new = XA_alloc(base, l, key, type);
		if (new)
			bzero(new,l);
	}
	return new;
}

extern XA_report punit;

static void
free_area(XA_memory *base, XA_block *this, XA_unit *at)
{
	DBG((" - free_unit\n"));
	free_unit(&this->used, at);
#if DEBUG
	sortins(&this->free, at);
	combine(&this->free, at);
	if (this->used.first == NULL)
	{
		XA_sanity(base, punit);
		free_block(base, this);
	}
#else
	if (this->used.first == NULL)
		free_block(base, this);
	else
		sortins(&this->free, at),
		combine(&this->free, at);
#endif
}

void
XA_free(XA_memory *base, void *area)
{
	XA_block *blk;

	if (!area)
		return;

	if (!base)
		base = &XA_default_base;

	blk = base->cur;
	if (blk)
	{
		if (blk->used.cur)
		{
			if (blk->used.cur->area == area)
			{
				DBG(("blk free cur %ld %ld:: %ld\n",
					blk, blk->used.cur, blk->used.cur->size));

				free_area(base, blk, blk->used.cur);
				return;
			}
		}
	}

	blk = base->first;
	DBG((" - bfirst %ld\n", blk));
	while (blk)
	{
		long x = (long) area;
		long b = (long) blk->area + unitprefix;

		if (   x >= b
		    && x < b + blk->size - (blockprefix + unitprefix))
		{
			XA_unit *at;

			if (blk->used.cur)
			{
				if (blk->used.cur->area == area)
				{
					DBG(("free cur %ld %ld:: %ld\n",
						blk, blk->used.cur, blk->used.cur->size));

					free_area(base, blk, blk->used.cur);
					return;
				}
			}

			at = blk->used.first;
			while (at)
			{
				if (at->area == area)
				{
					free_area(base, blk, at);
					return;
				}
				at = at->next;
			}
		}
		blk = blk->next;
	}
}

static bool
XA_match(XA_unit *at, XA_key key, XA_key type)
{
	bool m = (key  == -1 || (key  != -1 && key  == at->key))
		  && (type == -1 || (type != -1 && type == at->type));

	return m;
}

void
XA_free_all(XA_memory *base, XA_key key, XA_key type)
{
	XA_block *blk;

	if (!base)
		base = &XA_default_base;

	blk = base->first;

	/* So I can detect leaking */
#if !GENERATE_DIAGS
	if (key == -1 && type == -1)
	{
		while (blk)
		{
			XA_block *next = blk->next;
			Mfree(blk);
			blk = next;
		}
		base->first = base->last = base->cur = NULL;
	}
	else
#endif
	{
		while (blk)
		{
			XA_block *bx = blk->next;
			XA_unit *at = blk->used.first;
			while (at)
			{
				XA_unit *ax = at->next;
				if (XA_match(at, key, type))
					free_area(base, blk, at);
				at = ax;
			}
			blk = bx;
		}
	}
}

bool
XA_leaked(XA_memory *base, XA_key key, XA_key type, XA_report *report)
{
	bool reported = false;
	XA_block *blk;

	if (!base)
		base = &XA_default_base;

	blk = base->first;
	while (blk)
	{
		XA_unit *at = blk->used.first;
		while (at)
		{
			if (XA_match(at, key, type))
			{
				report(base, blk, at, "leak");
				reported = true;
			}
			at = at->next;
		}
		blk = blk->next;
	}
	return reported;
}

struct record
{
	long size;
	char d[0];
};

#if DEBUG
static void
XA_follow(XA_memory *base, XA_block *blk, XA_list *list, XA_report *report)
{
	/* Go up and down the list, finish at the same address. */
	XA_unit *un, *n;

	un = list->first;
	n = un;
	while (un)
	{
		n = un;
		un = un->next;
	}
	if (n != list->last)
	{
		report(base, blk, n, "last found");
		report(base, blk, list->last, "list->last");
	}

	un = list->last;
	n = un;
	while (un)
	{
		n = un;
		un = un->prior;
	}
	if (n != list->first)
	{
		report(base, blk, n, "first found");
		report(base, blk, list->first, "list->first");
	}
}

void
XA_sanity(XA_memory *base, XA_report *report)
{
	XA_block *blk;

	DBG(("\nSanity check\n\n"));

	if (!base)
		base = &XA_default_base;

	/* follow sizes and see if they fit. */
	blk = base->first;
	while (blk)
	{
		struct record *at = (struct record *) blk;
		struct record *to = (struct record *) blk;
		struct record *pr;

		at += blockprefix;
		to += blk->size;

		while (at < to)
		{
			pr = at;
			at += at->size;
		}

		if (at != to)
			report(base, blk, (XA_unit *)pr, "insane");

		DBG(("Follow free list\n"));
		XA_follow(base, blk, &blk->free, punit);

		DBG(("Follow used list\n"));
		XA_follow(base, blk, &blk->used, punit);

		blk = blk->next;
	}
}
#endif /* DEBUG */

void
XA_set_base(XA_memory *base, size_t chunk, short head, short flags)
{
	if (!base)
		base = &XA_default_base;

	base->first = base->last = base->cur = NULL;

	if (!chunk)
		base->chunk = XA_default_base.chunk;
	else if (chunk > 0)
		base->chunk = chunk;
	else
		base->chunk = 8192;

	if (!head)
		base->head = XA_default_base.head;
	else if (head > 0)
		base->head = head;
	else
		base->head = 16;

	base->mode = flags;
}
