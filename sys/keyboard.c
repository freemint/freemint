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

# ifndef NO_AKP_KEYBOARD

# include "libkern/libkern.h"	/* strcpy(), strcat(), ksprintf() */

# include "mint/errno.h"
# include "mint/mint.h"		/* FATAL() */
# include "mint/signal.h"	/* SIGQUIT */

# include "arch/intr.h"		/* click */
# include "arch/timer.h"	/* get_hz_200() */
# include "arch/tosbind.h"

# include "bios.h"		/* kbshft, kintr, *keyrec, ...  */
# include "biosfs.h"		/* struct tty */
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
# include "kmemory.h"		/* kmalloc(), kfree() */
# include "keyboard.h"		/* struct cad, struct keytbl */
# include "memory.h"		/* get_region(), attach_region() */
# include "proc.h"		/* rootproc */
# include "random.h"		/* add_keyboard_randomness() */
# include "signal.h"		/* killgroup() */
# include "timeout.h"		/* addroottimeout() */

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

# if 0
# define WITHOUT_TOS
# define KBD_USA
# endif

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

static const uchar modifiers[] =
{
	CONTROL, RSHIFT, LSHIFT, ALTERNATE, CLRHOME, INSERT,
	ALTGR, 0
};

/* Masks correspond to the above modifier scancodes */
static const uchar mmasks[] =
{
	MM_CTRL, MM_RSHIFT, MM_LSHIFT, MM_ALTERNATE, MM_CLRHOME, MM_INSERT,
	MM_ALTGR
};

struct	cad_def cad[3];		/* for halt, warm and cold resp. */

static	short cad_lock;		/* semaphore to avoid scheduling shutdown() twice */
static	short kbd_lock = 1;	/* semaphore to temporarily block the keyboard processing */
static	long hz_ticks;		/* place for saving the hz_200 timer value */

static	uchar numin[3];		/* buffer for storing ASCII code typed in via numpad */
static	ushort numidx;		/* index for the buffer above (0 = empty, 3 = full) */

/* Variables that deal with keyboard autorepeat */
static	uchar last_key[4];	/* last pressed key */
static	short key_pressed;	/* flag for keys pressed/released (0 = no key is pressed) */
static	ushort keydel;		/* keybard delay rate and keyboard repeat rate, respectively */
static	ushort krpdel;
static	ushort kdel, krep;	/* actual counters */

/* keyboard table pointers */
# ifdef WITHOUT_TOS
static struct keytab *tos_keytab = &sys_keytab;		/* see key_tables.h */
# else
static struct keytab *tos_keytab = NULL;		/* see init_keybd() */
# endif
static struct keytab *user_keytab = NULL;
static char *keytab_buffer = NULL;
static long keytab_size = 0;
static MEMREGION *user_keytab_region = NULL;

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

	ksprintf(pname, sizeof(pname), "%salthelp.sys", sysdir);
	ksprintf(cmdln, sizeof(cmdln), " %salthelp.sys", sysdir);

	cmdln[0] = (char)(strlen(cmdln) - 1);

	sys_pexec(100, pname, cmdln, _base->p_env);
}

/* The handler
 */
static void
put_key_into_buf(uchar c0, uchar c1, uchar c2, uchar c3)
{
	char *new_buf_pos;

	keyrec->tail += 4;
	if (keyrec->tail >= keyrec->buflen)
		keyrec->tail = 0;

	/* Precalculating this saves some memory */
	new_buf_pos = keyrec->bufaddr + keyrec->tail;

	*new_buf_pos++ = c0;
	*new_buf_pos++ = c1;
	*new_buf_pos++ = c2;
	*new_buf_pos++ = c3;

	/* c1 == 0 means that this "keypress" was generated
	 * by typing on the numpad while holding Alt down.
	 * We don't want this to be repeated.
	 */
	if (c1)
	{
		last_key[0] = c0;
		last_key[1] = c1;
		last_key[2] = c2;
		last_key[3] = c3;

		kbdclick((ushort)c2);
	}

	kintr = 1;
}

