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
	if (obtree <= (OBJECT *)0x1000L)
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
object_has_tedinfo(OBJECT *ob)
{
	switch (ob->ob_type & 0xff)
	{
		case G_TEXT:
		case G_BOXTEXT:
		case G_FTEXT:
		case G_FBOXTEXT:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}
inline bool
object_has_freestr(OBJECT *ob)
{
	switch (ob->ob_type & 0xff)
	{
		case G_STRING:
		case G_BUTTON:
		case G_TITLE:
		case G_SHORTCUT:
		{
			return true;
		}
		default:
		{
			return false;
		}
	}
}
inline bool
object_is_editable(OBJECT *ob)
{
	if ((ob->ob_flags & OF_EDITABLE) && !(ob->ob_state & OS_DISABLED))
	{
		if (object_has_tedinfo(ob) || (ob->ob_type & 0xff) == G_USERDEF)
			return true;
	}
	return false;
}

inline TEDINFO *
object_get_tedinfo(OBJECT *ob, XTEDINFO **x)
{
	TEDINFO *ted = NULL;
	
	if (x)
		*x = NULL;

	if (object_has_tedinfo(ob))
	{
		ted = object_get_spec(ob)->tedinfo;
	
		if (ted->te_ptext == (char *)-1L)
		{
			if (x)
			{
				*x = (XTEDINFO *)ted->te_ptmplt;
			}
			ted = &((XTEDINFO *)ted->te_ptmplt)->ti;
		}
	}
	return ted;
}
/*
 * extract string from object, return NULL if object is of a type
 * with no string
 */
char *
object_get_string(OBJECT *ob)
{
	char *ret = NULL;

	if (object_has_tedinfo(ob))
	{
		TEDINFO *ted = object_get_tedinfo(ob, NULL);
		ret = ted->te_ptext;
	}
	else if (object_has_freestr(ob))
	{
		ret = object_get_freestr(ob);
	}
	else
	{
		switch ((ob->ob_type & 0xff))
		{
			case G_CICON:
			{
				ret = object_get_spec(ob)->ciconblk->monoblk.ib_ptext;
				break;
			}
			case G_ICON:
			{
				ret = object_get_spec(ob)->iconblk->ib_ptext;
				break;
			}
			default:;
		}
	}
	return ret;
}
 
/* A quick hack to catch *most* of the problems with transparent objects */
bool
object_is_transparent(OBJECT *ob, bool pdistrans)
{
	bool ret = false;

	if (ob->ob_flags & OF_RBUTTON)
		ret = true;
	else
	{
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
		{
			ret = true;
			break;
		}
		case G_PROGDEF:
		{
			ret = pdistrans ? true : false;
			break;
		}
		case G_TEXT:
		case G_FTEXT:
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			union { short jc[2]; BFOBSPEC cw;} col;
			TEDINFO *ted;
			
			ted = object_get_tedinfo(ob, NULL);
			col.jc[0] = ted->te_just;
			col.jc[1] = ted->te_color;
			if (col.cw.textmode)
				ret = true;
			break;
		}
		default:;
	}
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
		if ( c == (CICON*)-1) break;

		if (c->num_planes <= screen.planes
		    && (!best_cicon || (c->num_planes > best_cicon->num_planes)))
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
	
	if (ted->te_ptext == (char *)-1L)
	{
		XTEDINFO *xt = (XTEDINFO *)ted->te_ptmplt;
		size += sizeof(*xt);
		ted = &xt->ti;
	}

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

static short
count_them(OBJECT *obtree, short parent, short start, short depth)
{
	short curr = start, objs = 0;
	
	DIAGS((" -- count_them: enter, parent %d", parent));
	
	do
	{
		OBJECT *ob = obtree + curr;

		objs++;
		
		DIAGS((" -- obj %d, type %x (n=%d, h=%d, t=%d)",
			curr, ob->ob_type, ob->ob_next, ob->ob_head, ob->ob_tail));
		
		if (ob->ob_head != -1 && depth)
			objs += count_them(obtree, curr, ob->ob_head, depth - 1);
			
		curr = obtree[curr].ob_next;
	
	} while (curr != parent && curr != -1);
	
	DIAGS((" -- count_them: return %d", objs));
	return objs;
}

short
ob_count_objs(OBJECT *obtree, short parent, short depth)
{
	short objs = 0, start = obtree[parent].ob_head;
	
	DIAG((D_rsrc, NULL, "ob_count_objs: tree %lx, parent %d", obtree, parent));

	DIAGS((" -- parent = %d", parent));

	if (depth && start != -1)
		objs = count_them(obtree, parent, start, depth - 1);
	
	DIAGS(("ob_count_objs: return %d", objs));

	return objs;
}

static long
obtree_len(OBJECT *obtree, short start, short *num_objs)
{
	long size;
	short parent, curr = start, objs = 0;
	struct xa_aes_object object;

	object = ob_get_parent(obtree, aesobj(obtree, start));
	parent = object.item;

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
				size += sizeof_tedinfo(object_get_spec(ob)->tedinfo);
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
copy_tedinfo(OBJECT *root_tree, TEDINFO *src, TEDINFO *dst)
{
	char *s = (char *)dst + sizeof(TEDINFO);

 	DIAGS(("copy_tedinfo: copy from %lx, to %lx", src, dst));
	
	*dst = *src;
	if (src->te_ptext == (char *)-1L)
	{
		XTEDINFO *xdst, *xsrc = (XTEDINFO *)src->te_ptmplt;
		
		xdst = (XTEDINFO *)s;
		s += sizeof(*xsrc);
		
		*xdst = *xsrc;
		xdst->o = aesobj(root_tree, aesobj_item(&xdst->o));
		dst->te_ptmplt = (char *)xdst;
		
		src = &xsrc->ti;
		dst = &xdst->ti;
	}

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
 	d = (short *)((char *)dst + sizeof(BITBLK));
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

	d = (char *)dst + sizeof(ICONBLK);
	words = ((src->ib_wicon + 15) >> 4) * src->ib_hicon;
	{
		short *fr, *dr = (short *)d;
		
		fr = src->ib_pdata;
		dst->ib_pdata = dr;
		for (i = 0; i < words; i++)
			*dr++ = *fr++;

		fr = src->ib_pmask;
		dst->ib_pmask = dr;
		for (i = 0; i < words; i++)
			*dr++ = *fr++;

		d = (char *)dr;
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
	short *fr, *d = (short *)((char *)dst + sizeof(CICON));
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

	d = (char *)dst + sizeof(CICONBLK);
	
	words = ((src->monoblk.ib_wicon + 15) >> 4) * src->monoblk.ib_hicon;
	{
		short *fr, *dr = (short *)d;
		
		fr = src->monoblk.ib_pdata;
		dst->monoblk.ib_pdata = dr;
		for (i = 0; i < words; i++)
			*dr++ = *fr++;

		fr = src->monoblk.ib_pmask;
		dst->monoblk.ib_pmask = dr;
		for (i = 0; i < words; i++)
			*dr++ = *fr++;

		d = (char *)dr;
	}
	dst->monoblk.ib_ptext = d;
	for (i = 0; i < 12; i++)
	{
		if (!(*d++ = src->monoblk.ib_ptext[i]))
			break;
	}

	d = (char *)(((long)d + 1) & 0xfffffffe);
	
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

		if (!*nxt_sci || *nxt_sci == (void *)-1L)
			*nxt_dci = NULL;
		else
		{
			while (*nxt_sci && *nxt_sci != (void *)-1L)
			{
				dst_ci = (CICON *)d;
				d = copy_cicon(*nxt_sci, dst_ci, words);
				*nxt_dci = dst_ci;
				nxt_sci = &((*nxt_sci)->next_res);
				nxt_dci = &dst_ci->next_res;
			
			#if 0
				dst_ci = (CICON *)d;
				d = copy_cicon(*nxt_sci, dst_ci, words);
				*nxt_dci = dst_ci;
			
				nxt_sci = &((*nxt_sci)->next_res);
				nxt_dci = &((*nxt_dci)->next_res);
			#endif
			}
		}
	}
	
	DIAGS((" --- return %lx", d));

	return (void *)d;	
}
	
static void
copy_obtree(OBJECT *obtree, short start, OBJECT *dst, void **data)
{
	short parent, curr = start;
	struct xa_aes_object object;

	object = ob_get_parent(obtree, aesobj(obtree, start));
	parent = object.item;

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
				*data = copy_tedinfo(dst, object_get_spec(ob)->tedinfo, *data);
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
				USERBLK *s, *d;
				
				d = *data;
				s = object_get_spec(ob)->userblk;
				*d = *s;
				on->ob_spec.userblk = d;
				d++;
				*data = d;
				break;
			}
			case G_BUTTON:
			case G_STRING:
			case G_TITLE:
			{
				char *s = object_get_spec(ob)->free_string;
				char *d = *data;
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

		data = (char *)new + ((long)objs * sizeof(OBJECT));

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
			case G_OBLINK:
			{
				struct oblink_spec *obl = object_get_oblink(ob);
				if (obl)
				{
					*ob = obl->save_obj;
					kfree(obl);
				}
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
/*
 * overwrite the string of entry 'obnum' in popup with new text.
 * Make sure the target textbuffer is large enough!
 */
void
obj_change_popup_entry(struct xa_aes_object obj, short obnum, char *s)
{
	if (valid_aesobj(&obj) && (aesobj_ob(&obj)->ob_type & 0xff) == G_POPUP)
	{
		POPINFO *pinf = object_get_popinfo(aesobj_ob(&obj));
		struct xa_aes_object this = aesobj(pinf->tree, obnum);
		char *obtxt;
		
		obtxt = object_get_string(aesobj_ob(&this));
		
		if (obtxt)
		{
			strcpy(obtxt + 2, s);
		}
	}
}

OBJECT *
create_popup_tree(struct xa_client *client, short type, short nobjs, short min_w, short min_h, void *(*cb)(short item, void **data), void **data)
{
	int i;
	OBJECT *new = NULL;
	long sl, ol, longest;
	struct xa_vdi_settings *v = client->vdi_settings;

	if (nobjs > 0)
	{
		sl = longest = 0;
		for (i = 0; i < nobjs; i++)
		{
			long l = strlen((*cb)(i, data)) + 4;
			
			if (l > longest)
				longest = l;
			sl += l;
		}

		ol = sizeof(*new) * (nobjs + 1);
		
		new = kmalloc(ol + sl + longest);

		if (new)
		{
			short x, y, w, h;
			char *entry, *this, *sepstr;
			
			this = (char *)new + ol;
			
			sepstr = this + sl;
			for (i = 0; i < longest; i++)
				sepstr[i] = '-';
			sepstr[longest] = '\0';

			x = y = 0;
			new->ob_next = -1;
			new->ob_head = 1;
			new->ob_tail = nobjs;
			
			new->ob_type = G_BOX;
			new->ob_flags = OF_NONE;
			new->ob_state = OS_NORMAL;
			
			new->ob_spec.index = 0x00FF1100L;
			new->ob_x = new->ob_y = 0;
			new->ob_width = min_w;
			new->ob_height = 0;

			(*v->api->t_font)(v, screen.standard_font_point, screen.standard_font_id);
			(*v->api->t_effects)(v, 0);

			for (i = 1; i <= nobjs; i++)
			{
				entry = (*cb)(i - 1, data);

				if (entry)
				{					
					new[i].ob_next = i + 1;
					new[i].ob_head = new[i].ob_tail = -1;
					new[i].ob_type = G_STRING;
					new[i].ob_flags = OF_NONE;
					
					if (*entry == '-')
					{
						new[i].ob_state = OS_DISABLED;
						new[i].ob_spec.free_string = sepstr;
						(*v->api->t_extent)(v, sepstr, &w, &h);
					}
					else
					{
						this[0] = ' ';
						this[1] = ' ';
						new[i].ob_state = OS_NORMAL;
						new[i].ob_spec.free_string = this;
						strcpy(this + 2, entry);
					
						(*v->api->t_extent)(v, this, &w, &h);
					
						this += strlen(entry) + 2;
						*this++ = ' ';
						*this++ = '\0';
					}
					if (new->ob_width < w)
						new->ob_width = w;
					new->ob_height += h;
						
					new[i].ob_y = y;
					new[i].ob_x = x;
					new[i].ob_width = 0;
					new[i].ob_height = h;
					
					y += h;
				}
			}
			new[nobjs].ob_next = 0;
			new[nobjs].ob_flags |= OF_LASTOB;
			w = new->ob_width;
			for (i = 1; i <= nobjs; i++)
				new[i].ob_width = w;
			
			if (new->ob_height < min_h)
				new->ob_height = min_h;
		}
	}
	return new;
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
			ted = object_get_tedinfo(ob, NULL);
			t = (signed char)ted->te_thickness;
		}
	}
	return t;
}

/*
 * Return offsets to add to object dimensions to account for borders, etc.
 */
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

static short
ob_get_previous(OBJECT *obtree, short parent, short obj)
{
	short head, next;

	head = obtree[parent].ob_head;

	if (head != obj)
	{
		while (1)
		{
			next = obtree[head].ob_next;
			if (next == obj)
				return head;
			head = next;
		}
	}
	return -1;
}

short
ob_remove(OBJECT *obtree, short obj)
{
	int parent, next, prev;
	struct xa_aes_object object;
	
	if (obj <= 0)
		return -1;

	next = obtree[obj].ob_next;
	object = ob_get_parent(obtree, aesobj(obtree, obj));
	parent = object.item;

	if (parent != -1)
	{
		if (obtree[parent].ob_head == obj)
		{
			if (obtree[parent].ob_tail == obj)
			{
				next = -1;
				obtree[parent].ob_tail = -1;
			}
			obtree[parent].ob_head = next;
		}
		else
		{
			prev = ob_get_previous(obtree, parent, obj);
			if (prev == -1)
				return -1;
			obtree[prev].ob_next = next;
			if (obtree[parent].ob_tail == obj)
				obtree[parent].ob_tail = prev;
		}
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
	int i;

	if (current == -1)		/* No siblings */
	{
		root[parent].ob_head = object;
		root[parent].ob_tail = object;
		root[object].ob_next = parent;
	}
	else if (!pos)			/* First among siblings */
	{
		root[object].ob_next = current;
		root[parent].ob_head = object;
	}
	else
	{
		if (pos == -1)
		{
			current = root[parent].ob_tail;
			
			root[object ].ob_next = parent;
			root[current].ob_next = object;
			root[parent ].ob_tail = object;
		}
		else
		{
			for (i = 1; i < pos; i++)
			{
				if ((current = root[current].ob_next) == -1)
					break;
			}
			if (current != -1)
			{
				root[object].ob_next = root[current].ob_next;
				root[current].ob_next = object;
			}
		}
	}
}

void
foreach_object(OBJECT *tree,
		struct xa_aes_object parent,
		struct xa_aes_object start,
		short stopf,
		short stops,
		bool(*f)(OBJECT *obtree, short obj, void *ret), void *data)
{
	struct xa_aes_object curr, next, stop;
	bool dothings = false;
	struct oblink_spec *oblink = NULL;

	curr = aesobj(tree, 0);
	stop = inv_aesobj();

	do
	{
		if (same_aesobj(&curr, &parent))
			stop = parent;

uplink:
		if (same_aesobj(&curr, &start))
		{
			dothings = true;
		}

		if (set_aesobj_uplink(&tree, &curr, &stop, &oblink))
			goto uplink;
		
		if (dothings)
		{
			if ((*f)(aesobj_tree(&curr), aesobj_item(&curr), data))
				break;
		}

		if (aesobj_ob(&curr)->ob_head != -1 && !(aesobj_ob(&curr)->ob_flags & OF_HIDETREE))
		{
			curr = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_head);
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
			
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);
				if (same_aesobj(&curr, &tail))
				{
					curr = next;
					next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
				}
				else break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&tree, &curr, &stop, &oblink))
				goto downlink;
			curr = next;
		}
	} while (valid_aesobj(&curr) && !same_aesobj(&curr, &stop));
	
	clean_aesobj_links(&oblink);
}

