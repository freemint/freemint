/*
 * $Id$
 *
 * Keyboard handling stuff
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 *
 */

/* PROGRAMS WHICH HAVE TO BE TESTED WHETHER THERE IS COMPATIBILITY PROBLEM:
 *
 * a) Apex Media
 * b) JML Snap
 * c) Midnight Commander (!)
 * d) something else I forgot
 *
 */

# include "libkern/libkern.h"	/* strcpy(), strcat(), ksprintf() */

# include "mint/signal.h"	/* SIGQUIT */
# include "mint/mint.h"		/* FATAL() */

# include "arch/intr.h"		/* click */

# include "bios.h"		/* kbshft, kintr, *keyrec, ...  */
# include "biosfs.h"		/* struct tty */
# include "cnf.h"		/* init_env */
# include "console.h"		/* c_conws() */
# include "cookie.h"		/* get_cookie(), set_cookie() */
# include "debug.h"		/* do_func_key() */
# include "dev-mouse.h"		/* mshift */
# include "dos.h"		/* s_hutdown() */
# include "dossig.h"		/* p_kill() */
# include "global.h"		/* tosver, mch, *sysdir */
# include "init.h"		/* boot_printf() */
# include "info.h"		/* messages */
# include "k_exec.h"		/* sys_pexec() */
# include "k_fds.h"		/* fp_alloc() */
# include "keyboard.h"		/* struct cad */
# include "memory.h"		/* get_region(), attach_region() */
# include "proc.h"		/* rootproc */
# include "random.h"		/* add_keyboard_randomness() */
# include "signal.h"		/* killgroup() */
# include "timeout.h"		/* addroottimeout() */

# include <osbind.h>		/* Keytbl() */

# define CAD_TIMEOUT	5*200
# define ROOT_TIMEOUT	1

/* The _AKP cookie value consists of:
 *
 * bits 0-7	keyboard nationality
 * bits 8-15	desktop nationality
 *
 * Actually some documentations say the opposite, but they lie.
 *
 * _AKP codes for the keyboard (the low byte of the low word)
 * are as follows:
 *
 * 127 = all nationalities supported (?)
 *
 *  0 = USA          8 = Ger.Suisse    16 = Hungary    24 = Romania
 *  1 = Germany      9 = Turkey        17 = Poland     25 = Bulgaria
 *  2 = France      10 = Finnland      18 = Lituania   26 = Slovenia
 *  3 = England     11 = Norway        19 = Latvia     27 = Croatia
 *  4 = Spain       12 = Danmark       20 = Estonia    28 = Serbia
 *  5 = Italy       13 = S. Arabia     21 = Bialorus   29 = Montenegro
 *  6 = Sweden      14 = Netherlands   22 = Ukraina    30 = Macedonia
 *  7 = Fr.Suisse   15 = Czech         23 = Slovakia   31 = Greece
 *
 * 32 = Russia      40 = Vietnam       48 = Bangladesh
 * 33 = Israel      41 = India
 * 34 = Sou. Africa 42 = Iran
 * 35 = Portugal    43 = Mongolia
 * 36 = Belgium     44 = Nepal
 * 37 = Japan       45 = Laos
 * 38 = China       46 = Kambodja
 * 39 = Korea       47 = Indonesia
 *
 * The rest of codes are reserved for future extensions. Add ones,
 * if you find a missing one. Consider there are various countries
 * which all speak the same language, like all the South America
 * speaks Spanish or Portughese, the North America English or French
 * etc.
 *
 */

# ifdef WITHOUT_TOS
#  include "key_tables.h"
# endif

/* Keyboard interrupt routine.
 *
 * TOS goes through here with the freshly baked and still
 * warm key scancode in hands. If there are any keys or
 * key combinations we want to handle (e.g. Ctrl/Alt/Del)
 * then we intercept it now.
 *
 * At the end, you need to return the original scancode for
 * TOS to process it. If you change the value, TOS will buy
 * this as well. If you want to omit the TOS routines at all,
 * return -1 (see code in intr.spp).
 *
 * Developing this code will probably allow us to get rid of
 * checkkeys() in bios.c, have processes waiting for keyboard
 * awaken immediately, load own scancode->ASCII keyboard tables
 * and do other such nifty things (draco).
 *
 */

