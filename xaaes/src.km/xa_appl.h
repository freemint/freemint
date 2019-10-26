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

#ifndef _xa_appl_h
#define _xa_appl_h

#include "global.h"
#include "xa_types.h"

void	init_client_mdbuff(struct xa_client *client);
void	client_nicename(struct xa_client *client, const char *n, bool autonice);

struct xa_client *init_client(int lock, bool sysclient);

bool is_client(struct xa_client *client);
int exit_proc(int lock, struct proc *proc, int code);
void exit_client(int lock, struct xa_client *client, int code, bool pexit, bool detach);

extern short info_tab[][4];

AES_function
	XA_appl_init,
	XA_appl_exit,
	XA_appl_yield,
	XA_appl_search,
	XA_appl_write,
	XA_appl_getinfo,
	XA_appl_options,
	XA_appl_find,
	XA_appl_control,
	XA_appl_trecord,
	XA_appl_tplay;

#endif /* _xa_appl_h */
