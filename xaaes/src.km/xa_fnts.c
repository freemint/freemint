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
#include "xa_fnts.h"

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
 * WDIALOG FUNCTIONS (fnts)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_FNTS

static struct xa_fnts_info *
new_fnts(void)
{
	struct xa_fnts_info *new;

	new = kmalloc(sizeof(*new));
	if (new)
	{
		bzero(new, sizeof(*new));
	}
	return new;
}

static struct xa_window *
get_fnts_wind(struct xa_client *client, void *fnts_ptr)
{
	struct xa_window *wl = window_list;

	while (wl)
	{
		if (wl->owner == client && (long)wl->data == (long)fnts_ptr)
			break;
		wl = wl->next;
	}
	return wl;
}

unsigned long
XA_fnts_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts = NULL;
	struct xa_window *wind = NULL;
	OBJECT *obtree = NULL;
	XA_TREE *wt = NULL;
	char *s, *d;
	long slen;
	XA_WIND_ATTR tp = MOVER|NAME;
	RECT r, or;

	DIAG((D_fnts, client, "XA_fnts_create"));

	pb->intout[0] = 0;
	
	if ((obtree = duplicate_obtree(C.Aes, ResourceTree(C.Aes_rsc, WDLG_FNTS), 0)))
	{
		wt = new_widget_tree(client, obtree);
		if (!wt)
			goto memerr;
		
		if (!(fnts = new_fnts()))
			goto memerr;
		
		wt->flags |= WTF_TREE_ALLOC | WTF_AUTOFREE;

		obtree->ob_state &= ~OS_OUTLINED;

		form_center(obtree, ICON_H);

		ob_area(obtree, 0, &or);

		r = calc_window(lock, client, WC_BORDER,
				tp,
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

		wt = set_toolbar_widget(lock, wind, client, obtree, -2, 0);
		
		fnts->wind		= wind;
		fnts->handle		= (void *)((unsigned long)fnts >> 16 | (unsigned long)fnts << 16);
		fnts->wt		= wt;
		fnts->vdi_handle	= pb->intin[0];
		fnts->num_fonts		= pb->intin[1];
		fnts->font_flags	= pb->intin[2];
		fnts->dialog_flags	= pb->intin[3];
		
		d = NULL;
		if ((s = (char *)pb->addrin[0]))
		{
			slen = strlen(s) + 1;
			d = kmalloc(slen);
			if (d) strcpy(d, s);
		}
		fnts->sample_text = d;
		d = NULL;
		if ((s = (char *)pb->addrin[1]))
		{
			slen = strlen(s) + 1;
			d = kmalloc(slen);
			if (d) strcpy(d, s);
		}
		fnts->opt_button = d;

		wind->data = fnts;

		pb->intout[0] = 1;
	}
	else
	{
memerr:
		if (wt)
			remove_wt(wt);
		else if (obtree)
			free_object_tree(C.Aes, obtree);
		if (fnts)
			kfree(fnts);
	}
	return XAC_DONE;
}

unsigned long
XA_fnts_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fnts, client, "XA_fnts_delete"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fnts_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	DIAG((D_fnts, client, "XA_fnts_open"));

	pb->intout[0] = 0;

	fnts = (struct xa_fnts_info *)((long)pb->addrin[0] >> 16 | (long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		struct widget_tree *wt = fnts->wt;

		wt->tree->ob_x = wind->wa.x;
		wt->tree->ob_y = wind->wa.y;
		open_window(lock, wind, wind->rc);
		pb->intout[0] = 1;
	}	
	return XAC_DONE;
}

unsigned long
XA_fnts_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	struct xa_fnts_info *fnts;
	struct xa_window *wind;
	
	DIAG((D_fnts, client, "XA_fnts_close"));

	pb->intout[0] = 0;
	fnts = (struct xa_fnts_info *)((long)pb->addrin[0] >> 16 | (long)pb->addrin[0] << 16);
	if (fnts && (wind = get_fnts_wind(client, fnts)))
	{
		close_window(lock, wind);
		pb->intout[0] = 1;
	}
	return XAC_DONE;
}

unsigned long
XA_fnts_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fnts, client, "XA_fnts_get"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fnts_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fnts, client, "XA_fnts_set"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fnts_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fnts, client, "XA_fnts_evnt"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fnts_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fnts, client, "XA_fnts_do"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

#endif /* WDIALOG_FNTS */
