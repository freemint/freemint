/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The u:\host\clipbrd\scrap.txt cookie operations
 *   - the CLIBRD NatFeat integration
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
#include "nfclip_nfapi.h"

#include "mint/ioctl.h"

#include "mint/arch/nf_ops.h"


unsigned long nf_clipbrd_id = 0L;


static long 
fetch_host_clipbrd (FILEPTR *f)
{
	short oldflags = f->flags;
	char *buff;

	if ( nf_call( NFCLIP(CLIP_OPEN), 1L, (long)O_WRONLY) )
		return E_OK;

	f->pos32 = 0;
	
	f->flags |= O_RDWR;
	f->dev->ioctl( f, FTRUNCATE, &f->pos32);

	buff = kmalloc (BLOCK_SIZE);
	if ( buff ) {
		do {
			long r = nf_call( NFCLIP(CLIP_READ), 1L, buff, (long)BLOCK_SIZE, (long)f->pos32);
			if ( r <= 0 )
				break;

			f->dev->write( f, buff, r);
		} while (1);

		kfree(buff);
	}

	f->flags = oldflags;
	nf_call( NFCLIP(CLIP_CLOSE), 1L);

	return E_OK;
}


static long 
save_host_clipbrd (FILEPTR *f)
{
	short oldflags = f->flags;
	char *buff;

	if ( nf_call( NFCLIP(CLIP_OPEN), 1L, (long)O_RDONLY) )
		return E_OK;

	f->flags |= O_RDWR;

	buff = kmalloc (BLOCK_SIZE);
	if (buff) {
		f->dev->lseek( f, 0, SEEK_SET);

		do {
			long pos = f->pos32;
			long r = f->dev->read( f, buff, BLOCK_SIZE);
			if ( r <= 0)
				break;

			nf_call( NFCLIP(CLIP_WRITE), 1L, buff, r, pos);
		} while (1);

		kfree(buff);
	}

	f->flags = oldflags;
	nf_call( NFCLIP(CLIP_CLOSE), 1L);

	return E_OK;
}


static long _cdecl
nfclip_open (FILEPTR *f)
{
	if ((f->flags & O_RWMODE) != O_RDONLY)
		return E_OK;

	/* attempt to open clipboard for reading:
	 *   fetch the clip data from the emulator
	 */
	return fetch_host_clipbrd( f);
}

static long _cdecl
nfclip_close (FILEPTR *f, int pid)
{
	if ((f->flags & O_RWMODE) == O_RDONLY)
		return E_OK;

	/* write into clipboard finished:
	 *   send the data to the emulator
	 */
	return save_host_clipbrd( f);
}

static long _cdecl
nfclip_stat(fcookie *fc, STAT *stat)
{
	/* find out the length -> IOW: read the clipboard now */
	FILEPTR f;
	long devsp;

	/* fake the complete FILEPTR structure
	 * to call normal ioctl() on the COOKIE */
	fc->fs->dupcookie( &f.fc, fc);
	f.links = 1;
	f.flags = O_RDONLY;
	f.pos32 = 0;
	f.devinfo = 0;
	f.dev = fc->fs->getdev( fc, &devsp);

	/* fetch the clip data */
	fetch_host_clipbrd( &f);

	/* as the operation actually writes to a file it also
	 * sets the clipboard modification time properly
	 * which is required to have up-to-date scrap.txt fcookie */

	fc->fs->release( &f.fc);

	return E_OK;
}

static DEVDRV devops =
{
	open:			nfclip_open,
	write:			NULL,
	read:			NULL,
	lseek:			NULL,
	ioctl:			NULL,
	datime:			NULL,
	close:			nfclip_close,
	select:			NULL,
	unselect:		NULL,
	writeb:			NULL,
	readb:			NULL
};

COPS nfclipops =
{
dev_ops:			&devops,
stat:				nfclip_stat
};


COPS *nfclip_init( void) {
	nf_clipbrd_id = KERNEL->nf_ops->get_id("CLIPBRD");
	return nf_clipbrd_id != 0 ? &nfclipops : NULL;
}
