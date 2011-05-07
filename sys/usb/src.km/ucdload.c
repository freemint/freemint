/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "global.h"
#include "ucdload.h"
#include "usb.h"
#include "hub.h"
#include "ucd.h"

#include "mint/basepage.h"

extern Path start_path;

static char no_reason[] = "Nothing";

static struct ucdinfo ai =
{
	ucd_getfreeunit,
	ucd_register,
	ucd_unregister,
	usb_maxpacket,
	usb_rh_wakeup,
	0,
};

static long
module_init(void *initfunc, struct kentry *k, struct ucdinfo *a, long reason)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		"lea     -45(sp),sp;"
		"movem.l d3-d7/a3-a6,(sp);"
		"move.l %4,-(sp);"
		"move.l	%3,-(sp);"
		"move.l	%2,-(sp);"
		"move.l	%1,a0;"
		"jsr	(a0);"
		"lea	12(sp),sp;"
		"movem.l (sp),d3-d7/a3-a6;"
		"lea    45(sp),sp;"
		: "=r"(ret)					/* outputs */
		: "g"(initfunc), "r"(k), "r"(a), "g"(reason)	/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "a2",			/* clobbered regs */
		  "memory"
	);
	return ret;
}

static long
load_ucd(struct basepage *b, const char *name, short *class, short *subclass)
{
	void *initfunc = (void *)b->p_tbase;
	char *reason = no_reason;
	long r;

//	display("load_ucd: enter (0x%lx, 0x%lx, %s)", initfunc, b, name);
	DEBUG(("load_ucd: enter (0x%lx, %s)", b, name));
	DEBUG(("load_ucd: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
// 	display("load_ucd: '%s' - text=%lx, data=%lx, bss=%lx", name, b->p_tbase, b->p_dbase, b->p_bbase);

	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	ai.fname = name;

	*class = MODCLASS_KMDEF;
	*subclass = 0;

	r = module_init(initfunc, KENTRY, &ai, (long)&reason);

	if (r == -1L)
	{
		ALERT(("Module %s error, reason: %s", name, reason));
// 		display("kentry updated, %s too old! Please update it", name);
	}

	ai.fname = NULL;
	DEBUG(("load_ucd: return %ld", r));
	return r;
}

void
ucd_load(bool first)
{
	if (first)
		c_conws("Loading USB host controller driver:\r\n");

	load_modules(start_path, ".ucd", load_ucd);
	DEBUG(("ucd_load: done"));
}

