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

#ifndef _vdi_parms_h_
#define _vdi_parms_h_

#include "xa_aes.h"

struct vdi_xfnt_info_parms
{
	short	flags;
	short	id;
	short	index;
	XFNT_INFO *info;
};

#define V_XFNT_INFO(_vpb, _handle, _flags, _id, _index, _info) {	\
	struct vdi_xfnt_info_parms *_p = (void *)_vpb->intin;		\
	_p->flags = _flags;						\
	_p->id = _id;							\
	_p->index = _index;						\
	_p->info = _info;						\
	VDI(_vpb, 229, 0, 5, 0, _handle);				\
}
#define VST_POINT(_vpb, _handle, _point, _ret)		\
	_vpb->intin[0] = _point;			\
	VDI(vpb, 107, 0, 1, 0, handle);			\
	_ret = _vpb->intout[0];

#endif /* _vdi_parms_h_ */
