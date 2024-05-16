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

#include "xa_types.h"
#include "xa_global.h"
#include "xa_menu.h"

#include "xaaes.h"

#include "app_man.h"
#include "form.h"
#include "app_man.h"
#include "draw_obj.h"
#include "obtree.h"
#include "rectlist.h"
#include "scrlobjc.h"
#include "widgets.h"
#include "c_window.h"
#include "k_mouse.h"
#include "menuwidg.h"
#include "keycodes.h"
#include "k_keybd.h"

static FormMouseInput	Click_form_do;
/*
 * Attatch a MODAL form_do session to a client.
 * client->wt (XA_TREE) is used for modal form_do.
 * wind->toolbar (XA_TREE) is used for nonmodal (windowed) form_do.
 */
static void
Set_form_do(struct xa_client *client,
	    OBJECT *obtree,
	    struct xa_aes_object edobj,
	    bool redraw)
{
	struct xa_aes_object new_obj;
	XA_TREE *wt;
	struct objc_edit_info *ei;

	wt = obtree_to_wt(client, obtree);

	if (!wt)
	{
		wt = new_widget_tree(client, obtree);
	}
	DIAG((D_form, client, "Set_form_do: wt=%lx, obtree=%lx, edobj=%d for %s",
		(unsigned long)wt, (unsigned long)obtree, edobj.item, client->name));


	/* XXX - We should perhaps check if the form really have an EXIT or
	 *       TOUCHEXIT button, and act accordingly.
	 */
	wt->exit_form	= Exit_form_do; //Classic_exit_form_do;

	/* Ozk:
	 * If the first obj_edit call fails, we call it again passing the
	 * object it returned, which obj_edit() looked up. See obj_edit()
	 */

	if (edobj.item == -2)
	{
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED/*, 0, 0*/);
	}

	if (!obj_edit(wt, client->vdi_settings, ED_INIT, edobj, 0, -1, NULL, redraw, NULL, NULL, NULL, &new_obj))
	{
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED/*, 0, 0*/);
		obj_edit(wt, client->vdi_settings, ED_INIT, edobj, 0, -1, NULL, redraw, NULL, NULL, NULL, NULL);
	}

	ei = wt->ei ? wt->ei : &wt->e;
	/* Ozk:
	 * Check if this obtree needs a keypress handler..
	 */
	if (edit_set(ei) || obtree_has_default(obtree) )
		client->fmd.keypress = Key_form_do;

	/*
	 * Install mouse button handler.
	 */
	client->fmd.mousepress = Click_form_do;
	client->fmd.wt = wt;
}

static struct xa_window *
create_fmd_wind(int lock, struct xa_client *client, XA_WIND_ATTR kind, WINDOW_TYPE dial, const GRECT *r)
{
	struct xa_window *wind = NULL;

	bool nolist = false;

	DIAGS(("create_fmd_wind: create window"));
#if 0
	if (C.update_lock && C.update_lock == client->p)
	{
		kind |= STORE_BACK;
		nolist = true;
	}
#endif
	kind |= (TOOLBAR | USE_MAX | BACKDROP);
	wind = create_window(lock,
			     do_winmesag,
			     do_formwind_msg,
			     client,
			     nolist,
			     kind,
			     dial,
			     0, /* frame size */
			     C.Aes->options.thinwork,
			     r,
			     NULL,
			     NULL);

	return wind;
}

static void
calc_fmd_wind(struct widget_tree *wt, XA_WIND_ATTR kind, WINDOW_TYPE dial, GRECT *r)
{
	DIAG((D_form, wt->owner, "calc_fmd_wind: Create window for %s", wt->owner->name));

	obj_area(wt, aesobj(wt->tree, 0), r);


	*r = calc_window(0,
			 wt->owner,
			 WC_BORDER,
			 kind, dial,
			 0,
			 C.Aes->options.thinwork,
			 r);

}

/*
 * XXX - Ozk:
 *     This call should never fail, so callers of this function
 *     must take that into account .. especially XA_form_do()
 */
bool
Setup_form_do(struct xa_client *client,
	      OBJECT *obtree,
	      struct xa_aes_object edobj,
	      /* Output */
	      struct xa_window **ret_wind,
	      struct xa_aes_object *ret_edobj)
{
	int lock = 0;
	XA_WIND_ATTR kind = NAME;
	XA_TREE *wt = NULL;
	struct xa_window *wind = NULL;
	struct objc_edit_info *ei;

	/*
	 * Window for form/dialogs already created by form_dial()?
	 */
	if (client->fmd.wind)
	{
		DIAG((D_form, client, "Setup_form_do: wind %d for %s", client->fmd.wind->handle, client->name));
		wind = client->fmd.wind;
	 	wt = get_widget(wind, XAW_TOOLBAR)->stuff;

	 	if( wt->tree == obtree )
	 	{
	 		goto okexit;	/* already installed */
	 	}
		wt = obtree_to_wt(client, obtree);
		if (!wt)
			wt = new_widget_tree(client, obtree);
		assert(wt);

		calc_fmd_wind(wt, kind, wind->dial, &client->fmd.r);
		wt = set_toolbar_widget(lock, wind, client, obtree, edobj, WIP_NOTEXT, 0, NULL, NULL);
		move_window(lock, wind, true, -1, client->fmd.r.g_x, client->fmd.r.g_y, client->fmd.r.g_w, client->fmd.r.g_h);
	}
	/*
	 * Should this client do classic blocking form_do's?
	 */
	else if (C.update_lock == client->p)
	{
		DIAG((D_form, client, "Setup_form_do: nonwindowed for %s", client->name));
// 		if (d) display("Setup_form_do: nonwindowed for %s", client->name);

		Set_form_do(client, obtree, edobj, true);
		wt = client->fmd.wt;
		goto okexit;
	}
	/*
	 * First time this client does a form_do/dial, do some preps
	 */
	else
	{
		wt = obtree_to_wt(client, obtree);
		if (!wt)
		{
			wt = new_widget_tree(client, obtree);
		}
		assert(wt);
		calc_fmd_wind(wt, kind, client->fmd.state ? created_for_FMD_START : created_for_FORM_DO, &client->fmd.r);

		if (!client->options.xa_nomove)
		{
			kind |= MOVER;//|STORE_BACK;
		}
		client->fmd.kind = kind;
	}

