/*
 * keyboard.h
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
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
 */

# ifndef _keyboard_h
# define _keyboard_h

# include "mint/mint.h"
# include "mint/emu_tos.h"

/* modifier masks for the kbshift() */
# define MM_RSHIFT	0x01
# define MM_LSHIFT	0x02
# define MM_CTRL	0x04
# define MM_ALTERNATE	0x08
# define MM_CAPS	0x10
# define MM_CLRHOME	0x00
# define MM_INSERT	0x40
# define MM_ALTGR	0x80	/* Milan */

/* masks for key combinations */
# define MM_ESHIFT	0x03	/* either shift */
# define MM_CTRLALT	0x0c

/* some key definitions */
# define CONTROL	0x1d	/* scan code for control key */
# define LSHIFT		0x2a	/* scan code for left shift */
# define RSHIFT		0x36	/* scan code for right shift */
# define ALTERNATE	0x38	/* scan code for alternate key */
# define CAPS		0x3a	/* scan code of caps lock key */
# define CLRHOME	0x47	/* scan code for clr/home key */
# define ALTGR		0x4c	/* scan code for AltGr key (on Milan) */
# define INSERT		0x52	/* scan code for insert key */
# define DEL		0x53	/* scan code of delete key */
# define UNDO		0x61	/* scan code of undo key */
# define HELP		0x62	/* scan code of help key */

# define UP_ARROW	0x48	/* cursor keys */
# define DOWN_ARROW	0x50
# define LEFT_ARROW	0x4b
# define RIGHT_ARROW	0x4d

# define NUMPAD_7	0x67	/* definitions for numpad scancodes */
# define NUMPAD_8	0x68
# define NUMPAD_9	0x69
# define NUMPAD_4	0x6a
# define NUMPAD_5	0x6b
# define NUMPAD_6	0x6c
# define NUMPAD_1	0x6d
# define NUMPAD_2	0x6e
# define NUMPAD_3	0x6f
# define NUMPAD_0	0x70

# define UNDEF		0x00	/* bios.c and tty.c */

# define MAXAKP		126	/* maximum _AKP code supported */

/* Vectors for scancode -> ASCII conversions
 */
struct keytab
{
	uchar *unshift;		/* every TOS */
	uchar *shift;		/* every TOS */
	uchar *caps;		/* every TOS */
	uchar *alt;		/* TOS 4.0x and above */
	uchar *altshift;	/* TOS 4.0x and above */
	uchar *altcaps;		/* TOS 4.0x and above */
	uchar *altgr;		/* Milan TOS */
	uchar *deadkeys;	/* FreeMiNT 1.17 */
};

/* Struct for the default action on C/A/D
 */
struct cad_def
{
	short	action;
	union {
		long pid;	/* e.g. pid to be signalled */
		char *path;	/* e.g. a path to executable file */
	} par;
	union {
		long arg;	/* e.g. signal number */
		char *cmd;	/* e.g. command line */
	} aux;
	char *env;		/* only valid for exec */
};

extern short kbd_pc_style_caps;
extern short kbd_mpixels;
extern short kbd_mpixels_fine;
extern struct cad_def cad[3];
extern int has_kbdvec;
extern bool emutos;

/* Interrupt routines */
void _cdecl ikbd_scan(ushort scancode, IOREC_T *rec);
void autorepeat_timer(void);

/* keyboard related BIOS system calls */
struct keytab * _cdecl sys_b_keytbl(char *unshift, char *shift, char *caps);
void _cdecl sys_b_bioskeys(void);
ushort _cdecl sys_b_kbrate(short del, short rep);
KBDVEC * _cdecl sys_b_kbdvbase(void);

/* internal support routines */
struct keytab *get_keytab(void);
long load_keyboard_table(const char *path, short flag);
void init_keybd(void);

# endif /* _keyboard_h */