struct anyflst_parms
{
	short	flags, f, s, mf, ms, ret, ret1;
	struct	xa_aes_object ret_object;
};

static bool
count_flst(OBJECT *tree, short o, void *_data)
{
	struct anyflst_parms *d = _data;
	short flg = 0;
	
	if ((!d->mf || !(tree[o].ob_flags & d->mf)) && (!d->ms || !(tree[o].ob_state & d->ms)))
	{
		if (d->flags & OBFIND_EXACTFLAG)
		{
			if (!d->f || (tree[o].ob_flags & d->f) == d->f)
				flg |= 1;
		}
		else if (!d->f || (tree[o].ob_flags & d->f))
			flg |= 1;

		if (d->flags & OBFIND_EXACTSTATE)
		{
			if (!d->s || (tree[o].ob_state & d->s) == d->s)
				flg |= 2;
		}
		else if (!d->s || (tree[o].ob_state))
			flg |= 2;
	}

	if (flg == 3)
	{
		d->ret += 1;
		d->ret1 = o;
		d->ret_object = aesobj(tree, o);
	}
	return false;
}

static bool
anyflst(OBJECT *tree, short o, void *_data)
{
	struct anyflst_parms *d = _data;
	short flg = 0;
	
	if ((!d->mf || !(tree[o].ob_flags & d->mf)) && (!d->ms || !(tree[o].ob_state & d->ms)))
	{
		if (d->flags & OBFIND_EXACTFLAG)
		{
			if (!d->f || (tree[o].ob_flags & d->f) == d->f)
				flg |= 1;
		}
		else if (!d->f || (tree[o].ob_flags & d->f))
			flg |= 1;

		if (d->flags & OBFIND_EXACTSTATE)
		{
			if (!d->s || (tree[o].ob_state & d->s) == d->s)
				flg |= 2;
		}
		else if (!d->s || (tree[o].ob_state))
			flg |= 2;
	}

	if (flg == 3)
	{
		d->ret = o;
		d->ret_object = aesobj(tree, o);
		return true;
	}
	return false;
}

/*
 * Returns the object number of this object's parent or -1 if it is the root
 */