	if( valid_aesobj( &wt->focus ) && edobj.item != wt->focus.item )
	{
		obj_draw(wt, client->vdi_settings, wt->focus, 0, NULL, NULL, UNDRAW_FOCUS);
	}
	wt->focus = edobj;
	/*
	 * If we get here and have no window pointer, this is the first time
	 * the client uses form_do.
	 *
	 */
	if (!wind)
	{
		if ((wind = create_fmd_wind(lock, client, kind, client->fmd.state ? created_for_FMD_START : created_for_FORM_DO, &client->fmd.r)))
		{
			client->fmd.wind = wind;
			wt = set_toolbar_widget(lock, wind, client, obtree, edobj, WIP_NOTEXT, 0, NULL, NULL);
		}
		else
		{
			client->fmd.wind = NULL;
			return false;
		}
	}
okexit:
	ei = wt->ei ? wt->ei : &wt->e;
	if( !valid_aesobj( &ei->o ) )
	{
		ei->o = edobj;
	}
	else
		edobj = ei->o;
#if 0
	if( !valid_aesobj(&wt->focus)/*aesobj_edit( &edobj )*/ )
	{
		wt->focus = edobj;
	}
#endif
	/* first draw edit-cursor */
	if( valid_aesobj( &edobj ) )
	{
		/* draw 1st cursor */
		wt->focus = edobj;
		obj_draw(wt, client->vdi_settings, edobj, 0, NULL, NULL, DRW_CURSOR);
	}
#if 0
	/* if edit-object != focus draw focus-cursor */
	else if( !same_aesobj( &wt->focus, &edobj ) )
	{
		/* draw 1st cursor */
		obj_draw(wt, client->vdi_settings, wt->focus, 0, NULL, NULL, DRW_CURSOR);
	}
#endif
	DIAGS(("Setup_form_do: returning - edobj=%d, wind %lx", edit_item(ei), (unsigned long)wind));

	if (ret_edobj)
		*ret_edobj = editfocus(ei); //edit_item(ei);
	if (ret_wind)
		*ret_wind = wind;

	return true;
}

void
Form_Center(OBJECT *form, short barsizes)
{
	form->ob_x = root_window->wa.g_x + (root_window->wa.g_w - form->ob_width) / 2;
	form->ob_y = root_window->wa.g_y + barsizes + (root_window->wa.g_h - form->ob_height) / 2;
}

#if INCLUDE_UNUSED
void
Form_Center_r(OBJECT *form, short barsizes, GRECT *r)
{
	r->g_x = root_window->wa.g_x + (root_window->wa.g_w - form->ob_width) / 2;
	r->g_y = root_window->wa.g_y + barsizes + (root_window->wa.g_h - form->ob_height) / 2;
	r->g_w = form->ob_width;
	r->g_h = form->ob_height;
}
#endif

void
center_rect(GRECT *r)
{
	struct xa_widget *widg = get_menu_widg();

	r->g_x = root_window->wa.g_x + ((root_window->wa.g_w - r->g_w) >> 1);
	r->g_y = root_window->wa.g_y + widg->ar.g_h + ((root_window->wa.g_h - r->g_h) >> 1);
}

/*
 * Ozk: Rewritten totally. Returns false if a TOUCHEXIT or EXIT button was selected.
 * puts a 0 in 'nxtob' if the object is not an [TOUCH]EXIT or EDITABLE. If object is
 * any of the two, its passed back in 'nxtob'.
 */
bool
Form_Button(XA_TREE *wt,
	    struct xa_vdi_settings *v,
	    struct xa_aes_object obj,
	    const struct moose_data *md,
	    unsigned long fbflags,
	    struct xa_rect_list **rl,
	    /* Outputs */
	    short *newstate,
	    struct xa_aes_object *nxtob,
	    short *clickmsk)
{
	GRECT *clip = NULL;
	OBJECT *obtree = wt->tree;
	struct  xa_aes_object next_obj = aesobj(wt->tree, 0);
	short flags, state, pstate;
	struct xa_rect_list *lrl = NULL;
	bool no_exit = true, dc, redraw = (fbflags & FBF_REDRAW);

	if (!rl)
		rl = &lrl;

	DIAG((D_form, NULL, "Form_Button: wt=%lx, obtree=%lx, obj=%d",
		(unsigned long)wt, (unsigned long)wt->tree, aesobj_item(&obj)));

// 	display("Form_Button: wt=%lx, obtree=%lx, obj=%d",
// 		wt, wt->tree, obj);

	flags = aesobj_flags(&obj);
	state = pstate = aesobj_state(&obj);
	dc = md->clicks > 1 ? true : false;


	/* find_object can't report click on a OF_HIDETREE object. */
	/* HR: Unfortunately it could. Fixed that. */

	if (flags & OF_TOUCHEXIT)
	{
		no_exit = false;
	}

	if ( ((flags & (OF_SELECTABLE|OF_EDITABLE)) || (fbflags & FBF_CHANGE_FOCUS)) && !(state & OS_DISABLED) )
	{
		short type = aesobj_type(&obj) & 0xff;

		if (type == G_SLIST)
		{
			if ((wt->flags & WTF_FBDO_SLIST) || (fbflags & FBF_DO_SLIST))
			{
				click_scroll_list(0, obtree, aesobj_item(&obj), md);
				no_exit = true;
			}
			else
				no_exit = false;
		}
		else
		{
			if (type == G_POPUP)
			{
				XAMENU xmn;
				XAMENU_RESULT result;
				POPINFO *pinf;
				short x, y, obnum;

				{
					pinf = object_get_popinfo(aesobj_ob(&obj));
					if ((obnum = pinf->obnum) > 0)
						pinf->tree[obnum].ob_state |= OS_CHECKED;
					else
						obnum = 0;

					xmn.menu.mn_tree = pinf->tree;
					xmn.menu.mn_menu = ROOT;
					xmn.menu.mn_item = obnum;
					xmn.menu.mn_scroll = 1;
					xmn.mn_selected = (fbflags & FBF_KEYBD) ? ((obnum > 0) ? obnum : -2) : -1;

					obj_offset(wt, obj, &x, &y);
					if (menu_popup(0, wt->owner, &xmn, &result, x, y, 2) && result.menu.mn_tree == xmn.menu.mn_tree)
					{
						if (result.menu.mn_item > 0)
						{
							no_exit = false;
							pinf->obnum = result.menu.mn_item;
						}
					}
					if (obnum > 0)
						pinf->tree[obnum].ob_state &= ~OS_CHECKED;

					obj_change(wt, v, obj, -1, state & ~OS_SELECTED, flags, true, clip, *rl, 0);

					/*
					 * Ozk: Dont know about this...
					 */
	// 				if (pinf->obnum != obnum)
	// 					no_exit = false;
				}
			}
			else if (type == G_SWBUTTON)
			{
				SWINFO *sinf;
				DIAGS(("Form_Button: g_swbutton"));

				if (redraw /*&& !(fbflags & FBF_KEYBD) && !(flags & OF_TOUCHEXIT)*/)
				{
					obj_watch(wt, v, obj, state^OS_SELECTED, state, clip, *rl);
					if (aesobj_sel(&obj))
					{
						sinf = object_get_swinfo(aesobj_ob(&obj));
						if (!(fbflags & FBF_KEYBD)) /* mouse click/double-click */
						{
							if (dc || (md->state & MBS_RIGHT))
								sinf->num--;
							else
								sinf->num++;
						}
						else /* keyboard */
						{
							sinf->num++;
						}

						if (sinf->num < 0)
							sinf->num = sinf->maxnum;
						if (sinf->num > sinf->maxnum)
							sinf->num = 0;
						obj_change(wt, v, obj, -1, state & ~OS_SELECTED & ~OS_WHITEBAK & 0x80ff, flags, redraw, clip, *rl, 0);
					}
				}
			}
			else if (flags & OF_RBUTTON)
			{
				DIAGS(("Form_Button: call obj_set_radio_button"));
				if (redraw && !(fbflags & FBF_KEYBD) && !(flags & OF_TOUCHEXIT))
				{
					if (!(state & OS_SELECTED))
					{
						obj_watch(wt, v, obj, state^OS_SELECTED, state, clip, *rl);
						if (aesobj_sel(&obj))
							obj_set_radio_button(wt, v, obj, redraw, clip, *rl);
					}
					else
					{
						if (obj_watch(wt, v, obj, state, state, clip, *rl))
							pstate &= ~OS_SELECTED;
					}
				}
				else
					obj_set_radio_button(wt, v, obj, redraw, clip, *rl);
			}
			else if( (flags & OF_SELECTABLE) )
			{
				if ( redraw && !(fbflags & FBF_KEYBD) && !(flags & OF_TOUCHEXIT))
				{
					DIAGS(("Form_Button: call obj_watch"));
					obj_watch(wt, v, obj, state^OS_SELECTED, state, clip, *rl);
				}
				else
				{
					obj_change(wt, v, obj, -1, state^OS_SELECTED, flags, redraw, clip, *rl, 0);
				}
			}
			state = aesobj_state(&obj);
		}
		if (type != G_SLIST && ((flags & OF_EDITABLE) || ((fbflags & FBF_CHANGE_FOCUS) && (flags & (OF_EXIT|OF_TOUCHEXIT)))))
		{
			struct xa_aes_object pf = focus(wt);
			if( flags & OF_EDITABLE ){
				struct objc_edit_info *ei = (wt->ei ? wt->ei : &wt->e);
				XTEDINFO *xted = 0;
				int x = 0, y = 0;

				// cmp obtree#3405
				if( !obj_is_focus(wt, &obj) ){
					if( obj.ob->ob_spec.tedinfo ){
						TEDINFO *ted;
						char *txt, *ptxt;

						ted = object_get_tedinfo(aesobj_ob(&obj), &xted);

						if( ted )
						{
							txt = ted->te_ptext;
							ptxt = ted->te_ptmplt;

							x = strlen(txt);
							for( ; ptxt[y] && ptxt[y] != '_'; y++ );
						}
					}
					if( xted )
					{
						if( (wt->owner->status & CS_FORM_DO) )	// not windowed form_do
							wt->ei = ei = xted;
					}
					else
						ei->o = obj;
					ei->edstart = y;
					ei->pos = x;

					ei->c_state |= OB_CURS_ENABLED;

				}
			}
			if (valid_aesobj(&pf))
			{
				obj_draw(wt, v, pf, 0, NULL, *rl, UNDRAW_FOCUS|DRW_CURSOR);
			}
			wt->focus = obj;
			obj_draw(wt, v, obj, 0, NULL, *rl, DRW_CURSOR);
		}
	}	/*/if ( ((flags & (OF_SELECTABLE|OF_EDITABLE)) || (fbflags & FBF_CHANGE_FOCUS)) && !(state & OS_DISABLED) ) */

	DIAGS(("Form_Button: state %x, flags %x",
		state, flags));

	if (!(state & OS_DISABLED))
	{

		if ((state & OS_SELECTED) && !(pstate & OS_SELECTED) && (flags & OF_EXIT))
		{
			no_exit = false;
		}

		if (!no_exit || ((flags & OF_EDITABLE) && (!(flags & OF_SELECTABLE) || ((state & OS_SELECTED) != (pstate & OS_SELECTED)) )))
			next_obj = obj;
	}
	else
	{
		next_obj = aesobj(wt->tree, 0);
		no_exit = true;

	}
	if (clickmsk)
		*clickmsk = dc ? 0x8000 : 0;

	if (nxtob)
		*nxtob = next_obj;

	if (newstate)
		*newstate = state;

	DIAGS(("Form_Button: return no_exit=%s, nxtob=%d, newstate=%x, clickmask=%x",
		no_exit ? "yes":"no", aesobj_item(&next_obj), state, dc ? 0x8000:0));

// 	display("Form_Button: return no_exit=%s, nxtob=%d, newstate=%x, clickmask=%x",
// 		no_exit ? "yes":"no", next_obj, state, dc ? 0x8000:0);

	return no_exit;
}

