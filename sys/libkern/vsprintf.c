/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * begin:	2000-04-17
 * last change:	2000-04-17
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations:
 * 
 */

# include "libkern.h"


# define TIMESTEN(x)	((((x) << 2) + (x)) << 1)

static long
PUTC (char *p, long *cnt, int c, int width)
{
	long put = 1;
	
	if (*cnt <= 0)
		return 0;
	
	*p++ = c;
	*cnt -= 1;
	while (*cnt > 0 && --width > 0)
	{
		*p++ = ' ';
		*cnt -= 1;
		put++;
	}
	
	return put;
}

static long
PUTS (char *p, long *cnt, const char *s, int width)
{
	long put = 0;
	
	if (!s) s = "(null)";
	
	while (*cnt > 0 && *s)
	{
		*p++ = *s++;
		put++;
		*cnt -= 1;
		width--;
	}
	
	while (width-- > 0 && *cnt > 0)
	{
		*p++ = ' ';
		put++;
		*cnt -= 1;
	}
	
	return put;
}

static long
PUTL (char *p, long *cnt, ulong u, int base, int width, int fill_char, int minus)
{
	char obuf[32];
	char *t = obuf;
	long put = 0;
	
	do {
		*t++ = "0123456789ABCDEF"[u % base];
		u /= base;
		width--;
	}
	while (u > 0);
	
	while (width-- > 0 && *cnt > 0)
	{
		*p++ = fill_char;
		put++;
		*cnt -= 1;
	}
	
	if( minus )
	{
		*p++ = '-';
		*cnt -= 1;
		put++;
	}
	while (*cnt > 0 && t != obuf)
	{
		*p++ = *--t;
		put++;
		*cnt -= 1;
	}
	
	return put;
}

long _cdecl
kvsprintf (char *buf, long buflen, const char *fmt, va_list args)
{
	char *p = buf;
	
	char c;
	char fill_char;
	
	long cnt = buflen - 1;
	int width;
	int long_flag;
	
	char *s_arg;
	int   i_arg;
	long  l_arg;
	
	
	while ((c = *fmt++) != 0)
	{
		if (c != '%')
		{
			p += PUTC (p, &cnt, c, 1);
			continue;
		}
		
		c = *fmt++;
		width = 0;
		long_flag = 0;
		fill_char = ' ';
		
		if (c == '0')
			fill_char = '0';
		
		while (c && isdigit (c))
		{
			width = TIMESTEN (width) + (c - '0');
			c = *fmt++;
		}
		
		if (c == 'l' || c == 'L')
		{
			long_flag = 1;
			c = *fmt++;
		}
		
		if (!c) break;
		
		switch (c)
		{
			case '%':
			{
				p += PUTC (p, &cnt, c, width);
				break;
			}
			case 'c':
			{
				i_arg = va_arg (args, int);
				p += PUTC (p, &cnt, i_arg, width);
				break;
			}
			case 's':
			{
				s_arg = va_arg (args, char *);
				p += PUTS (p, &cnt, s_arg, width);
				break;
			}
			case 'i':
			case 'd':
			{
				int minus = 0;
				if (long_flag)
					l_arg = va_arg (args, long);
				else
					l_arg = va_arg (args, int);
				
				if (l_arg < 0)
				{
					width--;
					l_arg = -l_arg;
					minus = 1;
				}
				
				p += PUTL (p, &cnt, l_arg, 10, width, fill_char, minus);
				break;
			}
			case 'o':
			{
				if (long_flag)
					l_arg = va_arg (args, long);
				else
					l_arg = va_arg (args, unsigned int);
				
				p += PUTL (p, &cnt, l_arg, 8, width, fill_char, 0);
				break;
			}
			case 'x':
			{
				if (long_flag)
					l_arg = va_arg (args, long);
				else
					l_arg = va_arg (args, unsigned int);
				
				p += PUTL (p, &cnt, l_arg, 16, width, fill_char, 0);
				break;
			}
			case 'p':
			{
				l_arg = (long)va_arg (args, void *);

				p += PUTL (p, &cnt, l_arg, 16, width, fill_char, 0);
				break;
			}
			case 'u':
			{
				if (long_flag)
					l_arg = va_arg (args, long);
				else
					l_arg = va_arg (args, unsigned);
				
				p += PUTL (p, &cnt, l_arg, 10, width, fill_char, 0);
				break;
			}
		}
	}
	
	*p = '\0';
	return (p - buf);
}
