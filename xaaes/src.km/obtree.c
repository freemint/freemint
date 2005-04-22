/*
 * $Id$
 *
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

#include "draw_obj.h"
#include "obtree.h"
#include "scrlobjc.h"
#include "rectlist.h"
#include "xa_global.h"
#include "k_mouse.h"

#include "mint/signal.h"


static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

inline OBSPEC *
object_get_spec(OBJECT *ob)
{
	if (ob->ob_flags & OF_INDIRECT)
	{
		return ob->ob_spec.indirect;
	}
	else
	{
		return &ob->ob_spec;
	}
}

bool
object_have_spec(OBJECT *ob)
{
	switch (ob->ob_type & 0xff)
	{
		case G_BOX:
		case G_IBOX:
		case G_BOXCHAR:
		return true;
	}
	return false;
}

bool
validate_obtree(struct xa_client *client, OBJECT *obtree, char *fdesc)
{
	if ((long)obtree <= 0x1000L)
	{
			/* inform user what's going on */
		ALERT(("%s: validate OBTREE for %s failed, object ptr = %lx, killing it!", fdesc, client->proc_name, obtree));
		raise(SIGKILL);
		return false;
	}
	return true;
}

/*
 * HR change ob_spec
 */
inline void
object_set_spec(OBJECT *ob, unsigned long cl)
{
	if (ob->ob_flags & OF_INDIRECT)
		ob->ob_spec.indirect->index = cl;
	else
		ob->ob_spec.index = cl;
}

inline bool
object_is_editable(OBJECT *ob)
{
	return ob->ob_flags & OF_EDITABLE ? true : false;
}

inline TEDINFO *
object_get_tedinfo(OBJECT *ob)
{
	return object_get_spec(ob)->tedinfo;
}

inline struct scroll_info *
object_get_slist(OBJECT *ob)
{
	return (struct scroll_info *)object_get_spec(ob)->index;
}

inline void
object_deselect(OBJECT *ob)
{
	ob->ob_state &= ~OS_SELECTED;
}

/* A quick hack to catch *most* of the problems with transparent objects */
bool
object_is_transparent(OBJECT *ob)
{
	bool ret = false;

	switch (ob->ob_type & 0xff)
	{
#if 0
		case G_BOX:
		case G_BOXCHAR:
		{
			if (!(*(BFOBSPEC *)object_get_spec(ob)).fillpattern)
				ret = true;
			break;
		}
#endif	
		case G_STRING:
		case G_SHORTCUT:
		case G_IBOX:
		case G_TITLE:
		case G_PROGDEF:
		{
			ret = true;
			break;
		}
		case G_TEXT:
		case G_FTEXT:
		{
			if (!(*(BFOBSPEC *)&object_get_tedinfo(ob)->te_just).textmode)
				ret = true;
			break;
		}
#if 0
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			//if (!(*(BFOBSPEC *)&object_get_tedinfo(ob)->te_just).fillpattern)
				ret = true;
			break;
		}
#endif
	}

	/* opaque */
	return ret;
}

CICON *
getbest_cicon(CICONBLK *ciconblk)
{
	CICON	*c, *best_cicon = NULL;

	c = ciconblk->mainlist;
	while (c)
	{
		/* Jinnee v<2.5 has misunderstood the next_res NULL rule :( */
		if ( c == (CICON*)-1 ) break;

		if (c->num_planes <= screen.planes
		    && (!best_cicon || (best_cicon && c->num_planes > best_cicon->num_planes)))
		{
			best_cicon = c;
		}
		c = c->next_res;	
	}
	/* No matching icon, so use the mono one instead */
	return best_cicon;
}

static long
sizeof_tedinfo(TEDINFO *ted)
{
	long size = sizeof(TEDINFO);
	size += strlen(ted->te_ptext) + 1;
	size += strlen(ted->te_ptmplt) + 1;
	size += strlen(ted->te_pvalid) + 1;
	DIAGS(("sizeof_tedinfo: %ld", size));
	return size;
}
static long
sizeof_bitblk(BITBLK *bb)
{
	long size = sizeof(BITBLK);
	size += 2L * bb->bi_wb * bb->bi_hl;
	DIAGS(("sizeof_bitblk: %ld", size));
	return size;
}
static long
sizeof_iconblk(ICONBLK *ib)
{
	long size = 3L * calc_back((RECT *)&ib->ib_xicon, 1);
	size += sizeof(ICONBLK);
	if (ib->ib_ptext)
		size += strlen(ib->ib_ptext) + 1;
	
	DIAGS(("sizeof_iconblk: %ld", size));
	return size;
}
static long
sizeof_cicon(ICONBLK *ib, CICON *i)
{
	long size, len, plen;

	len = calc_back((RECT *)&ib->ib_xicon, 1);

	size = sizeof_iconblk(ib);
	plen = len * i->num_planes;
	
	if (i->col_data)
		size += plen;
	if (i->col_mask)
		size += plen;
	if (i->sel_data)
		size += plen;
	if (i->sel_mask)
		size += plen;

	size += sizeof(CICONBLK) + sizeof(CICON);

	DIAGS(("sizeof_cicon: (planes %d, len %ld, plen %ld), %ld", i->num_planes, len, plen, size));
	return size;
}

static long
obtree_len(OBJECT *obtree, short start, short *num_objs)
{
	long size;
	short parent, curr = start, objs = 0;

	parent = ob_get_parent(obtree, start);
	
	DIAGS(("obtree_len: tree = %lx, start = %d, parent = %d", obtree, start, parent));

	size = 0;
	do
	{
		OBJECT *ob = obtree + curr;
		short type = ob->ob_type & 0x00ff;

		DIAGS((" -- ob %d, type = %d", curr, type));
		size += sizeof(OBJECT);
		objs++;

		switch (type)
		{
			case G_BOX:
			case G_IBOX:
			case G_BOXCHAR:
			{
				break;
			}
			case G_TEXT:
			case G_BOXTEXT:
			case G_FTEXT:
			case G_FBOXTEXT:
			{
				size += sizeof_tedinfo(object_get_tedinfo(ob));
				break;
			}
			case G_IMAGE:
			{
				size += sizeof_bitblk(object_get_spec(ob)->bitblk);
				break;
			}
			case G_PROGDEF:
			{
				size += sizeof(USERBLK) + sizeof(PARMBLK);
				break;
			}
			case G_BUTTON:
			case G_STRING:
			case G_TITLE:
			{
				size += strlen(object_get_spec(ob)->free_string) + 1;
				break;
			}
			case G_ICON:
			{
				size += sizeof_iconblk(object_get_spec(ob)->iconblk);
				break;
			}
			case G_CICON:
			{
				CICONBLK *cb;
				ICONBLK *ib;
				CICON *i;

				cb = object_get_spec(ob)->ciconblk;
				ib = (ICONBLK *)&cb->monoblk;
				i  = NULL; //getbest_cicon(cb);
				
				if (i)
				{
					size += sizeof_cicon(ib, i);
				}
				else
				{
					i = cb->mainlist;
					while (i && i != (void *)-1L)
					{					
						size += sizeof_cicon(ib, i);
						i = i->next_res;
					}
				}
				break;
			}
			case G_SLIST:
			{
				break;
			}
			default:
			{
				DIAGS(("Unknown object type %d", ob->ob_type));
				break;
			}
		}
		if (ob->ob_head != -1)
		{
			short no;
			size += obtree_len(obtree, ob->ob_head, &no);
			objs += no;
		}
	
	} while ((curr = obtree[curr].ob_next) != parent);

	DIAGS(("return obtreelen = %ld, objs = %d", size, objs));
	
	if (num_objs)
		*num_objs = objs;
	
	return size;
}
static void *
copy_tedinfo(TEDINFO *src, TEDINFO *dst)
{
	char *s = (char *)((long)dst + sizeof(TEDINFO));

 	DIAGS(("copy_tedinfo: copy from %lx, to %lx", src, dst));
	
	*dst = *src;

	dst->te_ptext = s;
	strcpy(s, src->te_ptext);
	s += strlen(src->te_ptext) + 1;

	dst->te_ptmplt = s;
	strcpy(s, src->te_ptmplt);
	s += strlen(src->te_ptmplt) + 1;

	dst->te_pvalid = s;
	strcpy(s, src->te_pvalid);
	s += strlen(src->te_pvalid) + 1;
	
 	DIAGS((" --- return %lx", ((long)s + 1) & 0xfffffffe));
	
	return (void *)(((long)s + 1) & 0xfffffffe);
}
static void *
copy_bitblk(BITBLK *src, BITBLK *dst)
{
	int i;
	short *s, *d;

	DIAGS(("copy_bitblk: from %lx, to %lx", src, dst));
	
	*dst = *src;
	
	s = src->bi_pdata;
 	d = (short *)((long)dst + sizeof(BITBLK));
	dst->bi_pdata = d;
	for (i = 0; i < (src->bi_wb * src->bi_hl); i ++)
		*d++ = *s++;

	DIAGS((" --- return %lx", d));

	return (void *)d;
}
static void *
copy_iconblk(ICONBLK *src, ICONBLK *dst)
{
	char *d;
	int words, i;

	DIAGS(("copy_iconblk: copy from %lx to %lx", src, dst));

	*dst = *src;

	d = (char *)((long)dst + sizeof(ICONBLK));
	words = ((src->ib_wicon + 15) >> 4) * src->ib_hicon;
	{
		short *fr;
		
		fr = src->ib_pmask;
		dst->ib_pmask = (short *)d;
		for (i = 0; i < words; i++)
			*(short *)d++ = *fr++;

		fr = src->ib_pdata;
		dst->ib_pdata = (short *)d;
		for (i = 0; i < words; i++)
			*(short *)d++ = *fr++;
	}
	
	dst->ib_ptext = d;
	for (i = 0; i < 12; i++)
	{
		if (!(*d++ = src->ib_ptext[i]))
			break;
	}
	
	DIAGS((" --- return %lx", ((long)d + 1) & 0xfffffffe));

	return (void *)(((long)d + 1) & 0xfffffffe);	
}

