/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2002 Adam Klobukowski <atari@gabo.pl>
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

#ifndef _mint_m68k_psg_h
#define _mint_m68k_psg_h
 
struct psg {
  uchar control;
  uchar pad0;
  uchar data;
};

#define PSG ((volatile struct psg *) 0xffff8800)

/* bits in PSG_MULTI register: */

/* TODO */

/* PSG registers */

#define PSG_MULTI  0x7
#define PSG_PORT_A 0xE
#define PSG_PORT_B 0xF

 
#endif /* _mint_m68k_psg_h */
