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

#include "mint/mintbind.h"

#include "util.h"


int
get_drv(char *p)
{
	if (*(p+1) == ':')
	{
		int c = *p;
		if (c >= 'a' && c <= 'z')
			return c - 'a';
		if (c >= 'A' && c <= 'Z')
			return c - 'A';
		if (c >= '0' && c <= '9')
			return (c - '0') + ('z' - ('a' - 1));
	}
	return -1;
}

int
drive_and_path(char *fname, char *path, char *name, bool n, bool set)
{
	int t; char *tn = fname;
	int drive = get_drv(fname);

	if (drive >= 0)
		tn += 2;

	strcpy(path, tn);

	if (n)
	{
		/* Seperate path & name */
		t = strlen(path)-1;
		while (t >= 0 && path[t] != '\\')
			t--;

		if (path[t] == '\\')
		{
			path[t] = '\0';
			if (name)
				strcpy(name, path + t + 1);
		}
		else
			if (name)
				strcpy(name, path);
	}

	if (set)
	{
		if (drive >= 0)
			Dsetdrv(drive);

		Dsetpath(path);
	}

	return drive;
}

void
strnupr(char *s, int n)
{
	int f;

	for (f = 0; f < n; f++)
		s[f] = toupper(s[f]);
}
