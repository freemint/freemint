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
	adi_unregister,
	adi_move,
	adi_button,
	adi_wheel,
	0,
};

static long
module_init(void *initfunc, struct kentry *k, struct adiinfo *a)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		"movem.l d3-d7/a3-a6,-(sp);"
		"move.l	%3,-(sp);"
		"move.l	%2,-(sp);"
		"move.l	%1,a0;"
		"jsr	(a0);"
		"addq.l	#8,sp;"
		"movem.l (sp)+,d3-d7/a3-a6;"
		: "=r"(ret)				/* outputs */
		: "g"(initfunc), "r"(k), "r"(a)		/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "a2",		/* clobbered regs */
		  "memory"
	);
	return ret;
}

static long
load_adi(struct basepage *b, const char *name)
{
	void *initfunc = (void *)b->p_tbase;
	long r;
	
	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
	DIAGS(("load_adi: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
// 	display("load_adi: '%s' - text=%lx, data=%lx, bss=%lx", name, b->p_tbase, b->p_dbase, b->p_bbase);

	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	ai.fname = name;
	
	r = module_init(initfunc, KENTRY, &ai);
	if (r == -1L)
		display("kentry updated, %s too old! Please update it", name);

	ai.fname = NULL;
	return r;
}

void
adi_load(bool first)
{
	if (first)
		display("Loading AES Device Drivers:");
	
	load_modules(C.Aes->home_path, ".adi", load_adi);
}
