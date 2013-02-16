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

#include "xa_graf.h"
#include "xa_global.h"

#include "k_main.h"
#include "k_mouse.h"
#include "c_mouse.h"
#include "draw_obj.h"
#include "obtree.h"
#include "widgets.h"
#include "c_window.h"
#include "xa_evnt.h"


#define THICKNESS 2

/*
 *	Ghost outline dragging routines.
 */
#define graf_void 0x8000


/* HR 150202: make rubber_box omnidirectional; helper functions. */

STATIC const RECT *
rect_dist(struct xa_client *client, RECT *r, RECT *d)
{
	short mb, x, y;

	check_mouse(client, &mb, &x, &y);

	d->x = r->x - x;
	d->y = r->y - y;
	d->w = r->x + r->w - x;
	d->h = r->y + r->h - y;

	return d;
}

const RECT *
rect_dist_xy(struct xa_client *client, short x, short y, RECT *r, RECT *d)
{
	d->x = r->x - x;
	d->y = r->y - y;
	d->w = r->x + r->w - x;
	d->h = r->y + r->h - y;

	return d;
}

void
check_wh_cp(RECT *c, COMPASS cp, short minw, short minh, short maxw, short maxh)
{
	switch (cp)
	{
		case NW:
		{
			if (minw && c->w < minw)
			{
				c->x -= (minw - c->w);
				c->w = minw;
			}
		}
		/* fallthrough */
		case N_:
		{
			if (minh && c->h < minh)
			{
				c->y -= (minh - c->h);
				c->h = minh;
			}
			break;
		}
		case SW:
		{
			if (minh && c->h < minh)
				c->h = minh;
		}
		/* fallthrough */
		case W_:
		{
			if (maxw && c->w > maxw)
			{
				c->x -= (c->w - maxw);
				c->w = maxw;
			}
			else if (minw && c->w < minw)
			{
				c->x -= (minw - c->w);
				c->w = minw;
			}
			break;
		}
		case SE:
		{
			if (minw && c->w < minw)
				c->w = minw;
		}
		/* fallthrough */
		case S_:
		{
			if (minh && c->h < minh)
				c->h = minh;
			break;
		}
		case NE:
		{
			if (minh && c->h < minh)
			{
				c->y -= (minh - c->h);
				c->h = minh;
			}
		}
		/* fallthrough */
		case E_:
		{
			if (maxw && c->w > maxw)
			{
				c->x += (c->w - maxw);
				c->w = maxw;
			}
			if (minw && c->w < minw)
			{
				c->w = minw;
			}
			break;
		}
		default:{};
	}
}

/* fit rectangle r in bounding rectangle b */
STATIC void
keep_inside(RECT *r, const RECT *b)
{
	if (r->x < b->x)
		r->x = b->x;
	else
	if (r->x + r->w > b->x + b->w)
		r->x = b->x + b->w - r->w;

	if (r->y < b->y)
		r->y = b->y;
	else
	if (r->y + r->h > b->y + b->h)
		r->y = b->y + b->h - r->h;
}

RECT
widen_rectangle(COMPASS xy, short mx, short my, RECT start, const RECT *d)
{
	RECT r = start;

	if( xy < E_ && my + d->y < root_window->wa.y ) /* cannot go above main-menubar */
		my = root_window->wa.y - d->y;
	switch (xy)
	{
	case NW:
		r.x = mx + d->x;
		r.w = (start.x + start.w) - r.x;
	/* v */
	case N_:
		r.y = my + d->y;
		r.h = (start.y + start.h) - r.y;
		break;
	case SW:
		r.h = my - r.y + d->h;
	/* v */
	case W_:
		r.x = mx + d->x;
		r.w = (start.x + start.w) - r.x;
		break;
	case SE:
		r.w = mx - r.x + d->w;
	/* v */
	case S_:
		r.h = my - r.y + d->h;
		break;
	case NE:
		r.y = my + d->y;
		r.h = (start.y + start.h) - r.y;
	/* v */
	case E_:
		r.w = mx - r.x + d->w;
		break;
	default:;
	}
	return r;
}

RECT
move_rectangle(short mx, short my, RECT r, const RECT *d)
{
	r.x = mx + d->x;
	r.y = my + d->y;
	return r;
}


STATIC bool
rect_changed(const RECT *n, const RECT *o)
{
	return		 n->x != o->x
		|| n->y != o->y
		|| n->w != o->w
		|| n->h != o->h;
}

static void
new_box(struct xa_vdi_settings *v, const RECT *r, RECT *o)
{
	if (o && !rect_changed(r, o))
		return;

	hidem();

	if (o)
	{
		(*v->api->gbox)(v, -1, o);
		(*v->api->gbox)(v, 0, o);
		*o = *r;
	}

	(*v->api->gbox)(v, -1, r);
	(*v->api->gbox)(v, 0, r);

	showm();
}

/* HR: dist for use with sizer: If you click in the middle of the sizer,
       than change your mind and release
       the button, without moving the mouse,
       the window size will not change.
*/

/* HR 150202: complete redesign of the functions drag_box and rubberbox.
              By removing the mouse distance calculation from these functions
              and widely use of the RECT structure, they now look rediculously
              simple. (Which is what they are, whether in a computer or not).
*/
/* HR 150202: make rubber_box omnidirectional. ;-) */