static void *
copy_cicon(CICON *src, CICON *dst, long words)
{
	short *fr, *d = (short *)((long)dst + sizeof(CICON));
	long w;

	DIAGS(("copy_cicon: from %lx to %lx(%lx), words %ld, planes %d", src, dst, d, words, src->num_planes));

	dst->num_planes	= src->num_planes;
	words *= src->num_planes;

	if (src->col_data)
	{
		fr = src->col_data;
		dst->col_data = d;
		for (w = 0; w < words; w++)
			*d++ = *fr++;
	}
	else
		dst->col_data = NULL;

	if (src->col_mask)
	{
		fr = src->col_mask;
		dst->col_mask = d;
		for (w = 0; w < words; w++)
			*d++ = *fr++;
	}
	else
		dst->col_mask = NULL;

	if (src->sel_data)
	{
		fr = src->sel_data;
		dst->sel_data = d;
		for (w = 0; w < words; w++)
			*d++ = *fr++;
	}
	else
		dst->sel_data = NULL;

	if (src->sel_mask)
	{
		fr = src->sel_mask;
		dst->sel_mask = d;
		for (w = 0; w < words; w++)
			*d++ = *fr++;
	}
	else
		dst->sel_mask = NULL;

	dst->next_res = NULL;
	
	DIAGS(("copy_cicon: return %lx", d));
	
	return (void *)d;
}

static void *
copy_ciconblk(CICONBLK *src, CICONBLK *dst, CICON *cicon)
{
	char *d;
	int words, i;

	DIAGS(("copy_ciconblk: from %lx to %lx - cicon=%lx", dst, src, cicon));

	*dst = *src;

	d = (char *)((long)dst + sizeof(CICONBLK));
	
	words = ((src->monoblk.ib_wicon + 15) >> 4) * src->monoblk.ib_hicon;
	{
		short *fr;
		
		fr = src->monoblk.ib_pmask;
		dst->monoblk.ib_pmask = (short *)d;
		for (i = 0; i < words; i++)
			*(short *)d++ = *fr++;

		fr = src->monoblk.ib_pdata;
		dst->monoblk.ib_pdata = (short *)d;
		for (i = 0; i < words; i++)
			*(short *)d++ = *fr++;

	}
	dst->monoblk.ib_ptext = d;
	for (i = 0; i < 12; i++)
	{
		if (!(*d++ = src->monoblk.ib_ptext[i]))
			break;
	}

	(long)d = ((long)d + 1) & 0xfffffffe;
	
	/*
	 * If a cicon is passed to us, we only take that one into the copy,
	 * ignoring the others in the source.
	 * If no cicon is passed here, however, we copy the whole thing.
	 */
	if (cicon)
	{
		dst->mainlist = (CICON *)d;
		d = copy_cicon(cicon, (CICON *)d, words);
	}
	else
	{
		CICON *dst_ci;
		CICON **nxt_sci = &src->mainlist;
		CICON **nxt_dci = &dst->mainlist;

		while (*nxt_sci && *nxt_sci != (void *)-1L)
		{
			dst_ci = (CICON *)d;
			d = copy_cicon(*nxt_sci, dst_ci, words);
			*nxt_dci = dst_ci;
			
			nxt_sci = &((*nxt_sci)->next_res);
			nxt_dci = &((*nxt_dci)->next_res);
		}
	}
	
	DIAGS((" --- return %lx", d));

	return (void *)d;	
}
	
static void
copy_obtree(OBJECT *obtree, short start, OBJECT *dst, void **data)
{
	short parent, curr = start;

	parent = ob_get_parent(obtree, start);

	DIAGS(("copy_obtree: tree = %lx, start = %d, parent = %d, data=%lx/%lx", obtree, start, parent, data, *data));

	do
	{
		OBJECT *ob = obtree + curr;
		OBJECT *on = dst + curr;

		short type = ob->ob_type & 0x00ff;

		DIAGS((" -- ob %d, type = %d, old=%lx, new=%lx", curr, type, ob, on));
		*on = *ob;
		switch (type)
		{
			case G_BOX:
			case G_IBOX:
			case G_BOXCHAR:
			{
				break;
			}
			case G_TEXT:
			case G_BOXTEXT:
			case G_FTEXT:
			case G_FBOXTEXT:
			{
				on->ob_spec.tedinfo = *data;
				*data = copy_tedinfo(object_get_tedinfo(ob), *data);
				break;
			}
			case G_IMAGE:
			{
				on->ob_spec.bitblk = *data;
				*data = copy_bitblk(object_get_spec(ob)->bitblk, *data);
				break;
			}
			case G_PROGDEF:
			{
				on->ob_spec.userblk = *data;
				(USERBLK *)*data = (USERBLK *)object_get_spec(ob)->userblk;
				(long)*data += sizeof(USERBLK);
				break;
			}
			case G_BUTTON:
			case G_STRING:
			case G_TITLE:
			{
				char *s = object_get_spec(ob)->free_string;
				char *d = (char *)*data;
				on->ob_spec.free_string = d;
				strcpy(d, s);
				d += strlen(s) + 1;				
				*data = (void *)(((long)d + 1) & 0xfffffffe);
				break;
			}
			case G_ICON:
			{
				on->ob_spec.iconblk = (ICONBLK *)*data;
				*data = copy_iconblk(object_get_spec(ob)->iconblk, *data);
				break;
			}
			case G_CICON:
			{
				CICON *i;

				i = NULL; //getbest_cicon(object_get_spec(ob)->ciconblk);
				
				on->ob_spec.ciconblk = *data;
				*data = copy_ciconblk(object_get_spec(ob)->ciconblk, *data, i);

				break;
			}
			case G_SLIST:
			{
				*on = *ob;
				on->ob_type = G_BOX;
				*(BFOBSPEC *)&on->ob_spec.obspec = (BFOBSPEC){0,1,1,1,1,0,0};
				break;
			}
			default:
			{
				DIAGS(("Unknown object type %d", ob->ob_type));
				break;
			}
		}
		if (ob->ob_head != -1)
		{
			copy_obtree(obtree, ob->ob_head, dst, data);
		}
	
	} while ((curr = obtree[curr].ob_next) != parent);
}