struct xa_aes_object
ob_get_parent(OBJECT *tree, struct xa_aes_object obj)
{
	struct par { void *prv; struct xa_aes_object this; } *parents;
	struct xa_aes_object curr, next, stop;
	struct oblink_spec *oblink = NULL;

	if (!aesobj_item(&obj))
		return inv_aesobj();

	curr = aesobj(tree, 0);
	stop = inv_aesobj();
	
	parents = kmalloc(sizeof(*parents));
	if (!parents)
		return inv_aesobj();

	parents->prv = NULL;
	parents->this = inv_aesobj();
	
	do
	{
uplink:
		if (same_aesobj(&curr, &obj))
		{
			break;
		}

		if (set_aesobj_uplink(&tree, &curr, &stop, &oblink))
			goto uplink;

		if (aesobj_ob(&curr)->ob_head != -1 && !(aesobj_ob(&curr)->ob_flags & OF_HIDETREE))
		{
			struct par *pn = kmalloc(sizeof(*pn));
			
			pn->prv = parents;
			pn->this = curr;
			parents = pn;
			curr = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_head);
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
			
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);
				if (same_aesobj(&curr, &tail))
				{
					struct par *pn;
					 
					if ((pn = parents->prv))
					{
						kfree(parents);
						parents = pn;
					}
					curr = next;
					next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
				}
				else break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&tree, &curr, &stop, &oblink))
				goto downlink;
			curr = next;
		}
	} while (valid_aesobj(&curr) && !same_aesobj(&curr, &stop));

	curr = parents->this;
	
	while (parents)
	{
		struct par *pn = parents->prv;
		kfree(parents);
		parents = pn;
	}
	
	DIAG((D_rsrc, NULL, "ob_get_parent: parent of %d in %lx is %d in %lx",
		aesobj_item(&obj), aesobj_tree(&obj), aesobj_item(&curr), aesobj_tree(&curr)));
	
	clean_aesobj_links(&oblink);

	return curr;
	
}
#if 0
struct xa_aes_object
ob_get_parent(OBJECT *obtree, struct xa_aes_object obj)
{
	DIAG((D_rsrc, NULL, "ob_get_parent: tree %lx, obj %d", obtree, obj));

	if (valid_aesobj(&obj) && aesobj_item(&obj) != 0)
	{
		struct xa_aes_object last, tail;

		do
		{
			last = obj;
			obj  = aesobj(aesobj_tree(&obj), aesobj_ob(&obj)->ob_next);
			tail = aesobj(aesobj_tree(&obj), aesobj_ob(&obj)->ob_tail);
		} while (valid_aesobj(&obj) && !same_aesobj(&last, &tail)); //obtree[obj].ob_tail != last);
	}
	else
		obj = inv_aesobj();

	DIAGS(("ob_get_parent: return %d", obj));

	return obj;
}
#endif
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
struct xa_aes_object
ob_find_flag(OBJECT *tree, short f, short mf, short stopf)
{
	struct anyflst_parms d;

	d.flags = OBFIND_EXACTFLAG;
	d.f = f;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, 0, anyflst, &d);
	return d.ret_object;
}
struct xa_aes_object
ob_find_any_flag(OBJECT *tree, short f, short mf, short stopf)
{
	struct anyflst_parms d;

	d.flags = 0;
	d.f = f;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = -1;
	d.ret_object = inv_aesobj();
	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, 0, anyflst, &d);
	return d.ret_object;
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
	struct anyflst_parms d;

	d.flags = OBFIND_EXACTFLAG;
	d.f = f;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = 0;
	d.ret1 = -1;

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, 0, count_flst, &d);

	if (count)
		*count = d.ret;
	return d.ret1;
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
	struct anyflst_parms d;
	
	d.flags = 0;
	d.f = f;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = 0;
	d.ret1 = -1;

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, 0, count_flst, &d);

	if (count)
		*count = d.ret;

	return d.ret1;	
}

struct xa_aes_object
ob_find_any_flst(OBJECT *tree, short f, short s, short mf, short ms, short stopf, short stops)
{
	struct anyflst_parms d;

	d.flags = 0;
	d.f = f;
	d.s = s;
	d.mf = mf;
	d.ms = ms;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, stops, anyflst, &d);
	return d.ret_object;
}

struct xa_aes_object
ob_find_flst(OBJECT *tree, short f, short s, short mf, short ms, short stopf, short stops)
{
	struct anyflst_parms d;
	
	d.flags = OBFIND_EXACTFLAG|OBFIND_EXACTSTATE;
	d.f = f;
	d.s = s;
	d.mf = mf;
	d.ms = ms;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), stopf, stops, anyflst, &d);
	return d.ret_object;
}

struct xa_aes_object
ob_find_next_any_flagstate(OBJECT *tree, struct xa_aes_object parent, struct xa_aes_object start, short f, short mf, short s, short ms, short stopf, short stops, short flags)
{
	int rel_depth = 1;
	short x, y, w, h, ax, cx, cy, cf, flg;
	bool dothings = false;
	OBJECT *this;
// 	bool d = false;
	struct oblink_spec *oblink = NULL;
	struct xa_aes_object curr, next, stop, co;
	RECT r;
	
	cx = cy = ax = 32000;

	co = stop = inv_aesobj();
	cf = 0;
	
// 	if (d) display(" ob_find_any_next_flagstate, parent %d, start %d", aesobj_item(&parent), aesobj_item(&start));
	
	DIAG((D_rsrc, NULL,"ob_find_any_next_flagstate, parent %d, start %d", aesobj_item(&parent), aesobj_item(&start)));
	
	if (aesobj_item(&start) <= 0)
	{
		if (aesobj_ob(&parent)->ob_head > 0)
		{
			start = aesobj(aesobj_tree(&parent), aesobj_ob(&parent)->ob_head);
			if (!(flags & OBFIND_LAST))
				flags |= OBFIND_FIRST;
		}
		else goto done;
	}
	
	if (aesobj_ob(&parent)->ob_head == -1)
	{
// 		if (d) display("parent head -1");
		return co;
	}

	curr = aesobj(tree, 0);
	x = 0;
	y = 0;
	
	if ((flags & OBFIND_FIRST))
	{
		r.x = r.y = -1;
		r.w = r.h = 1;
		flags &= ~(OBFIND_HOR|OBFIND_DOWN|OBFIND_NOWRAP);
		flags |= OBFIND_DOWN;
// 		if (d) display(" FIND FIRST");
	}
	else if ((flags & OBFIND_LAST))
	{
		ob_rectangle(tree, parent, &r);
		r.x += r.w;
		r.y += r.h;
		r.w = r.h = 1;
		flags &= ~(OBFIND_LAST|OBFIND_HOR|OBFIND_DOWN|OBFIND_NOWRAP);
		flags |= OBFIND_HOR|OBFIND_UP;
// 		if (d) display(" FIND LAST");
	}
	else
		ob_rectangle(tree, start, &r);
	
// 	if (d) display("  realstart %d (%d/%d/%d/%d)", aesobj_item(&start), r);
	
	do
	{
		flg = 0;
uplink:
		if (same_aesobj(&parent, &curr))
		{
			if (!dothings)
			{
// 				if (d) display("start doing things at obj %d, x=%d,y=%d", aesobj_item(&curr), x, y);
				dothings = true;
				rel_depth = 0;
				stop = parent;
			}
		}

		if (set_aesobj_uplink(&tree, &curr, &stop, &oblink))
		{
// 			if (d)
// 			{
// 				display("uplink from %d in %lx to %d in %lx",
// 					oblink->from.item, oblink->from.tree,
// 					oblink->to.item, oblink->to.tree);
// 			}
			goto uplink;
		}
		
		this = aesobj_ob(&curr);
		
		if (dothings && rel_depth > 0 && (!mf || !(this->ob_flags & mf)) && (!ms || !(this->ob_state & ms)))
		{
			if (flags & OBFIND_EXACTFLAG)
			{
				if (!f || (this->ob_flags & f) == f)
					flg |= 1;
			}
			else if (!f || (this->ob_flags & f))
				flg |= 1;

			if (flags & OBFIND_EXACTSTATE)
			{
				if (!s || (this->ob_state & s) == s)
					flg |= 2;
			}
			else if (!s || (this->ob_state))
				flg |= 2;
		}

		if (flg == 3)
		{
			flg = 0;

			x += this->ob_x;
			y += this->ob_y;
			w = this->ob_width;
			h = this->ob_height;

			if (!(flags & OBFIND_HOR))
			{
				if (!(flags & OBFIND_DOWN))
				{
					if ( ((x <= r.x && (x + w) > r.x) || (x > r.x && x < (r.x + r.w))))
					{
						if ((y + h) <= r.y)
						{
							if (!cf || (r.y - y) < cy)
							{
								cy = r.y - y;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if ((y + h) < r.y)
						{
							cx = x - r.x;
							if (cx < 0)
								cx = -cx;
					
							if (cx < ax || (cx == ax && (r.y - y) < cy))
							{
								ax = cx;
								cy = r.y - y;
								co = curr;
							}
						}
					}
				}
				else
				{
					if ( ((x <= r.x && (x + w) > r.x) || (x > r.x && x < (r.x + r.w))))
					{
						if (y >= (r.y + r.h))
						{
							if (!cf || (y - r.y) < cy)
							{
								cy = y - r.y;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if (y >= (r.y + r.h))
						{
							if ((y - r.y) == cy)
							{
								if (x < ax)
								{
									ax = x;
									cy = y - r.y;
									co = curr;
								}
							}
							else if ((y - r.y) < cy)
							{
								ax = x;
								cy = y - r.y;
								co = curr;
							}
						}
					}
				}
				
			}
			else
			{
				if (!(flags & OBFIND_DOWN))
				{
					if ( ((y <= r.y && (y + h) > r.y)) || (y > r.y && (y + h) < (r.y + r.h)))
					{
						if ((x < r.x))
						{
							if (!cf || (r.x - x) < cx)
							{
								cx = r.x - x;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if (y < r.y)
						{
							if ((r.y - y) == cy)
							{
								if (ax == 32000 || x > ax)
								{
									ax = x;
									cy = r.y - y;
									co = curr;
								}
							}
							else if ((r.y - y) < cy)
							{
								ax = x;
								cy = r.y - y;
								co = curr;
							}
						}
					}
				}
				else
				{
					if ( (y >= r.y && y < (r.y + r.h)) || (y < r.y && (y + h) >= (r.y + r.h)))
					{
						if (x > r.x)
						{
							if (!cf || (x - r.x) < cx)
							{
								cx = x - r.x;
								co = curr;
								cf = 1;
							}
						}
					}

					if (!cf)
					{
						if (y >= (r.y + r.h) || (y > r.y && (y + h) > (r.y + r.h)))
						{
							if ((y - r.y) == cy)
							{
								if (x < ax)
								{
									ax = x;
									cy = y - r.y;
									co = curr;
								}
							}
							else if ((y - r.y) < cy)
							{
								ax = x;
								cy = y - r.y;
								co = curr;
							}
						}		
					}
				}
			}
			
			x -= this->ob_x;
			y -= this->ob_y;
		}

		if (aesobj_ob(&curr)->ob_head != -1 && (!(aesobj_ob(&curr)->ob_flags & OF_HIDETREE) || (flags & OBFIND_HIDDEN)))
		{
// 			if (d) display(" parent %d, head = %d", aesobj_item(&curr), aesobj_ob(&curr)->ob_head);
			x += aesobj_ob(&curr)->ob_x;
			y += aesobj_ob(&curr)->ob_y;
			curr = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_head);
			rel_depth++;
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
			
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);
				if (same_aesobj(&curr, &tail))
				{
// 					if (d) display(" childs of %d done", aesobj_item(&curr));
					curr = next;
					x -= aesobj_ob(&curr)->ob_x;
					y -= aesobj_ob(&curr)->ob_y;
					next = aesobj(aesobj_tree(&curr), aesobj_ob(&curr)->ob_next);
					rel_depth--;
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&tree, &curr, &stop, &oblink))
			{
// 				if (d) display("downlink to %d in %lx", curr.item, curr.tree);
				x -= aesobj_ob(&curr)->ob_x;
				y -= aesobj_ob(&curr)->ob_y;
				goto downlink;
			}
			curr = next;
			
// 			if (d) display(" next %d", aesobj_item(&curr));
		}
	} while (valid_aesobj(&curr) && !same_aesobj(&curr, &stop) && rel_depth > 0);

	if (!cf && (flags & OBFIND_NOWRAP))
	{
// 		if (d) display(" nowrap !");
		co = inv_aesobj();
	}
done:
	clean_aesobj_links(&oblink);

// 	if (d) display("return %d", aesobj_item(&co));
	return co;
}

struct xa_aes_object
ob_find_next_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	struct xa_aes_object object = ob_find_any_flag(tree, OF_LASTOB, 0, 0);

	DIAGS(("ob_find_next_any_flag: start=%d, flags=%x, lastob=%d",
		start, f, object.item));
	/*
	 * Check if start is inside.
	 */
	if (!valid_aesobj(&object) || object.item < start)
	{
		return inv_aesobj();
	}
	
	/*
	 * Start at last object in tree? Wrap if so
	 */
	if (start == aesobj_item(&object))
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
			break;
		if (tree[o].ob_flags & OF_LASTOB)
			o = 0;
		else
			o++;
	}
	return aesobj(tree, o);
}
short
ob_find_prev_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	struct xa_aes_object object = ob_find_any_flag(tree, OF_LASTOB, 0, 0);

	/*
	 * If start == -1, start at last object.
	 */
	 if (start == -1)
	 	start = object.item;
	/*
	 * Check if start is inside.
	 */
	if (object.item < 0 || object.item < start)
		return -1;
	
	/*
	 * Start at first object in tree? Wrap if so
	 */
	if (!start)
		o = object.item;
	else
		o--;

	while ( o != start )
	{
		if (tree[o].ob_flags & f)
			return o;
		if (!o)
			o = object.item;
		else
			o--;
	}
	return -1;
}