static const ushort modifiers[] =
{
	CONTROL, RSHIFT, LSHIFT,
	ALTERNATE, CLRHOME, INSERT, 0
};

static const ushort mmasks[] =
{
	MM_CTRL, MM_RSHIFT, MM_LSHIFT,
	MM_ALTERNATE, MM_CLRHOME, MM_INSERT
};

struct	cad_def cad[3];	/* for halt, warm and cold resp. */
short	gl_kbd;		/* default keyboard layout */
static	short cad_lock;	/* semaphore to avoid scheduling shutdown() twice */
static	short kbd_lock;	/* semaphore to temporarily block the keyboard processing */
static	long hz_ticks;	/* place for saving the hz_200 timer value */

static	uchar numin[3];	/* buffer for storing ASCII code typed in via numpad */
static	ushort numidx;	/* index for the buffer (0 = empty, 3 = full) */

/* keyboard table pointers */
static struct keytab *syskeytab;
static struct keytab keytable_vecs;
static MEMREGION *key_region;

/* Routine called after the user hit Ctrl/Alt/Del
 */
static void
ctrl_alt_del(PROC *p, long arg)
{
	switch (cad[arg].action)
	{
		/* 1 is to signal a pid */
		case 1:
			if (sys_p_kill(cad[arg].par.pid, cad[arg].aux.arg) < 0)
				sys_s_hutdown(arg);
			break;

		/* 2 shall be to exec a program
		 * with a path pointed to by par.path
		 */
		case 2:
			if (sys_pexec (100, cad[arg].par.path, cad[arg].aux.cmd, cad[arg].env) < 0)
				sys_s_hutdown (arg);
			break;

		/* 0 is default */
		default:
			sys_s_hutdown (arg);
			break;
	}
}

/* synchronous Ctrl/Alt/F? callback
 * for the debug facilities
 */
static void
ctrl_alt_Fxx (PROC *p, long arg)
{
	do_func_key (arg);
}

/* Similar callback for the Alt/Help.
 *
 * To avoid problems with the actual current directory,
 * the program gets its path as the command line.
 * Or shall we explicitly force it to that?
 *
 * Also a problem here:
 * since the program is executed directly by the kernel,
 * it can at most get the same environment as init has.
 *
 */
static void
alt_help(void)
{
	char pname[32], cmdln[32];

	/* 0x0446L is the boot device */
	ksprintf(pname, sizeof(pname), "%salthelp.sys", sysdir);
	ksprintf(cmdln, sizeof(cmdln), " %salthelp.sys", sysdir);

	cmdln[0] = (char)(strlen(cmdln) - 1);

	sys_pexec(100, pname, cmdln, init_env);
}

/* The handler
 */
static void
put_key_into_buf(uchar c0, uchar c1, uchar c2, uchar c3)
{
	keyrec->tail += 4;
	if (keyrec->tail >= keyrec->buflen)
		keyrec->tail = 0;
	(keyrec->bufaddr + keyrec->tail)[0] = c0;
	(keyrec->bufaddr + keyrec->tail)[1] = c1;
	(keyrec->bufaddr + keyrec->tail)[2] = c2;
	(keyrec->bufaddr + keyrec->tail)[3] = c3;

	kintr = 1;
}

