/* Keyboard handling stuff
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

/* PROGRAMS WHICH HAVE TO BE TESTED WHETHER THERE IS COMPATIBILITY PROBLEM:
 *
 * a) Apex Media
 * b) JML Snap
 * c) Midnight Commander (!)
 * d) something else I forgot
 *
 */

# include "arch/intr.h"		/* click */
# include "mint/signal.h"	/* SIGQUIT */
# include "mint/mint.h"		/* FATAL() */

# include "libkern/libkern.h"	/* strcpy(), strcat(), ksprintf() */

# include "bios.h"		/* kbshft, kintr, *keyrec, ...  */
# include "biosfs.h"		/* struct tty */
# include "cnf.h"		/* init_env */
# include "console.h"		/* c_conws() */
# include "cookie.h"		/* get_cookie(), set_cookie() */
# include "debug.h"		/* do_func_key() */
# include "dev-mouse.h"		/* mshift */
# include "dos.h"		/* s_hutdown() */
# include "dossig.h"		/* p_kill() */
# include "init.h"		/* boot_printf(), *sysdir */
# include "info.h"		/* messages */
# include "k_exec.h"		/* sys_pexec() */
# include "k_fds.h"		/* fp_alloc() */
# include "keyboard.h"		/* struct cad */
# include "kmemory.h"		/* kmalloc() */
# include "proc.h"		/* rootproc */
# include "random.h"		/* add_keyboard_randomness() */
# include "signal.h"		/* killgroup() */
# include "timeout.h"		/* addroottimeout() */

# include <osbind.h>		/* Keytbl() */

# define CAD_TIMEOUT	5*200
# define ROOT_TIMEOUT	1

/* The _AKP cookie value consists of:
 *
 * bits 0-7	keyboard nationality
 * bits 8-15	desktop nationality
 *
 * Actually hardware.txt by Dan Hollis says the opposite, but lies.
 */
/* _AKP codes for the keyboard (the low byte of the low word)
 * are as follows:
 *
 * 127 = all nationalities supported (?)
 *
 *  0 = USA          8 = Ger.Suisse    16 = Hungary    24 = Romania
 *  1 = Germany      9 = Turkey        17 = Poland     25 = Bulgaria
 *  2 = France      10 = Finnland      18 = Lituania   26 = Slovenia
 *  3 = England     11 = Norway        19 = Latvia     27 = Croatia
 *  4 = Spain       12 = Danmark       20 = Estonia    28 = Serbia
 *  5 = Italy       13 = S. Arabia     21 = Bialorus   29 = Montenegro
 *  6 = Sweden      14 = Netherlands   22 = Ukraina    30 = Macedonia
 *  7 = Fr.Suisse   15 = Czech         23 = Slovakia   31 = Greece
 *
 * 32 = Russia      40 = Vietnam       48 = Bangladesh
 * 33 = Israel      41 = India
 * 34 = Sou. Africa 42 = Iran
 * 35 = Portugal    43 = Mongolia
 * 36 = Belgium     44 = Nepal
 * 37 = Japan       45 = Laos
 * 38 = China       46 = Kambodja
 * 39 = Korea       47 = Indonesia
 *
 * The rest of codes are reserved for future extensions. Add ones,
 * if you find a missing one. Consider there are various countries
 * which all speak the same language, like all the South America
 * speaks Spanish or Portughese, the North America English or French
 * etc.
 *
 */

/* Keyboard definition tables (ripped off TOS 4.04, with fixes) */

/* USA, _AKP code 0 */

/* Unshifted */

static const uchar usa_kbd[] =
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
	0x00
};

/* German, (1) */

/* Unshifted */

static const uchar german_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'û' ,'\'',0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'z' ,'u' ,'i' ,
	'o' ,'p' ,'Å' ,'+' ,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,'î' ,
	'Ñ' ,'#' ,0x00,'~' ,'y' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'!' ,'"' ,0xdd,'$' ,'%' ,'&' ,
	'/' ,'(' ,')' ,'=' ,'?' ,'`' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,'ö' ,'*' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,'ô' ,
	'é' ,'^' ,0x00,'|' ,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,';' ,':' ,'_' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'û' ,'\'',0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,'ö' ,'+' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,'ô' ,
	'é' ,'#' ,0x00,'~' ,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x00,