struct xa_aes_object
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
						{
							return aesobj(ob, f);
						}
					}
				}
			}
		}
	} while (!(ob[f++].ob_flags & OF_LASTOB));

	return inv_aesobj();
}

void
ob_fix_shortcuts(OBJECT *obtree, bool not_hidden)
{
	struct sc
	{
		short o;
		char c;
	};
	struct sc *sc, *scuts;
	short objs;
	long len;
	char nk;

	DIAG((D_rsrc, NULL, "ob_fix_shortcuts: tree=%lx)", obtree));
	
	objs = ob_count_objs(obtree, 0, -1);
	DIAGS((" -- %d objects", objs));
	
	len = ((long)objs + 1) * sizeof(struct sc);

	sc = kmalloc(len);
	
	DIAGS((" -- sc = %lx", sc));

	if (sc)
	{
		int i = 0;
		bzero(sc, len);
		
		do {
			OBJECT *ob = obtree + i;
			
			DIAGS((" -- obj %d, type %x (n=%d, h=%d, t=%d)",
				i, ob->ob_type, ob->ob_next, ob->ob_head, ob->ob_tail));

			if ((ob->ob_state & OS_WHITEBAK) && (!not_hidden || !(ob->ob_flags & OF_HIDETREE)))
			{
				short ty = ob->ob_type & 0xff;
			
				if (ty == G_BUTTON || ty == G_STRING)
				{
					int j = (ob->ob_state >> 8) & 0x7f;
// 					int nc = 0;
					DIAGS((" -- obj %d, und = %d", i, j));
					if (j < 126)
					{
						char *s = object_get_spec(ob)->free_string;
// 						display(" '%s'", s);
						if (s)
						{
							int slen = strlen(s);
							int nc = 0;
							
							if (j < slen)
							{
								scuts = sc;
								nk = toupper(*(s + j));
								
// 								ndisplay("got %c '", nk);
								
								while (scuts->c && nc < slen)
								{
// 									ndisplay(",%c", scuts->c);
									if (scuts->c == nk)
									{
										nk = toupper(*(s + nc));
										nc++;
										scuts = sc;
// 										ndisplay("-u->nk=%c(%d)", nk, nc);
									}
									else
										scuts++;
								}
// 								display("'");
								if (!scuts->c)
								{
// 									display(" free");
									scuts->c = nk;
									scuts->o = i;
								}
								if (nc)
								{
									if (nc <= slen)
									{
// 										display(" remaped from %d(%c) to %d(%c)", j, *(s + j), nc - 1, *(s + (nc - 1)));
										nc--;
										ob->ob_state = (ob->ob_state & 0x80ff) | ((nc & 0x7f) << 8);
									}
									else
									{
// 										display(" no free keys");
										ob->ob_state &= (~OS_WHITEBAK & 0x80ff);
									}
								}
							}
						}
					}
				}
			}
		} while ( !(obtree[i++].ob_flags & OF_LASTOB));

		kfree(sc);
	}
	DIAGS((" -- ob_fix_shortcuts: done"));	
}

struct xa_aes_object
ob_find_shortcut(OBJECT *tree, ushort nk)
{
	int i = 0;

	nk &= 0xff;

	DIAG((D_keybd, NULL, "find_shortcut: %d(0x%x), '%c'", nk, nk, nk));

	do {
		OBJECT *ob = tree + i;
		if ((ob->ob_state & (OS_WHITEBAK | OS_DISABLED)) == OS_WHITEBAK)
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
						if (j < strlen(s))
						{
							DIAG((D_keybd,NULL,"  -  in '%s' find '%c' on %d :: %c",s,nk,j, *(s+j)));
							if (toupper(*(s + j)) == nk)
							{
								return aesobj(tree, i);
							}
						}
					}
				}
			}
		}
	} while ( (tree[i++].ob_flags & OF_LASTOB) == 0);

	return inv_aesobj();
}

void
obj_init_focus(XA_TREE *wt, short flags)
{
	/*
	 * Ozk: init focus;
	 * 1. Look for editable, if found it is focus, else..
	 * 2. Look for default, if found it is focus, else..
	 * 3. Look for first clickable object, if found it is focus, else..
	 * 4. No focus found.
	 */
	if (flags & OB_IF_RESET)
		clear_focus(wt);

	if (!focus_set(wt))
	{
		struct xa_aes_object o;
		OBJECT *obtree = wt->tree;
		
		o = ob_find_next_any_flagstate(obtree, aesobj(obtree, 0), inv_aesobj(), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_EXACTFLAG);
		if (valid_aesobj(&o))
			wt->focus = o;
		else if (!(flags & OB_IF_ONLY_EDITS))
		{
			{
				o = ob_find_next_any_flagstate(obtree, aesobj(obtree, 0), inv_aesobj(), OF_DEFAULT, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_EXACTFLAG);
				if (valid_aesobj(&o) && (aesobj_ob(&o)->ob_flags & (OF_SELECTABLE|OF_EXIT|OF_TOUCHEXIT)))
					wt->focus = o;
			}
			if (!focus_set(wt))
			{
				o = ob_find_next_any_flagstate(obtree, aesobj(obtree, 0), inv_aesobj(), OF_SELECTABLE|OF_EXIT|OF_TOUCHEXIT, OF_HIDETREE, 0, OS_DISABLED, 0, 0, 0);
				if (valid_aesobj(&o))
					wt->focus = o;
			}
		}
	}
}

void
obj_set_g_popup(XA_TREE *swt, struct xa_aes_object sobj, POPINFO *pinf)
{
	short type;
	
	type = aesobj_ob(&sobj)->ob_type & 0xff;

	if (type == G_BUTTON || type == G_POPUP)
	{
		aesobj_ob(&sobj)->ob_type = G_POPUP;
		object_set_spec(aesobj_ob(&sobj), (unsigned long)pinf);
	}
}

void
obj_unset_g_popup(XA_TREE *swt, struct xa_aes_object sobj, char *txt)
{
	short type;
	
	type = aesobj_ob(&sobj)->ob_type & 0xff;

	if (type == G_POPUP)
	{
		aesobj_ob(&sobj)->ob_type = G_BUTTON;
		object_set_spec(aesobj_ob(&sobj), (unsigned long)txt);
	}
}

/*
 * Get the true screen coords of an object
 */
short
obj_offset(XA_TREE *wt, struct xa_aes_object object, short *mx, short *my)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object current = aesobj(obtree, 0), stop = inv_aesobj(), next;
	struct oblink_spec *oblink = NULL;
	short x = -wt->dx, y = -wt->dy;
	
	DIAG((D_objc, NULL, "obj_offset: obtree=%lx, obj=%d, xret=%lx, yret=%lx",
		obtree, aesobj_item(&object), mx, my));

	do
	{

uplink:
		/* Found the object in the tree? cool, return the coords */
		if (same_aesobj(&current, &object))
		{
			x += aesobj_ob(&current)->ob_x;
			y += aesobj_ob(&current)->ob_y;
			
			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "obj_offset: return found obj=%d at x=%d, y=%d",
				object, x, y));
			clean_aesobj_links(&oblink);
			return 1;
		}
		
		if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
			goto uplink;

		/* Any children? */
		if (aesobj_ob(&current)->ob_head != -1)
		{
			x += aesobj_ob(&current)->ob_x;
			y += aesobj_ob(&current)->ob_y;
			current = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_head);
		}
		else
		{
			/* Try for a sibling */
downlink:			
			next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);
			
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);
				if (same_aesobj(&current, &tail))
				{
					/* Trace back up tree if no more siblings */
					current = next;
					x -= aesobj_ob(&current)->ob_x;
					y -= aesobj_ob(&current)->ob_y;
					next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_ob(&current)->ob_x;
				y -= aesobj_ob(&current)->ob_y;
				goto downlink;
			}
			current = next;
		}
	}
	while (valid_aesobj(&current)); /* If 'current' is -1 then we have finished */

	clean_aesobj_links(&oblink);

	/* Bummer - didn't find the object, so return error */
	DIAG((D_objc, NULL, "obj_offset: didnt find object"));
	return 0;
}

