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

#include "xa_global.h"
#include "draw_obj.h"
#include "obtree.h"
#include "c_window.h"
#include "scrlobjc.h"
#include "rectlist.h"
#include "xa_global.h"
#include "k_mouse.h"
#include "keycodes.h"
#include "util.h"

#include "xaaes.h"

#include "mint/signal.h"

int copy_string_to_clipbrd( char *text );


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
validate_obtree(struct xa_client *client, OBJECT *obtree, const char *fdesc)
{
	if (obtree <= (OBJECT *)0x1000L)
	{
			/* inform user what's going on */
		ALERT((xa_strings(AL_VALOBTREE)/*"%s: validate OBTREE for %s failed, object ptr = %lx, killing it!"*/, fdesc, client->proc_name, obtree));
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

static inline void
aesobj_set_spec(struct xa_aes_object *o, unsigned long cl) { object_set_spec(aesobj_ob(o),cl); }

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

static inline bool
aesobj_has_tedinfo(struct xa_aes_object *o) { return object_has_tedinfo(aesobj_ob(o)); }

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

static inline bool
aesobj_has_freestr(struct xa_aes_object *o) { return object_has_freestr(aesobj_ob(o)); }

bool
object_is_editable(OBJECT *ob, short flags, short state)
{
	if (((ob->ob_flags & OF_EDITABLE) || (flags & OF_EDITABLE)) && !(ob->ob_state & OS_DISABLED || state & OS_DISABLED))
	{
		if (object_has_tedinfo(ob) || (ob->ob_type & 0xff) == G_USERDEF)
			return true;
	}
	return false;
}

inline bool
aesobj_is_editable(struct xa_aes_object *o, short flags, short state)
{
	if ((aesobj_edit(o) || (flags & OF_EDITABLE)) && !(aesobj_disabled(o) || (state & OS_DISABLED))) {
		if (aesobj_has_tedinfo(o) || (aesobj_type(o) & 0xff) == G_USERDEF)
			return true;
	}
	return false;
}

TEDINFO *
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

static inline TEDINFO *
aesobj_get_tedinfo(struct xa_aes_object *o, XTEDINFO **x) { return object_get_tedinfo(aesobj_ob(o), x); }


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
obj_is_transparent(struct widget_tree *wt, OBJECT *ob, bool pdistrans)
{
	return ((*wt->objcr_api->obj_is_transp)(wt, ob, pdistrans));
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

	size = (size + 1) & 0xfffffffe;
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
	long size = 2L * calc_back((GRECT *)&ib->ib_xicon, 1);

	size += sizeof(ICONBLK);
	if (ib->ib_ptext)
		size += strlen(ib->ib_ptext) + 1;

	size = (size + 1) & 0xfffffffe;
	DIAGS(("sizeof_iconblk: %ld", size));
	return size;
}

static long
sizeof_ciconblk(CICONBLK *cb, CICON *i)
{
	long size, plen, clen;
	ICONBLK *ib = &cb->monoblk;

	size = sizeof(CICONBLK);

	plen = calc_back((GRECT *)&ib->ib_xicon, 1);

	if (ib->ib_pdata)
		size += plen;
	if (ib->ib_pmask)
		size += plen;
	if (ib->ib_ptext)
		size += strlen(ib->ib_ptext) + 1;

	if (i)
	{
		clen = plen * i->num_planes;

		if (i->col_data)
			size += clen;
		if (i->col_mask)
			size += plen;
		if (i->sel_data)
			size += clen;
		if (i->sel_mask)
			size += plen;
		size += sizeof(CICON);
	}
	else
	{
		i = cb->mainlist;

		while (i && i != (void *)-1L)
		{
			clen = plen * i->num_planes;

			if (i->col_data)
				size += clen;
			if (i->col_mask)
				size += plen;
			if (i->sel_data)
				size += clen;
			if (i->sel_mask)
				size += plen;
			size += sizeof(CICON);
			i = i->next_res;
		}
	}

	size = (size + 1) & 0xfffffffe;

	DIAGS(("sizeof_ciconblk: %lx, %ld", (unsigned long)cb, size));
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

	DIAG((D_rsrc, NULL, "ob_count_objs: tree %lx, parent %d", (unsigned long)obtree, parent));

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

	DIAGS(("obtree_len: tree = %lx, start = %d, parent = %d", (unsigned long)obtree, start, parent));

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
				size = (size + 1) & 0xfffffffe;
				break;
			}
			case G_ICON:
			{
				size += sizeof_iconblk(object_get_spec(ob)->iconblk);
				break;
			}
			case G_CICON:
			{
				size += sizeof_ciconblk(object_get_spec(ob)->ciconblk, NULL);
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

 	DIAGS(("copy_tedinfo: copy from %lx, to %lx", (unsigned long)src, (unsigned long)dst));

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

	DIAGS(("copy_bitblk: from %lx, to %lx", (unsigned long)src, (unsigned long)dst));

	*dst = *src;

	s = src->bi_pdata;
 	d = (short *)((char *)dst + sizeof(BITBLK));
	dst->bi_pdata = d;
	for (i = 0; i < (src->bi_wb * src->bi_hl); i ++)
		*d++ = *s++;

	DIAGS((" --- return %lx", (unsigned long)d));

	return (void *)d;
}
static void *
copy_iconblk(ICONBLK *src, ICONBLK *dst)
{
	char *d;
	int words, i;

	DIAGS(("copy_iconblk: copy from %lx to %lx", (unsigned long)src, (unsigned long)dst));

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
	long w, cwords;

	DIAGS(("copy_cicon: from %lx to %lx(%lx), words %ld, planes %d", (unsigned long)src, (unsigned long)dst, (unsigned long)d, words, src->num_planes));

	dst->num_planes	= src->num_planes;
	cwords = words * src->num_planes;

	if (src->col_data)
	{
		fr = src->col_data;
		dst->col_data = d;
		for (w = 0; w < cwords; w++)
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
		for (w = 0; w < cwords; w++)
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

	DIAGS(("copy_cicon: return %lx", (unsigned long)d));

	return (void *)d;
}

static void *
copy_ciconblk(CICONBLK *src, CICONBLK *dst, CICON *cicon)
{
	char *d;
	int words, i;

	DIAGS(("copy_ciconblk: from %lx to %lx - cicon=%lx", (unsigned long)dst, (unsigned long)src, (unsigned long)cicon));

	*dst = *src;

	d = (char *)dst + sizeof(CICONBLK);

	words = ((src->monoblk.ib_wicon + 15) >> 4) * src->monoblk.ib_hicon;
	{
		short *fr, *dr = (short *)d;

		fr = src->monoblk.ib_pdata;
		if (fr)
		{
			dst->monoblk.ib_pdata = dr;
			for (i = 0; i < words; i++)
				*dr++ = *fr++;
		}
		else
			dst->monoblk.ib_pdata = NULL;

		fr = src->monoblk.ib_pmask;
		if (fr)
		{
			dst->monoblk.ib_pmask = dr;
			for (i = 0; i < words; i++)
				*dr++ = *fr++;
		}
		else
			dst->monoblk.ib_pmask = NULL;

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

	DIAGS((" --- return %lx", (unsigned long)d));

	return (void *)d;
}

static void
copy_obtree(OBJECT *obtree, short start, OBJECT *dst, void **data)
{
	short parent, curr = start;
	struct xa_aes_object object;

	object = ob_get_parent(obtree, aesobj(obtree, start));
	parent = object.item;

	DIAGS(("copy_obtree: tree = %lx, start = %d, parent = %d, data=%lx/%lx", (unsigned long)obtree, start, parent, (unsigned long)data, (unsigned long)*data));

	do
	{
		OBJECT *ob = obtree + curr;
		OBJECT *on = dst + curr;

		short type = ob->ob_type & 0x00ff;

		DIAGS((" -- ob %d, type = %d, old=%lx, new=%lx", curr, type, (unsigned long)ob, (unsigned long)on));
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

				i = NULL;

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

OBJECT * _cdecl
duplicate_obtree(struct xa_client *client, OBJECT *obtree, short start)
{
	long size;
	short objs;
	OBJECT *new;
	void *data;

	size = obtree_len(obtree, start, &objs);

	DIAGS(("final obtreelen with %d objs is %ld\n", objs, size));

	if (!client)
		client = C.Aes;

	if (client == C.Aes || client == C.Hlp)
		new = kmalloc(size);
	else
		new = umalloc(size);

	if (new)
	{
		bzero(new, size);

		data = (char *)new + ((long)objs * sizeof(OBJECT));

		copy_obtree(obtree, 0, new, &data);
		if ((long)data > (long)new + size)
		{
			DIAGS(("end should be %lx, is %lx -- should return FAIL!!!", (long)new + size, (long)data));
		}
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
				DIAG((D_rsrc, client, "Free extbox parameter bloc %lx", (unsigned long)p));
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

void _cdecl
free_object_tree(struct xa_client *client, OBJECT *obtree)
{
	if (obtree)
	{
		DIAG((D_objc, client, "free_object_tree: %lx for %s", (unsigned long)obtree, client->name));
		free_obtree_resources(client, obtree);
		if (client == C.Aes || client == C.Hlp)
		{
			kfree(obtree);
		}
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
	if (valid_aesobj(&obj) && (aesobj_type(&obj) & 0xff) == G_POPUP)
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

		new = kmalloc(ol + sl + longest + 4);

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

			(*v->api->t_font)(v, cfg.standard_font_point, cfg.font_id);
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

						entry = this;
						this += strlen(this);
						*this++ = ' ';
						*this++ = '\0';
						(*v->api->t_extent)(v, entry/*this*/, &w, &h);
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
#if 0
short
object_thickness(OBJECT *ob)
{
	short t = 0, flags;
	TEDINFO *ted;

	switch (ob->ob_type & 0xff)
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
#endif
#if 0
/*
 * Return offsets to add to object dimensions to account for borders, etc.
 */
void
object_offsets(OBJECT *ob, GRECT *c)
{
	short dx = 0, dy = 0, dw = 0, dh = 0, db = 0;
	short thick;

	if (ob->ob_type != G_PROGDEF)
	{
		thick = object_thickness(ob);   /* type dependent */

		if (thick < 0)
			db = thick;

		/* HR 0080801: oef oef oef, if foreground any thickness has the 3d enlargement!! */
		if (obj_is_foreground(ob))
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
	c->g_x = dx;
	c->g_y = dy;
	c->g_w = dw;
	c->g_h = dh;
}
#endif

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

static void
foreach_object(OBJECT *tree,
		struct xa_aes_object parent,
		struct xa_aes_object start,
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

		if (aesobj_head(&curr) != -1 && !aesobj_hidden(&curr))
		{
			curr = aesobj(aesobj_tree(&curr), aesobj_head(&curr));
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));

			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(&curr, &tail))
				{
					curr = next;
					next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));
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
	short	flags, f, s, mf, ms, t, ret, ret1;
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

		if( d->t && tree[o].ob_type != d->t )
			flg = 0;
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

		if (aesobj_head(&curr) != -1 && !aesobj_hidden(&curr))
		{
			struct par *pn = kmalloc(sizeof(*pn));

			pn->prv = parents;
			pn->this = curr;
			parents = pn;
			curr = aesobj(aesobj_tree(&curr), aesobj_head(&curr));
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));

			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(&curr, &tail))
				{
					struct par *pn;

					if ((pn = parents->prv))
					{
						kfree(parents);
						parents = pn;
					}
					curr = next;
					next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));
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
		aesobj_item(&obj), (unsigned long)aesobj_tree(&obj), aesobj_item(&curr), (unsigned long)aesobj_tree(&curr)));

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
			obj  = aesobj(aesobj_tree(&obj), aesobj_next(&obj));
			tail = aesobj(aesobj_tree(&obj), aesobj_tail(&obj));
		} while (valid_aesobj(&obj) && !same_aesobj(&last, &tail));
	}
	else
		obj = inv_aesobj();

	DIAGS(("ob_get_parent: return %d", obj));

	return obj;
}
#endif

#if INCLUDE_UNUSED
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
	d.t = 0;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), anyflst, &d);
	return d.ret_object;
}
#endif