/* Alternate shifted */

	0x1a,'\\',0x27,'{' ,0x28,'}' ,0x00,

/* Alternate Caps */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x00
};

/* French (2) */

/* Unshifted */

static const uchar french_kbd[] =
{
	0x00,0x1b,'&' ,'Ç' ,'"' ,'\'','(' ,'›' ,
	'ä' ,'!' ,'á' ,'Ö' ,')' ,'-' ,0x08,0x09,
	'a' ,'z' ,'e' ,'r' ,'t' ,'y' ,'u' ,'i' ,
	'o' ,'p' ,'^' ,'$' ,0x0d,0x00,'q' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,'m' ,
	'ó','`' ,0x00,'#' ,'w' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,',' ,';' ,':' ,'=' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,0xf8,'_' ,0x08,0x09,
	'A' ,'Z' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,0xb9,'*' ,0x0d,0x00,'Q' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,'M' ,
	'%' ,0x9c,0x00,'|' ,'W' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'?' ,'.' ,'/' ,'+' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'&' ,0x90,'"' ,'\'','(' ,0xdd,
	0x8a,'!' ,0x80,0xb6,')' ,'-' ,0x08,0x09,
	'A' ,'Z' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,'^' ,'$' ,0x0d,0x00,'Q' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,'M' ,
	0x97,'`' ,0x00,'#' ,'W' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,',' ,';' ,':' ,'=' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'[' ,0x1b,']' ,0x28,'\\',0x2b,'@' ,0x00,

/* Alternate shifted */

	0x1a,'{' ,0x1b,'}' ,0x28,'\\' ,0x00,

/* Alternate Caps */

	0x1a,'[' ,0x1b,']' ,0x28,'\\',0x2b,'@' ,0x00
};

/* British (3) */

/* Unshifted */

static const uchar british_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'y' ,'u' ,'i' ,
	'o' ,'p' ,'[' ,']' ,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,';' ,
	'\'','`' ,0x00,'#' ,'z' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'/' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'\\',0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'!' ,'"' ,0x9c,'$' ,'%' ,'^' ,
	'&' ,'*' ,'(' ,')' ,'_' ,'+' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,'{' ,'}' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,':' ,
	'@' ,0xff,0x00,'~' ,'Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,'<' ,'>' ,'?' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'|' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,'[' ,']' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,';' ,
	'\'','`' ,0x00,'#' ,'Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'/' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'\\',0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */
	0x00,

/* Alternate shifted */
	0x00,

/* Alternate Caps */
	0x00
};

/* Spanish (4) */

/* Unshifted */

static const uchar spanish_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'z' ,'u' ,'i' ,
	'o' ,'p' ,'\'','`' ,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,0xa4,
	';' ,0x87,0x00,'\\','y' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,0xf8,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,0xad,0xa8,0x9c,'$' ,'%' ,'/' ,
	'&' ,'*' ,'(' ,')' ,'_' ,'+' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,'"' ,'^' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0xa5,
	':' ,'~' ,0x00,'|' ,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,'?' ,'!' ,0xdd,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'-' ,'=' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,'\'','`' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0xa5,
	';' ,0x87,0x00,'\\','Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,0xf8,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'[' ,0x1b,']' ,0x2b,'#' ,0x28,'Å' ,
	0x00,

/* Alternate shifted */

	0x1a,'{' ,0x1b,'}' ,0x2b,'@' ,0x00,

/* Alternate Caps */

	0x1a,'[' ,0x1b,']' ,0x2b,'#' ,0x28,'Å' ,
	0x00
};

/* Italian (5) */

/* Unshifted */

static const uchar italian_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'',0x8d,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'y' ,'u' ,'i' ,
	'o' ,'p' ,0x8a,'+' ,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,0x95,
	0x85,0x97,0x00,'\\','z' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'!' ,'"' ,0x9c,'$' ,'%' ,'&' ,
	'/' ,'(' ,')' ,'=' ,'?' ,'^' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,0x82,'*' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,'@' ,
	'#' ,0xdd,0x00,'|' ,'Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,';' ,':' ,'_' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'',0x8d,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Y' ,'U' ,'I' ,
	'O' ,'P' ,0x8a,'+' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0x95,
	0x85,0x97,0x00,'\\','Z' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'[' ,0x1b,']' ,0x2b,0xf8,0x60,'`' ,0x00,

