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
# include "arch/init_intr.h"
# include "arch/syscall.h"

# include "libkern/libkern.h"

# include "mint/basepage.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/mem.h"
# include "mint/proc.h"
# include "mint/ssystem.h"
# include "mint/arch/asm.h"

# include "cookie.h"
# include "ssystem.h"
# include "dosdir.h"
# include "filesys.h"
# include "global.h"
# include "init.h"
# include "k_fds.h"
# include "k_prot.h"
# include "kentry.h"
# include "kerinfo.h"
# include "kmemory.h"
# include "memory.h"
# include "time.h"

# include "proc.h"

long _cdecl
kernel_opendir(struct dirstruct *dirh, const char *name)
{
	long r;

	r = path2cookie(get_curproc(), name, follow_links, &dirh->fc);
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
	{
		DEBUG(("kernel_readdir: bad file cookie???"));
		return EBADF;
	}

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
kernel_open(const char *path, int rwmode, long *err, XATTR *x)
{
	struct file *f;
	long r;

	r = do_open(&f, rootproc, path, rwmode, 0, x);
	if (r)
	{
		if( f )
			f->links--;
	}

	if (err) *err = r;
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


static long load_xfs (struct basepage *b, const char *name, short *class, short *subclass);
static long load_xdd (struct basepage *b, const char *name, short *class, short *subclass);

static const char *_types[] =
{
	".xdd",
	".xfs"
};

static long (*_loads[])(struct basepage *, const char *, short *, short *) =
{
	load_xdd,
	load_xfs
};

void
load_all_modules(unsigned long mask)
{
	int i;

	for (i = 0; i < (sizeof(_types) / sizeof(*_types)); i++)
	{
		/* load external xdd */
		if (mask & (1L << i))
		{
			DEBUG(("load_all_modules: processing \"%s\"", _types [i]));
			load_modules(sysdir, _types [i], _loads [i]);
			DEBUG(("load_all_modules: done with \"%s\"", _types [i]));

			stop_and_ask();
		}
	}
}

static struct kernel_module *loaded_modules = NULL;

static void
free_km(struct kernel_module *km)
{
	if (km->prev)
		km->prev->next = km->next;
	else
		loaded_modules = km->next;

	if (km->next)
		km->next->prev = km->prev;

	kfree(km);
}

static struct kernel_module *
load_module(const char *path, const char *filename, long *err)
{
	struct basepage *b;
	struct kernel_module *km = NULL;
	struct file *f;
	FILEHEAD fh;
	long size, kmsize;

	f = kernel_open(filename, O_DENYW | O_EXEC, err, NULL);
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

	kmsize = ((sizeof(*km) + 15) & 0xfffffff0);

	km = kmalloc(size + kmsize);
	if (!km)
	{
		DEBUG(("load_module: out of memory?"));

		*err = ENOMEM;
		goto failed;
	}

	mint_bzero(km, size + kmsize);
	b = (struct basepage *)((char *)km + kmsize);

	b->p_lowtpa = (long) b;
	b->p_hitpa = (long)((char *)b + size);

	b->p_flags = fh.flag;
	b->p_tbase = b->p_lowtpa + 256;
	b->p_tlen = fh.ftext;
	b->p_dbase = b->p_tbase + b->p_tlen;
	b->p_dlen = fh.fdata;
	b->p_bbase = b->p_dbase + b->p_dlen;
	b->p_blen = fh.fbss;

	*err = load_and_reloc(f, &fh, (char *)b + 256, 0, fh.ftext + fh.fdata, b);
	/* close file */
// 	kernel_close(f);

	/* Ozk: For some reason, this cpush is needed,
	 * else the kernel wont run on my Milan040!
	 */
	/* Why, oh why!!! is this necessary??
	 * Well..ozk, if you open your eyes when fixing things,
	 * you would understand!! Goddamned.
	 */
// 	cpushi(NULL, -1);

	/* check for errors */
	if (*err)
	{
		DEBUG(("load_and_reloc: failed"));
		goto failed;
	}

	km->next = loaded_modules;
	km->b = b;
	km->flags = MOD_LOADED;
	if (path)
	{
		strcpy(km->name, filename);
		strcpy(km->path, path);
	}
	else
	{
		char *sa, *sb;
		strcpy(km->path, filename);
		sa = strrchr(km->path, '\\');
		sb = strrchr(km->path, '/');
		if (sa && sb)
		{
			if (sa < sb)
				sa = sb;
		}
		else if (!sa)
			sa = sb;
		if (sa)
		{
			sa++;
			strcpy(km->name, sa);
			*sa = '\0';
		}
		else
			strcpy(km->name, filename);
	}
	loaded_modules = km;
	kernel_close(f);

	DEBUG(("load_module: name '%s', path '%s'", km->name, km->path));
	DEBUG(("load_module: basepage = %lx,km=%lx", b, km));
	return km;
failed:
	kernel_close(f);
	if (km) kfree(km);

	DEBUG(("load_module: -> NULL"));
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
load_modules_old(const char *ext, long (*loader)(struct basepage *, const char *))
{
	long (*l)(struct basepage *, const char *, short *, short *);

	l = (void *)loader;

	load_modules(sysdir, ext, l);
}

void _cdecl
load_modules(const char *path, const char *ext, long (*loader)(struct basepage *, const char *, short *, short *))
{
	struct dirstruct dirh;
	char buf[128];
	char *name;
	long len, r;

	DEBUG(("load_modules: enter(\"%s\", \"%s\", 0x%lx)", path ? path : sysdir, ext, loader));

	strcpy(buf, path ? path : sysdir);
	len = strlen(buf);

	/* add path delimiter if missing */
	if (buf[len-1] != '/' && buf[len-1] != '\\')
	{
		DEBUG(("load_modules: path separator is missing, adding it"));
		buf[len++] = '\\';
		buf[len] = '\0';
	}

	name = buf + len;
	len = sizeof(buf) - len;

	r = kernel_opendir(&dirh, buf);
	DEBUG(("load_modules: kernel_opendir (%s) = %li", buf, r));

	if (r == 0)
	{
		r = kernel_readdir(&dirh, name, len);
		DEBUG(("load_modules: kernel_readdir = %li (%s)", r, name+4));

		while (r == 0)
		{
			r = strlen(name+4) - 4;
			if ((r > 0) &&
			    stricmp(name+4 + r, ext) == 0 &&
			    !dont_load(name+4))
			{
				struct kernel_module *km;

				/* remove the 4 byte index from readdir */
				{
					char *ptr1 = name;
					char *ptr2 = name+4;
					long len2 = len - 1;

					while (*ptr2)
					{
						*ptr1++ = *ptr2++;
						len2--; assert(len2);
					}

					*ptr1 = '\0';
				}

				km = load_module(path, buf, &r);
				if (km)
				{
					DEBUG(("load_modules: load \"%s\"", buf));
					r = loader(km->b, name, &km->class, &km->subclass);
					DEBUG(("load_modules: load done \"%s\" (%li)", buf, r));

					if (r)
						free_km(km);
				}
				else
					DEBUG (("load_module of \"%s\" failed (%li)", buf, r));
			}

			r = kernel_readdir(&dirh, name, len);
			DEBUG(("load_modules: kernel_readdir = %li (%s)", r, name+4));
		}
		kernel_closedir(&dirh);
	}
}

#if 0
/*
 * this don't work?!
 * why need the registers to be preserved?
 * all the modules are gcc compiled too ...
 */
static void *
module_init(void *initfunc, struct kerinfo *k)
{
	void * _cdecl (*init)(struct kerinfo *);

	init = initfunc;
	return (*init)(k);
}
#else

#if __GNUC__ > 2 || __GNUC_MINOR__ > 5
# if __GNUC__ >= 3
   /* gcc 3 does not want a clobbered register to be input or output */
#  define LOCAL_CLOBBER_LIST	__CLOBBER_RETURN("d0") "d1", "d2", "a0", "a1", "a2", "memory"
# else
#  define LOCAL_CLOBBER_LIST	__CLOBBER_RETURN("d0") "d1", "d2", "a0", "a1", "a2", "memory"
# endif
#else
# define LOCAL_CLOBBER_LIST
#endif


static void *
module_init(void *initfunc, struct kerinfo *k)
{
	register void *ret __asm__("d0");

	__asm__ volatile
	(
		PUSH_SP("d3-d7/a3-a6", 36)
		"move.l	%2,-(sp)\n\t"
		"move.l	%1,a0\n\t"
		"jsr	(a0)\n\t"
		"addq.l	#4,sp\n\t"
		POP_SP("d3-d7/a3-a6", 36)
		: "=r"(ret)				/* outputs */
		: "r"(initfunc), "r"(k)			/* inputs  */
		: LOCAL_CLOBBER_LIST /* clobbered regs */
	);

	return ret;
}
#endif

/*
 * load file systems from disk
 * this routine is called after process 0 is set up, but before any user
 * processes are run
 */
long
load_xfs(struct basepage *b, const char *name, short *class, short *subclass)
{
	void *initfunc = (void *)b->p_tbase;
	FILESYS *fs;

	DEBUG(("load_xfs: enter (0x%lx, %s)", b, name));
	DEBUG(("load_xfs: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));

	fs = module_init(initfunc, &kernelinfo);
	if (fs)
	{
		DEBUG(("load_xfs: %s loaded OK (%lx)", name, fs));

		*class = MODCLASS_XFS;
		*subclass = 0;
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
long
load_xdd(struct basepage *b, const char *name, short *class, short *subclass)
{
	void *initfunc = (void *)b->p_tbase;
	DEVDRV *dev;

	DEBUG(("load_xdd: enter (0x%lx, %s)", b, name));
	DEBUG(("load_xdd: init 0x%lx, size %li", initfunc, (b->p_tlen + b->p_dlen + b->p_blen)));
	dev = module_init(initfunc, &kernelinfo);
	if (dev)
	{
		DEBUG(("load_xdd: %s loaded OK", name));

		*class = MODCLASS_XDD;
		*subclass = 0;
		if ((DEVDRV *) 1L != dev)
		{
			/* we need to install the device driver ourselves */

			char dev_name[PATH_MAX];
			struct dev_descr the_dev;
			int i;
			long r;

			DEBUG(("load_xdd: installing %s itself!\7", name));

			mint_bzero(&the_dev, sizeof(the_dev));
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

long _cdecl
register_trap2(long _cdecl (*dispatch)(void *), int mode, int flag, long extra)
{
	long _cdecl (**handler)(void *) = NULL;
	long *x = NULL;
	long ret = EINVAL;

	DEBUG(("register_trap2(0x%lx, %i, %i)", dispatch, mode, flag));

	if (flag == 0)
	{
		handler = &aes_handler;
	}
	else if (flag == 1)
	{
		handler = &vdi_handler;
		x = &gdos_version;
	}

	if (mode == 0)
	{
		/* install */

		DEBUG(("register_trap2: *handler=%lx,old_trap2=%lx", *handler, old_trap2 ));
		if (*handler == NULL)
		{
			DEBUG(("register_trap2: installing handler at 0x%lx", dispatch));

			*handler = dispatch;
			if (x)
				*x = extra;
			ret = 0;

			/* if trap #2 is not active install it now */
			/* in case someone unhooked the MiNT-trap, install again */
			if (old_trap2 == 0 || sys_s_system( S_XBRALOOKUP, TRAP2, COOKIE_MiNT ) )
				new_xbra_install(&old_trap2, TRAP2, mint_trap2);
		}
	}
	else if (mode == 1)
	{
		/* deinstall */

		if (*handler == dispatch)
		{
			DEBUG(("register_trap2: removing handler at 0x%lx", dispatch));

			*handler = NULL;

			if (x)
				*x = 0;
			ret = 0;
		}
	}

	return ret;
}

static long _cdecl
load_km(const char *path, struct kernel_module **res_km)
{
	struct kernel_module *km;
	long err;

	km = load_module(NULL, path, &err);
	if (km)
	{
		km->class = MODCLASS_KM;
		km->subclass = 0;
	}
	if (res_km)
		*res_km = km;
	return err;
}

static bool _cdecl
km_loaded(struct kernel_module *km)
{
	struct kernel_module *list = loaded_modules;

	if (!km)
		return false;

	while (list)
	{
		if (list == km)
			break;
		list = list->next;
	}
	return list ? true : false;
}
#if 0
static long _cdecl
probe_km(struct kernel_module *km)
{
	long ret;
	if (km_loaded(km))
	{
		long _cdecl (*probe)(struct kentry *, const char *name, struct km_api **);
		struct km_api *kmapi = NULL;

		probe = (long _cdecl(*)(struct kentry *, const char *, struct km_api **)km->b->p_tbase;
		ret = (*probe)(&kentry, km->path, &kmapi);
		if (ret != 0L)
		{
			if (kmapi)
				km->kmapi = *kmapi;
			else
				km->flags |= OLDMODULE;
		}
	}
	else
		ret = EBADARG;
	return ret;
}
#endif
/*
 * run a kernel module
 * kernel module init is to be intended to block until finished
 */
long _cdecl sys_c_conin(void);

static long _cdecl
run_km(const char *path)
{
	long err;
	struct kernel_module *km = NULL;

	err = load_km(path, &km);

	if (err == E_OK && km_loaded(km))
	{
		long _cdecl (*run)(struct kentry *, const struct kernel_module *);

		run = (long _cdecl(*)(struct kentry *, const struct kernel_module *))km->b->p_tbase;
		km->caller = curproc;	/* save caller for KM_FREE */
		err = (*run)(&kentry, km);
	}
	else
		err = EBADARG;

	return err;
}
#if 0
static long _cdecl
run_km(const char *path)
{
	struct kernel_module *km;
	long err;

	km = load_module(NULL, path, &err);
	if (km)
	{
		long _cdecl (*run)(struct kentry *, const char *path);

		FORCE("run_km(%s) ok (bp 0x%lx)!", path, km->b);

		//sys_c_conin();
		km->class = MODCLASS_KM;
		km->subclass = 0;

		run = (long _cdecl (*)(struct kentry *, const char *))km->b->p_tbase;
		err = (*run)(&kentry, path);

		free_km(km);
	}
	else
		FORCE("run_km(%s) failed -> %li", path, err);

	return err;
}
#endif
/*
 * kernel module device
 * intended for runtime configuration/loading/unloading
 * of kernel modules
 */

static long _cdecl
module_open(FILEPTR *f)
{
	DEBUG(("module_open [%i]: enter (%lx)", f->fc.aux, f->flags));

	if (!suser(get_curproc()->p_cred->ucr))
		return EPERM;

	return 0;
}

static long _cdecl
module_close(FILEPTR *f, int pid)
{
	DEBUG(("module_close [%i]: enter", f->fc.aux));

	return E_OK;
}

static long _cdecl
module_write(FILEPTR *f, const char *buf, long bytes)
{
	return 0;
}

static long _cdecl
module_read(FILEPTR *f, char *buf, long bytes)
{
	return 0;
}

static long _cdecl
module_lseek(FILEPTR *f, long where, int whence)
{
	return 0;
}

/* 1 if all km  must have different names */
#define NAMELOADCHECK	1

static long _cdecl
module_ioctl(FILEPTR *f, int mode, void *buf)
{
	long r = ENOSYS;
#if NAMELOADCHECK
	char *p1, *p2;
#endif
	struct kernel_module *km;

	DEBUG(("module_ioctl [%i]: (%x, (%c %i), %lx)",
		f->fc.aux, mode, (char)(mode >> 8), (mode & 0xff), buf));

	if( !buf )
		return EINVAL;

	/* only root may */
	if( !suser (curproc->p_cred->ucr) )
		return EPERM;

#if NAMELOADCHECK
	/* stored name may have path or not */
	p1 = strrchr( buf, '/');
	p2 = strrchr( buf, '\\');
	if( p2 > p1 )
		p1 = p2;
	if( !p1 )
		p1 = buf;
	else
		p1++;
#endif

	switch (mode)
	{
		case KM_RUN:
		{
#if NAMELOADCHECK
			/* check if module already loaded */
			for( km = loaded_modules; km; km = km->next )
			{
				DEBUG(("module_ioctl: run_km: looking(%s,%s)", km->name, p1));
				if( km->class == MODCLASS_KM && !(strcmp( p1, km->name ) && strcmp( buf, km->name )) )
				{
					DEBUG(("module_ioctl: module_ioctl:KM_RUN:(%s) already loaded",p1));
					r = EACCES;	/* ELOCKED? */
					break;
				}
			}
			if( !km )	/* not found -> run */
#endif
			{
				r = run_km(buf);
			}
			break;
		}

		case KM_FREE:
		{
			/* check if module started by caller is loaded */
			for( km = loaded_modules; km; km = km->next )
			{
				DEBUG(("module_ioctl: free_km: looking(%s) cur:%lx caller:%lx", km->name, curproc, km->caller));
				if( km->class == MODCLASS_KM && km->caller == curproc && !strcmp( buf, km->name ) )
				{
					DEBUG(("module_ioctl: free_km(%s)",km->name));
					free_km( km );
					r = 0;
					break;
				}
				if( !km )
				{
					r = ENOENT;
					DEBUG(("module_ioctl: free_km(%s) not found",buf));
				}
			}
			break;
		}
	}

	DEBUG (("module_ioctl(%d): return %li", mode&0xff, r));
	return r;
}

static long _cdecl
module_datime(FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (rwflag)
		return EACCES;

	*timeptr++ = timestamp;
	*timeptr = datestamp;

	return E_OK;
}

static long _cdecl
module_select(FILEPTR *f, long proc, int mode)
{
	/* default -- we don't know this mode, return 0 */
	return 0;
}

static void _cdecl
module_unselect(FILEPTR *f, long proc, int mode)
{
}


DEVDRV module_device =
{
	module_open,
	module_write, module_read, module_lseek, module_ioctl, module_datime,
	module_close,
	module_select, module_unselect,
	NULL, NULL
};