static void
key_repeat(void)
{
	put_key_into_buf(last_key[0], last_key[1], last_key[2], last_key[3]);
}

/* Called every 20ms */
void
autorepeat_timer(void)
{
	if (kbd_lock)
		return;

	/* conterm */
	if ((*(char *)0x0484L & 0x02) == 0)
		return;

	if (key_pressed)
	{
		if (kdel)
		{
			kdel--;
			if (kdel == 0)
				key_repeat();
		}
		else
		{
			krep--;
			if (krep == 0)
			{
				key_repeat();
				krep = krpdel;
			}
		}
	}
	else
	{
		kdel = keydel;
		krep = krpdel;
	}
}

/* Translate scancode into ASCII according to the keyboard
 * translation tables.
 */
INLINE uchar
scan2asc(uchar scancode)
{
	uchar asc = 0, *vec;

	/* The AKP table structure is:
	 * ss, aa, ss, aa, ss, aa, 0
	 * where 'ss' is scancode and 'aa' is corresponding
	 * ASCII value.
	 */
	if (*kbshft & (MM_ALTERNATE | MM_ALTGR))
	{
		if (*kbshft & MM_ALTGR)
			vec = user_keytab->altgr;
		else
		{
			if (*kbshft & MM_ESHIFT)
				vec = user_keytab->altshift;
			else if ((*kbshft & MM_CAPS) == 0)
				vec = user_keytab->alt;
			else
				vec = user_keytab->altcaps;
		}

		while (vec && *vec)
		{
			if (vec[0] == scancode)
			{
				asc = vec[1];
				break;
			}
			vec++; vec++;
		}
	}

	/* Hmmm, not sure if this should operate so.
	 * If the AKP translation results 0, we
	 * continue as if the Alt key was not depressed.
	 * Otherwise we ignore the "old" (not AKP)
	 * tables.
	 * N.AES Alt/Ctrl/key combos work with this.
	 */
	if (asc == 0)
	{
		/* Shift/1 should give "!" regardless of the Caps state
		 */
		if (*kbshft & MM_ESHIFT)
			vec = user_keytab->shift;
		else if ((*kbshft & MM_CAPS) == 0)
			vec = user_keytab->unshift;
		else
			vec = user_keytab->caps;

		if (vec)
			asc = vec[scancode];
	}

	/* I think that Ctrl key works as this:
	 */
	if (*kbshft & MM_CTRL)
	{
		if (asc == 0x0d)
			asc = 0x0a;		/* God bless great ideas */

		if ((asc & 0x80) == 0)
			asc &= 0x1f;
	}

	return asc;
}

# undef SCANCODE_TESTING

# ifdef SCANCODE_TESTING
static void
output_scancode(PROC *p, long arg)
{
	uchar ascii;

	ascii = scan2asc((ushort)arg);

	DEBUG(("Scancode: %02lx, ASCII %02x", arg, (short)ascii));
}
# endif

/* `scancode' is short, but only low byte matters. The high byte
 * is zeroed by newkeys().
 */
