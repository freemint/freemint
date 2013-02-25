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
 * TO DO:
 *
 * - a possibility to map scancodes to other scancodes (before processing)
 * - deadkey support (expand translation tables with new vectors?)
 * - translation tables in text form (avoid mktbl)
 * - three phases of translation (scancode -> unicode -> ISO)?
 *
 * CHANGES:
 * - Ozk: 4 June 2005.
 *        Moved handling of keyboard delay/repeat from VBL interrupt to
 *        roottimeouts.
 * - joska: 15 December 2009.
 *        Added handling of deadkeys.
 *
 * UNRESOLVED:
 * - Ozk: Since we cannot completely take over the keyboard interrupt when
 *   running on Milan hardware, and that Milan TOS calls ikbdsys vector
 *   for each repeated key, we cannot control those things by ourselves.
 *   So, we need to pass calls to Kbrate() to the ROM.
 * - joska: I'm not 100% sure what Ozk really means here. Milan TOS does
 *   not call anything when a key is repeated - the PS/2 keyboard handles
 *   key repeat itself so the Milan actually doesn't know the difference
 *   between a normal keypress and a repeated one. On Milan, Kbrate() informs
 *   the keyboard of the new repeat-rate. I don't see any need to change
 *   this.
 *
 */

# include "libkern/libkern.h"	/* strcpy(), strcat(), ksprintf() */

# include "mint/asm.h"		/* spl7() */
# include "mint/errno.h"
# include "mint/mint.h"		/* FATAL() */
# include "mint/signal.h"	/* SIGQUIT */

# include "arch/intr.h"		/* click */
# include "arch/init_intr.h"	/* syskey */
# include "arch/timer.h"	/* get_hz_200() */
# include "arch/tosbind.h"
# include "arch/syscall.h"

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

# ifndef NO_AKP_KEYBOARD

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
 *  3 = England     11 = Norway        19 = Russia     27 = Croatia
 *  4 = Spain       12 = Danmark       20 = Estonia    28 = Serbia
 *  5 = Italy       13 = S. Arabia     21 = Bialorus   29 = Montenegro
 *  6 = Sweden      14 = Netherlands   22 = Ukraina    30 = Macedonia
 *  7 = Fr.Suisse   15 = Czech         23 = Slovakia   31 = Greece
 *
 * 32 = Latvia      40 = Vietnam       48 = Bangladesh
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
 * which all speak the same language, like all South America
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

static const uchar modifiers[] =
{
	/* Don't add CAPS here, it's handled separately */
	CONTROL, RSHIFT, LSHIFT, ALTERNATE, CLRHOME, INSERT,
	ALTGR, 0
};

/* Masks correspond to the above modifier scancodes */
static const uchar mmasks[] =
{
	MM_CTRL, MM_RSHIFT, MM_LSHIFT, MM_ALTERNATE, MM_CLRHOME, MM_INSERT,
	MM_ALTGR
};

/* Exported variables */
long	iso_8859_code;	/* this is 2 for ISO-8859-2, 3 for ISO-8859-3 etc., or 0 for default/undefined */

short	kbd_pc_style_caps = 0;	/* PC-style vs. Atari-style for Caps operation */
short	kbd_mpixels = 8;	/* mouse pixel steps */
short	kbd_mpixels_fine = 1;	/* mouse pixel steps in 'fine' mode */
struct	cad_def cad[3];		/* for halt, warm and cold resp. */

/* Auxiliary variables for ikbd_scan() */
static	short cad_lock;		/* semaphore to avoid scheduling shutdown() twice */
static	short kbd_lock;		/* semaphore to temporarily block the keyboard processing */
static	long hz_ticks;		/* place for saving the hz_200 timer value */

/* Alt/numpad */
static	uchar numin[8];		/* buffer for storing ASCII code typed in via numpad */
static	ushort numidx;		/* index for the buffer above (0 = empty, 3 = full) */

/* Variables that deal with keyboard autorepeat */
static IOREC_T *last_iorec;
static	uchar last_key[4];	/* last pressed key */
//static	short key_pressed;	/* flag for keys pressed/released (0 = no key is pressed) */
//static	ushort keydel;		/* keybard delay rate and keyboard repeat rate, respectively */
//static	ushort krpdel;
//static	ushort kdel, krep;	/* actual counters */

static	long keydel_time;
static	long keyrep_time;
static	TIMEOUT *m_to;
#ifndef MILAN
static	TIMEOUT *k_to;
#endif
static	ushort mouse_step;

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


# define ROOT_TIMEOUT	1
# define CAD_TIMEOUT	5*200

/* Mouse event timeouts */

# define MOUSE_UP	0
# define MOUSE_DOWN	1
# define MOUSE_RIGHT	2
# define MOUSE_LEFT	3
# define MOUSE_LCLICK	5
# define MOUSE_RCLICK	6

# define MOUSE_TIMEOUT	40

static short keep_sending;		/* flag for mouse packets auto-repetition */
static char mouse_packet[6];

