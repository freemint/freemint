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

#include "xa_fslx.h"


/*
 * WDIALOG FUNCTIONS (fslx)
 *
 * documentation about this can be found in:
 *
 * - the GEMLIB documentation:
 *   http://arnaud.bercegeay.free.fr/gemlib/
 *
 * - the MagiC documentation project:
 *   http://www.bygjohn.fsnet.co.uk/atari/mdp/
 */

#if WDIALOG_FSLX

unsigned long
XA_fslx_open(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_open"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fslx_close(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_close"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fslx_getnxtfile(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_getnxtfile"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fslx_evnt(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_evnt"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fslx_do(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_do"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

unsigned long
XA_fslx_set_flags(enum locks lock, struct xa_client *client, AESPB *pb)
{
	DIAG((D_fslx, client, "XA_fslx_set_flags"));

	pb->intout[0] = 0;
	return XAC_DONE;
}

#endif /* WDIALOG_FSLX */
