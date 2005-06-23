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

#include "xa_types.h"
#include "xa_global.h"
#include "xa_menu.h"

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

/*
 * Attatch a MODAL form_do session to a client.
 * client->wt (XA_TREE) is used for modal form_do.
 * wind->toolbar (XA_TREE) is used for nonmodal (windowed) form_do.
 */
void
Set_form_do(struct xa_client *client,
	    OBJECT *obtree,
	    short edobj)
{
	short new_obj;
	XA_TREE *wt;

	wt = obtree_to_wt(client, obtree);
	if (!wt)
		wt = new_widget_tree(client, obtree);
	
	DIAG((D_form, client, "Set_form_do: wt=%lx, obtree=%lx, edobj=%d for %s",
		wt, obtree, edobj, client->name));

	/* XXX - We should perhaps check if the form really have an EXIT or
	 *       TOUCHEXIT button, and act accordingly.
	 */
	wt->exit_form	= Exit_form_do; //Classic_exit_form_do;
	
	/* Ozk:
	 * If the first obj_edit call fails, we call it again passing the
	 * object it returned, which obj_edit() looked up. See obj_edit()
	 */
	if (edobj == -2)
		edobj = ob_find_any_flst(obtree, OF_EDITABLE, 0, 0, OS_DISABLED, OF_LASTOB, 0);

	if (!obj_edit(wt, client->vdi_settings, ED_INIT, edobj, 0, -1, false, NULL, NULL, NULL, &new_obj))
		obj_edit(wt, client->vdi_settings, ED_INIT, new_obj, 0, -1, false, NULL, NULL, NULL, NULL);

	/* Ozk:
	 * Check if this obtree needs a keypress handler..
	 */
	if (wt->e.obj >= 0 || obtree_has_default(obtree) )
		client->fmd.keypress = Key_form_do;

	/*
	 * Install mouse button handler.
	 */
	client->fmd.mousepress = Click_form_do;
	client->fmd.wt = wt;
}

struct xa_window *
create_fmd_wind(enum locks lock, struct xa_client *client, XA_WIND_ATTR kind, WINDOW_TYPE dial, RECT *r)
{
	struct xa_window *wind = NULL;
	
	bool nolist = false;

	DIAGS(("Setup_form_do: create window"));

	if (C.update_lock && C.update_lock == client->p)
	{
		kind |= STORE_BACK;
		nolist = true;
	}

	kind |= (TOOLBAR | USE_MAX);
	wind = create_window(lock,
			     do_winmesag,
			     do_formwind_msg,
			     client,
			     nolist,
			     kind,
			     dial,
			     0, /* frame size */
			     C.Aes->options.thinwork,
			     *r,
			     NULL,
			     NULL);

	return wind;
}

static void
calc_fmd_wind(struct xa_client *client, OBJECT *obtree, XA_WIND_ATTR kind, WINDOW_TYPE dial, RECT *r)
{
	DIAG((D_form, client, "Setup_form_do: Create window for %s", client->name));

	ob_rectangle(obtree, 0, r);

	*r = calc_window(0,
			 client,
			 WC_BORDER,
			 kind, dial,
			 0,
			 C.Aes->options.thinwork,
			 *r);

}

/*
 * XXX - Ozk:
 *     This call should never fail, so callers of this function
 *     must take that into account .. especially XA_form_do()
 */
