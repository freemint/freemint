/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *
 * A multitasking AES replacement for MiNT
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

#include <mint/osbind.h>
#include "display.h"


/* reduced (faster) version */
int
vsdisplay(char *to, const char *f, va_list sp)
{
	char *s = to;

	do {
		char c = *f++;
		char *fro;
		int i, sz;
		int lo;
		unsigned long l;

		if (c == '%')
		{
			c = *f++;

			sz = 0;	/* for x */
			if (c == '0')
			{
				sz = *f++ - '0';
				c = *f++;
			}

			if ((lo = c == 'l') != 0)
				c = *f++;

			switch (c)
			{
			case 's':
				fro = va_arg(sp,char *);
				if (fro)
				{
					while ((*s++ = *fro++) != 0)
						;
					s--;
				}
				else
				{
					*s++ = '~';
					*s++ = 'Z';
					*s++ = '~';
				}
				break;

			case 'd':
				if (lo)
				{
					char x[16];
					long sign;

					l = va_arg(sp, long);
					if ((sign = l) < 0)
						l = -l;

					fro = &x[sizeof(x)-1];
					*fro = 0;
					do {
						*--fro = l % 10 + '0';
					}
					while ((l /= 10) > 0 );

					if (sign < 0)
						*--fro = '-';

					while ((*s++ = *fro++) != 0)
						;
					s--;
				}
				else
				{
					char x[7];
					int sign;

					i = va_arg(sp, int);
					if ((sign = i) < 0)
						i = -i;

					fro = &x[sizeof(x)-1];
					*fro = 0;
					do {
						*--fro = i % 10 + '0';
					}
					while ((i/=10) > 0 );

					if (sign < 0)
						*--fro = '-';
	
					while((*s++ = *fro++) != 0)
						;
					s--;
				}
				break;

			case 'X':
			case 'x':
			{
				char x[9];
				if (lo)
					l = va_arg(sp, unsigned long);
				else
					l = va_arg(sp, unsigned int);

				for (i = 0; i < sizeof(x); i++)
					x[i] = '0';

				fro = &x[sizeof(x)-1];
				*fro--=0;
				do  {
					i = (l % 16);
					*fro-- = i + (i < 10 ? '0' : 'W');
				}
				while ((l /= 16) > 0 );
				fro = x;
				if (!sz)
				{
					while (*fro == '0')
						fro++;
					if (*fro == 0)
						fro--;
				}
				else
					fro += sizeof(x) -1 - sz;	/* preliminary */
				while((*s++ = *fro++) != 0)
					;
				s--;
			}
			break;
			case 'c':
				*s++ = va_arg(sp,int);
				break;

			default:
				*s++ = c;
			}
		}
		else
			*s++ = c;
	}
	while (*f);

	*s = 0;
	return s - to;
}

static int
ins_cr(char line[], int l)
{
	if (line[l-1] == '\n')
	{
		if (line[l-2] != '\r')
		{
			l++;
			line[l-2] = '\r';
			line[l-1] = '\n';
			line[l  ] = 0;
		}
	}

	return l;
}

int
sdisplay(char *to, const char *t, ...)
{
	va_list args;
	int l;

	va_start(args,t);
	l = vsdisplay(to, t, args);
	va_end(args);

	return l;
}

int
fdisplay(int fl, int echo, const char *t, ...)
{
	char line[512];
	va_list args;
	int l;

	va_start(args,t);
	l = vsdisplay(line, t, args);
	l = ins_cr(line, l);
	va_end(args);

	if (echo)
		Cconws(line);

	return Fwrite(fl, l, line);
}

int
display(const char *t, ...)
{
	char line[512];
	va_list args;
	int l;

	va_start(args,t);
	l = vsdisplay(line, t, args);
	l = ins_cr(line, l);
	va_end(args);

	Cconws(line);
	return l;
}
