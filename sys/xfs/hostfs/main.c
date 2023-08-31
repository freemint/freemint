/*
 * The Host OS filesystem access driver - driver main.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa of ARAnyM dev team.
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Originally taken from the STonX CVS repository.
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
 */

#include "global.h"
#include "hostfs_xfs.h"
#include "hostfs_dev.h"
#include "hostfs.h"


/*
 * global kerinfo structure
 */
struct kerinfo *KERNEL;

/*
 * nf_ops->call function reference.
 */
long __CDECL (*nf_call)(long id, ...) = 0L;


FILESYS * _cdecl init_xfs(struct kerinfo *k)
{
	FILESYS *fs = NULL;

	KERNEL = k;

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);

#ifdef ALPHA
	c_conws (MSG_ALPHA);
#elif defined (BETA)
	c_conws (MSG_BETA);
#endif

	DEBUG(("Found MiNT %ld.%ld with kerinfo version %ld",
		   (long)MINT_MAJOR, (long)MINT_MINOR, (long)MINT_KVERSION));

	/* check for MiNT version */
	if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 16))
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE("This filesystem requires MiNT 1.16.x"));

		return NULL;
	}

	/* install filesystem */
	fs = hostfs_init();
	if ( fs ) {
		/* mount the drives according to
		 * the [HOSTFS] hostfsnfig section.
		 */
		fs = hostfs_mount_drives( fs );
	}

	return fs;
}

