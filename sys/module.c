/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
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
# include "global.h"
# include "k_fds.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "memory.h"


long _cdecl
kernel_opendir(struct dirstruct *dirh, const char *name)
{
	long r;

	r = path2cookie(name, follow_links, &dirh->fc);
	if (r == E_OK)
	{
		dirh->index = 0;
		dirh->flags = 0;

		r = xfs_opendir(dirh->fc.fs, dirh, 0);
		if (r) release_cookie(&dirh->fc);
	}

	return r;
}

long _cdecl
kernel_readdir(struct dirstruct *dirh, char *buf, int len)
{
	fcookie fc;
	long r;

	if (!dirh->fc.fs)
		return EBADF;

	r = xfs_readdir(dirh->fc.fs, dirh, buf, len, &fc);
	if (!r) release_cookie(&fc);

	return r;
}

void _cdecl
kernel_closedir(struct dirstruct *dirh)
{
	if (dirh->fc.fs)
	{
		xfs_closedir(dirh->fc.fs, dirh);
		release_cookie(&dirh->fc);
	}
}

struct file * _cdecl
kernel_open(const char *path, int rwmode, long *err)
{
	struct file *f;

	*err = FP_ALLOC(rootproc, &f);
	if (*err) return NULL;

	*err = do_open(&f, path, rwmode, 0, NULL);
	if (*err)
	{
		f->links--;
		FP_FREE(f);
		return NULL;
	}

	return f;
}

long _cdecl
kernel_read(struct file *f, void *buf, long buflen)
{
	return xdd_read(f, buf, buflen);
}

long _cdecl
kernel_write(struct file *f, const void *buf, long buflen)
{
	return xdd_write(f, buf, buflen);
}

long _cdecl
kernel_lseek(FILEPTR *f, long where, int whence)
{
	return xdd_lseek(f, where, whence);
}

void _cdecl
kernel_close(struct file *f)
{
	do_close(rootproc, f);
}


static long load_xfs (struct basepage *b, const char *name);
static long load_xdd (struct basepage *b, const char *name);

static const char *_types[] =
{
	".xdd",
	".xfs"
};

static long (*_loads[])(struct basepage *, const char *) =
{
	load_xdd,
	load_xfs
};

void
load_all_modules(const char *curpath, unsigned long mask)
{
	int i;

	for (i = 0; i < (sizeof(_types) / sizeof(*_types)); i++)
	{
		/* set path first to make sure that MiNT's directory matches
		 * GEMDOS's
		 */
		sys_d_setpath(curpath);

		/* load external xdd */
		if (mask & (1L << i))
			load_modules(_types [i], _loads [i]);
	}

	/* reset curpath just to be sure */
	sys_d_setpath(curpath);
}

static struct basepage *
load_module(const char *filename, long *err)
{
	struct basepage *b = NULL;
	struct file *f;
	FILEHEAD fh;
	long size;

	f = kernel_open(filename, O_DENYW | O_EXEC, err);
	if (!f) return NULL;

	size = kernel_read(f, (void *) &fh, sizeof(fh));
	if (size != sizeof(fh) || fh.fmagic != GEMDOS_MAGIC)
	{
		DEBUG(("load_module: file not executable"));

		*err = ENOEXEC;
		goto failed;
	}

	size = fh.ftext + fh.fdata + fh.fbss;
	size += 256; /* sizeof (struct basepage) */

	b = kmalloc(size);
	if (!b)
	{
		DEBUG(("load_module: out of memory?"));

		*err = ENOMEM;
		goto failed;
	}

	bzero(b, size);
	b->p_lowtpa = (long) b;
	b->p_hitpa = (long)((char *) b + size);

	b->p_flags = fh.flag;
	b->p_tbase = b->p_lowtpa + 256;
	b->p_tlen = fh.ftext;
	b->p_dbase = b->p_tbase + b->p_tlen;
	b->p_dlen = fh.fdata;
	b->p_bbase = b->p_dbase + b->p_dlen;
	b->p_blen = fh.fbss;

	size = fh.ftext + fh.fdata;

	*err = load_and_reloc(f, &fh, (char *) b + 256, 0, size, b);
	if (*err)
	{
		DEBUG(("load_and_reloc: failed"));
		goto failed;
	}

	kernel_close(f);

	/* just to be sure */
	cpush(NULL, -1);

	DEBUG(("load_module: basepage = %lx", b));
	return b;

failed:
	if (b) kfree(b);

	kernel_close(f);
	return NULL;
}