short
ikbd_scan (ushort scancode, IOREC_T *rec)
{
	ushort mod = 0, clk = 0, x = 0;
	uchar shift = *kbshft, *chartable, ascii;

	/* This is set during various keyboard table initializations
	 * e.g. when the user calls Bioskeys(), to prevent processing
	 * go according to incomplete keyboard translation tables.
	 */
	if (kbd_lock)
		return -1;

# ifdef SCANCODE_TESTING
	{
		TIMEOUT *t;

		t = addroottimeout (ROOT_TIMEOUT, (void _cdecl (*)(PROC *))output_scancode, 1);
		if (t)
			t->arg = scancode;
	}
# endif

# ifdef DEV_RANDOM
	add_keyboard_randomness ((ushort)((scancode << 8) | shift));
# endif

	/* We handle modifiers first
	 */
	while ((uchar)modifiers[x])
	{
		if (scancode == (ushort)modifiers[x])
		{
			shift |= mmasks[x];
			mod++;
			break;
		}
		else if (scancode == ((ushort)modifiers[x] | 0x80))
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

		*kbshft = mshift = shift;
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

					hz_ticks = get_hz_200();
				}
			}
			else
			{
				long mora;

				mora = get_hz_200() - hz_ticks;
				if (mora > CAD_TIMEOUT)
					return scancode;		/* we call TOS here */
			}

			return -1;
		}
		else if (scancode == UNDO)
		{
			killgroup (con_tty.pgrp, SIGQUIT, 1);

			kbdclick(scancode);

			return -1;
		}
		else if ((scancode >= 0x003b) && (scancode <= 0x0044))
		{
			TIMEOUT *t;

			if (shift & MM_ESHIFT)
				scancode += 0x0019;	/* emulate F11-F20 */

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			kbdclick(scancode);

			return -1;
		}
		/* This is in case the keyboard has real F11-F20 keys on it */
		else if ((scancode >= 0x0054) && (scancode <= 0x005d))
		{
			TIMEOUT *t;

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			kbdclick(scancode);

			return -1;
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

				kbdclick(scancode);

				return -1;
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

				chartable = user_keytab->unshift;
				ascii = chartable[scancode];

				if (ascii)
				{
					numin[numidx] = ascii;
					numidx++;
				}

				kbdclick(scancode);

				return -1;
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
	if (scancode & 0x80)
		key_pressed = 0;
	else
	{
		key_pressed = 1;

		ascii = scan2asc((uchar)scancode);

		put_key_into_buf(shift, (uchar)scancode, 0, ascii);
	}

	return -1;			/* don't go to TOS, just return */
}

/* The XBIOS' Keytbl() function
 *
 * This call imposes a problem. It namely allows programs to reallocate
 * parts of the keyboard translation tables, and moreover returns a
 * pointer to the global keytab structure so that programs can directly
 * manipulate that. This may severely affect system stability, if a
 * program changes the translation table and redirects one of the
 * vectors to own memory (even supervisor protected), and then suddenly
 * dies.
 *
 * The solution would be to fake the keyboard translation table and
 * the vectors inside program's memory space and allowing manipulate that.
 * Such manipulations of course would have no effect, but would be safe
 * for the system and more programs would continue to operate, even if
 * in a bit limited way.
 *
 * For now we just ignore the passed vectors and trust that programs
 * call this only to READ the keyboard tables, not write them (esp.
 * as the translation table is PROT_PR, so it can be read, but
 * cannot be written to).
 */

struct keytab *
sys_b_keytbl(char *unshifted, char *shifted, char *caps)
{
	if ((long)unshifted > 0 || (long)shifted > 0 || (long)caps > 0)
	{
		ALERT(MSG_keyboard_keytbl_ignored, (long)unshifted, (long)shifted, (long)caps);
	}

	return user_keytab;
}

/* The XBIOS' Kbrate() function
 */
ushort
sys_b_kbrate(ushort delay, ushort rate)
{
	ushort ret;

	ret = keydel & 0x00ff;
	ret <<= 8;
	ret |= (krpdel & 0x00ff);

	if (delay >= 0)
		keydel = delay & 0x00ff;
	if (rate >= 0)
		krpdel = rate & 0x00ff;

	return ret;
}

/* The XBIOS' Bioskeys() function
 */

/* Helper */
static uchar *
tbl_scan_fwd(uchar *tmp)
{
	while (*tmp)
		tmp++;

	return ++tmp;		/* skip the ending NULL */
}