static struct xa_aes_object
ob_find_any_flag(OBJECT *tree, short f, short mf)
{
	struct anyflst_parms d;

	d.flags = 0;
	d.f = f;
	d.t = 0;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = -1;
	d.ret_object = inv_aesobj();
	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), anyflst, &d);
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
	d.t = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = 0;
	d.ret1 = -1;

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), count_flst, &d);

	if (count)
		*count = d.ret;
	return d.ret1;
}

#if INCLUDE_UNUSED
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
	d.t = 0;
	d.s = 0;
	d.mf = mf;
	d.ms = 0;
	d.ret = 0;
	d.ret1 = -1;

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), count_flst, &d);

	if (count)
		*count = d.ret;

	return d.ret1;
}
#endif

struct xa_aes_object
ob_find_any_flst(OBJECT *tree, short f, short s, short mf, short ms/*, short stopf, short stops*/)
{
	struct anyflst_parms d;

	d.flags = 0;
	d.f = f;
	d.t = 0;
	d.s = s;
	d.mf = mf;
	d.ms = ms;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), anyflst, &d);
	return d.ret_object;
}

struct xa_aes_object
ob_find_flst(OBJECT *tree, short f, short s, short mf, short ms, short stopf, short stops)
{
	struct anyflst_parms d;

	d.flags = OBFIND_EXACTFLAG|OBFIND_EXACTSTATE;
	d.f = f;
	d.t = 0;
	d.s = s;
	d.mf = mf;
	d.ms = ms;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), anyflst, &d);
	return d.ret_object;
}

/*
 * find object with type t
 */
struct xa_aes_object
ob_find_type(OBJECT *tree, short t )
{
	struct anyflst_parms d;

	d.flags = 0;
	d.f = 0;
	d.t = t;
	d.s = 0;
	d.mf = 0;
	d.ms = 0;
	d.ret = -1;
	d.ret_object = inv_aesobj();

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), anyflst, &d);
	return d.ret_object;
}

/* set top/untop-rendering (for list-windows) */
static bool
set_wind(OBJECT *tree, short o, void *_data)
{
	struct anyflst_parms *d = _data;
	tree += o;
	if( tree->ob_type == d->f )
	{
		switch( d->f )
		{
		case G_SLIST:
		{
			struct scroll_info *list = object_get_slist(tree);

			if( list )
			{
				struct xa_window *lwind = list->wi;
				switch( d->t )
				{
				case WM_UNTOPPED:
					setwin_untopped( 0, lwind, true );
				break;
				case WM_ONTOP:
					setwin_ontop( 0, lwind, true );
				break;
				}
			}
		}
		break;
		}
	}
	return false;
}