static bool set_nxt_focus( struct xa_aes_object *nxt, struct xa_aes_object *o, struct xa_aes_object *ret_focus )
{
	if (valid_aesobj(nxt))
	{
		if (ret_focus)
			*ret_focus = *nxt;
		if ( o && aesobj_edit(nxt))
			*o = *nxt;
		return true;
	}
	return false;
}
/*
 * Form Keyboard Handler for toolbars
 *
 * Eat TAB, Cursor, Home, PgUp, PgDn (Navigation)
 */
struct xa_aes_object
Form_Cursor(XA_TREE *wt,
	    struct xa_vdi_settings *v,
	    ushort keycode,
	    ushort keystate,
	    struct xa_aes_object obj,
	    bool redraw,	// not used!
	    struct xa_rect_list **rl,
	/* output */
	    struct xa_aes_object *ret_focus,
	    unsigned short *keyout)
{
	OBJECT *obtree = wt->tree;
	short dir, kout = 0;
	short edcnt, flags;
	short last_ob;
	struct xa_rect_list *lrl = NULL;
	struct xa_aes_object nxt, o;
	struct xa_aes_object st = aesobj(obtree, 0);
	struct xa_client *client = wt->owner;

	if (!rl) rl = &lrl;

	o = obj;

	last_ob = ob_count_flag(obtree, OF_EDITABLE, 0, 0, &edcnt);
	UNUSED(last_ob);
	DIAG((D_form, NULL, "Form_Cursor: wt=%lx, obtree=%lx, obj=%d, keycode=%x, lastob=%d, editobjs=%d",
		(unsigned long)wt, (unsigned long)obtree, obj.item, keycode, last_ob, edcnt));

	if (ret_focus)
		*ret_focus = inv_aesobj();

	switch (keycode)
	{				/* The cursor keys are always eaten. */
		case SC_TAB:		/* TAB moves to next/previous field */
		{
			dir = OBFIND_HOR | ((keystate & (K_RSHIFT|K_LSHIFT)) ? OBFIND_UP : OBFIND_DOWN);

			if (!ret_focus)
			{
				flags = OF_EDITABLE;
			}
			else
				flags = OF_SELECTABLE|OF_EDITABLE|OF_EXIT|OF_TOUCHEXIT;

			nxt = ob_find_next_any_flagstate(wt, st, focus(wt), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);

			if (!valid_aesobj(&nxt))
			{
				/* if not set start-obj of the form_do-call is selected */
#ifndef NOT_WRAP_TO_START_ITEM
				if( !(keystate & (K_RSHIFT|K_LSHIFT)) && client && client->waiting_pb && client->waiting_pb->intin[0] > 0 ){
					nxt = aesobj(obtree, client->waiting_pb->intin[0] );	/* select initially selected item*/
				}
				if ( !valid_aesobj(&nxt) || !(nxt.ob->ob_flags & OF_SELECTABLE))
#endif
				{
					dir = OBFIND_VERT | (keystate & (K_RSHIFT|K_LSHIFT)) ? OBFIND_LAST : OBFIND_FIRST;
					nxt = ob_find_next_any_flagstate(wt, st, inv_aesobj(), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);
				}
			}

			set_nxt_focus( &nxt, &o, ret_focus );
			break;
		}
		case SC_INSERT:
			o = inv_aesobj();
			if( (keystate & (K_RSHIFT|K_LSHIFT)) )
			{
				obj_draw(wt, v, wt->focus, -2, NULL, *rl, UNDRAW_FOCUS);		/* remove cursor */
				wt->ei->o = wt->e.o = nxt = wt->focus = o;
			}
			else
			{
			kout = keycode;
			if( wt->focus.item < 0 || !(wt->focus.ob->ob_flags & OF_EDITABLE) )
			{
				nxt = ob_find_type( obtree, G_SLIST );
				if( set_nxt_focus( &nxt, &o, ret_focus ) )
				{
					wt->focus = nxt;
				}
			}
			}
		break;
		case SC_SHFT_UPARROW:	/* 0x4838: */ /* shift + up arrow */
		case SC_UPARROW:	/* 0x4800: */		/* UP ARROW moves to previous field */
		{
			dir = OBFIND_VERT | OBFIND_UP;

			if (!ret_focus)
			{
				flags = OF_EDITABLE;
			}
			else
				flags = OF_SELECTABLE|OF_EDITABLE|OF_EXIT|OF_TOUCHEXIT;

			nxt = ob_find_next_any_flagstate(wt, st, focus(wt), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);

			if (!valid_aesobj(&nxt))
				nxt = ob_find_next_any_flagstate(wt, st, inv_aesobj(), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_VERT|OBFIND_LAST);

			set_nxt_focus( &nxt, &o, ret_focus );
			break;
		}
		case SC_DNARROW:	/* 0x5000 */		/* Down ARROW moves to next object */
		case SC_SHFT_DNARROW:	/* 0x5032 */		/* Shift + down arrow */
		{
			dir = OBFIND_VERT | OBFIND_DOWN;

			if (!ret_focus)
			{
				flags = OF_EDITABLE;
			}
			else
				flags = OF_SELECTABLE|OF_EDITABLE|OF_EXIT|OF_TOUCHEXIT;

			nxt = ob_find_next_any_flagstate(wt, st, focus(wt), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);

			if (!valid_aesobj(&nxt))
				nxt = ob_find_next_any_flagstate(wt, st, inv_aesobj(), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_VERT|OBFIND_FIRST);

			set_nxt_focus( &nxt, &o, ret_focus );
			break;
		}
		case SC_LFARROW:	/* 0x4b00 */	/* Left ARROW */
		case SC_SHFT_LFARROW:	/* 0x4b34 */
		case SC_CTRL_LFARROW:	/* 0x7300 */
		{
			if (ret_focus && (keycode == SC_CTRL_LFARROW || !(focus_set(wt) && same_aesobj(&wt->focus, &o) && (focus_ob(wt)->ob_flags & OF_EDITABLE))) )
			{
				dir = OBFIND_HOR | OBFIND_UP;

				if (focus_set(wt) && (keystate & (K_RSHIFT|K_LSHIFT)))
				{
					flags = OF_EDITABLE;
				}
				else
					flags = OF_SELECTABLE|OF_EDITABLE|OF_EXIT|OF_TOUCHEXIT;

				nxt = ob_find_next_any_flagstate(wt, st, focus(wt), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);

				if (!valid_aesobj(&nxt))
					nxt = ob_find_next_any_flagstate(wt, st, inv_aesobj(), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_VERT|OBFIND_LAST);

				set_nxt_focus( &nxt, &o, ret_focus );
			}
			else
			{
				o = inv_aesobj();
				kout = keycode;
			}
			break;
		}
		case SC_RTARROW:	/* 0x4d00 */	/* Right ARROW */
		case SC_SHFT_RTARROW:	/* 0x4d36 */
		case SC_CTRL_RTARROW:	/* 0x7400 */
		{
			if (ret_focus && (keycode == SC_CTRL_RTARROW || !(focus_set(wt) && same_aesobj(&wt->focus, &o) && (focus_ob(wt)->ob_flags & OF_EDITABLE))) )
			{
				dir = OBFIND_HOR | OBFIND_DOWN;

				if (focus_set(wt) && (keystate & (K_RSHIFT|K_LSHIFT)))
				{
					flags = OF_EDITABLE;
				}
				else
					flags = OF_SELECTABLE|OF_EDITABLE|OF_EXIT|OF_TOUCHEXIT;

				nxt = ob_find_next_any_flagstate(wt, st, focus(wt), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, dir);

				if (!valid_aesobj(&nxt))
					nxt = ob_find_next_any_flagstate(wt, st, inv_aesobj(), flags, OF_HIDETREE, 0, OS_DISABLED, 0,0, OBFIND_VERT|OBFIND_FIRST);

				set_nxt_focus( &nxt, &o, ret_focus );
			}
			else
			{
				o = inv_aesobj();
				kout = keycode;
			}
			break;
		}
		case SC_SHFT_CLRHOME:	/* 0x4737 */	/* SHIFT+HOME */
		case SC_PGDN:		/* 0x5100 */	/* page down key (Milan &| emulators)   */
		case SC_END:		/* 0x4f00 */		/* END key (Milan &| emus)		*/
		{
			nxt = ob_find_next_any_flagstate(wt, st, focus(wt),
				OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_LAST);
			set_nxt_focus( &nxt, &o, ret_focus );
			DIAGS(("Form_Cursor: SHIFT+HOME from %d to %d", aesobj_item(&o), aesobj_item(&o)));
			break;
		}
		case SC_CLRHOME:	/* 0x4700 */	/* HOME */
		case SC_PGUP:		/* 0x4900 */	/* page up key (Milan &| emulators)    */
		{
			nxt = ob_find_next_any_flagstate(wt, aesobj(obtree,0), focus(wt),
				OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_FIRST);
			set_nxt_focus( &nxt, &o, ret_focus );
			DIAGS(("Form_Cursor: HOME from %d to %d", aesobj_item(&o), aesobj_item(&o)));
			break;
		}
		default:
		{
			kout = keycode;
			o = inv_aesobj();		/* This is also a safeguard.  */
			break;
		}
	}

	if (keyout)
		*keyout = kout;

	DIAGS(("Form_Cursor: from obj=%d to obj=%d, wt-edit_obj=%d, wt->e.pos=%d",
		obj.item, o.item, edit_item(&wt->e), wt->e.pos));

	/* At last this piece of code is on the right spot.
	 * This is important! Now I know that bug fixes in here are good enough for all cases.
	 */

	return o;
}
/*
 * Returns false of exit/touchexit object, true otherwise
 * Insert simulates left-click
 */
