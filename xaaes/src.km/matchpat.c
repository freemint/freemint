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

/* HR: isolated completely from the XaAES & dirs environment.
 * 
 * Pattern matching
 * - If you want better filtering of files, put the code here...
 * 
 * Valid patterns are:
 * 	?      Any single char
 * 	*      A string of any char
 * 	!X     Any char except for X
 * 	[abcd] One of (any one of a,b,c or d)
 * Examples:
 * 	*      All files in dir
 * 	a*     All files begining with 'a'
 * 	*.c*   All files with a extension starting with c
 * 	*.o    All '.o' files
 * 	*.!o   All files not ending in '.o' 
 * 	!z*.?  All files not starting with 'z', and having a single character extension
 * 	*.[co] All '.o' and '.c' files
 * 	*.[ch]* All files with a extension starting with c or h
 */

#include "global.h"
#include "matchpat.h"


int
match_pattern(char *t, char *pat)
{
	int valid = 1;

	while (valid && ((*t && *pat) || (!*t && *pat == '*')))
	{
		switch (*pat)
		{
			/* Any character */
			case '?':
			{
				t++;
				pat++;
				break;
			}
			/* String of any character */
			case '*':
			{
				pat++;
				while (*t && (toupper(*t) != toupper(*pat)))
					t++;
				break;
			}
			/* !X means any character but X */
			case '!':
			{
				if (toupper(*t) != toupper(pat[1]))
				{
					t++;
					pat += 2;
				}
				else
					valid = 0;
				break;
			}
			/* [<chars>] means any one of <chars> */
			case '[':
			{
				while ((*(++pat) != ']') && (toupper(*t) != toupper(*pat)))
					;
				if (*pat == ']')
					valid = 0;
				else
					while (*++pat != ']')
						;
				pat++;
				t++;
				break;
			}
			default:
			{
				if (toupper(*t++) != toupper(*pat++))
					valid = 0;
				break;
			}
		}
	}

	return (valid && (toupper(*t) == toupper(*pat)));
}