/*
 *
 */
void ob_set_wind(OBJECT *tree, short f, short t )
{
	struct anyflst_parms d;

	d.f = f;
	d.t = t;

	foreach_object(tree, aesobj(tree, 0), aesobj(tree, 0), set_wind, &d);
}


#define SY_TOL	4	/* sloppy y-coord-matching */

struct xa_aes_object
ob_find_next_any_flagstate(struct widget_tree *wt, struct xa_aes_object parent,
	struct xa_aes_object start, short f, short mf, short s, short ms, short stopf, short stops,
	short flags)
{
	int rel_depth = 1;
	short x, y, w, h, ax, cx, cy, cf, flg;
	bool dothings = false;
	OBJECT *tree = wt->tree, *this;
	struct oblink_spec *oblink = NULL;
	struct xa_aes_object curr, next, stop, co;
	GRECT r;

	cx = cy = ax = 32000;

	co = stop = inv_aesobj();
	cf = 0;

	DIAG((D_rsrc, NULL,"ob_find_any_next_flagstate, parent %d, start %d", aesobj_item(&parent), aesobj_item(&start)));

	if (aesobj_item(&start) <= 0)
	{
		if (aesobj_head(&parent) > 0)
		{
			start = aesobj(aesobj_tree(&parent), aesobj_head(&parent));
			if (!(flags & OBFIND_LAST))
				flags |= OBFIND_FIRST;
		}
		else goto done;
	}

	if (aesobj_head(&parent) == -1)
	{
		return co;
	}

	curr = aesobj(tree, 0);
	x = 0;
	y = 0;

	if ((flags & OBFIND_FIRST))
	{
		r.g_x = r.g_y = -1;
		r.g_w = r.g_h = 1;
		flags &= ~(OBFIND_HOR|OBFIND_DOWN|OBFIND_NOWRAP);
		flags |= OBFIND_DOWN;
	}
	else if ((flags & OBFIND_LAST))
	{
		obj_rectangle(wt, parent, &r);
		r.g_x += r.g_w;
		r.g_y += r.g_h;
		r.g_w = r.g_h = 1;
		flags &= ~(OBFIND_LAST|OBFIND_HOR|OBFIND_DOWN|OBFIND_NOWRAP);
		flags |= OBFIND_HOR|OBFIND_UP;
	}
	else
		obj_rectangle(wt/*tree*/, start, &r);

	do
	{
		flg = 0;
uplink:
		if (same_aesobj(&parent, &curr))
		{
			if (!dothings)
			{
				dothings = true;
				rel_depth = 0;
				stop = parent;
			}
		}

		if (set_aesobj_uplink(&tree, &curr, &stop, &oblink))
		{
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
					if ( ((x <= r.g_x && (x + w) > r.g_x) || (x > r.g_x && x < (r.g_x + r.g_w))))
					{
						if ((y + h) <= r.g_y)
						{
							if (!cf || (r.g_y - y) < cy)
							{
								cy = r.g_y - y;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if ((y + h) < r.g_y)
						{
							cx = x - r.g_x;
							if (cx < 0)
								cx = -cx;

							if (cx < ax || (cx == ax && (r.g_y - y) < cy))
							{
								ax = cx;
								cy = r.g_y - y;
								co = curr;
							}
						}
					}
				}
				else	/*(flags & OBFIND_DOWN)*/
				{
					if ( ((x <= r.g_x && (x + w) > r.g_x) || (x > r.g_x && x < (r.g_x + r.g_w))))
					{
						if (y >= (r.g_y + r.g_h))
						{
							if (!cf || (y - r.g_y) < cy)
							{
								cy = y - r.g_y;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if (y >= (r.g_y + r.g_h))
						{
							if ((y - r.g_y) == cy)
							{
								if (x < ax)
								{
									ax = x;
									cy = y - r.g_y;
									co = curr;
								}
							}
							else if ((y - r.g_y) < cy)
							{
								ax = x;
								cy = y - r.g_y;
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
					if ( (y-h/2 <= r.g_y && y + h > (r.g_y + r.g_h/2)) || (y > r.g_y && (y + h/2) <= (r.g_y + r.g_h)) )
					{
						if ((x < r.g_x))
						{
							if (!cf || (r.g_x - x) < cx)
							{
								cx = r.g_x - x;
								co = curr;
								cf = 1;
							}
						}
					}
					if (!cf)
					{
						if (y < r.g_y)
						{
							if ((r.g_y - y) <= cy + SY_TOL && y + h/2 < r.g_y )
							{
								if( cy == 32000 || ((r.g_y - y) < cy )){	/* line better: forget x */
									cy = r.g_y - y;
									ax = 32000;
								}
								if (ax == 32000 || x > ax)
								{
									ax = x;
									co = curr;
								}
							}
						}
					}
				}
				else
				{
					if ( (y+h/2 >= r.g_y && y+h/2 < (r.g_y + r.g_h)) || (y < r.g_y && (y + h) >= (r.g_y + r.g_h/2)) )
					{
						if (x > r.g_x)
						{
							if (!cf || (x - r.g_x) < cx)
							{
								cx = x - r.g_x;
								co = curr;
								cf = 1;

							}
						}
					}

					if (!cf)
					{
						if (y >= (r.g_y + r.g_h) /*|| (y > r.g_y && (y + h) > (r.g_y + r.g_h))*/)
						{
							if ((y - r.g_y) <= cy + SY_TOL)
							{
								if (x < ax)
								{
									ax = x;
									cy = y - r.g_y;
									co = curr;
								}
							}
						}
					}
				}
			}

			x -= this->ob_x;
			y -= this->ob_y;

		}

		if (aesobj_head(&curr) != -1 && (!aesobj_hidden(&curr) || (flags & OBFIND_HIDDEN)))
		{
			x += aesobj_getx(&curr);
			y += aesobj_gety(&curr);
			curr = aesobj(aesobj_tree(&curr), aesobj_head(&curr));
			rel_depth++;
		}
		else
		{
downlink:
			next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));

			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(&curr, &tail))
				{
					curr = next;
					x -= aesobj_getx(&curr);
					y -= aesobj_gety(&curr);
					next = aesobj(aesobj_tree(&curr), aesobj_next(&curr));
					rel_depth--;
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&tree, &curr, &stop, &oblink))
			{
				x -= aesobj_getx(&curr);
				y -= aesobj_gety(&curr);
				goto downlink;
			}
			curr = next;

		}
	} while (valid_aesobj(&curr) && !same_aesobj(&curr, &stop) && rel_depth > 0);

	if (!cf && (flags & OBFIND_NOWRAP))
	{
		co = inv_aesobj();
	}

done:
	clean_aesobj_links(&oblink);

	return co;
}

static struct xa_aes_object
ob_find_next_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	struct xa_aes_object object = ob_find_any_flag(tree, OF_LASTOB, 0/*, 0*/);

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

#if INCLUDE_UNUSED
short
ob_find_prev_any_flag(OBJECT *tree, short start, short f)
{
	short o = start;
	struct xa_aes_object object = ob_find_any_flag(tree, OF_LASTOB, 0/*, 0*/);

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
#endif

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

static short xa_isalnum( short c )
{
	if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= 0xb0 && c <= 0xff) || (c >= '0' && c <= '9') )
		return true;
	return false;
}