/* Alternate shifted */

	0x1a,'{' ,0x1b,'}' ,0x2b,'~' ,0x60,'`' ,0x00,

/* Alternate Caps */

	0x1a,'[' ,0x1b,']' ,0x2b,0xf8,0x60,'`' ,0x00
};

/* Swiss French (7) */

/* Unshifted */

static const uchar sw_french_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'','^' ,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'z' ,'u' ,'i' ,
	'o' ,'p' ,0x8a,0xb9,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,0x82,
	0x85,0xdd,0x00,'$' ,'y' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'+' ,'"' ,'*' ,0x87,'%' ,'&' ,
	'/' ,'(' ,')' ,'=' ,'?' ,'`' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,0x81,'!' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0x94,
	0x84,0xf8,0x00,0x9c,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,';' ,':' ,'_' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'','^' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,0x8a,0xb9,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0x82,
	0x85,0xdd,0x00,'$' ,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x1b,'#' ,
	0x2b,'~' ,0x00,

/* Alternate shifted */

	0x1a,'\\',0x27,'{' ,0x28,'}' ,0x1b,'#' ,
	0x2b,'|' ,0x00,

/* Alternate Caps */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x1b,'#' ,
	0x2b,'~' ,0x00
};

/* Swiss German (8) */

/* Unshifted */

static const uchar sw_german_kbd[] =
{
	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'','^' ,0x08,0x09,
	'q' ,'w' ,'e' ,'r' ,'t' ,'z' ,'u' ,'i' ,
	'o' ,'p' ,0x81,0xb9,0x0d,0x00,'a' ,'s' ,
	'd' ,'f' ,'g' ,'h' ,'j' ,'k' ,'l' ,0x94,
	0x84,0xdd,0x00,'$' ,'y' ,'x' ,'c' ,'v' ,
	'b' ,'n' ,'m' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Shifted */

	0x00,0x1b,'+' ,'"' ,'*' ,0x87,'%' ,'&' ,
	'/' ,'(' ,')' ,'=' ,'?' ,'`' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,0x8a,'!' ,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0x82,
	0x85,0xf8,0x00,0x9c,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,';' ,':' ,'_' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,'7' ,
	'8' ,0x00,'-' ,'4' ,0x00,'6' ,'+' ,0x00,
	'2' ,0x00,'0' ,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'>' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Caps */

	0x00,0x1b,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,
	'7' ,'8' ,'9' ,'0' ,'\'','^' ,0x08,0x09,
	'Q' ,'W' ,'E' ,'R' ,'T' ,'Z' ,'U' ,'I' ,
	'O' ,'P' ,'ö' ,0xb9,0x0d,0x00,'A' ,'S' ,
	'D' ,'F' ,'G' ,'H' ,'J' ,'K' ,'L' ,0x99,
	0x8e,0xdd,0x00,'$' ,'Y' ,'X' ,'C' ,'V' ,
	'B' ,'N' ,'M' ,',' ,'.' ,'-' ,0x00,0x00,
	0x00,' ' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,'-' ,0x00,0x00,0x00,'+' ,0x00,
	0x00,0x00,0x00,0x7f,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'<' ,0x00,0x00,'(' ,')' ,'/' ,'*' ,'7' ,
	'8' ,'9' ,'4' ,'5' ,'6' ,'1' ,'2' ,'3' ,
	'0' ,'.' ,0x0d,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* Alternate */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x1b,'#' ,
	0x2b,'~' ,0x00,

/* Alternate shifted */

	0x1a,'\\',0x27,'{' ,0x28,'}' ,0x1b,'#' ,
	0x2b,'|' ,0x00,

/* Alternate Caps */

	0x1a,'@' ,0x27,'[' ,0x28,']' ,0x1b,'#' ,
	0x2b,'~' ,0x00
};

/* Keyboard interrupt routine.
 *
 * TOS goes through here with the freshly baked and still
 * warm key scancode in hands. If there are any keys or
 * key combinations we want to handle (e.g. Ctrl/Alt/Del)
 * then we intercept it now.
 *
 * At the end, you need to return the original scancode for
 * TOS to process it. If you change the value, TOS will buy
 * this as well. If you want to omit the TOS routines at all,
 * return -1 (see code in intr.spp).
 *
 * Developing this code will probably allow us to get rid of
 * checkkeys() in bios.c, have processes waiting for keyboard
 * awaken immediately, load own scancode->ASCII keyboard tables
 * and do other such nifty things (draco).
 *
 */