void
sys_b_bioskeys(void)
{
	long akp_val = 0;
	char *buf, *tables;
	struct keytab *pointers;

	/* First block the keyboard processing code */
	kbd_lock = 1;

	/* Release old user keytables and vectors */
	if (user_keytab_region)
	{
		detach_region(rootproc, user_keytab_region);

		user_keytab_region->links--;
		assert(user_keytab_region->links == 0);

		free_region(user_keytab_region);
	}

	/* Reserve one region for both keytable and its vectors */
	user_keytab_region = get_region(core, keytab_size + sizeof(struct keytab), PROT_PR);

	buf = (char *)attach_region(rootproc, user_keytab_region);
	pointers = (struct keytab *)buf;
	tables = buf + sizeof(struct keytab);

	assert(pointers);

	/* Copy the master table over */
	quickmove(tables, keytab_buffer, keytab_size);

	/* Setup the standard vectors */
	pointers->unshift = tables;
	pointers->shift = tables + 128;
	pointers->caps = tables + 128 + 128;

	/* and the AKP vectors */
	pointers->alt = tables + 128 + 128 + 128;
	pointers->altshift = tbl_scan_fwd(pointers->alt);
	pointers->altcaps = tbl_scan_fwd(pointers->altshift);
	pointers->altgr = tbl_scan_fwd(pointers->altcaps);

# if 1
	/* Fix the _AKP cookie, gl_kbd may get changed in load_table().
	 *
	 * XXX must be changed for -DJAR_PRIVATE (forward to all processes).
	 */
	get_cookie(NULL, COOKIE__AKP, &akp_val);
	akp_val &= 0xffffff00L;
	akp_val |= (gl_kbd & 0x000000ff);
	set_cookie(NULL, COOKIE__AKP, akp_val);

# else
	/* Reset the TOS BIOS vectors (this is only necessary
	 * until we replace all BIOS keyboard routines).
	 */

	tos_keytab->unshift = pointers->unshift;
	tos_keytab->shift = pointers->shift;
	tos_keytab->caps = pointers->caps;

	if (tosvers >= 0x0400)
	{
		tos_keytab->alt = pointers->alt;
		tos_keytab->altshift = pointers->altshift;
		tos_keytab->altcaps = pointers->altcaps;
		if (mch == MILAN_C)
			tos_keytab->altgr = pointers->altgr;

		/* Fix the _AKP cookie, gl_kbd may get changed in load_table().
		 *
		 * XXX must be changed for -DJAR_PRIVATE (forward to all processes).
		 */
		get_cookie(NULL, COOKIE__AKP, &akp_val);
		akp_val &= 0xffffff00L;
		akp_val |= gl_kbd;
		set_cookie(NULL, COOKIE__AKP, akp_val);
	}
# endif

	user_keytab = pointers;

	/* Done! */
	kbd_lock = 0;
}

/* Init section
 *
 * Notice that we deal with two tables here:
 *
 * - the "master" table, which is kept inside keytab_buffer and is used
 *   to restore the user table, when Bioskeys() is called;
 * - the user table, which is kept inside user_keytab_region.
 *
 * The load_keyboard_table(), which is called at startup and can be
 * called at runtime via Ssystem(), loads the master table, so that
 * after Ssystem(LOAD_KBD) you cannot restore previous settings by
 * simply calling Bioskeys(). To return to previous config you must
 * reload the previous table again.
 */

/* Two following routines prepare the master copy of the keyboard
 * translation table. The master copy is not available for the user,
 * it only serves for the Bioskeys() call to restore the user
 * keyboard table, when necessary.
 *
 * Notice that full tables are prepared, i.e. with the AKP extensions,
 * regardless of the TOS version.
 */

