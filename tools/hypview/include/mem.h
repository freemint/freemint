/*
 * $Id$
 * 
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _mem_h
#define _mem_h

/*	Mxalloc()-Modus	*/
#define MX_STRAM	0
#define MX_TTRAM	1
#define	MX_PREFST	2
#define MX_PREFTT	3

/* Protection bits.  */
#define MX_MPROT	(1<<3)

#define MX_PRIVATE	(1<<4)
#define MX_GLOBAL	(2<<4) 			/*	globaler Speicher anfordern	*/
#define MX_SUPER	(3<<4)
#define MX_READABLE	(4<<4)

#endif
