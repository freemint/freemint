/*
 * Keyboard definition tables (ripped off TOS 4.04, with fixes)
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 *
 */

/* The default and only keyboard table is the AKP 0,
 * for other types you must provide an external one,
 * see the code in keyboard.c
 */

/* You can define other keyboards in this module, only
 * include them in appropriate ifdefs, e.g. for Spanish
 * keyboard:
 *
 * # ifdef KBD_SPAIN
 * ...
 * # endif
 *
 * and define everything inside, i.e. the keyboard, the sys_keytable struct
 * and default_akp. Then if you remove -DKBD_USA from KERNELDEFS, and do
 * -DKBD_SPAIN, kernel with spanish keyboard table will be compiled.
 */
 
# ifdef KBD_USA

/* USA, _AKP code 0 */

/* Unshifted */

static uchar usa_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'y' ,'u' ,'i' ,
	'o' ,'p' ,'[' ,']' ,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,';' ,
	'\'','`' ,0x00,'\\','z' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'/' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'!' ,'@' ,'#' ,'$' ,'%' ,'^' ,
	'&' ,'*' ,'(' ,')' ,'_' ,'+' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,'{' ,'}' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,':' ,
	'"' ,'~' ,0x00,'|' ,'Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,'<' ,'>' ,'?' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,'[' ,']' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,';' ,
	'\'','`' ,0x00,'\\','Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'/' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */
	0x00,

/* Alternate shifted */
	0x00,

/* Alternate Caps */
	0x00,

/* Alternate Gr */
	0x00,

/* Deadkeys */
	0x00
};

static const short default_akp = 0;

static struct keytab sys_keytab =
{
	(uchar *)usa_kbd,
	(uchar *)usa_kbd + 128,
	(uchar *)usa_kbd + 256,
	(uchar *)usa_kbd + 384,
	(uchar *)usa_kbd + 385,
	(uchar *)usa_kbd + 386,
	(uchar *)usa_kbd + 387,
	(uchar *)usa_kbd + 388
};

# endif

/* EOF */
