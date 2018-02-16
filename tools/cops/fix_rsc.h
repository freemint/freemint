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

#ifndef _fix_rsc_h
#define _fix_rsc_h

#include <gem.h>
#include "global.h"

struct foobar;

void fix_rsc(short num_objs, short num_frstr, short num_frimg, short num_tree,
	     OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_string[],
	     ICONBLK *rs_iconblk, BITBLK *rs_bitblk, long *rs_frstr,
	     long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope);

#endif /* _fix_rsc_h */
