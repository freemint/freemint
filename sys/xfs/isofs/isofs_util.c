/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 */

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "isofs_global.h"

#include "isofs_util.h"

/*
 * Get one character out of an iso filename
 * Return number of bytes consumed
 */
int
isochar(const uchar *isofn, const uchar *isoend, int joliet_level, uchar *c)
{
	*c = *isofn++;

	if (joliet_level == 0 || isofn == isoend)
		/* (00) and (01) are one byte in Joliet, too */
		return 1;

	/* No Unicode support yet :-( */
	switch (*c)
	{
		default:
			*c = '?';
			break;
		case '\0':
			*c = *isofn;
			break;
	}

	return 2;
}

/*
 * translate and compare a filename
 * Note: Version number plus ';' may be omitted.
 */
int
isofncmp(const uchar *fn, int fnlen, const uchar *isofn, int isolen, int joliet_level)
{
	const uchar *isoend = isofn + isolen;
	int i, j;
	char c;

	while (--fnlen >= 0)
	{
		if (isofn == isoend)
			return *fn;

		isofn += isochar(isofn, isoend, joliet_level, &c);
		if (c == ';')
		{
			switch (*fn++)
			{
				default:
					return *--fn;
				case 0:
					return 0;
				case ';':
					break;
			}

			for (i = 0; --fnlen >= 0; i = i * 10 + *fn++ - '0')
			{
				if (*fn < '0' || *fn > '9')
					return -1;
			}

			for (j = 0; isofn != isoend; j = j * 10 + c - '0')
				isofn += isochar(isofn, isoend, joliet_level, &c);

			return i - j;
		}

		if (((uchar) c) != *fn)
		{
			if (c >= 'A' && c <= 'Z')
			{
				if (c + ('a' - 'A') != *fn)
				{
					if (*fn >= 'a' && *fn <= 'z')
						return *fn - ('a' - 'A') - c;
					else
						return *fn - c;
				}
			}
			else
				return *fn - c;
		}
		fn++;
	}

	if (isofn != isoend)
	{
		isofn += isochar(isofn, isoend, joliet_level, &c);
		switch (c)
		{
			default:
				return -1;

			case '.':
				if (isofn != isoend)
				{
					isochar(isofn, isoend, joliet_level, &c);
					if (c == ';')
						return 0;
				}
				return -1;

			case ';':
				return 0;
		}
	}

	return 0;
}

/*
 * translate a filename
 */
void
isofntrans(uchar *infn, int infnlen, uchar *outfn, ushort *outfnlen,
	   int original, int casetrans, int assoc, int joliet_level)
{
	uchar *infnend = infn + infnlen;
	int fnidx = 0;
	
	if (assoc)
	{
		*outfn++ = ASSOCCHAR;
		fnidx++;
	}

	for (; infn != infnend; fnidx++)
	{
		char c;

		infn += isochar(infn, infnend, joliet_level, &c);

		if (casetrans && joliet_level == 0 && c >= 'A' && c <= 'Z')
		{
			*outfn++ = c + ('a' - 'A');
		}
		else if (!original && c == ';')
		{
			if (fnidx > 0 && outfn[-1] == '.')
				fnidx--;

			break;
		}
		else
			*outfn++ = c;
	}

	*outfnlen = fnidx;
}
