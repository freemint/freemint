/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
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
 * begin:	2005-06-10
 * last change:	2005-6-20
 *
 * Author:	Jan Krupka <jkrupka@volny.cz>
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */


#include <mint/mintbind.h>
#include <mint/ssystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
	long  dummy = 0, out = 0;


	if ( (argc < 2) || (argc >= 2 && ( (strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) ) )
	{
		printf("MiNT keyboard table switcher\n");	
		printf("No keyboard table specified.\n");
		printf("Usage: %s keyboard-table \n", argv[0]);
			return 1;
	}


	out = Ssystem(S_LOADKBD, argv[1], &dummy);
	

	if (!out)
		printf("Switched to: %s \n", argv[1]);
	else
		printf("Sorry, %s is not keyboard table \n", argv[1]);


	return (int) out;
}


