/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
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

#include "xa_objc.h"
#include "xa_global.h"

#include "rectlist.h"
#include "objects.h"
#include "menuwidg.h"
#include "widgets.h"
#include "xa_form.h"

#include "mint/fcntl.h"


/*
 * OBJECT TREE ROUTINES
 * - New version of the object display routine modularises the whole system.
 */

static bool ed_scrap_copy(XA_TREE *wt, TEDINFO *ed_txt);
static bool ed_scrap_cut(XA_TREE *wt, TEDINFO *ed_txt);
static bool ed_scrap_paste(XA_TREE *wt, TEDINFO *ed_txt, int *cursor_pos);

/*
 * HR change ob_spec
 */
void
set_ob_spec(OBJECT *root, int s_ob, unsigned long cl)
{
	if (root[s_ob].ob_flags & OF_INDIRECT)
		root[s_ob].ob_spec.indirect->index = cl;
	else
		root[s_ob].ob_spec.index = cl;
}

/*
 * Set clipping to entire screen
 */
void
clear_clip(void)
{
	rtopxy(C.global_clip, &screen.r);
	vs_clip(C.vh, 1, C.global_clip);
}

void
set_clip(const RECT *r)
{
	if (r->w <= 0 || r->h <= 0)
		rtopxy(C.global_clip,&screen.r);
	else	
		rtopxy(C.global_clip,r);

	vs_clip(C.vh, 1, C.global_clip);
}

/* HR: I wonder which one is faster;
		This one is smaller. and easier to follow. */
void
g2d_box(int b, RECT *r, int colour)
{
	/* inside runs from 3 to 0 */
	if (b > 0)
	{
		if (b >  4) b =  3;
		else        b--;
		l_color(colour);
		while (b >= 0)
			gbox(-b, r), b--;
	}
	/* outside runs from 4 to 1 */
	else if (b < 0)
	{
		if (b < -4) b = -4;
		l_color(colour);
		while (b < 0)
			gbox(-b, r), b++;
	}
}

/* sheduled for redundancy */
void
draw_2d_box(short x, short y, short w, short h, int border_thick, int colour)
{
	RECT r;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	g2d_box(border_thick, &r, colour);
}

void
write_disable(RECT *r, int colour)
{
	static short pattern[16] =
	{
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa,
		0x5555, 0xaaaa, 0x5555, 0xaaaa
	};

	wr_mode(MD_TRANS);
	f_color(colour);
	vsf_udpat(C.vh, pattern, 1);
	f_interior(FIS_USER);
	gbar(0, r);
}

bool
is_menu(OBJECT *tree)
{
	bool m = false;
	int title;

	title = tree[0].ob_head;
	if (title > 0)
		title = tree[title].ob_head;
	if (title > 0)
		title = tree[title].ob_head;
	if (title > 0)
		m = (tree[title].ob_type&0xff) == G_TITLE;

	return m;
}

#if GENERATE_DIAGS

static char *pstates[] =
{
	"SEL",
	"CROSS",
	"\10",
	"DIS",
	"OUTL",
	"SHA",
	"WBAK",
	"D3D",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15"
};

static char *pflags[] =
{
	"S",
	"DEF",
	"EXIT",
	"ED",
	"RBUT",
	"LAST",
	"TOUCH",
	"HID",
	">>",
	"INDCT",
	"BACKGR",
	"SUBM",
	"12",
	"13",
	"14",
	"15"
};

static char *ob_types[] =
{
	"box",
	"text",
	"boxtext",
	"image",
	"progdef",
	"ibox",
	"button",
	"boxchar",
	"string",
	"ftext",
	"fboxtext",
	"icon",
	"title",
	"cicon",
	"xaaes slist",
	"popup",
	"resv",
	"edit",
	"shortcut",
	"39",
	"40"
};

