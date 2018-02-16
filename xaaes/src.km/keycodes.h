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

#ifndef _keycodes_h_
#define	_keycodes_h_

#define	SC_CTRL_C		0x2e03
#define SC_CTRL_V		0x2f16
#define SC_CTRL_X		0x2d18

#define SC_SPACE		0x3920
#define SC_ESC			0x011b
#define SC_BACKSPACE		0x0e08
#define SC_DEL			0x537f
#define SC_TAB			0x0f09

#define SC_NMPAD_ENTER		0x720d

#define SC_HELP			0x6200
#define SC_UNDO			0x6100
#define SC_INSERT		0x5200
#define SC_RETURN		0x1c0d

#define SC_CLRHOME		0x4700
#define SC_SHFT_CLRHOME		0x4737
#define SC_CTRL_CLRHOME		0x7700

#define SC_UPARROW		0x4800
#define SC_DNARROW		0x5000
#define SC_LFARROW		0x4b00
#define SC_RTARROW		0x4d00
#define SC_SHFT_UPARROW		0x4838
#define SC_SHFT_DNARROW		0x5032
#define SC_SHFT_LFARROW		0x4b34
#define SC_SHFT_RTARROW		0x4d36
/*
 * Left/Right arrows have different scan with ctrl,
 * while up/down arrows are unaffected by ctrl
 */
#define SC_CTRL_LFARROW		0x7300
#define SC_CTRL_RTARROW		0x7400

/*
 * Keys supported by Milan
 */
#define SC_PGUP			0x4900
#define SC_PGDN			0x5100
#define SC_END			0x4f00

#endif /* _keycodes_h_ */
