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

# include "mint/errno.h"
# include "mint/mint.h"		/* FATAL() */
# include "mint/signal.h"	/* SIGQUIT */

# include "arch/intr.h"		/* click */
# include "arch/tosbind.h"

# include "bios.h"		/* kbshft, kintr, *keyrec, ...  */
# include "biosfs.h"		/* struct tty */
# include "cnf_mint.h"		/* init_env */
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

struct	cad_def cad[3];		/* for halt, warm and cold resp. */
short	gl_kbd;			/* default keyboard layout */
static	short cad_lock;		/* semaphore to avoid scheduling shutdown() twice */
static	short kbd_lock = 1;	/* semaphore to temporarily block the keyboard processing */
static	long hz_ticks;		/* place for saving the hz_200 timer value */

static	uchar numin[3];		/* buffer for storing ASCII code typed in via numpad */
static	ushort numidx;		/* index for the buffer above (0 = empty, 3 = full) */

/* keyboard table pointers */
static struct keytab *tos_keytab;
static struct keytab *user_keytab;
static char *keytab_buffer;
static long keytab_size;
static MEMREGION *user_keytab_region;

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

	sys_pexec(100, pname, cmdln, init_env);
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

				chartable = tos_keytab->unshift;
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
	if (scancode < 0x80)
		kintr = 1;

	return scancode;

key_done:
	if (scancode < 0x80)
		kbdclick(scancode);	/* produce keyclick */

	return -1;			/* don't go to TOS, just return */
}

void
key_repeat(void)
{
	register uchar c0, c1, c2, c3;
	char *new_buf_pos;

	/* Precalculating this saves some memory */
	new_buf_pos = keyrec->bufaddr + keyrec->tail;

	c0 = *new_buf_pos++;
	c1 = *new_buf_pos++;
	c2 = *new_buf_pos++;
	c3 = *new_buf_pos++;

	put_key_into_buf(c0, c1, c2, c3);

	kbdclick(c2);
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
	return user_keytab;
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
load_external_table(FILEPTR *fp, char *name, long size)
{
	uchar *kbuf;
	long ret = 0;

	/* This is 128+128+128 for unshifted, shifted and caps
	 * tables respectively; plus 3 bytes for three alt ones,
	 * plus two bytes magic header, gives 389 bytes minimum.
	 */
	if (size < 389L)
		return EFTYPE;

	kbuf = kmalloc(size + 1);	/* Append a zero (if the table is missing the altgr part) */
	if (!kbuf)
		return ENOMEM;

	bzero(kbuf, size + 1);

	if ((*fp->dev->read)(fp, kbuf, size) == size)
	{
		switch (*(ushort *) kbuf)
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
				ushort *sbuf = (ushort *) kbuf;

				if (sbuf[1] <= MAXAKP)
					gl_kbd = sbuf[1];

				quickmove(kbuf, kbuf + sizeof(long), size - sizeof(long) + 1);
				break;
			}
			default:
			{
				ret = EFTYPE;		/* wrong format */
				break;
			}
		}
	}

	if (ret < 0)
	{
		kfree(kbuf);
		return ret;
	}

	/* Release old buffer. This can only happen when the keytable
	 * is loaded at runtime via Ssystem() call.
	 */
	if (keytab_buffer)
		kfree(keytab_buffer);
	keytab_buffer = kbuf;

	return 0;
}

/* Returns size */
static long
load_internal_table(void)
{
	uchar *kbuf, *p;
	long size, len;

	size = 128 + 128 + 128;

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
		size += 5;	/* a byte for each missing part plus a NUL */

	/* If a buffer was allocated previously, we can perhaps reuse it.
	 */
	if (keytab_buffer && (keytab_size >= size))
		kbuf = keytab_buffer;
	else
	{
		if (keytab_buffer)
			kfree(keytab_buffer);
		kbuf = kmalloc(size);
	}

	if (!kbuf)
		return ENOMEM;

	bzero(kbuf, size);

	p = kbuf;

	quickmove(p, tos_keytab->unshift, 128);
	p += 128;

	quickmove(p, tos_keytab->shift, 128);
	p += 128;

	quickmove(p, tos_keytab->caps, 128);
	p += 128;

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

	keytab_buffer = kbuf;

	return size;
}

/* This routine has to load the keyboard table into memory.
 * First the loading from the disk is attempted, and when
 * this fails, the TOS table is used.
 *
 * Flag:
 * 0x01 - don't load TOS BIOS table on failure
 * 0x02 - don't generate any messages
 */
long
load_keyboard_table(char *name, short flag)
{
	XATTR xattr;
	FILEPTR *fp;
	long ret, size = 0;

	ret = FP_ALLOC(rootproc, &fp);

	if (ret == 0)
	{
		ret = do_open(&fp, name, O_RDONLY, 0, &xattr);
		if (!ret)
		{
			ret = load_external_table(fp, name, xattr.size);
			do_close(rootproc, fp);
			if (ret == 0)
			{
				/* don't add 1, the size is already bigger 2 or 4 bytes
				 * than it is needed to keep the table.
				 */
				size = xattr.size;
# ifdef VERBOSE_BOOT
				/* During startup generate a message */
				if ((flag & 0x02) == 0)
					boot_printf(MSG_keytable_loading, name);
# endif
			}
		}
		else
		{
			fp->links = 0;	/* XXX suppress complaints */
			FP_FREE(fp);
		}
	}

	/* Not "else if" */
	if (((flag & 0x01) == 0) && (ret < 0 || size == 0))
	{
		size = load_internal_table();
# ifdef VERBOSE_BOOT
		/* During startup generate a message */
		if ((flag & 0x02) == 0)
			boot_printf(MSG_keytable_internal);
# endif
	}

	return size;
}

/* This is called from init.c at startup
 */
void
load_keytbl(void)
{
	char name[32];

	/* `keybd.tbl' is already used by gem.sys, we can't conflict
	 */
	ksprintf(name, sizeof(name), "%skeyboard.tbl", sysdir);

	/* After this the keytab_buffer points to the loaded AKP-style
	 * keyboard table, and keytab_size contains its size.
	 */
	keytab_size = load_keyboard_table(name, 0);

# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_loaded, gl_kbd);
# endif

	/* Install */
	sys_b_bioskeys();
}

/* Pre-initialize the built-in keyboard tables.
 * This must be done before init_intr()!
 */
void
init_keybd(void)
{
	/* call the underlying XBIOS */
	tos_keytab = TRAP_Keytbl(-1, -1, -1);
}

/* EOF */