short
ikbd_scan (ushort scancode)
{
	ushort mod = 0, clk = 0, shift = *kbshft, x = 0;
	uchar *chartable, ascii;

	/* This is set during various keyboard table initializations
	 * e.g. when the user calls Bioskeys(), to prevent processing
	 * go according to incomplete keyboard translation tables.
	 */
	if (kbd_lock)
		return -1;

	scancode &= 0x00ff;		/* better safe than sorry */

# ifdef DEV_RANDOM
	add_keyboard_randomness ((ushort)((scancode << 8) | shift));
# endif

	/* We handle modifiers first
	 */
	while (modifiers[x])
	{
		if (scancode == modifiers[x])
		{
			shift |= mmasks[x];
			mod++;
			break;
		}
		else if (scancode == (modifiers[x] | 0x80))
		{
			shift &= ~mmasks[x];
			mod++;
			break;
		}
		x++;
	}

	switch (scancode)
	{
		/* Caps toggles its bit, when hit, it also makes keyclick */
		case	CAPS:
		{
			shift ^= MM_CAPS;
			mod++;
			clk++;
			break;
		}
		/* Releasing Alternate should generate a character, whose ASCII
		 * code was typed in via the numpad
		 */
		case	ALTERNATE+0x80:
		{
			if (numidx)
			{
				ushort ascii_c = 0, tempidx = 0;

				while(numidx--)
				{
					ascii_c += (numin[tempidx] & 0x0f);
					if (numidx)
					{
						tempidx++;
						ascii_c *= 10;
					}
				}

				/* Only the values 0-255 are valid. Silently
				 * ignore the elder byte
				 */
				ascii_c &= 0x00ff;

				/* Reset the buffer for next use */
				numidx = 0;

				put_key_into_buf(0, 0, 0, ascii_c);
			}
			break;
		}
	}

	if (mod)
	{
		ushort sc = scancode;

		*kbshft = mshift = (char)shift;
		if (clk)
			kbdclick(sc);
		sc &= 0x7f;
		/* this catches i.a. release code of CapsLock */
		if ((sc != CLRHOME) && (sc != INSERT))
			return -1;
	}

	/* Here we handle keys of `system wide' meaning. These are:
	 *
	 * Ctrl/Alt/Del		-> warm start
	 * Ctrl/Alt/RShift/Del	-> cold start
	 * Ctrl/Alt/LShift/Del	-> halt
	 * Ctrl/Alt/Undo	-> SIGQUIT to the group
	 * Ctrl/Alt/Fx		-> debug information
	 * Ctrl/Alt/Shift/Fx	-> debug information
	 *
	 */
	if ((shift & MM_CTRLALT) == MM_CTRLALT)
	{
		if (scancode == DEL)
		{
			if (!cad_lock)
			{
				TIMEOUT *t;

				t = addroottimeout (ROOT_TIMEOUT, (void _cdecl (*)(PROC *))ctrl_alt_del, 1);
				if (t)
				{
					t->arg = cad_lock = 1;

					if ((shift & MM_ESHIFT) == MM_RSHIFT)
						t->arg = 2;
					else if ((shift & MM_ESHIFT) == MM_LSHIFT)
						t->arg = 0;

					hz_ticks = *(long *)0x04baL;
				}
			}
			else
			{
				long mora;

				mora = *(long *)0x04baL - hz_ticks;
				if (mora > CAD_TIMEOUT)
					return scancode;
			}

			return -1;
		}
		else if (scancode == UNDO)
		{
			killgroup (con_tty.pgrp, SIGQUIT, 1);

			goto key_done;
		}
		else if ((scancode >= 0x003b) && (scancode <= 0x0044))
		{
			TIMEOUT *t;

			if (shift & MM_ESHIFT)
				scancode += 0x0019;	/* emulate F11-F20 */

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			goto key_done;
		}
		/* This is in case the keyboard has real F11-F20 keys on it */
		else if ((scancode >= 0x0054) && (scancode <= 0x005d))
		{
			TIMEOUT *t;

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			goto key_done;
		}
		/* We ignore release codes, but catch them to avoid
		 * spurious execution of checkkeys() on every release of a key.
		 */
		else if ((scancode == DEL+0x80) || (scancode == UNDO+0x80))
			return -1;
		else if ((scancode >= 0x003b+0x80) && (scancode <= 0x0044+0x80))
			return -1;
		else if ((scancode >= 0x0054+0x80) && (scancode <= 0x005d+0x80))
			return -1;
	}

	if ((shift & MM_ALTERNATE) == MM_ALTERNATE)
	{
		switch (scancode)
		{
			/* Alt/Help fires up a program named `althelp.sys'
			 * located in the system directory (e.g. `c:\multitos\')
			 */
			case HELP:
			{
				addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *))alt_help, 1);
				goto key_done;
				break;
			}
			/* Alt/Numpad generates ASCII codes like in TOS 2.0x.
			 */
			case NUMPAD_0:
			case NUMPAD_1:
			case NUMPAD_2:
			case NUMPAD_3:
			case NUMPAD_4:
			case NUMPAD_5:
			case NUMPAD_6:
			case NUMPAD_7:
			case NUMPAD_8:
			case NUMPAD_9:
			{
				if (numidx > 2)		/* buffer full? reset it */
					numidx = 0;

				chartable = syskeytab->unshift;
				ascii = chartable[scancode];
				if (ascii)
				{
					numin[numidx] = ascii;
					numidx++;
				}
				goto key_done;
				break;
			}
			/* Ignore release codes as usual.
			 */
			case HELP+0x80:
			case NUMPAD_0+0x80:
			case NUMPAD_1+0x80:
			case NUMPAD_2+0x80:
			case NUMPAD_3+0x80:
			case NUMPAD_4+0x80:
			case NUMPAD_5+0x80:
			case NUMPAD_6+0x80:
			case NUMPAD_7+0x80:
			case NUMPAD_8+0x80:
			case NUMPAD_9+0x80:
			{
				return -1;
				break;
			}
		}
	}

	/* Ordinary keyboard here.
	 */
