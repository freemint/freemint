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

#include "xa_global.h"

#if INCLUDE_UNUSED
long
mvdi_device(long d1, long a0, short cmd, long *ret)
{
	long r_d0 = -1L, r_a0 = 0L;
	void (*dispatch)(void) = mvdi_api.dispatch;

	if (dispatch)
	{
		__asm__ volatile
		(
			"move.w	%4, d0\n\t"			\
			"swap	d0\n\t"				\
			"move.w #2,d0\n\t"			\
			"move.l %2,d1\n\t"			\
			"move.l %3,a0\n\t"			\
			"jsr	(%5)\n\t"			\
			"move.l d0,%0\n\t"			\
			"move.l a0,%1\n\t"			\
			: "=d"(r_d0),"=d"(r_a0)			\
			: "d"(d1), "d"(a0), "d"(cmd), "a"(dispatch)
			: "d0","d1","d2","a0","a1","a2","memory"
		);
		if (r_d0 >= 0)
		{
			*ret = r_a0;
		}
		else
			*ret = 0L;
	}

	if (r_d0 >= 0)
		*ret = r_a0;
	else
		*ret = 0L;

	return r_d0;
}
#endif

short
vcheckmode(short mode)
{
	short ret;

	__asm__ volatile
	(
		"move.w		%1,-(sp)\n\t"		\
		"move.w		#0x5f,-(sp)\n\t"	\
		"trap		#14\n\t"		\
		"addq.l		#4,sp\n\t"		\
		"move.w		d0,%0\n\t"		\
		: "=d"(ret)
		: "d"(mode)
		: "d0","d1","d2","a0","a1","a2","memory"
	);
	return ret;
}

#if INCLUDE_UNUSED
short
vsetmode(short mode)
{
	short ret;

	__asm__ volatile
	(
		"move.w		%1,-(sp)\n\t"		\
		"move.w		#0x58,-(sp)\n\t"	\
		"trap		#14\n\t"		\
		"addq.l		#4,sp\n\t"		\
		"move.w		d0,%0\n\t"		\
		: "=d"(ret)
		: "d"(mode)
		: "d0","d1","d2","a0","a1","a2","memory"
	);
	return ret;
}
#endif

#if INCLUDE_UNUSED
void
mSetscreen(unsigned long p1, unsigned long p2, short cmd)
{
	__asm__ volatile
	(
		"move.w #0x4d49,-(sp)\n\t"	\
		"move.w %2,-(sp)\n\t"		\
		"move.l %1,-(sp)\n\t"		\
		"move.l %0,-(sp)\n\t"		\
		"move.w #5,-(sp)\n\t"		\
		"trap #14\n\t"			\
		"add.l	#14,sp\n\t"		\
		:
		: "d"(p1), "d"(p2), "d"(cmd)
		: "d0","d1","d2","a0","a1","a2","memory"
	);
}
#endif