/*
 * define '_'-shortcuts
 * if scp != 0 use external shorcut-buffer to combine dialogs
 */
void
ob_fix_shortcuts(OBJECT *obtree, bool not_hidden, struct sc2 *scp)
{
	struct sc *sc, *scuts;
	short objs;
	char nk;

	DIAG((D_rsrc, NULL, "ob_fix_shortcuts: tree=%lx)", (unsigned long)obtree));

	if( !scp )
	{
		long len;
		objs = ob_count_objs(obtree, 0, -1);
		len = (long)objs * sizeof(struct sc);
		if( !(sc = kmalloc(len) ) )
		{
			BLOG((0,"ob_fix_shortcuts:kmalloc(%ld) failed.", len));
			return;
		}
	}
	else
	{
		objs = scp->objs;
		sc = scp->sc;
	}

	if (sc)
	{
		int i, k;
		short flag = 0, flag2 = 0;

		/*
		 * 1. predefined shortcuts (>0!)
		 * 2. default,exit
		 * 3. touchexit
		 * 4. other
		 */
		for( k = 0; k < 4; k++ )
		{
			switch( k )
			{
			case 0:
				flag = OF_SELECTABLE;
			break;
			case 1:
				flag = OF_DEFAULT | OF_EXIT;
			break;
			case 2:
				flag2 = flag;
				flag = OF_TOUCHEXIT;
			break;
			case 3:
				flag2 |= flag;
				flag = OF_SELECTABLE;
			break;
			}
			for( i = 1; i <= objs; i++ )
			{
				OBJECT *ob = obtree + i;
				DIAGS((" -- obj %d, type %x (n=%d, h=%d, t=%d)",
					i, ob->ob_type, ob->ob_next, ob->ob_head, ob->ob_tail));

				/* look for any buttons match flag and not flag2 - WHITEBAK? */
				if ((ob->ob_flags & flag) && !(ob->ob_flags & flag2) && (!not_hidden || !(ob->ob_flags & OF_HIDETREE)))
				{
					short ty = ob->ob_type & 0xff;

					if (ty == G_BUTTON || ty == G_STRING )
					{
						int j = (ob->ob_state >> 8) & 0x7f;
						DIAGS((" -- obj %d, und = %d", i, j));
						if (j < 126)
						{
							unsigned char *s = (unsigned char *)object_get_spec(ob)->free_string;
							if (s)
							{
								int slen = strlen((char*)s);
								int nc;

								/* skip non-alpha-numeric-chars in start of free_string
								*/
								for( nc = 0; nc < slen && !xa_isalnum(s[nc]); nc++ );
								if( nc >= slen )	/* try any char except blank */
								{
									for( nc = 0; nc < slen && s[nc] <= ' '; nc++ );
								}
								if( j == 0 )
								{
									if( k == 0 )
										continue;	/* only predfined -> WHITEBAK? */
									j = nc;
								}
								else
								{
									if( k > 0 )
										continue;
								}

								if (j < slen)
								{
									scuts = sc;
									nk = xa_toupper((char)*(s + j));

									while (scuts-sc < objs && scuts->c)
									{
										if (i != scuts->o && scuts->c == nk )
										{
											if( nc < slen )
												nk = xa_toupper(*(s + nc++));
											else
												break;
											scuts = sc;
										}
										else
											scuts++;
									}
									if( scuts-sc >= objs )
									{
										BLOG((0,"ob_fix_shortcuts: '%s': buffer-overflow", s));
										continue;
									}
									if (!scuts->c )
									{
										scuts->c = nk;
										scuts->o = i;
										if( nc <= j )
										{
											nc = j;	/* first non-white or preselected char */
										}
										else
											if( nc > 0 )
												nc--;		/* if searched and found nc is 1 char too far */
										if (nc < slen)
										{
											ob->ob_state = (ob->ob_state & 0x80ff) | ((nc & 0x7f) << 8) | OS_WHITEBAK;
										}
										else
										{
											ob->ob_state &= (~OS_WHITEBAK & 0x80ff);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if( !scp )
			kfree(sc);
	}
	DIAGS((" -- ob_fix_shortcuts: done"));
}

/* follow  G_OBLINK
 * todo: same in fix_shortcuts
 */
struct xa_aes_object
ob_find_shortcut(OBJECT *tree, ushort nk, short stop_type)
{
	int i = 1;

	nk &= 0xff;

	DIAG((D_keybd, NULL, "find_shortcut: %d(0x%x), '%c'", nk, nk, nk));

	for( i = 1; tree[i].ob_type != stop_type; i++ )
	{
		OBJECT *ob = tree + i;
		if( ob->ob_type == G_OBLINK )
		{
			struct oblink_spec *oblink = object_get_oblink(ob);
			OBJECT *tr = oblink->to.tree + oblink->to.item;
			struct xa_aes_object xo = ob_find_shortcut( tr, nk, G_BOX );
			if( valid_aesobj( &xo ) )
				return xo;
		}
		else if ((ob->ob_state & (OS_WHITEBAK | OS_DISABLED)) == OS_WHITEBAK)
		{
			int ty = ob->ob_type & 0xff;
			if (ty == G_BUTTON || ty == G_STRING)
			{
				int j = (ob->ob_state >> 8) & 0x7f;
				if (j < 126)
				{
					unsigned char *s = (unsigned char *)object_get_spec(ob)->free_string;
					if (s)
					{
						if (j < strlen((char*)s))
						{
							short c = *(s+j);
							DIAG((D_keybd,NULL,"  -  in '%s' find '%c' on %d :: %c",s,nk,j, *(s+j)));
							if (xa_toupper(c) == xa_toupper(nk))
							{
								return aesobj(tree, i);
							}
						}
					}
				}
			}
		}

		if( (tree[i].ob_flags & OF_LASTOB) )
			break;
	}

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

		o = ob_find_next_any_flagstate(wt, aesobj(obtree, 0), inv_aesobj(), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_EXACTFLAG);
		if (valid_aesobj(&o))
		{
			wt->e.o = wt->focus = o;
		}
		else if (!(flags & OB_IF_ONLY_EDITS))
		{
			{
				o = ob_find_next_any_flagstate(wt, aesobj(obtree, 0), inv_aesobj(), OF_DEFAULT, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_EXACTFLAG);
				if (valid_aesobj(&o) && (aesobj_flags(&o) & (OF_SELECTABLE|OF_EXIT|OF_TOUCHEXIT)))
				{
					wt->focus = o;
				}
			}
			if (!focus_set(wt))
			{
				o = ob_find_next_any_flagstate(wt, aesobj(obtree, 0), inv_aesobj(), OF_SELECTABLE|OF_EXIT|OF_TOUCHEXIT, OF_HIDETREE, 0, OS_DISABLED, 0, 0, 0);
				if (valid_aesobj(&o))
				{
					wt->focus = o;
				}
			}
		}
	}
}

void
obj_set_g_popup(XA_TREE *swt, struct xa_aes_object sobj, POPINFO *pinf)
{
	short type;

	type = aesobj_type(&sobj) & 0xff;

	if (type == G_BUTTON || type == G_POPUP)
	{
		aesobj_settype(&sobj, G_POPUP);
		object_set_spec(aesobj_ob(&sobj), (unsigned long)pinf);
	}
}

void
obj_unset_g_popup(XA_TREE *swt, struct xa_aes_object sobj, char *txt)
{
	short type;

	type = aesobj_type(&sobj) & 0xff;

	if (type == G_POPUP)
	{
		aesobj_settype(&sobj, G_BUTTON);
		object_set_spec(aesobj_ob(&sobj), (unsigned long)txt);
	}
}

/*************************/
#if !AGGRESSIVE_INLINING

bool
disable_object(OBJECT *ob, bool set)
{
	if (set)
	{
		if (!(ob->ob_state & OS_DISABLED)) {
			ob->ob_state |= OS_DISABLED;
			return true;
		}
		else
			return false;
	}
	else
	{
		if ((ob->ob_state & OS_DISABLED)) {
			ob->ob_state &= ~OS_DISABLED;
			return true;
		}
		else
			return false;
	}
}


bool
set_aesobj_uplink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink)
{
	if ((aesobj_type(c) & 0xff) == G_OBLINK)
	{
		struct oblink_spec *obl = object_get_oblink(aesobj_ob(c));
		if (obl)
		{
			obl->save_to_r = *(GRECT *)&obl->to.tree[obl->to.item].ob_x;

			obl->to.tree[obl->to.item].ob_x = aesobj_getx(c);
			obl->to.tree[obl->to.item].ob_y = aesobj_gety(c);

			obl->d.pmisc[1] = *oblink;
			*oblink = obl;

			obl->savestop = *s;
			*t = obl->to.tree;
			*c = *s = aesobj(obl->to.tree, obl->to.item);
			return true;
		}
	}
	return false;
}
bool
set_aesobj_downlink(OBJECT **t, struct xa_aes_object *c, struct xa_aes_object *s, struct oblink_spec **oblink)
{
	if (*oblink)
	{
		OBJECT *tree = (*oblink)->to.tree + (*oblink)->to.item;

		tree->ob_x = (*oblink)->save_to_r.g_x;
		tree->ob_y = (*oblink)->save_to_r.g_y;
		tree->ob_width = (*oblink)->save_to_r.g_w;
		tree->ob_height = (*oblink)->save_to_r.g_h;


		*t = (*oblink)->from.tree;
		*c = aesobj((*oblink)->from.tree, (*oblink)->from.item);
		*s = (*oblink)->savestop;
		*oblink = (*oblink)->d.pmisc[1];
		return true;
	}
	return false;
}

void
clean_aesobj_links(struct oblink_spec **oblink)
{
	while (*oblink)
	{
		OBJECT *tree = (*oblink)->to.tree + (*oblink)->to.item;

		tree->ob_x = (*oblink)->save_to_r.g_x;
		tree->ob_y = (*oblink)->save_to_r.g_y;
		tree->ob_width = (*oblink)->save_to_r.g_w;
		tree->ob_height = (*oblink)->save_to_r.g_h;

		*oblink = (*oblink)->d.pmisc[1];
	}
}
#endif
/*********************************/

/*
 * Get the true screen coords of an object
 *
 * todo: make this and ob_offset one
 *
 */
short
obj_offset(XA_TREE *wt, struct xa_aes_object object, short *mx, short *my)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object current = aesobj(obtree, 0), stop = inv_aesobj(), next;
	struct oblink_spec *oblink = NULL;
	short x = -wt->dx, y = -wt->dy;

	DIAG((D_objc, NULL, "obj_offset: obtree=%lx, obj=%d, xret=%lx, yret=%lx",
		(unsigned long)obtree, aesobj_item(&object), (unsigned long)mx, (unsigned long)my));
	DIAG((D_objc, NULL, " ---- --  : dx=%d, dy=%d", wt->dx, wt->dy));

	do
	{

uplink:
		/* Found the object in the tree? cool, return the coords */
		if (same_aesobj(&current, &object))
		{
			x += aesobj_getx(&current);
			y += aesobj_gety(&current);

			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "obj_offset: return found obj=%d at x=%d, y=%d",
				aesobj_item(&object), x, y));
			if( oblink )
				clean_aesobj_links(&oblink);
			return 1;
		}

		if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
			goto uplink;

		/* Any children? */
		if (aesobj_head(&current) != -1)
		{
			x += aesobj_getx(&current);
			y += aesobj_gety(&current);
			current = aesobj(aesobj_tree(&current), aesobj_head(&current));
		}
		else
		{
			/* Try for a sibling */
downlink:
			next = aesobj(aesobj_tree(&current), aesobj_next(&current));

			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(&current, &tail))
				{
					/* Trace back up tree if no more siblings */
					current = next;
					x -= aesobj_getx(&current);
					y -= aesobj_gety(&current);
					next = aesobj(aesobj_tree(&current), aesobj_next(&current));
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && oblink && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_getx(&current);
				y -= aesobj_gety(&current);
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
		(unsigned long)obtree, aesobj_item(&object), (unsigned long)mx, (unsigned long)my));

	do
	{

uplink:
		/* Found the object in the tree? cool, return the coords */
		if (same_aesobj(&current, &object))
		{
			x += aesobj_getx(&current);
			y += aesobj_gety(&current);

			*mx = x;
			*my = y;

			DIAG((D_objc, NULL, "ob_offset: return found obj=%d at x=%d, y=%d",
				aesobj_item(&object), x, y));
			clean_aesobj_links(&oblink);
			return 1;
		}
		if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
			goto uplink;

		/* Any children? */
		if (aesobj_head(&current) != -1)
		{
			x += aesobj_getx(&current);
			y += aesobj_gety(&current);
			current = aesobj(aesobj_tree(&current), aesobj_head(&current));
		}
		else
		{
			/* Try for a sibling */
downlink:
			next = aesobj(aesobj_tree(&current), aesobj_next(&current));

			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(&current, &tail))
				{
					/* Trace back up tree if no more siblings */
					current = next;
					x -= aesobj_getx(&current);
					y -= aesobj_gety(&current);
					next = aesobj(aesobj_tree(&current), aesobj_next(&current));
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_getx(&current);
				y -= aesobj_gety(&current);
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
obj_rectangle(XA_TREE *wt, struct xa_aes_object obj, GRECT *c)
{
	short sx = wt->dx, sy = wt->dy;

	wt->dx = wt->dy = 0;

	if (!obj_offset(wt, obj, &c->g_x, &c->g_y))
	{
		obj = aesobj(wt->tree, 0);
		obj_offset(wt, obj, &c->g_x, &c->g_y);
	}
	c->g_w = aesobj_getw(&obj);
	c->g_h = aesobj_geth(&obj);
	wt->dx = sx;
	wt->dy = sy;
}

#if INCLUDE_UNUSED
void
obj_orectangle(XA_TREE *wt, struct xa_aes_object obj, GRECT *c)
{
	if (!ob_offset(aesobj_tree(&obj), obj, &c->x, &c->y))
	{
		obj = aesobj(aesobj_tree(&obj), 0);
		ob_offset(aesobj_tree(&obj), obj, &c->x, &c->y);
	}
	c->w = aesobj_getw(&obj);
	c->h = aesobj_geth(&obj);
}
#endif
bool
obj_area(XA_TREE *wt, struct xa_aes_object obj, GRECT *c)
{
	bool found = true;
	GRECT r;

	if (!obj_offset(wt, obj, &c->g_x, &c->g_y))
	{
		obj = aesobj(wt->tree, 0);
		obj_offset(wt, obj, &c->g_x, &c->g_y);
		found = false;
	}
	c->g_w = aesobj_getw(&obj);
	c->g_h = aesobj_geth(&obj);
	(*wt->objcr_api->obj_offsets)(wt, aesobj_ob(&obj), &r);
	c->g_x += r.g_x;
	c->g_y += r.g_y;
	c->g_w -= r.g_w;
	c->g_h -= r.g_h;
	return found;
}

/*
 * Calculate the differance between border corrections for two
 * objects. For example a slider where the slide parent is a non-3d
 * object while the slider is, we need to take 3d border offsets
 * into accrount when positioning/sizing the slider.
 */
void
obj_border_diff(struct widget_tree *wt, struct xa_aes_object obj1, struct xa_aes_object obj2, GRECT *r)
{
	GRECT r1, r2;

	(*wt->objcr_api->obj_offsets)(wt, aesobj_ob(&obj1), &r1);
	(*wt->objcr_api->obj_offsets)(wt, aesobj_ob(&obj2), &r2);

	r->g_x = r1.g_x - r2.g_x;
	r->g_y = r1.g_y - r2.g_y;
	r->g_w = r1.g_w - r2.g_w;
	r->g_h = r1.g_h - r2.g_h;
}

void
ob_spec_xywh(OBJECT *obtree, short obj, GRECT *r)
{
	short type = obtree[obj].ob_type & 0xff;

	switch (type)
	{
		case G_IMAGE:
		{
			BITBLK *bb = object_get_spec(obtree + obj)->bitblk;

			r->g_x = bb->bi_x;
			r->g_y = bb->bi_y;
			r->g_w = bb->bi_wb;
			r->g_h = bb->bi_hl;
			break;
		}
		case G_ICON:
		{
			*r = *(GRECT *)&(object_get_spec(obtree + obj))->iconblk->ib_xicon;
			break;
		}
		case G_CICON:
		{
			*r = *(GRECT *)&(object_get_spec(obtree + obj))->ciconblk->monoblk.ib_xicon;
			break;
		}
		default:
		{
			*r = *(GRECT *)&(obtree[obj].ob_x);
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
obj_find(XA_TREE *wt, struct xa_aes_object object, short depth, short mx, short my, GRECT *c)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object next;
	struct xa_aes_object current = aesobj(wt->tree, 0), stop = inv_aesobj(), pos_object = inv_aesobj();
	short rel_depth = 1;
	short x = -wt->dx, y = -wt->dy;
	bool start_checking = false;
	struct oblink_spec *oblink = NULL;
	GRECT r, or = (GRECT){0,0,0,0};

	DIAG((D_objc, NULL, "obj_find: obj=%d, depth=%d, obtree=%lx, obtree at %d/%d/%d/%d, find at %d/%d",
		object.item, depth, (unsigned long)obtree, obtree->ob_x, obtree->ob_y, obtree->ob_width, obtree->ob_height,
		mx, my));
	do {

uplink:
		if (same_aesobj(&current, &object))	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}

		if (!aesobj_hidden(&current))
		{
			if (set_aesobj_uplink(&obtree, &current, &stop, &oblink))
				goto uplink;

			if (start_checking)
			{
				GRECT cr;
				if (depth & 0x8000)
					(*wt->objcr_api->obj_offsets)(wt, aesobj_ob(&current), &or);

				cr.g_x = x + aesobj_getx(&current) + or.g_x;
				cr.g_y = y + aesobj_gety(&current) + or.g_y;
				cr.g_w = aesobj_getw(&current) - or.g_w;
				cr.g_h = aesobj_geth(&current) - or.g_h;

				if (   cr.g_x		<= mx
				    && cr.g_y		<= my
				    && cr.g_x + cr.g_w	> mx
				    && cr.g_y + cr.g_h	> my )
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
		    &&  (aesobj_head(&current) != -1)
		    &&  !aesobj_hidden(&current))
		{
			/* Any children? */
			x += aesobj_getx(&current);
			y += aesobj_gety(&current);
			rel_depth++;
			current = aesobj(aesobj_tree(&current), aesobj_head(&current));
		}
		else
		{
downlink:
			/* Try for a sibling */
			next = aesobj(aesobj_tree(&current), aesobj_next(&current));

			/* Trace back up tree if no more siblings */
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));

				if (same_aesobj(&current, &tail))
				{
					current = next;
					x -= aesobj_getx(&current);
					y -= aesobj_gety(&current);
					next = aesobj(aesobj_tree(&current), aesobj_next(&current));
					rel_depth--;
				}
				else
					break;
			}
			if (same_aesobj(&next, &stop) && set_aesobj_downlink(&obtree, &current, &stop, &oblink))
			{
				x -= aesobj_getx(&current);
				y -= aesobj_gety(&current);
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
#if 0
short
ob_find(OBJECT *obtree, short object, short depth, short mx, short my)
{
	short next;
	short current = 0, rel_depth = 1;
	short x = 0, y = 0;
	bool start_checking = false;
	short pos_object = -1;
	GRECT or;

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
			GRECT cr;

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
#endif

#if GENERATE_DIAGS
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
#endif
bool
obtree_has_default(OBJECT *obtree)
{
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_DEFAULT, 0, 0, 0/*, 0, 0*/);
	return o.item >= 0 ? true : false;
}
#if INCLUDE_UNUSED
bool
obtree_has_exit(OBJECT *obtree)
{
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_EXIT, 0, 0, 0/*, 0, 0*/);
	return o.item >= 0 ? true : false;
}
#endif
#if INCLUDE_UNUSED
bool
obtree_has_touchexit(OBJECT *obtree)
{
	struct xa_aes_object o = ob_find_any_flst(obtree, OF_TOUCHEXIT, 0, 0, 0/*, 0, 0*/);
	return o.item >= 0 ? true : false;
}
#endif
/* HR 120601: Objc_Change:
 * We go back thru the parents of the object until we meet a opaque object.
 *    This is to ensure that transparent objects are drawn correctly.
 * Care must be taken that outside borders or shadows are included in the draw.
 * Only the objects area is redrawn, so it must be intersected with clipping rectangle.
 * New function object_area(GRECT *c, tree, item)
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
	   const GRECT *clip,
	   struct xa_rect_list *rl, short dflags)
{
	bool draw = false;

	if (aesobj_flags(&obj) != flags) {
		aesobj_setflags(&obj, flags);
		draw |= 1;
	}
	if (aesobj_state(&obj) != state) {
		aesobj_setstate(&obj, state);
		draw |= 2;
	}

	if (draw && redraw) {
#if 1
		GRECT r;
		/* "sysinfo"-fix */
		if( obj.ob->ob_type == G_USERDEF && !clip)
		{
			r.g_x = obj.ob->ob_x;
			r.g_y = obj.ob->ob_y;
			r.g_w = obj.ob->ob_width;
			r.g_h = obj.ob->ob_height;
			clip = &r;
		}
#endif

		if( obj.ob->ob_y == 0 && (aesobj_type(&obj) == G_TITLE || aesobj_type(&obj) == G_USERDEF) )
			rl = 0;
		obj_draw(wt, v, obj, transdepth, clip, rl, dflags);	/* of y=0 and G_TITLE it's a menu-title (don't clip) */
	}
}

void
obj_draw(XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object obj, int transdepth, const GRECT *clip, struct xa_rect_list *rl, short flags)
{
	struct xa_aes_object start = obj;
	GRECT or;
	bool pd = false;

	if (!obj_area(wt, obj, &or))
	{
		return;
	}

	hidem();

	if (transdepth == -2)
		start = aesobj(wt->tree, 0);
	else {
		if (transdepth != -1 && (transdepth & 0x8000)) {
			transdepth &= ~0x8000;
			pd = true;
		}

		while (obj_is_transparent(wt, aesobj_ob(&start), pd)) {
			start = ob_get_parent(wt->tree, start);
			if (!valid_aesobj(&start) || !transdepth)
				break;
			transdepth--;
		}
	}

	if (rl) {
		GRECT r;
		do {
			r = rl->r;
			if (!clip || xa_rect_clip(clip, &r, &r)) {
				if (xa_rect_clip(&or, &r, &r)) {
					(*v->api->set_clip)(v, &r);
					draw_object_tree(0, wt, wt->tree, v, start, MAX_DEPTH, NULL, flags);
				}
			}
		} while ((rl = rl->next));
	} else {
		if (clip)
			xa_rect_clip(clip, &or, &or);

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

	if (ei->m_end > ei->m_start) {
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

static bool ed_scrap_copy(struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_cut(struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool ed_scrap_paste(struct objc_edit_info *ei, TEDINFO *ed_txt);
static bool obj_ed_char(struct objc_edit_info *ei, TEDINFO *ed_txt, XTEDINFO *xted, ushort keycode);


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
ed_scrap_cut(struct objc_edit_info *ei, TEDINFO *ed_txt)
{
	ed_scrap_copy(ei, ed_txt);

	if (!(delete_marked(ei, ed_txt->te_ptext)))
	{
		/* clear the edit control */
		*ed_txt->te_ptext = '\0';
		ei->pos = 0;
	}
	return true;
}

/*
 * wt unused!
 */
static bool
ed_scrap_copy(struct objc_edit_info *ei, TEDINFO *ed_txt)
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
ed_scrap_paste(struct objc_edit_info *ei, TEDINFO *ed_txt)
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
					obj_ed_char(ei, ed_txt, NULL, (unsigned short)data[i]);

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
obj_ed_char(
	    struct objc_edit_info *ei,
	    TEDINFO *ted,
	    XTEDINFO *xted,
	    ushort keycode)
{
	char *txt;
	int x, key, tmask=-1, n=-1, chg;
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
	case SC_ESC:	/* 0x011b */	/* ESCAPE clears the field */
	{
		txt[0] = '\0';
		ei->pos = 0;
		ei->m_start = ei->m_end = 0;
		ei->t_offset = ei->p_offset = 0;
		update = true;
		break;
	}
	case SC_DEL:	/* 0x537f */	/* DEL deletes character under cursor */
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
	case SC_BACKSPACE: /* 0x0e08 */	/* BACKSPACE deletes character left of cursor (if any) (todo: skip non-edit-chars) */
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
	case SC_RTARROW: /* 0x4d00 */	/* RIGHT ARROW moves cursor right */
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
	case SC_SHFT_RTARROW: /* 0x4d36 */ /* SHIFT+RIGHT ARROW move cursor to far right of current text */
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
	case SC_LFARROW: /* 0x4b00 */	/* LEFT ARROW moves cursor left (todo: skip non-edit-chars) */
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
	case SC_SHFT_LFARROW: /* 0x4b34 */ /* SHIFT+LEFT ARROW move cursor to start of field */
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
	case SC_CLRHOME: /* 0x4700 */	/* CLR/HOME also moves to far left */
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
	case SC_CTRL_C:	/* 0x2e03 */	/* CTRL+C */
	{
		update = ed_scrap_copy(ei, ted);
		break;
	}
	case SC_CTRL_X: /* 0x2d18 */ 	/* CTRL+X */
	{
		update = ed_scrap_cut(ei, ted);
		break;
	}
	case SC_CTRL_V: /* 0x2f16 */ 	/* CTRL+V */
	{
		update = ed_scrap_paste(ei, ted);
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

		if( key == 0 )
			return false;

		if ((n = strlen(ted->te_pvalid) - 1) < 0)
		{
			txt[ted->te_txtlen - 2] = '\0';
			for (x = ted->te_txtlen - 1; x > ei->pos; x--)
				txt[x] = txt[x - 1];
			ted->te_ptext[ei->pos++] = (char)key;
			update = true;
			break;
		}

		if (ei->pos < n)
			n = ei->pos;

		tmask = character_type[key];
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
				for (n = ei->pos; n < x; n++)
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
	return update;
}

#if GENERATE_DIAGS
static const char *const edfunc[] =
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
	    !object_is_editable( obtree + obj, OF_EDITABLE, 0 ) )
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
#define INGORE_EDITABLE	4
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
		(unsigned long)ted, old_ed_obj.item));

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
	    const GRECT *clip,
	    struct xa_rect_list *rl)
{
	(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);

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
		(*wt->objcr_api->eor_cursor)(wt, v, rl);
		showm();
	}
	wt->e.c_state ^= OB_CURS_EOR;
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
	 const GRECT *clip,
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
	const char *funcstr = func < 0 || func > 3 ? edfunc[4] : edfunc[func];
	DIAG((D_form,wt->owner,"  --  obj_edit: func %s, wt=%lx obtree=%lx, obj:%d, k:%x, pos:%x",
	      funcstr, (unsigned long)wt, (unsigned long)obtree, aesobj_item(&obj), keycode, pos));
#endif

	if (valid_aesobj(&obj))
		ted = object_get_tedinfo(aesobj_ob(&obj), &xted);

	if (!xted)
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
				{
					(*wt->objcr_api->eor_cursor)(wt, v, rl);
				}
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
								(*wt->objcr_api->eor_cursor)(wt, v, rl);
								if (obj_ed_char(&wt->e, ted, NULL, keycode))
									obj_draw(wt, v, editfocus(&wt->e), -1, clip, rl, 0);
								(*wt->objcr_api->eor_cursor)(wt, v, rl);
							}
							else
								obj_ed_char(&wt->e, ted, NULL, keycode);

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

						DIAGS((" -- obj_edit: ted=%lx", (unsigned long)ted));

						if (redraw)
						{
							(*wt->objcr_api->eor_cursor)(wt, v, rl);
							if (obj_ed_char(ei, ted, NULL, keycode))
								obj_draw(wt, v, obj, -1, clip, rl, 0);
							(*wt->objcr_api->eor_cursor)(wt, v, rl);
						}
						else
							obj_ed_char(ei, ted, NULL, keycode);

						pos = ei->pos;
					}
					showm();
				}
				break;
			}
			case ED_STRING:
			{
				if (string)
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
								(*wt->objcr_api->eor_cursor)(wt, v, rl);
								obj_ed_char(&wt->e, ted, NULL, 0x011b);
								while (*string)
									obj_ed_char(&wt->e, ted, NULL, *string++);

								obj_draw(wt, v, editfocus(&wt->e), -1, clip, rl, 0);
								(*wt->objcr_api->eor_cursor)(wt, v, rl);
							}
							else
							{
								obj_ed_char(&wt->e, ted, NULL, 0x011b);
								while (*string)
									obj_ed_char(&wt->e, ted, NULL, *string++);
							}
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

						DIAGS((" -- obj_edit: ted=%lx", (unsigned long)ted));

						if (redraw)
						{
							(*wt->objcr_api->eor_cursor)(wt, v, rl);
							obj_ed_char(ei, ted, NULL, SC_ESC);
							while (*string)
								obj_ed_char(ei, ted, NULL, *string++);

							obj_draw(wt, v, obj, -1, clip, rl, 0);
							(*wt->objcr_api->eor_cursor)(wt, v, rl);
						}
						else
						{
							obj_ed_char(ei, ted, NULL, 0x011b);
							while (*string)
								obj_ed_char(ei, ted, NULL, *string++);
						}

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
					(*wt->objcr_api->eor_cursor)(wt, v, rl);
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
				{
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
					/* obj.item, xted->o.item differ ??? */
				}

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
						obj = ob_find_next_any_flagstate(wt, aesobj(obtree, 0), inv_aesobj(), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_FIRST);
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
						(*wt->objcr_api->set_cursor)(wt, v, ei);
						if (redraw)
						{
							obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);
						}
						(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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
					(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);
				break;
			}
			case ED_CRSRON:
			{
				if ((ei = wt->ei))
					(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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
				{
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				}

				if (!ei || ei != xted)
				{
					DIAGS((" -- obj_edit: on object=%d without cursor focus", obj.item));

					if ((ei = xted))
					{
						obj_xED_INIT(wt, ei, pos, CLRMARKS);
						(*wt->objcr_api->set_cursor)(wt, v, ei);
					}
				}
				else
					drwcurs = true;

				if (ei)
				{

					DIAGS((" -- obj_edit: ted=%lx", (long)&xted->ti));

					if (ei != wt->ei)

					hidem();
					if (drwcurs)
						(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);
					if (obj_ed_char(ei, NULL, xted, keycode))
					{
						if (redraw)
						{
							obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);
						}
					}
					(*wt->objcr_api->set_cursor)(wt, v, ei);

					if (drwcurs)
					{
						(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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
				{
					ted = aesobj_get_tedinfo(&obj, &xted);
				}

				if (!ei || ei != xted)
				{
					if ((ei = xted))
						obj_xED_INIT(wt, ei, -1, CLRMARKS);
				}
				else
					drwcurs = true;

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
						(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);
					if (redraw)
						obj_draw(wt, v, obj, -1, clip, rl, 0);
					if (drwcurs)
						(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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
				{
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				}

				if (!ei || ei != xted)
				{
					if ((ei = xted))
					{
						obj_xED_INIT(wt, ei, pos, CLRMARKS);
						(*wt->objcr_api->set_cursor)(wt, v, ei);
					}
				}
				else
					drwcurs = true;

				DIAGS((" -- obj_edit: ted=%lx", (unsigned long)ted));
				if (ei)
				{
					hidem();
					if (drwcurs)
						(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);

					if (string && string != ei->ti.te_ptext && *string)
					{
						while (*string)
							obj_ed_char(ei, NULL, xted, *(unsigned char *)string++);
						if ((keycode & 1) && wt->ei == ei)
							obj_xED_INIT(wt, ei, -1, SETMARKS);
					}
					else
					{
						if (!string || !*string)
							ei->ti.te_ptext[0] = '\0';
						obj_xED_INIT(wt, ei, -1, ((keycode & 1) && wt->ei == ei) ? SETMARKS : CLRMARKS);
					}

					(*wt->objcr_api->set_cursor)(wt, v, ei);
					if (redraw)
						obj_draw(wt, v, editfocus(ei), -1, clip, rl, 0);

					if (drwcurs)
					{
						(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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
				{
					ted = object_get_tedinfo(aesobj_ob(&obj), &xted);
				}

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
						(*wt->objcr_api->set_cursor)(wt, v, ei);

						if (redraw)
						{
							if (drwcurs)
								(*wt->objcr_api->undraw_cursor)(wt, v, rl, redraw);
							obj_draw(wt, v, obj, -1, clip, rl, 0);
							if (drwcurs)
								(*wt->objcr_api->draw_cursor)(wt, v, rl, redraw);
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

void _cdecl
obj_set_radio_button(XA_TREE *wt,
		      struct xa_vdi_settings *v,
		      struct xa_aes_object obj,
		      bool redraw,
		      const GRECT *clip,
		      struct xa_rect_list *rl)
{
	OBJECT *obtree = wt->tree;
	struct xa_aes_object parent, o;

	parent = ob_get_parent(obtree, obj);

	DIAG((D_objc, NULL, "obj_set_radio_button: wt=%lx, obtree=%lx, obj=%d, parent=%d",
		(unsigned long)wt, (unsigned long)obtree, obj.item, parent.item));

	if (valid_aesobj(&parent))
	{
		o = aesobj(aesobj_tree(&parent), aesobj_head(&parent));

		while (valid_aesobj(&o) && !same_aesobj(&o, &parent))
		{
			if ( (aesobj_flags(&o) & OF_RBUTTON) && aesobj_sel(&o) )
			{
				DIAGS(("radio: change obj %d", o.item));
				if (!same_aesobj(&o, &obj))
				{
					obj_change(wt,
						   v,
						   o, -1,
						   aesobj_state(&o) & ~OS_SELECTED,
						   aesobj_flags(&o),
						   redraw,
						   clip,
						   rl, 0);
				}
			}
			o = aesobj(aesobj_tree(&o), aesobj_next(&o));
		}
		DIAGS(("radio: set obj %d", obj.item));
		obj_change(wt,
			   v,
			   obj, -1,
			   aesobj_state(&obj) | OS_SELECTED,
			   aesobj_flags(&obj),
			   redraw, clip,
			   rl, 0);
	}
}

struct xa_aes_object _cdecl
obj_get_radio_button(XA_TREE *wt,
		      struct xa_aes_object parent,
		      short state)
{
	struct xa_aes_object o = parent;

	DIAG((D_objc, NULL, "obj_set_radio_button: wt=%lx, obtree=%lx, parent=%d",
		(unsigned long)wt, (unsigned long)aesobj_tree(&parent), aesobj_item(&parent)));

	if (valid_aesobj(&parent))
	{
		o = aesobj(aesobj_tree(&parent), aesobj_head(&parent));

		while (!same_aesobj(&o, &parent))
		{
			if ( (aesobj_flags(&o) & OF_RBUTTON) && (aesobj_state(&o) & state))
				break;
			o = aesobj(aesobj_tree(&o), aesobj_next(&o));
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
	   const GRECT *clip,
	   struct xa_rect_list *rl)
{
	struct xa_aes_object obf = obj, pobf = aesobj(wt->tree, -2);
	short mx, my, mb, x, y, omx, omy;
	short flags = aesobj_flags(&obj);

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

/*
 * copy a string to clipboard (public)
 * return 0 on success
 */
int copy_string_to_clipbrd( char *text )
{
	int len = 0;
	if( text )
		len = strlen(text);
	if (len)
	{
		char scrp[256];
		struct file *f;

		f = kernel_open(ed_scrap_filename(scrp, sizeof(scrp)),
				O_WRONLY|O_CREAT|O_TRUNC, NULL, NULL);
		if (f)
		{
			kernel_write(f, text, len);
			kernel_close(f);
			return 0;
		}
		return 1;
	}
	return 2;
}