OBJECT *
duplicate_obtree(struct xa_client *client, OBJECT *obtree, short start)
{
	long size;
	short objs;
	OBJECT *new;
	void *data;
	
	size = obtree_len(obtree, start, &objs);
	DIAGS(("final obtreelen with %d objs is %ld\n", objs, size));

	if (client == C.Aes)
		new = kmalloc(size + 1024);
	else
		new = umalloc(size + 1024);
	if (new)
	{
		bzero(new, size);

		(long)data = (long)new + ((long)objs * sizeof(OBJECT));

		copy_obtree(obtree, 0, new, &data);
	}
	return new;
}

void
free_obtree_resources(struct xa_client *client, OBJECT *obtree)
{
	int j = 0;
	short type;
	
	do
	{
		OBJECT *ob = obtree + j;

		type = ob->ob_type & 0x00ff;
		
		switch (type)
		{
			case G_TEXT:
			case G_BOXTEXT:
			case G_IMAGE:
			case G_BUTTON:
			case G_STRING:
			case G_SHORTCUT:
			case G_FTEXT:
			case G_FBOXTEXT:
			case G_TITLE:
			case G_ICON:
			{
				break;
			}
			case G_CICON:
			{
				break;
			}
			case G_PROGDEF:
			{
				break;
			}
			case G_IBOX:
			case G_BOX:
			case G_BOXCHAR:
			{
				break;
			}
			case G_SLIST:
			{
				struct scroll_info *list = object_get_slist(ob);
				list->destroy(list);
				break;
			}
			case G_EXTBOX:
			{
				struct extbox_parms *p;
				p = (struct extbox_parms *)object_get_spec(ob)->index;
				object_set_spec(ob, p->obspec);
				ob->ob_type = p->type;
				DIAG((D_rsrc, client, "Free extbox parameter bloc %lx", p));
				kfree(p);
				break;
			}
			default:
			{
				DIAG((D_rsrc, client, "Unknown object type %d", type));
				break;
			}
		}
	} while (!(obtree[j++].ob_flags & OF_LASTOB));
}

void
free_object_tree(struct xa_client *client, OBJECT *obtree)
{
	if (obtree)
	{
		DIAG((D_objc, client, "free_object_tree: %lx for %s", obtree, client->name));
		free_obtree_resources(client, obtree);
		if (client == C.Aes)
			kfree(obtree);
		else
			ufree(obtree);
	}
}

short
object_thickness(OBJECT *ob)
{
	short t = 0, flags;
	TEDINFO *ted;

	switch(ob->ob_type & 0xff)
	{
		case G_BOX:
		case G_IBOX:
		case G_BOXCHAR:
		{
			t = object_get_spec(ob)->obspec.framesize;
			break;
		}
		case G_BUTTON:
		{
			flags = ob->ob_flags;
			t = -1;
			if (flags & OF_EXIT)
				t--;
			if (flags & OF_DEFAULT)
				t--;
			break;
		}
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			ted = object_get_tedinfo(ob);
			t = (signed char)ted->te_thickness;
		}
	}
	return t;
}

void
object_offsets(OBJECT *ob, RECT *c)
{
	short dx = 0, dy = 0, dw = 0, dh = 0, db = 0;
	short thick;
	
	if (ob->ob_type != G_PROGDEF)
	{
		thick = object_thickness(ob);   /* type dependent */

		if (thick < 0)
			db = thick;

		/* HR 0080801: oef oef oef, if foreground any thickness has the 3d enlargement!! */
		if (d3_foreground(ob)) 
			db -= 2;

		dx = db;
		dy = db;
		dw = 2 * db;
		dh = 2 * db;

		if (ob->ob_state & OS_OUTLINED)
		{
			dx = min(dx, -3);
			dy = min(dy, -3);
			dw = min(dw, -6);
			dh = min(dh, -6);
		}

		/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
		if (thick < 0 && ob->ob_state & OS_SHADOWED)
		{
			dw += 2 * thick;
			dh += 2 * thick;
		}
	}
	c->x = dx;
	c->y = dy;
	c->w = dw;
	c->h = dh;
}	

/*
 * Returns the object number of this object's parent or -1 if it is the root
 */
short
ob_get_parent(OBJECT *obtree, short obj)
{
	if (obj)
	{
		short last;

		do
		{
			last = obj;
			obj = obtree[obj].ob_next;
		} while (obtree[obj].ob_tail != last);

		return obj;
	}

	return -1;
}

short
ob_remove(OBJECT *obtree, short object)
{
	int parent, current, last;

	current = object;

	do
	{
		/* Find parent */
		last = current;
		current = obtree[current].ob_next;

	} while (obtree[current].ob_tail != last);

	parent = current;
	
	if (obtree[parent].ob_head == object)
	{
		/* First child */

		if (obtree[object].ob_next == parent)
			/* No siblings */
			obtree[parent].ob_head = obtree[parent].ob_tail = -1;
		else
			/* Siblings */
			obtree[parent].ob_head = obtree[object].ob_next;
	}
	else
	{
		/* Not first child */

		current = obtree[parent].ob_head;
		do {
			/* Find adjacent sibling */
			last = current;
			current = obtree[current].ob_next;
		}
		while (current != object);
		obtree[last].ob_next = obtree[object].ob_next;

		if (obtree[object].ob_next == parent)
			/* Last child removed */
			obtree[parent].ob_tail = last;
	}
	return parent;
}

short
ob_add(OBJECT *obtree, short parent, short aobj)
{		
	short last_child;

	if (aobj == parent || aobj == -1 || parent == -1)
		return 0;
	else
	{
		last_child = obtree[parent].ob_tail;
		obtree[aobj].ob_next = parent;
		obtree[parent].ob_tail = aobj;
		if (last_child == -1)	/* No siblings */
			obtree[parent].ob_head = aobj;
		else
			obtree[last_child].ob_next = aobj;

/* 
	JH: 04/15/2003 Incompatible with N.AES, TOS, etc... 
	regarding to the objc_add documentation ob_head, ob_tail
	has to be initialized by the application

		obtree[new_child].ob_head = -1;
		obtree[new_child].ob_tail = -1;
*/
	}
	return 1;
}

void
ob_order(OBJECT *root, short object, ushort pos)
{
	short parent = ob_remove(root, object);

	short current = root[parent].ob_head;

	if (current == -1)		/* No siblings */
	{
		root[parent].ob_head = root[parent].ob_tail = object;
		root[object].ob_next = parent;
	}
	else if (!pos)			/* First among siblings */
	{
		root[object].ob_next = current;
		root[parent].ob_head = object;
	}
	else				/* Search for position */
	{
		for (pos--; pos && root[current].ob_next != parent; pos--)
			current = root[current].ob_next;
		if (root[current].ob_next == parent)
			root[parent].ob_tail = object;
		root[object].ob_next = root[current].ob_next;
		root[current].ob_next = object;
	}
}

/*
 * Find object whose flags is set to 'f', state set to 's' and
 * have none of 'mf' flags set and status is none of 'ms'.
 * Stop searching when object with any of the flags 'stopf' or
 * has any of 'stops' state bits set.
 *
 * Note that ALL flags of 'f' and ALL bits set in 's' must be set
 * in object for it to find the object.
 *
 * Also not that ONE OF the bits in 'mf', 'ms', 'stopf' or 'stops'
 * is enough to cancel search.
 */
short
ob_find_flag(OBJECT *tree, short f, short mf, short stopf)
{
	int o = -1;

	do
	{
		o++;
		if ((!f || (tree[o].ob_flags & f) == f))
		{
			if ((!mf || !(tree[o].ob_flags & mf)))
				return o;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)));

	return -1;
}
short
ob_find_any_flag(OBJECT *tree, short f, short mf, short stopf)
{
	short o = -1;

	do
	{
		o++;
		if ((!f || (tree[o].ob_flags & f)))
		{
			if ((!mf || !(tree[o].ob_flags & mf)))
				return o;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)));

	return -1;
}
/*
 * Count objects whose flags equals those in 'f',
 * and have none of the flags in 'mf' set.
 * Stop search at obj with any flags of 'stopf' is set.
 * counted objects written to 'count'
 * Returns index of stop object
 */