bool
Setup_form_do(struct xa_client *client,
	      OBJECT *obtree,
	      short edobj,
	      /* Output */
	      struct xa_window **ret_wind,
	      short *ret_edobj)
{
	enum locks lock = 0;
	XA_WIND_ATTR kind = NAME;
	XA_TREE *wt = NULL;
	struct xa_window *wind = NULL;

	/*
	 * Window for form/dialogs already created by form_dial()?
	 */
	if (client->fmd.wind)
	{
		DIAG((D_form, client, "Setup_form_do: wind %d for %s", client->fmd.wind->handle, client->name));
		wind = client->fmd.wind;
		calc_fmd_wind(client, obtree, kind, wind->dial, (RECT *)&client->fmd.r);
		wt = set_toolbar_widget(lock, wind, client, obtree, edobj, WIP_NOTEXT, true, NULL, &client->fmd.r);
		wt->zen = false;
		move_window(lock, wind, true, -1, client->fmd.r.x, client->fmd.r.y, client->fmd.r.w, client->fmd.r.h);
	}
	/*
	 * Should this client do classic blocking form_do's?
	 */
	else if (C.update_lock == client->p)
	{
		DIAG((D_form, client, "Setup_form_do: nonwindowed for %s", client->name));
		Set_form_do(client, obtree, edobj);
		wt = client->fmd.wt;
		goto okexit;
	}
	/*
	 * First time this client does a form_do/dial, do some preps
	 */
	else
	{
		calc_fmd_wind(client, obtree, kind, client->fmd.state ? created_for_FMD_START : created_for_FORM_DO, (RECT *)&client->fmd.r);

		if (!client->options.xa_nomove)
			kind |= MOVER;

		client->fmd.kind = kind;
	}

	/*
	 * If we get here and have no window pointer, this is the first time
	 * the client uses form_do.
	 */
	if (!wind)
	{
		if ((wind = create_fmd_wind(lock, client, kind, client->fmd.state ? created_for_FMD_START : created_for_FORM_DO, &client->fmd.r)))
		{
			client->fmd.wind = wind;
			wt = set_toolbar_widget(lock, wind, client, obtree, edobj, WIP_NOTEXT, true, NULL, &client->fmd.r);
			wt->zen = false;
		}
		else
		{
			client->fmd.wind = NULL;
			return false;
		}
	}
okexit:
	DIAGS(("Key_form_do: returning - edobj=%d, wind %lx",
		wt->e.obj, wind));
	if (ret_edobj)
		*ret_edobj = wt->e.obj;
	if (ret_wind)
		*ret_wind = wind;

	return true;
}

void
form_center(OBJECT *form, short barsizes)
{
	form->ob_x = root_window->wa.x + (root_window->wa.w - form->ob_width) / 2;
	form->ob_y = root_window->wa.y + barsizes + (root_window->wa.h - form->ob_height) / 2;
}
void
form_center_r(OBJECT *form, short barsizes, RECT *r)
{
	r->x = root_window->wa.x + (root_window->wa.w - form->ob_width) / 2;
	r->y = root_window->wa.y + barsizes + (root_window->wa.h - form->ob_height) / 2;
	r->w = form->ob_width;
	r->h = form->ob_height;
}

void
center_rect(RECT *r)
{
	struct xa_widget *widg = get_menu_widg();

	r->x = root_window->wa.x + ((root_window->wa.w - r->w) >> 1);
	r->y = root_window->wa.y + widg->ar.h + ((root_window->wa.h - r->h) >> 1);
}

/*
 * Ozk: Rewritten totally. Returns false if a TOUCHEXIT or EXIT button was selected.
 * puts a 0 in 'nxtob' if the object is not an [TOUCH]EXIT or EDITABLE. If object is
 * any of the two, its passed back in 'nxtob'.
 */
