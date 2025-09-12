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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include <stdlib.h>
# include <stdio.h>

# include "../mint/mint.h"

# include "../mint/basepage.h"
# include "../mint/block_IO.h"
# include "../mint/dcntl.h"
# include "../mint/default.h"
# include "../mint/file.h"
# include "../mint/mem.h"
# include "../mint/proc.h"
# include "../mint/ktypes.h"
# include "../mint/xbra.h"

int
main (void)
{
	printf ("sizeof (BASEPAGE)\t= %li\n", sizeof (BASEPAGE));
	printf ("sizeof (DI)\t\t= %li\n", sizeof (DI));
	printf ("sizeof (UNIT)\t\t= %li\n", sizeof (UNIT));
	printf ("sizeof (fcookie)\t= %li\n", sizeof (fcookie));
	printf ("sizeof (DTABUF)\t\t= %li\n", sizeof (DTABUF));
	printf ("sizeof (DIR)\t\t= %li\n", sizeof (DIR));
	printf ("sizeof (XATTR)\t\t= %li\n", sizeof (XATTR));
	printf ("sizeof (FILEPTR)\t= %li\n", sizeof (FILEPTR));
	printf ("sizeof (LOCK)\t\t= %li\n", sizeof (LOCK));
	printf ("sizeof (DEVDRV)\t\t= %li\n", sizeof (DEVDRV));
	printf ("sizeof (FILESYS)\t= %li\n", sizeof (FILESYS));
	printf ("sizeof (MEMREGION)\t= %li\n", sizeof (MEMREGION));
	printf ("sizeof (SHTEXT)\t\t= %li\n", sizeof (SHTEXT));
	printf ("sizeof (FILEHEAD)\t= %li\n", sizeof (FILEHEAD));
	printf ("sizeof (crp_reg)\t= %li\n", sizeof (crp_reg));
	printf ("sizeof (page_type)\t= %li\n", sizeof (page_type));
	printf ("sizeof (long_desc)\t= %li\n", sizeof (long_desc));
	printf ("sizeof (tc_reg)\t\t= %li\n", sizeof (tc_reg));
	printf ("sizeof (CONTEXT)\t= %li\n", sizeof (CONTEXT));
	printf ("sizeof (TIMEOUT)\t= %li\n", sizeof (TIMEOUT));
	printf ("sizeof (PROC)\t\t= %li\n", sizeof (PROC));
	printf ("sizeof (IOREC_T)\t= %li\n", sizeof (IOREC_T));
	printf ("sizeof (BCONMAP2_T)\t= %li\n", sizeof (BCONMAP2_T));
	
	return 0;
}