void
rubber_box(struct xa_client *client, COMPASS cp,
		 RECT r,
		 const RECT *dist,
		 int minw, int minh,
		 int maxw, int maxh,
		 RECT *last)
{
	struct xa_vdi_settings *v = client->vdi_settings;
	short x, y, mb;
	RECT old = r;

	(*v->api->l_color)(v, G_BLACK);

	(*v->api->wr_mode)(v, MD_XOR);
	new_box(v, &r, NULL);

	S.wm_count++;
	check_mouse(client, &mb, &x, &y);
	while (mb)
	{
		r = widen_rectangle(cp, x, y, r, dist);
		check_wh_cp(&r, cp, minw, minh, maxw, maxh);
		new_box(v, &r, &old);
		wait_mouse(client, &mb, &x, &y);
	}
	S.wm_count--;
	new_box(v, &r, NULL);
	(*v->api->wr_mode)(v, MD_TRANS);

	*last = r;
}

void
drag_box(struct xa_client *client, RECT r,
	 const RECT *bound,
	 const RECT *dist,
	 RECT *last)
{
	struct xa_vdi_settings *v = client->vdi_settings;
	short mb, x, y;
	RECT old = r;

	(*v->api->l_color)(v, G_BLACK);

	(*v->api->wr_mode)(v, MD_XOR);
	new_box(v, &r, NULL);

	S.wm_count++;
	check_mouse(client, &mb, &x, &y);
	while (mb)
	{
		r = move_rectangle(x, y, r, dist);
		keep_inside(&r, bound);
		new_box(v, &r, &old);
		wait_mouse(client, &mb, &x, &y);
	}
	S.wm_count--;

	new_box(v, &r,NULL);
	(*v->api->wr_mode)(v, MD_TRANS);

	*last = r;
}

/*
 * INTERFACE TO INTERACTIVE BOX ROUTINES
 */

unsigned long
XA_graf_dragbox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	RECT r, last, dist;

	CONTROL(8,3,0)

	r.x = pb->intin[2]; /* Strange binding. */
	r.y = pb->intin[3];
	r.w = pb->intin[0];
	r.h = pb->intin[1];

	drag_box(client, r, (const RECT *)&pb->intin[4],
		 rect_dist(client, &r, &dist), &last);

	pb->intout[0] = 1;
	pb->intout[1] = last.x;
	pb->intout[2] = last.y;

	DIAG((D_graf,client,"_drag_box"));

	return XAC_DONE;
}

unsigned long
XA_graf_rubberbox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	RECT r, d = {0};
	short mb, x, y;

	CONTROL(4,3,0)

	check_mouse(client, &mb, &x, &y);
	r.x = pb->intin[0];
	r.y = pb->intin[1];
	r.w = pb->intin[0] - x;
	if (r.w < 0)
		r.w = 0;
	r.h = pb->intin[1] - y;
	if (r.h < 0)
		r.h = 0;

	rubber_box(client, SE, r, &d,
		pb->intin[2], 	/* minimum */
		pb->intin[3],
		screen.r.w, 	/* maximum */
		screen.r.h,
		&r);

	pb->intout[0] = 1;
	pb->intout[1] = r.w;
	pb->intout[2] = r.h;

	DIAG((D_graf,client,"_rubbox x=%d, y=%d, w=%d, h=%d",pb->intin[0],pb->intin[1],r.w,r.h));

	return XAC_DONE;
}

unsigned long
XA_graf_watchbox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	XA_TREE *wt;

	CONTROL(4,1,1)

	if (validate_obtree(client, obtree, "XA_graf_watchbox:"))
	{
		DIAG((D_graf, client, "graf_watchbox"));

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

		pb->intout[0] = obj_watch( wt,
						C.Aes->vdi_settings,
						aesobj(wt->tree, pb->intin[1]),
						pb->intin[2],
						pb->intin[3],
						NULL,
						NULL);

		DIAG((D_graf,client,"_watchbox"));
	}
	return XAC_DONE;
}

/*
 * MagiC 5.10 extension
 */
unsigned long
XA_graf_wwatchbox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	struct xa_window *wind;
	XA_TREE *wt;

	CONTROL(4,1,1)

	if (validate_obtree(client, obtree, "XA_graf_wwatchbox:"))
	{
		if ((wind = get_wind_by_handle(lock, pb->intin[3])))
		{
			DIAG((D_graf, client, "graf_wwatchbox"));

			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);

			ret = obj_watch(wt,
					client->vdi_settings,
					aesobj(wt->tree, pb->intin[0]),
					pb->intin[1],
					pb->intin[2],
					NULL,
					wind->rect_list.start);

			DIAG((D_graf,client,"_wwatchbox"));
		}
	}
	pb->intout[0] = ret;
	return XAC_DONE;
}

