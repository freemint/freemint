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

#if WDIALOG_PDLG

unsigned long
XA_pdlg_create(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_delete(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_get(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_set(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

unsigned long
XA_pdlg_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
}

#endif