/* Mouse movements in four directions */
static void
mouse_up(PROC *p, long pixels)
{
	long to;

	mouse_packet[0] = 0xf8;		/* header */
	mouse_packet[1] = 0;		/* X axis */
	mouse_packet[2] = -pixels;	/* Y axis */

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	if (keep_sending)
	{
		if (!mouse_step)
		{
			to = 500;
			mouse_step++;
		}
		else
			to = 10;

		m_to = addroottimeout(to, mouse_up, 0);
		if (m_to) m_to->arg = pixels;
	}
	else
		mouse_step = 0, m_to = NULL;
}

static void
mouse_down(PROC *p, long pixels)
{
	long to;

	mouse_packet[0] = 0xf8;		/* header */
	mouse_packet[1] = 0;		/* X axis */
	mouse_packet[2] = pixels;	/* Y axis */

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	if (keep_sending)
	{
		if (!mouse_step)
		{
			to = 500; //keydel;
			mouse_step++;
		}
		else
			to = 10; //krpdel;

		m_to = addroottimeout(to, mouse_down, 0);
		if (m_to) m_to->arg = pixels;
	}
	else
		mouse_step = 0, m_to = NULL;
}

static void
mouse_left(PROC *p, long pixels)
{
	long to;

	mouse_packet[0] = 0xf8;		/* header */
	mouse_packet[1] = -pixels;	/* X axis */
	mouse_packet[2] = 0;		/* Y axis */

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	if (keep_sending)
	{
		if (!mouse_step)
		{
			to = 500; //keydel;
			mouse_step++;
		}
		else
			to = 10; //krpdel;

		m_to = addroottimeout(to, mouse_left, 0);
		if (m_to) m_to->arg = pixels;
	}
	else
		mouse_step = 0, m_to = NULL;

}

static void
mouse_right(PROC *p, long pixels)
{
	long to;

	mouse_packet[0] = 0xf8;		/* header */
	mouse_packet[1] = pixels;	/* X axis */
	mouse_packet[2] = 0;		/* Y axis */

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	if (keep_sending)
	{
		if (!mouse_step)
		{
			to = 500; //keydel;
			mouse_step++;
		}
		else
			to = 10; //krpdel;

		m_to = addroottimeout(to, mouse_right, 0);
		if (m_to) m_to->arg = pixels;
	}
	else
		mouse_step = 0, m_to = NULL;
}

/* Generate "no button" packet, simulating the mouse key release */
static void
mouse_noclick(PROC *p, long arg)
{
	mouse_packet[0] = 0xf8;		/* header */
	mouse_packet[1] = 0;		/* X axis */
	mouse_packet[2] = 0;		/* Y axis */

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);
}

/* Generate right click */
/* Note: Atari Compendium is wrong (as usual), when states that right
 * click generates packet 0xfa, and left - 0xf9. This is exactly the
 * other way around.
 */
static void
mouse_rclick(PROC *p, long arg)
{
	mouse_packet[0] = 0xf9;		/* header */
	mouse_packet[1] = 0;		/* X axis */
	mouse_packet[2] = 0;		/* Y axis */

	*kbshft &= ~MM_ALTERNATE;

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	/* Generate "release" packet */
	addroottimeout(MOUSE_TIMEOUT, mouse_noclick, 0);
}

/* Generate left click */
static void
mouse_lclick(PROC *p, long arg)
{
	mouse_packet[0] = 0xfa;		/* header */
	mouse_packet[1] = 0;		/* X axis */
	mouse_packet[2] = 0;		/* Y axis */

	*kbshft &= ~MM_ALTERNATE;

	send_packet(syskey->mousevec, mouse_packet, mouse_packet + 3);

	/* Generate "release" packet */
	addroottimeout(MOUSE_TIMEOUT, mouse_noclick, 0);
}

/* Generate double left click */
static void
mouse_dclick(PROC *p, long arg)
{
	mouse_lclick(p, arg);
	addroottimeout(MOUSE_TIMEOUT + 20, mouse_lclick, 0);
}

static void
set_mouse_timeout( void _cdecl (*f)(PROC *, long arg), short make, short delta, long to)
{
	if (make)
	{
		if (!m_to)
		{
			m_to = addroottimeout(to, f, 1);
			if (m_to)
				m_to->arg = delta;
		}
	}
	else if (m_to)
	{
		cancelroottimeout(m_to);
		m_to = NULL;
		mouse_step = 0;
	}
}


static bool
is_eiffel_mouse_key(ushort scan)
{
	return ((scan >= 0x59 && scan <= 0x5f) && scan != 0x5b)
		|| scan == 0x37;
}


# ifndef MILAN
static void put_key_into_buf(IOREC_T *iorec, uchar c0, uchar c1, uchar c2, uchar c3);
static void
kbd_repeat(PROC *p, long arg)
{
	put_key_into_buf(last_iorec, last_key[0], last_key[1], last_key[2], last_key[3]);
	kbdclick(last_key[1]);
	k_to = addroottimeout(keyrep_time, kbd_repeat, 1);
}