unsigned long
XA_graf_slidebox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short d;
	RECT p, c,				/* parent/child rectangles. */
			 dist, last;			/* mouse distance, result. */
	OBJECT *tree = (OBJECT*)pb->addrin[0];
	int 	pi = pb->intin[0],
				ci = pb->intin[1];

	CONTROL(3,1,1)
	if (validate_obtree(client, tree, "XA_graf_slidebox:"))
	{
		p = *(RECT *)&tree[pi].ob_x;
		ob_offset(tree, aesobj(tree, pi), &p.x, &p.y);
		c = *(RECT *)&tree[ci].ob_x;
		ob_offset(tree, aesobj(tree, ci), &c.x, &c.y);

		rect_dist(client, &c, &dist); 	/* relative position of mouse in child rectangle */

		DIAG((D_graf,client,"XA_graf_slidebox dx:%d, dy:%d, p:%d/%d,%d/%d c:%d/%d,%d/%d",
			dist.x, dist.y, p, c));

		drag_box(client, c, &p, &dist, &last);

		if (pb->intin[2])
			d = pix_to_sl(last.y - p.y, p.h - c.h);
		else
			d = pix_to_sl(last.x - p.x, p.w - c.w);

		pb->intout[0] = d < 0 ? 0 : (d > SL_RANGE ? SL_RANGE : d);

		DIAG((D_graf,client," 	 -- 		d:%d last.x%d, last.y%d  p:%d/%d,%d/%d c:%d/%d,%d/%d",
			d, last.x, last.y, p, c));
	}
	return XAC_DONE;
}

#if 0
/* If shrinkbox isnt there, why then growbox? or move_box? */
unsigned long
XA_graf_growbox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(8,1,0)

	int x = pb->intin[0];
	int y = pb->intin[1];
	int w = pb->intin[2];
	int h = pb->intin[3];
	int xe = pb->intin[4];
	int ye = pb->intin[5];
	int we = pb->intin[6];
	int he = pb->intin[7];
	short dx, dy, dw, dh;
	int f;

	dx = (xe - x) / GRAF_STEPS;
	dy = (ye - y) / GRAF_STEPS;
	dw = (we - w) / GRAF_STEPS;
	dh = (he - h) / GRAF_STEPS;

	wr_mode(MD_XOR);

	hidem();

	for (f = 0; f < GRAF_STEPS; f++)
	{
		draw_2d_box(x, y, w, h, 1, G_BLACK);	/* Draw initial growing outline */
		x += dx;
		y += dy;
		w += dw;
		h += dh;
#if 0
		if (f % 2)
			Vsync();
#endif
	}

	x = pb->intin[0]; 					/* Reset to initial area */
	y = pb->intin[1];
	w = pb->intin[2];
	h = pb->intin[3];

	for (f = 0; f < GRAF_STEPS; f++)	/* Erase growing outline */
	{
		draw_2d_box(x, y, w, h, 1, G_BLACK);
		x += dx;
		y += dy;
		w += dw;
		h += dh;
#if 0
		if (f % 2)
			Vsync();
#endif
	}

	showm();

	wr_mode(MD_TRANS);

	pb->intout[0] = 1;

DIAG((D_graf,client->pid,"_growbox"));

	return XAC_DONE;
}


unsigned long
XA_graf_movebox(enum locks lock, struct xa_client *client, AESPB *pb)
{
	int w = pb->intin[0];
	int h = pb->intin[1];
	int x = pb->intin[2];
	int y = pb->intin[3];
	int xe = pb->intin[4];
	int ye = pb->intin[5];
	short dx, dy;
	int f;

	CONTROL(6,1,0)

	dx = (xe - x) / GRAF_STEPS;
	dy = (ye - y) / GRAF_STEPS;

	wr_mode(MD_XOR);

	hidem();

	for (f = 0; f < GRAF_STEPS; f++)	/* Draw initial images */
	{
		draw_2d_box(x, y, w, h, 1, G_BLACK);
		x += dx;
		y += dy;
#if 0
		if (f % 2)
			Vsync();
#endif
	}

	x = pb->intin[2]; 				/* Reset to go back over same area */
	y = pb->intin[3];

	for (f = 0; f < GRAF_STEPS; f++)	/* Erase them again */
	{
		draw_2d_box(x, y, w, h, 1, G_BLACK);
		x += dx;
		y += dy;
#if 0
		if (f % 2)
			Vsync();
#endif
	}

	showm();

	wr_mode(MD_TRANS);

	pb->intout[0] = 1;

DIAG((D_graf,client->pid,"_movebox"));

	return XAC_DONE;
}
#endif

/*
 * standard GEM arrow
 */
static MFORM M_ARROW_MOUSE =
{
	0x0000, 0x0000, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0xC000, 0xE000, 0xF000, 0xF800,
	0xFC00, 0xFE00, 0xFF00, 0xFF80,
	0xFFC0, 0xFFE0, 0xFE00, 0xEF00,
	0xCF00, 0x8780, 0x0780, 0x0380 },
	{ /* Cursor data */
	0x0000, 0x4000, 0x6000, 0x7000,
	0x7800, 0x7C00, 0x7E00, 0x7F00,
	0x7F80, 0x7C00, 0x6C00, 0x4600,
	0x0600, 0x0300, 0x0300, 0x0000 }
};

/*
 * standard GEM busy bee
 */
static MFORM M_BEE_MOUSE =
{
	0x0008, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x1C7E, 0x1CFF, 0x1CFF, 0xEFFF,
	0xFFFF, 0xFFFF, 0x3FFE, 0x3FFC,
	0x7FFE, 0xFFFE, 0xFFFF, 0xFFFF,
	0xFFFF, 0xFFFF, 0xFEFF, 0x7C3E },
	{ /* Cursor data */
	0x0800, 0x083C, 0x0062, 0x06C2,
	0xC684, 0x198A, 0x1B54, 0x06E0,
	0x1D58, 0x33FC, 0x6160, 0x42DE,
	0x44D8, 0x4A56, 0x3414, 0x0000 }
};