short
ob_count_flag(OBJECT *tree, short f, short mf, short stopf, short *count)
{
	short o = -1, cnt = 0;

	do
	{
		o++;
		if ((!f || (tree[o].ob_flags & f) == f))
		{
			if ((!mf || !(tree[o].ob_flags & mf)))
				cnt++;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)));

	if (count)
		*count = cnt;

	return o;
}
/*
 * Count objects who has any 'f' bit(s) set,
 * and have none of the flags in 'mf' set.
 * Stop search at obj who has any of 'stopf' bits set.
 * counted objects written to 'count'
 * Returns index of stop object
 */
short
ob_count_any_flag(OBJECT *tree, short f, short mf, short stopf, short *count)
{
	short o = -1, cnt = 0;
	do
	{
		o++;
		if ((!f || (tree[o].ob_flags & f)))
		{
			if ((!mf || !(tree[o].ob_flags & mf)))
				cnt++;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)));

	if (count)
		*count = cnt;

	return o;
}

short
ob_find_flst(OBJECT *tree, short f, short s, short mf, short ms, short stopf, short stops)
{
	int o = -1;

	do
	{
		o++;
		if ( (!f || (tree[o].ob_flags & f) == f) && (!s || (tree[o].ob_state & s) == s) )
		{
			if ( (!mf || !(tree[o].ob_flags & mf)) && (!ms || !(tree[o].ob_state & ms)) )
				return o;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)) && (!stops || !(tree[o].ob_state & stops)) );

	return -1;
}
short
ob_find_any_flst(OBJECT *tree, short f, short s, short mf, short ms, short stopf, short stops)
{
	short o = -1;

	do
	{
		o++;
		if ( (!f || (tree[o].ob_flags & f)) && (!s || (tree[o].ob_state & s)) )
		{
			if ( (!mf || !(tree[o].ob_flags & mf)) && (!ms || !(tree[o].ob_state & ms)) )
				return o;
		}
	} while ( (!stopf || !(tree[o].ob_flags & stopf)) && (!stops || !(tree[o].ob_state & stops)) );

	return -1;
}

short
ob_find_next_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	short l = ob_find_any_flag(tree, OF_LASTOB, 0, OF_LASTOB);

	DIAGS(("ob_find_next_any_flag: start=%d, flags=%x, lastob=%d",
		start, f, l));
	/*
	 * Check if start is inside.
	 */
	if (l < 0 || l < start)
		return -1;
	
	/*
	 * Start at last object in tree? Wrap if so
	 */
	if (start == l)
		o = 0;
	else
		o++;

	DIAGS(("ob_find_next_any_flag: start=%d, start search=%d",
		start, o));

	while ( o != start )
	{
		DIAGS((" ---- looking at %d, flags=%x",
			o, tree[o].ob_flags));

		if (tree[o].ob_flags & f)
			return o;
		if (tree[o].ob_flags & OF_LASTOB)
			o = 0;
		else
			o++;
	}
	return -1;
}
short
ob_find_prev_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	short l = ob_find_any_flag(tree, OF_LASTOB, 0, OF_LASTOB);

	/*
	 * If start == -1, start at last object.
	 */
	 if (start == -1)
	 	start = l;
	/*
	 * Check if start is inside.
	 */
	if (l < 0 || l < start)
		return -1;
	
	/*
	 * Start at first object in tree? Wrap if so
	 */
	if (!start)
		o = l;
	else
		o--;

	while ( o != start )
	{
		if (tree[o].ob_flags & f)
			return o;
		if (!o)
			o = l;
		else
			o--;
	}
	return -1;
}

short
ob_find_cancel(OBJECT *ob)
{
	int f = 0;

	do {
		if ((ob[f].ob_type  & 0xff) == G_BUTTON &&
		    (ob[f].ob_flags & (OF_SELECTABLE|OF_TOUCHEXIT|OF_EXIT)) != 0)
		{
			char t[32];
			char *s = t;
			char *e;
			int l;

			e = object_get_spec(ob+f)->free_string;
			l = strlen(e);
			if (l < 32)
			{
				strcpy(t, e);

				/* strip surrounding spaces */
				e = t + l;
				while (*s == ' ')
					s++;
				while (*--e == ' ') 
					;
				*++e = '\0';

				if (e - s < CB_L) /* No use comparing longer strings */
				{
					int i;

					for (i = 0; cfg.cancel_buttons[i][0]; i++)
					{
						if (stricmp(s,cfg.cancel_buttons[i]) == 0)
							return f;
					}
				}
			}
		}
	} while (!(ob[f++].ob_flags & OF_LASTOB));

	return -1;
}

short
ob_find_shortcut(OBJECT *tree, ushort nk)
{
	int i = 0;

	nk &= 0xff;

	DIAG((D_keybd, NULL, "find_shortcut: %d(0x%x), '%c'", nk, nk, nk));

	do {
		OBJECT *ob = tree + i;
		if ((ob->ob_state & OS_WHITEBAK) && ((ob->ob_state & OS_DISABLED) == 0))
		{
			int ty = ob->ob_type & 0xff;
			if (ty == G_BUTTON || ty == G_STRING)
			{
				int j = (ob->ob_state >> 8) & 0x7f;
				if (j < 126)
				{
					char *s = object_get_spec(ob)->free_string;
					if (s)
					{
						DIAG((D_keybd,NULL,"  -  in '%s' find '%c' on %d :: %c",s,nk,j, *(s+j)));
						if (j < strlen(s) && toupper(*(s+j)) == nk)
							return i;
					}
				}
			}
		}
	} while ( (tree[i++].ob_flags & OF_LASTOB) == 0);

	return -1;
}

/*
 * Get the true screen coords of an object
 */
short
obj_offset(XA_TREE *wt, short object, short *mx, short *my)
{
	OBJECT *obtree = wt->tree;
	short next;
	short current = 0;
	short x = -wt->dx, y = -wt->dy;
	
	DIAG((D_objc, NULL, "obj_offset: obtree=%lx, obj=%d, xret=%lx, yret=%lx",
		obtree, object, mx, my));

	do
	{
		/* Found the object in the tree? cool, return the coords */
		if (current == object)
		{
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			
			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "obj_offset: return found obj=%d at x=%d, y=%d",
				object, x, y));
			return 1;
		}

		/* Any children? */
		if (obtree[current].ob_head != -1)
		{
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			current = obtree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = obtree[current].ob_next;

			while ((next != -1) && (obtree[next].ob_tail == current))
			{
				/* Trace back up tree if no more siblings */
				current = next;
				x -= obtree[current].ob_x;
				y -= obtree[current].ob_y;
				next = obtree[current].ob_next;
			}
			current = next;
		}
	}
	while (current != -1); /* If 'current' is -1 then we have finished */

	/* Bummer - didn't find the object, so return error */
	DIAG((D_objc, NULL, "obj_offset: didnt find object"));
	return 0;
}
short
ob_offset(OBJECT *obtree, short object, short *mx, short *my)
{
	short next;
	short current = 0;
	short x = 0, y = 0;
	
	DIAG((D_objc, NULL, "ob_offset: obtree=%lx, obj=%d, xret=%lx, yret=%lx",
		obtree, object, mx, my));

	do
	{
		/* Found the object in the tree? cool, return the coords */
		if (current == object)
		{
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			
			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "ob_offset: return found obj=%d at x=%d, y=%d",
				object, x, y));
			return 1;
		}

		/* Any children? */
		if (obtree[current].ob_head != -1)
		{
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			current = obtree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = obtree[current].ob_next;

			while ((next != -1) && (obtree[next].ob_tail == current))
			{
				/* Trace back up tree if no more siblings */
				current = next;
				x -= obtree[current].ob_x;
				y -= obtree[current].ob_y;
				next = obtree[current].ob_next;
			}
			current = next;
		}
	}
	while (current != -1); /* If 'current' is -1 then we have finished */

	/* Bummer - didn't find the object, so return error */
	DIAG((D_objc, NULL, "ob_offset: didnt find object"));
	return 0;
}

