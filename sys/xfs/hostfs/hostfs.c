/*
 * The Host OS filesystem access driver - entry point.
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

#include "mint/kerinfo.h"
#include "mint/arch/nf_ops.h"

#undef DriveToLetter
#define DriveToLetter(d) ((d) < 26 ? 'a' + (d) : (d) - 26 + '1')


/*
 * filesystem basic description
 */
static struct fs_descr hostfs_descr =
    {
	NULL,
	0, /* this is filled in by MiNT at FS_MOUNT */
	0, /* FIXME: what about flags? */
	{0,0,0,0}  /* reserved */
    };


#if __KERNEL__ == 1

FILESYS *hostfs_mount_drives(FILESYS *fs)
{
	long r;
	ulong drv_mask = fs_drive_bits();
	ushort drv_number = 0;
	char mount_point[] = "u:\\XX";

	c_conws("mounts: ");

	while ( drv_mask ) {
		/* search the 1st log 1 bit position -> drv_number */
		while( ! (drv_mask & 1) ) { drv_number++; drv_mask>>=1; }

		mount_point[3] = DriveToLetter(drv_number);
		mount_point[4] = '\0';

		DEBUG(("hostfs: drive: %d", drv_number));

		c_conws( (const char*)mount_point );
		c_conws(" ");

		/* r = d_cntl(FS_MOUNT, mount_point, (long) &hostfs_descr); */
		/* the d_cntl(FS_MOUNT) starts with dev_no = 100 */
		hostfs_descr.dev_no = 50+drv_number;
		hostfs_descr.file_system = fs;
		DEBUG(("hostfs: Dcnt(FS_MOUNT) dev_no: %d", hostfs_descr.dev_no));

		r = fs_native_init( hostfs_descr.dev_no, mount_point, "/", 0 /*caseSensitive*/,
				fs, &hostfs_fs_devdrv );
		/* set the drive bit */
		if ( !r )
			*((long *) 0x4c2L) |= 1UL << drv_number;

		drv_number++; drv_mask>>=1;
	}
	c_conws("\r\n");
	c_conws("\r\n");

	return fs;
}

#else /* __KERNEL__ == 1 */

FILESYS *hostfs_mount_drives(FILESYS *fs)
{
	long r;
	int succ = 0;
	int keep = 0;
	int rollback_failed = 0;

	/**
	 * FIXME: TODO: If there are really different settings in the filesystem
	 * mounting to provide full case sensitive filesystem (like ext2) then
	 * we need to strip the FS_NOXBIT from the filesystem flags. For now
	 * every mount is half case sensitive and behaves just like mint's fatfs
	 * implementation in this sense.
	 *
	 * We would also need to allocate a duplicate copy if there are different
	 * styles of case sensitivity used in order to provide the kernel with
	 * appropriate filesystem information. Also some NF function would be needed
	 * to go though the configuration from the host side (at this point there
	 * is only the fs_drive_bits() which doesn't say much).
	 **/

	hostfs_descr.file_system = fs;

	/* Install the filesystem */
	r = d_cntl (FS_INSTALL, "u:\\", (long) &hostfs_descr);
	DEBUG(("hostfs: Dcntl(FS_INSTALL) descr=%p", &hostfs_descr));
	if (r != 0 && r != (long)KERNEL)
	{
		DEBUG(("Return value was %li", r));

		/* Nothing installed, so nothing to stay resident */
		return NULL;
	}

	{
		ulong drv_mask = fs_drive_bits();
		ushort drv_number = 0;
		char mount_point[] = "u:\\XX";

		succ |= 1;

		c_conws("\r\nMounts: ");

		while ( drv_mask ) {
			/* search the 1st log 1 bit position -> drv_number */
			while( ! (drv_mask & 1) ) { drv_number++; drv_mask>>=1; }

			/* ready */
			succ = 0;

			mount_point[3] = DriveToLetter(drv_number);
			mount_point[4] = '\0';

			DEBUG(("hostfs: drive: %d", drv_number));

			c_conws( (const char*)mount_point );
			c_conws(" ");

			/* mount */
			r = d_cntl(FS_MOUNT, mount_point, (long) &hostfs_descr);
			DEBUG(("hostfs: Dcnt(FS_MOUNT) dev_no: %d", hostfs_descr.dev_no));
			if (r != hostfs_descr.dev_no )
			{
				DEBUG(("hostfs: return value was %li", r));
			} else {
				succ |= 2;
				/* init */

				r = fs_native_init( hostfs_descr.dev_no, mount_point, "/", 0 /*caseSensitive*/,
										   fs, &hostfs_fs_devdrv );
				DEBUG(("hostfs: native_init mount_point: %s", mount_point));
				if ( r < 0 ) {
					DEBUG(("hostfs: return value was %li", r));
				} else {
					succ = 0; /* do not unmount */
					keep = 1; /* at least one is mounted */
				}
			}

			/* Try to uninstall, if necessary */
			if ( succ & 2 ) {
				/* unmount */
				r = d_cntl(FS_UNMOUNT, mount_point,
						   (long) &hostfs_descr);
				DEBUG(("hostfs: Dcntl(FS_UNMOUNT) descr=%p", &hostfs_descr));
				if ( r < 0 ) {
					DEBUG(("hostfs: return value was %li", r));
					/* Can't uninstall, because unmount failed */
					rollback_failed |= 2;
				}
			}

			drv_number++; drv_mask>>=1;
		}

		c_conws("\r\n");
	}

	/* everything OK */
	if ( keep )
		return fs; /* We where successfull */

	/* Something went wrong here -> uninstall the filesystem */
	r = d_cntl(FS_UNINSTALL, "u:\\", (long) &hostfs_descr);
	DEBUG(("hostfs: Dcntl(FS_UNINSTALL) descr=%p", &hostfs_descr));
	if ( r < 0 ) {
		DEBUG(("hostfs: return value was %li", r));
		/* Can't say NULL,
		 * because uninstall failed */
		rollback_failed |= 1;
	}

       	/* Can't say NULL, if IF_UNINSTALL or FS_UNMOUNT failed */
	if ( rollback_failed ) return (FILESYS *) 1;

       	/* Nothing installed, so nothing to stay resident */
	return NULL;
}

#endif /* __KERNEL__ == 1 */
