/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * begin:	1998-07
 * last change: 1998-11-23
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _fstring_h
# define _fstring_h


/*
 * fast string functions
 * (for use in time critical parts)
 */

# define _STRLEN(s)		_mint_fstrlen		(s)
# define _STRCMP(s1, s2)	_mint_fstrcmp		(s1, s2)
# define _STRICMP(s1, s2)	_mint_fstricmp		(s1, s2)
# define _STRNCMP(s1, s2, l)	_mint_fstrncmp		(s1, s2, l)
# define _STRNICMP(s1, s2, l)	_mint_fstrnicmp		(s1, s2, l)
# define _STRCPY(d, s)		_mint_fstrcpy		(d, s)
# define _STRNCPY(d, s, l)	_mint_fstrncpy		(d, s, l)
# define _STRNCPY_F(d, s, l)	_mint_fstrncpy_f	(d, s, l)
# define _STRLWR(s)		_mint_fstrlwr		(s)
# define _STRUPR(s)		_mint_fstrupr		(s)


INLINE long
_mint_fstrlen (register const char *scan)
{
	register const char *start = scan + 1;
	
	while (*scan++) ;
	
	return ((long) (scan - start));
}

INLINE long
_mint_fstrcmp (register const char *str1, register const char *str2)
{
	register char c1, c2;
	
	do {
		c1 = *str1++;
		c2 = *str2++;
	}
	while (c1 && c1 == c2);
	
	return (c1 - c2);
}

INLINE long
_mint_fstricmp (register const char *str1, register const char *str2)
{
	register char c1, c2;
	
	do {
		c1 = TOLOWER (*str1); str1++;
		c2 = TOLOWER (*str2); str2++;
	}
	while (c1 && c1 == c2);
	
	return (c1 - c2);
}

INLINE long
_mint_fstrncmp (register const char *str1, register const char *str2, register long len)
{
	register char c1, c2;
	
	do {
		c1 = *str1++;
		c2 = *str2++;
	}
	while (--len >= 0 && c1 && c1 == c2);
	
	if (len < 0)
		return 0;
	
	return (c1 - c2);
}

INLINE long
_mint_fstrnicmp (register const char *str1, register const char *str2, register long len)
{
	register char c1, c2;
	
	do {
		c1 = TOLOWER (*str1); str1++;
		c2 = TOLOWER (*str2); str2++;
	}
	while (--len >= 0 && c1 && c1 == c2);
	
	if (len < 0 || c1 == c2)
		return 0;
	
	return (c1 - c2);
}

INLINE void
_mint_fstrcpy (register char *dst, register const char *src)
{
	while ((*dst++ = *src++) != 0)
		;
}

INLINE void
_mint_fstrncpy (register char *dst, register const char *src, register long len)
{
	while (--len >= 0 && (*dst++ = *src++) != 0)
		;
}

INLINE void
_mint_fstrncpy_f (register char *dst, register const char *src, register long len)
{
	if (len <= 0)
		return;
	
	while (*src && --len > 0)
		*dst++ = *src++;
	
	*dst = '\0';
}

INLINE void
_mint_fstrlwr (register char *s)
{
	register char c;
	
	while ((c = *s) != 0)
	{
		*s++ = TOLOWER (c);
	}
}

INLINE void
_mint_fstrupr (register char *s)
{
	register char c;
	while ((c = *s) != 0)
	{
		*s++ = TOUPPER (c);
	}
}


# endif /* _fstring_h */