bool
Form_Keyboard(XA_TREE *wt,
	      struct xa_vdi_settings *v,
	      struct xa_aes_object obj,
	      const struct rawkey *key,
	      bool redraw,
	      struct xa_rect_list **rl,
	      /* outputs */
	      struct xa_aes_object *nxtobj,
	      short *newstate,
	      unsigned short *nxtkey)
{
	ushort keycode	= key->aes,
	       keystate = key->raw.conin.state;
	struct fmd_result fr;
	struct xa_rect_list *lrl = NULL;
	OBJECT *obtree = wt->tree;
	struct objc_edit_info *ei;
	struct xa_aes_object next_obj, new_eobj, new_focus;

	if (!rl) rl = &lrl;

	DIAG((D_form, NULL, "Form_Keyboard: wt=%lx, obtree=%lx, wt->owner=%lx, obj=%d, key=%x(%x), nrmkey=%x for %s",
		(unsigned long)wt, (unsigned long)wt->tree, (unsigned long)wt->owner,
		aesobj_item(&obj), keycode, key->aes, key->norm, wt->owner->name));

	ei = (wt->wind->dial & created_for_TOOLBAR) && wt->ei ? wt->ei : &wt->e;

// 	display("Form_Keyboard:   wt=%lx, obtree=%lx, wt->owner=%lx(%lx), obj=%d, key=%x(%x), nrmkey=%x for %s",
// 		ei->obj, wt, wt->tree, wt->owner, wt->owner, obj, keycode, key->aes, key->norm, wt->owner->name);

	fr.no_exit = true;

	if (!aesobj_item(&obj))
		new_eobj = ei->o;
	else
		new_eobj = obj;

	if (new_eobj.item < 0 || (/*!(wt->wind->dial & created_for_TOOLBAR) &&*/ !object_is_editable(new_eobj.ob, 0, 0)) )
	{
		new_eobj = ob_find_next_any_flagstate(wt, aesobj(obtree, 0), focus(wt), OF_EDITABLE, OF_HIDETREE, 0, OS_DISABLED, 0, 0, OBFIND_FIRST);
	}

	/*
	 * Cursor?
	 */
	next_obj = new_eobj;

	if (focus_set(wt))
	{

		switch (focus_ob(wt)->ob_type & 0xff)
		{
g_slist:
			case G_SLIST:
			{
				struct scroll_info *list = object_get_slist(focus_ob(wt));
				if ( list )
				{
					if( list->keypress )
					{
						/* cursor-keys */
						if ((*list->keypress)(list, keycode, keystate) == 0)
						{
							keycode = 0;
							goto done;
						}
					}
					/* pg-up etc. */
					if ( scrl_cursor(list, keycode, keystate) == 0)
					{
						keycode = 0;
						goto done;
					}
				}
				break;
			}
			default:;
		}
	}

	next_obj = Form_Cursor(wt, v, keycode, keystate, next_obj, redraw, rl, &new_focus, &keycode);
	if (keycode)
	{
		bool has_focus = focus_set(wt);

		if (keycode == SC_SPACE && has_focus && !(focus_ob(wt)->ob_flags & OF_EDITABLE))
		{
			next_obj = wt->focus;
		}
		else if (keycode == SC_INSERT && has_focus)
		{
			next_obj = wt->focus;
		}
		else if (keycode == SC_RETURN || keycode == SC_NMPAD_ENTER)
		{
			next_obj = ob_find_flst(obtree, OF_DEFAULT, 0, 0, OS_DISABLED, 0, 0);
			DIAG((D_keybd, NULL, "Form_Keyboard: Got RETRURN key - default obj=%d for %s",
				next_obj.item, wt->owner->name));
		}
		else if (keycode == SC_UNDO)	/* UNDO */
		{
			next_obj = ob_find_cancel(obtree);

			DIAG((D_keybd, NULL, "Form_Keyboard: Got UNDO key - cancel obj=%d for %s",
				next_obj.item, wt->owner->name));

		}
		else
		{
			if ( ((key->raw.conin.state & (K_CTRL|K_ALT)) == K_ALT )
				|| (wt->owner->status & CS_FORM_ALERT))
			{
				char c = toupper(key->norm & 0x00ff);
				next_obj = ob_find_shortcut(obtree, c, 0);
				DIAG((D_keybd, NULL, "Form_Keyboard: shortcut %d for %s",
					next_obj.item, wt->owner->name));
			}
		}

		if (valid_aesobj(&next_obj))
		{
			struct moose_data md;

			check_mouse(wt->owner, &md.cstate, &md.x, &md.y);
			md.state = MBS_LEFT;

			fr.no_exit = Form_Button(wt, v,
					         next_obj,
					         &md,
					         FBF_REDRAW|FBF_KEYBD, //|FBF_CHANGE_FOCUS,
					         rl,
					         &fr.obj_state,
					         &fr.obj,
					         &fr.dblmask);
			keycode = 0;
		}
		else if (keycode != SC_RETURN && keycode != SC_NMPAD_ENTER)
		{
			if (!focus_set(wt) || same_aesobj(&wt->focus, &new_eobj))
			{
				if (same_aesobj(&new_eobj, &obj) && valid_aesobj(&new_eobj))
					new_eobj = aesobj(wt->tree, 0);
			}
			else if (focus_set(wt) && valid_aesobj(&new_eobj) &&
				(!(focus_ob(wt)->ob_flags & OF_EDITABLE) || !same_aesobj(&wt->focus, &new_eobj)))
			{
				new_focus = new_eobj;
				if (same_aesobj(&new_eobj, &obj) && valid_aesobj(&new_eobj))
					new_eobj = aesobj(wt->tree, 0);
			}
			else
			{
				if (!same_aesobj(&new_eobj, &obj))
					next_obj = new_eobj;
				else if (valid_aesobj(&new_eobj))
					new_eobj = aesobj(wt->tree, 0);
			}
		}
		else
			keycode = 0;
	}
	else
	{
		if (valid_aesobj(&next_obj))
			new_eobj = next_obj;
		if (same_aesobj(&new_eobj, &obj) && valid_aesobj(&new_eobj))
			new_eobj = aesobj(wt->tree, 0);
	}

	if (valid_aesobj(&new_focus))
	{
		if (!same_aesobj(&wt->focus, &new_focus))
		{
			short md = 0;
			struct xa_aes_object pf = wt->focus;
			wt->focus = new_focus;
			if (aesobj_edit(&new_focus))
				md = DRW_CURSOR;
			wt->e.c_state |= OB_CURS_EOR;
			if (valid_aesobj(&pf))
			{
				obj_draw(wt, v, pf, 0, NULL, *rl, UNDRAW_FOCUS|DRW_CURSOR);		/* remove cursor+focus */
			}
			if( !(new_focus.ob->ob_state & OS_SELECTED) )
				md |= DRAW_FOCUS;
			obj_draw(wt, v, new_focus, 0, NULL, *rl, md);
		}
		if( new_focus.ob->ob_type == G_SLIST )
		{
		/*
		 * this is true if user uses a cursor-key right after opening of a list-window
		 * and focus was not set to the list
		 */
			keycode = key->aes;
			goto g_slist;
		}
	}
	//else
		//wt->focus.item = -1;

done:
	if (fr.no_exit)
		next_obj = new_eobj;
	/* Ozk: We return a 'next object' value of -1 when the key was
	 *	not used by form_keybaord() function
	 */
// 	if (next_obj < 0)
// 		next_obj = -1;
	if (nxtobj)
		*nxtobj = next_obj;
	if (newstate)
		*newstate = valid_aesobj(&next_obj) ? aesobj_state(&next_obj) : 0;

	if (nxtkey)
		*nxtkey = keycode;

	DIAG((D_keybd, NULL, "Form_Keyboard: no_exit=%s(%d), nxtobj=%d, obstate=%x, for %s",
		fr.no_exit ? "true" : "false", fr.no_exit, aesobj_item(&next_obj), aesobj_state(&next_obj), wt->owner->name));


// 	display("Form_Keyboard: no_exit=%s(%d), nxtobj=%d, nxteobj=%d, obstate=%x, for %s",
// 		fr.no_exit ? "true" : "false", fr.no_exit, next_obj, new_eobj, wt->tree[next_obj].ob_state, wt->owner->name);

	return fr.no_exit ? 1 : 0;
}

