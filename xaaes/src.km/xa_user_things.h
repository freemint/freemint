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

#ifndef _xa_user_things_h
#define _xa_user_things_h

#include "global.h"

/*
 * ATTENTION: DO NOT change structures in this file unless you know
 *	      what you do!!!!
 */
struct xa_user_things
{
	const long len;		/* number of bytes to copy (this struct and the code) */
	long	progdef_p;
	long	userblk_pp;
	long	ret_p;
	long	parmblk_p;
};

struct co_lboxsel_parms
{
	long	box;
	long	tree;
	long	item;
	long	user_data;
	short	obj_index;
	short	last_state;
};

struct co_lboxset_parms
{
	long	box;
	long	tree;
	long	item;
	short	obj_index;
	long	user_data;
	long	rect;
	short	first;
};

struct co_wdlgexit_parms
{
	long	dialog;
	long	evnt;
	short	obj;
	short	clicks;
	long	data;
};

struct co_pdlg_sub_parms
{
	long settings;
	long sub;
	short obj;
};


struct xa_callout_parms
{
	long	ret;
	long	func;
	short	plen;
	short	parms;
};
struct xa_callout_head
{
	const long len;
	long	sighand_p;
	struct	xa_callout_parms *parm_p;
};

extern const struct xa_user_things	xa_user_things;
extern const struct xa_callout_head	xa_callout_user;

#endif /* _xa_user_things_h */