static const ushort modifiers[] =
{
	CONTROL, RSHIFT, LSHIFT,
	ALTERNATE, CLRHOME, INSERT, 0
};

static const ushort mmasks[] =
{
	MM_CTRL, MM_RSHIFT, MM_LSHIFT,
	MM_ALTERNATE, MM_CLRHOME, MM_INSERT
};

struct	cad_def cad[3];	/* for halt, warm and cold resp. */
short	gl_kbd;		/* keyboard layout, set by getmch() in init.c */
static	short cad_lock;	/* semaphore to avoid scheduling shutdown() twice */
static	short kbd_lock;	/* semaphore to temporarily block the keyboard processing */
static	long hz_ticks;	/* place for saving the hz_200 timer value */

static	uchar numin[3];	/* buffer for storing ASCII code typed in via numpad */
static	ushort numidx;	/* index for the buffer (0 = empty, 3 = full) */

/* keyboard table pointers indexed by AKP */
struct keytab keyboards[MAXAKP+1];

static uchar *current_keytab;	/* keyboard table being in use */

/* Routine called after the user hit Ctrl/Alt/Del
 */
static void
ctrl_alt_del(PROC *p, long arg)
{
	switch (cad[arg].action)
	{
		/* 1 is to signal a pid */
		case 1:
			if (p_kill(cad[arg].par.pid, cad[arg].aux.arg) < 0)
				s_hutdown(arg);
			break;

		/* 2 shall be to exec a program
		 * with a path pointed to by par.path
		 */
		case 2:
			if (sys_pexec (100, cad[arg].par.path, cad[arg].aux.cmd, cad[arg].env) < 0)
				s_hutdown (arg);
			break;

		/* 0 is default */
		default:
			s_hutdown (arg);
			break;
	}
}

/* synchronous Ctrl/Alt/F? callback
 * for the debug facilities
 */
static void
ctrl_alt_Fxx (PROC *p, long arg)
{
	do_func_key (arg);
}

/* Similar callback for the Alt/Help.
 *
 * To avoid problems with the actual current directory,
 * the program gets its path as the command line.
 * Or shall we explicitly force it to that?
 *
 * Also a problem here:
 * since the program is executed directly by the kernel,
 * it can at most get the same environment as init has.
 *
 */
static void
alt_help(void)
{
	char pname[32], cmdln[32];

	strcpy(pname, "a:");
	strcpy(cmdln, " a:");		/* first byte is len */
	pname[0] += *(short *)0x0446L;	/* boot device */
	cmdln[1] += *(short *)0x0446L;
	strcat(pname, sysdir);
	strcat(cmdln, sysdir);
	strcat(pname, "althelp.sys");

	cmdln[0] = (char)(strlen(cmdln) - 1);

	sys_pexec(100, pname, cmdln, init_env);
}

/* The handler
 */