/*
 * Main FormExit(). Installed in wt->exit_form().
 * If wind is NULL, we're exiting a classic AES form_do session.
 * If wind is valid and the window used by AES form_do, we're exiting a windowed AES form_do.
 * If wind is valid and not used by AES form_do, we're exiting a dialog thing thang.
 */
void
Exit_form_do( struct xa_client *client,
	      struct xa_window *wind,
	      XA_TREE *wt,
	      struct fmd_result *fr)
{
	int lock = 0;

	if (wind)
	{
#if 1
		/* XXX - Ozk: complete this someday, having most XaAES-controlled
		 *	 dialog-windows using the same exit code..
		 */
		if ((wind->dial & created_for_ALERT)) //wind == client->alert)
		{
			struct xa_aes_object f = fr->obj;
			/* Is f a valid button? */
			if ( aesobj_item(&f) >= ALERT_BUT1 && aesobj_item(&f) < ALERT_BUT1 + ALERT_BUTTONS && !aesobj_hidden(&f))
			{
				client->waiting_pb->intout[0] = aesobj_item(&f) - ALERT_BUT1 + 1;
				if (client->waiting_pb)
				{
					client->usr_evnt = 1;
					client->waiting_pb = NULL;
				}
			}
			if (client->alert == wind)
				client->alert = NULL;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
			{
			struct xa_client *n = 0;
			if( !( wind->owner == C.Aes || wind->owner == C.Hlp) )
				n = wind->owner;
			else if( wind->next )
				n = wind->next->owner;
			if( n )
			{
				app_in_front(lock, n, true, true, true);
			}
			}

		}
#endif
		else if (wind == client->fmd.wind)
		{
			DIAG((D_form, NULL, "Exit_form_do: exit windowed form_do() for %s",
				client->name));

			if (wind->dial & created_for_FORM_DO)
			{
				client->fmd.wind = NULL;
				client->fmd.state = 0;
				close_window(lock, wind);
				delayed_delete_window(lock, wind);
			}
			/* Ozk:
			 * If window was created by XA_form_dial(FMD_START)
			 * we let the window live until client calls
			 * form_dial(FMD_FINISH)
			 */
		}
		else if (wind->dial & created_for_TOOLBAR)
		{
			DIAG((D_form, NULL, "Exit_form_do: send WM_TOOLBAR to %s",
				client->name));

			if (wind->send_message && valid_aesobj(&fr->obj))
			{
				short keystate;

				if (fr->md)
				{
					keystate = fr->md->kstate;
				}
				else
					vq_key_s(C.P_handle, &keystate);


				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_NORM,
						WM_TOOLBAR, 0, 0, wind->handle,
						aesobj_item(&fr->obj), fr->dblmask ? 2 : 1, keystate, 0);
			}
			return;
		}
		else
		{
			DIAG((D_form, NULL, "Exit_form_do: remove toolbar"));
			remove_widget(lock, wind, XAW_TOOLBAR);
		}
	}
	else
	{
		DIAG((D_form, NULL, "Exit_form_do: Classic"));
		client->fmd.keypress = NULL;
		client->fmd.mousepress = NULL;
	}

	if (client->waiting_pb)
	{
		client->waiting_pb->intout[0] = (aesobj_item(&fr->obj) >= 0 ? aesobj_item(&fr->obj) | fr->dblmask : 0);
		client->usr_evnt = 1;
	}
}
/*
 * WidgetBehaviour()
 * Return is used by do_widgets() to check if state of obj is
 * to be reset or not
 */
