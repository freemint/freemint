/*
 * sysdir.tos
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

#include <stdlib.h>
#include <stdio.h>
#include <mintbind.h>

/* no need to run global constructors/destructors here */
void __main(void);
void __main(void) { }

int main(void)
{
	const char *sysdir = getenv("SYSDIR");

	if (sysdir && Fsymlink(sysdir, "u:\\sysdir") == 0) {
		printf("Symlinked %s to u:\\sysdir.\n", sysdir);
	} else {
		printf("u:\\sysdir could not be created.\n");
	}

	return 0;
}
