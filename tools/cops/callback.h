/*
 * $Id$
 * 
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

/*
 * XCPB-Funktionen
 */

#ifndef _callback_h
#define _callback_h

#include "global.h"
#include "cpx.h"


extern XCPB xctrl_pb;


CPX_LIST *_cdecl get_cpx_list(void);

short _cdecl save_header(CPX_LIST *cpx_list);

void _cdecl
cpx_fix_rsh(CPX_DESC *cpx_desc, short num_objs, short num_frstr, short num_frimg, short num_tree,
	    OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_string[], ICONBLK *rs_iconblk, BITBLK *rs_bitblk,
	    long *rs_frstr, long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope);

void _cdecl
cpx_rsh_obfix(CPX_DESC *cpx_desc, OBJECT *tree, short ob);

void _cdecl rsh_fix(short num_objs, short num_frstr, short num_frimg, short num_tree,
		    OBJECT *rs_object, TEDINFO *rs_tedinfo, char *rs_strings[],
		    ICONBLK *rs_iconblk, BITBLK *rs_bitblk,
		    long *rs_frstr, long *rs_frimg, long *rs_trindex, struct foobar *rs_imdope);

void _cdecl rsh_obfix(OBJECT *tree, short ob);

short _cdecl Popup(char **strs, short no_strs, short slct, short font,
		   GRECT *button_rect, GRECT *tree_rect);

void _cdecl Sl_size(OBJECT *tree, short base, short slider, short num_items,
		    short visible, short direction, short min_size);

void _cdecl Sl_x(OBJECT *tree, short base, short slider, short value,
		 short min, short max, void (*userdef)(void));

void _cdecl Sl_y(OBJECT *tree, short base, short slider, short value,
		 short min, short max, void (*userdef)(void));

void _cdecl Sl_arrow(OBJECT *tree, short base, short slider, short obj, short inc,
		     short min, short max, short *value, short direction, void (*userdef)(void));

void _cdecl Sl_dragx(OBJECT *tree, short base, short slider,
		     short min, short max, short *value, void (*userdef)(void));

void _cdecl Sl_dragy(OBJECT *tree, short base, short slider,
		     short min, short max, short *value, void (*userdef)(void));

CPX_DESC *cpx_form_do(CPX_DESC *cpx_desc, OBJECT *tree, short edit_obj, short *msg);
GRECT *cpx_get_first_rect(CPX_DESC *cpx_desc, GRECT *redraw_area);
GRECT *cpx_get_next_rect(CPX_DESC *cpx_desc);
void cpx_set_evnt_mask(CPX_DESC *cpx_desc, short mask, MOBLK *m1, MOBLK *m2, long evtime);
short cpx_save_data(CPX_DESC *cpx_desc, void *buf, long no_bytes);

short _cdecl XGen_Alert(short alert_id);

short _cdecl CPX_Save(void *ptr, long bytes);

void *cpx_get_tmp_buffer(CPX_DESC *cpx_desc);

short _cdecl getcookie(long cookie, long *p_value);

void _cdecl MFsave(short flag, MFORM *mf);

#endif /* _callback_h */