bool
form_button(XA_TREE *wt,
	    struct xa_vdi_settings *v,
	    short obj,
	    const struct moose_data *md,
	    unsigned long fbflags,
	    struct xa_rect_list *rl,
	    /* Outputs */
	    short *newstate,
	    short *nxtob,
	    short *clickmsk)
{
	RECT *clip = NULL;
	OBJECT *obtree = wt->tree;
	short next_obj = 0;
	short flags, state;
	bool no_exit = true, dc, redraw = (fbflags & FBF_REDRAW);

	DIAG((D_form, NULL, "form_button: wt=%lx, obtree=%lx, obj=%d",
		wt, wt->tree, obj));

	flags = obtree[obj].ob_flags;
	state = obtree[obj].ob_state;
	dc = md->clicks > 1 ? true : false;

	/* find_object can't report click on a OF_HIDETREE object. */
	/* HR: Unfortunately it could. Fixed that. */
	
	if (flags & OF_TOUCHEXIT)
	{
		no_exit = false;
	}
	
	if ( (obtree[obj].ob_flags & OF_SELECTABLE) && !(obtree[obj].ob_state & OS_DISABLED) )
	{
		short type = obtree[obj].ob_type & 0xff;

		if (type == G_SLIST)
		{
			if ((wt->flags & WTF_FBDO_SLIST) || (fbflags & FBF_DO_SLIST))
			{
				click_scroll_list(0, obtree, obj, md);
				no_exit = true;
			}
			else
				no_exit = false;
		}
		else if (type == G_POPUP)
		{
			MENU mn, result;
			POPINFO *pinf;
			short x, y, obnum;

			display("form_button: G_POPUP! %s", wt->owner->name);

			pinf = object_get_popinfo(obtree + obj);
			if ((obnum = pinf->obnum) > 0)
				pinf->tree[obnum].ob_state |= OS_CHECKED;
			else
				obnum = 0;

			mn.mn_tree = pinf->tree;
			mn.mn_menu = ROOT;
			mn.mn_item = obnum;
			mn.mn_scroll = 1;

			obj_offset(wt, obj, &x, &y);
			if (menu_popup(0, wt->owner, &mn, &result, x, y, 2) && result.mn_tree == mn.mn_tree)
				pinf->obnum = result.mn_item;
			if (obnum > 0)
				obtree[obnum].ob_state &= ~OS_CHECKED;
			obj_change(wt, v, obj, 1, state & ~OS_SELECTED, flags, redraw, clip, rl);
		}
		else if (flags & OF_RBUTTON)
		{
			DIAGS(("form_button: call obj_set_radio_button"));
			obj_set_radio_button(wt, v, obj, redraw, clip, rl);
		}
		else
		{
			if (redraw)
			{
				DIAGS(("form_button: call obj_watch"));
				obj_watch(wt, v, obj, state^OS_SELECTED, state, clip, rl);
			}
			else
			{
				DIAGS(("form_button: switch state"));
				obj_change(wt, v, obj, -1, state^OS_SELECTED, flags, redraw, clip, rl);
			}
		}
		state = obtree[obj].ob_state;	
	}

	DIAGS(("form_button: state %x, flags %x",
		state, flags));

	if ((state & OS_SELECTED) && (flags & OF_EXIT))
		no_exit = false;
		
	if (!no_exit || (flags & OF_EDITABLE))
		next_obj = obj;

	if (clickmsk)
		*clickmsk = dc ? 0x8000 : 0;

	if (nxtob)
		*nxtob = next_obj;
	
	if (newstate)
		*newstate = state;

	DIAGS(("form_button: return no_exit=%s, nxtob=%d, newstate=%x, clickmask=%x",
		no_exit ? "yes":"no", next_obj, state, dc ? 0x8000:0));

	return no_exit;
}

/*
 * Form Keyboard Handler for toolbars
 */
short
form_cursor(XA_TREE *wt,
	    struct xa_vdi_settings *v,
	    ushort keycode,
	    short obj,
	    bool redraw,
	    struct xa_rect_list *rl)
{
	OBJECT *obtree = wt->tree;
	short o = obj;
	short edcnt;
	short last_ob;

	last_ob = ob_count_flag(obtree, OF_EDITABLE, 0, OF_LASTOB, &edcnt);

	DIAG((D_form, NULL, "form_cursor: wt=%lx, obtree=%lx, obj=%d, keycode=%x, lastob=%d, editobjs=%d",
		wt, obtree, obj, keycode, last_ob, edcnt));