short
ob_offset(OBJECT *obtree, struct xa_aes_object object, short *mx, short *my)
{
	struct xa_aes_object next;
	struct xa_aes_object current = aesobj(obtree, 0), stop = inv_aesobj();
	struct oblink_spec *oblink = NULL;
	short x = 0, y = 0;
	
	DIAG((D_objc, NULL, "ob_offset: obtree=%lx, obj=%d, xret=%lx, yret=%lx",
		obtree, aesobj_item(&object), mx, my));

	do
	{

uplink:
		/* Found the object in the tree? cool, return the coords */
		if (same_aesobj(&current, &object))
		{
			x += aesobj_ob(&current)->ob_x;
			y += aesobj_ob(&current)->ob_y;
			
			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "obj_offset: return found obj=%d at x=%d, y=%d",
				object, x, y));
			clean_aesobj_links(&oblink);
			return 1;
		}
		if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
			goto uplink;

		/* Any children? */
		if (aesobj_ob(&current)->ob_head != -1)
		{
			x += aesobj_ob(&current)->ob_x;
			y += aesobj_ob(&current)->ob_y;
			current = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_head);
		}
		else
		{
			/* Try for a sibling */
downlink:			
			next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);
			
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);
				if (same_aesobj(&current, &tail))
				{
					/* Trace back up tree if no more siblings */
					current = next;
					x -= aesobj_ob(&current)->ob_x;
					y -= aesobj_ob(&current)->ob_y;
					next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_ob(&current)->ob_x;
				y -= aesobj_ob(&current)->ob_y;
				goto downlink;
			}
			current = next;
		}
	}
	while (valid_aesobj(&current)); /* If 'current' is -1 then we have finished */

	clean_aesobj_links(&oblink);

	/* Bummer - didn't find the object, so return error */
	DIAG((D_objc, NULL, "ob_offset: didnt find object"));
	return 0;
}

void
obj_rectangle(XA_TREE *wt, struct xa_aes_object obj, RECT *c)
{
	if (!obj_offset(wt, obj, &c->x, &c->y))
	{
		obj = aesobj(wt->tree, 0);
		obj_offset(wt, obj, &c->x, &c->y);
	}
	c->w = aesobj_ob(&obj)->ob_width;
	c->h = aesobj_ob(&obj)->ob_height;
}
void
ob_rectangle(OBJECT *obtree, struct xa_aes_object obj, RECT *c)
{
	if (!ob_offset(obtree, obj, &c->x, &c->y))
	{
		obj = aesobj(obtree, 0);
		ob_offset(obtree, obj, &c->x, &c->y);
	}
	c->w = aesobj_ob(&obj)->ob_width;
	c->h = aesobj_ob(&obj)->ob_height;
}
void
obj_area(XA_TREE *wt, struct xa_aes_object obj, RECT *c)
{
	RECT r;
	
	if (!obj_offset(wt, obj, &c->x, &c->y))
	{
		obj = aesobj(wt->tree, 0);
		obj_offset(wt, obj, &c->x, &c->y);
	}
	c->w = aesobj_ob(&obj)->ob_width;
	c->h = aesobj_ob(&obj)->ob_height;
	object_offsets(aesobj_ob(&obj), &r);
	c->x += r.x;
	c->y += r.y;
	c->w -= r.w;
	c->h -= r.h;
}

void
ob_area(OBJECT *obtree, struct xa_aes_object obj, RECT *c)
{
	RECT r;
	
	if (!ob_offset(obtree, obj, &c->x, &c->y))
	{
		obj = aesobj(obtree, 0);
		ob_offset(obtree, obj, &c->x, &c->y);
	}
	c->w = aesobj_ob(&obj)->ob_width;
	c->h = aesobj_ob(&obj)->ob_height;
	object_offsets(aesobj_ob(&obj), &r);
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
ob_border_diff(OBJECT *obtree, struct xa_aes_object obj1, struct xa_aes_object obj2, RECT *r)
{
	RECT r1, r2;

	object_offsets(aesobj_ob(&obj1), &r1);
	object_offsets(aesobj_ob(&obj2), &r2);

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
struct xa_aes_object
obj_find(XA_TREE *wt, struct xa_aes_object object, short depth, short mx, short my, RECT *c)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object next;
	struct xa_aes_object current = aesobj(wt->tree, 0), stop = inv_aesobj(), pos_object = inv_aesobj();
	short rel_depth = 1;
	short x = -wt->dx, y = -wt->dy;
	bool start_checking = false;
	struct oblink_spec *oblink = NULL;
	RECT r, or = (RECT){0,0,0,0};

	DIAG((D_objc, NULL, "obj_find: obj=%d, depth=%d, obtree=%lx, obtree at %d/%d/%d/%d, find at %d/%d",
		object, depth, obtree, obtree->ob_x, obtree->ob_y, obtree->ob_width, obtree->ob_height,
		mx, my));
	do {
	
uplink:
		if (same_aesobj(&current, &object))	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}
		
		if (!(aesobj_ob(&current)->ob_flags & OF_HIDETREE))
		{
			if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
				goto uplink;
			
			if (start_checking)
			{
				RECT cr;
				if (depth & 0x8000)
					object_offsets(aesobj_ob(&current), &or);
			
				cr.x = x + aesobj_ob(&current)->ob_x + or.x;
				cr.y = y + aesobj_ob(&current)->ob_y + or.y;
				cr.w = aesobj_ob(&current)->ob_width - or.w;
				cr.h = aesobj_ob(&current)->ob_height - or.h;
			
				if (   cr.x		<= mx
				    && cr.y		<= my
				    && cr.x + cr.w	> mx
				    && cr.y + cr.h	> my )
				{
					/* This is only a possible object, as it may have children on top of it. */
					if (c)
					{
						r = cr;
					}
					pos_object = current;
				}
			}
		}

		if ( ((!start_checking) || (rel_depth < depth))
		    &&  (aesobj_ob(&current)->ob_head != -1)
		    && !(aesobj_ob(&current)->ob_flags & OF_HIDETREE))
		{
			/* Any children? */
			x += aesobj_ob(&current)->ob_x;
			y += aesobj_ob(&current)->ob_y;
			rel_depth++;
			current = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_head);
		}
		else
		{
downlink:
			/* Try for a sibling */
			next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);

			/* Trace back up tree if no more siblings */
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_ob(&next)->ob_tail);

				if (same_aesobj(&current, &tail))
				{
					current = next;
					x -= aesobj_ob(&current)->ob_x;
					y -= aesobj_ob(&current)->ob_y;
					next = aesobj(aesobj_tree(&current), aesobj_ob(&current)->ob_next);
					rel_depth--;
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_ob(&current)->ob_x;
				y -= aesobj_ob(&current)->ob_y;
				goto downlink;
			}
			current = next;
		}	
	}
	while (valid_aesobj(&current) && !same_aesobj(&current, &stop) && (rel_depth > 0));

	clean_aesobj_links(&oblink);

	if (c && valid_aesobj(&pos_object))
	{
		if (!xa_rect_clip(c, &r, &r))
			pos_object = inv_aesobj();
	}
	
	DIAG((D_objc, NULL, "obj_find: found %d", aesobj_item(&pos_object)));

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
	RECT or;

	DIAG((D_objc, NULL, "ob_find: obj=%d, depth=%d, obtree=%lx, obtree at %d/%d/%d/%d, find at %d/%d",
		object, depth, obtree, obtree->ob_x, obtree->ob_y, obtree->ob_width, obtree->ob_height,
		mx, my));
	do {
		if (current == object)	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}
		
		if (start_checking && !(obtree[current].ob_flags & OF_HIDETREE))
		{
			RECT cr;
			
			if (depth & 0x8000)
				object_offsets(obtree + current, &or);
			
			cr.x = x + obtree[current].ob_x + or.x;
			cr.y = y + obtree[current].ob_y + or.y;
			cr.w = obtree[current].ob_width - or.w;
			cr.h = obtree[current].ob_height - or.h;
			
			if (   cr.x		<= mx
			    && cr.y		<= my
			    && cr.x + cr.w	> mx
			    && cr.y + cr.h	> my )
			{
				/* This is only a possible object, as it may have children on top of it. */
				pos_object = current;
			}
		}

		if ( ((!start_checking) || (rel_depth < depth))
		    &&  (obtree[current].ob_head != -1)
		    && !(obtree[current].ob_flags & OF_HIDETREE))
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
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_DEFAULT, 0, 0, 0, 0, 0);
	return o.item >= 0 ? true : false;
}
bool
obtree_has_exit(OBJECT *obtree)
{
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_EXIT, 0, 0, 0, 0, 0);
	return o.item >= 0 ? true : false;
} 
bool
obtree_has_touchexit(OBJECT *obtree)
{
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_TOUCHEXIT, 0, 0, 0, 0, 0);
	return o.item >= 0 ? true : false;
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
	   struct xa_vdi_settings *v,
	   struct xa_aes_object obj,
	   int transdepth,
	   short state,
	   short flags,
	   bool redraw,
	   const RECT *clip,
	   struct xa_rect_list *rl, short dflags)
{
	bool draw = false;

	if (aesobj_ob(&obj)->ob_flags != flags)
	{
		aesobj_ob(&obj)->ob_flags = flags;
		draw |= true;
	}
	if (aesobj_ob(&obj)->ob_state != state)
	{
		aesobj_ob(&obj)->ob_state = state;
		draw |= true;
	}

	if (draw && redraw)
	{
		obj_draw(wt, v, obj, transdepth, clip, rl, dflags);
	}
}