static void
set_keyrepeat_timeout(short make)
{
	if (make)
	{
		if (k_to)
			cancelroottimeout(k_to);

		k_to = addroottimeout(keydel_time, kbd_repeat, 1);
	}
	else if (k_to)
	{
		cancelroottimeout(k_to);
		k_to = NULL;
	}
}
# endif

INLINE short
generate_mouse_event(uchar shift, ushort scan, ushort make)
{
	short delta = (shift & MM_ESHIFT) ? kbd_mpixels_fine : kbd_mpixels;

	switch (scan)
	{
		case UP_ARROW:
		{
			set_mouse_timeout(mouse_up, make, delta, ROOT_TIMEOUT);
			if ((keep_sending = make))
				kbdclick(scan);
			return -1;
		}
		case DOWN_ARROW:
		{
			set_mouse_timeout(mouse_down, make, delta, ROOT_TIMEOUT);
			if ((keep_sending = make))
				kbdclick(scan);
			return -1;
		}
		case RIGHT_ARROW:
		{
			set_mouse_timeout(mouse_right, make, delta, ROOT_TIMEOUT);
			if ((keep_sending = make))
				kbdclick(scan);
			return -1;
		}
		case LEFT_ARROW:
		{
			set_mouse_timeout(mouse_left, make, delta, ROOT_TIMEOUT);
			if ((keep_sending = make))
				kbdclick(scan);
			return -1;
		}
		case INSERT:
		{
			if (make)
			{
				if (shift & MM_ESHIFT)
					addroottimeout(ROOT_TIMEOUT, mouse_dclick, 1);
				else
					addroottimeout(ROOT_TIMEOUT, mouse_lclick, 1);

				kbdclick(scan);
			}

			return -1;
		}
		case CLRHOME:
		{
			if (make)
			{
				addroottimeout(ROOT_TIMEOUT, mouse_rclick, 1);

				kbdclick(scan);
			}

			return -1;
		}
	}
	return 0;
}

/* Routine called after the user hit Ctrl/Alt/Del
 */
static void
ctrl_alt_del(PROC *p, long arg)
{
	switch (cad[arg].action)
	{
		/* 1 is to signal a pid */
		case 1:
		{
			if (sys_p_kill(cad[arg].par.pid, cad[arg].aux.arg) < 0)
				sys_s_hutdown(arg);
			cad_lock = 0;
			break;
		}

		/* 2 shall be to exec a program
		 * with a path pointed to by par.path
		 */
		case 2:
		{
			if (sys_pexec (100, cad[arg].par.path, cad[arg].aux.cmd, cad[arg].env) < 0)
				sys_s_hutdown(arg);
			cad_lock = 0;
			break;
		}

		/* 0 is default */
		default:
		{
			sys_s_hutdown(arg);
			break;
		}
	}

	cad_lock = 0;
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
alt_help(PROC *p, long arg)
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
put_key_into_buf(IOREC_T *iorec, uchar c0, uchar c1, uchar c2, uchar c3)
{
	char *new_buf_pos;

# if 0
	/* bit 2 of conterm variable decides, whether we
	 * put the shift status to the buffer or not.
	 */
	if ((*(uchar *)0x0484L & 0x04) == 0)
		c0 = 0;
# endif
// 	display("c0 %x, c1 %x, c2 %x, c3 %x", c0, c1, c2, c3);

	iorec->tail += 4;
	if (iorec->tail >= iorec->buflen)
		iorec->tail = 0;

	/* Precalculating this saves some memory */
	new_buf_pos = iorec->bufaddr + iorec->tail;

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
		last_iorec  = iorec;
		last_key[0] = c0;
		last_key[1] = c1;
		last_key[2] = c2;
		last_key[3] = c3;
	}

	kintr = 1;
}

/* Translate scancode into ASCII according to the keyboard
 * translation tables.
 */

static const uchar iso_1_ctype[128] =
{
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xc0 */ _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu,
/* 0xd0 */ _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, 0x00, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu,
/* 0xe0 */ _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl,
/* 0xf0 */ _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, 0x00, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl
};

static const uchar iso_1_tolower[128] =
{
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xc0 */ 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
/* 0xd0 */ 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0x00, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
/* 0xe0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xf0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uchar iso_1_toupper[128] =
{
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xc0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xd0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xe0 */ 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
/* 0xf0 */ 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0x00, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf
};

static const uchar iso_2_ctype[128] =
{
/*         0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F */
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, _CTu, 0x00, _CTu, 0x00, _CTu, _CTu, 0x00, 0x00, _CTu, _CTu, _CTu, _CTu, 0x00, _CTu, _CTu,
/* 0xb0 */ 0x00, _CTl, 0x00, _CTl, 0x00, _CTl, _CTl, 0x00, 0x00, _CTl, _CTl, _CTl, _CTl, 0x00, _CTl, _CTl,
/* 0xc0 */ _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu,
/* 0xd0 */ _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, 0x00, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, _CTu, 0x00,
/* 0xe0 */ _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl,
/* 0xf0 */ _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, 0x00, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, _CTl, 0x00
};

