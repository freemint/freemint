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

#ifndef _xa_user_things_h
#define _xa_user_things_h

#include "global.h"

struct xa_user_things
{
	const long len;		/* number of bytes to copy (this struct and the code) */
	long progdef_p;
	long userblk_pp;
	long ret_p;
	long parmblk_p;
};

struct xa_co_wdlgexit
{
	const long len;
	long	sighand_p;
	long	exit_p;
	long	userdata_p;
	long	mclicks_p;
	long	nxtobj_p;
	long	ev_p;
	long	handle_p;
	long	ret_p;
	long	feedback_p;
};

struct xa_co_lbox
{
	const long len;
	long	sighand_p;
	long	parm_p;
};

struct co_lboxsel_parms
{
	long	funct;
	short	last_state;
	short	obj_index;
	long	user_data;
	long	item;
	long	tree;
	long	box;
};

struct co_lboxset_parms
{
	long	funct;
	short	first;
	long	rect;
	long	user_data;
	short	obj_index;
	long	item;
	long	tree;
	long	box;
	long	ret;
};

struct co_lboxscrl_parms
{
	long	funct;
	short	n;
	long	lbox_slide;
	long	box;
	long	ret;
};

extern const struct xa_user_things xa_user_things;
extern const struct xa_co_wdlgexit xa_co_wdlgexit;

extern const struct xa_co_lbox xa_co_lboxselect;
extern const struct xa_co_lbox xa_co_lboxset;
extern const struct xa_co_lbox xa_co_lboxscroll;

#endif /* _xa_user_things_h */