/*
 * standard GEM open hand
 */
#if 0
static MFORM M_HAND_MOUSE =
{
	0x0008, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0300, 0x1FB0, 0x3FF8, 0x3FFC,
	0x7FFE, 0xFFFE, 0xFFFE, 0x7FFF,
	0x7FFF, 0xFFFF, 0xFFFF, 0x7FFF,
	0x3FFF, 0x0FFF, 0x01FF, 0x003F },
	{ /* Cursor data */
	0x0300, 0x1CB0, 0x2448, 0x2224,
	0x7112, 0x9882, 0x8402, 0x4201,
	0x7001, 0x9801, 0x8401, 0x4000,
	0x3000, 0x0E00, 0x01C0, 0x0030 }
};
#else
/* non standard sliding hand */
static MFORM M_HAND_MOUSE =
{
	4,4,1,0,1,
	{ /* Mask data */
	0x7A00, 0x7F40, 0xFFE0, 0xFFF0,
	0xFFF8, 0x3FFC, 0x0FFE, 0xC7FF,
	0xEFFF, 0xFFFF, 0xFFFF, 0x7FFF,
	0x3FFF, 0x0FFF, 0x03FE, 0x00FC },
	{ /* Cursor data */
	0x5000, 0x2A00, 0x1540, 0xCAA0,
	0x3550, 0x0AA8, 0x0404, 0x0252,
	0xC402, 0xAA01, 0x5001, 0x3001,
	0x0C01, 0x0300, 0x00C0, 0x0000 }
};
#endif

/*
 * standard GEM outlined cross
 */
static MFORM M_OCRS_MOUSE =
{
	0x0007, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x07C0, 0x07C0, 0x06C0, 0x06C0,
	0x06C0, 0xFEFE, 0xFEFE, 0xC006,
	0xFEFE, 0xFEFE, 0x06C0, 0x06C0,
	0x06C0, 0x07C0, 0x07C0, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0380, 0x0280, 0x0280,
	0x0280, 0x0280, 0x7EFC, 0x4004,
	0x7EFC, 0x0280, 0x0280, 0x0280,
	0x0280, 0x0380, 0x0000, 0x0000 }
};

/*
 * standard GEM pointing finger
 */
static MFORM M_POINT_MOUSE =
{
	0x0000, 0x0000, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x3000, 0x7C00, 0x7E00, 0x1F80,
	0x0FC0, 0x3FF8, 0x3FFC, 0x7FFC,
	0xFFFE, 0xFFFE, 0x7FFF, 0x3FFF,
	0x1FFF, 0x0FFF, 0x03FF, 0x00FF },
	{ /* Cursor data */
	0x3000, 0x4C00, 0x6200, 0x1980,
	0x0C40, 0x32F8, 0x2904, 0x6624,
	0x93C2, 0xCF42, 0x7C43, 0x2021,
	0x1001, 0x0C41, 0x0380, 0x00C0 }
};

/*
 * standard GEM thin cross
 */
static MFORM M_TCRS_MOUSE =
{
	0x0007, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0380, 0x0380, 0x0380, 0x0380,
	0x0280, 0x0280, 0xFEFE, 0xF01E,
	0xFEFE, 0x0280, 0x0280, 0x0380,
	0x0380, 0x0380, 0x0380, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x7FFC,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0000, 0x0000 }
};

/*
 * standard GEM thick cross
 */
static MFORM M_THKCRS_MOUSE =
{
	0x0007, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x07C0, 0x07C0, 0x07C0, 0x07C0,
	0x07C0, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0x07C0, 0x07C0,
	0x07C0, 0x07C0, 0x07C0, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0380, 0x0380, 0x0380,
	0x0380, 0x0380, 0x7FFC, 0x7FFC,
	0x7FFC, 0x0380, 0x0380, 0x0380,
	0x0380, 0x0380, 0x0000, 0x0000 }
};

/*
 * standard GEM text cursor
 */
static MFORM M_TXT_MOUSE =
{
	0x0007, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x7E7E, 0x7FFE, 0x07E0, 0x03C0,
	0x03C0, 0x03C0, 0x03C0, 0x03C0,
	0x03C0, 0x03C0, 0x03C0, 0x03C0,
	0x03C0, 0x07E0, 0x7FFE, 0x7E7E },
	{ /* Cursor data */
	0x3C3C, 0x0660, 0x03C0, 0x0180,
	0x0180, 0x0180, 0x0180, 0x0180,
	0x0180, 0x0180, 0x0180, 0x0180,
	0x0180, 0x03C0, 0x0660, 0x3C3C }
};

/*
 * XaAES Data Uncertain logo cursor
 */
static MFORM M_BUBD_MOUSE =
{
	0x0007, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x07C0, 0x1FF0, 0x3FF8, 0x7FFC,
	0x7FFC, 0xFFFE, 0xFFFE, 0xFFFE,
	0xFFFE, 0xFFFE, 0x7FFC, 0x7FFC,
	0x3FF8, 0x1FF0, 0x07C0, 0x0000 },
	{ /* Cursor data */
	0x07C0, 0x1930, 0x2AA8, 0x5094,
	0x7114, 0x9012, 0x9112, 0x9FF2,
	0x8002, 0x8FE2, 0x4A24, 0x4B24,
	0x2B28, 0x1A30, 0x07C0, 0x0000 }
};

