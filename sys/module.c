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

# include "arch/cpu.h"
# include "libkern/libkern.h"
# include "mint/basepage.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/mem.h"

# include "dosdir.h"
# include "filesys.h"
# include "k_fds.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "memory.h"


static long load_xfs (struct basepage *b, const char *name);
static long load_xdd (struct basepage *b, const char *name);


static const char *_types [] =
{
	".xdd",
	".xfs"
};

static long (*_loads [])(struct basepage *, const char *) =
{
	load_xdd,
	load_xfs
};

void
load_all_modules (const char *curpath, ulong mask)
{
	int i;
	
	for (i = 0; i < (sizeof (_types) / sizeof (*_types)); i++)
	{
		/* set path first to make sure that MiNT's directory matches
		 * GEMDOS's
		 */
		d_setpath (curpath);
		
		/* load external xdd */
		if (mask & (1L << i))
			load_modules (_types [i], _loads [i]);
	}
	
	/* reset curpath just to be sure */
	d_setpath (curpath);
}


static long
_d_opendir (DIR *dirh, const char *name)
{
	long r;
	
	r = path2cookie (name, follow_links, &dirh->fc);
	if (r == E_OK)
	{
		dirh->index = 0;
		dirh->flags = 0;
		
		r = xfs_opendir (dirh->fc.fs, dirh, 0);
		if (r) release_cookie (&dirh->fc);
	}
	
	return r;
}

static void
_d_closedir (DIR *dirh)
{
	if (dirh->fc.fs)
	{
		xfs_closedir (dirh->fc.fs, dirh);
		release_cookie (&dirh->fc);
	}
}

static long
_d_readdir (DIR *dirh, char *buf, int len)
{
	fcookie fc;
	long r;
	
	if (!dirh->fc.fs)
		return EBADF;
	
	r = xfs_readdir (dirh->fc.fs, dirh, buf, len, &fc);
	if (!r) release_cookie (&fc);
	
	return r;
}

static struct basepage *
load_module (const char *filename, long *err)
{
	struct basepage *b = NULL;
	FILEPTR *f;
	long size;
	FILEHEAD fh;
	
	*err = FP_ALLOC (rootproc, &f);
	if (*err) return NULL;
	
	*err = do_open (&f, filename, O_DENYW | O_EXEC, 0, NULL);
	if (*err)
	{
		f->links--;
		FP_FREE (f);
		return NULL;
	}
	
	size = xdd_read (f, (void *) &fh, sizeof (fh));
	if (fh.fmagic != GEMDOS_MAGIC || size != sizeof (fh))
	{
		DEBUG (("load_module: file not executable"));
		
		*err = ENOEXEC;
		goto failed;
	}
	
	size = fh.ftext + fh.fdata + fh.fbss;
	size += 256; /* sizeof (struct basepage) */
	
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
	
	do_close (rootproc, f);
	
	/* just to be sure */
	cpush (NULL, -1);
	
	DEBUG (("load_module: basepage = %lx", b));
	return b;
	
failed:
	if (b) kfree (b);
	
	do_close (rootproc, f);
	return NULL;
}

static const char *paths [] =
{
	".",
	"\\MINT",
	"\\MULTITOS"
};

static const char *dont_load_list [] =
{
	"fnramfs.xfs",
	"sockdev.xdd"
};

static int
dont_load (const char *name)
{
	int i;
	
	for (i = 0; i < (sizeof (dont_load_list) / sizeof (*dont_load_list)); i++)
		if (stricmp (dont_load_list [i], name) == 0)
			return 1;
	
	return 0;
}

void
load_modules (const char *extension, long (*loader)(struct basepage *, const char *))
{
	long i;
	
	DEBUG (("load_modules: enter (%s)", extension));
	
	for (i = 0; i < (sizeof (paths) / sizeof (*paths)); i++)
	{
		DIR dirh;
		char buf [128];
		long len;
		char *name;
		long r;
		
		strcpy (buf, paths [i]);
		len = strlen (buf);
		buf [len++] = '\\';
		buf [len] = '\0';
		name = buf + len;
		len = sizeof (buf) - len;
		
		r = _d_opendir (&dirh, buf);
		DEBUG (("load_modules: d_opendir (%s) = %li", buf, r));
		
		if (r == 0)
		{
			r = _d_readdir (&dirh, name, len);
			DEBUG (("load_modules: d_readdir = %li (%s)", r, name+4));
			
			while (r == 0)
			{
				r = strlen (name+4) - 4;
				if ((r > 0) &&
				    stricmp (name+4 + r, extension) == 0 &&
				    !dont_load (name+4))
				{
					char *ptr1 = name;
					char *ptr2 = name+4;
					long len2 = len - 1;
					struct basepage *b;
					
					while (*ptr2)
					{
						*ptr1++ = *ptr2++;
						len2--; assert (len2);
					}
					
					*ptr1 = '\0';
					
					b = load_module (buf, &r);
					if (b)
					{
						DEBUG (("load_modules: load \"%s\"", buf));
						r = loader (b, name);
						DEBUG (("load_modules: load done \"%s\" (%li)", buf, r));
						
						if (r) kfree (b);
					}
					else
						DEBUG (("load_module of \"%s\" failed (%li)", buf, r));
					
					/* just to be sure */
					cpush (NULL, -1);
				}
				
				r = _d_readdir (&dirh, name, len);
				DEBUG (("load_modules: d_readdir = %li (%s)", r, name+4));
			}
			
			_d_closedir (&dirh);
		}
	}
	
	DEBUG (("load_modules: finished, i = %li", i));
}


