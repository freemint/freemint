/*
 *	Load AES device drivers (*.adi) from /mint/ and /multitos/.
 *
 *	06/22/94, Kay Roemer.
 */

#include "mint/mint.h"
#include "adi.h"
#include "adiload.h"
#include "xa_global.h"
#include "k_mouse.h"

#include "mint/basepage.h"
//#include "mint/config.h"
//#include "mint/emu_tos.h"

extern struct kentry *kentry;

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
callout_init (void *initfunction, struct kerinfo *k, struct adiinfo *a)
{
	register long ret __asm__("d0");
	
	__asm__ volatile
	(
		"moveml d3-d7/a3-a6,sp@-;"
		"movl	%3,sp@-;"
		"movl	%2,sp@-;"
		"movl   %1,a0;"
		"jsr    a0@;"
		"addqw  #8,sp;"
		"moveml sp@+,d3-d7/a3-a6;"
		: "=r"(ret)				/* outputs */
		: "g"(initfunction), "r"(k), "r"(a)	/* inputs  */
		: "d0", "d1", "d2", "a0", "a1", "a2",   /* clobbered regs */
		  "memory"
	);
	
	return ret;
}

static long
load_adi (struct basepage *b, const char *name)
{
	long r;
	
	DIAGS(("load_adi: enter (0x%lx, %s)", b, name));
	DIAGS(("load_adi: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	/* pass a pointer to the drivers file name on to the
	 * driver.
	 */
	ai.fname = name;
	
	r = callout_init ((void *) b->p_tbase, kentry, &ai);
	
	ai.fname = NULL;
	
	return r;
}

void
adi_load (void)
{
	c_conws ("Loading AES Device Drivers:\r\n");
	DIAGS(("Loading AES Device Drivers:"));
	
	if (!load_modules)
	{
		ALERT (("AESSYS: Loading adi's require an uptodate 1.16 kernel!"));
		return;
	}
	
	load_modules (".adi", load_adi);
}
