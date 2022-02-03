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
#include "mint/emu_tos.h"
#include "mint/arch/nf_ops.h"
#if __KERNEL__ == 1
#include "dosdir.h"
#include "kmemory.h"
#endif


/*
 * filesystem basic description
 */
static struct fs_descr arafs_descr =
    {
	NULL,
	0, /* this is filled in by MiNT at FS_MOUNT */
	0, /* FIXME: what about flags? */
	{0,0,0,0}  /* reserved */
    };


static short mount(FILESYS *fs)
{
	long r;

	arafs_descr.file_system = fs;

	/* Install the filesystem */
	r = d_cntl (FS_INSTALL, "u:\\", (long) &arafs_descr);
	DEBUG(("arafs: Dcntl(FS_INSTALL) descr=%p", &arafs_descr));
	if (r != 0 && r != (long)KERNEL)
	{
		DEBUG(("arafs: Dcntl(FS_INSTALL) return value was %li", r));

		/* Nothing installed, so nothing to stay resident */
		return 0;
	}

	r = d_cntl(FS_MOUNT, "u:\\host", (long) &arafs_descr);
	DEBUG(("arafs: Dcntl(FS_MOUNT) dev_no: %d", arafs_descr.dev_no));
	if ( r == arafs_descr.dev_no ) return r; /* mount successfull */

	DEBUG(("arafs: Dcntl(FS_MOUNT) return value was %li", r));

	r = d_cntl(FS_UNMOUNT, "u:\\host", (long) &arafs_descr);
	if ( r < 0 ) {
		DEBUG(("arafs: Dcntl(FS_UNMOUNT) return value was %li", r));
		/* Can't uninstall, because unmount failed */
		return -1;
	}

	/* Something went wrong here -> uninstall the filesystem */
	r = d_cntl(FS_UNINSTALL, "u:\\", (long) &arafs_descr);
	if ( r < 0 ) {
		DEBUG(("arafs: Dcntl(FS_UNINSTALL) return value was %li", r));
		/* Can't uninstall, because unmount failed */
		return -1;
	}

    /* Nothing installed */
	return 0;
}


FILESYS *aranymfs_init(void)
{
#if __KERNEL__ == 1
	boot_print (MSG_BOOT);
	boot_print (MSG_GREET);
	boot_print ("\r\n");
#endif

	if ( !KERNEL->nf_ops ) {
		c_conws("Native Features not present on this system\r\n");
		return NULL;
	}

	nf_call = KERNEL->nf_ops->call;

	return &arafs_filesys;
}


FILESYS *arafs_mount_drives(FILESYS *fs)
{
	short dev;

	/* install and mount the filesystem */
	dev = mount(fs);

	/* something went wrong */
	if ( dev < 0 )
		return (FILESYS*) 1;

	/* nothing installed */
	if ( !dev )
		return NULL;


	if ( arafs_init(dev) ) {
		static fcookie clipbrd;
		static fcookie scrap;
		fcookie root;
		COOKIE *c;

		/* get the root fcookie */
		if ( fs->root( dev, &root) < 0 )
			return NULL; /* FIXME!!! */

		/* 'mount' the other files/filesystems */

		if ( nfclip_init() ) {
			/* /arafs/clipbrd - normal folder */
			fs->mkdir (&root,  "clipbrd", S_IFDIR | S_IRWXUGO);
			fs->lookup (&root, "clipbrd", &clipbrd);
			c = (COOKIE *) clipbrd.index;
			c->flags |= S_PERSISTENT;

			/* /arafs/clipbrd/scrap.txt - special device file */
			fs->creat (&clipbrd, "scrap.txt", S_IFREG | S_IRWXUGO, 0, &scrap);
			/* cannot not be deleted */
			c = (COOKIE *) scrap.index;
			c->flags |= S_PERSISTENT;
			c->cops = &nfclipops;

			/* release the clipbrd cookie */
			fs->release( &clipbrd);
		}

		/* set IMMUTABLE so that no more files/folders can be
		 * created in the root of the filesystem */
		c = (COOKIE *) root.index;
		c->flags |= S_IMMUTABLE;

		/* release the root cookie */
		fs->release( &root);
	}

	return fs;
}