short
ikbd_scan (ushort scancode)
{
	ushort mod = 0, clk = 0, shift = *kbshft, x = 0;
	uchar *chartable;

	/* This is set during various keyboard table initializations
	 * e.g. when the user calls Bioskeys(), to prevent processing
	 * go according to incomplete keyboard translation tables.
	 */
	if (kbd_lock)
		return -1;

	scancode &= 0x00ff;		/* better safe than sorry */

# ifdef DEV_RANDOM
	add_keyboard_randomness ((ushort)((scancode << 8) | shift));
# endif

	/* We handle modifiers first
	 */
	while(modifiers[x])
	{
		if (scancode == modifiers[x])
		{
			shift |= mmasks[x];
			mod++;
			break;
		}
		else if (scancode == (modifiers[x] | 0x80))
		{
			shift &= ~mmasks[x];
			mod++;
			break;
		}
		x++;
	}

	switch (scancode)
	{
		/* Caps toggles its bit, when hit, it also makes keyclick */
		case	CAPS:
		{
			shift ^= MM_CAPS;
			mod++;
			clk++;
			break;
		}
		/* Releasing Alternate should generate a character, whose ASCII
		 * code was typed in via the numpad
		 */
		case	ALTERNATE+0x80:
		{
			if (numidx)
			{
				ushort ascii = 0, tempidx = 0;

				while(numidx--)
				{
					ascii += (numin[tempidx] & 0x0f);
					if (numidx)
					{
						tempidx++;
						ascii *= 10;
					}
				}

				/* Only the values 0-255 are valid. Silently
				 * ignore the elder byte
				 */
				ascii &= 0x00ff;

				/* Reset the buffer for next use */
				numidx = 0;

				/* Insert the character to the keyboard buffer.
				 * XXX: is this correct?
				 */
				keyrec->tail += 4;
				if (keyrec->tail >= keyrec->buflen)
					keyrec->tail = 0;
				(keyrec->bufaddr + keyrec->tail)[0] = 0;
				(keyrec->bufaddr + keyrec->tail)[1] = 0;
				(keyrec->bufaddr + keyrec->tail)[2] = 0;
				(keyrec->bufaddr + keyrec->tail)[3] = (char)ascii;
				kintr = 1;
			}
			break;
		}
	}

	if (mod)
	{
		ushort sc = scancode;

		*kbshft = mshift = (char)shift;
		if (clk)
			kbdclick(sc);
		sc &= 0x7f;
		/* this catches i.a. release code of CapsLock */
		if ((sc != CLRHOME) && (sc != INSERT))
			return -1;
	}

	/* Here we handle keys of `system wide' meaning. These are:
	 *
	 * Ctrl/Alt/Del		-> warm start
	 * Ctrl/Alt/RShift/Del	-> cold start
	 * Ctrl/Alt/LShift/Del	-> halt
	 * Ctrl/Alt/Undo	-> SIGQUIT to the group
	 * Ctrl/Alt/Fx		-> debug information
	 * Ctrl/Alt/Shift/Fx	-> debug information
	 *
	 */
	if ((shift & MM_CTRLALT) == MM_CTRLALT)
	{
		if (scancode == DEL)
		{
			if (!cad_lock)
			{
				TIMEOUT *t;

				t = addroottimeout (ROOT_TIMEOUT, (void _cdecl (*)(PROC *))ctrl_alt_del, 1);
				if (t)
				{
					t->arg = cad_lock = 1;

					if ((shift & MM_ESHIFT) == MM_RSHIFT)
						t->arg = 2;
					else if ((shift & MM_ESHIFT) == MM_LSHIFT)
						t->arg = 0;

					hz_ticks = *(long *)0x04baL;
				}
			}
			else
			{
				long mora;

				mora = *(long *)0x04baL - hz_ticks;
				if (mora > CAD_TIMEOUT)
					return scancode;
			}

			return -1;
		}
		else if (scancode == UNDO)
		{
			killgroup (con_tty.pgrp, SIGQUIT, 1);

			goto key_done;
		}
		else if ((scancode >= 0x003b) && (scancode <= 0x0044))
		{
			TIMEOUT *t;

			if (shift & MM_ESHIFT)
				scancode += 0x0019;	/* emulate F11-F20 */

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			goto key_done;
		}
		/* This is in case the keyboard has real F11-F20 keys on it */
		else if ((scancode >= 0x0054) && (scancode <= 0x005d))
		{
			TIMEOUT *t;

			t = addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *)) ctrl_alt_Fxx, 1);
			if (t) t->arg = scancode;

			goto key_done;
		}
		/* We ignore release codes, but catch them to avoid
		 * spurious execution of checkkeys() on every release of a key.
		 */
		else if ((scancode == DEL+0x80) || (scancode == UNDO+0x80))
			return -1;
		else if ((scancode >= 0x003b+0x80) && (scancode <= 0x0044+0x80))
			return -1;
		else if ((scancode >= 0x0054+0x80) && (scancode <= 0x005d+0x80))
			return -1;
	}

	if ((shift & MM_ALTERNATE) == MM_ALTERNATE)
	{
		switch (scancode)
		{
			uchar ascii;

			/* Alt/Help fires up a program named `althelp.sys'
			 * located in the system directory (e.g. `c:\multitos\')
			 */
			case HELP:
			{
				addroottimeout(ROOT_TIMEOUT, (void _cdecl (*)(PROC *))alt_help, 1);
				goto key_done;
				break;
			}
			/* Alt/Numpad generates ASCII codes like in TOS 2.0x.
			 */
			case NUMPAD_0:
			case NUMPAD_1:
			case NUMPAD_2:
			case NUMPAD_3:
			case NUMPAD_4:
			case NUMPAD_5:
			case NUMPAD_6:
			case NUMPAD_7:
			case NUMPAD_8:
			case NUMPAD_9:
			{
				if (numidx > 2)		/* buffer full? reset it */
					numidx = 0;

				chartable = keyboards[gl_kbd].unshift;
				ascii = chartable[scancode];
				if (ascii)
				{
					numin[numidx] = ascii;
					numidx++;
				}
				goto key_done;
				break;
			}
			/* Ignore release codes as usual.
			 */
			case HELP+0x80:
			case NUMPAD_0+0x80:
			case NUMPAD_1+0x80:
			case NUMPAD_2+0x80:
			case NUMPAD_3+0x80:
			case NUMPAD_4+0x80:
			case NUMPAD_5+0x80:
			case NUMPAD_6+0x80:
			case NUMPAD_7+0x80:
			case NUMPAD_8+0x80:
			case NUMPAD_9+0x80:
			{
				return -1;
				break;
			}
		}
	}

	/* Ordinary keyboard add here ...
	 */

	if ((scancode & 0x80) == 0)
		kintr = 1;

	return scancode;		/* for now, give the scancode away to TOS */

