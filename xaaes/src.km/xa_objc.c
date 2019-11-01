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
XA_objc_draw(int lock, struct xa_client *client, AESPB *pb)
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
		/* if form_do object-cursor off */
		if (client->fmd.state & 1)
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
XA_objc_wdraw(int lock, struct xa_client *client, AESPB *pb)
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
XA_objc_offset(int lock, struct xa_client *client, AESPB *pb)
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
XA_objc_find(int lock, struct xa_client *client, AESPB *pb)
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

#if 0
 		if (wt)
#endif
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
XA_objc_change(int lock, struct xa_client *client, AESPB *pb)
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
XA_objc_wchange(int lock, struct xa_client *client, AESPB *pb)
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
XA_objc_add(int lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];

	CONTROL(2,1,1)

	DIAG((D_form, client, "xa_objc_add: obtree=%lx, parent=%d, child=%d",
		(unsigned long)obtree, pb->intin[0], pb->intin[1]));

	if (validate_obtree(client, obtree, "XA_objc_add:"))
	{
		ret = ob_add(obtree, pb->intin[0], pb->intin[1]);
	}
	pb->intout[0] = ret;

	DIAGS((" -- return %d", pb->intout[0]));

	return XAC_DONE;
}

unsigned long
XA_objc_delete(int lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];

	CONTROL(1,1,1)

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
XA_objc_order(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(2,1,1)

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
XA_objc_edit(int lock, struct xa_client *client, AESPB *pb)
{
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(4,2,1)

	DIAG((D_form, client, "objc_edit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

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

	return XAC_DONE;
}

unsigned long
XA_objc_wedit(int lock, struct xa_client *client, AESPB *pb)
{
	short ret = 0;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];
	CONTROL(5,2,1)

	DIAG((D_form, client, "objc_edit (%d %d %d %d)",
		pb->intin[0], pb->intin[1], pb->intin[2], pb->intin[3]));

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
XA_objc_sysvar(int lock, struct xa_client *client, AESPB *pb)
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

/*
 * opcode 140
 * objc_data(tree, object, what, size, wh, (void *)data0, (void *)data1, *short, *long);
 * objc_data(tree, obj_idx, what, flags, winhand, clip, ...);
 * intin[0] = obj_idx
 * intin[1] = what
 * intin[2] = flags
 * intin[3] = winhand
 *
 * addrin[0] = tree
 * addrin[1] = clip RECT ptr
 *
 * intout[0] - 1 = OK, 0 = error
 *
 * addrout[0] - mode dependant
 */
unsigned long
XA_objc_data(int lock, struct xa_client *client, AESPB *pb)
{
	short what = pb->intin[1] & ~0x8000;
	short obj = pb->intin[0];
	short ret0 = 0, ret1 = 0;
	bool set = pb->intin[1] & 0x8000;
	OBJECT *obtree = (OBJECT *)pb->addrin[0];

	if (validate_obtree(client, obtree, "XA_objc_data:"))
	{
		struct widget_tree *wt;
		struct xa_window *wind = NULL;
		struct xa_rect_list *rl = NULL;
		RECT *clip = NULL;
		struct xa_vdi_settings *v;
		short obt = obtree[obj].ob_type & 0xff;

		if (!(wt = obtree_to_wt(client, obtree)))
			wt = new_widget_tree(client, obtree);

		assert(wt);

		if (set)
		{
			clip = (RECT *)pb->addrin[1];
			wind = get_wind_by_handle(lock, pb->intin[3]);
		}

		if (wind)
		{
			rl = wind->rect_list.start;
			v = wind->vdi_settings;
		}
		else
			v = client->vdi_settings;

		switch (what)
		{
			case OBGET_OBTYPE:
			{
				/* objc_get_type(tree, obj_idx, what, &type); */
				pb->intout[1] = obtree[obj].ob_type;
				if (set)
					obtree[obj].ob_type = pb->intin[4];
				ret0 = 1;
				break;
			}
			case OBGET_STRING:
			{
				/*
				 * objc_data(tree, obj_idx, what, flags, winhand, clip, ...);
				 * objc_[s/g]et_string(tree, obj_idx, wh, blen, text, clip);
				 * intin[0] = obj_idx
				 * intin[1] = what
				 * intin[2] = flags
				 * intin[3] = winhand
				 * intin[4] = blen
				 *
				 * addrin[0] = tree
				 * addrin[1] = clip RECT ptr
				 * addrin[2] = strbuf

				 * intout[0] 1 = sucess - 0 = error
				 * intout[1] = strlen when mode == get
				 */
			/*
			 * ozk: REMEMBER to CHANGE to object_get_string() usage!!!
			 */
				short blen;
				short slen, sl;
				char *d = (char *)pb->addrin[2];
				char *s;
				struct xa_aes_object object;

				object = aesobj(obtree, obj);

				if (object_has_tedinfo(obtree + obj))
				{
					TEDINFO *ted;
					XTEDINFO *xted;

					ted = object_get_tedinfo(obtree + obj, &xted);
					if (ted && d)
					{
						sl = strlen(ted->te_ptext) + 1;
						if (set)
						{
							obj_edit(wt, v, ED_STRING, object, 0, 0, d, rl ? true : false, NULL,rl, NULL,NULL);
						}
						else
						{
							blen = pb->intin[4];
							slen = sl;
							s = ted->te_ptext;
							if (blen != -1 && slen > blen)
								slen = blen;
							strncpy(d, s, slen);
							d[slen] = '\0';
							sl = strlen(d);
						}
						ret0 = 1;
					}
				}
				else if (object_has_freestr(obtree + obj))
				{
					s = object_get_freestr(obtree + obj);
					if (s && d)
					{
						sl = strlen(s) + 1;
						if (set)
						{
							strcpy(s, d);
							if (rl)
								obj_draw(wt, v, aesobj(wt->tree, obj), -1, clip, rl, 0);
						}
						else
						{
							blen = pb->intin[4];
							if (blen == -1)
								blen = sl;
							strncpy(d, s, blen);
							d[blen - 1] = '\0';
						}
						ret0 = 1;
					}
				}
				else
				{
					switch (obt)
					{
						case G_CICON:
						case G_ICON:
						{
							s = object_get_spec(obtree + obj)->iconblk->ib_ptext;
							sl = strlen(s) + 1;
							if (set)
							{
								strcpy(s, d);
								if (rl)
									obj_draw(wt, v, aesobj(wt->tree, obj), -1, NULL, rl, 0);
							}
							else
								strcpy(d, s);
							ret0 = 1;
							break;
						}
						default:;
					}
				}
				break;
			}
			case OBGET_OBSPEC:
			{
				long *obs = (long *)pb->addrin[2];
				/*
				 * objc_get_spec(handle, obj_idx, wh, &spec, clip);
				 * intin[0] = obj_idx
				 * intin[1] = what
				 * intin[2] = flags
				 * intin[3] = winhand
				 *
				 * addrin[0] = tree
				 * addrin[1] = clip RECT ptr
				 * addrin[2] = obspec
				 *
				 * intout[0] 1 = sucess - 0 = error
				 * intout[1]
				 */
				if (obs)
				{
					long tmp;
					if (obt == G_USERDEF)
					{
						USERBLK *ub = object_get_spec(obtree + obj)->userblk;
						tmp = ub->ub_parm;
						if (set)
							ub->ub_parm = *obs;
					}
					else
					{
						tmp = object_get_spec(obtree + obj)->index;
						if (set)
							object_set_spec(obtree + obj, *obs);
					}
					*obs = tmp;
					ret0 = 1;
				}

				break;
			}
			case OBGET_BFOBSPEC:
			{
				BFOBSPEC tmp;
				BFOBSPEC *o  = (BFOBSPEC *)pb->addrin[2];
				/*
				 * objc_get_bfobspec(handle, obj_idx, wh, clip, &bfobspec);
				 * intin[0] = obj_idx
				 * intin[1] = what
				 * intin[2] = flags
				 * intin[3] = winhand
				 *
				 * addrin[0] = tree
				 * addrin[1] = clip RECT ptr
				 * addrin[2] = obspec ptr
				 *
				 * intout[0] 1 = sucess - 0 = error
				 */

				if (o)
				{
					tmp = object_get_spec(obtree + obj)->obspec;
					if (set)
						object_set_spec(obtree + obj, *(unsigned long *)o);
					*o = tmp;
					ret0 = 1;
				}
				break;
			}
			case OBGET_OBFLAGS:
			{
				/*
				 * objc_get_obflags(handle, obj_idx, wh, clip, flags);
				 * objc_set_obflags(handle, obj_idx, wh, clip, flags);
				 * intin[0] = obj_idx
				 * intin[1] = what
				 * intin[2] = flags
				 * intin[3] = winhand
				 * intin[4] = obflags
				 *
				 * addrin[0] = tree
				 * addrin[1] = clip RECT ptr
				 *
				 * intout[0] 1 = sucess - 0 = error
				 * intout[1] = obflags
				 */
				ret1 = obtree[obj].ob_flags;
				if (set)
				{
					short f = (ret1 & ~pb->intin[4]) | pb->intin[5];
					if (ret1 != f)
					{
						obtree[obj].ob_flags = f;
						if (clip || rl)
							obj_draw(wt, v, aesobj(obtree, obj), -1, clip, rl, 0);
					}
				}
				else
				{
					ret1 = obtree[obj].ob_flags;
				}
				ret0 = 1;
				pb->intout[1] = ret1;
				break;
			}
			case OBGET_OBSTATE:
			{
				/*
				 * objc_get_obstate(handle, obj_idx, wh, clip, flags);
				 * objc_set_obstate(handle, obj_idx, wh, clip, flags);
				 *
				 * intin[0] = obj_idx
				 * intin[1] = what
				 * intin[2] = flags
				 * intin[3] = winhand
				 * intin[4] = obstate
				 *
				 * addrin[0] = tree
				 * addrin[1] = clip RECT ptr
				 *
				 * intout[0] 1 = sucess - 0 = error
				 * intout[1] = obstate
				 */
				ret1 = obtree[obj].ob_state;
				if (set)
				{
					short s = (obtree[obj].ob_state & ~pb->intin[4]) | pb->intin[5];
					if (ret1 != s)
					{
						obtree[obj].ob_state = s;
						if (clip || rl)
							obj_draw(wt, v, aesobj(obtree, obj), -1, clip, rl, 0);
					}
				}
				else
				{
					ret1 = obtree[obj].ob_state;
				}
				ret0 = 1;
				pb->intout[1] = ret1;
				break;
			}
			case OBGET_AREA:
			{
				if (!set)
				{
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