bool
Click_windowed_form_do(	int lock,
			struct xa_window *wind,
			struct xa_widget *widg,
			const struct moose_data *md)
{
	struct xa_client *client = wind->owner;
	XA_TREE *wt;

	wt = widg->stuff;

	DIAG((D_form, client, "Click_windowed_form_do: client=%lx, wind=%lx, wt=%lx",
		(unsigned long)client, (unsigned long)wind, (unsigned long)wt));

// 	display("Click_windowed_form_do: client=%lx, wind=%lx, wt=%lx",
// 		client, wind, wt);

	Click_form_do(lock, client, wind, wt, md);

	return false;
}

/*
 * FormMouseInput()
 * Installed at client->fmd.mousepress and is called by AESSYS
 * when a click happened. Also called by windowed form_do() sessions.
 *
 */
static bool
Click_form_do(int lock,
	      struct xa_client *client,
	      struct xa_window *wind,
	      struct widget_tree *wt,
	      const struct moose_data *md)
{
// 	struct xa_widget *widg = get_widget(wind, XAW_TOOLBAR);
	struct xa_vdi_settings *v;
	OBJECT *obtree = NULL;
	GRECT r;

	DIAG((D_form, client, "Click_form_do: %s formdo for %s",
		wind ? "windowed":"classic", client->name));

// 	display("Click_form_do: %s formdo for %s",
// 		wind ? "windowed":"classic", client->name);
	/*
	 * If window is not NULL, the form_do is a windowed one,
	 * else it is a classic blocking form_do
	 */
	if (wind)
	{
		v = wind->vdi_settings;

		if (!wind->nolist && !is_topped(wind) && is_toppable(wind)) // !wind->nolist && wind != window_list && !(wind->active_widgets & NO_TOPPED) )
		{
			DIAGS(("Click_form_do: topping window"));
// 			display("Click_form_do: topping window");
			top_window(lock, true, false, wind);
			return false;
		}

		if (!wt)
		{
			DIAGS(("Click_form_do: using wind->toolbar"));
// 			display("Click_form_do: using wind->toolbar");
			wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		}

		if (wt)
		{
			obtree = rp_2_ap(wind, wt->widg, &r);
// 			display("Click_form_do: wt=%lx, wt_tree=%lx, widg=%lx, obtree = %lx",
// 				wt, wt->tree, wt->widg, obtree);
// 			display(" ---===---     widg=%lx, widg->stuff=%lx", widg, widg->stuff);
		}
	}
	/*
	 * Not a windowed form session.
	 */
	else
	{
		v = client->vdi_settings;

		if (!wt)
		{
			DIAGS(("Click_form_do: using client->wt"));
// 			display("Click_form_do: using client->wt");
			wt = client->fmd.wt;
		}
		if (wt)
			obtree = wt->tree;
	}

	if (obtree)
	{
		struct fmd_result fr;

		fr.obj = obj_find(wt, aesobj(wt->tree, 0), 10, md->x, md->y, NULL);

		if (aesobj_item(&fr.obj) >= 0 &&
		    !Form_Button(wt, v,
				 fr.obj,
				 md,
				 FBF_REDRAW|FBF_CHANGE_FOCUS,
				 wind ? &wind->rect_list.start : NULL,
				 &fr.obj_state,
				 &fr.obj,
				 &fr.dblmask))
		{
// 			display("Click_form_do: exit_for = %lx", wt->exit_form);
			if (wt->exit_form)
			{
				DIAGS(("Click_form_do: calling exit_form"));
// 				display("Click_form_do: calling exit_form");
				fr.md = md;
				fr.key = NULL;
				fr.no_exit = false;
				(*wt->exit_form)(client, wind, wt, &fr);
			}
		}
	} else
	{
		DIAGS(("Click_form_do: NO OBTREE!!"));
	}

	DIAGS(("Click_form_do: return"));
	return false;
}

