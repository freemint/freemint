/*
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

#ifndef _about_h
#define _about_h

#include "global.h"
#include "taskman.h"

extern GRECT about_r;
extern GRECT view_r;

void reset_about(void);
void open_about(int lock, struct xa_client *client, bool open, char *fn);
void add_keybd_switch(char *k);

enum CtrlAltMd{
	Set = 0,
	Get,
	Reset
};

int DoCtrlAlt( enum CtrlAltMd md, int orig, int repl );

#endif /* _about_h */