/*
 * XaAES re-sizer cursor
 */
#if 0
static MFORM M_SIZER_MOUSE =
{
	0x0007, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0000, 0xFE00, 0x8100, 0x8E00,
	0x8400, 0xA200, 0xB100, 0xA888,
	0x4454, 0x0234, 0x0114, 0x0084,
	0x01C4, 0x0204, 0x01FC, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0000, 0x7E00, 0x7000,
	0x7800, 0x5C00, 0x4E00, 0x4700,
	0x0388, 0x01C8, 0x00E8, 0x0078,
	0x0038, 0x01F8, 0x0000, 0x0000 }
};
#else
/*
 * XaAES south east sizer cursor
 */
static MFORM M_SE_SIZER_MOUSE =
{
	0x0007, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0000, 0x7F80, 0x7F80, 0x7F80,
	0x7E00, 0x7F00, 0x7F80, 0x77CE,
	0x73EE, 0x01FE, 0x00FE, 0x007E,
	0x01FE, 0x01FE, 0x01FE, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0000, 0x3F00, 0x3C00,
	0x3C00, 0x3E00, 0x2700, 0x2380,
	0x01C4, 0x00E4, 0x007C, 0x003C,
	0x003C, 0x00FC, 0x0000, 0x0000 }
};
#endif
/*
 * XaAES north east sizer cursor
 */
static MFORM M_NE_SIZER_MOUSE =
{
	0x0007, 0x0008, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0000, 0x01FE, 0x01FE, 0x01FE,
	0x007E, 0x00FE, 0x01FE, 0x73EE,
	0x77CE, 0x7F80, 0x7F00, 0x7E00,
	0x7F80, 0x7F80, 0x7F80, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0000, 0x00FC, 0x003C,
	0x003C, 0x007C, 0x00E4, 0x01C4,
	0x2380, 0x2700, 0x3E00, 0x3C00,
	0x3C00, 0x3F00, 0x0000, 0x0000 }
};

/*
 * XaAES vertical sizer cursor
 */
static MFORM M_VERTSIZER_MOUSE =
{
	0x0008, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0180, 0x03C0, 0x07E0, 0x0FF0,
	0x1FF8, 0x1FF8, 0x03C0, 0x03C0,
	0x03C0, 0x03C0, 0x1FF8, 0x1FF8,
	0x0FF0, 0x07E0, 0x03C0, 0x0180 },
	{ /* Cursor data */
	0x0000, 0x0180, 0x03C0, 0x07E0,
	0x0FF0, 0x0180, 0x0180, 0x0180,
	0x0180, 0x0180, 0x0180, 0x0FF0,
	0x07E0, 0x03C0, 0x0180, 0x0000 }
};

/*
 * XaAES horizontal sizer cursor
 */
static MFORM M_HORSIZER_MOUSE =
{
	0x0008, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0000, 0x0000, 0x0000, 0x0C30,
	0x1C38, 0x3C3C, 0x7FFE, 0xFFFF,
	0xFFFF, 0x7FFE, 0x3C3C, 0x1C38,
	0x0C30, 0x0000, 0x0000, 0x0000 },
	{ /* Cursor data */
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0810, 0x1818, 0x381C, 0x7FFE,
	0x7FFE, 0x381C, 0x1818, 0x0810,
	0x0000, 0x0000, 0x0000, 0x0000 }
};

/*
 * XaAES mover cursor
 */
static MFORM M_MOVER_MOUSE =
{
	0x0008, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x0000, 0x0080, 0x01C0, 0x0080,
	0x0080, 0x0080, 0x1084, 0x3FFE,
	0x1084, 0x0080, 0x0080, 0x0080,
	0x01C0, 0x0080, 0x0000, 0x0000 },
	{ /* Cursor data */
	0x0080, 0x0140, 0x0220, 0x0140,
	0x0140, 0x1144, 0x2F7A, 0x4001,
	0x2F7A, 0x1144, 0x0140, 0x0140,
	0x0220, 0x0140, 0x0080, 0x0000 }
};

/*static*/ MFORM M_POINTSLIDE_MOUSE =
{
	0x0008, 0x0007, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0x03C0, 0x03C0, 0x3FFC, 0x3FFC,
	0x1FF8, 0x0FF0, 0x07E0, 0x03C0,
	0x03C0, 0x07E0, 0x0FF0, 0x1FF8,
	0x3FFC, 0x3FFC, 0x03C0, 0x03C0 },
	{ /* Cursor data */
	0x0000, 0x0180, 0x0180, 0x1FF8,
	0x0FF0, 0x07E0, 0x03C0, 0x0180,
	0x0180, 0x03C0, 0x07E0, 0x0FF0,
	0x1FF8, 0x0180, 0x0180, 0x0000 }
};