/*
 * FormKeyInput()
 * Ozk:
 * If wind is NULL, this is a classic form_do session, and XA_TREE to use
 * is client->wt.
 * If wind is not NULL, this is a windowed form_dialog session and XA_TREE
 * to use is wind->toolbar.
 */
bool
Key_form_do(int lock,
	    struct xa_client *client,
	    struct xa_window *wind,
	    struct widget_tree *wt,
	    const struct rawkey *key,

	    struct fmd_result *ret_fr)
{
	struct xa_vdi_settings *v;
	GRECT *clip = NULL;
	OBJECT *obtree = NULL;
	struct objc_edit_info *ei;
	struct xa_rect_list *lrl = NULL, **rl = wind ? &wind->rect_list.start : &lrl;
	struct fmd_result fr;
	GRECT r;

	fr.no_exit = true;
	fr.flags = 0;

	DIAG((D_form, client, "Key_form_do: %s formdo for %s",
		wind ? "windowed":"classic", client->name));

	/*
	 * If window is not NULL, the form_do is a windowed one,
	 * else it is a classic blocking form_do
	 */
	if (wind)
	{
		DIAGS(("Key_form_do: using wind->toolbar"));
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		obtree = rp_2_ap(wind, wt->widg, &r);
		v = wind->vdi_settings;
	}
	/*
	 * Not a windowed form session.
	 */
	else
	{
		if (!wt)
		{
			DIAGS(("Key_form_do: using client->wt"));
			wt = client->fmd.wt;
		}
		obtree = wt->tree;
		v = client->vdi_settings;
	}

	ei = !(wt->wind->dial & created_for_TOOLBAR) && wt->ei ? wt->ei : &wt->e;

	if (obtree)
	{
		fr.dblmask = 0;

		if( ei->o.item >= 0
			|| !(wt->wind->dial & created_for_TOOLBAR) || key->aes == SC_RETURN || (key->raw.conin.state & (K_CTRL|K_ALT)) == K_ALT )
			fr.no_exit = Form_Keyboard(wt,
					   v,
					   ei->o,
					   key,
					   true,
					   rl,
					   &fr.obj,
					   &fr.obj_state,
					   &fr.aeskey);

		DIAGS(("Key_form_do: Form_Keyboard - no_exit=%s, obj=%d, objstat=%x, aeskey %x",
			fr.no_exit ? "yes":"no", aesobj_item(&fr.obj), fr.obj_state, fr.aeskey));

		if (fr.no_exit)
		{
			if (fr.aeskey && ei->o.item >= 0)
			{
				obj_edit(wt,
					  v,
					  ED_CHAR,
					  ei->o,
					  fr.aeskey,
					  ei->pos,
					  NULL,
					  true,
					  clip,
					  *rl,
					  NULL,
					  NULL);
				fr.aeskey = 0;
				fr.flags |= FMDF_EDIT;

				DIAGS(("Key_form_do: obj_edit - edobj=%d, edpos=%d",
					edit_item(ei), ei->pos));
			}
			else if (aesobj_item(&fr.obj) > 0 && !obj_is_edit(ei, &fr.obj))
			{
				obj_edit(wt, v, ED_END, fr.obj, 0, 0, NULL, true, clip, *rl, NULL, NULL);
				obj_edit(wt, v, ED_INIT, fr.obj, 0, -1, NULL, true, clip, *rl, NULL, NULL);
			}
		}
		else
		{
			if (wt->exit_form)
			{
				DIAGS(("Key_form_do: calling exit_form"));
				fr.md = NULL;
				fr.key = key;
				wt->exit_form(client, wind, wt, &fr);
			}
		}
	}
	if (ret_fr)
		*ret_fr = fr;

	DIAGS(("Key_form_do: returning - no_exit=%s", fr.no_exit ? "yes":"no"));
	return fr.no_exit;
}

static void
dfwm_redraw(struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt, GRECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;

	if (wt && wt->tree)
	{
		GRECT dr;
		struct xa_rect_list *rl;

		rl = wind->rect_list.start;

		if (wdg_is_inst(widg) && rl)
		{
			hidem();
			while (rl)
			{
				if (xa_rect_clip(&wind->wa, &rl->r, &dr))
				{
					if (clip)
					{
						if (xa_rect_clip(clip, &dr, &dr))
						{
							(*v->api->set_clip)(v, &dr);
							widg->m.r.draw(wind, widg, &dr);
						}
					}
					else
					{
						(*v->api->set_clip)(v, &dr);
						widg->m.r.draw(wind, widg, &dr);
					}
				}
				rl = rl->next;
			}
			(*v->api->clear_clip)(v);
			showm();
		}
	}
}

/*
 * WinMsgHandler() - This MUST run in the context of the window owner!
 */
