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

#include RSCHNAME

#include "xa_global.h"
#include "xa_pdlg.h"
#include "xa_wdlg.h"

#include "k_main.h"
#include "k_mouse.h"

#include "app_man.h"
#include "form.h"
#include "obtree.h"
#include "draw_obj.h"
#include "c_window.h"
#include "rectlist.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "scrlobjc.h"

#include "xa_user_things.h"
#include "nkcc.h"
#include "mint/signal.h"


/*
 * WDIALOG FUNCTIONS (pdlg)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_PDLG
extern struct toolbar_handlers wdlg_th;

static struct xa_window *
get_pdlg_wind(struct xa_client *client, void *pdlg_ptr)
{
	struct xa_pdlg_info *pdlg = lookup_xa_data(&client->xa_data, pdlg_ptr);
	
	if (pdlg)
		return pdlg->wind;
	else
		return NULL;
}

static XA_TREE *
duplicate_pdlg_obtree(struct xa_client *client)
{
	OBJECT *obtree = NULL;
	XA_TREE *wt = NULL;

	if ((obtree = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, WDLG_PDLG), 0)))
	{
		wt = new_widget_tree(client, obtree);
		if (wt)
		{
			/*
			 * We let delete_pdlg_info free the widget_tree
			 */
			wt->flags |= WTF_AUTOFREE | WTF_TREE_ALLOC;
			obtree->ob_state &= ~OS_OUTLINED;
			form_center(obtree, ICON_H);
		}
		else
			free_object_tree(C.Aes, obtree);
	}
	return wt;
}

static void
delete_pdlg_info(void *_pdlg)
{
	struct xa_pdlg_info *pdlg = _pdlg;

	if (pdlg)
	{
		
		kfree(pdlg);
	}
}

static struct xa_pdlg_info *
create_new_pdlg(struct xa_client *client, struct xa_window *wind, struct widget_tree *wt)
{
	struct xa_pdlg_info *pdlg;

	pdlg = kmalloc(sizeof(*pdlg));
	if (pdlg)
	{
		bzero(pdlg, sizeof(*pdlg));
		add_xa_data(&client->xa_data, pdlg, delete_pdlg_info);

		pdlg->handle = (void *)((unsigned long)pdlg >> 16 | (unsigned long)pdlg << 16);
		pdlg->wind = wind;
		pdlg->wt = wt;
		
	}
	return pdlg;
}

unsigned long
XA_pdlg_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg = NULL;
	struct xa_window *wind = NULL;
	OBJECT *obtree = NULL;
	XA_TREE *wt = NULL;
	XA_WIND_ATTR tp = MOVER|NAME;
	RECT r, or;

	DIAG((D_pdlg, client, "XA_pdlg_create"));
	pb->intout[0] = 0;
	pb->addrout[0] = 0L;

	if ((wt = duplicate_pdlg_obtree(client)))
	{
		obtree = wt->tree;
		ob_area(obtree, 0, &or);
		
		r = calc_window(lock, client, WC_BORDER,
				tp, created_for_WDIAL,
				client->options.thinframe,
				client->options.thinwork,
				*(RECT *)&or);

		if (!(wind = create_window(lock,
				     send_app_message,
				     NULL,
				     client,
				     false,
				     tp,
				     created_for_WDIAL,
				     client->options.thinframe,
				     client->options.thinwork,
				     r, NULL, NULL)))
			goto memerr;

		if (!(pdlg = create_new_pdlg(client, wind, wt)))
			goto memerr;

		wt = set_toolbar_widget(lock, wind, client, obtree, -2, 0, &wdlg_th);
		wt->zen = false;

		pb->addrout[0] = (long)pdlg->handle;

		pb->intout[0] = 1;
	}
	else
	{
memerr:
		if (wt)
			remove_wt(wt, true);
		else if (obtree)
			free_object_tree(C.Aes, obtree);

		if (wind)
			delete_window(lock, wind);
		
		if (pdlg)
			kfree(pdlg);
	}
	return XAC_DONE;

}

unsigned long
XA_pdlg_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_delete"));
	
	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		
		if (wind->window_status & XAWS_OPEN)
			close_window(lock, wind);

		delayed_delete_window(lock, wind);
		delete_xa_data(&client->xa_data, pdlg);

		pb->intout[0] = 1;
	}
	return XAC_DONE;
}