char *
object_type(OBJECT *tree, int t)
{
	static char other[80];

	unsigned int ty = tree[t].ob_type,tx;

	if (ty >= G_BOX && ty < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		return ob_types[ty-G_BOX];

	tx = ty & 0xff;
	if (tx >= G_BOX && tx < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		sprintf(other, sizeof(other), "ext: 0x%x + %s", ty >> 8, ob_types[tx-G_BOX]);
	else
		sprintf(other, sizeof(other), "unknown: 0x%x,%d", ty, ty);

	return other;
}

static char *
object_txt(OBJECT *tree, int t)			/* HR: I want to know the culprit in a glance */
{
    	static char nother[160];
	int ty = tree[t].ob_type;

	*nother = 0;

	switch (ty & 0xff)
	{
		case G_FTEXT:
		case G_TEXT:
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			TEDINFO *ted = get_ob_spec(tree + t)->tedinfo;
			sprintf(nother, sizeof(nother), " '%s'", ted->te_ptext);
			break;
		}
		case G_BUTTON:
		case G_TITLE:
		case G_STRING:
		case G_SHORTCUT:
			sprintf(nother, sizeof(nother), " '%s'", get_ob_spec(tree + t)->free_string);
			break;
	}	
	return nother;
}
#endif

/*
 * Walk an object tree, calling display for each object
 * HR: is_menu is true if a menu.
 */

XA_TREE nil_tree = { 0 };

/*
 * Initialise the object display jump table
 */

static ObjectDisplay *objc_jump_table[G_UNKNOWN];

void
init_objects(void)
{
	int f;

	for (f = 0; f <= G_UNKNOWN; f++)
		/* Anything with a NULL pointer won't get called */
		objc_jump_table[f] = NULL;

	objc_jump_table[G_BOX     ] = d_g_box;
	objc_jump_table[G_TEXT    ] = d_g_text;
	objc_jump_table[G_BOXTEXT ] = d_g_boxtext;
	objc_jump_table[G_IMAGE   ] = d_g_image;
	objc_jump_table[G_PROGDEF ] = d_g_progdef;
	objc_jump_table[G_IBOX    ] = d_g_ibox;
	objc_jump_table[G_BUTTON  ] = d_g_button;
	objc_jump_table[G_BOXCHAR ] = d_g_boxchar;
	objc_jump_table[G_STRING  ] = d_g_string;
	objc_jump_table[G_FTEXT   ] = d_g_ftext;
	objc_jump_table[G_FBOXTEXT] = d_g_fboxtext;
	objc_jump_table[G_ICON    ] = d_g_icon;
	objc_jump_table[G_TITLE   ] = d_g_title;
	objc_jump_table[G_CICON   ] = d_g_cicon;
	objc_jump_table[G_SLIST   ] = d_g_slist;
	objc_jump_table[G_SHORTCUT] = d_g_string;
}

/*
 * Display a primitive object
 */
void
display_object(enum locks lock, XA_TREE *wt, int item, short parent_x, short parent_y, int which)
{
	RECT r;
	OBJECT *ob = wt->tree + item;
	ObjectDisplay *display_routine = NULL;

	/* HR: state_mask is for G_PROGDEF originally.
	 * But it means that other objects must unflag what they
	 * can do themselves in the same manner.
	 * The best thing (no confusion) is to generalize the concept.
	 * Which I did. :-)
	 */
	unsigned short state_mask = (OS_SELECTED|OS_CROSSED|OS_CHECKED|OS_DISABLED|OS_OUTLINED);
	unsigned int t = ob->ob_type & 0xff;

	r.x = parent_x + ob->ob_x;
	r.y = parent_y + ob->ob_y;
	r.w = ob->ob_width; 
	r.h = ob->ob_height;

	if (   r.x       > C.global_clip[2]	/* x + w */
	    || r.x+r.w-1 < C.global_clip[0]	/* x     */
	    || r.y       > C.global_clip[3]	/* y + h */
	    || r.y+r.h-1 < C.global_clip[1])	/* y     */
		return;

	if (t <= G_UNKNOWN)
		/* Get display routine for this type of object from jump table */
		display_routine = objc_jump_table[t];

	if (display_routine == NULL)
	{
		DIAG((D_objc,wt->owner,"no display_routine! ob_type: %d(0x%x)", t, ob->ob_type));
		/* dont attempt doing what could be indeterminate!!! */
		return;
	}

	/* Fill in the object display parameter structure */			
	wt->current = item;
	wt->parent_x = parent_x;
	wt->parent_y = parent_y;
	/* absolute RECT, ready for use everywhere. */
	wt->r = r;
	wt->state_mask = &state_mask;

	/* Better do this before AND after (fail safe) */
	wr_mode(MD_TRANS);

#if GENERATE_DIAGS
	if (wt->tree != get_widgets())
	{
		char flagstr[128];
		char statestr[128];

		show_bits(ob->ob_flags, "flg=", pflags, flagstr);
		show_bits(ob->ob_state, "st=", pstates, statestr);

		DIAG((D_o, wt->owner, "ob=%d, %d/%d,%d/%d [%d: 0x%lx]; %s%s %s %s",
			 item,
			 r.x, r.y, r.w, r.h,
			 t, display_routine,
			 object_type(wt->tree, item),
			 object_txt(wt->tree, item),
			 flagstr,
			 statestr));
	}
#endif

	/* Call the appropriate display routine */
	(*display_routine)(lock, wt);

	wr_mode(MD_TRANS);

	/* Handle CHECKED object state: */
	if ((ob->ob_state & state_mask) & OS_CHECKED)
	{
		t_color(G_BLACK);
		/* ASCII 8 = checkmark */
		v_gtext(C.vh, r.x + 2, r.y, "\10");
	}

	/* Handle DISABLED state: */
	if ((ob->ob_state & state_mask) & OS_DISABLED)
		write_disable(&r, G_WHITE);

	/* Handle CROSSED object state: */
	if ((ob->ob_state & state_mask) & OS_CROSSED)
	{
		short p[4];
		l_color(G_BLACK);
		p[0] = r.x;
		p[1] = r.y;
		p[2] = r.x + r.w - 1;
		p[3] = r.y + r.h - 1;
		v_pline(C.vh, 2, p);
		p[0] = r.x + r.w - 1;
		p[2] = r.x;
		v_pline(C.vh, 2, p);
	}

	/* Handle OUTLINED object state: */
	if ((ob->ob_state & state_mask) & OS_OUTLINED)
	{
		/* special handling of root object. */
		if (!wt->zen || item != 0)
		{
			if (!MONO && d3_any(ob))
			{
				tl_hook(1, &r, screen.dial_colours.lit_col);
				br_hook(1, &r, screen.dial_colours.shadow_col);
				tl_hook(2, &r, screen.dial_colours.lit_col);
				br_hook(2, &r, screen.dial_colours.shadow_col);
				gbox(3, &r);
			}
			else
			{
				l_color(G_WHITE);
				gbox(1, &r);
				gbox(2, &r);
				l_color(G_BLACK);
				gbox(3, &r);
			}
		}
	}

	if ((ob->ob_state & state_mask) & OS_SELECTED)
		write_selection(0, &r);

	wr_mode(MD_TRANS);
}

int
draw_object_tree(enum locks lock, XA_TREE *wt, OBJECT *tree, int item, int depth, int which)
{
	XA_TREE this;
	int next;
	int current = 0, rel_depth = 1, head;
	short x, y;
	bool start_drawing = false;

	IFDIAG(short *cl = C.global_clip;)

	if (wt == NULL)
	{
		this = nil_tree;

		wt = &this;
		wt->edit_obj = -1;
		wt->owner = C.Aes;
	}

	if (tree == NULL)
		tree = wt->tree;
	else
		wt->tree = tree;

	if (!wt->owner)
		wt->owner = C.Aes;

	/* dx,dy are provided by sliders if present. */
	x = -wt->dx;
	y = -wt->dy;
	DIAG((D_objc, wt->owner, "dx = %d, dy = %d", x, y));
	DIAG((D_objc, wt->owner, "[%d]draw_object_tree for %s to %d/%d,%d/%d; %lx + %d depth:%d",
		which, t_owner(wt), x + tree->ob_x, y + tree->ob_y,
		tree->ob_width, tree->ob_height, tree, item, depth));
	DIAG((D_objc, wt->owner, "  -   (%d)%s%s",
		wt->is_menu, is_menu(tree) ? "menu" : "object", wt->zen ? " with zen" : ""));
	DIAG((D_objc, wt->owner, "  -   clip: %d.%d/%d.%d    %d/%d,%d/%d",
		cl[0], cl[1], cl[2], cl[3], cl[0], cl[1], cl[2] - cl[0] + 1, cl[3] - cl[1] + 1));

	depth++;

	do {
		if (current == item)
		{
			start_drawing = true;
			rel_depth = 0;
		}

		if (start_drawing && !(tree[current].ob_flags & OF_HIDETREE))
		{
			/* Display this object */
			display_object(lock, wt, current, x, y, 10);
		}

		head = tree[current].ob_head;

		/* Any non-hidden children? */
		if (    head != -1
		    && (tree[current].ob_flags & OF_HIDETREE) == 0
		    && (start_drawing == 0
			|| (start_drawing != 0
			    && rel_depth < depth)))
		{
			x += tree[current].ob_x;
			y += tree[current].ob_y;

			rel_depth++;

			current = head;
		}
		else
		{
			/* Try for a sibling */
			next = tree[current].ob_next;

			/* Trace back up tree if no more siblings */
			while (next != -1 && tree[next].ob_tail == current)
			{
				current = next;
				x -= tree[current].ob_x;
				y -= tree[current].ob_y;
				next = tree[current].ob_next;
				rel_depth--;
			}
			current = next;
		}
	}
	while (current != -1 && !(start_drawing && rel_depth < 1));

	wr_mode(MD_TRANS);
	f_interior(FIS_SOLID);

	return true;
}

/*
 * Walk an object tree, calling display for each object
 * HR: is_menu is true if a menu.
 */

/* draw_object_tree */

/*
 * Get the true screen coords of an object
 */
int
object_offset(OBJECT *tree, int object, short dx, short dy, short *mx, short *my)
{
	int next;
	int current = 0;
	short x = -dx, y = -dy;
	
	do {
		/* Found the object in the tree? cool, return the coords */
		if (current == object)
		{
			*mx = x + tree[current].ob_x;
			*my = y + tree[current].ob_y;
			return 1;
		}

		/* Any children? */
		if (tree[current].ob_head != -1)
		{
			x += tree[current].ob_x;
			y += tree[current].ob_y;
			current = tree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = tree[current].ob_next;

			while ((next != -1) && (tree[next].ob_tail == current))
			{
				/* Trace back up tree if no more siblings */
				current = next;
				x -= tree[current].ob_x;
				y -= tree[current].ob_y;
				next = tree[current].ob_next;
			}
			current = next;
		}
	}
	while (current != -1); /* If 'current' is -1 then we have finished */

	/* Bummer - didn't find the object, so return error */
	return 0;
}

/*
 * Find which object is at a given location
 *
 */
int
find_object(OBJECT *tree, int object, int depth, short mx, short my, short dx, short dy)
{
	int next;
	int current = 0, rel_depth = 1;
	short x = -dx, y = -dy;
	bool start_checking = false;
	int pos_object = -1;

	do {
		if (current == object)	/* We can start considering objects at this point */
		{
			start_checking = true;
			rel_depth = 0;
		}
		
		if (start_checking)
		{
			if (  (tree[current].ob_flags & OF_HIDETREE) == 0
			    && tree[current].ob_x + x                     <= mx
			    && tree[current].ob_y + y                     <= my
			    && tree[current].ob_x + x + tree[current].ob_width  >= mx
			    && tree[current].ob_y + y + tree[current].ob_height >= my)
			{
				/* This is only a possible object, as it may have children on top of it. */
				pos_object = current;
			}
		}

		if (((!start_checking) || (rel_depth < depth))
		    && (tree[current].ob_head != -1)
		    && (tree[current].ob_flags & OF_HIDETREE) == 0)
		{
			/* Any children? */
			x += tree[current].ob_x;
			y += tree[current].ob_y;
			rel_depth++;
			current = tree[current].ob_head;
		}
		else
		{
			/* Try for a sibling */
			next = tree[current].ob_next;

			/* Trace back up tree if no more siblings */
			while ((next != -1) && (tree[next].ob_tail == current))
			{
				current = next;
				x -= tree[current].ob_x;
				y -= tree[current].ob_y;
				next = tree[current].ob_next;
				rel_depth--;
			}
			current = next;
		}	
	}
	while ((current != -1) && (rel_depth > 0));

	return pos_object;
}

XA_TREE *
check_widget_tree(enum locks lock, struct xa_client *client, OBJECT *tree)
{
	struct xa_window *wind = client->fmd.wind;
	XA_TREE *wt = &client->wt;

	DIAG((D_form, client, "check_widget_tree for %s: wh=%d obj:%lx, ed:%d, tree:%lx, %d/%d",
		c_owner(client), wind?wind->handle:-1,
		wt->tree, wt->edit_obj, tree, tree->ob_x, tree->ob_y));

	/* HR 220401: x & y governed by fmd.wind */
	if (wind)
	{
		XA_WIDGET *widg = get_widget(wind, XAW_TOOLBAR);
		XA_TREE *ct = widg->stuff;

		if (ct)
			wt = ct;

		DIAG((D_form, client, "check_widget_tree: fmd.wind: %d/%d, wt: %d/%d",
			client->fmd.wind->wa.x, client->fmd.wind->wa.y,
			wt->tree ? wt->tree->ob_x : -1, wt->tree ? wt->tree->ob_y : -1));

		if (!ct) {
			tree->ob_x = client->fmd.wind->wa.x,
			tree->ob_y = client->fmd.wind->wa.y;
		} /* else governed by widget.loc */

		wt->zen = true;
	}
	else if (tree != wt->tree)
	{
		bzero(wt, sizeof(XA_TREE));
		wt->edit_obj = -1;
	}

	wt->tree = tree;
	wt->owner = client;

	DIAG((D_form, client, "  --  zen: %d", wt->zen));
	return wt;
}

/*
 * Object Tree Handling Interface
 */
unsigned long
XA_objc_draw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	const RECT *r = (const RECT *)&pb->intin[2];
	OBJECT *tree = (OBJECT*)pb->addrin[0];
	int item = pb->intin[0];
	CONTROL(6,1,1)

	DIAG((D_objc,client,"objc_draw rectangle: %d/%d,%d/%d", r->x, r->y, r->w, r->h));

	if (tree)
	{
#if 0
		struct xa_window *wind = client->fmd.wind;

		if (wind)
			if (!wind->dial_followed)
		{
			if (item != 0)
			{
				f_color(screen.dial_colours.bg_col);
				gbar(0, &client->fmd.wind->wa);
			}
			wind->dial_followed = true;
		}
#endif
		hidem();
		set_clip(r);		/* HR 110601: checks for special case? w <= 0 or h <= 0 */
	
		pb->intout[0] = draw_object_tree(lock,
						 check_widget_tree(lock, client, tree),
						 tree,
						 item,
						 pb->intin[1],		/* depth */
						 0);
		clear_clip();
		showm();
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_offset(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *tree = (OBJECT*)pb->addrin[0];

	CONTROL(1,3,1)

	pb->intout[0] = object_offset(tree, 
				      pb->intin[0],
				      0,
				      0,
				      pb->intout + 1,
				      pb->intout + 2);

	return XAC_DONE;
}

unsigned long
XA_objc_find(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(4,1,1)

	pb->intout[0] = find_object((OBJECT*)pb->addrin[0],
				    pb->intin[0],
				    pb->intin[1],
				    pb->intin[2],
				    pb->intin[3],
				    0, 0);

	DIAG((D_o,client,"XA_objc_find %lx + %d, %d/%d --> %d",
		pb->addrin[0], pb->intin[0], pb->intin[2], pb->intin[3], pb->intout[0]));

	return XAC_DONE;
}

/* HR 120601: objc_change:
 * We go back thru the parents of the object until we meet a opaque object.
 *    This is to ensure that transparent objects are drawn correctly.
 * Care must be taken that outside borders or shadows are included in the draw.
 * Only the objects area is redrawn, so it must be intersected with clipping rectangle.
 * New function object_area(RECT *c, tree, item)
 *
 *	Now we can use this for the standard menu's and titles!!!
 */
int
thickness(OBJECT *ob)
{
	int t = 0, flags;
	TEDINFO *ted;

	switch(ob->ob_type & 0xff)
	{
	case G_BOX:
	case G_IBOX:
	case G_BOXCHAR:
		t = get_ob_spec(ob)->obspec.framesize;
		break;
	case G_BUTTON:
		flags = ob->ob_flags;
		t = -1;
		if (flags&OF_EXIT)
			t--;
		if (flags&OF_DEFAULT)
			t--;
		break;
	case G_BOXTEXT:
	case G_FBOXTEXT:
		ted = get_ob_spec(ob)->tedinfo;
		t = (signed char)ted->te_thickness;
	}

	return t;
}

static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

void
object_rectangle(RECT *c, OBJECT *ob, int i, short transx, short transy)
{
	OBJECT *b = ob + i;

	object_offset(ob, i, transx, transy, &c->x, &c->y);
	c->w = b->ob_width;
	c->h = b->ob_height;
}

void
object_area(RECT *c, OBJECT *ob, int i, short transx,  short transy)
{
	OBJECT *b = ob + i;
	short dx = 0, dy = 0, dw = 0, dh = 0, db = 0;
	int thick = thickness(b);   /* type dependent */

	object_rectangle(c, ob, i, transx, transy);

	if (thick < 0)
		db = thick;

	/* HR 0080801: oef oef oef, if foreground any thickness has the 3d enlargement!! */
	if (d3_foreground(b)) 
		db -= 2;

	dx = db;
	dy = db;
	dw = 2*db;
	dh = 2*db;

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
		dw += 2*thick;
		dh += 2*thick;
	}

	c->x += dx;
	c->y += dy;
	c->w -= dw;
	c->h -= dh;
# if 0
	DIAGS(("object_area: %s thick: %d, 3d: 0x%04x",
		object_type(ob,i), thick, b->ob_flags));
	DIAGS(("  --   c:%d/%d,%d/%d  d:%d/%d,%d/%d",
		c->x,c->y,c->w,c->h,dx,dy,dw,dh));
# endif
}

/* A quick hack to catch *most* of the problems with transparent objects */
static bool
transparent(OBJECT *root, int i)
{
	switch (root[i].ob_type & 0xff)
	{
	case G_STRING:
	case G_SHORTCUT:
	case G_TITLE:
	case G_IBOX:
# if 0
	case G_TEXT:		need more evaluation, not urgent
	case G_FTEXT:
# endif
		/* transparent */
		return true;
	}

	/* opaque */
	return false;
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

void
redraw_object(enum locks lock, XA_TREE *wt, int item)
{
	hidem();
	draw_object_tree(lock, wt, NULL, item, MAX_DEPTH, 2);
	showm();
}

unsigned long
XA_objc_change(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *tree = (OBJECT*)pb->addrin[0];

	CONTROL(8,1,1)

	if (tree)
	{
		change_object(lock,
			      check_widget_tree(lock, client, tree),
			      tree,
			      pb->intin[0],
			      (const RECT *)&pb->intin[2],
			      pb->intin[6],
			      pb->intin[7]);	
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

/* HR 020604: child must be inserted at the END of the list of children!!! */

unsigned long
XA_objc_add(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *root = (OBJECT *)pb->addrin[0];
	int parent, new_child, last_child;
	
	CONTROL(2,1,1)

	parent = pb->intin[0];
	new_child = pb->intin[1];

	if (new_child == parent || new_child == -1 || parent == -1)
		pb->intout[0] = 0;
	else
	{
		last_child = root[parent].ob_tail;
		root[new_child].ob_next = parent;
		root[parent   ].ob_tail = new_child;
		if (last_child == -1)	/* No siblings */
			root[parent    ].ob_head = new_child;
		else
			root[last_child].ob_next = new_child;

/* 
	JH: 04/15/2003 Incompatible with N.AES, TOS, etc... 
	regarding to the objc_add documentation ob_head, ob_tail
	has to be initialized by the application

		root[new_child].ob_head = -1;
		root[new_child].ob_tail = -1;
*/

		pb->intout[0] = 1;
	}

	return XAC_DONE;
}

static int
remove_object(OBJECT *root, int object)
{
	int parent, current, last;

	current = object;
	do {
		/* Find parent */
		last = current;
		current = root[current].ob_next;
	}
	while (root[current].ob_tail != last);
	parent = current;
	
	if (root[parent].ob_head == object)
	{
		/* First child */

		if (root[object].ob_next == parent)
			/* No siblings */
			root[parent].ob_head = root[parent].ob_tail = -1;
		else
			/* Siblings */
			root[parent].ob_head = root[object].ob_next;
	}
	else
	{
		/* Not first child */

		current = root[parent].ob_head;
		do {
			/* Find adjacent sibling */
			last = current;
			current = root[current].ob_next;
		}
		while (current != object);
		root[last].ob_next = root[object].ob_next;

		if (root[object].ob_next == parent)
			/* Last child removed */
			root[parent].ob_tail = last;
	}
	
	return parent;
}

unsigned long
XA_objc_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(1,1,1)

	remove_object((OBJECT *)pb->addrin[0], pb->intin[0]);

	pb->intout[0] = 1;

	return XAC_DONE;
}

unsigned long
XA_objc_order(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *root = (OBJECT *)pb->addrin[0];
	int parent, object, current;
	ushort pos;			/* -1 (top) is a large positive number */
	
	CONTROL(2,1,1)

	object = pb->intin[0];
	parent = remove_object(root, object);

	pos = pb->intin[1];	
	current = root[parent].ob_head;
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

	pb->intout[0] = 1;

	return XAC_DONE;
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

bool
ed_char(XA_TREE *wt, TEDINFO *ed_txt, ushort keycode)
{
	char *txt = ed_txt->te_ptext;
	int cursor_pos = wt->edit_pos, x, key, tmask, n, chg;
	bool update = false;

	DIAG((D_objc, NULL, "ed_char keycode=0x%04x", keycode));

	switch (keycode)
	{	
	case 0x011b:	/* ESCAPE clears the field */
		txt[0] = '\0';
		cursor_pos = 0;
		update = true;
		break;

	case 0x537f:	/* DEL deletes character under cursor */
		if (txt[cursor_pos])
		{
			for(x = cursor_pos; x < ed_txt->te_txtlen - 1; x++)
				txt[x] = txt[x + 1];
			
			update = true;
		}
		break;
			
	case 0x0e08:	/* BACKSPACE deletes character left of cursor (if any) */
		if (cursor_pos)
		{
			for(x = cursor_pos; x < ed_txt->te_txtlen; x++)
				txt[x - 1] = txt[x];

			cursor_pos--;
			update = true;
		}
		break;
				
	case 0x4d00:	/* RIGHT ARROW moves cursor right */
		if ((txt[cursor_pos]) && (cursor_pos < ed_txt->te_txtlen - 1))
		{
			cursor_pos++;
			update = true;
		}
		break;

	case 0x4d36:	/* SHIFT+RIGHT ARROW move cursor to far right of current text */
		for(x = 0; txt[x]; x++)
			;

		if (x != cursor_pos)
		{
			cursor_pos = x;
			update = true;
		}
		break;
			
	case 0x4b00:	/* LEFT ARROW moves cursor left */
		if (cursor_pos)
		{
			cursor_pos--;
			update = true;
		}
		break;
			
	case 0x4b34:	/* SHIFT+LEFT ARROW move cursor to start of field */
	case 0x4700:	/* CLR/HOME also moves to far left */
		if (cursor_pos)
		{
			cursor_pos = 0;
			update = true;
		}
		break;

	case 0x2e03:	/* CTRL+C */
		update = ed_scrap_copy(wt, ed_txt);
		break;
	case 0x2d18: 	/* CTRL+X */
		update = ed_scrap_cut(wt, ed_txt);
		break;
	case 0x2f16: 	/* CTRL+V */
		update = ed_scrap_paste(wt, ed_txt, &cursor_pos);
		break;

	default:	/* Just a plain key - insert character */
		chg = 0;/* Ugly hack! */
		if (cursor_pos == ed_txt->te_txtlen - 1)
		{
			cursor_pos--;
			chg = 1;
		}
				
		key = keycode & 0xff;
		tmask=character_type[key];

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
				wt->edit_pos = cursor_pos;
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
	wt->edit_pos = cursor_pos;
	return update;
}


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
ed_scrap_cut(XA_TREE *wt, TEDINFO *ed_txt)
{
	ed_scrap_copy(wt, ed_txt);

	/* clear the edit control */
	*ed_txt->te_ptext = '\0';
	wt->edit_pos = 0;
	return true;
}

static bool
ed_scrap_copy(XA_TREE *wt, TEDINFO *ed_txt)
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
ed_scrap_paste(XA_TREE *wt, TEDINFO *ed_txt, int *cursor_pos)
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
					ed_char(wt, ed_txt, (unsigned short)data[i]);

					if (wt->edit_pos >= ed_txt->te_txtlen - 1)
					{
						cnt = 0;
						break;
					}
				}
			}
		}

		kernel_close(f);

		/* move the cursor to the end of the pasted text */
		*cursor_pos = wt->edit_pos;
		return true;
	}

	return false;
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

	/* go through the objects until the LASTOB */
	do {
		/* exit if ed_obj is not editable */
		if (last == ed_obj && (form[last].ob_flags & OF_EDITABLE) == 0)
			return 0;
	} while (!(form[last++].ob_flags & OF_LASTOB));

	/* the ed_obj is past the LASTOB (last is past the one -> decrement) */
	if (ed_obj >= --last)
		return 0;

	wt = check_widget_tree(lock, client, form);
	if (otree == form && wt->edit_obj != ed_obj)
	{
		old_edit_obj = wt->edit_obj;
		update = true;
	}

	/* set the object to edit to the widget structure */
	wt->edit_obj = ed_obj;

	ted = get_ob_spec(&form[ed_obj])->tedinfo;

	switch(func)
	{
	/* set current edit field */
	case ED_INIT:
		if (*(ted->te_ptext) == '@')
			*(ted->te_ptext) = 0;
		wt->edit_pos = strlen(ted->te_ptext);
		update = true;
		break;

	/* process a character */
	case ED_CHAR:
		update = update || ed_char(wt, ted, keycode);
		break;

	/* turn off the cursor */
	case ED_END:
		wt->edit_obj = -1;
		update = true;
		break;

	/* ED_INIT with the edit_position setup for the x coordinate value
	   (comming in keycode) */
	case ED_CRSR:
		/* TODO: x coordinate -> cursor position conversion */

		/* TODO: REMOVE: begin ... ED_INIT like position return */
		if (*(ted->te_ptext) == '@')
			wt->edit_pos = 0;
		else
			wt->edit_pos = strlen(ted->te_ptext);
		/* TODO: REMOVE: end */
		break;

	default:
		return 1;
	}

	/* update the cursor position */
	*newpos = wt->edit_pos;

	if (update)
	{
		DIAGS(("newpos wt %lx, old_ed_obj %d, ed_obj %d ed_pos %d", wt, old_edit_obj, ed_obj, wt->edit_pos));
		if (old_edit_obj != -1)
			redraw_object(lock, wt, old_edit_obj);
		redraw_object(lock, wt, ed_obj);
	}

	return 1;
}
/* HR: Only the '\0' in the te_ptext field is determining the corsor position, NOTHING ELSE!!!!
		te_tmplen is a constant and strictly owned by the APP.
*/