	switch(keycode)
	{			/* The cursor keys are always eaten. */
	case 0x0f09:		/* TAB moves to next field */
	case 0x5000:		/* DOWN ARROW also moves to next field */
	{
		if (edcnt > 1)
		{
			short nxt = ob_find_next_any_flag(obtree, o, OF_EDITABLE);
			if (nxt >= 0)
				o = nxt;

			DIAGS(("form_cursor: TAB/DOWN ARROW from %d to %d", obj, o));
		}
		break;
	}
	case 0x4800:		/* UP ARROW moves to previous field */
	{
		if (edcnt > 1)
		{
			short nxt = ob_find_prev_any_flag(obtree, o, OF_EDITABLE);
			if (nxt >= 0)
				o = nxt;

			DIAGS(("form_cursor: UP ARROW from %d to %d", obj, o));
		}
		break;
	}
	case 0x4737:		/* SHIFT+HOME */
	case 0x5032:		/* SHIFT+DOWN ARROW moves to last field */
	case 0x5100:		/* page down key (Milan &| emulators)   */
	{
		if (edcnt > 1)
		{
			short nxt = ob_find_prev_any_flag(obtree, -1, OF_EDITABLE);
			if (nxt >= 0)
				o = nxt;

			DIAGS(("form_cursor: SHIFT+HOME from %d to %d", obj, o));
		}
		break;
	}
	case 0x4700:		/* HOME */
	case 0x4838:		/* SHIFT+UP ARROW moves to first field */
	case 0x4900:		/* page up key (Milan &| emulators)    */
	{
		if (edcnt > 1)
		{
			short nxt = ob_find_next_any_flag(obtree, 0, OF_EDITABLE);
			if (nxt >= 0)
				o = nxt;

			DIAGS(("form_cursor: HOME from %d to %d", obj, o));
		}
		break;
	}
	default:
	{
		o = -1;		/* This is also a safeguard.  */
		break;
	}
	}

	DIAGS(("form_cursor: from obj=%d to obj=%d, wt-edit_obj=%d, wt->e.pos=%d",
		obj, o, wt->e.obj, wt->e.pos));
	
	/* At last this piece of code is on the right spot.
	 * This is important! Now I know that bug fixes in here are good enough for all cases.
	 */

#if 0
	/* Big mistake - it is up to the apps themselves to actually move the cursor
	 * But this could perhaps be used in an extended XaAES mode of some kind?
	 */
	if (o >= 0 && o != obj)
	{	
		/* If edit field has changed, update the screen */
		obj_edit(wt, v, ED_END, 0, 0, 0, redraw, rl, NULL, NULL);
		obj_edit(wt, v, ED_INIT, o, 0, -1, redraw, rl, NULL, NULL);
	}
#endif
	return o;
}
/*
 * Returns false of exit/touchexit object, true otherwise
 */