static void
get_document_name(struct xa_pdlg_info *pdlg, const char *name)
{
	strncpy(pdlg->document_name, name, 255);
}

unsigned long
XA_pdlg_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_open"));
	
	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		if (!(wind->window_status & XAWS_OPEN))
		{
			struct widget_tree *wt = pdlg->wt;
			RECT r = wind->wa;
			XA_WIND_ATTR tp = wind->active_widgets | MOVER|NAME;
		
			set_toolbar_handlers(&wdlg_th, wind, get_widget(wind, XAW_TOOLBAR), get_widget(wind, XAW_TOOLBAR)->stuff);
			
			if (pb->intin[1] == -1 || pb->intin[2] == -1)
			{
				r.x = (root_window->wa.w - r.w) / 2;
				r.y = (root_window->wa.h - r.h) / 2;
			}
			else
			{
				r.x = pb->intin[1];
				r.y = pb->intin[2];
			}
			{
				RECT or;

				obj_area(wt, 0, &or);
				or.x = r.x;
				or.y = r.y;
				change_window_attribs(lock, client, wind, tp, true, or, NULL);
			}

			pdlg->option_flags = pb->intin[0];
			pdlg->settings = (PRN_SETTINGS *)pb->addrin[1];
			get_document_name(pdlg, (const char *)pb->addrin[2]);
			
			wt->tree->ob_x = wind->wa.x;
			wt->tree->ob_y = wind->wa.y;
			if (!wt->zen)
			{
				wt->tree->ob_x += wt->ox;
				wt->tree->ob_y += wt->oy;
			}
			open_window(lock, wind, wind->rc);
		}
		pb->intout[0] = wind->handle;
	}
	return XAC_DONE;
}

unsigned long
XA_pdlg_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_close"));
	
	pb->intout[0] = 0;

	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		pb->intout[0] = 1;
		pb->intout[1] = pdlg->wt->tree->ob_x;
		pb->intout[2] = pdlg->wt->tree->ob_y;
		close_window(lock, wind);			
	}
	return XAC_DONE;
}

unsigned long
XA_pdlg_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_get"));

	pb->intout[0] = 0;

	switch (pb->intin[0])
	{
		case 0:		/* pdlg_get_setsize */
		{
			*(long *)&pb->intout[0] = sizeof(PRN_SETTINGS);
			break;
		}
	}	
	return XAC_DONE;
}

unsigned long
XA_pdlg_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_set"));

	pb->intout[0] = 0;
	
	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		switch (pb->intin[0])
		{
			case 0:		/* pdlg_add_printers	*/
			{
				break;
			}
			case 1:		/* pdlg_remove_printers	*/
			{
				break;
			}
			case 2:		/* Update		*/
			{
				break;
			}
			case 3:		/* pdlg_add_dialogs	*/
			{
				break;
			}
			case 4:		/* pdlg_remove_dialogs	*/
			{
				break;
			}
			case 5:		/* pdlg_new_settings	*/
			{
				break;
			}
			case 6:		/* pdlg_free_settings	*/
			{
				break;
			}
			case 7:		/* pdlg_dflt_settings	*/
			{
				break;
			}
			case 8:		/* pdlg_validate_settings */
			{
				break;
			}
			case 9:		/* pdlg_use_settings	*/
			{
				break;
			}
			default:;
		}
	}
	return XAC_DONE;
}

static int
check_internal_objects(struct xa_pdlg_info *pdlg, short obj)
{
	OBJECT *obtree = pdlg->wt->tree;
	long id;
	int ret = 0;

	/* return 1 if internal button, 0 otherwise */
	return ret;
}