static void
set_mouse_shape(short m_shape, MFORM *m_form, struct xa_client *client, bool aesm)
{
	bool chg = false;

	if (aesm)
	{
		/* If the shape to set is -1, we want to turn off the AES-mouse, which
		 * is used to indicate possible actions underneath it, like showing that
		 * a widget can be used to resize a window.
		 */
		if (m_shape == -1)
		{
			chg = C.realmouse != C.mouse ? true : false;
			C.aesmouse = m_shape;
			C.aesmouse_form = NULL;
			C.realmouse = C.mouse;
			C.realmouse_form = C.mouse_form;
			C.realmouse_owner = C.mouse_owner;
		}
		/* If a valid shape, we want to turn on AES-mouse, it overrides current
		 * application settings to show possible actions..
		 */
		else if (C.realmouse != m_shape)
		{
			C.aesmouse = m_shape;
			C.aesmouse_form = m_form;
			chg = true;
			C.realmouse = m_shape;
			C.realmouse_form = m_form;
			C.realmouse_owner = NULL;
		}
	}
	else
	{
		/* If aesmouse shape == -1, the AES have not overridded the current
		 * mouseshape (which it does to indicate that a widget can be used
		 * to resize, for example). So we do the actual mouse-shape change...
		 */
		if (C.aesmouse == -1)
		{
			chg = (C.realmouse != m_shape || C.realmouse_owner != client) ? true : false;
			C.realmouse = m_shape;
			C.realmouse_form = m_form;
			C.realmouse_owner = client;
			C.mouse = m_shape;
			C.mouse_form = m_form;
			C.mouse_owner = client;
		}
		/* If the AES is currently indicating a possible action using mouse
		 * shape, we store this applications mouse-shape setting for later
		 * use
		 */
		else if (m_shape != C.mouse)
		{
			C.mouse = m_shape;
			C.mouse_form = m_form;
			C.mouse_owner = client;
		}
	}
	if (chg)
	{
		hidem();
		vsc_form(C.P_handle, (short *)C.realmouse_form);
		showm();
	}
}

inline static MFORM *
get_mform(short m_shape)
{
	MFORM *ret_mf;

	switch (m_shape)
	{
		case ARROW:
			ret_mf = &M_ARROW_MOUSE;
			break;
		case TEXT_CRSR:
			ret_mf = &M_TXT_MOUSE;
			break;
		case HOURGLASS:
			ret_mf = &M_BEE_MOUSE;
			break;
		case POINT_HAND:
			ret_mf = &M_POINT_MOUSE;
			break;
		case FLAT_HAND:
			ret_mf = &M_HAND_MOUSE;
			break;
		case THIN_CROSS:
			ret_mf = &M_TCRS_MOUSE;
			break;
		case THICK_CROSS:
			ret_mf = &M_THKCRS_MOUSE;
			break;
		case OUTLN_CROSS:
			ret_mf = &M_OCRS_MOUSE;
			break;
		case XACRS_BUBBLE_DISC: 		/* The Data Uncertain logo */
			ret_mf = &M_BUBD_MOUSE;
			break;
		case XACRS_RESIZER: 		/* The 'resize window' cursor */
			ret_mf = &M_SE_SIZER_MOUSE;
			break;
		case XACRS_NE_SIZER:
			ret_mf = &M_NE_SIZER_MOUSE;
			break;
		case XACRS_MOVER: 		/* The 'move window' cursor */
			ret_mf = &M_MOVER_MOUSE;
			break;
		case X_UPDOWN:
		case XACRS_VERTSIZER: 		/* The 'vertical size window' cursor */
			ret_mf = &M_VERTSIZER_MOUSE;
			break;
		case X_LFTRT:
		case XACRS_HORSIZER:			/* The 'horizontal size window' cursor */
			ret_mf = &M_HORSIZER_MOUSE;
			break;
		case XACRS_POINTSLIDE:
			ret_mf = &M_POINTSLIDE_MOUSE;
			break;
		default:
		{
			ret_mf = NULL;
			break;
		}
	}
	return ret_mf;
}

/*
 * AES xa_graf_mouse() routines
 * Small extension to give a couple of extra cursor shapes
 * (Data Uncertain logo, mover & sizers)
 */
void
xa_graf_mouse(int m_shape, MFORM *mf, struct xa_client *client, bool aesm)
{
	if (m_shape == -1 && aesm)
	{
		set_mouse_shape(m_shape, NULL, client, aesm);
		return;
	}

	switch (m_shape)
	{
	case M_ON:
		showm();
		return;
	case M_OFF:
		hidem();
		return;
	case M_SAVE:
	case M_RESTORE:
	case M_LAST:
		return;
	case USER_DEF:
		set_mouse_shape(m_shape, mf ? mf : &M_BUBD_MOUSE, client, aesm);
		//set_mouse_shape(m_shape, &M_BUBD_MOUSE, client, aesm);
		break;
	case X_MRESET:
	case M_FORCE:
		forcem();
		return;
	default:
	{
		if ((mf = get_mform(m_shape)))	//case X_MGET??
			set_mouse_shape(m_shape, mf, client, aesm);
		break;
	}
	}
}

void
set_client_mouse(struct xa_client *client, short which, short m_shape, MFORM *mf)
{
	short *m;
	MFORM **dmf;
	bool set = which & 0x8000;

	which &= ~0x8000;
//	display("set client mouse %x, set=%s", which, set ? "true":"false");
	switch (which)
	{
		case SCM_MAIN:
		{
			m = &client->mouse;
			dmf = &client->mouse_form;
			break;
		}
		case SCM_PREV:
		{
			m = &client->prev_mouse;
			dmf = &client->prev_mouse_form;
			break;
		}
		case SCM_SAVE:
		{
			m = &client->save_mouse;
			dmf = &client->save_mouse_form;
			break;
		}
		default: dmf = NULL, m = NULL; break;
	}
	if (dmf)
	{
		if (!mf)
			mf = get_mform(m_shape);
		if (mf)
		{
			*m = m_shape;
			*dmf = mf;
			if (set)
			{
				set_mouse_shape(m_shape, mf, client, false);
			}
		}
	}
}

