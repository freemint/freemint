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
#include "c_window.h"

#include "mint/fcntl.h"

/*
 * Object Tree Handling Interface
 */
unsigned long
XA_objc_draw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	const RECT *r = (const RECT *)&pb->intin[2];
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	struct xa_vdi_settings *v = client->vdi_settings;
	short item = pb->intin[0];
	CONTROL(6,1,1)

	DIAG((D_objc,client,"objc_draw rectangle: %d/%d,%d/%d", r->x, r->y, r->w, r->h));

	DIAG((D_objc, client, "objc_draw (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	if (validate_obtree(client, obtree, "XA_objc_draw:"))
	{
		XA_TREE *wt;
		
		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

		assert(wt);

		obj_init_focus(wt, OB_IF_ONLY_EDITS);

		wt->e.c_state &= ~OB_CURS_EOR;
		{
			hidem();
			(*v->api->set_clip)(v, r);		/* HR 110601: checks for special case? w <= 0 or h <= 0 */
	
			pb->intout[0] = draw_object_tree(lock,
							 wt,
							 obtree,
							 v,
							 aesobj(wt->tree, item),
							 pb->intin[1],		/* depth */
							 NULL,
							 0);

			(*v->api->clear_clip)(v);
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
XA_objc_wdraw(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	struct xa_vdi_settings *v;
	struct xa_window *wind;
	struct xa_rect_list *rl;
	short item = pb->intin[0], ret = 0;
	
	CONTROL(3,0,2)

	DIAG((D_objc,client,"objc_wdraw "));

	DIAG((D_objc, client, "objc_draw (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	if (validate_obtree(client, obtree, "XA_objc_draw:"))
	{
		if ((wind = get_wind_by_handle(lock, pb->intin[2])))
		{
			XA_TREE *wt;
		
			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);

			assert(wt);

			rl = wind->rect_list.start;
			v = client->vdi_settings;
			obj_init_focus(wt, OB_IF_ONLY_EDITS);

			wt->e.c_state &= ~OB_CURS_EOR;
			
			hidem();
			if (rl)
			{
				RECT r;
				do
				{
					r = rl->r;
					if (!pb->addrin[1] || xa_rect_clip((RECT *)pb->addrin[1], &r, &r))
					{
						(*v->api->set_clip)(v, &r);
						draw_object_tree(0, wt, wt->tree, v, aesobj(wt->tree, item), pb->intin[1], NULL, 0);
					}
				} while ((rl = rl->next));
			}
			
			/*
			 * Ozk: Ok.. looks like the AES should automagically draw the cursor...
			 * Nope!
			 */
			(*v->api->clear_clip)(v);
			showm();
			ret = 1;
		}
	}

	pb->intout[0] = ret;

	DIAG((D_objc, client, "objc_draw exit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

	return XAC_DONE;
}

unsigned long
XA_objc_offset(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT*)pb->addrin[0];
	CONTROL(1,3,1)

	if (validate_obtree(client, obtree, "XA_objc_offset:"))
	{
		pb->intout[0] = ob_offset(obtree,
					  aesobj(obtree, pb->intin[0]),
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
	short depth;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(4,1,1)

	if (validate_obtree(client, obtree, "XA_objc_find:"))
	{
		struct widget_tree *wt;
		struct xa_aes_object o;
		
		depth = pb->intin[1] & ~0x8000;
		
		if (pb->control[0] == 49)	/* objc_xfind? */
			depth |= 0x8000;
		
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);

		assert(wt);

// 		if (wt)
		{
			o = obj_find(
				 wt,
				 aesobj(wt->tree, pb->intin[0]),
				 depth,
				 pb->intin[2],
				 pb->intin[3],
				 NULL);
			pb->intout[0] = aesobj_item(&o);
		}
	#if 0
		else
		{
			pb->intout[0] = ob_find(obtree,
						pb->intin[0],
						depth,
						pb->intin[2],
						pb->intin[3]);
		}
	#endif
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

	if (validate_obtree(client, obtree, "XA_objc_change:"))
	{
		short obj = pb->intin[0];
		XA_TREE *wt;
		struct xa_rect_list rl;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

		assert(wt);

		rl.next = NULL;
		rl.r = *(const RECT *)(pb->intin + 2);

		obj_change(wt,
			   C.Aes->vdi_settings,
			    aesobj(wt->tree, obj), -1,
			    pb->intin[6],
			    obtree[obj].ob_flags,
			    pb->intin[7],
			    NULL,
			    &rl, 0);

		pb->intout[0] = 1;
	}
	else
		pb->intout[0] = 0;

	return XAC_DONE;
}

unsigned long
XA_objc_wchange(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT*)pb->addrin[0];

	CONTROL(3,0,2)

	if (validate_obtree(client, obtree, "XA_objc_change:"))
	{
		short obj = pb->intin[0];
		XA_TREE *wt;
		struct xa_window *wind;

		if ((wind = get_wind_by_handle(lock, pb->intin[2])))
		{		
			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);

			assert(wt);

			obj_change(wt,
				   C.Aes->vdi_settings,
				    aesobj(wt->tree, obj), -1,
				    pb->intin[1],
				    obtree[obj].ob_flags,
				    true,
				    (RECT *)pb->addrin[1],
				    wind->rect_list.start, 0);

			ret = 1;
		}
	}

	pb->intout[0] = ret;
	
	return XAC_DONE;
}

/* HR 020604: child must be inserted at the END of the list of children!!! */
unsigned long
XA_objc_add(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	
	CONTROL(2,1,1)
	
	DIAG((D_form, client, "xa_objc_add: obtree=%lx, parent=%d, child=%d",
		obtree, pb->intin[0], pb->intin[1]));
	
// 	display("xa_objc_add: %s, obtree=%lx, parent=%d, child=%d",
// 		client->name, obtree, pb->intin[0], pb->intin[1]);

	if (validate_obtree(client, obtree, "XA_objc_add:"))
	{
		ret = ob_add(obtree, pb->intin[0], pb->intin[1]);
	}
	pb->intout[0] = ret;

	DIAGS((" -- return %d", pb->intout[0]));
	
	return XAC_DONE;
}

unsigned long
XA_objc_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	
	CONTROL(1,1,1)

// 	display("xa_objc_delete: %s, obtree=%lx, obj=%d",
// 		client->name, obtree, pb->intin[0]);
	
	if (validate_obtree(client, obtree, "XA_objc_delete:"))
	{
		ret = ob_remove(obtree, pb->intin[0]);
		if (ret < 0)
			ret = 0;
		else
			ret = 1;
	}
	pb->intout[0] = ret;

	return XAC_DONE;
}

unsigned long
XA_objc_order(enum locks lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(2,1,1)

// 	display("xa_objc_add: %s, obtree=%lx, obj=%d, pos=%d",
// 		client->name, obtree, pb->intin[0], pb->intin[1]);
	
	if (validate_obtree(client, obtree, "XA_objc_order:"))
	{
		ob_order(obtree, pb->intin[0], pb->intin[1]);
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


// 	display("objc_edit (%d %d %d %d) for %s",
// 		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3], client->name);
	
	if (validate_obtree(client, obtree, "XA_objc_edit:"))
	{
		XA_TREE *wt;
		struct xa_aes_object object;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

		assert(wt);

		object = aesobj(obtree, pb->intin[0]);

		pb->intout[0] = obj_edit(wt,
					 C.Aes->vdi_settings,
					 pb->intin[3],		/* function	*/
					 object,		/* object	*/
					 pb->intin[1],		/* key		*/
					 -1,/* pb->intin[2],*/	/* pos		*/
					 NULL,			/* string	*/
					 true,			/* redraw flag	*/
					 NULL,			/* Clip rect	*/
					 NULL,			/* rect list	*/
					 &pb->intout[1],	/* new pos	*/
					 NULL);			/* new obj	*/
	}
	else
		pb->intout[0] = 0;

	DIAG((D_form, client, "objc_edit exit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

// 	display("xa_objc_edit: exit (%d %d %d %d)\n",
// 		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]);
	return XAC_DONE;
}
unsigned long
XA_objc_wedit(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(5,2,1)

	DIAG((D_form, client, "objc_edit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));


// 	display("objc_edit (%d %d %d %d) for %s",
// 		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3], client->name);
	
	if (validate_obtree(client, obtree, "XA_objc_edit:"))
	{
		XA_TREE *wt;
		struct xa_window *wind;

		if ((wind = get_wind_by_handle(lock, pb->intin[4])))
		{
			struct xa_aes_object object;
			
			if (!(wt = obtree_to_wt(client, obtree)))
				wt = new_widget_tree(client, obtree);

			assert(wt);

			object = aesobj(obtree, pb->intin[0]);
			
			ret =   obj_edit(wt,
					 client->vdi_settings,
					 pb->intin[3],		/* function	*/
					 object,		/* object	*/
					 pb->intin[1],		/* key		*/
					 -1,/* pb->intin[2],*/	/* pos		*/
					 NULL,			/* string	*/
					 true,			/* redraw flag	*/
					 NULL,			/* Clip rect	*/
					 wind->rect_list.start,	/* rect list	*/
					 &pb->intout[1],	/* new pos	*/
					 NULL);			/* new obj	*/
			
		}
	}
	
	pb->intout[0] = ret;

	DIAG((D_form, client, "objc_edit exit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

// 	display("xa_objc_edit: exit (%d %d %d %d)\n",
// 		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]);
	return XAC_DONE;
}

/* HR: objc_sysvar
   I think some programs need this to be able to do extended
    objects in 3D using progdefs.
    And sure they have the right to call this when they can do
    3D objects at all.    (appl_getinfo).
    .....
    I was right: Hip hip hurray!!! Luna.app buttons now OK.
    Now many more modern programs will behave correct.
*/
 
unsigned long
XA_objc_sysvar(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;

	DIAG((D_appl, client, "objc_sysvar %s: %d for %s",
		pb->intin[0] == SV_INQUIRE ? "inq" : "set",
		pb->intin[1], c_owner(client)));

	CONTROL(4,3,0)

	if (client->objcr_api)
	{
		short i, vals[4];
		ret = (*client->objcr_api->objc_sysvar)(client->objcr_theme, pb->intin[0], pb->intin[1], &vals[0],&vals[1],&vals[2],&vals[3]);
		if (ret >= 0)
		{
			for (i = 0; i < ret; i++)
				pb->intout[i + 1] = vals[i];
			ret = 1;
		}
		else if (ret == -1)
			ret = 0;
	}
	pb->intout[0] = ret;

#if 0		
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
			pb->intout[1] = objc_rend.dial_colours.bg_col;
			break;
		}
		case ACTBUTCOL:
		{
			pb->intout[1] = objc_rend.dial_colours.bg_col;
			break;
		}
		case BACKGRCOL:
		{
			pb->intout[1] = objc_rend.dial_colours.bg_col;
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
#endif
	return XAC_DONE;
}

/* GET/SET object type
 * If set, ob_type is set to type found in intin[2].
 * Returns (if SET, previous) ob_type in intout[1].
 */
#define OBGET_OBTYPE	0x0001
#define OBSET_OBTYPE	0x8001

#define OBGET_STRING	0x0002
#define OBSET_STRING	0x8002

#define OBGET_CICON	0x0003
#define OBSET_CICON	0x8003

#define OBGET_OBSPEC	0x0004
#define OBSET_OBSPEC	0x8004

#define OBGET_BFOBSPEC	0x0005
#define OBSET_BFOBSPEC	0x8005

#define OBGET_OBJECT	0x0006

#define OBGET_AREA	0x0007

#define OBGET_OBFLAGS	0x0008
#define OBSET_OBFLAGS	0x8008

#define OBGET_OBSTATE	0x0009
#define OBSET_OBSTATE	0x8009

// opcode 66
// objc_data(tree, object, what, size, wh, (void *)data0, (void *)data1, *short, *long);
// objc_data(tree, obj_idx, what, flags, winhand, clip, ...);
// intin[0] = obj_idx
// intin[1] = what
// intin[2] = flags
// intin[3] = winhand
//
// addrin[0] = tree
// addrin[1] = clip RECT ptr
//
// intout[0] - 1 = OK, 0 = error
//
// addrout[0] - mode dependant

/*
        intin      intin[0]    index
        intin+2    intin[1]    request
        intin+4    intin[2]    windowhandle
        intin+6    intin[3]    p1
        intin+8    intin[4]    p2
        intin+10   intin[5]    p3
        intin+12   intin[6]    p4

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip
        addrin+8   addrin[2]   pin0
        addrin+12  addrin[3]   pin1
    
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)

        intout+2   intout[1]  out1
        intout+4   intout[2]  out2
        intout+6   intout[3]  out3
        intout+8   intout[4]  out4

        addrout    addrout[0] aout0
        addrout+4  addrout[1] aout1
        addrout+8  addrout[2] aout2
        addrout+12 addrout[4] aout3
*/
unsigned long
XA_objc_data(enum locks lock, struct xa_client *client, AESPB *pb)
{
	short what = pb->intin[0] & ~0x8000;
	short obj = pb->intin[1];
	short ret0 = 0, ret1 = 0;
	bool set = pb->intin[0] & 0x8000;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];

// 	display("XA_objc_data: obtree=%lx, obj=%d, what=%x", pb->addrin[0], pb->intin[1], pb->intin[0]);
	if (validate_obtree(client, obtree, "XA_objc_data:"))
	{
		struct widget_tree *wt;
		struct xa_window *wind;
		struct xa_rect_list *rl;
		RECT *clip;
		struct xa_vdi_settings *v;
		short obt = obtree[obj].ob_type & 0xff;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);
		
		assert(wt);
		
		if (set) {
			clip = (RECT *)pb->addrin[1];
			wind = pb->intin[3] != -1 ? get_wind_by_handle(lock, pb->intin[3]) : NULL;
			if (wind) {
				rl = wind->rect_list.start;
				v = wind->vdi_settings;
			} else {
				rl = NULL;
				v = client->vdi_settings;
			}
		} else {
			clip = NULL;
			wind = NULL;
			rl = NULL;
			v = client->vdi_settings;
		}

		switch (what) {
			case OBGET_OBTYPE: {
/*
objc_data(void *handle, short obj_idx, short request, short *type);
objc_get_obtype(void *handle, short obj_idx, short *type);
        control      control[0]   140        # Opcode
        control+2    control[1]   2          # Entries in intin
        control+4    control[2]   2          # Entries in intout
        control+6    control[3]   1          # Entries in addrin 
        control+8    control[4]   0          # entries in addrout 
       
	intin        intin[0]    request
        intin+2      intin[1]    obj_idx

        addrin      addrin[0]    handle
 Output:
        intout       intout[0]  Return value (1 = OK, 0 = error)
        intout+2     intout[1]  type

objc_data(void *handle, short obj_idx, short request, short newtype, short *oldtype, short windowhandle, grect *clip);
objc_set_obtype(void *handle, short obj_idx, short newtype, short *oldtype, short windowhandle, grect *clip);
        control      control[0]   140        # Opcode
        control+2    control[1]   5          # Entries in intin
        control+4    control[2]   2          # Entries in intout
        control+6    control[3]   2          # Entries in addrin 
        control+8    control[4]   0          # entries in addrout 
       
	intin        intin[0]    request
        intin+2      intin[1]    obj_idx
        intin+4      intin[2]    flags
	intin+6      intin[3]    windowhandle
        intin+8      intin[4]    newtype

        addrin      addrin[0]    handle
        addrin+4    addrin[1]    clip
 Output:
        intout      intout[0]   Return value (1 = OK, 0 = error)
        intout+2    intout[1]   oldtype
*/

// 				display(" OBGET_OBTYPE:");
				pb->intout[1] = obtree[obj].ob_type;
				if (set) {
					obtree[obj].ob_type = pb->intin[4];
					if (clip || rl)
						obj_draw(wt, v, aesobj(wt->tree, obj), -1, clip, rl, 0);
				}
				ret0 = 1;
				break;
			}
			case OBGET_STRING: {
/*
objc_data(handle, obj_idx, request, flags, string, windowhandle, clip);
objc_set_obstring(handle, obj_idx, flags, string, windowhandle, clip);
        control    control[0]   140        # Opcode
        control+2  control[1]   4          # Entries in intin
        control+4  control[2]   1          # Entries in intout
        control+6  control[3]   3          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
        intin+4    intin[2]    flags
	intin+6    intin[3]    windowhandle

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip rect ptr
        addrin+8   addrin[2]   string
 Output:
        intout     intout[0]  Return value (1 = OK, 0 = error)

objc_data(handle, obj_idx, request, buf, buflen);
objc_get_obstring(handle, obj_idx, buf, buflen);
        control    control[0]   140        # Opcode
        control+2  control[1]   3          # Entries in intin
        control+4  control[2]   1          # Entries in intout
        control+6  control[3]   2          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
	intin+4    intin[2]    buflen

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   buf
 Output:
        intout     intout[0]  Return value (1 = OK, 0 = error)

*/
			/*
			 * ozk: REMEMBER to CHANGE to object_get_string() usage!!!
			 */
				short blen;
				short slen, sl;
				char *s, *d = set ? (char *)pb->addrin[2] : (char *)pb->addrin[1];
				struct xa_aes_object object;
				
// 				display(" OB%s_STRING:", set ? "SET":"GET");
				object = aesobj(obtree, obj);
				
				if (aesobj_has_tedinfo(&object)) {
					TEDINFO *ted;
					XTEDINFO *xted;
					
					ted = aesobj_get_tedinfo(&object, &xted);
// 					display(" -- TED = %lx, XTED = %lx", ted, xted);
// 					display(" -- d = %lx, '%s'", d, d);
					if (ted && d) {
						sl = strlen(ted->te_ptext);
						if (set) {
							obj_edit(wt, v, ED_STRING, object, 0, 0, d, (rl || clip) ? true : false, clip,rl, NULL,NULL);
						} else {
							blen = pb->intin[2] - 1;
							slen = sl;
							s = ted->te_ptext;
							if (blen != -2 && slen > blen)
								slen = blen;
							strncpy(d, s, slen);
							d[slen] = '\0';
							sl = strlen(d);
						}
						ret0 = 1;
					}
				} else if (aesobj_has_freestr(&object)) {
					s = aesobj_get_freestr(&object);
// 					display(" -- obj has freestr at %lx(%s)", s, s);
// 					display(" -- d = %lx(%s)", d, d);
					if (s && d)
					{
						sl = strlen(s);
						if (set) {
							strcpy(s, d);
							if (rl || clip)
								obj_draw(wt, v, aesobj(wt->tree, obj), -1, clip, rl, 0);
						} else {
							blen = pb->intin[2] - 1;
							if (blen == -2)
								blen = sl;
							strncpy(d, s, blen);
							d[blen] = '\0';
						}
						ret0 = 1;
					}
				} else {
					switch (obt) {
						case G_CICON:
						case G_ICON: {
							s = aesobj_get_spec(&object)->iconblk->ib_ptext;
							sl = strlen(s);
							if (set) {
								strcpy(s, d);
								if (rl || clip)
									obj_draw(wt, v, aesobj(wt->tree, obj), -1, clip, rl, 0);
							} else {
								if (pb->intin[2] == -1)
									strcpy(d, s);
								else
									strncpy(d,s,pb->intin[2]);
							}
							ret0 = 1;
							break;
						}
						default:;
					}
				}
				break;
			}
			case OBGET_OBSPEC: {
/*
objc_data(handle, obj_idx, request, flags, obspec, windowhandle, clip);
objc_set_obspec(handle, obj_idx, obspec, windowhandle, clip);
        control    control[0]   140        # Opcode
        control+2  control[1]   6          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   2          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
        intin+4    intin[2]    flags
	intin+6    intin[3]    windowhandle
        intin+8    intin[4]    High word of new obspec
        intin+10   intin[5]    Low  word of new obspec

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip rect ptr
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   High word of returned obspec
        intout+4   intout[2]   low  word of returned obspec

objc_data(handle, obj_idx, request, obspec);
objc_get_obspec(handle, obj_idx, obspec);
        control    control[0]   140        # Opcode
        control+2  control[1]   2          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   1          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx

        addrin     addrin[0]   handle
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   High word of returned obspec
        intout+4   intout[2]   low  word of returned obspec

*/
				unsigned long tmp;
// 				display(" OBGET_OBSPEC:");

				if (obt == G_USERDEF) {
					USERBLK *ub = object_get_spec(obtree + obj)->userblk;
					tmp = ub->ub_parm;
					if (set)
						ub->ub_parm = (unsigned long)pb->intin[4] << 16 | pb->intin[5];
				} else {
					tmp = object_get_spec(obtree + obj)->index;
					if (set)
						object_set_spec(obtree + obj, (unsigned long)pb->intin[4] << 16 | pb->intin[5]);
				}
				pb->intout[1] = tmp >> 16;
				pb->intout[2] = (short)tmp;
				ret0 = 1;
				break;
			}
			case OBGET_BFOBSPEC: {
/*
objc_data(handle, obj_idx, request, flags, newbfobspec, oldbfobspec, windowhandle, clip);
objc_set_bfobspec(handle, obj_idx, flags, newbfobspec, oldbfobspec, windowhandle, clip);
        control    control[0]   140        # Opcode
        control+2  control[1]   6          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   2          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
        intin+4    intin[2]    flags
	intin+6    intin[3]    windowhandle
        intin+8    intin[4]    High word of newbfobspec
        intin+10   intin[5]    Low  word of newbfobspec

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip rect ptr
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   High word of oldbfobspec
        intout+4   intout[2]   Low  word of oldbfobspec

objc_data(handle, obj_idx, request, bfobspec);
objc_get_bfobspec(handle, obj_idx, bfobspec);
        control    control[0]   140        # Opcode
        control+2  control[1]   2          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   1          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx

        addrin     addrin[0]   handle
 Output:
        intout     intout[0]  Return value (1 = OK, 0 = error)
        intout+2   intout[1]   High word of returned bfobspec
        intout+4   intout[2]   Low  word of returned bfobspec


*/
				BFOBSPEC tmp;
				union { short s[2]; unsigned long l; BFOBSPEC o; } o;
// 				display(" OBGET_BFOBSPEC:");
				tmp = object_get_spec(obtree + obj)->obspec;
				if (set) {
					o.s[0] = pb->intin[4];
					o.s[1] = pb->intin[5];
					object_set_spec(obtree + obj, o.l);
				}
				o.o = tmp;
				pb->intout[1] = o.s[0];
				pb->intout[2] = o.s[1];
				ret0 = 1;
				break;
			}
			case OBGET_OBFLAGS: {
/*
objc_data(handle, obj_idx, request, flags, obmask, obflags, oldobflags, windowhandle, clip);
objc_set_obflags(handle, obj_idx, flags, obmask, obflags, oldobflags, windowhandle, clip);
        control    control[0]   140        # Opcode
        control+2  control[1]   6          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   2          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
        intin+4    intin[2]    flags
	intin+6    intin[3]    windowhandle
        intin+8    intin[4]    obmask
        intin+10   intin[5]    obflags

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip rect ptr
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   Resulting obflags

objc_data(handle, obj_idx, request, obflags);
objc_get_obflags(handle, obj_idx, obflags);
        control    control[0]   140        # Opcode
        control+2  control[1]   2          # Entries in intin
        control+4  control[2]   2          # Entries in intout
        control+6  control[3]   1          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx

        addrin     addrin[0]   handle
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   Return obflags


*/
// 				display(" OBGET_OBFLAGS:");
				ret1 = obtree[obj].ob_flags;
				if (set) {
					short f = (ret1 & ~pb->intin[4]) | pb->intin[5];
					if (ret1 != f) {
						obtree[obj].ob_flags = ret1 = f;
						if (clip || rl)
							obj_draw(wt, v, aesobj(obtree, obj), -1, clip, rl, 0);
					}
				}
				ret0 = 1;
				pb->intout[1] = ret1;
				break;
			}
			case OBGET_OBSTATE: {
/*
objc_data(handle, obj_idx, request, flags, obmask, obstate, oldobstate, windowhandle, clip);
objc_set_obflags(handle, obj_idx, flags, obmask, obstate, oldobstate, windowhandle, clip);
        control    control[0]   140        # Opcode
        control+2  control[1]   6          # Entries in intin
        control+4  control[2]   3          # Entries in intout
        control+6  control[3]   2          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx
        intin+4    intin[2]    flags
	intin+6    intin[3]    windowhandle
        intin+8    intin[4]    obmask
        intin+10   intin[5]    obstate

        addrin     addrin[0]   handle
        addrin+4   addrin[1]   clip rect ptr
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   Resulting obstate

objc_data(handle, obj_idx, request, obstate);
objc_get_obflags(handle, obj_idx, obstate);
        control    control[0]   140        # Opcode
        control+2  control[1]   2          # Entries in intin
        control+4  control[2]   2          # Entries in intout
        control+6  control[3]   1          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx

        addrin     addrin[0]   handle
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   Return obstate


*/
// 				display(" OBGET_OBSTATE:");
				ret1 = obtree[obj].ob_state;
				if (set) {
					short s = (ret1 & ~pb->intin[4]) | pb->intin[5];
					if (ret1 != s) {
						obtree[obj].ob_state = ret1 = s;
						if (clip || rl)
							obj_draw(wt, v, aesobj(obtree, obj), -1, clip, rl, 0);
					}
				}
				ret0 = 1;
				pb->intout[1] = ret1;
				break;
			}
			case OBGET_AREA: {
/*
objc_data(void *handle, short obj_idx, short request, grect *rect);
objc_get_obarea(void *handle, short obj_idx, grect *obstate);
        control    control[0]   140        # Opcode
        control+2  control[1]   2          # Entries in intin
        control+4  control[2]   5          # Entries in intout
        control+6  control[3]   1          # Entries in addrin 
        control+8  control[4]   0          # entries in addrout 
       
	intin      intin[0]    request
        intin+2    intin[1]    obj_idx

        addrin     addrin[0]   handle
 Output:
        intout     intout[0]   Return value (1 = OK, 0 = error)
        intout+2   intout[1]   x
        intout+2   intout[1]   y
        intout+2   intout[1]   w
        intout+2   intout[1]   h

*/
				if (!set) {
					obj_area(wt, aesobj(obtree, obj), (RECT *)(pb->intout + 1));
					ret0 = 1;
				}
				break;
			}
			default:;
		}
		/* more cases here */
	}

	pb->intout[0] = ret0;
	
	return XAC_DONE;
}
