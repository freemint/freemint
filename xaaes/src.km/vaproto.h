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

/*
 * AV-protocol, extended definitions from 26.06.1995
 * 
 * You can find a detailed description of these messages
 * in the Thing documentation (thing.hyp, starting from version 0.54)
 */

#ifndef _vaproto_h
#define _vaproto_h

#define VAPRO 1

/* AES-Messages */
enum av_va
{
	AV_PROTOKOLL      = 0x4700,
	VA_PROTOSTATUS    = 0x4701,
	AV_GETSTATUS      = 0x4703,
	AV_STATUS         = 0x4704,
	VA_SETSTATUS      = 0x4705,
	AV_SENDKEY        = 0x4710,
	VA_START          = 0x4711,
	AV_ASKFILEFONT    = 0x4712,
	VA_FILEFONT       = 0x4713,
	AV_ASKCONFONT     = 0x4714,
	VA_CONFONT        = 0x4715,
	AV_ASKOBJECT      = 0x4716,
	VA_OBJECT         = 0x4717,
	AV_OPENCONSOLE    = 0x4718,
	VA_CONSOLEOPEN    = 0x4719,
	AV_OPENWIND       = 0x4720,
	VA_WINDOPEN       = 0x4721,
	AV_STARTPROG      = 0x4722,
	VA_PROGSTART      = 0x4723,
	AV_ACCWINDOPEN    = 0x4724,
	VA_DRAGACCWIND    = 0x4725,
	AV_ACCWINDCLOSED  = 0x4726,

	AV_COPY_DRAGGED   = 0x4728,
	VA_COPY_COMPLETE  = 0x4729,
	AV_PATH_UPDATE    = 0x4730,
	AV_WHAT_IZIT      = 0x4732,
	VA_THAT_IZIT      = 0x4733,
	AV_DRAG_ON_WINDOW = 0x4734,
	VA_DRAG_COMPLETE  = 0x4735,
	AV_EXIT           = 0x4736,
	AV_STARTED        = 0x4738,
	VA_FONTCHANGED    = 0x4739,
	AV_XWIND          = 0x4740,
	VA_XOPEN          = 0x4741,

	/* new messages since 26.06.1995 */

	AV_VIEW           = 0x4751,
	VA_VIEWED         = 0x4752,
	AV_FILEINFO       = 0x4753,
	VA_FILECHANGED    = 0x4754,
	AV_COPYFILE       = 0x4755,
	VA_FILECOPIED     = 0x4756,
	AV_DELFILE        = 0x4757,
	VA_FILEDELETED    = 0x4758,
	AV_SETWINDPOS     = 0x4759,
	VA_PATH_UPDATE    = 0x4760,

	VA_HIGH
};

/* Objekttypen for VA_THAT_IZIT */
enum
{
	VA_OB_UNKNOWN,
	VA_OB_TRASHCAN,
	VA_OB_SHREDDER,
	VA_OB_CLIPBOARD,
	VA_OB_FILE,
	VA_OB_FOLDER,
	VA_OB_DRIVE,
	VA_OB_WINDOW
};

enum av_va_give
{
	VV_SETSTATUS	= 0x0001,
	VV_START	= 0x0002,
	VV_STARTED	= 0x0004,
	VV_FONTCHANGED	= 0x0008,
	VV_ACC_QUOTING	= 0x0010,
	VV_PATH_UPDATE	= 0x0020
};

enum av_va_have
{
	/* mp[3]: */
	AA_SENDKEY	= 0x0001,	/*  b0: MAGXDESK, THING */
	AA_ASKFILEFONT	= 0x0002,	/*  b1:           THING */
	AA_ASKCONFONT	= 0x0004,	/*  b2:           THING */
	AA_ASKOBJECT	= 0x0008,
	AA_OPENWIND	= 0x0010,	/*  b4: MAGXDESK, THING */
	AA_STARTPROG	= 0x0020,	/*  b5: MAGXDESK, THING */
	AA_ACCWIND	= 0x0040,	/*  b6:           THING */
	AA_STATUS	= 0x0080,	/*  b7:           THING */
	AA_COPY_DRAGGED	= 0x0100,	/*  b8:           THING */
	AA_DRAG_ON_WINDOW=0x0200,	/*  b9: MAGXDESK, THING */
	AA_EXIT		= 0x0400,	/* b10: MAGXDESK, THING */
	AA_XWIND	= 0x0800,	/* b11: MAGXDESK, THING */
	AA_FONTCHANGED	= 0x1000,	/* b2:            THING */
	AA_STARTED	= 0x2000,	/* b13: MAGXDESK, THING */
	AA_SRV_QUOTING	= 0x4000,	/* b14:           THING */
	AA_FILE		= 0x8000,	/* b15:           THING */
	/* mp[4]: */
	AA_COPY		= 0x0001,
	AA_DELETE	= 0x0002,
	AA_VIEW		= 0x0004,
	AA_SETWINDPOS	= 0x0008
};

/* macros for quotation test */
#define VA_ACC_QUOTING(a)    ((a) & VV_ACC_QUOTING)
#define VA_SERVER_QUOTING(a) ((a) & AA_SRV_QUOTING)

#endif /* _vaproto_h */
