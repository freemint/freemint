/*
 * $Id$
 *
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

#include "xa_types.h"
#include "win_draw.h"

static DrawWinElement draw_border;

struct xawin_functions xawf = 
{
	draw_border,
	NULL,		/* title */
	NULL,		/* info */
	NULL,		/* widgets */
	NULL,		/* hslide */
	NULL,		/* vslide */
	NULL,		/* sizer */
};

static void
draw_border(struct xa_window *wind)
{
	if (wind->frame > 0)
	{
		RECT cl = wind->r;
		short i, p[16];

		for (i = wind->frame; i < 0; i++)
		{
			p[ 0] = cl.x;
			p[ 1] = cl.y;
			p[ 2] = cl.x + cl.w - 1;
			p[ 3] = p[ 1];

			p[ 4] = p[ 2];
			p[ 5] = p[ 1] + cl.h - 1;
		
			p[ 6] = p[ 0];
			p[ 7] = p[ 5];

			p[ 8] = p[ 0];
			p[ 9] = p[ 1];
		
			v_pline(wind->vdi_handle, 5, p);

			cl.x++;
			cl.y++;
			cl.w -= 2;
			cl.h += 2;
		}
	}
}