static const char *dont_load_list[] =
{
	"fnramfs.xfs",
	"sockdev.xdd"
};

static int
dont_load(const char *name)
{
	int i;

	for (i = 0; i < (sizeof(dont_load_list) / sizeof(*dont_load_list)); i++)
		if (stricmp(dont_load_list[i], name) == 0)
			return 1;

	return 0;
}

void _cdecl
load_modules(const char *extension, long (*loader)(struct basepage *, const char *))
{
	struct dirstruct dirh;
	char buf[128];
	long len;
	char *name;
	long r;

	DEBUG(("load_modules: enter (%s)", extension));

	strcpy(buf, sysdir);
	len = strlen(buf);
# if 0
	buf[len++] = '\\';
	buf[len] = '\0';
# endif
	name = buf + len;
	len = sizeof(buf) - len;

	r = kernel_opendir(&dirh, buf);
	DEBUG(("load_modules: d_opendir (%s) = %li", buf, r));

	if (r == 0)
	{
		r = kernel_readdir(&dirh, name, len);
		DEBUG(("load_modules: d_readdir = %li (%s)", r, name+4));

		while (r == 0)
		{
			r = strlen(name+4) - 4;
			if ((r > 0) &&
			    stricmp(name+4 + r, extension) == 0 &&
			    !dont_load(name+4))
			{
				char *ptr1 = name;
				char *ptr2 = name+4;
				long len2 = len - 1;
				struct basepage *b;

				while (*ptr2)
				{
					*ptr1++ = *ptr2++;
					len2--; assert(len2);
				}

				*ptr1 = '\0';

				b = load_module(buf, &r);
				if (b)
				{
					DEBUG(("load_modules: load \"%s\"", buf));
					r = loader(b, name);
					DEBUG(("load_modules: load done \"%s\" (%li)", buf, r));

					if (r) kfree(b);
				}
				else
					DEBUG(("load_module of \"%s\" failed (%li)", buf, r));

				/* just to be sure */
				cpush(NULL, -1);
			}

			r = kernel_readdir(&dirh, name, len);
			DEBUG(("load_modules: d_readdir = %li (%s)", r, name+4));
		}

		kernel_closedir(&dirh);
	}
}

static void *
callout_init(void *initfunction, struct kerinfo *k)
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
load_xfs(struct basepage *b, const char *name)
{
	FILESYS *fs;

	DEBUG(("load_xfs: enter (0x%lx, %s)", b, name));
	DEBUG(("load_xfs: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));

	fs = callout_init((void *) b->p_tbase, &kernelinfo);
	if (fs)
	{
		DEBUG(("load_xfs: %s loaded OK (%lx)", name, fs));

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

				DEBUG(("load_xfs: xfs_add (%lx)", fs));
				xfs_add(fs);
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
load_xdd(struct basepage *b, const char *name)
{
	DEVDRV *dev;

	DEBUG(("load_xdd: enter (0x%lx, %s)", b, name));
	DEBUG(("load_xdd: init 0x%lx, size %li", (void *) b->p_tbase, (b->p_tlen + b->p_dlen + b->p_blen)));

	dev = callout_init((void *) b->p_tbase, &kernelinfo);
	if (dev)
	{
		DEBUG(("load_xdd: %s loaded OK", name));

		if ((DEVDRV *) 1L != dev)
		{
			/* we need to install the device driver ourselves */

			char dev_name[PATH_MAX];
			struct dev_descr the_dev;
			int i;
			long r;

			DEBUG(("load_xdd: installing %s itself!\7", name));

			bzero(&the_dev, sizeof(the_dev));
			the_dev.driver = dev;

			strcpy(dev_name, "u:\\dev\\");
			i = strlen(dev_name);

			while (*name && *name != '.')
				dev_name[i++] = tolower((int)*name++ & 0xff);

			dev_name[i] = '\0';

			DEBUG(("load_xdd: final -> %s", dev_name));

			r = sys_d_cntl(DEV_INSTALL, dev_name, (long) &the_dev);
			return r;
		}

		return 0;
	}

	return -1;
}
