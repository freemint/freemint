/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann
 *
 * A multitasking AES replacement for MiNT
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

/*
 * Kernal Message Handler
 */

#ifndef _k_main_h 
#define _k_main_h

#include "global.h"
#include "xa_types.h"

int dispatch_cevent(struct xa_client *client);
void Block(struct xa_client *client, int which);
void Unblock(struct xa_client *client, unsigned long value, int which);

void multi_intout(struct xa_client *client, short *o, int evnt);
void cancel_evnt_multi(struct xa_client *client, int which);

void k_main(void *);

Tab *new_task(Tab *new);
void free_task(Tab *, int *);

#endif /* _k_main_h */
