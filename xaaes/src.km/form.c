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
	if (!obj_edit(wt, ED_INIT, edobj, 0, -1, false, NULL, NULL, &new_obj))
		obj_edit(wt, ED_INIT, new_obj, 0, -1, false, NULL, NULL, NULL);

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
		wt = set_toolbar_widget(lock, wind, obtree, edobj);
		wt->zen = false; //true;
	}
	/*
	 * Should this client do classic blocking form_do's?
	 */
#if 0
	else if (client->fmd.lock)
	{
		DIAG((D_form, client, "Setup_form_do: nonwindowed for %s", client->name));
		Set_form_do(client, obtree, edobj);
		wt = client->fmd.wt;
		goto okexit;
	}
#endif
	/*
	 * First time this client does a form_do/dial, do some preps
	 */
	else
	{
		RECT r;

		DIAG((D_form, client, "Setup_form_do: Create window for %s", client->name));
		ob_area(obtree, 0, &r);

		client->fmd.r = calc_window(lock,
					    client,
					    WC_BORDER,
					    kind,
					    MG,
					    0,
					    C.Aes->options.thinwork,
					    r);
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
		RECT r = client->fmd.r;

		DIAGS(("Setup_form_do: create window"));

		kind |= (TOOLBAR | USE_MAX);
		wind = create_window(lock,
				     do_winmesag, //handle_form_window,
				     do_formwind_msg,
				     client,
				     false,
				     kind,
				     client->fmd.state ? created_for_FMD_START : created_for_FORM_DO,
				     MG,
				     0, //C.Aes->options.thinframe,
				     C.Aes->options.thinwork,
				     r,
				     NULL,
				     NULL);
		if (wind)
		{
			client->fmd.wind = wind;
			wt = set_toolbar_widget(lock, wind, obtree, edobj);
			wt->zen = false;
		}
		else
		{
			client->fmd.wind = NULL;
		/* XXX - Ozk:
		 *     Force a classic formdo here if allocating resources for a 
		 *     window fails... on the todo list
		 */
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

/*
 * Ozk: Rewritten totally. Returns false if a TOUCHEXIT or EXIT button was selected.
 * puts a 0 in 'nxtob' if the object is not an [TOUCH]EXIT or EDITABLE. If object is
 * any of the two, its passed back in 'nxtob'.
 */
bool
form_button(XA_TREE *wt,
	    short obj,
	    const struct moose_data *md,
	    bool redraw,
	    struct xa_rect_list *rl,
	    /* Outputs */
	    short *newstate,
	    short *nxtob,
	    short *clickmsk)
{
	//enum locks lock = 0;
	OBJECT *obtree = wt->tree;
	short next_obj = 0;
	short flags, state;
	bool no_exit = true, dc;

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

		if ((obtree[obj].ob_type & 0xff) == G_SLIST)
		{
			no_exit = false;		
		}
		else if (flags & OF_RBUTTON)
		{
			DIAGS(("form_button: call obj_set_radio_button"));
			obj_set_radio_button(wt, obj, redraw, rl);
		}
		else
		{
			if (redraw)
			{
				DIAGS(("form_button: call obj_watch"));
				obj_watch(wt, obj, state^OS_SELECTED, state, rl);
			}
			else
			{
				DIAGS(("form_button: switch state"));
				obj_change(wt, obj, state^OS_SELECTED, flags, redraw, rl);
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
		*clickmsk = dc ? 0x8000:0;

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
	if (o >= 0 && o != obj)
	{	
		/* If edit field has changed, update the screen */
		obj_edit(wt, ED_END, 0, 0, 0, redraw, rl, NULL, NULL);
		obj_edit(wt, ED_INIT, o, 0, -1, redraw, rl, NULL, NULL);
	}
	return o;
}
/*
 * Returns false of exit/touchexit object, true otherwise
 */
bool
form_keyboard(XA_TREE *wt,
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
	if (wt->e.obj >= 0)
		next_obj = wt->e.obj;
	else
		next_obj = obj;
	
	next_obj = form_cursor(wt, keycode, next_obj, redraw, rl);
	
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
			vq_key_s(C.vh, &ks);
			if ( (ks & (K_CTRL|K_ALT)) == K_ALT )
				next_obj = ob_find_shortcut(obtree, key->norm);

			DIAG((D_keybd, NULL, "form_keyboard: shortcut %d for %s",
				next_obj, client->name));
		}

		if (next_obj >= 0)
		{
			struct moose_data md;

			check_mouse(wt->owner, &md.cstate, &md.x, &md.y);
			md.state = MBS_RIGHT;
					
			fr.no_exit = form_button(wt,
					         next_obj,
					         &md,
					         redraw,
					         rl,
					         &fr.obj_state,
					         &fr.obj,
					         &fr.dblmask);
			/* Ozk:
			 * If form_button() returned a new object-of-focus,
			 * return it only if it differs from the one that was
			 * passed here.
			 */
#if 0
			if (fr.obj > 0)
				next_obj = fr.obj;
			else
				next_obj = obj;
#endif
		}
		else if (keycode !=0x1c0d && keycode != 0x720d)
			next_key = keycode;
	}

	if (next_obj < 0)
		next_obj = obj;

	if (nxtobj)
		*nxtobj = next_obj;
	if (newstate)
		*newstate = wt->tree[next_obj].ob_state;
	if (nxtkey)
		*nxtkey = next_key;

	DIAG((D_keybd, NULL, "form_keyboard: no_exit=%s, nxtobj=%d, nxtkey=%x, obstate=%x, for %s",
		fr.no_exit ? "true" : "false", next_obj, next_key, wt->tree[next_obj].ob_state, client->name));

	return fr.no_exit;
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
		if (wind == client->fmd.wind)
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

			if (wind->send_message)
			{
				struct xa_widget *widg = wt->widg;
				wind->send_message(lock, wind, NULL,
						WM_TOOLBAR, 0, 0, wind->handle,
						fr->obj, fr->dblmask ? 2 : 1, widg->k, 0);
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
		client->waiting_pb->intout[0] = fr->obj | fr->dblmask;
	client->usr_evnt = 1;
}
#if 0
/*
 * XXX - Ozk:
 *     Redo this as soon as possible!
 */
void
XA_form_exit( struct xa_client *client,
	      struct xa_window *wind,
	      XA_TREE *wt,
	      struct fmd_result *fr)
{
	
	if (os != -1)
	{
		wt->tree[f].ob_state = os;
		redraw_object(lock, wt, f);
	}
	wt->which = 0;			/* After redraw of object. :-) */
	wt->current = f|dbl;		/* pass the double click to the internal handlers as well. */
	wt->exit_handler(lock, wt);	/* XaAES application point of view. */
}	

void
exit_toolbar(struct xa_client *client,
	     struct xa_window *wind,
	     XA_TREE *wt,
	     struct fmd_result *fr)
{
	if (wind->send_message)
		wind->send_message(lock, wind, NULL,
				   WM_TOOLBAR, 0, 0, wind->handle,
				   f, dbl ? 2 : 1, widg->k, 0);
}
#endif

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
	OBJECT *obtree;
	RECT r;

	DIAG((D_form, client, "Click_form_do: %s formdo for %s",
		wind ? "windowed":"classic", client->name));
	/*
	 * If window is not NULL, the form_do is a windowed one,
	 * else it is a classic blocking form_do
	 */
	if (wind)
	{
		if (wind != window_list && !(wind->active_widgets & NO_TOPPED) )
		{
			DIAGS(("Click_form_do: topping window"));
			top_window(lock, wind, 0);
			after_top(lock, false);
			return false;
		}

		if (!wt)
		{
			DIAGS(("Click_form_do: using wind->toolbar"));
			wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		}
		obtree = rp_2_ap(wind, wt->widg, &r);
	}
	/*
	 * Not a windowed form session.
	 */
	else
	{
		if (!wt)
		{
			DIAGS(("Click_form_do: using client->wt"));
			wt = client->fmd.wt;
		}
		obtree = wt->tree;
	}

	if (obtree)
	{
		struct fmd_result fr;

		fr.obj = ob_find(obtree, 0, 10, md->x, md->y);
		
		if (fr.obj >= 0 &&
		    !form_button(wt,
				 fr.obj,
				 md,
				 true,
				 wind ? wind->rect_start : NULL,
				 &fr.obj_state,
				 &fr.obj,
				 &fr.dblmask))
		{
			if (wt->exit_form)
			{
				DIAGS(("Click_form_do: calling exti_form"));
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
	OBJECT *obtree;
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
	}

	if (obtree)
	{
		fr.dblmask = 0;

		fr.no_exit = form_keyboard(wt,
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
					  ED_CHAR,
					  wt->e.obj,
					  fr.aeskey,
					  wt->e.pos,
					  true,
					  rl,
					  NULL,
					  NULL);
				fr.aeskey = 0;
				DIAGS(("Key_form_do: obj_edit - edobj=%d, edpos=%d",
					wt->e.obj, wt->e.pos));
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

/* big simplification by constructing function do_active_widget()
 * This eliminates redrawing of the sliders when the mouse isnt moved.
 */
struct woken_active_widget_data
{
	struct xa_client *client;
	enum locks lock;
};

static void
woken_active_widget(struct proc *p, long arg)
{
	struct woken_active_widget_data *data = (struct woken_active_widget_data *)arg;

	do_active_widget(data->lock, data->client);
	kfree(data);
}

void
set_button_timer(enum locks lock, struct xa_window *wind)
{
	short exit_mb, x, y;

	check_mouse(wind->owner, &exit_mb, &x, &y);

	/* still down? */
	if (exit_mb)
	{
		struct woken_active_widget_data *data;

		data = kmalloc(sizeof(*data));
		if (data)
		{
			struct timeout *t;

			t = addroottimeout(50, woken_active_widget, 0);
			if (t)
			{
				data->client = wind->owner;
				data->lock = lock;

				t->arg = (long)data;
			}
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
	short *msg)
{
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
			if (wt && wt->tree)
			{
				RECT *clip = (RECT *)&msg[4];
				RECT dr;
				struct xa_rect_list *rl;

				if ((rl = wind->rect_start))
				{
					if (wind->owner != C.Aes)
						lock_screen(wind->owner, false, NULL, 1);
					hidem();
					while (rl)
					{
						if (xa_rect_clip(clip, &rl->r, &dr))
						{
							set_clip(&dr);
							draw_object_tree(0, wt, wt->tree, 0, 10, 1);
						}
						rl = rl->next;
					}
					clear_clip();
					showm();
					if (wind->owner != C.Aes)
						unlock_screen(wind->owner, 2);
				}
			}
			break;
		}
		case WM_MOVED:
		{
			if (wt && wt->tree)
			{
				move_window(0, wind, -1, msg[4], msg[5], msg[6], msg[7]);
			}
			break;
		}
		case WM_TOPPED:
		{
			if (wind != root_window && wind->is_open && !is_topped(wind))
			{
				if (is_hidden(wind))
					unhide_window(0, wind);
				top_window(0, wind, 0);
				swap_menu(0, wind->owner, true, 0);
				after_top(0, true);
			}
			break;
		}
		case WM_BOTTOMED:
		{
			if (wind != root_window && wind->is_open)
				bottom_window(0, wind);
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
			} else
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
			break;
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
				move_window(0, wind, -1, msg[4], msg[5], msg[6], msg[7]);
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
			wt->dx = dx;
			wt->dy = dy;
			display_window(0, 120, wind, NULL);
		}
		set_button_timer(0, wind);
	}
}
