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
#include "xa_form.h"

#include "objects.h"
#include "c_window.h"
#include "widgets.h"
#include "xa_graf.h"
#include "xa_rsrc.h"
#include "xa_form.h"
#include "objects.h"
#include "scrlobjc.h"

#include "nkcc.h"


/*
 * WDIALOG FUNCTIONS (lbox)
 */

#if LBOX

unsigned long
XA_lbox_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(4,0,8)

	DIAG((D_lbox, client, "XA_lbox_create"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_lbox_update(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,0,2)

	DIAG((D_lbox, client, "XA_lbox_update"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_lbox_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(1,1,1)

	DIAG((D_lbox, client, "XA_lbox_do"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_lbox_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,1)

	DIAG((D_lbox, client, "XA_lbox_delete"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_lbox_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(1,0,1)

	DIAG((D_lbox, client, "XA_lbox_get"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_lbox_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(2,0,2)

	DIAG((D_lbox, client, "XA_lbox_set"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

#endif /* LBOX */