void
obj_draw(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, int transdepth, const RECT *clip, struct xa_rect_list *rl, short flags)
{
	struct xa_aes_object start = obj;
	RECT or;
	bool pd = false;
// 	struct xa_aes_object object;

	hidem();

	obj_area(wt, obj, &or);

	if (transdepth == -2)
		start = aesobj(wt->tree, 0);
	else
	{
		if (transdepth != -1 && (transdepth & 0x8000))
		{
			transdepth &= ~0x8000;
			pd = true;
		}

		while (object_is_transparent(aesobj_ob(&start), pd))
		{
			start = ob_get_parent(wt->tree, start);
			if (!valid_aesobj(&start) || !transdepth)
				break;
			transdepth--;
		}
	}
	
	if (rl)
	{
		RECT r;
		do
		{
			r = rl->r;
			if (!clip || xa_rect_clip(clip, &r, &r))
			{
				if (xa_rect_clip(&or, &r, &r))
				{
					(*v->api->set_clip)(v, &r);
					draw_object_tree(0, wt, wt->tree, v, start, MAX_DEPTH, NULL, flags);
				}
			}
		} while ((rl = rl->next));
	}
	else
	{
		(*v->api->set_clip)(v, &or);
		draw_object_tree(0, wt, wt->tree, v, start, MAX_DEPTH, NULL, flags);
	}
	(*v->api->clear_clip)(v);

	showm();
}
/*
 **************************************************************************
 * object edit functions
 **************************************************************************
 */
static short
delete_marked(struct objc_edit_info *ei, char *txt)
{
	int x, sl = strlen(txt);
	char *s, *d;
	
	if (ei->m_end > ei->m_start)
	{
		s = txt + ei->m_end;
		d = txt + ei->m_start;
		
		for ( x = sl - ei->m_end + 1; x > 0; x--)
			*d++ = *s++;

		ei->pos = ei->m_start;
		ei->m_start = ei->m_end = 0;

		return 1;
	}
	return 0;
}

static bool ed_scrap_copy(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_cut(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_paste(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool obj_ed_char(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt, XTEDINFO *xted, ushort keycode);

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

	if (!(delete_marked(ei, ed_txt->te_ptext)))
	{
		/* clear the edit control */
		*ed_txt->te_ptext = '\0';
		ei->pos = 0;
	}
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
			if (ei->m_end > ei->m_start)
				kernel_write(f, ed_txt->te_ptext + ei->m_start, ei->m_end - ei->m_start);
			else
				kernel_write(f, ed_txt->te_ptext, len);
			
			kernel_close(f);
		}
	}
	return false;
}

static bool
ed_scrap_paste(XA_TREE *wt, struct objc_edit_info *ei, TEDINFO *ed_txt)
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
					obj_ed_char(wt, ei, ed_txt, NULL, (unsigned short)data[i]);

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
	    TEDINFO *ted,
	    XTEDINFO *xted,
	    ushort keycode)
{
	char *txt;
	int x, key, tmask, n, chg;
	bool update = false;

	if (!ted)
		ted = &xted->ti;

	txt = ted->te_ptext;
	
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
		ei->pos = 0;
		ei->m_start = ei->m_end = 0;
		ei->t_offset = ei->p_offset = 0;
		update = true;
		break;
	}
	case 0x537f:	/* DEL deletes character under cursor */
	{
		if (txt[ei->pos])
		{
			if (!(delete_marked(ei, txt)))
			{
				for (x = ei->pos; x < ted->te_txtlen - 1; x++)
					txt[x] = txt[x + 1];
			}
			update = true;
		}
		break;
	}
	case 0x0e08:	/* BACKSPACE deletes character left of cursor (if any) */
	{
		if (!(delete_marked(ei, txt)) && ei->pos)
		{
			for(x = ei->pos; x < ted->te_txtlen; x++)
				txt[x - 1] = txt[x];
			ei->pos--;
		}
		update = true;
		break;
	}
	case 0x4d00:	/* RIGHT ARROW moves cursor right */
	{
		if ((txt[ei->pos]) && (ei->pos < ted->te_txtlen - 1))
		{
			ei->pos++;
			update = false;
		}
		if (ei->m_end > ei->m_start)
		{
			ei->m_start = ei->m_end = 0;
			update = true;
		}
		break;
	}
	case 0x4d36:	/* SHIFT+RIGHT ARROW move cursor to far right of current text */
	{
		if (xted)
		{
			if (txt[ei->pos])
			{
				short npos = ei->pos + 1;
			
				if (ei->m_start == ei->m_end)
				{
					ei->m_start = ei->pos;
					ei->m_end = npos;
				}
				else if (ei->pos > ei->m_start)
					ei->m_end = npos;
				else
					ei->m_start = npos;
			
				ei->pos++;
				update = true;
			}
		}
		else
		{
			for(x = 0; txt[x]; x++)
				;

			if (x != ei->pos)
			{
				ei->pos = x;
				update = false;
			}
		}
		break;
	}
	case 0x4b00:	/* LEFT ARROW moves cursor left */
	{
		if (ei->pos)
		{
			ei->pos--;
			update = false;
		}
		if (ei->m_end > ei->m_start)
		{
			ei->m_start = ei->m_end = 0;
			update = true;
		}
		break;
	}
	case 0x4b34:	/* SHIFT+LEFT ARROW move cursor to start of field */
	{
		if (xted)
		{
			if (ei->pos)
			{
				short npos = ei->pos - 1;
		
				if (ei->m_start == ei->m_end)
				{
					ei->m_end = ei->pos;
					ei->m_start = npos;
				}
				else if (ei->pos > ei->m_start)
					ei->m_end = npos;
				else
					ei->m_start = npos;
			
				ei->pos--;
				update = true;
			}
			break;
		}
		/* else fall through */
	}
	case 0x4700:	/* CLR/HOME also moves to far left */
	{
		if (ei->pos)
		{
			ei->pos = 0;
			update = false;
		}
		if (ei->m_end > ei->m_start)
		{
			ei->m_start = ei->m_end = 0;
			update = true;
		}
		break;
	}
	case 0x2e03:	/* CTRL+C */
	{
		update = ed_scrap_copy(wt, ei, ted);
		break;
	}
	case 0x2d18: 	/* CTRL+X */
	{
		update = ed_scrap_cut(wt, ei, ted);
		break;
	}
	case 0x2f16: 	/* CTRL+V */
	{
		update = ed_scrap_paste(wt, ei, ted);
		break;
	}
	default:	/* Just a plain key - insert character */
	{
		delete_marked(ei, txt);

		chg = 0;/* Ugly hack! */
		if (ei->pos == ted->te_txtlen - 1)
		{
			ei->pos--;
			chg = 1;
		}
				
		key = keycode & 0xff;
		tmask = character_type[key];

		n = strlen(ted->te_pvalid) - 1;
		if (ei->pos < n)
			n = ei->pos;

		switch(ted->te_pvalid[n])
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
			for (n = x = 0; ted->te_ptmplt[n]; n++)
			{
				if (ted->te_ptmplt[n] == '_')
					x++;
				else if (ted->te_ptmplt[n] == key && x >= ei->pos)
					break;
			}
			if (key && (ted->te_ptmplt[n] == key))
			{
				for(n = ei->pos; n < x; n++)
					txt[n] = ' ';
				txt[x] = '\0';
				ei->pos = x;
			}
			else
			{
				ei->pos += chg;		/* Ugly hack! */
				return XAC_DONE;
			}
		}
		else
		{
			txt[ted->te_txtlen - 2] = '\0';	/* Needed! */
			for (x = ted->te_txtlen - 1; x > ei->pos; x--)
				txt[x] = txt[x - 1];

			txt[ei->pos] = (char)key;

			ei->pos++;
		}

		update = true;
		break;
	}
	}
// 	ei->pos = cursor_pos;
	return update;
}