# if 0
	if (scancode < 0x80)
	{
		if (shift & MM_ALTERNATE)
		{
			if (shift & MM_ESHIFT)
				chartable = keytable_vecs.altshift;
			else if (shift & MM_CAPS)
				chartable = keytable_vecs.altcaps;
			else
				chartable = keytable_vecs.alt;

			ascii = 0;

			while (*chartable)
			{
				if (chartable[0] == scancode)
				{
					ascii = chartable[1];
					break;
				}
				chartable += 2;
			}
		}
		else
		{
			if (shift & MM_ESHIFT)
				chartable = keytable_vecs.shift;
			else if (shift & MM_CAPS)
				chartable = keytable_vecs.caps;
			else
				chartable = keytable_vecs.unshift;

			ascii = chartable[scancode];

			if (shift & MM_CTRL)
			{
				if (isupper(ascii))
					ascii &= ~0x40;
				else if (islower(ascii))
					ascii &= ~0x60;
			}
		}

		put_key_into_buf(0, 0, (uchar)scancode, ascii);
	}
# else
	if (scancode < 0x80)
		kintr = 1;

	return scancode;
# endif

key_done:
	if (scancode < 0x80)
		kbdclick(scancode);	/* produce keyclick */

	return -1;			/* don't go to TOS, just return */
}

void
key_repeat(void)
{
	register uchar c0, c1, c2, c3;

	c0 = (keyrec->bufaddr + keyrec->tail)[0];
	c1 = (keyrec->bufaddr + keyrec->tail)[1];
	c2 = (keyrec->bufaddr + keyrec->tail)[2];
	c3 = (keyrec->bufaddr + keyrec->tail)[3];

	put_key_into_buf(c0, c1, c2, c3);

	kbdclick(c2);
}

/* Init section
 *
 * There's our private set of keyboard translation vectors (the
 * struct keytable_vecs above). The init_keybd() called from init.c
 * pre-initializes these to point to the embedded table (usa_kbd[]
 * in key_tables.h). Some time later init.c calls load_keytbl() to
 * load an user supplied keyboard table, if one exists. The file is
 * opened by load_keyboard_table() and its contents is loaded to the
 * memory by load_table(). This one also allocates the buffer long
 * enough to hold the entire table. If the file loaded doesn't look
 * like a keyboard table, the buffer is released and the function fails.
 *
 * In this case the vectors remain as they are after init_keybd().
 *
 * Otherwise the load_table() changes the vectors to point to the newly
 * loaded table and exits with success. Next and last step is bioskeys()
 * which actually moves the contents of the keytable_vecs to the user
 * visible structure in the memory and fixes the value in the Cookie
 * Jar to reflect the current state.
 *
 */

/* The XBIOS' Bioskeys() function
 */
