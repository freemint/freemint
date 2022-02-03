/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The u:\host filesystem driver - driver main.
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team.
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
#include "aranym_fsdev.h"

#include "nfclip_dev.h"

# include "mint/emu_tos.h"

# include "mint/arch/nf_ops.h"


extern FILESYS * _cdecl init(struct kerinfo *k);


/*
 * nf_ops->call function reference.
 */
long __CDECL (*nf_call)(long id, ...) = 0L;

/*
 * global kerinfo structure
 */
struct kerinfo *KERNEL;


FILESYS * _cdecl init(struct kerinfo *k)
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
	c_conws ("\r\n");

	KERNEL_DEBUG("Found MiNT %ld.%ld with kerinfo version %ld",
		   (long)MINT_MAJOR, (long)MINT_MINOR, (long)MINT_KVERSION);

	/* check for MiNT version */
	if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 16))
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE("This filesystem requires MiNT 1.16.x"));

		return NULL;
	}

	/* install and mount the filesystem */
	fs = aranymfs_init();
	if (fs)
		fs = arafs_mount_drives(fs);

	return fs;
}