/* Returns status */
static long
load_external_table(FILEPTR *fp, const char *name, long size)
{
	uchar *kbuf;
	long ret = 0;

	/* This is 128+128+128 for unshifted, shifted and caps
	 * tables respectively; plus 3 bytes for three alt ones,
	 * plus two bytes magic header, gives 389 bytes minimum.
	 */
	if (size < 389L)
	{
		DEBUG(("%s(): failure (size %ld)", __FUNCTION__, size));
		return EFTYPE;
	}

	kbuf = kmalloc(size+1); /* Append a zero (if the table is missing the altgr part) */
	if (!kbuf)
	{
		DEBUG(("%s(): out of memory", __FUNCTION__));
		return ENOMEM;
	}

	bzero(kbuf, size+1);

	if ((*fp->dev->read)(fp, kbuf, size) == size)
	{
		switch (*(ushort *)kbuf)
		{
			case 0x2771:		/* magic word for std format */
			{
				quickmove(kbuf, kbuf + sizeof(short), size - sizeof(short) + 1);
				break;
			}
			case 0x2772:		/* magic word for ext format */
			{
				/* The extended format is identical as the old one
				 * with exception that the second word of the data
				 * contains the AKP code for the keyboard table
				 * loaded.
				 */
				ushort *sbuf = (ushort *)kbuf;

				if (sbuf[1] <= MAXAKP)
					gl_kbd = sbuf[1];

				quickmove(kbuf, kbuf + sizeof(long), size - sizeof(long) + 1);
				break;
			}
			default:
			{
				DEBUG(("%s(): unknown format 0x%04x", __FUNCTION__, (ushort *)kbuf));

				ret = EFTYPE;	/* wrong format */
				break;
			}
		}
	}

	if (ret < 0)
	{
		kfree(kbuf);
		return ret;
	}

	/* Release old buffer.
	 */
	if (keytab_buffer)
		kfree(keytab_buffer);

	keytab_buffer = kbuf;
	keytab_size = size+1;

	TRACE(("%s(): keytab_size %ld", __FUNCTION__, keytab_size));

	/* Install */
	sys_b_bioskeys();

	return 0;
}

static void
load_internal_table(void)
{
	uchar *kbuf, *p;
	long size, len;

	size = 128 + 128 + 128;

# ifndef WITHOUT_TOS

	if (tosvers >= 0x0400)
	{
		size += strlen(tos_keytab->alt) + 1;
		size += strlen(tos_keytab->altshift) + 1;
		size += strlen(tos_keytab->altcaps) + 1;
		if (mch == MILAN_C)
			size += strlen(tos_keytab->altgr) + 1;
		else
			size += 2;
	}
	else
		size += 16; /* a byte for each missing part plus a NUL plus some space */
# else
	/* Our default keyboard table (see key_table.h) is always
	 * a complete one.
	 */
	size += strlen(tos_keytab->alt) + 1;
	size += strlen(tos_keytab->altshift) + 1;
	size += strlen(tos_keytab->altcaps) + 1;
	size += strlen(tos_keytab->altgr) + 1;

	size += 8; /* add some space */
# endif

	/* If a buffer was allocated previously, we can perhaps reuse it.
	 */
	if (keytab_buffer && (keytab_size >= size))
	{
		kbuf = keytab_buffer;
	}
	else
	{
		if (keytab_buffer)
		{
			kfree(keytab_buffer);
			keytab_buffer = NULL;
		}

		kbuf = kmalloc(size);
	}

	assert(kbuf);
	bzero(kbuf, size);

	p = kbuf;

	quickmove(p, tos_keytab->unshift, 128);
	p += 128;

	quickmove(p, tos_keytab->shift, 128);
	p += 128;

	quickmove(p, tos_keytab->caps, 128);
	p += 128;

# ifndef WITHOUT_TOS

	if (tosvers >= 0x0400)
	{
		len = strlen(tos_keytab->alt) + 1;
		quickmove(p, tos_keytab->alt, len);
		p += len;

		len = strlen(tos_keytab->altshift) + 1;
		quickmove(p, tos_keytab->altshift, len);
		p += len;

		len = strlen(tos_keytab->altcaps) + 1;
		quickmove(p, tos_keytab->altcaps, len);
		p += len;

		if (mch == MILAN_C)
		{
			len = strlen(tos_keytab->altgr) + 1;
			quickmove(p, tos_keytab->altgr, len);
		}
	}

# else
	/* Our default keyboard table (see key_table.h) is always
	 * a complete one.
	 */

	len = strlen(tos_keytab->alt) + 1;
	quickmove(p, tos_keytab->alt, len);
	p += len;

	len = strlen(tos_keytab->altshift) + 1;
	quickmove(p, tos_keytab->altshift, len);
	p += len;

	len = strlen(tos_keytab->altcaps) + 1;
	quickmove(p, tos_keytab->altcaps, len);
	p += len;

	len = strlen(tos_keytab->altgr) + 1;
	quickmove(p, tos_keytab->altgr, len);

	gl_kbd = default_akp;
# endif

	keytab_buffer = kbuf;
	keytab_size = size;

	TRACE(("%s(): keytab_size %ld", __FUNCTION__, keytab_size));

	/* Install */
	sys_b_bioskeys();
}