void
obj_rectangle(XA_TREE *wt, short obj, RECT *c)
{
	OBJECT *b;

	if (!obj_offset(wt, obj, &c->x, &c->y))
	{
		obj = 0;
		obj_offset(wt, obj, &c->x, &c->y);
	}
	b = wt->tree + obj;
	c->w = b->ob_width;
	c->h = b->ob_height;
}
void
ob_rectangle(OBJECT *obtree, short obj, RECT *c)
{
	OBJECT *b;

	if (!ob_offset(obtree, obj, &c->x, &c->y))
	{
		obj = 0;
		ob_offset(obtree, obj, &c->x, &c->y);
	}
	b = obtree + obj;
	c->w = b->ob_width;
	c->h = b->ob_height;
}
void
obj_area(XA_TREE *wt, short obj, RECT *c)
{
	OBJECT *b;
	RECT r;
	
	if (!obj_offset(wt, obj, &c->x, &c->y))
	{
		obj = 0;
		obj_offset(wt, obj, &c->x, &c->y);
	}
	b = wt->tree + obj;
	c->w = b->ob_width;
	c->h = b->ob_height;
	object_offsets(b, &r);
	c->x += r.x;
	c->y += r.y;
	c->w -= r.w;
	c->h -= r.h;
}

void
ob_area(OBJECT *obtree, short obj, RECT *c)
{
	OBJECT *b;
	RECT r;
	
	if (!ob_offset(obtree, obj, &c->x, &c->y))
	{
		obj = 0;
		ob_offset(obtree, obj, &c->x, &c->y);
	}
	b = obtree + obj;
	c->w = b->ob_width;
	c->h = b->ob_height;
	object_offsets(b, &r);
	c->x += r.x;
	c->y += r.y;
	c->w -= r.w;
	c->h -= r.h;
}

/*
 * Calculate the differance between border corrections for two
 * objects. For example a slider where the slide parent is a non-3d
 * object while the slider is, we need to take 3d border offsets 
 * into accrount when positioning/sizing the slider.
 */
void
ob_border_diff(OBJECT *obtree, short obj1, short obj2, RECT *r)
{
	RECT r1, r2;

	object_offsets(obtree + obj1, &r1);
	object_offsets(obtree + obj2, &r2);

	r->x = r1.x - r2.x;
	r->y = r1.y - r2.y;
	r->w = r1.w - r2.w;
	r->h = r1.h - r2.h;
}

void
ob_spec_xywh(OBJECT *obtree, short obj, RECT *r)
{
	short type = obtree[obj].ob_type & 0xff;

	switch (type)
	{
		case G_IMAGE:
		{
			BITBLK *bb = object_get_spec(obtree + obj)->bitblk;
			
			r->x = bb->bi_x;
			r->y = bb->bi_y;
			r->w = bb->bi_wb;
			r->h = bb->bi_hl;
			break;
		}
		case G_ICON:
		{
			*r = *(RECT *)&(object_get_spec(obtree + obj))->iconblk->ib_xicon;
			break;
		}
		case G_CICON:
		{
			*r = *(RECT *)&(object_get_spec(obtree + obj))->ciconblk->monoblk.ib_xicon;
			break;
		}
		default:
		{
			*r = *(RECT *)&(obtree[obj].ob_x);
			break;
		}
	}
}

void
object_spec_wh(OBJECT *ob, short *w, short *h)
{
	short type = ob->ob_type & 0xff;
	switch (type)
	{
		case G_IMAGE:
		{
			BITBLK *bb = object_get_spec(ob)->bitblk;
			
			*w = bb->bi_wb;
			*h = bb->bi_hl;
			break;
		}
		case G_ICON:
		{
			ICONBLK *ib = object_get_spec(ob)->iconblk;

			*w = ib->ib_wicon;
			*h = ib->ib_hicon;
			break;
		}
		case G_CICON:
		{
			CICONBLK *cb = object_get_spec(ob)->ciconblk;
			*w = cb->monoblk.ib_wicon;
			*h = cb->monoblk.ib_hicon;
			break;
		}
		default:
		{
			*w = ob->ob_width;
			*h = ob->ob_height;
			break;
		}
	}
}

/*
 * Find which object is at a given location
 *
 */
short
obj_find(XA_TREE *wt, short object, short depth, short mx, short my, RECT *c)
{
	OBJECT *obtree = wt->tree;
	short next;
	short current = 0, rel_depth = 1;
	short x = -wt->dx, y = -wt->dy;
	bool start_checking = false;
	short pos_object = -1;
	RECT r;

	DIAG((D_objc, NULL, "obj_find: obj=%d, depth=%d, obtree=%lx, obtree at %d/%d/%d/%d, find at %d/%d",
		object, depth, obtree, obtree->ob_x, obtree->ob_y, obtree->ob_width, obtree->ob_height,
		mx, my));
	do {
		if (current == object)	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}
		
		if (start_checking)
		{
			if (  (obtree[current].ob_flags & OF_HIDETREE) == 0
			    && obtree[current].ob_x + x                     <= mx
			    && obtree[current].ob_y + y                     <= my
			    && obtree[current].ob_x + x + obtree[current].ob_width  >= mx
			    && obtree[current].ob_y + y + obtree[current].ob_height >= my)
			{
				/* This is only a possible object, as it may have children on top of it. */
				if (c)
				{
					r.x = obtree[current].ob_x + x;
					r.y = obtree[current].ob_y + y;
					r.w = obtree[current].ob_width;
					r.h = obtree[current].ob_height;
				}
				pos_object = current;
			}
		}

		if ( ((!start_checking) || (rel_depth < depth))
		    && (obtree[current].ob_head != -1)
		    && (obtree[current].ob_flags & OF_HIDETREE) == 0)
		{
			/* Any children? */
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			rel_depth++;
			current = obtree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = obtree[current].ob_next;

			/* Trace back up tree if no more siblings */
			while ((next != -1) && (obtree[next].ob_tail == current))
			{
				current = next;
				x -= obtree[current].ob_x;
				y -= obtree[current].ob_y;
				next = obtree[current].ob_next;
				rel_depth--;
			}
			current = next;
		}	
	}
	while ((current != -1) && (rel_depth > 0));

	if (c && pos_object >= 0)
	{
		if (!xa_rect_clip(c, &r, &r))
			pos_object = -1;
	}

	DIAG((D_objc, NULL, "obj_find: found %d", pos_object));

	return pos_object;
}

short
ob_find(OBJECT *obtree, short object, short depth, short mx, short my)
{
	short next;
	short current = 0, rel_depth = 1;
	short x = 0, y = 0;
	bool start_checking = false;
	short pos_object = -1;

	DIAG((D_objc, NULL, "ob_find: obj=%d, depth=%d, obtree=%lx, obtree at %d/%d/%d/%d, find at %d/%d",
		object, depth, obtree, obtree->ob_x, obtree->ob_y, obtree->ob_width, obtree->ob_height,
		mx, my));
	do {
		if (current == object)	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}
		
		if (start_checking)
		{
			if (  (obtree[current].ob_flags & OF_HIDETREE) == 0
			    && obtree[current].ob_x + x                     <= mx
			    && obtree[current].ob_y + y                     <= my
			    && obtree[current].ob_x + x + obtree[current].ob_width  >= mx
			    && obtree[current].ob_y + y + obtree[current].ob_height >= my)
			{
				/* This is only a possible object, as it may have children on top of it. */
				pos_object = current;
			}
		}

		if ( ((!start_checking) || (rel_depth < depth))
		    && (obtree[current].ob_head != -1)
		    && (obtree[current].ob_flags & OF_HIDETREE) == 0)
		{
			/* Any children? */
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			rel_depth++;
			current = obtree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = obtree[current].ob_next;

			/* Trace back up tree if no more siblings */
			while ((next != -1) && (obtree[next].ob_tail == current))
			{
				current = next;
				x -= obtree[current].ob_x;
				y -= obtree[current].ob_y;
				next = obtree[current].ob_next;
				rel_depth--;
			}
			current = next;
		}	
	}
	while ((current != -1) && (rel_depth > 0));

	DIAG((D_objc, NULL, "ob_find: found %d", pos_object));

	return pos_object;
}

