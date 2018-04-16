/*
 * NetUSBee USB driver for FreeMiNT.
 * Modified for USB by David Galvez. 2010 - 2011
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Henrik Gilda
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
 */

#ifndef _netusbee_int_h
#define _netusbee_int_h

#ifdef TOSONLY
#define SET_INT_LVL6		Supexec(set_int_lvl6)
#define SET_OLD_INT_LVL		Supexec(set_old_int_lvl)
#else
#define SET_INT_LVL6		set_int_lvl6()
#define SET_OLD_INT_LVL		set_old_int_lvl()
#endif

// old handler
extern void (*old_200Hz_int)(void);

// interrupt wrapper routine
void interrupt_200Hz(void);

void set_old_int_lvl(void);
void set_int_lvl5(void);
void set_int_lvl6(void);
void set_int_lvl7(void);

#endif // _netusbee_int_h
