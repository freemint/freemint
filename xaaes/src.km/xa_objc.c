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

#include "xa_objc.h"
#include "xa_global.h"

#include "rectlist.h"
#include "draw_obj.h"
#include "obtree.h"
#include "menuwidg.h"
#include "widgets.h"
#include "xa_form.h"

#include "mint/fcntl.h"

/*
 * Object Tree Handling Interface
 */
unsigned long
XA_objc_draw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	const RECT *r = (const RECT *)&pb->intin[2];
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	int item = pb->intin[0];
	CONTROL(6,1,1)

	DIAG((D_objc,client,"objc_draw rectangle: %d/%d,%d/%d", r->x, r->y, r->w, r->h));

	DIAG((D_objc, client, "objc_draw (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	if (obtree)
	{
		XA_TREE *wt;
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
		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);
		if (!wt)
			wt = set_client_wt(client, obtree);

		assert(wt);

#if 0
		if (client->fmd.state && !client->fmd.wind)
		{
			short nextobj;
			struct xa_window *wind;
			if (Setup_form_do(client, obtree, -2, &wind, &nextobj))
			{
				if (!(wind->window_status & XAWS_OPEN))
					open_window(lock, wind, wind->r);
				else if (!wind->nolist && !is_topped(wind))
					top_window(lock, wind, client);
				else
					display_window(lock, 4, wind, NULL);
			}
		}
		else
#endif
		{
			hidem();
			set_clip(r);		/* HR 110601: checks for special case? w <= 0 or h <= 0 */
	
			pb->intout[0] = draw_object_tree(lock,
							 wt,
							 obtree,
							 item,
							 pb->intin[1],		/* depth */
							 0);
			clear_clip();
			showm();
		}
	}
	else
		pb->intout[0] = 0;


	DIAG((D_objc, client, "objc_draw exit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	return XAC_DONE;
}

unsigned long
XA_objc_offset(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	CONTROL(1,3,1)

	if (obtree)
	{
		pb->intout[0] = ob_offset(obtree,
					  pb->intin[0],
					  &pb->intout[1],
					  &pb->intout[2]);
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_find(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(4,1,1)

	if (obtree)
	{
		struct widget_tree *wt;

		wt = obtree_to_wt(client, obtree);
		if (wt)
		{
			pb->intout[0] = obj_find(wt,
						 pb->intin[0],
						 pb->intin[1],
						 pb->intin[2],
						 pb->intin[3],
						 NULL);
		}
		else
		{
			pb->intout[0] = ob_find(obtree,
						pb->intin[0],
						pb->intin[1],
						pb->intin[2],
						pb->intin[3]);
		}
	}
	else
		pb->intout[0] = -1;

	DIAG((D_o,client,"XA_objc_find %lx + %d, %d/%d --> %d",
		pb->addrin[0], pb->intin[0], pb->intin[2], pb->intin[3], pb->intout[0]));

	return XAC_DONE;
}

unsigned long
XA_objc_change(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT*)pb->addrin[0];

	CONTROL(8,1,1)

	if (obtree)
	{
		short obj = pb->intin[0];
		XA_TREE *wt;
		struct xa_rect_list rl;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);
		if (!wt)
			wt = set_client_wt(client, obtree);

		assert(wt);

		rl.next = NULL;
		rl.r = *(RECT *)((long)&pb->intin[2]);

		obj_change(wt,
			    obj,
			    pb->intin[6],
			    obtree[obj].ob_flags,
			    pb->intin[7],
			    &rl);

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
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	
	CONTROL(2,1,1)

	if (obtree)
		pb->intout[0] = ob_add(obtree, pb->intin[0], pb->intin[1]);
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(1,1,1)

	if (obtree)
	{
		ob_remove(obtree, pb->intin[0]);
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_order(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(2,1,1)

	if (obtree)
	{
		ob_order((OBJECT *)pb->addrin[0], pb->intin[0], pb->intin[1]);
		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_edit(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(4,2,1)

	DIAG((D_form, client, "objc_edit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	if (obtree)
	{
		XA_TREE *wt;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);
		if (!wt)
			wt = set_client_wt(client, obtree);

		assert(wt);

		pb->intout[0] = obj_edit(wt,
					 pb->intin[3],		/* function	*/
					 pb->intin[0],		/* object	*/
					 pb->intin[1],		/* key		*/
					 pb->intin[2],		/* pos		*/
					 true,			/* redraw flag	*/
					 NULL,			/* rect list	*/
					 &pb->intout[1],	/* new pos	*/
					 NULL);			/* new obj	*/
	}
	else
		pb->intout[0] = 0;

	DIAG((D_form, client, "objc_edit exit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

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
		{
			pb->intout[1] = 0;	/* text move (selecting) */
			pb->intout[2] = 0;	/* color change (selecting) */
			break;
		}
		case LK3DACT:
		{
			pb->intout[1] = 1;	/* text move (selecting) */
			pb->intout[2] = 0;	/* color change (selecting) */
			break;
		}
		case INDBUTCOL:
		{
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		}
		case ACTBUTCOL:
		{
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		}
		case BACKGRCOL:
		{
			pb->intout[1] = screen.dial_colours.bg_col;
			break;
		}
		case AD3DVAL:
		{
			pb->intout[1] = 2;	/* d3_pushbutton enlargement values :-) */
			pb->intout[2] = 2;
			break;
		}
		default:
		{
			pb->intout[0] = 0;
		}
		}
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}