/* This routine loads the keyboard table into memory.
 *
 * path is the complete path to the keyboard table or NULL.
 * If path is NULL the default path of <sysdir>/keyboard.tbl
 * is used.
 *
 * flag: 0x1 - print messages
 */
long
load_keyboard_table(const char *path, short flag)
{
	char buf[64];
	const char *name;
	FILEPTR *fp;
	long ret;

	if (!path)
	{
		/* `keybd.tbl' is already used by gem.sys, we can't conflict
		 */
		ksprintf(buf, sizeof(buf), "%skeyboard.tbl", sysdir);
		name = buf;
	}
	else
		name = path;

	TRACE(("%s(): path `%s'", __FUNCTION__, name));

	ret = FP_ALLOC(rootproc, &fp);
	if (ret == 0)
	{
		XATTR xattr;

		ret = do_open(&fp, name, O_RDONLY, 0, &xattr);
		if (ret == 0)
		{
# ifdef VERBOSE_BOOT
			if (flag & 0x1)
				boot_printf(MSG_keytable_loading, name);
# endif
			ret = load_external_table(fp, name, xattr.size);
# ifdef VERBOSE_BOOT
			if (ret == 0 && (flag & 0x1))
				boot_printf(MSG_keytable_loaded, gl_kbd);
# endif
			do_close(rootproc, fp);
		}
		else
		{
			fp->links = 0;	/* XXX suppress complaints */
			FP_FREE(fp);
		}
	}

	return ret;
}

/* Pre-initialize the built-in keyboard tables.
 * This must be done before init_intr()!
 */
void
init_keybd(void)
{
	ushort delayrate;

	/* Call the underlying XBIOS to get some defaults.
	 *
	 * On WITHOUT_TOS, the tos_keytab is defined as
	 * static pointer to an initialized struct in
	 * key_tables.h
	 */

# ifndef WITHOUT_TOS
	tos_keytab = TRAP_Keytbl(-1, -1, -1);
# endif

	TRACE(("%s(): BIOS keyboard table at 0x%08lx", __FUNCTION__, tos_keytab));

# ifndef WITHOUT_TOS
	delayrate = TRAP_Kbrate(-1, -1);
# else
	delayrate = 0x0f02;	/* this is what TRAP_Kbrate() normally returns */
# endif

	keydel = kdel = delayrate >> 8;
	krpdel = krep = delayrate & 0x00ff;

	TRACE(("%s(): delay 0x%02x, rate 0x%02x", __FUNCTION__, keydel, krpdel));
	TRACE(("%s(): conterm 0x%02x", __FUNCTION__, (short)(*(char *)0x0484L)));

	/* initialize internal table */
# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_internal);
# endif
	load_internal_table(); /* either from BIOS or builtin */
# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_loaded, gl_kbd);
# endif
}

# endif	/* NO_AKP_KEYBOARD */

/* EOF */
