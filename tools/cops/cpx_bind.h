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

#ifndef _cpx_bind_h
#define _cpx_bind_h

#include "global.h"
#include "cpx.h"

short  _cdecl Xform_do(OBJECT *tree, short edit_obj, short *msg);
GRECT *_cdecl GetFirstRect(GRECT *prect);
GRECT *_cdecl GetNextRect(void);
void   _cdecl Set_Evnt_Mask(short mask, MOBLK *m1, MOBLK *m2, long evtime);
void * _cdecl Get_Buffer(void);

void a_call_main(void);
void new_context(CPX_DESC *cpx_desc);
void switch_context(CPX_DESC *cpx_desc);

void cpx_userdef(void (*cpx_userdef)(void));

/* cpx callbacks */
CPXINFO	*cpx_init(CPX_DESC *cpx_desc, XCPB *xcpb);
short cpx_call(CPX_DESC *cpx_desc, GRECT *work);
void cpx_draw(CPX_DESC *cpx_desc, GRECT *clip);
void cpx_wmove(CPX_DESC *cpx_desc, GRECT *work);
short cpx_timer(CPX_DESC *cpx_desc);
short cpx_key(CPX_DESC *cpx_desc, short kstate, short key);
short cpx_button(CPX_DESC *cpx_desc, MRETS *mrets, short nclicks);
short cpx_m1(CPX_DESC *cpx_desc, MRETS *mrets);
short cpx_m2(CPX_DESC *cpx_desc, MRETS *mrets);
short cpx_hook(CPX_DESC *cpx_desc, short event, short *msg, MRETS *mrets, short *key, short *nclicks);
void cpx_close(CPX_DESC *cpx_desc, short flag);

#endif /* _cpx_bind_h */