unsigned long
XA_objc_edit(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(4,2,1)

	DIAG((D_form, client, "objc_edit"));

	if (pb->addrin[0])
		pb->intout[0] = edit_object(lock,
					    client,
				            pb->intin[3],
				            &client->wt,
					    (OBJECT*)pb->addrin[0],
					    pb->intin[0],
					    pb->intin[1],
					    &pb->intout[1]);
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

/* HR: objc_sysvar
   I think some programs need this to be able to do extended
    objects in 3D using progdefs.
    And shure they have the right to call this when they can do
    3D objects at all.    (appl_getinfo).
    .....
    I was right: Hip hip hurray!!! Luna.app buttons now OK.
    Now many more modern programs will behave correct.
*/
 
unsigned long
XA_objc_sysvar(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_appl, client, "objc_sysvar %s: %d for %s",
		pb->intin[0] == SV_INQUIRE ? "inq" : "set",
		pb->intin[1], c_owner(client)));

	CONTROL(4,3,0)

	if (pb->intin[0] == SV_INQUIRE)		/* SV_SET later: or not??? */
	{
		pb->intout[0] = 1;
		switch(pb->intin[1])
		{
		case LK3DIND:
			pb->intout[1] = 0;	/* text move (selecting) */
			pb->intout[2] = 0;	/* color change (selecting) */
			break;
		case LK3DACT:
			pb->intout[1] = 1;	/* text move (selecting) */
			pb->intout[2] = 0;	/* color change (selecting) */
			break;
		case INDBUTCOL:
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		case ACTBUTCOL:
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		case BACKGRCOL:
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		case AD3DVAL:
			pb->intout[1] = 2;	/* d3_pushbutton enlargement values :-) */
			pb->intout[2] = 2;
			break;
		default:
			pb->intout[0] = 0;
		}
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}