static const uchar iso_2_tolower[128] =
{
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0xB1, 0x00, 0xB3, 0x00, 0xB5, 0xB6, 0x00, 0x00, 0xB9, 0xBA, 0xBB, 0xBC, 0x00, 0xBE, 0xBF,
/* 0xb0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xc0 */ 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
/* 0xd0 */ 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0x00, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0x00,
/* 0xe0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xf0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uchar iso_2_toupper[128] =
{
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb0 */ 0xA0, 0xA1, 0x00, 0xA3, 0x00, 0xA5, 0xA6, 0x00, 0x00, 0xA9, 0xAA, 0xAB, 0xAC, 0x00, 0xAE, 0xAF,
/* 0xc0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xd0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xe0 */ 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
/* 0xf0 */ 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0x00, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0x00
};

static const uchar *iso_tables[] =
{
	NULL, iso_1_ctype, iso_2_ctype, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL
};

static const uchar *iso_tolower_tables[] =
{
	NULL, iso_1_tolower, iso_2_tolower, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL
};

static const uchar *iso_toupper_tables[] =
{
	NULL, iso_1_toupper, iso_2_toupper, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL
};


INLINE uchar
iso_isupper(uchar c)
{
	const uchar *table;

	if (c < 128)
		return isupper(c);

	table = iso_tables[iso_8859_code];

	if (table == NULL)
		return isupper(c);

	c -= 128;

	return table[c] & _CTu;
}

INLINE uchar
iso_islower(uchar c)
{
	const uchar *table;

	if (c < 128)
		return islower(c);

	table = iso_tables[iso_8859_code];

	if (table == NULL)
		return islower(c);

	c -= 128;

	return table[c] & _CTl;
}

INLINE uchar
iso_tolower(uchar asc)
{
	const uchar *table;
	uchar c;

	if (asc < 128)
		return tolower(asc);

	if (iso_isupper(asc))
	{
		table = iso_tolower_tables[iso_8859_code];

		if (table == NULL)
			return tolower(asc);

		c = asc - 128;

		if (table[c])
			return table[c];
	}

	return asc;
}

INLINE uchar
iso_toupper(uchar asc)
{
	const uchar *table;
	uchar c;

	if (asc < 128)
		return toupper(asc);

	if (iso_islower(asc))
	{
		table = iso_toupper_tables[iso_8859_code];

		if (table == NULL)
			return toupper(asc);

		c = asc - 128;

		if (table[c])
			return table[c];
	}

	return asc;
}

