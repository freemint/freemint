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
			 * We let delete_fnts_info free the widget_tree
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

static struct xa_pdlg_info *
create_new_pdlg(struct xa_client *client)
{
	return NULL;
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

		if (!(pdlg = create_new_pdlg(client)))
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
	DIAG((D_pdlg, client, "XA_pdlg_delete"));
	
	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_open"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_close"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_get"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_set"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_evnt"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_pdlg_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_pdlg, client, "XA_pdlg_do"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

#endif