bool
obtree_is_menu(OBJECT *tree)
{
	bool m = false;
	short title;

	title = tree[0].ob_head;
	if (title > 0)
		title = tree[title].ob_head;
	if (title > 0)
		title = tree[title].ob_head;
	if (title > 0)
		m = (tree[title].ob_type & 0xff) == G_TITLE;

	return m;
}

bool
obtree_has_default(OBJECT *obtree)
{
	short o = ob_find_any_flst(obtree, OF_DEFAULT, 0, 0, 0, OF_LASTOB, 0);
	return o >= 0 ? true : false;
}
bool
obtree_has_exit(OBJECT *obtree)
{
	short o = ob_find_any_flst(obtree, OF_EXIT, 0, 0, 0, OF_LASTOB, 0);
	return o >= 0 ? true : false;
} 
bool
obtree_has_touchexit(OBJECT *obtree)
{
	short o = ob_find_any_flst(obtree, OF_TOUCHEXIT, 0, 0, 0, OF_LASTOB, 0);
	return o >= 0 ? true : false;
} 

/* HR 120601: Objc_Change:
 * We go back thru the parents of the object until we meet a opaque object.
 *    This is to ensure that transparent objects are drawn correctly.
 * Care must be taken that outside borders or shadows are included in the draw.
 * Only the objects area is redrawn, so it must be intersected with clipping rectangle.
 * New function object_area(RECT *c, tree, item)
 *
 *	Now we can use this for the standard menu's and titles!!!
 *
 */
void
obj_change(XA_TREE *wt,
	   short obj,
	   short state,
	   short flags,
	   bool redraw,
	   const RECT *clip,
	   struct xa_rect_list *rl)
{
	OBJECT *obtree = wt->tree;
	bool draw = false;

	if (obtree[obj].ob_flags != flags)
	{
		obtree[obj].ob_flags = flags;
		draw |= true;
	}
	if (obtree[obj].ob_state != state)
	{
		obtree[obj].ob_state = state;
		draw |= true;
	}

	if (draw && redraw)
	{
		obj_draw(wt, obj, clip, rl);
	}
}

void
obj_draw(XA_TREE *wt, short obj, const RECT *clip, struct xa_rect_list *rl)
{
	short start = obj, i;
	RECT or;

	hidem();

	obj_area(wt, obj, &or);

	while (object_is_transparent(wt->tree + start))
	{
		if ((i = ob_get_parent(wt->tree, start)) < 0)
			break;
		start = i;
	}

	if (rl)
	{
		RECT r;
		do
		{
			r = rl->r;
			if (!clip || (clip && xa_rect_clip(clip, &r, &r)))
			{
				if (xa_rect_clip(&or, &r, &r))
				{
					set_clip(&r);
					draw_object_tree(0, wt, wt->tree, start, MAX_DEPTH, NULL);
				}
			}
		} while ((rl = rl->next));
	}
	else
	{
		set_clip(&or);
		draw_object_tree(0, wt, wt->tree, start, MAX_DEPTH, NULL);
	}
	clear_clip();

	showm();
}
/*
 **************************************************************************
 * object edit functions
 **************************************************************************
 */
