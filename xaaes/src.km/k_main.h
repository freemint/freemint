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

/*
 * Kernal Message Handler
 */

#ifndef _k_main_h 
#define _k_main_h

#include "global.h"
#include "xa_types.h"

void ceExecfunc(enum locks lock, struct c_event *ce, bool cancel);
// void shutdown_timeout(struct proc *p, long arg);
struct timeout * set_shutdown_timeout(long delta);
void kick_shutdn_if_last_client(void);

void cancel_cevents(struct xa_client *client);
bool CE_exists(struct xa_client *client, void *f);
void cancel_CE(struct xa_client *client, void *f, bool(*callback)(struct c_event *ce, long arg), long arg);

void post_cevent(struct xa_client *client, void (*func)(enum locks, struct c_event *, bool cancel), void *ptr1, void *ptr2, int d0, int d1, const RECT *r, const struct moose_data *md);
short dispatch_cevent(struct xa_client *client);
short dispatch_selcevent(struct xa_client *client, void *f, bool cancel);

//short check_cevents(struct xa_client *client);

void do_block(struct xa_client *client);
void cBlock(struct xa_client *client, int which);
void Block(struct xa_client *client, int which);
void Unblock(struct xa_client *client, unsigned long value, int which);

void multi_intout(struct xa_client *client, short *o, int evnt);
void cancel_evnt_multi(struct xa_client *client, int which);

void TP_entry(void *client);
void TP_terminate(enum locks lock, struct c_event *ce, bool cancel);
void cancel_tpcevents(struct xa_client *client);
void post_tpcevent(struct xa_client *client, void (*func)(enum locks, struct c_event *, bool cancel), void *ptr1, void *ptr2, int d0, int d1, RECT *r, const struct moose_data *md);
short dispatch_tpcevent(struct xa_client *client);

void _cdecl dispatch_shutdown(short flags, unsigned long arg);
void k_main(void *);

extern int aessys_timeout;

#endif /* _k_main_h */