key_done:
	kbdclick(scancode);		/* produce keyclick */
	return -1;			/* don't go to TOS, just return */
}

/* Initialization routines
 */

static short
fill_keystruct(short index, uchar *table)
{
	uchar *tmp = table + 384;
	uchar *unshift, *shift, *caps, *alt, *altshift, *altcaps;
	short sanity = 1;

	unshift = table;
	shift = table + 128;
	caps = table + 256;
	alt = tmp;

	while((sanity != 0) && (*tmp != 0))
	{
		sanity++;
		tmp++;
	}
	if (!sanity) return 0;
	sanity = 1;
	while((sanity != 0) && (*tmp == 0))
	{
		sanity++;
		tmp++;
	}
	if (!sanity) return 0;
	altshift = tmp;

	sanity = 1;
	while((sanity != 0) && (*tmp != 0))
	{
		sanity++;
		tmp++;
	}
	if (!sanity) return 0;
	sanity = 1;
	while((sanity != 0) && (*tmp == 0))
	{
		sanity++;
		tmp++;
	}
	if (!sanity) return 0;
	altcaps = tmp;

	keyboards[index].unshift = unshift;
	keyboards[index].shift = shift;
	keyboards[index].caps = caps;
	keyboards[index].alt = alt;
	keyboards[index].altshift = altshift;
	keyboards[index].altcaps = altcaps;

	return 1;	/* OK */
}

static short
load_table(FILEPTR *fp, char *name, long size)
{
	char *kbuf;
	short ret = 0;

	/* This is 128+128+128 for unshifted, shifted and caps
	 * tables respectively; plus 3 bytes for three alt ones,
	 * plus two bytes magic header, gives 389 bytes minimum.
	 */
	if (size < 389L) return 0;

	kbuf = kmalloc(size);
	if (!kbuf) return 0;

	if ((*fp->dev->read)(fp, kbuf, size) == size)
	{
		switch(*(ushort *)kbuf)
		{
			case	0x2771:		/* magic word for std format */
			{
				ret = fill_keystruct(gl_kbd, (uchar *)kbuf + sizeof(short));
				break;
			}
			case	0x2772:		/* magic word for ext format */
			{
				/* The extended format is identical as the old one
				 * with exception that the second word of the data
				 * contains the AKP code for the keyboard table
				 * loaded.
				 */
				ushort *sbuf = (ushort *)kbuf;

				if (sbuf[1] <= MAXAKP)
				{
					ret = fill_keystruct(sbuf[1], (uchar *)kbuf + sizeof(long));
					if (ret)
						gl_kbd = sbuf[1];
				}
				break;
			}
		}
	}

	if (!ret)
	{
		c_conws(MSG_keytable_faulty);
		kfree(kbuf);
		return 0;
	}

	/* Success */
	{
		char msg[64];

		ksprintf(msg, sizeof(msg), MSG_keytable_loaded, gl_kbd);
		c_conws(msg);
	}

	return 1;
}

/* The XBIOS' Bioskeys() function
 */