#if GENERATE_DIAGS
char *edfunc[] =
{
	"ED_START",
	"ED_INIT",
	"ED_CHAR",
	"ED_END",
	"ED_4",
	"ED_DISABLE",
	"ED_ENABLE",
	"ED_CRSRON",
	"ED_CRSROFF",
	"ED_MARK",
	"ED_STRING",
	"ED_SETPTEXT",
	"ED_SETPTMPLT",
	"ED_SETPVALID",
	"ED_GETPTEXT",
	"ED_GETPTMPLT",
	"ED_GETPVALID",
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

#define CLRMARKS	0
#define SETMARKS	1
#define SETACTIVE	2

static void
obj_xED_INIT(struct widget_tree *wt,
	     struct objc_edit_info *ei,
	     short pos,
	     short flags)
{
	TEDINFO *ted = &ei->ti;
	short p;

	/* Ozk:
	 * See if the object passed to us really is editable.
	 * We give up here if object not editable, because I think
	 * thats what other aes's does. But before doing that we search
	 * for another editable object which we pass back.
	 * XXX - see how continuing setup with the lookedup object affects
	 *       applications.
	 */
	{
		/* Ozk: 
		 * do things here to end edit of current obj...
		 */
		/* --- */
		
// 		display("xED_INIT: ei=%lx", ei);
// 		display(" -- ted %lx, xted %lx, oted %lx", ted, ei, ei->p_ti);
// 		display(" -- ptext %lx, txtlen %d", ted->te_ptext, ted->te_txtlen);
// 		display(" -- text '%s'", ted->te_ptext);
		
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;

		p = strlen(ted->te_ptext);
		if (pos && (pos == -1 || pos > p))
			pos = p;
		
		ei->m_start = 0;
			
		if (flags & SETMARKS)
			ei->m_end = pos;
		else
			ei->m_end = 0;
		ei->t_offset = 0;
		ei->p_offset = 0;
		
		ei->pos = pos;

		if (flags & SETACTIVE)
		{
			wt->e = *ei;
			wt->ei = ei;
		}
		DIAGS(("xED_INIT: te_ptext='%s', %lx", ted->te_ptext, (long)ted->te_ptext));
// 		display("xED_INIT: te_ptext='%s', %lx", ted->te_ptext, (long)ted->te_ptext);
	}
}

static short
obj_ED_INIT(struct widget_tree *wt,
	    struct objc_edit_info *ei,
	    short obj, short pos, short last,
	    short flags,
	    TEDINFO **ted_ret,
	    XTEDINFO **xted_ret,
	    short *old_ed)
{
	TEDINFO *ted = NULL;
	XTEDINFO *xted = NULL;
	OBJECT *obtree = wt->tree;
	struct xa_aes_object old_ed_obj;
	short p, ret = 0;

	old_ed_obj = inv_aesobj();

	/* Ozk:
	 * See if the object passed to us really is editable.
	 * We give up here if object not editable, because I think
	 * thats what other aes's does. But before doing that we search
	 * for another editable object which we pass back.
	 * XXX - see how continuing setup with the lookedup object affects
	 *       applications.
	 */
	if ( chk_edobj(obtree, obj, last) && (ted = object_get_tedinfo(obtree + obj, &xted)) )
	{
		/* Ozk: 
		 * do things here to end edit of current obj...
		 */
		/* --- */
		old_ed_obj = ei->o;
		/* Ozk:
		 * Init the object.
		 * If new posititon == -1, we place the cursor at the very end
		 * of the text to edit.
		 */
		set_edit(ei, wt->tree, obj);
		
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;

		p = strlen(ted->te_ptext);

		if (pos && (pos == -1 || pos > p))
			pos = p;
		
		if (xted_ret && xted)
		{
			ei->m_start = 0;
			
			if (flags & SETMARKS)
				ei->m_end = pos;
			else
				ei->m_end = 0;
			ei->t_offset = 0;
			ei->p_offset = 0;
		}
		else
			ei->m_start = ei->m_end = 0;
		
		ei->pos = pos;

		DIAGS(("ED_INIT: type %d, te_ptext='%s', %lx", obtree[obj].ob_type, ted->te_ptext, (long)ted->te_ptext));
		ret = 1;
	}
	else
	{
		old_ed_obj = ei->o;
		clear_edit(ei);
		ret = 0;
	}

	DIAGS(("ED_INIT: return ted=%lx, old_ed=%d",
		ted, old_ed_obj));

	if (ted_ret)
		*ted_ret = ted;
	if (xted_ret)
		*xted_ret = xted;
	if (old_ed)
		*old_ed = old_ed_obj.item;

	return ret;
}

static short
obj_xED_END(struct widget_tree *wt,
	    struct xa_vdi_settings *v,
	    struct objc_edit_info *ei,
	    bool redraw,
	    const RECT *clip,
	    struct xa_rect_list *rl)
{
	undraw_objcursor(wt, v, rl, redraw);

	if (ei->m_start != ei->m_end)
	{
		ei->m_start = ei->m_end = 0;
		if (redraw)
			obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);
	}
	return ei->pos;
}

static void
obj_ED_END(struct widget_tree *wt,
	   struct xa_vdi_settings *v,
	   bool redraw,
	   struct xa_rect_list *rl)
{
	/* Ozk: Just turn off cursor :)
	 */
	if (redraw)
	{
		hidem();
		eor_objcursor(wt, v, rl);
		showm();
	}
	wt->e.c_state ^= OB_CURS_EOR;
// 	display("ED_END: eors=%d, obj=%d for %s", wt->e.c_state & DRW_CURSOR, edit_item(&wt->e), wt->owner->name);
}
/*
 * Returns 1 if successful (character eaten), or 0 if not.
 */
short
obj_edit(XA_TREE *wt,
	 struct xa_vdi_settings *v,
	 short func,
	 struct xa_aes_object obj,
	 short keycode,
	 short pos,	/* -1 sets position to end of text */
	 char *string,
	 bool redraw,
	 const RECT *clip,
	 struct xa_rect_list *rl,
	/* outputs */
	 short *ret_pos,
	 struct xa_aes_object *ret_obj)
{
	short ret = 1;
	TEDINFO *ted = NULL;
	XTEDINFO *xted = NULL;
	OBJECT *obtree = wt->tree;
	struct objc_edit_info *ei;
	short old_ed_obj = -1;

#if GENERATE_DIAGS
	char *funcstr = func < 0 || func > 3 ? edfunc[4] : edfunc[func];
	DIAG((D_form,wt->owner,"  --  obj_edit: func %s, wt=%lx obtree=%lx, obj:%d, k:%x, pos:%x",
	      funcstr, wt, obtree, aesobj_item(&obj), keycode, pos));
#endif
#if 0
	char *funcstr = func < 0 || func > 3 ? edfunc[4] : edfunc[func];

	display("obj_edit: %s", wt->owner->name);
	display("  -- func %s, wt=%lx obtree=%lx, obj:%d, k:%x, pos:%x",
	      funcstr, wt, obtree, aesobj_item(&obj), keycode, pos);
#endif

	if (valid_aesobj(&obj))
		ted = object_get_tedinfo(aesobj_ob(&obj), &xted);

