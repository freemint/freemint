/*
 * $Id$
 * 
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

/* modifier masks for the kbshift() */
# define MM_RSHIFT	0x01
# define MM_LSHIFT	0x02
# define MM_CTRL	0x04
# define MM_ALTERNATE	0x08
# define MM_CAPS	0x10
# define MM_CLRHOME	0x20
# define MM_INSERT	0x40

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
# define INSERT		0x52	/* scan code for insert key */
# define DEL		0x53	/* scan code of delete key */
# define UNDO		0x61	/* scan code of undo key */
# define HELP		0x62	/* scan code of help key */

# define NUMPAD_0	0x70	/* definitions for numpad scancodes */
# define NUMPAD_1	0x6d
# define NUMPAD_2	0x6e
# define NUMPAD_3	0x6f
# define NUMPAD_4	0x6a
# define NUMPAD_5	0x6b
# define NUMPAD_6	0x6c
# define NUMPAD_7	0x67
# define NUMPAD_8	0x68
# define NUMPAD_9	0x69

# define MAXAKP		126	/* maximum _AKP code supported */

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

/* Vectors for scancode -> ASCII conversions
 */
struct keytab
{
	uchar *unshift;
	uchar *shift;
	uchar *caps;
	uchar *alt;
	uchar *altshift;
	uchar *altcaps;
};

short ikbd_scan(ushort scancode);

void init_keybd(void);
void load_keytbl(void);
void bioskeys(void);

extern short gl_kbd;
extern struct cad_def cad[3];


# endif /* _keyboard_h */