void
sys_b_bioskeys(void)
{
	long akp_val = 0;

	kbd_lock = 1;

	/* Reset the vectors */
	syskeytab->unshift = keytable_vecs.unshift;
	syskeytab->shift = keytable_vecs.shift;
	syskeytab->caps = keytable_vecs.caps;
	syskeytab->alt = keytable_vecs.alt;
	syskeytab->altshift = keytable_vecs.altshift;
	syskeytab->altcaps = keytable_vecs.altcaps;
	if (mch == MILAN_C)
		syskeytab->altgr = keytable_vecs.altgr;

	/* Fix the _AKP cookie, gl_kbd may get changed in load_table()
	 */
	get_cookie(NULL, COOKIE__AKP, &akp_val);
	akp_val &= 0xffffff00L;
	akp_val |= gl_kbd;
	set_cookie(NULL, COOKIE__AKP, akp_val);

	/* Done! */
	kbd_lock = 0;
}

static uchar *
tbl_scan_fwd(uchar *tmp, uchar *end)
{
	while (*tmp && tmp < end)
		tmp++;

	/* skip terminating '0' */
	if (tmp < end)
		tmp++;

	if (tmp == end)
		return NULL;

	return tmp;
}

static long
fill_keystruct(uchar *table, uchar *end)
{
	uchar *unshift, *shift, *caps, *alt, *altshift, *altcaps, *altgr;

	unshift = table;
	shift = table + 128;
	caps = table + 256;
	alt = table + 384;

	altshift = tbl_scan_fwd(alt, end);
	if (!altshift)
		return 0;

	altcaps = tbl_scan_fwd(altshift, end);
	if (!altcaps)
		return 0;

	altgr = tbl_scan_fwd(altcaps, end);

	keytable_vecs.unshift = unshift;
	keytable_vecs.shift = shift;
	keytable_vecs.caps = caps;
	keytable_vecs.alt = alt;
	keytable_vecs.altshift = altshift;
	keytable_vecs.altcaps = altcaps;
	keytable_vecs.altgr = altgr;

	/* OK */
	return 1;
}

static long
load_table(FILEPTR *fp, char *name, long size)
{
	uchar *kbuf;
	long ret = 0;
	MEMREGION *key_reg;

	/* This is 128+128+128 for unshifted, shifted and caps
	 * tables respectively; plus 3 bytes for three alt ones,
	 * plus two bytes magic header, gives 389 bytes minimum.
	 */
	if (size < 389L)
		return -1;

	/* Crap, the keyboard table must be globally readable :/ */
	/* XXX perhaps it would be safer to go for PROT_PR here? */
	key_reg = get_region(core, size, PROT_G);
	kbuf = (uchar *) attach_region(rootproc, key_reg);

	if ((*fp->dev->read)(fp, kbuf, size) == size)
	{
		switch (*(ushort *) kbuf)
		{
			case 0x2771:		/* magic word for std format */
			{
				ret = fill_keystruct(kbuf + sizeof(short), kbuf + size);
				break;
			}
			case 0x2772:		/* magic word for ext format */
			{
				/* The extended format is identical as the old one
				 * with exception that the second word of the data
				 * contains the AKP code for the keyboard table
				 * loaded.
				 */
				ushort *sbuf = (ushort *) kbuf;

				if (sbuf[1] <= MAXAKP)
				{
					ret = fill_keystruct(kbuf + sizeof(long), kbuf + size);
					if (ret)
						gl_kbd = sbuf[1];
				}
				break;
			}
		}
	}

	if (!ret)
	{
		detach_region(rootproc, key_reg);

		key_reg->links--;
		assert(key_reg->links == 0);

		free_region(key_reg);

		return -2;
	}

	/* Success */
	if (key_region)
	{
		detach_region(rootproc, key_region);

		key_region->links--;
		assert(key_region->links == 0);

		free_region(key_region);
	}
	key_region = key_reg;

	return 0;
}