/* Slight differance from GEM here - each application can have a different mouse form,
 * and the one that is used is for the client with the top window.
 * A non-top application can still hide & show the mouse though, to ensure that redraws
 * are done correctly
 */
/*
 * Ozk: XA_xa_graf_mouse() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
#if 1
unsigned long
XA_xa_graf_mouse(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short m = pb->intin[0];

	CONTROL(1,1,1)

	if (m == M_OFF || m == M_ON)
	{
		/* Any client can hide the mouse (required for redraws by clients that aren't top) */
		xa_graf_mouse(m, NULL, NULL, false);
#if GENERATE_DIAGS
		if (client)
			DIAG((D_f,client,"mouse %d %s", client->mouse, m == M_ON ? "on" : "off"));
		else
			DIAG((D_f,NULL,"mouse (non AES process (pid %ld)) %s", p_getpid(), m == M_ON ? "on" : "off"));
#endif
	}
	/*
	 * Ozk: For now we ignore modes other than M_OFF & M_ON
	 * when process is not a valid AES process.
	 */
	else if (client)
	{
		if (m == M_SAVE)
		{
			set_client_mouse(client, SCM_SAVE, client->mouse, client->mouse_form);
			DIAG((D_f,client,"M_SAVE; mouse_form %d", client->mouse));
		}
		else if (m == M_RESTORE)
		{
			/*
			 * Ozk: Not sure if M_RESTORE should set the previous mouse
			 * cursor ... but we do it until someone complains, incase
			 * it is correct
			 */
			set_client_mouse(client, SCM_PREV, client->save_mouse, client->save_mouse_form);
//			xa_graf_mouse(client->save_mouse, client->save_mouse_form, client, false);
			DIAG((D_f,client,"M_RESTORE; mouse_form from %d to %d", client->mouse, client->save_mouse));
			set_client_mouse(client, SCM_MAIN|0x8000, client->save_mouse, client->save_mouse_form);
		}
		else if (m == M_PREVIOUS)
		{
			short pm;
			MFORM *pmf;

			/*
			 * Ozk: Not sure about this either, but methinks that as we set to the previous
			 * mouse cursor, the current becomes new previous. Consecutive M_PREVIOUS calls
			 * will then toggle between the two last used mouse shapes.
			 */
//			xa_graf_mouse(client->prev_mouse, client->prev_mouse_form, client, false);
			DIAG((D_f,client,"M_PREVIOUS; mouse_form from %d to %d", client->mouse, C.mouse));
			pm			= client->mouse;
			pmf 		= client->mouse_form;
			set_client_mouse(client, SCM_MAIN|0x8000, client->prev_mouse, client->prev_mouse_form);
			set_client_mouse(client, SCM_PREV, pm, pmf);
		}
		else
		{
			MFORM *ud = NULL;

			if (m == USER_DEF && (ud = (MFORM *)pb->addrin[0]))
			{
				client->user_def = *ud;
				ud = &client->user_def;
			}

			set_client_mouse(client, SCM_PREV, client->mouse, client->mouse_form);
//			xa_graf_mouse(m, ud, client, false);
			set_client_mouse(client, SCM_MAIN|0x8000, m, ud);
			DIAG((D_f,client,"mouse_form to %d(%lx)", m, ud));
		}
	}
	else if( m == X_MGET )
	{
		pb->intout[0] = tellm();
		if( pb->addrin[0] && C.realmouse_form )
			*(MFORM*)pb->addrin[0] = *C.realmouse_form;
		return XAC_DONE;
	}
	else if (m != M_SAVE && m != M_RESTORE && m != M_PREVIOUS)
	{
		xa_graf_mouse(m, (MFORM *)pb->addrin[0], client, false);
		DIAG((D_f, NULL, "mouse form to %d for non AES process (pid %ld)", m, p_getpid()));
	}

	/* Always return no error */
	pb->intout[0] = 1;

	return XAC_DONE;
}
#else
unsigned long
XA_xa_graf_mouse(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short m = pb->intin[0];

	CONTROL(1,1,1)

	if (m == M_OFF || m == M_ON)
	{
		/* Any client can hide the mouse (required for redraws by clients that aren't top) */
		xa_graf_mouse(m, NULL, NULL, false);
#if GENERATE_DIAGS
		if (client)
			DIAG((D_f,client,"mouse %d %s", client->mouse, m == M_ON ? "on" : "off"));
		else
			DIAG((D_f,NULL,"mouse (non AES process (pid %ld)) %s", p_getpid(), m == M_ON ? "on" : "off"));
#endif
	}
	/*
	 * Ozk: For now we ignore modes other than M_OFF & M_ON
	 * when process is not a valid AES process.
	 */
	else if (client)
	{
		if (m == M_SAVE)
		{
			client->save_mouse			= client->mouse;
			client->save_mouse_form = client->mouse_form;
			DIAG((D_f,client,"M_SAVE; mouse_form %d", client->mouse));
		}
		else if (m == M_RESTORE)
		{
			/*
			 * Ozk: Not sure if M_RESTORE should set the previous mouse
			 * cursor ... but we do it until someone complains, incase
			 * it is correct
			 */
			client->prev_mouse = client->save_mouse;
			client->prev_mouse_form = client->save_mouse_form;

			xa_graf_mouse(client->save_mouse, client->save_mouse_form, client, false);
			DIAG((D_f,client,"M_RESTORE; mouse_form from %d to %d", client->mouse, client->save_mouse));
			client->mouse 			= client->save_mouse;
			client->mouse_form	= client->save_mouse_form;
		}
		else if (m == M_PREVIOUS)
		{
			short pm;
			MFORM *pmf;

			/*
			 * Ozk: Not sure about this either, but methinks that as we set to the previous
			 * mouse cursor, the current becomes new previous. Consecutive M_PREVIOUS calls
			 * will then toggle between the two last used mouse shapes.
			 */
			xa_graf_mouse(client->prev_mouse, client->prev_mouse_form, client, false);
			DIAG((D_f,client,"M_PREVIOUS; mouse_form from %d to %d", client->mouse, C.mouse));
			pm			= client->mouse;
			pmf 		= client->mouse_form;
			client->mouse 	= client->prev_mouse;
			client->mouse_form	= client->prev_mouse_form;
			client->prev_mouse	= pm;
			client->prev_mouse_form = pmf;
		}
		else
		{
			MFORM *ud = NULL; //(MFORM *)pb->addrin[0];

			if (m == USER_DEF && (ud = (MFORM *)pb->addrin[0]))
			{
				client->user_def = *ud;
				ud = &client->user_def;
			}

			client->prev_mouse = client->mouse;
			client->prev_mouse_form = client->mouse_form;

			xa_graf_mouse(m, ud, client, false);
			client->mouse = m;
			client->mouse_form = ud;
			DIAG((D_f,client,"mouse_form to %d(%lx)", m, ud));
		}
	}
	else if (m != M_SAVE && m != M_RESTORE && m != M_PREVIOUS)
	{
		xa_graf_mouse(m, (MFORM *)pb->addrin[0], client, false);
		DIAG((D_f, NULL, "mouse form to %d for non AES process (pid %ld)", m, p_getpid()));
	}

	/* Always return no error */
	pb->intout[0] = 1;

	return XAC_DONE;
}
#endif