	if (!xted) //(!(wt->flags & WTF_OBJCEDIT)) //!(wt->owner->options.app_opts & XAAO_OBJC_EDIT))
	{
		short last = 0;
		while (!(obtree[last].ob_flags & OF_LASTOB))
			last++;

		if (wt->ei)
		{
			obj_xED_END(wt, v, wt->ei, redraw, clip, rl);
			wt->ei = NULL;
			clear_edit(&wt->e);
			wt->e.c_state = 0;
		}
		switch (func)
		{
			case ED_INIT:
			{
				hidem();
				
				obj_ED_INIT(wt, &wt->e, obj.item, -1, last, CLRMARKS, NULL, NULL, &old_ed_obj);
				
				if (redraw)
					eor_objcursor(wt, v, rl);
				wt->e.c_state ^= OB_CURS_EOR;
				
				showm();
				pos = wt->e.pos;
				break;
			}
			case ED_END:
			{
				obj_ED_END(wt, v, redraw, rl);
				if (edit_set(&wt->e))
					pos = wt->e.pos;
				break;
			}
			case ED_CHAR:
			{
				if (!keycode)
					obj_ED_INIT(wt, &wt->e, obj.item, -1, last, CLRMARKS, NULL, NULL, &old_ed_obj);
				else
				{
					hidem();
					if ( !edit_set(&wt->e) ||
					     !valid_aesobj(&obj) ||
					     !same_aesobj(&obj, &wt->e.o))
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

						if (!valid_aesobj(&obj))
						{
							obj = wt->e.o;
							if (!valid_aesobj(&obj))
								obj = ob_find_next_any_flag(obtree, 0, OF_EDITABLE);
						}
						if (obj_ED_INIT(wt, &wt->e, obj.item, pos, last, CLRMARKS, &ted, NULL, &old_ed_obj))
						{
							if (redraw)
							{
								eor_objcursor(wt, v, rl);
								if (obj_ed_char(wt, &wt->e, ted, NULL, keycode))
									obj_draw(wt, v, editfocus(&wt->e), -1, clip, rl, 0);
								eor_objcursor(wt, v, rl);
							}
							else
								obj_ed_char(wt, &wt->e, ted, NULL, keycode);
								
							pos = wt->e.pos;
						}
					}
					else
					{
						/* Ozk:
						 * Object is the one with cursor focus, so we do it normally
						 */
						ted = object_get_tedinfo(aesobj_ob(&obj), NULL);
						ei = &wt->e;

						DIAGS((" -- obj_edit: ted=%lx", ted));

						if (redraw)
						{
							eor_objcursor(wt, v, rl);
							if (obj_ed_char(wt, ei, ted, NULL, keycode))
								obj_draw(wt, v, obj, -1, clip, rl, 0);
							eor_objcursor(wt, v, rl);
						}
						else
							obj_ed_char(wt, ei, ted, NULL, keycode);
					
						pos = ei->pos;
					}
					showm();
				}
				break;
			}
			case ED_CRSR:
			{
				hidem();
				
				obj_ED_INIT(wt, &wt->e, obj.item, pos, last, CLRMARKS, NULL, NULL, &old_ed_obj);
				
				if (redraw)
					eor_objcursor(wt, v, rl);
				wt->e.c_state ^= OB_CURS_EOR;
				
				showm();
				pos = wt->e.pos;
				break;
#if 0
				/* TODO: x coordinate -> cursor position conversion */

				/* TODO: REMOVE: begin ... ED_INIT like position return */
				if (*(ted->te_ptext) == '@')
					wt->e.pos = 0;
				else
					wt->e.pos = strlen(ted->te_ptext);
				/* TODO: REMOVE: end */
#endif
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
		bool drwcurs = false;
		switch (func)
		{
			case ED_INIT:
			{
				ei = wt->ei;
				
				if (ei && same_aesobj(&ei->o, &obj))
					xted = ei;
				else if (valid_aesobj(&obj))
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				
				if (!ei || ei != xted)
				{
					hidem();
					
					if (ei)
					{
						obj_xED_END(wt, v, ei, redraw, clip, rl);
					}
						
					/* Ozk:
					 * If xted is NULL, obj is not a valid object.
					 * We then check if obj is -1, in which case we
					 * automatically place editfocus at first editable
					 * field we find.
					 * If object was just wrong for this tree, we return
					 * a -1 in next obj to indicate this.
					 */
					if (!valid_aesobj(&obj))
					{
						obj = ob_find_next_any_flagstate(obtree, aesobj(obtree, 0), inv_aesobj(), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_FIRST);
						if (valid_aesobj(&obj))
							ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
					}
					if (!xted)
					{
						wt->ei = NULL;
						clear_edit(&wt->e);
						ei = NULL;
					}
					else
					{
						ei = xted;
					
						obj_xED_INIT(wt, ei, pos, SETMARKS|SETACTIVE);
						set_objcursor(wt, v, ei);
// 						enable_objcursor(wt, v);
						if (redraw)
						{
							obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);
						}
						draw_objcursor(wt, v, rl, redraw);
					}
					showm();
				}
				if (ei)
					pos = ei->pos;
				else
					pos = 0;
				break;
			}
			case ED_DISABLE:
			case ED_END:
			{
				if ((ei = wt->ei))
				{
					hidem();
					pos = obj_xED_END(wt, v, ei, redraw, clip, rl);
					showm();
					if (func == ED_END)
					{
						wt->ei = NULL;
						clear_edit(&wt->e);
					}
				}
				else
					ret = 0;
				break;
			}
		#if 0
			case ED_ENABLE:
			{
				if ((ei = wt->ei))
					enable_objcursor(wt, v);
				break;
			}
		#endif
			case ED_CRSROFF:
			{	
				if ((ei = wt->ei))
					undraw_objcursor(wt, v, rl, redraw);
				break;
			}
			case ED_CRSRON:
			{
				if ((ei = wt->ei))
					draw_objcursor(wt, v, rl, redraw);
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

				ei = wt->ei;
				
				if (ei && same_aesobj(&ei->o, &obj))
					xted = ei;
				else if (valid_aesobj(&obj))
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				
				if (!ei || ei != xted)
				{
					DIAGS((" -- obj_edit: on object=%d without cursor focus", obj));
					
					if ((ei = xted))
					{
						obj_xED_INIT(wt, ei, pos, CLRMARKS);
						set_objcursor(wt, v, ei);
					}
				}
				else
					drwcurs = true; //redraw;

				if (ei)
				{
				
					DIAGS((" -- obj_edit: ted=%lx", (long)&xted->ti));

					if (ei != wt->ei)
					
					hidem();
					if (drwcurs)
						undraw_objcursor(wt, v, rl, redraw);
					if (obj_ed_char(wt, ei, NULL, xted, keycode))
					{
						if (redraw)
						{
							obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);
						}
					}
					set_objcursor(wt, v, ei);

					if (drwcurs)
					{
						draw_objcursor(wt, v, rl, redraw);
					}
					pos = ei->pos;
					showm();
				}
				else
				{
					ret = 0;
					pos = 0;
				}
				break;
			}
			case ED_MARK:
			{
				ei = wt->ei;
				
				if (ei && same_aesobj(&ei->o, &obj))
					xted = ei;
				else if (valid_aesobj(&obj))
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				
				if (!ei || ei != xted)
				{
					if ((ei = xted))
						obj_xED_INIT(wt, ei, -1, CLRMARKS);
				}
				else
					drwcurs = true; //redraw;
				
				if (ei)
				{
					unsigned short sl = strlen(ei->ti.te_ptext);
					
					if (sl > keycode && (pos == -1 || pos > keycode))
					{
						ei->m_start = keycode;
						if (pos == -1)
							ei->m_end = sl;
						else
							ei->m_end = pos;
					}
					else
						ei->m_start = ei->m_end = 0;
						
					if (drwcurs)
						undraw_objcursor(wt, v, rl, redraw);
					if (redraw)
						obj_draw(wt, v, obj, -1, clip, rl, 0);
					if (drwcurs)
						draw_objcursor(wt, v, rl, redraw);
				}
				else
					ret = 0;
				
				break;
			}
			case ED_STRING:
			{
				/* Ozk:
				 * Hmm... we allow to edit objects other than the one in wt->e.obj.
				 * However, how should we act here when that other object is not an
				 * editable? Use the wt->e.obj or return with error?
				 * For now we return with error.
				 */

				ei = wt->ei;
				if (ei && same_aesobj(&ei->o, &obj))
					xted = ei;
				else if (valid_aesobj(&obj))
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				
				if (!ei || ei != xted)
				{
					if ((ei = xted))
					{
						obj_xED_INIT(wt, ei, pos, CLRMARKS);
						set_objcursor(wt, v, ei);
					}
				}
				else
					drwcurs = true;

				DIAGS((" -- obj_edit: ted=%lx", ted));
				if (ei)
				{
					hidem();
					if (drwcurs)
						undraw_objcursor(wt, v, rl, redraw);
				
					if (string && string != ei->ti.te_ptext && *string)
					{
						while (*string)
							obj_ed_char(wt, ei, NULL, xted, *(unsigned char *)string++);
						if ((keycode & 1) && wt->ei == ei)
							obj_xED_INIT(wt, ei, -1, SETMARKS);
					}
					else
					{
						if (!string || !*string)
							ei->ti.te_ptext[0] = '\0';
						obj_xED_INIT(wt, ei, -1, ((keycode & 1) && wt->ei == ei) ? SETMARKS : CLRMARKS);
					}

					set_objcursor(wt, v, ei);
					if (redraw)
						obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);

					if (drwcurs)
					{
						draw_objcursor(wt, v, rl, redraw);
					}
					showm();
					pos = ei->pos;
				}
				else
				{
					pos = 0;
					ret = 0;
				}
				break;
			}
			case ED_SETPTEXT:
			{
				ei = wt->ei;
				if (ei && same_aesobj(&ei->o, &obj))
					xted = ei;
				else if (valid_aesobj(&obj))
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				
				if (!ei || ei != xted)
				{
					ei = xted;
				}
				else
					drwcurs = true;
				
				if (ei)
				{
					if (string)
					{
						int chrs;
						
						if (pos == 1)
						{
							chrs = strlen(xted->ti.te_ptext) + 1;
							if (chrs >= xted->ti.te_txtlen)
								chrs = xted->ti.te_txtlen - 1;
							if (chrs > keycode)
								chrs = keycode;
	
							xted->ti.te_ptext[chrs] = '\0';
							strcpy(string, xted->ti.te_ptext);
						}
						xted->ti.te_ptext = string;
						xted->ti.te_txtlen = keycode;
						
						obj_xED_INIT(wt, ei, -1, CLRMARKS);
						set_objcursor(wt, v, ei);
					
						if (redraw)
						{
							if (drwcurs)
								undraw_objcursor(wt, v, rl, redraw);
							obj_draw(wt, v, obj, -1, clip, rl, 0);
							if (drwcurs)
								draw_objcursor(wt, v, rl, redraw);
						}
					}
					pos = ei->pos;
				}
				else
				{
					pos = 0;
					ret = 0;
				}
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
		old_ed_obj, obj.item, wt->e.o.item, pos, ret));

	return ret;
}

void
obj_set_radio_button(XA_TREE *wt,
		      struct xa_vdi_settings *v,
		      struct xa_aes_object obj,
		      bool redraw,
		      const RECT *clip,
		      struct xa_rect_list *rl)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object parent, o;
	
	parent = ob_get_parent(obtree, obj);
	
	DIAG((D_objc, NULL, "obj_set_radio_button: wt=%lx, obtree=%lx, obj=%d, parent=%d",
		wt, obtree, obj, parent));

	if (valid_aesobj(&parent))
	{
		o = aesobj(aesobj_tree(&parent), aesobj_ob(&parent)->ob_head);

		while (valid_aesobj(&o) && !same_aesobj(&o, &parent))
		{
			if ( aesobj_ob(&o)->ob_flags & OF_RBUTTON && aesobj_ob(&o)->ob_state & OS_SELECTED )
			{
				DIAGS(("radio: change obj %d", o));
				if (!same_aesobj(&o, &obj))
				{
					obj_change(wt,
						   v,
						   o, -1,
						   aesobj_ob(&o)->ob_state & ~OS_SELECTED,
						   aesobj_ob(&o)->ob_flags,
						   redraw,
						   clip,
						   rl, 0);
				}	
			}
			o = aesobj(aesobj_tree(&o), aesobj_ob(&o)->ob_next);
		}
		DIAGS(("radio: set obj %d", obj));
		obj_change(wt,
			   v,
			   obj, -1,
			   aesobj_ob(&obj)->ob_state | OS_SELECTED,
			   aesobj_ob(&obj)->ob_flags,
			   redraw, clip,
			   rl, 0);
	}
}

struct xa_aes_object
obj_get_radio_button(XA_TREE *wt,
		      struct xa_aes_object parent,
		      short state)
{
	struct xa_aes_object o = parent;
	
	DIAG((D_objc, NULL, "obj_set_radio_button: wt=%lx, obtree=%lx, parent=%d",
		wt, aesobj_tree(&parent), aesobj_item(&parent)));

	if (valid_aesobj(&parent))
	{
		o = aesobj(aesobj_tree(&parent), aesobj_ob(&parent)->ob_head);

		while (!same_aesobj(&o, &parent))
		{
			if ( aesobj_ob(&o)->ob_flags & OF_RBUTTON && (aesobj_ob(&o)->ob_state & state))
				break;
			o = aesobj(aesobj_tree(&o), aesobj_ob(&o)->ob_next);
		}
	}
	return same_aesobj(&o, &parent) ? inv_aesobj() : o;
}

short
obj_watch(XA_TREE *wt,
	   struct xa_vdi_settings *v,
	   struct xa_aes_object obj,
	   short in_state,
	   short out_state,
	   const RECT *clip,
	   struct xa_rect_list *rl)
{
// 	OBJECT *focus = wt->tree + obj;
// 	short pobf = -2;
	struct xa_aes_object obf = obj, pobf = aesobj(wt->tree, -2);
	short mx, my, mb, x, y, omx, omy;
	short flags = aesobj_ob(&obj)->ob_flags;

	obj_offset(wt, obj, &x, &y);

	x--;
	y--;

	check_mouse(wt->owner, &mb, &omx, &omy);

	if (!mb)
	{
		/* If mouse button is already released, assume that was just
		 * a click, so select
		 */
		obj_change(wt, v, obj, -1, in_state, flags, true, clip, rl, 0);
	}
	else
	{
		S.wm_count++;
		obj_change(wt, v, obj, -1, in_state, flags, true, clip, rl, 0);
		while (mb)
		{
			short s;

			wait_mouse(wt->owner, &mb, &mx, &my);

			if ((mx != omx) || (my != omy))
			{
				omx = mx;
				omy = my;
				obf = obj_find(wt, obj, 10, mx, my, NULL);

				if (same_aesobj(&obf, &obj))
					s = in_state;
				else
					s = out_state;

				if (!same_aesobj(&pobf, &obf))
				{
					pobf = obf;
					obj_change(wt, v, obj, -1, s, flags, true, clip, rl, 0);
				}
			}
		}
		S.wm_count--;
	}

	if (same_aesobj(&obf, &obj))
		return 1;
	else
		return 0;
}
