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

#ifndef MX_STRAM
#define MX_STRAM		0x00
#endif

#ifndef MX_TTRAM
#define MX_TTRAM		0x01
#endif
#ifndef MX_PREFSTRAM
#define MX_PREFSTRAM		0x02
#endif
#ifndef MX_PREFTTRAM
#define MX_PREFTTRAM		0x03
#endif
#ifndef MX_MPROT
#define MX_MPROT		0x04
#endif
/* if bit #3 is set, then */
#ifndef MX_HEADER
#define MX_HEADER		0x00
#endif
#ifndef MX_PRIVATE
#define MX_PRIVATE		0x10
#endif
#ifndef MX_GLOBAL
#define MX_GLOBAL		0x20
#endif
#ifndef MX_SUPERVISOR
#define MX_SUPERVISOR	0x30
#endif
#ifndef MX_READABLE
#define MX_READABLE		0x40
#endif

#endif /* _mem_h */