void
bioskeys(void)
{
	short x;
	uchar *src, *dst;
	long unshift, shift, caps, alt, altshift, altcaps;
	struct keytab *syskeytab;

	kbd_lock = 1;

	/* Copy the basic keycode definitions (unshifted, shifted
	 * and caps) to the current buffer
	 */
	src = keyboards[gl_kbd].unshift;
	dst = current_keytab;

	for (x = 0; x < 384; x++)
		*dst++ = *src++;

	/* Now copy the definitions with Alternate */
	src = keyboards[gl_kbd].alt;
	do
	{
		*dst++ = *src;
	} while (*src++);

	src = keyboards[gl_kbd].altshift;
	do
	{
		*dst++ = *src;
	} while (*src++);

	src = keyboards[gl_kbd].altcaps;
	do
	{
		*dst++ = *src;
	} while (*src++);

	/* Calculate the addresses */
	unshift = (long)keyboards[gl_kbd].unshift;

	shift = (long)keyboards[gl_kbd].shift;
	shift -= unshift;
	caps = (long)keyboards[gl_kbd].caps;
	caps -= unshift;
	alt = (long)keyboards[gl_kbd].alt;
	alt -= unshift;
	altshift = (long)keyboards[gl_kbd].altshift;
	altshift -= unshift;
	altcaps = (long)keyboards[gl_kbd].altcaps;
	altcaps -= unshift;

	unshift = (long)current_keytab;

	shift += unshift;
	caps += unshift;
	alt += unshift;
	altshift += unshift;
	altcaps += unshift;

	/* Now these hold the real addresses inside the table */

	syskeytab = Keytbl(-1, -1, -1);	/* call the underlying XBIOS */

	syskeytab->unshift = (uchar *)unshift;
	syskeytab->shift = (uchar *)shift;
	syskeytab->caps = (uchar *)caps;
	syskeytab->alt = (uchar *)alt;
	syskeytab->altshift = (uchar *)altshift;
	syskeytab->altcaps = (uchar *)altcaps;

	/* Done! */
	kbd_lock = 0;
}

/* Initialize the built-in keyboard tables.
 * This must be done before init_intr()!
 */
void
init_keybd(void)
{
	short x;

	kbd_lock = 1;

	/* The USA keyboard is default */
	for (x = 0; x < 127; x++)
		fill_keystruct(x, (uchar *)usa_kbd);

	fill_keystruct(1, (uchar *)german_kbd);
	fill_keystruct(2, (uchar *)french_kbd);
	fill_keystruct(3, (uchar *)british_kbd);
	fill_keystruct(4, (uchar *)spanish_kbd);
	fill_keystruct(5, (uchar *)italian_kbd);
	fill_keystruct(6, (uchar *)british_kbd);
	fill_keystruct(7, (uchar *)sw_french_kbd);
	fill_keystruct(8, (uchar *)sw_german_kbd);

	kbd_lock = 0;
}

void
load_keytbl(void)
{
	XATTR xattr;
	FILEPTR *fp;
	long ret, table_len = 387L, akp_val = 0L;
	char name[32];		/* satis uidetur */

	ret = FP_ALLOC(rootproc, &fp);
	if (ret) return;

	/* `keybd.tbl' is already used by gem.sys, we can't conflict
	 */
	strcpy(name, sysdir);
	strcat(name, "keyboard.tbl");

	if (!do_open(&fp, name, O_RDONLY, 0, &xattr))
	{
		ret = load_table(fp, name, xattr.size);
		do_close(rootproc, fp);
	}
	else
	{
		fp->links = 0;		/* suppress complaints */
		FP_FREE(fp);
	}

	/* Now fix the _AKP code in the Cookie Jar, if it changes */
	if (ret)
	{
		get_cookie(COOKIE__AKP, &akp_val);
		akp_val &= 0xffffff00L;
		akp_val |= gl_kbd;
		set_cookie(COOKIE__AKP, akp_val);
	}

	/* Alloc the buffer for the `current' keyboard table */
	table_len += strlen(keyboards[gl_kbd].alt);
	table_len += strlen(keyboards[gl_kbd].altshift);
	table_len += strlen(keyboards[gl_kbd].altcaps);

	current_keytab = kmalloc(table_len);

	if (!current_keytab)
		FATAL(__FILE__ ": out of RAM in " __FUNCTION__);

	/* Install the tables in the system */
	bioskeys();
}

/* EOF */