void
do_formwind_msg(
	struct xa_window *wind,
	struct xa_client *to_client,			/* if different from wind->owner */
	short amq, short qmflags,
	short *msg)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_WIDGET *widg = wind->tool;
	bool draw = false;

	DIAG((D_form, wind->owner, "do_formwind_msg: wown %s, to %s, widg=%lx, msg %d, %d, %d, %d, %d, %d, %d, %d",
		wind->owner->name, to_client->name, (unsigned long)widg, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	if (widg)
	{
		XA_TREE *wt = widg->stuff;
		OBJECT *ob = wt->tree + widg->start;
		short ww = wind->wa.g_w,			/* window measures */
		      wh = wind->wa.g_h,
		      dx = wt->dx,			/* object displacement */
		      dy = wt->dy,
		      ow = ob->ob_width,		/* object measures */
		      oh = ob->ob_height;
		switch (msg[0])
		{
		case WM_REDRAW:
		{
			if (!wt->ei && edit_set(&wt->e))
			{
				obj_edit(wt, v, ED_END, editfocus(&wt->e), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);
			}

			dfwm_redraw(wind, widg, wt, (GRECT *)&msg[4]);

			if (!wt->ei && edit_set(&wt->e))
			{
				obj_edit(wt, v, ED_END, editfocus(&wt->e), 0, 0, NULL, true, &wind->wa, wind->rect_list.start, NULL, NULL);
			}
			kick_mousemove_timeout();
			break;
		}
		case WM_MOVED:
		{
			if (wind != root_window && wt && wt->tree)
			{
				move_window(0, wind, true, -1, msg[4],msg[5],msg[6],msg[7]);
			}
			break;
		}
		case WM_TOPPED:
		{
			if (wind->owner->fmd.wind == wind)
			{
				top_window(0, true, false, wind);
			}
			else if ( wind != root_window &&
				 (wind->window_status & XAWS_OPEN) &&
				 (wind->nolist ? !wind_has_focus(wind) : !is_topped(wind))
				)
			{
				if (is_hidden(wind))
					unhide_window(0, wind, true);

				top_window(0, true, false, wind);
			}
			if( !(wind->dial & created_for_FMD_START) )
				ob_set_wind( wt->tree, G_SLIST, msg[0] );
			break;
		}
		case WM_ONTOP:
		case WM_UNTOPPED:
		{
			if( !(wind->dial & created_for_FMD_START) )
				ob_set_wind( wt->tree, G_SLIST, msg[0] );
			break;
		}
#if 1
		case WM_BOTTOMED:
		{
			if (wind->owner->fmd.wind == wind)
				bottom_window(0, false, true, wind);

			else if ( wind != root_window &&
				 (wind->window_status & XAWS_OPEN) &&
				 (wind->nolist ? wind_has_focus(wind) : is_topped(wind))
				)
				bottom_window(0, false, true, wind);
			break;
		}
#endif
		case WM_ARROWED:
		{
			if (msg[4] < WA_LFPAGE || msg[4] == WA_UPSCAN || msg[4] == WA_DNSCAN)
			{
				if (wh < oh)
				{
					switch (msg[4])
					{
						case WA_UPLINE:
							dy -= screen.c_max_h;
							break;
						case WA_DNLINE:
							dy += screen.c_max_h;
							break;
						case WA_UPPAGE:
							dy -= wh/*oh*/ - screen.c_max_h;
							break;
						case WA_DNPAGE:
							dy += wh/*oh*/ - screen.c_max_h;
							break;
						case WA_UPSCAN:
							dy += msg[5];
							break;
						case WA_DNSCAN:
							dy -= msg[5];
							break;
					}
/* align ( not that object height is always >= work area height) */
					if (dy < 0)
						dy = 0;
					if (dy > oh - wh)
						dy = oh - wh;
				}
			}
			else
			{
				if (ww < ow)
				{
					switch (msg[4])
					{
						case WA_LFLINE:
							dx -= screen.c_max_w;
							break;
						case WA_RTLINE:
							dx += screen.c_max_w;
							break;
						case WA_LFPAGE:
							dx -= ww/*ow*/ - screen.c_max_w;
							break;
						case WA_RTPAGE:
							dx += ww/*ow*/ - screen.c_max_w;
					}
					if (dx < 0)
						dx = 0;
					if (dx > ow - ww)		/* align */
						dx = ow - ww;
				}
			}
			break;
		}
		case WM_CLOSED:
		{
			close_window(0, wind);
			if (!(wind->window_status & XAWS_NODELETE))
				delayed_delete_window(0, wind);
			return;
		}
		case WM_VSLID:
		{
			if (wh < oh)
				dy = sl_to_pix(oh - wh, msg[4]);
			break;
		}
		case WM_HSLID:
		{
			if (ww < ow)
				dx = sl_to_pix(ow - ww, msg[4]);
			break;
		}

		case WM_SIZED:
		{
			short n = 0;
			/* if (!wind->nolist && (wind->active_widgets & SIZE)) */
			{
				if( msg[6] < wind->min.g_w )
				{
					msg[4] = wind->r.g_x;
					msg[6] = wind->min.g_w;
					n |= 1;
				}

				if( msg[7] < wind->min.g_h )
				{
					msg[5] = wind->r.g_y;
					msg[7] = wind->min.g_h;
					n |= 2;
				}
				if( n == 3 )
					return;

				/*
				 * resize internal window (except fsel)
				 * move window here, set RESIZED-flag, and recalc during redraw in draw_object_tree "on the fly"
				 * return then
				 *
				 */
				 /* misuse wind->min.x/y for saving old width/height*/
				wind->min.g_x = wind->pr.g_w;
				wind->min.g_y = wind->pr.g_h;

				wind->window_status |= XAWS_RESIZED;
				move_window(0, wind, false, -1, msg[4], msg[5], msg[6], msg[7]);
				wind->pr = wind->r;

				return;

			}
			break;
		}
		default:
		{
			return;
		}
		}

		if (dx != wt->dx)
		{
			draw = true;
			XA_slider(wind, XAW_HSLIDE, ow, ww, dx);
		}

		if (dy != wt->dy)
		{
			draw = true;
			XA_slider(wind, XAW_VSLIDE, oh, wh, dy);
		}

		if (draw)
		{
			GRECT sc, clip, *clp_p = NULL;
// 			display("getting here?");
			hidem();
			if (wt->dy != dy && ((wind->nolist && nolist_list == wind) || (!wind->nolist && is_topped(wind))))
			{
				short yoff;
				GRECT from, to;

				yoff = dy - wt->dy; // - dy;
				if (yoff < 0)
				{
// 					display("blit up");
					yoff = -yoff;

					if (yoff < wind->wa.g_h - 4)
					{
						/* wincontent moving down to show elements above */
						from.g_x = to.g_x = wind->wa.g_x;
						from.g_y = wind->wa.g_y;
						from.g_w = to.g_w = wind->wa.g_w;
						from.g_h = to.g_h = wind->wa.g_h - yoff;

						to.g_y = from.g_y + yoff;
						(*v->api->form_copy)(&from, &to);
						clip.g_x = wind->wa.g_x;
						clip.g_y = wind->wa.g_y;
						clip.g_w = wind->wa.g_w;
						clip.g_h = yoff;
						clp_p = &clip;
					}
				}
				else if (yoff > 0)
				{
// 					display("blit down");
					/* wincontent moving up to show elements below */
					if (yoff < wind->wa.g_h - 4)
					{
						from.g_x = to.g_x = wind->wa.g_x;
						from.g_y = wind->wa.g_y + yoff;
						from.g_w = to.g_w = wind->wa.g_w;
						from.g_h = to.g_h = wind->wa.g_h - yoff;
						to.g_y = wind->wa.g_y;
						(*v->api->form_copy)(&from, &to);
						clip.g_x = wind->wa.g_x;
						clip.g_y = wind->wa.g_y + from.g_h;
						clip.g_w = wind->wa.g_w;
						clip.g_h = yoff;
						clp_p = &clip;
					}
				}

			}
			wt->dx = dx;
			wt->dy = dy;
			(*v->api->save_clip)(v, &sc);
// 			display_window(0, 120, wind, NULL);

			display_widget(0, wind, get_widget(wind, XAW_VSLIDE), wind->rect_list.start);
			dfwm_redraw(wind, widg, wt, clp_p/*NULL*/);
			(*v->api->restore_clip)(v, &sc);
			showm();
		}
	}
}
