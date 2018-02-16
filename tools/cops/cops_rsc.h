/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _cops_rsc_h
#define _cops_rsc_h

#include <gem.h>
#include "global.h"

#define	EXTERN extern
#define BYTE char
#define WORD short
#define LONG long
#define rs_tedinfo	rsc_rs_tedinfo
#define rs_ciconblk	rsc_rs_ciconblk
#define rs_cicon	rsc_rs_cicon
#define rs_iconblk	rsc_rs_iconblk
#define rs_bitblk	rsc_rs_bitblk
#define rs_frstr	fstring_addr
#define rs_frimg	rsc_rs_frimg
#define rs_trindex	tree_addr
#define rs_object	rsc_rs_object

#include "cops_rs.h"
#include "cops_rs.rh"

#ifdef WITH_RSC_DEFINITIONS
#include "cops_rs.rsh"
#endif

#undef rs_tedinfo
#undef rs_ciconblk
#undef rs_cicon
#undef rs_iconblk
#undef rs_bitblk
#undef rs_frstr
#undef rs_frimg
#undef rs_trindex
#undef rs_object
#undef LONG
#undef WORD
#undef BYTE
#undef EXTERN

#endif /* _cops_rsc_h */
