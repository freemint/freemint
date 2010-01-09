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

extern struct kernel_module *self;
extern struct xa_api xa_api;

static struct adiinfo ai =
{
	"AdiInfo        \0",
	adi_getfreeunit,
	adi_register,
	adi_unregister,
	adi_move,
	adi_button,
	adi_wheel,
};
/*
 * This is called by the kernels module framework when the module is loaded
 * and all (or some) of the modules devices have been probed.
 * We call the attach method now to make the modules register itself with
 * XaAES.
 */
static long
load_adi(struct kernel_module *km, const char *name)
{
	long r = E_OK;
	device_t dev;

// 	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
// 	DIAGS(("load_adi: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
// 	display("load_adi: '%s' - text=%lx, data=%lx, bss=%lx", name, b->p_tbase, b->p_dbase, b->p_bbase);

	km->class = MODCLASS_KMDEF;
	TAILQ_FOREACH(dev, &km->devices, next) {
		device_attach(dev);
	}
	return r;
}

/*
 * Called by AESSYS itself (init.c) to load AES Device drivers (moose.adm)
 */
void
adi_load(bool first)
{
	if (first)
		display("Loading AES Device Drivers:");

	load_kmodules(self, C.Aes->home_path, ".adm", &ai, load_adi);
}

/*
 * Called by AESSYS when its time to load AES modules.
 * This invokes the kernels module framework to load
 * modules for us.
 */
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
		load_kmodules(self, path, ".xam", &xa_api, load_adi);
		kfree(path);
	}
}
