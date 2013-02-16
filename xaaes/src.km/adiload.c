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
#include "mint/arch/asm.h"

static char no_reason[] = "Nothing";

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
module_init(void *initfunc, struct kentry *k, struct adiinfo *a, long reason)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		PUSH_SP("d3-d7/a3-a6", 36)
		"move.l	%4,-(sp)\n\t"
		"move.l	%3,-(sp)\n\t"
		"move.l	%2,-(sp)\n\t"
		"move.l	%1,a0\n\t"
		"jsr	(a0)\n\t"
		"lea	12(sp),sp\n\t"
		POP_SP("d3-d7/a3-a6", 36)
		: "=r"(ret)					/* outputs */
		: "g"(initfunc), "r"(k), "r"(a), "g"(reason)	/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "a2",		/* clobbered regs */
		  "memory"
	);
	return ret;
}

static long
load_adi(struct basepage *b, const char *name, short *class, short *subclass)
{
	void *initfunc = (void *)b->p_tbase;
	char *reason = no_reason;
	long r;

	BLOG((0,"load_adi: enter (0x%lx, 0x%lx, %s)", initfunc, b, name));
	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
	DIAGS(("load_adi: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
// 	display("load_adi: '%s' - text=%lx, data=%lx, bss=%lx", name, b->p_tbase, b->p_dbase, b->p_bbase);

	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	ai.fname = name;

	*class = MODCLASS_KMDEF;
	*subclass = 0;

	r = module_init(initfunc, KENTRY, &ai, (long)&reason);

	if (r == -1L)
	{
		char *s = "";
		if( !strcmp( reason, "Nothing" ) )
			s = " (possibly from other kernel)";
		display("Module %s error, reason: %s%s", name, reason, s);
// 		display("kentry updated, %s too old! Please update it", name);
	}

	ai.fname = NULL;
	BLOG((0,"load_adi: return %ld", r));
	return r;
}

void
adi_load(bool first)
{
	if (first)
		BLOG((0,"Loading AES Device Drivers:"));

	load_modules(C.Aes->home_path, ".adi", load_adi);
	BLOG((0,"adi_load: done"));
}
/* *************************************************************************************** */
static long
xam_init(void *initfunc, struct kentry *k, struct xa_module_api *a, long arg, long reason)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		PUSH_SP("d3-d7/a3-a6", 36)
		"move.l	%5,-(sp)\n\t"
		"move.l	%4,-(sp)\n\t"
		"move.l	%3,-(sp)\n\t"
		"move.l	%2,-(sp)\n\t"
		"move.l	%1,a0\n\t"
		"jsr	(a0)\n\t"
		"lea	16(sp),sp\n\t"
		POP_SP("d3-d7/a3-a6", 36)
		: "=r"(ret)						/* outputs */
		: "g"(initfunc), "r"(k), "r"(a), "r"(arg), "g"(reason)	/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "a2",		/* clobbered regs */
		  "memory"
	);
	return ret;
}

extern struct xa_module_api xam_api;

static long
load_xam(struct basepage *b, const char *name, short *class, short *subclass)
{
	void *initfunc = (void *)b->p_tbase;
	long r;
	char *reason = no_reason;

	DIAGS(("load_xam: enter (0x%lx, %s)", b, name));
	DIAGS(("load_xam: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
	BLOG((false, "load_xam: '%s' - text=%lx, data=%lx, bss=%lx", name, b->p_tbase, b->p_dbase, b->p_bbase));

	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	*class = MODCLASS_KMDEF;
	*subclass = 2;

	r = xam_init(initfunc, KENTRY, &xam_api, 0, (long)&reason);
	if (r == -1L)
		BLOG((true, "load_xam: module '%s' says '%s'", name, reason));
	return r;
}

void
xam_load(bool first)
{
	char *path;
	short plen;

	plen = strlen(C.Aes->home_path);
	path = kmalloc(plen + 10);
	if (path)
	{
		char c;

		strcpy(path, C.Aes->home_path);
		c = path[plen - 1];
		if (!(c == '/' || c == '\\')) {
			path[plen++] = '\\';
			path[plen] = '\0';
		}
		strcat(path, "xam");

		if (first)
			BLOG((false, "Loading AES modules..."));
		load_modules(path, ".xam", load_xam);
		kfree(path);
	}
}