static void *
callout_init (void *initfunction, struct kerinfo *k)
{
	register void *ret __asm__("d0");
	
	__asm__ volatile
	(
		"moveml d3-d7/a3-a6,sp@-;"
		"movl	%2,sp@-;"
		"movl   %1,a0;"
		"jsr    a0@;"
		"addqw  #4,sp;"
		"moveml sp@+,d3-d7/a3-a6;"
		: "=r"(ret)				/* outputs */
		: "g"(initfunction), "r"(k)		/* inputs  */
		: "d0", "d1", "d2", "a0", "a1", "a2",   /* clobbered regs */
		  "memory"
	);
	
	return ret;
}

/*
 * load file systems from disk
 * this routine is called after process 0 is set up, but before any user
 * processes are run
 */
static long
load_xfs (struct basepage *b, const char *name)
{
	FILESYS *fs;
	
	DEBUG (("load_xfs: enter (0x%lx, %s)", b, name));
	DEBUG (("load_xfs: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	fs = callout_init ((void *) b->p_tbase, &kernelinfo);
	if (fs)
	{
		DEBUG (("load_xfs: %s loaded OK (%lx)", name, fs));
		
		/* link it into the list of drivers
		 * 
		 * uk: but only if it has not installed itself
		 *     via Dcntl() after checking if file
		 *     system is already installed, so we know
		 *     for sure that each file system in at
		 *     most once in the chain (important for
		 *     removal!)
		 * also note: this doesn't preclude loading
		 * two different instances of the same file
		 * system driver, e.g. it's perfectly OK to
		 * have a "cdromy1.xfs" and "cdromz2.xfs";
		 * the check below just makes sure that a
		 * given instance of a file system is
		 * installed at most once. I.e., it prevents
		 * cdromy1.xfs from being installed twice.
		 */
		if ((FILESYS *) 1L != fs)
		{
			FILESYS *f;
			
			for (f = active_fs; f; f = f->next)
				if (f == fs)
					break;
			
			if (!f)
			{
				/* we ran completly through the list
				 */
				
				DEBUG (("load_xfs: xfs_add (%lx)", fs));
				xfs_add (fs);
			}
		}
		
		return 0;
	}
	
	return -1;
}

/*
 * uk: load device driver in files called *.xdd (external device driver)
 *     from disk
 * maybe this should go into biosfs.c ??
 *
 * this routine is called after process 0 is set up, but before any user
 * processes are run, but before the loadable file systems come in,
 * so they can make use of external device drivers
 */
static long
load_xdd (struct basepage *b, const char *name)
{
	DEVDRV *dev;
	
	DEBUG (("load_xdd: enter (0x%lx, %s)", b, name));	
	DEBUG (("load_xdd: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));
	
	dev = callout_init ((void *) b->p_tbase, &kernelinfo);
	if (dev)
	{
		DEBUG (("load_xdd: %s loaded OK", name));
		
		if ((DEVDRV *) 1L != dev)
		{
			/* we need to install the device driver ourselves */
			
			char dev_name[PATH_MAX];
			struct dev_descr the_dev;
			int i;
			long r;
			
			DEBUG (("load_xdd: installing %s itself!\7", name));
			
			bzero (&the_dev, sizeof (the_dev));
			the_dev.driver = dev;
			
			strcpy (dev_name, "u:\\dev\\");
			i = strlen (dev_name);
			
			while (*name && *name != '.')
				dev_name [i++] = tolower (*name++);
			
			dev_name [i] = '\0';
			
			DEBUG (("load_xdd: final -> %s", dev_name));
			
			r = d_cntl (DEV_INSTALL, dev_name, (long) &the_dev);
			return r;
		}
		
		return 0;
	}
	
	return -1;
}