static long
load_default_table(void)
{
# ifdef WITHOUT_TOS
	uchar *kbuf;

	if (key_region)
	{
		detach_region(rootproc, key_region);

		key_region->links--;
		assert(key_region->links == 0);

		free_region(key_region);
	}

	key_region = get_region(core, 387L, PROT_PR);
	kbuf = (uchar *) attach_region(rootproc, key_region);
	quickmove(kbuf, usa_kbd, 387L);
	fill_keystruct(kbuf, kbuf + 387L);

	return 0;
# else
	uchar *kbuf, *p;
	long size;

# if 1
	size = 1024;
# else
	size = 128 + 128 + 128;
	size += strlen(syskeytab->alt) + 1;
	size += strlen(syskeytab->altshift) + 1;
	size += strlen(syskeytab->altcaps) + 1;
	size += strlen(syskeytab->altgr) + 1;
# endif

	if (key_region)
	{
		detach_region(rootproc, key_region);

		key_region->links--;
		assert(key_region->links == 0);

		free_region(key_region);
	}

	key_region = get_region(core, size, PROT_G);
	kbuf = (uchar *) attach_region(rootproc, key_region);

	p = kbuf;

	quickmove(p, syskeytab->unshift, 128);
	p += 128;

	quickmove(p, syskeytab->shift, 128);
	p += 128;

	quickmove(p, syskeytab->caps, 128);
	p += 128;

	if (tosvers >= 0x200)
	{
		long len;

		len = strlen(syskeytab->alt) + 1;
		quickmove(p, syskeytab->alt, len);
		p += len;

		len = strlen(syskeytab->altshift) + 1;
		quickmove(p, syskeytab->altshift, len);
		p += len;

		len = strlen(syskeytab->altcaps) + 1;
		quickmove(p, syskeytab->altcaps, len);
		p += len;

		if (mch == MILAN_C)
		{
			long len;

			len = strlen(syskeytab->altgr) + 1;
			quickmove(p, syskeytab->altgr, len);
			p += len;
		}
	}

	size = fill_keystruct(kbuf, kbuf + size);
	assert(size == 1);

	return 0;
# endif
}

/* If `flag' is 0, a failure to load new table causes the
 * internal one to be initialized instead.
 */
long
load_keyboard_table(char *name, short flag)
{
	XATTR xattr;
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret) return -1;

	ret = do_open(&fp, name, O_RDONLY, 0, &xattr);
	if (!ret)
	{
		ret = load_table(fp, name, xattr.size);
		do_close(rootproc, fp);

		if (!ret)
			sys_b_bioskeys();
	}
	else
	{
		fp->links = 0; /* XXX suppress complaints */
		FP_FREE(fp);

		/* Special case: `load' the default table */
		if (flag == 0)
		{
			load_default_table();
			sys_b_bioskeys();
		}
	}

	return ret;
}

/* This is called from init.c at startup
 */
void
load_keytbl(void)
{
	char name[64];
	long r;

	/* `keybd.tbl' is already used by gem.sys, we can't conflict
	 */
	ksprintf(name, sizeof(name), "%skeyboard.tbl", sysdir);

# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_loading, name);
# endif

	r = load_keyboard_table(name, 0);
# ifdef VERBOSE_BOOT
	if (r == 0)
		boot_printf(MSG_keytable_loaded, gl_kbd);
	else
		boot_printf(MSG_init_error, r);
# endif
}

/* Initialize the built-in keyboard tables.
 * This must be done before init_intr()!
 */
void
init_keybd(void)
{
# ifdef WITHOUT_TOS
	fill_keystruct(usa_kbd, kbuf + sizeof(usa_kbd));
	gl_kbd = 0;
# else
	/* call the underlying XBIOS */
	syskeytab = Keytbl(-1, -1, -1);

	/* temporary init */
	keytable_vecs.unshift = syskeytab->unshift;
	keytable_vecs.shift = syskeytab->shift;
	keytable_vecs.caps = syskeytab->caps;
	keytable_vecs.alt = syskeytab->alt;
	keytable_vecs.altshift = syskeytab->altshift;
	keytable_vecs.altcaps = syskeytab->altcaps;
	if (mch == MILAN_C)
		keytable_vecs.altgr = syskeytab->altgr;
# endif
}

/* EOF */
