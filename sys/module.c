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
 * begin:	2000-08-29
 * last change:	2000-08-29
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * changes since last version:
 * 
 * 2000-08-29:
 * 
 * - initial revision
 * 
 * known bugs:
 * 
 * todo:
 * 
 */

# include "module.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/mem.h"

# include "dosfile.h"
# include "filesys.h"
# include "kmemory.h"
# include "memory.h"


BASEPAGE *
load_module (const char *filename, long *err)
{
	BASEPAGE *b = NULL;
	FILEPTR *f;
	long size;
	FILEHEAD fh;
	
	f = do_open (filename, O_DENYW | O_EXEC, 0, NULL, err);
	if (!f) return NULL;
	
	size = xdd_read (f, (void *) &fh, (long) sizeof (fh));
	if (fh.fmagic != GEMDOS_MAGIC || size != (long) sizeof (fh))
	{
		DEBUG (("load_module: file not executable"));
		
		*err = ENOEXEC;
		goto failed;
	}
	
	size = fh.ftext + fh.fdata + fh.fbss;
	size += 256; /* sizeof BASEPAGE */
	
	b = kmalloc (size);
	if (!b)
	{
		DEBUG (("load_module: out of memory?"));
		
		*err = ENOMEM;
		goto failed;
	}
	
	bzero (b, size);
	b->p_lowtpa = (long) b;
	b->p_hitpa = (long) ((char *) b + size);
	
	b->p_flags = fh.flag;
	b->p_tbase = b->p_lowtpa + 256;
	b->p_tlen = fh.ftext;
	b->p_dbase = b->p_tbase + b->p_tlen;
	b->p_dlen = fh.fdata;
	b->p_bbase = b->p_dbase + b->p_dlen;
	b->p_blen = fh.fbss;
	
	size = fh.ftext + fh.fdata;
	
	*err = load_and_reloc (f, &fh, (char *) b + 256, 0, size, b);
	if (*err)
	{
		DEBUG (("load_and_reloc: failed"));
		goto failed;
	}
	
	DEBUG (("load_module: basepage = %lx", b));
	
	do_close (f);
	return b;
	
failed:
	if (b) kfree (b);
	
	do_close (f);
	return NULL;
}
