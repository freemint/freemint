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

#ifndef _key_map_h
#define _key_map_h

#ifndef K_SHIFT
#define K_SHIFT (K_LSHIFT | K_RSHIFT)
#endif

#define	KbSCAN		0x8000
#define	KbNUM		0x4000
#define	KbALT		(K_ALT << 8)
#define	KbCONTROL	(K_CTRL << 8)
#define	KbSHIFT		(K_SHIFT << 8)
#define	KbLSHIFT	(K_LSHIFT << 8)
#define	KbRSHIFT	(K_RSHIFT << 8)
#define	KbNORMAL	0x0000

#define	KbISO		0x37

#define	KbN1		0x02

#define	KbF1		0x3b
#define	KbF2		0x3c
#define	KbF3		0x3d
#define	KbF4		0x3e
#define	KbF5		0x3f
#define	KbF6		0x40
#define	KbF7		0x41
#define	KbF8		0x42
#define	KbF9		0x43
#define	KbF10		0x44
#define	KbF11		0x54
#define	KbF12		0x55
#define	KbF13		0x56
#define	KbF14		0x57
#define	KbF15		0x58
#define	KbF16		0x59
#define	KbF17		0x5a
#define	KbF18		0x5b
#define	KbF19		0x5c
#define	KbF20		0x5d

#define KbRETURN	0x1c
#define KbTAB		0x0f
#define	KbUNDO		0x61
#define	KbHELP		0x62
#define	KbINSERT	0x52
#define	KbHOME		0x47
#define	KbUP		0x48
#define	KbDOWN		0x50
#define	KbLEFT		0x4b
#define	KbRIGHT		0x4d
#define	KbCtrlLEFT	0x73
#define	KbCtrlRIGHT	0x74

#define	KbNumMINUS	0x4a	/* - (Ziffernblock) */
#define	KbNumPLUS	0x4e	/* + (Ziffernblock) */
#define	KbNumBrOPEN	0x63	/* ( (Ziffernblock */
#define	KbNumBrCLOSE	0x64	/* ) (Ziffernblock) */
#define	KbNumDIVIDE	0x65	/* / (Ziffernblock) */
#define	KbNumMULTIPLY	0x66	/* * (Ziffernblock) */
#define	KbNum7		0x67	/* 7 (Ziffernblock) */
#define	KbNum8		0x68	/* 8 (Ziffernblock) */
#define	KbNum9		0x69	/* 9 (Ziffernblock) */
#define	KbNum4		0x6a	/* 4 (Ziffernblock) */
#define	KbNum5		0x6b	/* 5 (Ziffernblock) */
#define	KbNum6		0x6c	/* 6 (Ziffernblock) */
#define	KbNum1		0x6d	/* 1 (Ziffernblock) */
#define	KbNum2		0x6e	/* 2 (Ziffernblock) */
#define	KbNum3		0x6f	/* 3 (Ziffernblock) */
#define	KbNum0		0x70	/* 0 (Ziffernblock) */
#define	KbNumPOINT	0x71	/* . (Ziffernblock) */
#define	KbNumENTER	0x72	/* Enter (Ziffernblock) */

#define	KbAlt1		0x78
#define	KbAlt2		0x79
#define	KbAlt3		0x7a
#define	KbAlt4		0x7b
#define	KbAlt5		0x7c
#define	KbAlt6		0x7d
#define	KbAlt7		0x7e
#define	KbAlt8		0x7f
#define	KbAlt9		0x80
#define	KbAlt0		0x81
#define	KbAltSS		0x82
#define	KbAltAPOSTR	0x83

#endif /* _key_map_h */