static bool ed_scrap_copy(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_cut(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_paste(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt, int *cursor_pos);
static bool obj_ed_char(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt, ushort keycode);

static char *
ed_scrap_filename(char *scrp, size_t len)
{
	size_t path_len;

	strncpy(scrp, cfg.scrap_path, len);
	path_len = strlen(scrp);

	if (scrp[path_len-1] != '\\')
		strcat(scrp+path_len, "\\");

	strcat(scrp+path_len, "scrap.txt");
	return scrp;
}

static bool
ed_scrap_cut(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt)
{
	ed_scrap_copy(wt, ei, ed_txt);

	/* clear the edit control */
	*ed_txt->te_ptext = '\0';
	ei->pos = 0;
	return true;
}

static bool
ed_scrap_copy(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt)
{
	int len = strlen(ed_txt->te_ptext);
	if (len)
	{
		char scrp[256];
		struct file *f;

		f = kernel_open(ed_scrap_filename(scrp, sizeof(scrp)),
				O_WRONLY|O_CREAT|O_TRUNC, NULL, NULL);
		if (f)
		{
			kernel_write(f, ed_txt->te_ptext, len);
			kernel_close(f);
		}
	}
	return false;
}

static bool
ed_scrap_paste(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt, int *cursor_pos)
{
	char scrp[256];
	struct file *f;

	f = kernel_open(ed_scrap_filename(scrp, sizeof(scrp)), O_RDONLY, NULL, NULL);
	if (f)
	{
		unsigned char data[128];
		long cnt = 1;

		while (cnt)
		{
			int i;

			cnt = kernel_read(f, data, sizeof(data));
			if (!cnt)
				break;

			for (i = 0; i < cnt; i++)
			{
				/* exclude some chars */
				switch (data[i])
				{
				case '\r':
				case '\n':
				case '\t':
					break;
				default:
					obj_ed_char(wt, ei, ed_txt, (unsigned short)data[i]);

					if (ei->pos >= ed_txt->te_txtlen - 1)
					{
						cnt = 0;
						break;
					}					
				}
			}
		}

		kernel_close(f);

		/* move the cursor to the end of the pasted text */
		*cursor_pos = ei->pos;
		return true;
	}

	return false;
}

/* Johan's versions of these didn't work on my system, so I've redefined them 
   - This is faster anyway */

static const unsigned char character_type[] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	CGs, 0, 0, 0, 0, 0, 0, 0,
	0, 0, CGw, 0, 0, 0, CGdt, 0,
	CGd, CGd, CGd, CGd, CGd, CGd, CGd, CGd,
	CGd, CGd, CGp, 0, 0, 0, 0, CGw,
	0, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa,
	CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa,
	CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa, CGu|CGa,
	CGu|CGa, CGu|CGa, CGu|CGa, 0, CGp, 0, 0, CGxp,
	0, CGa, CGa, CGa, CGa, CGa, CGa, CGa,
	CGa, CGa, CGa, CGa, CGa, CGa, CGa, CGa,
	CGa, CGa, CGa, CGa, CGa, CGa, CGa, CGa,
	CGa, CGa, CGa, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static bool
obj_ed_char(XA_TREE *wt,
	    struct objc_edit_info *ei,
	    TEDINFO *ed_txt,
	    ushort keycode)
{
	char *txt = ed_txt->te_ptext;
	int cursor_pos = ei->pos, x, key, tmask, n, chg;
	bool update = false;

	DIAG((D_objc, NULL, "ed_char keycode=0x%04x", keycode));

	switch (keycode)
	{
	case 0x0000:
	{
		update = false;
		break;
	}	
	case 0x011b:	/* ESCAPE clears the field */
	{
		txt[0] = '\0';
		cursor_pos = 0;
		update = true;
		break;
	}
	case 0x537f:	/* DEL deletes character under cursor */
	{
		if (txt[cursor_pos])
		{
			for(x = cursor_pos; x < ed_txt->te_txtlen - 1; x++)
				txt[x] = txt[x + 1];
			
			update = true;
		}
		break;
	}
	case 0x0e08:	/* BACKSPACE deletes character left of cursor (if any) */
	{
		if (cursor_pos)
		{
			for(x = cursor_pos; x < ed_txt->te_txtlen; x++)
				txt[x - 1] = txt[x];

			cursor_pos--;
			update = true;
		}
		break;
	}
	case 0x4d00:	/* RIGHT ARROW moves cursor right */
	{
		if ((txt[cursor_pos]) && (cursor_pos < ed_txt->te_txtlen - 1))
		{
			cursor_pos++;
			update = false;
		}
		break;
	}
	case 0x4d36:	/* SHIFT+RIGHT ARROW move cursor to far right of current text */
	{
		for(x = 0; txt[x]; x++)
			;

		if (x != cursor_pos)
		{
			cursor_pos = x;
			update = false;
		}
		break;
	}
	case 0x4b00:	/* LEFT ARROW moves cursor left */
	{
		if (cursor_pos)
		{
			cursor_pos--;
			update = false;
		}
		break;
	}
	case 0x4b34:	/* SHIFT+LEFT ARROW move cursor to start of field */
	case 0x4700:	/* CLR/HOME also moves to far left */
	{
		if (cursor_pos)
		{
			cursor_pos = 0;
			update = false;
		}
		break;
	}
	case 0x2e03:	/* CTRL+C */
	{
		update = ed_scrap_copy(wt, ei, ed_txt);
		break;
	}
	case 0x2d18: 	/* CTRL+X */
	{
		update = ed_scrap_cut(wt, ei, ed_txt);
		break;
	}
	case 0x2f16: 	/* CTRL+V */
	{
		update = ed_scrap_paste(wt, ei, ed_txt, &cursor_pos);
		break;
	}
	default:	/* Just a plain key - insert character */
	{
		chg = 0;/* Ugly hack! */
		if (cursor_pos == ed_txt->te_txtlen - 1)
		{
			cursor_pos--;
			chg = 1;
		}
				
		key = keycode & 0xff;
		tmask = character_type[key];

		n = strlen(ed_txt->te_pvalid) - 1;
		if (cursor_pos < n)
			n = cursor_pos;

		switch(ed_txt->te_pvalid[n])
		{
		case '0':
			tmask &= CGd|CGdt;
			break;
		case '9':
			tmask &= CGd;
			break;
		case 'a':
			tmask &= CGa|CGs;
			break;
		case 'n':
			tmask &= CGa|CGd|CGs;
			break;
		case 'p':
			tmask &= CGa|CGd|CGp|CGxp;
			/*key = toupper((char)key);*/
			break;
		case 'A':
			tmask &= CGa|CGs;
			key = toupper((char)key);
			break;
		case 'N':
			tmask &= CGa|CGd|CGs;
			key = toupper((char)key);
			break;
		case 'F':
			tmask &= CGa|CGd|CGp|CGxp|CGw;
			/*key = toupper((char)key);*/
			break;
		case 'f':
			tmask &= CGa|CGd|CGp|CGxp|CGw;
			/*key = toupper((char)key);*/
			break;
		case 'P':
			tmask &= CGa|CGd|CGp|CGxp|CGw;
			/*key = toupper((char)key);*/
			break;
		case 'X':
			tmask = 1;
			break;
		case 'x':
			tmask = 1;
			key = toupper((char)key);
			break;
		default:
			tmask = 0;
			break;			
		}
		
		if (!tmask)
		{
			for(n = x = 0; ed_txt->te_ptmplt[n]; n++)
			{
				if (ed_txt->te_ptmplt[n] == '_')
					x++;
				else if (ed_txt->te_ptmplt[n] == key && x >= cursor_pos)
					break;
			}
			if (key && (ed_txt->te_ptmplt[n] == key))
			{
				for(n = cursor_pos; n < x; n++)
					txt[n] = ' ';
				txt[x] = '\0';
				cursor_pos = x;
			}
			else
			{
				cursor_pos += chg;		/* Ugly hack! */
				ei->pos = cursor_pos;
				return XAC_DONE;
			}
		}
		else
		{
			txt[ed_txt->te_txtlen - 2] = '\0';	/* Needed! */
			for(x = ed_txt->te_txtlen - 1; x > cursor_pos; x--)
				txt[x] = txt[x - 1];

			txt[cursor_pos] = (char)key;

			cursor_pos++;
		}

		update = true;
		break;
	}
	}
	ei->pos = cursor_pos;
	return update;
}

#if GENERATE_DIAGS
char *edfunc[] =
{
	"ED_START",
	"ED_INIT",
	"ED_CHAR",
	"ED_END",
	"*ERR*"
};
#endif

inline static bool
chk_edobj(OBJECT *obtree, short obj, short lastobj)
{
	if (obj < 0 ||
	    obj > lastobj ||
	    !object_is_editable( obtree + obj ) )
	{
		return false;
	}
	else
	{
		return true;
	}
}

static short
obj_ED_INIT(struct widget_tree *wt,
	    struct objc_edit_info *ei,
	    short obj, short pos, short last,
	    TEDINFO **ted_ret,
	    short *old_ed)
{
	TEDINFO *ted = NULL;
	OBJECT *obtree = wt->tree;
	short p, ret = 0, old_ed_obj = -1;

	/* Ozk:
	 * See if the object passed to us really is editable.
	 * We give up here if object not editable, because I think
	 * thats what other aes's does. But before doing that we search
	 * for another editable object which we pass back.
	 * XXX - see how continuing setup with the lookedup object affects
	 *       applications.
	 */
	if ( chk_edobj(obtree, obj, last) && (ted = object_get_tedinfo(obtree + obj)) )
	{
		/* Ozk: 
		 * do things here to end edit of current obj...
		 */
		/* --- */
		old_ed_obj = ei->obj;
		/* Ozk:
		 * Init the object.
		 * If new posititon == -1, we place the cursor at the very end
		 * of the text to edit.
		 */
		ei->obj = obj;
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;

		p = strlen(ted->te_ptext);
		if (pos && (pos == -1 || pos > p))
			pos = p;
		ei->pos = pos;
		DIAGS(("ED_INIT: type %d, te_ptext='%s', %lx", obtree[obj].ob_type, ted->te_ptext, (long)ted->te_ptext));
		ret = 1;
	}
	else
	{
		old_ed_obj = ei->obj;
		ei->obj = -1;
		ret = 0;
	}

	DIAGS(("ED_INIT: return ted=%lx, old_ed=%d",
		ted, old_ed_obj));

	if (ted_ret)
		*ted_ret = ted;
	if (old_ed)
		*old_ed = old_ed_obj;

	return ret;
}
/*
 * Returns 1 if successful (character eaten), or 0 if not.
 */
short
obj_edit(XA_TREE *wt,
	 short func,
	 short obj,
	 short keycode,
	 short pos,	/* -1 sets position to end of text */
	 bool redraw,
	 const RECT *clip,
	 struct xa_rect_list *rl,
	/* outputs */
	 short *ret_pos,
	 short *ret_obj)
{
	short ret = 1;
	TEDINFO *ted;
	OBJECT *obtree = wt->tree;
	struct objc_edit_info *ei;
	short last = 0, old_ed_obj = -1;

#if GENERATE_DIAGS
	char *funcstr = func < 0 || func > 3 ? edfunc[4] : edfunc[func];
	DIAG((D_form,wt->owner,"  --  obj_edit: func %s, wt=%lx obtree=%lx, obj:%d, k:%x, pos:%x",
	      funcstr, wt, obtree, obj, keycode, pos));
#endif
	last = ob_find_any_flag(obtree, OF_LASTOB, 0, OF_LASTOB);

	if (wt->e.obj != -1 && wt->e.obj > last)
		wt->e.obj = -1;

	if (!wt->owner->options.xa_objced)
	{
		switch (func)
		{
			case ED_INIT:
			{
				hidem();
				obj_ED_INIT(wt, &wt->e, obj, -1, last, NULL, &old_ed_obj);
				if (redraw)
					eor_objcursor(wt, rl);
				showm();
				pos = wt->e.pos;
				break;
			}
			case ED_END:
			{
				/* Ozk: Just turn off cursor :)
				 */
				if (wt->e.obj > 0)
				{
					pos = wt->e.pos;
				}
				if (redraw)
				{
					hidem();
					eor_objcursor(wt, rl);
					showm();
				}
				break;
			}
			case ED_CHAR:
			{
				hidem();
				if ( wt->e.obj == -1 ||
				     obj == -1 ||
				     obj != wt->e.obj)
				{
					/* Ozk:
					 * I am not sure if this is correct, but if ED_INIT have not been
					 * called before ED_CHAR, we get passed an object value of -1.
					 * If so, we search for for the first editable object in the
					 * obtree and perform edit on that one. This fixes BoinkOut2's
					 * 'set timing value' dialog. Wondering if we should do automatic
					 * ED_INIT in this case.... ?
					 * 
					 */

					if (obj == -1)
					{
						obj = wt->e.obj;
						if (obj == -1)
							obj = ob_find_next_any_flag(obtree, 0, OF_EDITABLE);
					}
					if (obj_ED_INIT(wt, &wt->e, obj, pos, last, &ted, &old_ed_obj))
					{
						if (redraw)
						{
							eor_objcursor(wt, rl);
							if (obj_ed_char(wt, &wt->e, ted, keycode))
								obj_draw(wt, wt->e.obj, clip, rl);
							eor_objcursor(wt, rl);
						}
						else
							obj_ed_char(wt, &wt->e, ted, keycode);
							
						pos = wt->e.pos;
					}
				}
				else
				{
					/* Ozk:
					 * Object is the one with cursor focus, so we do it normally
					 */
					ted = object_get_tedinfo(obtree + obj);
					ei = &wt->e;

					DIAGS((" -- obj_edit: ted=%lx", ted));

					if (redraw)
					{
						eor_objcursor(wt, rl);
						if (obj_ed_char(wt, ei, ted, keycode))
							obj_draw(wt, obj, clip, rl);
						eor_objcursor(wt, rl);
					}
					else
						obj_ed_char(wt, ei, ted, keycode);
					
					pos = ei->pos;
				}
				showm();
				break;
			}
			case ED_CRSR:
			{
#if 0
				/* TODO: x coordinate -> cursor position conversion */

				/* TODO: REMOVE: begin ... ED_INIT like position return */
				if (*(ted->te_ptext) == '@')
					wt->e.pos = 0;
				else
					wt->e.pos = strlen(ted->te_ptext);
				/* TODO: REMOVE: end */
#endif
				return 1;
				break;
			}
			default:
			{
				return 0;
			}
		}
	}
	else
	{
		switch (func)
		{
			case ED_INIT:
			{
				ei = &wt->e;
				hidem();
				if (ei->obj >= 0 && redraw)
					undraw_objcursor(wt, rl);

				if (!obj_ED_INIT(wt, ei, obj, -1, last, NULL, &old_ed_obj))
					disable_objcursor(wt, rl);
				else
				{
					enable_objcursor(wt);
					if (redraw)
						draw_objcursor(wt, rl);
				}
				showm();
				pos = ei->pos;
				break;
			}
			case ED_DISABLE:
			case ED_END:
			{
				/* Ozk: Just turn off cursor :)
				 */
				if (wt->e.obj > 0)
					pos = wt->e.pos;

				hidem();
				disable_objcursor(wt, rl);
				showm();
				break;
			}
			case ED_ENABLE:
			{
				enable_objcursor(wt);
				break;
			}
			case ED_CRSROFF:
			{
				undraw_objcursor(wt, rl);
				break;
			}
			case ED_CRSRON:
			{
				draw_objcursor(wt, rl);
				break;
			}
			case ED_CHAR:
			{
				/* Ozk:
				 * Hmm... we allow to edit objects other than the one in wt->e.obj.
				 * However, how should we act here when that other object is not an
				 * editable? Use the wt->e.obj or return with error?
				 * For now we return with error.
				 */

				hidem();
				if ( wt->e.obj == -1 ||
				     obj == -1 ||
				     obj != wt->e.obj)
				{
					struct objc_edit_info lei;

					/* Ozk:
					 * If the object is not initiated, or if its different
					 * from the object holding cursor focus, we do it like this;
					 */
					DIAGS((" -- obj_edit: on object=%d without cursor focus(=%d)",
						obj, wt->e.obj));
					lei.obj = -1;
					lei.pos = 0;
					lei.c_state = 0;
				
					if (obj_ED_INIT(wt, &lei, obj, -1, last, &ted, &old_ed_obj))
					{
						if (obj_ed_char(wt, &lei, ted, keycode))
							obj_draw(wt, obj, clip, rl);

						pos = lei.pos;
					}
				}
				else
				{
					/* Ozk:
					 * Object is the one with cursor focus, so we do it normally
					 */
					ted = object_get_tedinfo(obtree + obj);
					ei = &wt->e;

					DIAGS((" -- obj_edit: ted=%lx", ted));

					undraw_objcursor(wt, rl);
					if (obj_ed_char(wt, ei, ted, keycode))
						obj_draw(wt, obj, clip, rl);
					set_objcursor(wt);
					draw_objcursor(wt, rl);
					pos = ei->pos;
				}
				showm();
				break;
			}
			case ED_CRSR:
			{
#if 0
				/* TODO: x coordinate -> cursor position conversion */

				/* TODO: REMOVE: begin ... ED_INIT like position return */
				if (*(ted->te_ptext) == '@')
					wt->e.pos = 0;
				else
					wt->e.pos = strlen(ted->te_ptext);
			/* TODO: REMOVE: end */
#endif
				return 1;
				break;
			}
			default:
			{
				return 0;
			}
		}
	}
	
		
	if (ret_pos)
		*ret_pos = pos;
	if (ret_obj)
		*ret_obj = obj;

	DIAG((D_objc, NULL, "obj_edit: old_obj=%d, return ret_obj=%d(%d), ret_pos=%d - ret=%d",
		old_ed_obj, obj, wt->e.obj, pos, ret));

	return ret;
}

void
obj_set_radio_button(XA_TREE *wt,
		      short obj,
		      bool redraw,
		      const RECT *clip,
		      struct xa_rect_list *rl)
{
	OBJECT *obtree = wt->tree;
	short parent, o;
	
	parent = ob_get_parent(obtree, obj);

	DIAG((D_objc, NULL, "obj_set_radio_button: wt=%lx, obtree=%lx, obj=%d, parent=%d",
		wt, obtree, obj, parent));

	if (parent != -1)
	{
		o = obtree[parent].ob_head;

		while (o != parent)
		{
			if ( obtree[o].ob_flags & OF_RBUTTON && obtree[o].ob_state & OS_SELECTED )
			{
				DIAGS(("radio: change obj %d", o));
				if (o != obj)
				{
					obj_change(wt,
						   o,
						   obtree[o].ob_state & ~OS_SELECTED,
						   obtree[o].ob_flags,
						   redraw,
						   clip,
						   rl);
				}	
			}
			o = obtree[o].ob_next;
		}
		DIAGS(("radio: set obj %d", obj));
		obj_change(wt,
			   obj,
			   obtree[obj].ob_state | OS_SELECTED,
			   obtree[obj].ob_flags,
			   redraw, clip,
			   rl);
	}
}

short
obj_watch(XA_TREE *wt,
	   short obj,
	   short in_state,
	   short out_state,
	   const RECT *clip,
	   struct xa_rect_list *rl)
{
	OBJECT *focus = wt->tree + obj;
	short pobf = -2, obf = obj;
	short mx, my, mb, x, y, omx, omy;
	short flags = focus->ob_flags;

	obj_offset(wt, obj, &x, &y);

	x--;
	y--;

	check_mouse(wt->owner, &mb, &omx, &omy);

	if (!mb)
	{
		/* If mouse button is already released, assume that was just
		 * a click, so select
		 */
		obj_change(wt, obj, in_state, flags, true, clip, rl);
	}
	else
	{
		S.wm_count++;
		obj_change(wt, obj, in_state, flags, true, clip, rl);
		while (mb)
		{
			short s;

			wait_mouse(wt->owner, &mb, &mx, &my);

			if ((mx != omx) || (my != omy))
			{
				omx = mx;
				omy = my;
				obf = obj_find(wt, obj, 10, mx, my, NULL);

				if (obf == obj)
					s = in_state;
				else
					s = out_state;

				if (pobf != obf)
				{
					pobf = obf;
					obj_change(wt, obj, s, flags, true, clip, rl);
				}
			}
		}
		S.wm_count--;
	}

	if (obf == obj)
		return 1;
	else
		return 0;
}