INLINE uchar
scan2asc(uchar scancode)
{
	uchar asc = 0, *vec, shift = *kbshft;

	/* The AKP table structure is:
	 * ss, aa, ss, aa, ss, aa, 0
	 * where 'ss' is scancode and 'aa' is corresponding
	 * ASCII value.
	 */
	if (shift & (MM_ALTERNATE | MM_ALTGR))
	{
		if (shift & MM_ALTGR)
			vec = user_keytab->altgr;
		else
		{
			if (shift & MM_CTRL)
			{
				vec = NULL;

				if (shift & MM_ESHIFT)
					asc = user_keytab->shift[scancode];
				else
					asc = user_keytab->unshift[scancode];
			}
			else if (shift & MM_ESHIFT)
				vec = user_keytab->altshift;
			else if (shift & MM_CAPS)
				vec = user_keytab->altcaps;
			else
				vec = user_keytab->alt;
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
	else
	{
		/* Shift/1 should give "!" regardless of the Caps state
		 */
		if (shift & MM_ESHIFT)
			vec = user_keytab->shift;
		else if (shift & MM_CAPS)
			vec = user_keytab->caps;
		else
			vec = user_keytab->unshift;

		if (vec)
			asc = vec[scancode];
	}

	/* We can optionally emulate the PC-like behaviour of Caps/Shift */
	if (kbd_pc_style_caps)
	{
		if (((shift & MM_ALTGR) == 0) && (shift & MM_CAPS) && (shift & MM_ESHIFT))
			asc = iso_tolower(asc);
	}

	/* Ctrl key works as this regardless of the Alt/AltGr state.
	 * Otherwise the keyboard shortcuts (like Ctrl/Alt/Q) don't work
	 * anymore in N.AES.
	 */
	if (shift & MM_CTRL)
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

/* Keyboard interrupt routine.
 *
 * The scancode is passed from newkeys(), which in turn is called
 * by BIOS.
 *
 */

/* `scancode' is short, but only low byte matters. The high byte
 * is zeroed by newkeys().
 */

struct scanb_entry
{
	IOREC_T *iorec;
	unsigned short scan;
};

static TIMEOUT *ikbd_to = NULL;
static int scanb_head = 0;
static int scanb_tail = 0;
static struct scanb_entry scanb[16];
static void _cdecl IkbdScan(PROC *, long);

void _cdecl
ikbd_scan(ushort scancode, IOREC_T *rec)
{
	int tail = (scanb_tail + 1) & 0xf;

	if (tail != scanb_head)
	{
		scanb[scanb_tail].iorec = rec;
		scanb[scanb_tail].scan = scancode;
		scanb_tail = tail;
	}

# ifdef WITH_SINGLE_TASK_SUPPORT
	if( curproc->modeflags & M_SINGLE_TASK )
	{
# ifdef DEBUG_INFO
		extern short in_kernel;
		DEBUG(("ikbd_scan directly for '%s' head=%d p_flags=%lx slices=%d in_kernel=%x", curproc->name, scanb_head, curproc->p_mem->base->p_flags, curproc->slices, in_kernel ));
# endif
		IkbdScan( curproc, 1);
	}
	else
# endif

	if (!ikbd_to)
	{
		ikbd_to = addroottimeout(0L, IkbdScan, 1);
	}
}

static void _cdecl
IkbdScan(PROC *p, long arg)
{
	do
	{
		IOREC_T *iorec;
		ushort scancode;
		ushort mod = 0, clk = 0, x = 0, scan, make;
		uchar shift = *kbshft, ascii;

		iorec      = scanb[scanb_head].iorec;
		scancode   = scanb[scanb_head].scan;
		scanb_head = (scanb_head + 1) & 0xf;

		DEBUG(("ikbd_scan: scancode=%x, rec=%lx, h=%i, t=%i", scancode, iorec, scanb_head, scanb_tail));
// 		display("ikbd_scan: scancode=%x, rec=%lx, h=%i, t=%i", scancode, iorec, scanb_head, scanb_tail);

		scan = scancode & 0xff;

		if (is_eiffel_mouse_key(scan))
		{
			put_key_into_buf(iorec, shift, (uchar)scan, 0, 0);
			continue; //goto again;
		}

		/* This is set during various keyboard table initializations
		 * e.g. when the user calls Bioskeys(), to prevent processing
		 * go according to incomplete keyboard translation tables.
		 */
		if (kbd_lock)
			break;

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
		scan &= 0x7f;
		make = (scancode & 0x80) ? 0 : 1;

		/* We handle modifiers first
		 */
		while (modifiers[x])
		{
			if (scan == (ushort)modifiers[x])
			{
				if (make)
					shift |= mmasks[x];
				else
					shift &= ~mmasks[x];
				mod = 1;
				break;
			}
			x++;
		}

		switch (scan)
		{
			/* Caps toggles its bit, when hit, it also makes keyclick */
			case CAPS:
			{
				if (make)
				{
					shift ^= MM_CAPS;
					clk = mod = 1;
				}
				break;
			}
			/* Releasing Alternate should generate a character, whose ASCII
			 * code was typed in via the numpad.
			 */
			case ALTERNATE:
			{
				if (!make)
				{
					if (numidx)
					{
						ushort ascii_c;

						ascii_c = (ushort)atol((char *)numin);

						/* Only the values 0-255 are valid. Silently
						 * ignore the elder byte
						 */
						ascii_c &= 0x00ff;

						/* Reset the numin[] buffer for next use */
						numidx = 0;
						numin[0] = 0;

						put_key_into_buf(iorec, 0, 0, 0, (uchar)ascii_c);
						kbdclick(0);
					}
				}
				break;
			}
		}

		if (mod)
		{
			*kbshft = mshift = shift;

			/*
			 * CapsLock, ClrHome, Insert make keyclick,
			 * Alt, AltGr, Control and Shift don't.
			 */
			if (clk && make)
				kbdclick(scan);

			/*
			 * CapsLock key cannot be auto-repeated (unlike on original TOS).
			 * ClrHome and Insert must be handled as other keys.
			 */
			if ((scan != CLRHOME) && (scan != INSERT))
				continue;
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
# if 0
		if ((shift == MM_CTRLALT) && (scan == UNDO))
		{
			if (make)
			{
				killgroup(con_tty.pgrp, SIGQUIT, 1);
				kbdclick(scan);
			}

			return;
		}
# endif

		if ((shift & MM_CTRLALT) == MM_CTRLALT)
		{
			switch (scan)
			{
				case DEL:
				{
					if (make)
					{
						kbdclick(scan);

						if (!cad_lock)
						{
							TIMEOUT *t;

							t = addroottimeout (ROOT_TIMEOUT, ctrl_alt_del, 1);
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
							long mora = get_hz_200() - hz_ticks;

							if (mora > CAD_TIMEOUT)
							{
								spl7();
								reboot();
							}
						}
					}
					continue;
				}
				/* Function keys */
				case 0x003b ... 0x0044:
				{
					TIMEOUT *t;

					if (make)
					{
						if (shift & MM_ESHIFT)
							scan += 0x0019;		/* emulate F11-F20 */

						t = addroottimeout(ROOT_TIMEOUT, ctrl_alt_Fxx, 1);
						if (t) t->arg = scan;

						kbdclick(scan);
					}
					continue;
				}
				/* This is in case the keyboard has real F11-F20 keys on it */
				case 0x0054 ... 0x005d:
				{
					TIMEOUT *t;

					if (make)
					{
						t = addroottimeout(ROOT_TIMEOUT, ctrl_alt_Fxx, 1);
						if (t) t->arg = scan;

						kbdclick(scan);
					}

					continue;
				}
			}
		}

		/*
		 * Alt/arrow, alt/insert and alt/clrhome emulate the mouse events
		 */
		if ((shift & MM_ALTERNATE) && !(shift & (MM_CTRL|MM_ALTGR)))
		{
			if (generate_mouse_event(shift, scan, make) == -1)
				continue; //goto again;

			/* Ozk:
			 * Scan codes between 0x02 -> 0x0d are modified by 0x76
			 */
			if (scan >= 0x02 && scan <= 0x0d)
				scan += 0x76;
		}

		/*
		 * Emulate F11-F20 keys
		 */
		if (shift & MM_ESHIFT)
		{
			if (scan >= 0x003b && scan <= 0x0044)
				scan += 0x0019;
		}

		/*
		* Control alters scancodes of some keys
		*/
		if (shift & MM_CTRL)
		{
			if (scan == 0x004b) scan = 0x0073;	/* arrow */
			else if (scan == 0x004d) scan = 0x0074;	/* arrow */
			else if (scan == 0x0047) scan = 0x0077;	/* Clr/Home */
		}

		if (shift == MM_ALTERNATE)
		{
			switch (scan)
			{
				/* Alt/Help fires up a program named `althelp.sys'
				 * located in the system directory (e.g. `c:/mint/')
				 */
				case HELP:
				{
					if (make)
					{
						addroottimeout(ROOT_TIMEOUT, alt_help, 1);
						kbdclick(scan);
					}
					continue;
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
					if (make)			/* we ignore release codes */
					{
						if (numidx > 2)		/* buffer full? reset it */
						{
							numin[0] = 0;
							numidx = 0;
						}

						ascii = user_keytab->unshift[scan];

						if (ascii)
						{
							numin[numidx] = ascii;
							numidx++;
							numin[numidx] = 0;
						}

						kbdclick(scan);
					}

					continue; //goto again;
				}
			}
		}

		/* All keys below undergo user-defined translation and autorepetition
		 */

		/* Ordinary keyboard here.
		 */
		if (make)
		{
			/* Deadkeys.
			 * The deadkey table structure is as follows:
			 * dd,bb,aa,dd,bb,aa,...,aa,bb,aa,0
			 * Where dd is the deadkey character, aa is the base
			 * character and aa the accented character.
			 * So '^','a','ƒ' means that '^' followed by 'a' results
			 * in an 'ƒ'.
			 */
			uchar *vec = user_keytab->deadkeys;
			ascii = scan2asc((uchar)scan);

			if (vec)
			{
				static unsigned int last_deadkey_scan = 0;
				static uchar last_deadkey = 0;
				uchar deadkey, base, accented = 0;
				int is_deadkey = 0;

				while (*vec)
				{
					deadkey = *vec++;
					base = *vec++;
					accented = *vec++;

					if (ascii == deadkey)
					{
						is_deadkey = 1;
						accented = 0;
						break;
					}

					if (deadkey == last_deadkey && ascii == base)
						break;

					accented = 0;
				}

				if (last_deadkey)
				{
					if (accented)
						put_key_into_buf(iorec, shift, (uchar)scan, 0, accented);
					else
					{
						put_key_into_buf(iorec, shift, (uchar)last_deadkey_scan, 0, last_deadkey);

						if (ascii && ascii != ' ' && ascii != '\t' && (ascii == last_deadkey ? 0:1))
							put_key_into_buf(iorec, shift, (uchar)scan, 0, ascii);
					}

					last_deadkey = 0;
				}
				else
				{
					if (is_deadkey)
					{
						last_deadkey = ascii;
						last_deadkey_scan = scan;
					}
					else
						put_key_into_buf(iorec, shift, (uchar)scan, 0, ascii);
				}
			}
			else
				put_key_into_buf(iorec, shift, (uchar)scan, 0, ascii);

			kbdclick(scan);
		}
#ifndef MILAN
		set_keyrepeat_timeout(make);
#endif
	} while (scanb_head != scanb_tail);

	ikbd_to = NULL;
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

struct keytab *get_keytab(void) { return user_keytab; }

struct keytab * _cdecl
sys_b_keytbl(char *unshifted, char *shifted, char *caps)
{
	if ((long)unshifted > 0 || (long)shifted > 0 || (long)caps > 0)
	{
		ALERT(MSG_keyboard_keytbl_ignored, (long)unshifted, (long)shifted, (long)caps);
	}

	return user_keytab;
}

static void
set_kbrate_ms(short delay, short rate)
{
	if (delay >= 0)
		keydel_time = delay ? ((long)((long)(delay & 0x00ff) * 1000) / 50) : 20;

	if (rate >= 0)
		keyrep_time = rate ? ((long)((long)(rate & 0x00ff) * 1000) / 50) : 20;
}

/*
 * The XBIOS' Kbrate() function
 */
ushort _cdecl
sys_b_kbrate(short delay, short rate)
{
	ushort ret;
#ifdef MILAN
	ushort mdelay, mrate;
#endif

#ifdef MILAN
	ret = mdelay = (unsigned short)((keydel_time * 50) / 1000) & 0x00ff;
#else
	ret = (unsigned short)((keydel_time * 50) / 1000) & 0x00ff;
#endif
	ret <<= 8;
#ifdef MILAN
	ret |= (mrate = (unsigned short)((keyrep_time * 50) / 1000) & 0x00ff);
#else
	ret |= (unsigned short)((keyrep_time * 50) / 1000) & 0x00ff;
#endif
	set_kbrate_ms(delay, rate);

#ifdef MILAN
	(void) ROM_Kbrate(mdelay, mrate);
#endif

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
typedef struct
{
  unsigned int bootpref;
  char reserved[4];
  unsigned char language;
  unsigned char keyboard;
  unsigned char datetime;
  char separator;
  unsigned char bootdelay;
  char reserved2[3];
  unsigned int vmode;
  unsigned char scsi;
} NVM;

static short get_NVM_lang(void)
{
	NVM nvm;
	if( !TRAP_NVMaccess( 0, 0, sizeof(NVM), &nvm ) )
		return nvm.language;
	return -1;
}

void _cdecl
sys_b_bioskeys(void)
{
	long akp_val = 0, r;
	unsigned char *buf, *tables;
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

	/* Reserve one region for both keytable and its vectors (globally accessible!) */
	user_keytab_region = get_region(core, keytab_size + sizeof(struct keytab), PROT_G);

	buf = (unsigned char *)attach_region(rootproc, user_keytab_region);

	pointers = (struct keytab *)buf;
	tables = buf + sizeof(struct keytab);

	assert(pointers);

	/* Copy the master table over */
	quickmovb(tables, keytab_buffer, keytab_size);

	/* Setup the standard vectors */
	pointers->unshift = tables;
	pointers->shift = tables + 128;
	pointers->caps = tables + 128 + 128;

	/* and the AKP vectors */
	pointers->alt = tables + 128 + 128 + 128;
	pointers->altshift = tbl_scan_fwd(pointers->alt);
	pointers->altcaps = tbl_scan_fwd(pointers->altshift);
	pointers->altgr = tbl_scan_fwd(pointers->altcaps);

	/* and the deadkeys */
	pointers->deadkeys = tbl_scan_fwd(pointers->altgr);

	/* Fix the _AKP cookie, gl_kbd may get changed in load_table().
	 *
	 * XXX must be changed for -DJAR_PRIVATE (forward to all processes).
	 */

	/* _AKP specifies the hardware keyboard layout */
	r = get_cookie(NULL, COOKIE__AKP, (unsigned long *)&akp_val);
	if( r )
	{
		r = get_NVM_lang();
		if( r < 0 )
		{
			if( gl_lang < 127 )
				r = gl_lang;
			else
				r = 0;	/* ERROR! */
		}
		gl_lang = r;
	}
	akp_val &= 0xffff0000L;
	akp_val |= (gl_kbd & 0x000000ff) | (gl_lang << 8);
	r = set_cookie(NULL, COOKIE__AKP, akp_val);

	/* _ISO specifies the real keyboard/font nationality */
	set_cookie(NULL, COOKIE__ISO, iso_8859_code);

	user_keytab = pointers;
	/* Done! */
	kbd_lock = 0;

	DEBUG(("*sys_b_bioskeys:return"));
}

/* Kbdvbase() */
KBDVEC * _cdecl
sys_b_kbdvbase(void)
{
	return syskey;
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
		DEBUG(("%s(): invalid size %ld", __FUNCTION__, size));
		return EFTYPE;
	}

	kbuf = kmalloc(size+2); /* Append two zeros in case the altgr + deadkey tables are missing from the file. */
	if (!kbuf)
	{
		DEBUG(("%s(): out of memory", __FUNCTION__));
		return ENOMEM;
	}

	mint_bzero(kbuf, size+2); /* Don't forget to clear the buffer in case the file is missing any tables. */

	if ((*fp->dev->read)(fp, (char *)kbuf, size) == size)
	{
		switch (*(ushort *)kbuf)
		{
			case 0x2771:		/* magic word for std format */
			{
				quickmovb(kbuf, kbuf + sizeof(short), size - sizeof(short) + 2);
				break;
			}
			case 0x2772:		/* magic word for ext format */
			{
				/* The extended format is identical as the old one
				 * with exception that the second word of the data
				 * contains the AKP code for the keyboard table
				 * loaded.
				 */
				short *sbuf = (short *)kbuf;

				/* -1 for the AKP field means "ignore" */
				if ((sbuf[1] >= 0) && (sbuf[1] <= MAXAKP))
					gl_kbd = sbuf[1];

				quickmovb(kbuf, kbuf + sizeof(long), size - sizeof(long) + 2);
				break;
			}
			case 0x2773:		/* the ISO format (as of 30.VII.2004) */
			{
				/* The ISO format header consists of:
				 * 0x2773, the AKP code as above, and the longword containing
				 * ISO-8859 code for the keyboard (0 - undefined).
				 */
				short *sbuf = (short *)kbuf;

				/* -1 for the AKP field means "ignore" */
				if ((sbuf[1] >= 0) && (sbuf[1] <= MAXAKP))
					gl_kbd = sbuf[1];

				if ((sbuf[3] > 0) && (sbuf[3] <= 10))
				{
					iso_8859_code = (long)sbuf[3];
					quickmovb(kbuf, kbuf + (sizeof(long)*2), \
							size - (sizeof(long)*2) + 2);
				}
				else
				{
					DEBUG(("%s(): invalid ISO header", __FUNCTION__));

					ret = EFTYPE;	/* wrong format */
				}

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

	keytab_buffer = (char *)kbuf;
	keytab_size = size+2;

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
		size += strlen((char *)tos_keytab->alt) + 1;
		size += strlen((char *)tos_keytab->altshift) + 1;
		size += strlen((char *)tos_keytab->altcaps) + 1;
		if (mch == MILAN_C)
			size += strlen((char *)tos_keytab->altgr) + 1;
		else
			size += 2;
		size += 2; /* For the empty deadkey table */
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
	size += strlen(tos_keytab->deadkeys) + 1;

	size += 8; /* add some space */
# endif

	/* If a buffer was allocated previously, we can perhaps reuse it.
	 */
	if (keytab_buffer && (keytab_size >= size))
	{
		kbuf = (unsigned char *)keytab_buffer;
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

	p = kbuf;
	mint_bzero(p, size);

	quickmovb(p, tos_keytab->unshift, 128);
	p += 128;

	quickmovb(p, tos_keytab->shift, 128);
	p += 128;

	quickmovb(p, tos_keytab->caps, 128);
	p += 128;

# ifndef WITHOUT_TOS

	if (tosvers >= 0x0400)
	{
		len = strlen((char *)tos_keytab->alt) + 1;
		quickmovb(p, tos_keytab->alt, len);
		p += len;

		len = strlen((char *)tos_keytab->altshift) + 1;
		quickmovb(p, tos_keytab->altshift, len);
		p += len;

		len = strlen((char *)tos_keytab->altcaps) + 1;
		quickmovb(p, tos_keytab->altcaps, len);
		p += len;

		if (machine == machine_milan)
		{
			len = strlen((char *)tos_keytab->altgr) + 1;
			quickmovb(p, tos_keytab->altgr, len);
		}
	}

# else
	/* Our default keyboard table (see key_table.h) is always
	 * a complete one.
	 */

	len = strlen((char *)tos_keytab->alt) + 1;
	quickmovb(p, tos_keytab->alt, len);
	p += len;

	len = strlen((char *)tos_keytab->altshift) + 1;
	quickmovb(p, tos_keytab->altshift, len);
	p += len;

	len = strlen((char *)tos_keytab->altcaps) + 1;
	quickmovb(p, tos_keytab->altcaps, len);
	p += len;

	len = strlen((char *)tos_keytab->altgr) + 1;
	quickmovb(p, tos_keytab->altgr, len);
	p += len;

	len = strlen((char *)tos_keytab->deadkeys) + 1;
	quickmovb(p, tos_keytab->deadkeys, len);

	gl_kbd = default_akp;
# endif

	keytab_buffer = (char *)kbuf;
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
			if (flag & 0x01)
			{
				if (ret == 0)
					boot_printf(MSG_keytable_loaded, gl_kbd, iso_8859_code);
				else
					boot_printf(MSG_init_error, ret);
			}
			boot_printf("\r\n");
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

	set_kbrate_ms(delayrate >> 8, delayrate & 0x00ff);

	TRACE(("%s(): delay 0x%04ld, rate 0x%04ld", __FUNCTION__, keydel_time, keyrep_time));
	TRACE(("%s(): conterm 0x%02x", __FUNCTION__, (short)(*(char *)0x0484L)));

	/* initialize internal table */
# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_internal);
# endif
	load_internal_table(); /* either from BIOS or builtin */
# ifdef VERBOSE_BOOT
	boot_printf(MSG_keytable_loaded, gl_kbd, iso_8859_code);
	boot_printf("\r\n");
# endif

}

# else

struct keytab * _cdecl
sys_b_keytbl(char *unshifted, char *shifted, char *caps)
{
	return ROM_Keytbl(unshifted, shifted, caps);
}

void _cdecl
sys_b_bioskeys(void)
{
	ROM_Bioskeys();
}

ushort _cdecl
sys_b_kbrate(short delay, short rate)
{
	return ROM_Kbrate(delay, rate);
}

KBDVEC * _cdecl
sys_b_kbdvbase(void)
{
	return (KBDVEC * _cdecl)ROM_Kbdvbase();
}

# endif	/* NO_AKP_KEYBOARD */

/* EOF */
