/*
 * Copyright 2000 Konrad M. Kokoszkiewicz <draco@atari.org>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 */

# include <mintbind.h>
# include <fcntl.h>
# include <string.h>

#include "util.h"


void
bell(void)
{
	Cconout(7);
}

void
quote_and_cat(char *to, const char *from)
{
	char tmp[128], *t = tmp, c;

	if (*from == 0)
		return;

	if (strchr(from, ' ') || strchr(from, '\''))
	{
		*t++ = '\'';
		for(;;) {
			c = *from++;
			if (c == 0)
				break;
			if (c == '\'')
				*t++ = '\'';
			*t++ = c;
		}
		*t++ = '\'';
		*t = 0;
		strcat(to, tmp);
	} else
		strcat(to, from);
}