bool
form_keyboard(XA_TREE *wt,
	      struct xa_vdi_settings *v,
	      short obj,
	      const struct rawkey *key,
	      bool redraw,
	      struct xa_rect_list *rl,
	      /* outputs */
	      short *nxtobj,
	      short *newstate,
	      short *nxtkey)
{
#if GENERATE_DIAGS
	struct xa_client *client = wt->owner;
#endif
	ushort keycode = key->aes, next_key = 0;
	short next_obj;
	struct fmd_result fr;

	DIAG((D_form, NULL, "form_keyboard: wt=%lx, obtree=%lx, wt->owner=%lx(%lx), obj=%d, key=%x(%x), nrmkey=%x for %s",
		wt, wt->tree, wt->owner, client, obj, keycode, key->aes, key->norm, wt->owner->name));

	fr.no_exit = true;

	/*
	 * Cursor?
	 */
	if (wt->owner->options.xa_objced && wt->e.obj >= 0)
		next_obj = wt->e.obj;
	else
		next_obj = obj;
	
	next_obj = form_cursor(wt, v, keycode, next_obj, redraw, rl);
	
	if (next_obj < 0)
	{
		OBJECT *obtree = wt->tree;

		if (keycode == 0x1c0d || keycode == 0x720d)
		{
			next_obj = ob_find_flst(obtree, OF_DEFAULT, 0, 0, OS_DISABLED, OF_LASTOB, 0);

			DIAG((D_keybd, NULL, "form_keyboard: Got RETRURN key - default obj=%d for %s",
				next_obj, client->name));
		}
		else if (keycode == 0x6100)	/* UNDO */
		{
			next_obj = ob_find_cancel(obtree);

			DIAG((D_keybd, NULL, "form_keyboard: Got UNDO key - cancel obj=%d for %s",
				next_obj, client->name));
		}
		else 
		{
			short ks;
			
			vq_key_s(C.P_handle, &ks);
			if ( (ks & (K_CTRL|K_ALT)) == K_ALT )
				next_obj = ob_find_shortcut(obtree, key->norm & 0x00ff);

			DIAG((D_keybd, NULL, "form_keyboard: shortcut %d for %s",
				next_obj, client->name));
		}

		if (next_obj >= 0)
		{
			struct moose_data md;

			check_mouse(wt->owner, &md.cstate, &md.x, &md.y);
			md.state = MBS_LEFT;
					
			fr.no_exit = form_button(wt, v,
					         next_obj,
					         &md,
					         FBF_REDRAW,
					         rl,
					         &fr.obj_state,
					         &fr.obj,
					         &fr.dblmask);
		}
		else if (keycode != 0x1c0d && keycode != 0x720d)
			next_key = keycode;
	}

	/* Ozk: We return a 'next object' value of -1 when the key was
	 *	not used by form_keybaord() function
	 */
	if (next_obj < 0)
		next_obj = -1;
	
	if (nxtobj)
		*nxtobj = next_obj;
	if (newstate)
		*newstate = wt->tree[next_obj].ob_state;
	if (nxtkey)
		*nxtkey = next_key;

	DIAG((D_keybd, NULL, "form_keyboard: no_exit=%s(%d), nxtobj=%d, nxtkey=%x, obstate=%x, for %s",
		fr.no_exit ? "true" : "false", fr.no_exit, next_obj, next_key, wt->tree[next_obj].ob_state, client->name));

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
	enum locks lock = 0;

	if (wind)
	{
#if 0
		/* XXX - Ozk: complete this someday, having most XaAES-controlled
		 *	 dialog-windows using the same exit code..
		 */
		if (wind == client->alert)
		{
			client->alert == NULL;
			close_window(lock, wind);
			delayed_delete_window(lock, wind);
		}
#endif
		/*else*/ if (wind == client->fmd.wind)
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

			if (wind->send_message && fr->obj >= 0)
			{
				//struct xa_widget *widg = wt->widg;
				short keystate;

				if (fr->md)
					keystate = fr->md->kstate;
				else
					vq_key_s(C.P_handle, &keystate);

				wind->send_message(lock, wind, NULL, AMQ_NORM, QMF_NORM,
						WM_TOOLBAR, 0, 0, wind->handle,
						fr->obj, fr->dblmask ? 2 : 1, keystate, 0);
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
		client->waiting_pb->intout[0] = (fr->obj >= 0 ? fr->obj | fr->dblmask : 0);

	client->usr_evnt = 1;
}
/*
 * WidgetBehaviour()
 * Return is used by do_widgets() to check if state of obj is
 * to be reset or not
 */
bool
Click_windowed_form_do(	enum locks lock,
			struct xa_window *wind,
			struct xa_widget *widg,
			const struct moose_data *md)
{
	struct xa_client *client = wind->owner;
	XA_TREE *wt;
	
	wt = widg->stuff;

	DIAG((D_form, client, "Click_windowed_form_do: client=%lx, wind=%lx, wt=%lx",
		client, wind, wt));

	Click_form_do(lock, client, wind, wt, md);

	return false;
}

/*
 * FormMouseInput()
 * Installed at client->fmd.mousepress and is called by AESSYS
 * when a click happened. Also called by windowed form_do() sessions.
 *
 */
bool
Click_form_do(enum locks lock,
	      struct xa_client *client,
	      struct xa_window *wind,
	      struct widget_tree *wt,
	      const struct moose_data *md)
{
	struct xa_vdi_settings *v;
	OBJECT *obtree = NULL;
	RECT r;

	DIAG((D_form, client, "Click_form_do: %s formdo for %s",
		wind ? "windowed":"classic", client->name));
	/*
	 * If window is not NULL, the form_do is a windowed one,
	 * else it is a classic blocking form_do
	 */
	if (wind)
	{
		v = wind->vdi_settings;

		if (!wind->nolist && !is_topped(wind)) // !wind->nolist && wind != window_list && !(wind->active_widgets & NO_TOPPED) )
		{
			DIAGS(("Click_form_do: topping window"));
			top_window(lock, true, false, wind, (void *)-1L);
			return false;
		}
		
		if (!wt)
		{
			DIAGS(("Click_form_do: using wind->toolbar"));
			wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		}
		if (wt)
			obtree = rp_2_ap(wind, wt->widg, &r);
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
			wt = client->fmd.wt;
		}
		if (wt)
			obtree = wt->tree;
	}

	if (obtree)
	{
		struct fmd_result fr;

		fr.obj = obj_find(wt, 0, 10, md->x, md->y, NULL);
		
		if (fr.obj >= 0 &&
		    !form_button(wt, v,
				 fr.obj,
				 md,
				 FBF_REDRAW,
				 wind ? wind->rect_start : NULL,
				 &fr.obj_state,
				 &fr.obj,
				 &fr.dblmask))
		{
			if (wt->exit_form)
			{
				DIAGS(("Click_form_do: calling exit_form"));
				fr.md = md;
				fr.key = NULL;
				wt->exit_form(client, wind, wt, &fr);
			}
		}
	}
#if GENERATE_DIAGS
	else
		DIAGS(("Click_form_do: NO OBTREE!!"));
#endif

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
Key_form_do(enum locks lock,
	    struct xa_client *client,
	    struct xa_window *wind,
	    struct widget_tree *wt,
	    const struct rawkey *key)
{
	struct xa_vdi_settings *v;
	RECT *clip = NULL;
	OBJECT *obtree = NULL;
	struct xa_rect_list *rl = wind ? wind->rect_start : NULL;
	struct fmd_result fr;
	RECT r;

	fr.no_exit = true;

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

	if (obtree)
	{
		fr.dblmask = 0;

		fr.no_exit = form_keyboard(wt,
					   v,
					   wt->e.obj,
					   key,
					   true,
					   rl,
					   &fr.obj,
					   &fr.obj_state,
					   &fr.aeskey);

		DIAGS(("Key_form_do: form_keyboard - no_exit=%s, obj=%d, objstat=%x, aeskey %x",
			fr.no_exit ? "yes":"no", fr.obj, fr.obj_state, fr.aeskey));

		if (fr.no_exit)
		{
			if (fr.aeskey)
			{
				obj_edit(wt,
					  v,
					  ED_CHAR,
					  wt->e.obj,
					  fr.aeskey,
					  wt->e.pos,
					  true,
					  clip,
					  rl,
					  NULL,
					  NULL);
				fr.aeskey = 0;
				DIAGS(("Key_form_do: obj_edit - edobj=%d, edpos=%d",
					wt->e.obj, wt->e.pos));
			}
			else if (fr.obj >= 0 && wt->e.obj != fr.obj)
			{
				obj_edit(wt, v, ED_END, 0, 0, 0, true, clip, rl, NULL, NULL);
				obj_edit(wt, v, ED_INIT, fr.obj, 0, -1, true, clip, rl, NULL, NULL);
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

	DIAGS(("Key_form_do: returning - no_exit=%s", fr.no_exit ? "yes":"no"));
	return fr.no_exit;
}

static void
dfwm_redraw(struct xa_window *wind, struct xa_widget *widg, struct widget_tree *wt, RECT *clip)
{
	struct xa_vdi_settings *v = wind->vdi_settings;

	if (wt && wt->tree)
	{
		RECT dr;
		struct xa_rect_list *rl;
		
		rl = wind->rect_start;

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
	struct xa_client *to,			/* if different from wind->owner */
	short amq, short qmflags,
	short *msg)
{
	struct xa_vdi_settings *v = wind->vdi_settings;
	XA_WIDGET *widg = wind->tool;
	bool draw = false;

	DIAG((D_form, wind->owner, "do_formwind_msg: wown %s, to %s, wdig=%lx, msg %d, %d, %d, %d, %d, %d, %d, %d",
		wind->owner->name, to->name, widg, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7]));

	if (widg)
	{
		XA_TREE *wt = widg->stuff;
		OBJECT *ob = wt->tree + widg->start;
		short ww = wind->wa.w,			/* window measures */
		      wh = wind->wa.h,
		      dx = wt->dx,			/* object displacement */
		      dy = wt->dy,
		      ow = ob->ob_width,		/* object measures */
		      oh = ob->ob_height;
#if 0
		short wd = ow - ww,			/* space ouside workarea */
		      hd = oh - wh;
#endif		
		switch (msg[0])
		{
		case WM_REDRAW:
		{
			if (!wt->owner->options.xa_objced && wt->e.obj > 0)
			{
				obj_edit(wt, v, ED_END, wt->e.obj, 0, 0, true, &wind->wa, wind->rect_start, NULL, NULL);
			}
			dfwm_redraw(wind, widg, wt, (RECT *)&msg[4]);
			if (!wt->owner->options.xa_objced && wt->e.obj > 0)
			{
				obj_edit(wt, v, ED_END, wt->e.obj, 0, 0, true, &wind->wa, wind->rect_start, NULL, NULL);
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
			if (!wind->nolist && wind != root_window && (wind->window_status & XAWS_OPEN) && !is_topped(wind))
			{
				if (is_hidden(wind))
					unhide_window(0, wind, true);
					
				top_window(0, true, false, wind, (void *)-1L);
			}
			break;
		}
		case WM_BOTTOMED:
		{
			if (!wind->nolist && wind != root_window && (wind->window_status & XAWS_OPEN))
				bottom_window(0, false, true, wind);
			break;
		}
		case WM_ARROWED:
		{
			if (msg[4] < WA_LFPAGE)
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
						dy -= oh - screen.c_max_h;
					break;
					case WA_DNPAGE:
						dy += oh - screen.c_max_h;
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
						dx -= ow - screen.c_max_w;
					break;
					case WA_RTPAGE:
						dx += ow - screen.c_max_w;
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
#if 0
		case WM_SIZED:
		{
			/* if (!wind->nolist && (wind->active_widgets & SIZE)) */
			{
				move_window(0, wind, true, -1, msg[4], msg[5], msg[6], msg[7]);
				ww = wind->wa.w,		/* window measures */
				wh = wind->wa.h,
				wd = ow - ww,			/* space ouside workarea */
				hd = oh - wh;
				if (dx > ow - ww)		/* align */
					dx = ow - ww;
				if (dy > oh - wh)		
					dy = oh - wh;
				XA_slider(wind, XAW_HSLIDE, ow, ww, dx);
				XA_slider(wind, XAW_VSLIDE, oh, wh, dy);
				draw = true;
			}
			break;
		}
#endif
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
			RECT sc;
			wt->dx = dx;
			wt->dy = dy;
			(*v->api->save_clip)(v, &sc);
			display_window(0, 120, wind, NULL);
			dfwm_redraw(wind, widg, wt, NULL);
			(*v->api->restore_clip)(v, &sc);
		}
	}
}
