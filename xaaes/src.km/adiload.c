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

#include "adiload.h"
#include "xa_global.h"

#include "adi.h"
#include "k_mouse.h"

#include "mint/basepage.h"


static struct adiinfo ai =
{
	adi_getfreeunit,
	adi_register,
	adi_move,
	adi_button,
	adi_wheel,
	0,
};

static void *	 
module_init(void *initfunc, struct kentry *k, struct adiinfo *a)
{
	register void *ret __asm__("d0");	 

	__asm__ volatile	 
	(	 
		"moveml	d3-d7/a3-a6,sp@-;"	 
		"movl	%3,sp@-;"	 
		"movl	%2,sp@-;"	 
		"movl	%1,a0;"	 
		"jsr	a0@;"	 
		"addqw	#8,sp;"	 
		"moveml	sp@+,d3-d7/a3-a6;"	 
		: "=r"(ret)				/* outputs */	 
		: "g"(initfunc), "r"(k), "r"(a)		/* inputs  */	 
		: "d0", "d1", "d2", "a0", "a1", "a2",	/* clobbered regs */	 
		"memory"	 
	);	 

	return ret;	 
}

static long
load_adi(struct basepage *b, const char *name)
{
	long (*init)(struct kentry *, struct adiinfo *);
	long r;
	
	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
	DIAGS(("load_adi: init 0x%lx, size %li", (void *)b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	ai.fname = name;
	
	init = (long (*)(struct kentry *, struct adiinfo *))b->p_tbase;
	r = (*init)(KENTRY, &ai);
	
	ai.fname = NULL;
	
	return r;
}

void
adi_load(void)
{
	display("Loading AES Device Drivers:");
	
	load_modules(C.Aes->home_path, ".adi", load_adi);
}
