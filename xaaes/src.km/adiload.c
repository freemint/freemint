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

static long
load_adi(struct basepage *b, const char *name)
{
	long (*init)(struct kentry *, struct adiinfo *);
	long r;
	
	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
	DIAGS(("load_adi: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
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
	c_conws("Loading AES Device Drivers:\r\n");
	DIAGS(("Loading AES Device Drivers:"));
	
	if (!load_modules)
	{
		ALERT(("AESSYS: Loading adi's require an uptodate 1.16 kernel!"));
		return;
	}
	
	load_modules(".adi", load_adi);
}