unsigned long
XA_pdlg_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_evnt"));

	pb->intout[0] = 0;
	
	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		OBJECT *obtree = pdlg->wt->tree;
		struct wdlg_evnt_parms wep;

		wep.wind	= wind;
		wep.wt		= get_widget(wind, XAW_TOOLBAR)->stuff;
		wep.ev		= (EVNT *)pb->addrin[2];
		wep.wdlg	= NULL;
		wep.callout	= NULL;
		wep.redraw	= wdialog_redraw;
		wep.obj		= 0;

		ret = wdialog_event(lock, client, &wep);
		
		if (check_internal_objects(pdlg, wep.obj))
		{
			wep.obj = 0;
			ret = 1;
		}
		else
		{
			if (wep.obj > 0 && (obtree[wep.obj].ob_state & OS_SELECTED))
				obj_change(pdlg->wt, wep.obj, obtree[wep.obj].ob_state & ~OS_SELECTED, obtree[wep.obj].ob_flags, true, &wind->wa, wind->rect_start);

			/* prepare return stuff here */
		}
		pb->intout[1] = wep.obj;
	}
	pb->intout[0] = ret;

	DIAG((D_pdlg, client, " --- return %d, obj=%d",
		pb->intout[0], pb->intout[1]));
	
	return XAC_DONE;
}
static FormKeyInput	Keypress;
static FormExit		Formexit;

static bool
Keypress(enum locks lock,
	 struct xa_client *client,
	 struct xa_window *wind,
	 struct widget_tree *wt,
	 const struct rawkey *key)
{
	bool no_exit;

	if ((no_exit = Key_form_do(lock, client, wind, wt, key)))
	{
		struct scroll_info *list;
		struct xa_pdlg_info *pdlg;
		
		wt = get_widget(wind, XAW_TOOLBAR)->stuff;
		list = object_get_slist(wt->tree + FNTS_FNTLIST);
		pdlg = list->data;
	}
	return no_exit;	
}

static void
Formexit( struct xa_client *client,
	  struct xa_window *wind,
	  XA_TREE *wt,
	  struct fmd_result *fr)
{
	struct xa_pdlg_info *pdlg = (struct xa_pdlg_info *)(object_get_slist(wt->tree + FNTS_FNTLIST))->data;
	
	if (!check_internal_objects(pdlg, fr->obj))
	{
		fnts->exit_button = fr->obj >= 0 ? fr->obj : 0;
		client->usr_evnt = 1;
	}
}

static struct toolbar_handlers pdlg_th =
{
	Formexit,		/* FormExit		*exitform;	*/
	Keypress,		/* FormKeyInput		*keypress;	*/

	NULL,			/* DisplayWidget	*display;	*/
	NULL,			/* WidgetBehaviour	*click;		*/
	NULL,			/* WidgetBehaviour	*dclick;	*/
	NULL,			/* WidgetBehaviour	*drag;		*/
	NULL,			/* WidgetBehaviour	*release;	*/
	NULL,			/* void (*destruct)(struct xa_widget *w); */
};

unsigned long
XA_pdlg_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_pdlg_info *pdlg;
	struct xa_window *wind;

	DIAG((D_pdlg, client, "XA_pdlg_do"));

	pb->intout[0] = 0;
	
	pdlg = (struct xa_pdlg_info *)((unsigned long)pb->addrin[0] >> 16 | (unsigned long)pb->addrin[0] << 16);
	if (pdlg && (wind = get_pdlg_wind(client, pdlg)))
	{
		XA_TREE *wt = pdlg->wt;
		OBJECT *obtree = wt->tree;
		RECT or;
		XA_WIND_ATTR tp = wind->active_widgets & ~STD_WIDGETS;

		form_center(obtree, ICON_H);
		
		ob_area(obtree, 0, &or);

		change_window_attribs(lock, client, wind, tp, true, or, NULL);
		
		set_toolbar_handlers(&pdlg_th, wind, get_widget(wind, XAW_TOOLBAR), get_widget(wind, XAW_TOOLBAR)->stuff);
		wt->flags |= WTF_FBDO_SLIST;
	/* ............. */
		open_window(lock, wind, wind->rc);

		client->status |= CS_FORM_DO;
		Block(client, 0);
		client->status &= ~CS_FORM_DO;
		
		wt->flags &= ~WTF_FBDO_SLIST;

		close_window(lock, wind);
		
		pb->intout[0] = pdlg->exit_button;
		pb->intout[1] = get_cbstatus(fnts->wt->tree);
		*(long *)&pb->intout[2] = fnts->fnts_selected->f.id;
		*(long *)&pb->intout[4] = fnts->fnt_pt;
		*(long *)&pb->intout[6] = fnts->fnt_ratio;
	
	}
	pb->intout[0] = 0;
	return XAC_DONE;
}

#endif
