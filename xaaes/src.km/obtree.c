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
#include "rectlist.h"
#include "xa_global.h"
#include "k_mouse.h"

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

//is_spec(OBJECT *tree, int item)
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

/*
 * HR change ob_spec
 */
//set_ob_spec(OBJECT *root, int s_ob, unsigned long cl)
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

inline void
object_deselect(OBJECT *ob)
{
	ob->ob_state &= ~OS_SELECTED;
}

bool
object_is_transparent(OBJECT *ob)
{
	switch (ob->ob_type & 0xff)
	{
	case G_STRING:
	case G_SHORTCUT:
	case G_TITLE:
	case G_IBOX:
#if 0
	case G_TEXT:		need more evaluation, not urgent
	case G_FTEXT:
#endif
		/* transparent */
		return true;
	}

	/* opaque */
	return false;
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

/*
 * Returns the object number of this object's parent or -1 if it is the root
 */
//get_parent(OBJECT *t, int object)
short
ob_get_parent(OBJECT *obtree, short obj)
{
	if (obj)
	{
		short last;

		do {
			last = obj;
			obj = obtree[obj].ob_next;
		}
		while(obtree[obj].ob_tail != last);

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

/* A quick hack to catch *most* of the problems with transparent objects */
//transparent(OBJECT *root, int i)
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
		if (ob->ob_state & OS_WHITEBAK)
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
//object_offset(OBJECT *tree, int object, short dx, short dy, short *mx, short *my)
short
ob_offset(OBJECT *obtree, short object, short *mx, short *my)
{
	short next;
	short current = 0;
	short x = 0, y = 0;
	
	do
	{
		/* Found the object in the tree? cool, return the coords */
		if (current == object)
		{
			x += obtree[current].ob_x;
			y += obtree[current].ob_y;
			
			*mx = x; // + obtree[current].ob_x;
			*my = y; // + obtree[current].ob_y;

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

//object_rectangle(RECT *c, OBJECT *ob, int i, short transx, short transy)
void
ob_rectangle(OBJECT *obtree, short obj, RECT *c)
{
	OBJECT *b = obtree + obj;

	ob_offset(obtree, obj, &c->x, &c->y);
	c->w = b->ob_width;
	c->h = b->ob_height;
}

//object_area(RECT *c, OBJECT *ob, int i, short transx,  short transy)
void
ob_area(OBJECT *obtree, short obj, RECT *c)
{
	OBJECT *b = obtree + obj;
	short dx = 0, dy = 0, dw = 0, dh = 0, db = 0;
	short thick = object_thickness(b);   /* type dependent */

	ob_rectangle(obtree, obj, c);

	if (thick < 0)
		db = thick;

	/* HR 0080801: oef oef oef, if foreground any thickness has the 3d enlargement!! */
	if (d3_foreground(b)) 
		db -= 2;

	dx = db;
	dy = db;
	dw = 2 * db;
	dh = 2 * db;

	if (b->ob_state & OS_OUTLINED)
	{
		dx = min(dx, -3);
		dy = min(dy, -3);
		dw = min(dw, -6);
		dh = min(dh, -6);
	}

	/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
	if (thick < 0 && b->ob_state & OS_SHADOWED)
	{
		dw += 2 * thick;
		dh += 2 * thick;
	}

	c->x += dx;
	c->y += dy;
	c->w -= dw;
	c->h -= dh;

}

/*
 * Find which object is at a given location
 *
 */
//find_object(OBJECT *tree, int object, int depth, short mx, short my, short dx, short dy)
short
ob_find(OBJECT *obtree, short object, short depth, short mx, short my)
{
	short next;
	short current = 0, rel_depth = 1;
	short x = 0, y = 0;
	bool start_checking = false;
	short pos_object = -1;

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

	DIAG((D_objc, NULL, "obj_find: found %d", pos_object));

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
		m = (tree[title].ob_type&0xff) == G_TITLE;

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
		obj_draw(wt, obj, rl);
	}
}

void
obj_draw(XA_TREE *wt, short obj, struct xa_rect_list *rl)
{
	RECT or;

	hidem();

	ob_area(wt->tree, obj, &or);

	if (rl)
	{
		RECT r;
		do
		{
			if (xa_rect_clip(&rl->r, &or, &r))
			{
				set_clip(&r);
				draw_object_tree(0, wt, wt->tree, 0, MAX_DEPTH, 1);
			}
		} while ((rl = rl->next));
	}
	else
	{
		set_clip(&or);
		draw_object_tree(0, wt, wt->tree, 0, MAX_DEPTH, 1);
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
				O_WRONLY|O_CREAT|O_TRUNC, NULL);
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

	f = kernel_open(ed_scrap_filename(scrp, sizeof(scrp)), O_RDONLY, NULL);
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

//ed_char(XA_TREE *wt, TEDINFO *ed_txt, ushort keycode)
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
				ei->pos = cursor_pos;	//wt->e.pos = cursor_pos;
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
	ei->pos = cursor_pos; //wt->e.pos = cursor_pos;
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
	if ( !chk_edobj(obtree, obj, last) || !(ted = object_get_tedinfo(obtree + obj)) )
	{
		old_ed_obj = ei->obj; //wt->e.obj;
		ei->obj = -1; //wt->e.obj = -1;
		ret = 0;
	}
	else
	{
		/* Ozk: 
		 * do things here to end edit of current obj...
		 */
		/* --- */
		old_ed_obj = ei->obj; //wt->e.obj;
		/* Ozk:
		 * Init the object.
		 * If new posititon == -1, we place the cursor at the very end
		 * of the text to edit.
		 */
		ei->obj = obj; //wt->e.obj = obj;
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;

		p = strlen(ted->te_ptext);
		if (pos && (pos == -1 || pos > p))
			pos = p;
		ei->pos = pos; //wt->e.pos = pos;
		DIAGS(("ED_INIT: te_ptext='%s'", ted->te_ptext));
		ret = 1;
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

	switch (func)
	{
		case ED_INIT:
		{
			ei = &wt->e;
			hidem();
			if (ei->obj >= 0)
				undraw_objcursor(wt);

			if (!obj_ED_INIT(wt, ei, obj, -1, last, NULL, &old_ed_obj))
				disable_objcursor(wt);
			else
			{
				enable_objcursor(wt);
				draw_objcursor(wt);
			}
			showm();
			pos = ei->pos;
			break;
		}
		case ED_END:
		{
			/* Ozk: Just turn off cursor :)
			 */
			hidem();
			disable_objcursor(wt);
			showm();
			pos = wt->e.pos;
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
						obj_draw(wt, obj, rl);

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

				undraw_objcursor(wt);
				if (obj_ed_char(wt, ei, ted, keycode))
					obj_draw(wt, obj, rl);
				set_objcursor(wt);
				draw_objcursor(wt);
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
			return 0;
			break;
		}
		default:
		{
			return 0;
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
			   redraw,
			   rl);
	}
}

short
obj_watch(XA_TREE *wt,
	   short obj,
	   short in_state,
	   short out_state,
	   struct xa_rect_list *rl)
{
	OBJECT *focus = wt->tree + obj;
	short pobf = -2, obf = obj;
	short mx, my, mb, x, y, omx, omy;
	short flags = focus->ob_flags;

	ob_offset(wt->tree, obj, &x, &y);

	x--;
	y--;

	check_mouse(wt->owner, &mb, &omx, &omy);

	if (!mb)
	{
		/* If mouse button is already released, assume that was just
		 * a click, so select
		 */
		obj_change(wt, obj, in_state, flags, true, rl);
	}
	else
	{
		S.wm_count++;
		while (mb)
		{
			short s;

			wait_mouse(wt->owner, &mb, &mx, &my);

			if ((mx != omx) || (my != omy))
			{
				omx = mx;
				omy = my;
				obf = ob_find(wt->tree, obj, 10, mx, my);

				if (obf == obj)
					s = in_state; //(dial + ob)->ob_state = in_state;
				else
					s = out_state; //(dial + ob)->ob_state = out_state;

				if (pobf != obf)
				{
					pobf = obf;
					obj_change(wt, obj, s, flags, true, rl);
				}
			}
		}
		S.wm_count--;
	}

	//f_interior(FIS_SOLID);
	//wr_mode(MD_TRANS);

	if (obf == obj)
		return 1;
	else
		return 0;
}

/* ************************************************************ */
/*      OLD STUFF   */
/* ************************************************************ */
#if 0

/* HR: I wonder which one is faster;
		This one is smaller. and easier to follow. */
/* sheduled for redundancy */


void
redraw_object(enum locks lock, XA_TREE *wt, int item)
{
	hidem();
	draw_object_tree(lock, wt, NULL, item, MAX_DEPTH, 2);
	showm();
}

void
change_object(enum locks lock, XA_TREE *wt, OBJECT *root, int i, const RECT *r, int state,
              bool draw)
{
	int start  = i;

	root[start].ob_state = state;

	if (draw)
	{
		RECT c; int q;

		while (transparent(root, start))
			if ((q = get_parent(root, start)) < 0)
				break;
			else
				start = q;

		hidem();
		object_area(&c,root,i,wt ? wt->dx : 0, wt ? wt->dy : 0);
		if (!r || (r && xa_rc_intersect(*r, &c)))
		{
			set_clip(&c);
			draw_object_tree(lock, wt, root, start, MAX_DEPTH, 1);
			clear_clip();
		}
		showm();
	}
}


/* HR 271101: separate function, also used by wdlg_xxx extension. */
int
edit_object(enum locks lock,
	    struct xa_client *client,
	    int func,
	    XA_TREE *wt,
	    OBJECT *form,
	    int ed_obj,
	    int keycode,
	    short *newpos)
{
	TEDINFO *ted;
	OBJECT *otree = wt->tree;
	int  last = 0, old_edit_obj = -1;
	bool update = false;

#if GENERATE_DIAGS
	char *funcstr = func < 0 || func > 3 ? edfunc[4] : edfunc[func];
	DIAG((D_form,wt->owner,"  --  edit_object: wt=%lx form=%lx, obj:%d, k:%x, m:%s", wt, form, ed_obj, keycode, funcstr));
#endif

	/* 
	 * go through the objects until the LASTOB
	 */
	do
	{
		/*
		 * exit if ed_obj is not editable
		 */
		if (last == ed_obj && (form[last].ob_flags & OF_EDITABLE) == 0)
			return 0;
	} while (!(form[last++].ob_flags & OF_LASTOB));

	/* the ed_obj is past the LASTOB (last is past the one -> decrement) */
	if (ed_obj >= --last)
		return 0;

	if (otree == form && wt->e.obj != ed_obj)
	{
		old_edit_obj = wt->e.obj;
		update = true;
	}

	/* set the object to edit to the widget structure */
	wt->e.obj = ed_obj;

	ted = get_ob_spec(&form[ed_obj])->tedinfo;

	switch(func)
	{
	/* set current edit field */
	case ED_INIT:
	{
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;
		wt->e.pos = strlen(ted->te_ptext);
		update = true;
		break;
	}
	/* process a character */
	case ED_CHAR:
	{
		update = update || ed_char(wt, ted, keycode);
		break;
	}
	/* turn off the cursor */
	case ED_END:
	{
		wt->e.obj = -1;
		update = true;
		break;
	}
	/* ED_INIT with the edit_position setup for the x coordinate value
	   (comming in keycode) */
	case ED_CRSR:
	{
		/* TODO: x coordinate -> cursor position conversion */

		/* TODO: REMOVE: begin ... ED_INIT like position return */
		if (*(ted->te_ptext) == '@')
			wt->e.pos = 0;
		else
			wt->e.pos = strlen(ted->te_ptext);
		/* TODO: REMOVE: end */
		break;
	}
	default:
	{
		return 1;
	}
	}

	/* update the cursor position */
	if (newpos)
		*newpos = wt->e.pos;

	if (update)
	{
		DIAGS(("newpos wt %lx, old_ed_obj %d, ed_obj %d ed_pos %d", wt, old_edit_obj, ed_obj, wt->e.pos));
		if (old_edit_obj != -1)
			redraw_object(lock, wt, old_edit_obj);
		redraw_object(lock, wt, ed_obj);
	}

	return 1;
}
/* HR: Only the '\0' in the te_ptext field is determining the corsor position, NOTHING ELSE!!!!
		te_tmplen is a constant and strictly owned by the APP.
*/
#endif
