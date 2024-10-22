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

/*
 * Kernal Message Handler
 */

#ifndef _k_main_h
#define _k_main_h

#include "global.h"
#include "xa_types.h"

void ceExecfunc(int lock, struct c_event *ce, short cancel);

struct timeout * set_shutdown_timeout(long delta);
void kick_shutdn_if_last_client(void);

void cancel_cevents(struct xa_client *client);
bool CE_exists(struct xa_client *client, void *f);
void cancel_CE(struct xa_client *client, void *f, bool(*callback)(struct c_event *ce, long arg), long arg);

void post_cevent(struct xa_client *client, void (*func)(int, struct c_event *, short cancel), void *ptr1, void *ptr2, int d0, int d1, const GRECT *r, const struct moose_data *md);
short dispatch_selcevent(struct xa_client *client, void *f, bool cancel);


void do_block(struct xa_client *client);
void cBlock(struct xa_client *client);
void Block(struct xa_client *client);
void Unblock(struct xa_client *client, unsigned long value);

void multi_intout(struct xa_client *client, short *o, int evnt);
void cancel_evnt_multi(struct xa_client *client);

void TP_entry(void *client);
void TP_terminate(int lock, struct c_event *ce, bool cancel);
void cancel_tpcevents(struct xa_client *client);
void post_tpcevent(struct xa_client *client, void (*func)(int, struct c_event *, bool cancel), void *ptr1, void *ptr2, int d0, int d1, GRECT *r, const struct moose_data *md);
short dispatch_tpcevent(struct xa_client *client);

void _cdecl dispatch_shutdown(short flags);
void _cdecl ce_dispatch_shutdown(int l, struct xa_client *client, short b);
void load_grd( void *fn );
void load_bkg_img( void *fn );
void load_palette( void *fn );
void k_main(void *);

extern int aessys_timeout;
extern int ferr;

#if CHECK_STACK
/* Read the current stack pointer value */
static __inline__ void* get_sp(void)
{
     register void* ret __asm__("sp");
     return ret;
}
#endif

/* Set the user stack pointer */
static __inline__ void set_usp(void *p)
{
	__asm__ volatile
	(
		"move.l	%0,%%usp"
	:		/* outputs */
	: "a"(p)	/* inputs */
	: "memory"	/* clobbered regs */
	);
}

#endif /* _k_main_h */