/*
 * Ozk: XA_graf_handle() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
unsigned long
XA_graf_handle(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,5,0)

	/* graf_handle returns the physical workstation handle */
	pb->intout[0] = C.P_handle;
	pb->intout[1] = screen.c_max_w;
	pb->intout[2] = screen.c_max_h;
	pb->intout[3] = screen.c_max_w + 2;
	pb->intout[4] = screen.c_max_h + 2;



#if GENERATE_DIAGS
	if (client)
		DIAG((D_graf,client,"_handle %d", pb->intout[0]));
	else
		DIAG((D_graf,NULL,"_handle %d for non AES process", pb->intout[0]));
#endif

	return XAC_DONE;
}


/*
 * Ozk: XA_graf_mkstate() may be called by processes not yet called
 * appl_init(). So, it must not depend on client being valid!
 */
unsigned long
XA_graf_mkstate(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,5,0)

	if (client)
	{
		struct mbs mbs;

		/* Ozk:
		 * Make sure button events are delivered here .. eb_model doesnt
		 * enter evnt_multi() inside of its own popup implementation.
		 * Why, oh WHY! cannot programmers use the OS's functions!?
		 */

//		while (dispatch_selcevent(client, cXA_deliver_button_event))
//			;
#if 1
		if (client->md_head->clicks == -1 && client->md_head == client->md_tail)
		{
			if (!dispatch_selcevent(client, cXA_deliver_button_event, false))
				dispatch_selcevent(client, cXA_button_event, false);

//			if (client->md_head->clicks == -1 && client->md_head == client->md_tail)
//				check_mouse(client, &client->md_head->cstate, NULL,NULL);

//			check_mouse(client, &pb->intout[3], &pb->intout[1], &pb->intout[2]);
//			vq_key_s(C.P_handle, &pb->intout[4]);
		}
//		else
//		{
		get_mbstate(client, &mbs);
		pb->intout[1] = mbs.x;
		pb->intout[2] = mbs.y;
		pb->intout[3] = mbs.b;
		pb->intout[4] = mbs.ks;
//		}
#else
		get_mbstate(client, &mbs);
		pb->intout[1] = mbs.x;
		pb->intout[2] = mbs.y;
		pb->intout[3] = mbs.b;
		pb->intout[4] = mbs.ks;
#endif
	}
	else
		multi_intout(NULL, pb->intout, 0);

	pb->intout[0] = 1;

#if GENERATE_DIAGS
	if (client)
	{
		DIAG((D_mouse,client,"_mkstate: %d/%d, b=0x%x, ks=0x%x",
			pb->intout[1], pb->intout[2], pb->intout[3], pb->intout[4]));
	}
	else
	{
		DIAG((D_mouse,NULL,"_mkstate: %d/%d, b=0x%x, ks=0x%x",
			mainmd.x, mainmd.y, mainmd.state, mainmd.kstate));
	}
#endif
	return XAC_DONE;
}
