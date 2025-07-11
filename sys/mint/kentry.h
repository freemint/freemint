/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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

# ifndef _mint_kentry_h
# define _mint_kentry_h

/* Force mass storage support on so that kernel modules are built correctly for
 * all targets. Affects *.km modules, but not xif, xdd, xfs etc.
 *
 * Do not remove unless you know what you are doing.
 */

# ifndef __KERNEL__
# error not a KERNEL source
# endif

# include <stdarg.h>

# include "kcompiler.h"
# include "ktypes.h"
# include "mint/xbiosvecs.h"
# include "mint/biosvecs.h"
# include "mint/dosvecs.h"

/* forward declarations */
struct basepage;
struct bio;
struct bpb;
struct businfo;
struct create_process_opts;
struct devinfo;
struct dirstruct;
struct dlong;
struct dma;
struct file;
struct global;
struct ilock;
struct kerinfo;
struct memregion;
struct mfp;
struct module_callback;
struct nf_ops;
struct parser_item;
struct parsinf;
struct pci_conv_adr;
struct scsicmd;
struct scsidrv;
struct semaphore;
struct target;
struct timeout;
struct timeval;


/* kentry - kernel entry vector
 * ----------------------------
 *
 * New kernel entry interface. This is a replacement for the existing
 * kerinfo interface for xdd and xfs modules. It's better sorted and include
 * much, much more useful kernel routines that are exported to the new kernel
 * modules.
 *
 * It also have a seperate version number (major and minor version). A module
 * must check the major version and only if the major version match against
 * the major version the module was compiled the module can be loaded.
 *
 *
 * All functions should be called using the GCC calling conventions:
 * -----------------------------------------------------------------
 *
 * (1) parameters are passed on the stack, aligned on 16 bit boundaries
 * (2) registers d0-d1 and a0-a1 are scratch registers and may be modified
 *     by the called functions
 * (3) if the function returns a value, it will be returned in register
 *     d0
 *
 * data types:
 * -----------
 * int   is 16bit
 *
 * short is 16bit
 * long  is 32bit
 *
 * and unsigned
 *
 */

/* ATTENTION!
 * ----------
 * Every structure size modification *must* increase the major version.
 * Every other modifcation must increase the minor version.
 *
 * major and minor are of type unsigned char. I hope 255 major and minor
 * versions are enough :-)
 */
#define KENTRY_MAJ_VERSION	0
#define KENTRY_MIN_VERSION	22

/* hardware dependant vector
 */
struct kentry_mch
{
	/* global values/flags */
	const struct global * const global;

	/* for udelay timing loop */
	const unsigned long *loops_per_sec;

	/* address of the linear 20ms counter */
	const unsigned long *c20ms;

	/* mfp base register */
	struct mfp *mfpregs;

	/* CPU dependant functions */
	void _cdecl (*cpush)(const void *base, long size);
	void _cdecl (*cpushi)(const void *base, long size);

	/* nf operation vector if available on this machine; NULL otherwise */
	struct nf_ops *nf_ops;
};
#define DEFAULTS_kentry_mch \
{ \
	&global, \
	&loops_per_sec, \
	&c20ms, \
	_mfpregs, \
	cpush, \
	cpushi, \
	NULL, \
}


/* process related functions
 */
struct kentry_proc
{
	/* utility functions for dealing with pauses and sleep
	 * ---------------------------------------------------
	 *
	 * nap(n)
	 *
	 * go to sleep temporarily for about "n" milliseconds
	 * (the exact time slept may vary)
	 *
	 * sleep(que, cond)
	 *
	 * wait on system queue "que" until a condition occurs
	 *
	 * wake(que, cond)
	 *
	 * wake all processes on queue "que" that are waiting for
	 * condition "cond"
	 *
	 * wakeselect(p)
	 *
	 * wake a process that is doing a select(); "param" should be the
	 * process code passed to select()
	 */
	void	_cdecl	(*nap)(unsigned);
	int	_cdecl	(*sleep)(int que, long cond);
	void	_cdecl	(*wake)(int que, long cond);
	void	_cdecl	(*wakeselect)(struct proc *p);

	/* interrupt safe kill and wake
	 * --------------------------
	 *
	 * ikill(p, sig)
	 * iwake(que, cond, pid)
	 */
	long	_cdecl	(*ikill)(int, unsigned short);
	void	_cdecl	(*iwake)(int que, long cond, short pid);

	long	_cdecl	(*killgroup)(int, unsigned short, int);
	void	_cdecl	(*raise)(unsigned short sig);

	/* functions for adding/cancelling timeouts
	 * ----------------------------------------
	 *
	 * addtimeout(delta, func)
	 *
	 * schedules a callback to occur at some future time "delta" is the
	 * number of milliseconds before the timeout is to occur, and func is
	 * a pointer to the function which should be called at that time. Note
	 * that the kernel's timeout facility does not actually have a
	 * millisecond resolution, so the function will most likely be called
	 * some time after specified time has elapsed (i.e. don't rely on this
	 * for really accurate timing). Also note that the timeout may occur
	 * in the context of a different process than the one which scheduled
	 * the timeout, i.e. the given function should not make assumptions
	 * involving the process context, including memory space.
	 * addtimeout() returns a long "magic cookie" which may be passed to
	 * the canceltimeout() function to cancel the pending time out.
	 * This function is only available in MiNT versions 1.06 and later;
	 * otherwise the pointer will be NULL.
	 *
	 * canceltimeout(which)
	 *
	 * cancels a pending timeout. The parameter is the "magic cookie"
	 * returned from addtimeout(). If the timeout has already occured,
	 * canceltimeout does nothing, otherwise it removes the timeout from the
	 * kernel's schedule.
	 *
	 * addroottimeout(delta, func, flags)
	 *
	 * same as addtimeout() but the timeout is attached to the root
	 * process (MiNT) and is never implicitly (programm termination)
	 * canceled
	 * flags - bitvektor, only bit 1 defined at the moment
	 *         if bit 1 is set, addroottimeout is called from an interrupt
	 *         handler and use a static list for new timeouts
	 *         Useful for Softinterrupts!
	 *       - all other bits must be set to 0
	 *
	 * cancelroottimeout(which)
	 *
	 * same as canceltimeout() but for root timeouts
	 */
	struct timeout *_cdecl (*addtimeout)(long, void (*)(PROC *, long arg));
	void _cdecl (*canceltimeout)(struct timeout *);
	struct timeout *_cdecl (*addroottimeout)(long, void (*)(PROC *, long arg), unsigned short);
	void _cdecl (*cancelroottimeout)(struct timeout *);

	/* create a new process */
	long _cdecl (*create_process)(const void *ptr1, const void *ptr2, const void *ptr3,
				      struct proc **pret, long stack,
				      struct create_process_opts *);

	/*
	 * fork a kernel thread for process p
	 *
	 * arguments:
	 * ----------
	 * p    - the process context for which the kernel thread is created;
	 *        can be NULL, in this case a kernel thread of rootproc is created
	 *        NOTE: Anything except the signal handler is shared!
	 *
	 * func - the function where the thread starts
	 *
	 * arg  - additional argument passed to func
	 *
	 * fmt  - printf format string for the process name
	 *
	 * ...  - printf args
	 */
	long _cdecl (*kthread_create)(struct proc *p, void _cdecl (*func)(void *), void *arg,
				      struct proc **np, const char *fmt, ...) __attribute__((format(printf, 5, 6)));
	/*
	 * leave kernel thread previously created by kthread_create
	 *
	 * NOTE: can only be called from INSIDE the thread
	 */
	void _cdecl (*kthread_exit)(short code);

	/* return current process context descriptor */
	struct proc *_cdecl (*get_curproc)(void);
	struct proc *_cdecl (*pid2proc)(int pid);

	/* proc extension management */
	void *_cdecl (*lookup_extension)(struct proc *p, long ident);
	void *_cdecl (*attach_extension)(struct proc *p, long ident,
					 unsigned long flags, unsigned long size, struct module_callback *);
	void  _cdecl (*detach_extension)(struct proc *p, long ident);

	/* internal setuid/setgid */
	long _cdecl (*proc_setuid)(struct proc *p, unsigned short uid);
	long _cdecl (*proc_setgid)(struct proc *p, unsigned short gid);
};
#define DEFAULTS_kentry_proc \
{ \
	nap, \
	sleep, \
	wake, \
	wakeselect, \
	\
	ikill, \
	iwake, \
	killgroup, \
	raise, \
	\
	addtimeout_curproc, \
	canceltimeout, \
	addroottimeout, \
	cancelroottimeout, \
	\
	create_process, \
	\
	kthread_create, \
	kthread_exit, \
	\
	get_curproc, \
	pid2proc, \
	\
	proc_lookup_extension, \
	proc_attach_extension, \
	proc_detach_extension, \
	\
	proc_setuid, \
	proc_setgid, \
}


/* memory management related
 */
struct kentry_mem
{
	/* memory allocation functions
	 * ---------------------------
	 *
	 * kmalloc(size) / kfree (ptr)
	 *
	 * kmalloc and kfree should be used for most purposes, and act like
	 * malloc and free respectively; kmalloc'ed memory is *exclusive*
	 * kernel memory, supervisor proteced
	 *
	 * umalloc(size) / ufree (ptr)
	 *
	 * umalloc and ufree may be used to allocate/free memory that is
	 * attached to the current process, and which is freed automatically
	 * when the process exits; this is generally not of much use to a file
	 * system driver
	 *
	 */

	void *_cdecl (*kcore)(unsigned long size, const char *func);
	void *_cdecl (*kmalloc)(unsigned long size, const char *func);
	void  _cdecl (*kfree)(void *place, const char *func);

	void *_cdecl (*dmabuf_alloc)(unsigned long size, short cmode, const char *func);

	void *_cdecl (*umalloc)(unsigned long size, const char *func);
	void  _cdecl (*ufree)(void *place, const char *func);

	struct memregion *_cdecl (*addr2mem)(struct proc *p, long addr);
	long  _cdecl (*attach_region)(struct proc *proc, struct memregion *reg);
	void  _cdecl (*detach_region)(struct proc *proc, struct memregion *reg);
};
#define DEFAULTS_kentry_mem \
{ \
	NULL, \
	_kmalloc, \
	_kfree, \
	\
	_dmabuf_alloc, \
	\
	_umalloc, \
	_ufree, \
	\
	addr2mem, \
	attach_region, \
	detach_region, \
}


/* filesystem related
 */
struct kentry_fs
{
	/* extended filesystem vector
	 */

	unsigned short default_perm; /* default file permissions */
	unsigned short default_dir_perm; /* default directory permissions */

	/* media change vector
	 * -------------------
	 * call this if a device driver detects a disk
	 * change during a read or write operation. The parameter is the BIOS
	 * device number of the disk that changed.
	 */
	void _cdecl (*_changedrv)(unsigned short drv, const char *function);

	/* file system utility functions
	 * -----------------------------
	 *
	 * denyshare(list, f)
	 *
	 * "list" is a list of open files, "f" is a new file that is being
	 * opened.
	 * If the file sharing mode of f conflicts with any of the FILEPTRs
	 * in the list, then this returns 1, otherwise 0.
	 *
	 * denylock(list, newlock)
	 *
	 * checks a list of locks to see if the new lock conflicts with any
	 * one in the list. If so, the first conflicting lock is returned;
	 * otherwise, a NULL is returned.
	 * This function is only available if maj_version > 0 or
	 * min_version >= 94. Otherwise, it will be a null pointer.
	 */
	int _cdecl (*denyshare)(struct file *, struct file *);
	LOCK *_cdecl (*denylock)(ushort pid, struct ilock *, struct ilock *);

	struct bio *bio; /* buffered block I/O */
	const struct timeval *xtime; /* pointer to current kernel time - UTC */

	long _cdecl (*kernel_opendir)(struct dirstruct *dirh, const char *name);
	long _cdecl (*kernel_readdir)(struct dirstruct *dirh, char *buf, int len);
	void _cdecl (*kernel_closedir)(struct dirstruct *dirh);

	struct file * _cdecl (*kernel_open)(const char *path, int rwmode, long *err, XATTR *x);
	long _cdecl (*kernel_read)(struct file *f, void *buf, long buflen);
	long _cdecl (*kernel_write)(struct file *f, const void *buf, long buflen);
	long _cdecl (*kernel_lseek)(FILEPTR *f, long where, int whence);
	void _cdecl (*kernel_close)(struct file *f);

	long _cdecl (*path2cookie)(struct proc *p, const char *path, char *lastname, fcookie *res);
	long _cdecl (*relpath2cookie)(struct proc *p, fcookie *relto, const char *path, char *lastname, fcookie *res, int depth);
	void _cdecl (*release_cookie)(fcookie *fc);
};
#define DEFAULTS_kentry_fs \
{ \
	DEFAULT_MODE, \
	DEFAULT_DIRMODE, \
	\
	_changedrv, \
	\
	denyshare, \
	denylock, \
	\
	&bio, \
	&xtime, \
	\
	kernel_opendir, \
	kernel_readdir, \
	kernel_closedir, \
	\
	kernel_open, \
	kernel_read, \
	kernel_write, \
	kernel_lseek, \
	kernel_close, \
	\
	path2cookie, \
	relpath2cookie, \
	release_cookie \
}


/* socket related
 */
struct kentry_sockets
{
	void _cdecl (*so_register)(short, struct dom_ops *);
	void _cdecl (*so_unregister)(short);
	long _cdecl (*so_release)(struct socket *);
	void _cdecl (*so_sockpair)(struct socket *, struct socket *);
	long _cdecl (*so_connect)(struct socket *, struct socket *, short, short, short);
	long _cdecl (*so_accept)(struct socket *, struct socket *, short);
	long _cdecl (*so_create)(struct socket **, short, short, short);
	long _cdecl (*so_dup)(struct socket **, struct socket *);
	void _cdecl (*so_free)(struct socket *);
};
#define DEFAULTS_kentry_sockets \
{ \
	so_register, \
	so_unregister, \
	so_release, \
	so_sockpair, \
	so_connect, \
	so_accept, \
	so_create, \
	so_dup, \
	so_free, \
}


/* module related
 */
#define MOD_LOADED	1

#define MODCLASS_XIF	1
#define MODCLASS_XDD	2
#define MODCLASS_XFS	3
#define MODCLASS_KM	4
#define MODCLASS_KMDEF	5

struct kentry_module
{
	/* Load modules with filename extension specified with 'ext' argument
	 * from path.
	 *
	 * 'path' can be NULL, in this case <sysdir> is used to search for the
	 * modules.
	 *
	 * 'ext' need to include the '.' that begin the filename extension.
	 */
	void _cdecl (*load_modules)(const char *path,
				    const char *ext,
				    long _cdecl (*loader)(struct basepage *, const char *, short *, short *));

	/* register VDI or AES trap handler
	 *
	 * mode = 0 -> install
	 * mode = 1 -> remove
	 *
	 * flag = 0 -> AES dispatcher
	 * flag = 1 -> VDI dispatcher
	 *
	 * return 0 on success
	 * or error number for a failure
	 */
	long _cdecl (*register_trap2)(long _cdecl (*dispatch)(void *), int mode, int flag, long extra);
};
#define DEFAULTS_kentry_module \
{ \
	load_modules, \
	register_trap2, \
}


/* cnf related
 */
struct kentry_cnf
{
	long _cdecl (*parse_cnf)(const char *path, struct parser_item *, void *, unsigned long);
	long _cdecl (*parse_include)(const char *path, struct parsinf *, struct parser_item *);
	void _cdecl (*parser_msg)(struct parsinf *, const char *msg);
};
#define DEFAULTS_kentry_cnf \
{ \
	parse_cnf, \
	parse_include, \
	parser_msg, \
}


/* miscellanous things
 */
struct kentry_misc
{
	/* easy to use DMA interface, see dma.c for more details */
	struct dma *dma;

	/* lookup cookies in original TOS cookiejar */
	long _cdecl (*get_toscookie)(unsigned long tag, unsigned long *val);

	long _cdecl (*add_rsvfentry)(char *, char, char);
	long _cdecl (*del_rsvfentry)(char *);

	/* return the number of milliseconds remaining to preemption time.
	 */
	unsigned long _cdecl (*remaining_proc_time)(void);

	/*
	 * TOS trap vectors
	 */
	long _cdecl (*trap_1_emu)(short fnum, ...);
	long _cdecl (*trap_13_emu)(short fnum, ...);
	long _cdecl (*trap_14_emu)(short fnum, ...);
};
#define DEFAULTS_kentry_misc \
{ \
	&dma, \
	\
	get_toscookie, \
	\
	add_rsvfentry, \
	del_rsvfentry, \
	\
	remaining_proc_time, \
	\
	trap_1_emu, \
	trap_13_emu, \
	trap_14_emu, \
}

/* debug support
 */
struct kentry_debug
{
	/* Debugging stuff
	 * ---------------
	 *
	 * trace - informational messages
	 * debug - error messages
	 * alert - really serious errors
	 * fatal - fatal errors
	 * force - always prints, even with debug level 0
	 */
	int *debug_level;
	void	_cdecl (*trace)(const char *, ...) __attribute__((format(printf, 1, 2)));
	void	_cdecl (*debug)(const char *, ...) __attribute__((format(printf, 1, 2)));
	void	_cdecl (*alert)(const char *, ...) __attribute__((format(printf, 1, 2)));
	EXITING	_cdecl (*fatal)(const char *, ...) NORETURN __attribute__((format(printf, 1, 2)));
	void	_cdecl (*force)(const char *, ...) __attribute__((format(printf, 1, 2)));
};
extern int debug_level;
#define DEFAULTS_kentry_debug \
{ \
	&debug_level, \
	Trace, \
	Debug, \
	ALERT, \
	FATAL, \
	FORCE, \
}


/* libkern exported functions
 */
struct kentry_libkern
{
	/*
	 * kernel character classification and conversion
	 */

	unsigned char *_ctype;

	int	_cdecl (*tolower)(int c);
	int	_cdecl (*toupper)(int c);

	/*
	 * kernel string functions
	 *
	 * vsprintf, sprintf: floating point formats are not supported!
	 */

	long	_cdecl (*vsprintf)(char *buf, long buflen, const char *fmt, va_list args) __attribute__((format(printf, 3, 0)));
	long	_cdecl (*sprintf)(char *dst, long buflen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

	char *	_cdecl (*getenv)(struct basepage *bp, const char *var);

	long	_cdecl (*atol)(const char *s);
	long	_cdecl (*strtonumber)(const char *name, long *result, int neg, int zerolead);

	long	_cdecl (*strlen)(const char *s);

	long	_cdecl (*strcmp)(const char *str1, const char *str2);
	long	_cdecl (*strncmp)(const char *str1, const char *str2, long len);

	long	_cdecl (*stricmp)(const char *str1, const char *str2);
	long	_cdecl (*strnicmp)(const char *str1, const char *str2, long len);

	char *	_cdecl (*strcpy)(char *dst, const char *src);
	char *	_cdecl (*strncpy)(char *dst, const char *src, long len);
	void	_cdecl (*strncpy_f)(char *dst, const char *src, long len);

	char *	_cdecl (*strlwr)(char *s);
	char *	_cdecl (*strupr)(char *s);

	char *	_cdecl (*strcat)(char *dst, const char *src);
	char *	_cdecl (*strchr)(const char *s, long charwanted);
	char *	_cdecl (*strrchr)(const char *str, long which);
	char *	_cdecl (*strrev)(char *s);

	char *	_cdecl (*strstr)(const char *s, const char *w);

	long	_cdecl (*strtol)(const char *nptr, char **endptr, long base);
	ulong	_cdecl (*strtoul)(const char *nptr, char **endptr, long base);

	void *	_cdecl (*memchr)(void *s, long search, unsigned long size);
	long	_cdecl (*memcmp)(const void *s1, const void *s2, unsigned long size);

	/*
	 * kernel time help functions
	 * --------------------------
	 *
	 * millis_time(ms, timeptr)
	 *
	 * convert "ms" milliseconds into a DOS time (in td[0]) and date
	 * (in td[1]) convert a DOS style time and date into a Unix style
	 * time; returns the Unix time
	 *
	 * unixtime(time, date)
	 *
	 * convert a DOS style time and date into a Unix style time; returns
	 * the Unix time
	 *
	 * dostime(time)
	 *
	 * convert a Unix time into a DOS time (in the high word of the
	 * returned value) and date (in the low word)
	 */

	void	_cdecl (*ms_time)(unsigned long ms, short *timeptr);
	void	_cdecl (*unix2calendar)(long tv_sec,
					unsigned short *year, unsigned short *month,
					unsigned short *day, unsigned short *hour,
					unsigned short *minute, unsigned short *second);
	long	_cdecl (*unix2xbios)(long tv_sec);
	long	_cdecl (*dostime)(long tv_sec);
	long	_cdecl (*unixtime)(unsigned short time, unsigned short date);

	/*
	 * kernel block functions
	 */

	void	_cdecl (*blockcpy)(char *dst, const char *src, unsigned long nblocks);
	void	_cdecl (*quickcpy)(void *dst, const void *src, unsigned long nbytes);
	void	_cdecl (*quickswap)(void *dst, void *src, unsigned long nbytes);
	void	_cdecl (*quickzero)(char *place, unsigned long size);

	void *	_cdecl (*memcpy)(void *dst, const void *src, unsigned long nbytes);
	void *	_cdecl (*memset)(void *dst, int ucharfill, unsigned long size);

	void	_cdecl (*bcopy)(const void *src, void *dst, unsigned long nbytes);
	void	_cdecl (*bzero)(void *dst, unsigned long size);
};
#define DEFAULTS_kentry_libkern \
{ \
	_mint_ctype, \
	_mint_tolower, \
	_mint_toupper, \
	kvsprintf, \
	ksprintf, \
	_mint_getenv, \
	_mint_atol, \
	strtonumber, \
	_mint_strlen, \
	_mint_strcmp, \
	_mint_strncmp, \
	_mint_stricmp, \
	_mint_strnicmp, \
	_mint_strcpy, \
	_mint_strncpy, \
	_mint_strncpy_f, \
	_mint_strlwr, \
	_mint_strupr, \
	_mint_strcat, \
	_mint_strchr, \
	_mint_strrchr, \
	_mint_strrev, \
	_mint_strstr, \
	_mint_strtol, \
	_mint_strtoul, \
	_mint_memchr, \
	_mint_memcmp, \
	\
	ms_time, \
	unix2calendar, \
	unix2xbios, \
	dostime, \
	unixtime, \
	\
	_mint_blockcpy, \
	_mint_quickcpy, \
	_mint_quickswap, \
	_mint_quickzero, \
	memcpy, \
	memset, \
	bcopy, \
	mint_bzero, \
}

struct kentry_xfs
{
	void _cdecl (*block)(FILESYS *fs, ushort dev, const char *func);
	void _cdecl (*deblock)(FILESYS *fs, ushort dev, const char *func);


	long _cdecl (*root)(FILESYS *fs, int drv, fcookie *fc);
	long _cdecl (*lookup)(FILESYS *fs, fcookie *dir, const char *name, fcookie *fc);

	DEVDRV * _cdecl (*getdev)(FILESYS *fs, fcookie *fc, long *devsp);

	long _cdecl (*getxattr)(FILESYS *fs, fcookie *fc, XATTR *xattr);

	long _cdecl (*chattr)(FILESYS *fs, fcookie *fc, int attr);
	long _cdecl (*chown)(FILESYS *fs, fcookie *fc, int uid, int gid);
	long _cdecl (*chmode)(FILESYS *fs, fcookie *fc, unsigned mode);

	long _cdecl (*mkdir)(FILESYS *fs, fcookie *dir, const char *name, unsigned mode);
	long _cdecl (*rmdir)(FILESYS *fs, fcookie *dir, const char *name);
	long _cdecl (*creat)(FILESYS *fs, fcookie *dir, const char *name, unsigned mode, int attr, fcookie *fc);
	long _cdecl (*remove)(FILESYS *fs, fcookie *dir, const char *name);
	long _cdecl (*getname)(FILESYS *fs, fcookie *root, fcookie *dir, char *buf, int len);
	long _cdecl (*rename)(FILESYS *fs, fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);

	long _cdecl (*opendir)(FILESYS *fs, DIR *dirh, int flags);
	long _cdecl (*readdir)(FILESYS *fs, DIR *dirh, char *nm, int nmlen, fcookie *fc);
	long _cdecl (*rewinddir)(FILESYS *fs, DIR *dirh);
	long _cdecl (*closedir)(FILESYS *fs, DIR *dirh);

	long _cdecl (*pathconf)(FILESYS *fs, fcookie *dir, int which);
	long _cdecl (*dfree)(FILESYS *fs, fcookie *dir, long *buf);
	long _cdecl (*writelabel)(FILESYS *fs, fcookie *dir, const char *name);
	long _cdecl (*readlabel)(FILESYS *fs, fcookie *dir, char *name, int namelen);

	long _cdecl (*symlink)(FILESYS *fs, fcookie *dir, const char *name, const char *to);
	long _cdecl (*readlink)(FILESYS *fs, fcookie *fc, char *buf, int len);
	long _cdecl (*hardlink)(FILESYS *fs, fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
	long _cdecl (*fscntl)(FILESYS *fs, fcookie *dir, const char *name, int cmd, long arg);
	long _cdecl (*dskchng)(FILESYS *fs, int drv, int mode);

	long _cdecl (*release)(FILESYS *fs, fcookie *fc);
	long _cdecl (*dupcookie)(FILESYS *fs, fcookie *dst, fcookie *src);

	long _cdecl (*mknod)(FILESYS *fs, fcookie *dir, const char *name, ulong mode);
	long _cdecl (*unmount)(FILESYS *fs, int drv);
	long _cdecl (*stat64)(FILESYS *fs, fcookie *fc, STAT *stat);
};

#define DEFAULTS_kentry_xfs	\
{	\
	xfs_block, \
	xfs_deblock, \
	\
	xfs_root, \
	xfs_lookup, \
	\
	xfs_getdev, \
	\
	xfs_getxattr, \
	\
	xfs_chattr, \
	xfs_chown, \
	xfs_chmode, \
	\
	xfs_mkdir, \
	xfs_rmdir, \
	xfs_creat, \
	xfs_remove, \
	xfs_getname, \
	xfs_rename, \
	\
	xfs_opendir, \
	xfs_readdir, \
	xfs_rewinddir, \
	xfs_closedir, \
	\
	xfs_pathconf, \
	xfs_dfree, \
	xfs_writelabel, \
	xfs_readlabel, \
	\
	xfs_symlink, \
	xfs_readlink, \
	xfs_hardlink, \
	xfs_fscntl, \
	xfs_dskchng, \
	\
	xfs_release, \
	xfs_dupcookie, \
	\
	xfs_mknod, \
	xfs_unmount, \
	xfs_stat64, \
}

struct kentry_xdd
{
	long _cdecl (*open)(FILEPTR *f);
	long _cdecl (*write)(FILEPTR *f, const char *buf, long bytes);
	long _cdecl (*read)(FILEPTR *f, char *buf, long bytes);
	long _cdecl (*lseek)(FILEPTR *f, long where, int whence);
	long _cdecl (*ioctl)(FILEPTR *f, int mode, void *buf);
	long _cdecl (*datime)(FILEPTR *f, ushort *timeptr, int rwflag);
	long _cdecl (*close)(FILEPTR *f, int pid);
};
#define DEFAULTS_kentry_xdd	\
{ \
	xdd_open, \
	xdd_write, \
	xdd_read, \
	xdd_lseek, \
	xdd_ioctl, \
	xdd_datime, \
	xdd_close, \
}

struct kentry_pcibios
{
	unsigned long *pcibios_installed;
	long _cdecl (*Find_pci_device)(unsigned long id, unsigned short index);
	long _cdecl (*Find_pci_classcode)(unsigned long class, unsigned short index);
	long _cdecl (*Read_config_byte)(long handle, unsigned short reg, unsigned char *address);
	long _cdecl (*Read_config_word)(long handle, unsigned short reg, unsigned short *address);
	long _cdecl (*Read_config_longword)(long handle, unsigned short reg, unsigned long *address);
	unsigned char _cdecl (*Fast_read_config_byte)(long handle, unsigned short reg);
	unsigned short _cdecl (*Fast_read_config_word)(long handle, unsigned short reg);
	unsigned long _cdecl (*Fast_read_config_longword)(long handle, unsigned short reg);
	long _cdecl (*Write_config_byte)(long handle, unsigned short reg, unsigned short val);
	long _cdecl (*Write_config_word)(long handle, unsigned short reg, unsigned short val);
	long _cdecl (*Write_config_longword)(long handle, unsigned short reg, unsigned long val);
	long _cdecl (*Hook_interrupt)(long handle, unsigned long *routine, unsigned long *parameter);
	long _cdecl (*Unhook_interrupt)(long handle);
	long _cdecl (*Special_cycle)(unsigned short bus, unsigned long data);
	long _cdecl (*Get_routing)(long handle);
	long _cdecl (*Set_interrupt)(long handle);
	long _cdecl (*Get_resource)(long handle);
	long _cdecl (*Get_card_used)(long handle, unsigned long *address);
	long _cdecl (*Set_card_used)(long handle, unsigned long *callback);
	long _cdecl (*Read_mem_byte)(long handle, unsigned long offset, unsigned char *address);
	long _cdecl (*Read_mem_word)(long handle, unsigned long offset, unsigned short *address);
	long _cdecl (*Read_mem_longword)(long handle, unsigned long offset, unsigned long *address);
	unsigned char _cdecl (*Fast_read_mem_byte)(long handle, unsigned long offset);
	unsigned short _cdecl (*Fast_read_mem_word)(long handle, unsigned long offset);
	unsigned long _cdecl (*Fast_read_mem_longword)(long handle, unsigned long offset);
	long _cdecl (*Write_mem_byte)(long handle, unsigned long offset, unsigned short val);
	long _cdecl (*Write_mem_word)(long handle, unsigned long offset, unsigned short val);
	long _cdecl (*Write_mem_longword)(long handle, unsigned long offset, unsigned long val);
	long _cdecl (*Read_io_byte)(long handle, unsigned long offset, unsigned char *address);
	long _cdecl (*Read_io_word)(long handle, unsigned long offset, unsigned short *address);
	long _cdecl (*Read_io_longword)(long handle, unsigned long offset, unsigned long *address);
	unsigned char _cdecl (*Fast_read_io_byte)(long handle, unsigned long offset);
	unsigned short _cdecl (*Fast_read_io_word)(long handle, unsigned long offset);
	unsigned long _cdecl (*Fast_read_io_longword)(long handle, unsigned long offset);
	long _cdecl (*Write_io_byte)(long handle, unsigned long offset, unsigned short val);
	long _cdecl (*Write_io_word)(long handle, unsigned long offset, unsigned short val);
	long _cdecl (*Write_io_longword)(long handle, unsigned long offset, unsigned long val);
	long _cdecl (*Get_machine_id)(void);
	long _cdecl (*Get_pagesize)(void);
	long _cdecl (*Virt_to_bus)(long handle, unsigned long address, struct pci_conv_adr *pointer);
	long _cdecl (*Bus_to_virt)(long handle, unsigned long address, struct pci_conv_adr *pointer);
	long _cdecl (*Virt_to_phys)(unsigned long address, struct pci_conv_adr *pointer);
	long _cdecl (*Phys_to_virt)(unsigned long address, struct pci_conv_adr *pointer);
};
#define DEFAULTS_kentry_pcibios \
{ \
	&pcibios_installed, \
	Find_pci_device, \
	Find_pci_classcode, \
	Read_config_byte, \
	Read_config_word, \
	Read_config_longword, \
	Fast_read_config_byte, \
	Fast_read_config_word, \
	Fast_read_config_longword, \
	Write_config_byte, \
	Write_config_word, \
	Write_config_longword, \
	Hook_interrupt, \
	Unhook_interrupt, \
	Special_cycle, \
	Get_routing, \
	Set_interrupt, \
	Get_resource, \
	Get_card_used, \
	Set_card_used, \
	Read_mem_byte, \
	Read_mem_word, \
	Read_mem_longword, \
	Fast_read_mem_byte, \
	Fast_read_mem_word, \
	Fast_read_mem_longword, \
	Write_mem_byte, \
	Write_mem_word, \
	Write_mem_longword, \
	Read_io_byte, \
	Read_io_word, \
	Read_io_longword, \
	Fast_read_io_byte, \
	Fast_read_io_word, \
	Fast_read_io_longword, \
	Write_io_byte, \
	Write_io_word, \
	Write_io_longword, \
	Get_machine_id, \
	Get_pagesize, \
	Virt_to_bus, \
	Bus_to_virt, \
	Virt_to_phys, \
	Phys_to_virt, \
}

struct kentry_xhdi
{
long _cdecl (*XHGetVersion)(void);
long _cdecl (*XHInqTarget)(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name);
long _cdecl (*XHReserve)(ushort major, ushort minor, ushort do_reserve, ushort key);
long _cdecl (*XHLock)(ushort major, ushort minor, ushort do_lock, ushort key);
long _cdecl (*XHStop)(ushort major, ushort minor, ushort do_stop, ushort key);
long _cdecl (*XHEject)(ushort major, ushort minor, ushort do_eject, ushort key);
long _cdecl (*XHDrvMap)(void);
long _cdecl (*XHInqDev)(ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, struct bpb *bpb);
long _cdecl (*XHInqDriver)(ushort bios_device, char *name, char *ver, char *company, ushort *ahdi_version, ushort *maxIPL);
long _cdecl (*XHNewCookie)(void *newcookie);
long _cdecl (*XHReadWrite)(ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf);
long _cdecl (*XHInqTarget2)(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen);
long _cdecl (*XHInqDev2)(ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, struct bpb *bpb, ulong *blocks, char *partid);
long _cdecl (*XHDriverSpecial)(ulong key1, ulong key2, ushort subopcode, void *data);
long _cdecl (*XHGetCapacity)(ushort major, ushort minor, ulong *blocks, ulong *bs);
long _cdecl (*XHMediumChanged)(ushort major, ushort minor);
long _cdecl (*XHMiNTInfo)(ushort opcode, struct kerinfo *data);
long _cdecl (*XHDOSLimits)(ushort which, ulong limit);
long _cdecl (*XHLastAccess)(ushort major, ushort minor, ulong *ms);
long _cdecl (*XHReaccess)(ushort major, ushort minor);
};
#define DEFAULTS_kentry_xhdi \
{ \
	XHGetVersion, \
	XHInqTarget, \
	XHReserve, \
	XHLock, \
	XHStop, \
	XHEject, \
	XHDrvMap, \
	XHInqDev, \
	XHInqDriver, \
	XHNewCookie, \
	XHReadWrite, \
	XHInqTarget2, \
	XHInqDev2, \
	XHDriverSpecial, \
	XHGetCapacity, \
	XHMediumChanged, \
	XHMiNTInfo, \
	XHDOSLimits, \
	XHLastAccess, \
	XHReaccess, \
}

struct kentry_scsidrv
{
long _cdecl (*scsidrv_In)(struct scsicmd *par);
long _cdecl (*scsidrv_Out)(struct scsicmd *par);
long _cdecl (*scsidrv_InquireSCSI)(short what, struct businfo *info);
long _cdecl (*scsidrv_InquireBus)(short what, short BusNo, struct devinfo *dev);
long _cdecl (*scsidrv_CheckDev)(short BusNo, const struct dlong *SCSIId, char *Name, ushort *Features);
long _cdecl (*scsidrv_RescanBus)(short BusNo);
long _cdecl (*scsidrv_Open)(short BusNo, const struct dlong *SCSIId, ulong *MaxLen);
long _cdecl (*scsidrv_Close)(short *handle);
long _cdecl (*scsidrv_Error)(short *handle, short rwflag, short ErrNo);
long _cdecl (*scsidrv_Install)(ushort bus, struct target *handler);
long _cdecl (*scsidrv_Deinstall)(ushort bus, struct target *handler);
long _cdecl (*scsidrv_GetCmd)(ushort bus, char *cmd);
long _cdecl (*scsidrv_SendData)(ushort bus, char *buf, ulong len);
long _cdecl (*scsidrv_GetData)(ushort bus, void *buf, ulong len);
long _cdecl (*scsidrv_SendStatus)(ushort bus, ushort status);
long _cdecl (*scsidrv_SendMsg)(ushort bus, ushort msg);
long _cdecl (*scsidrv_GetMsg)(ushort bus, ushort *msg);
long _cdecl (*scsidrv_InstallNewDriver)(struct scsidrv *newdrv);
};
#define DEFAULTS_kentry_scsidrv \
{ \
	scsidrv_In, \
	scsidrv_Out, \
	scsidrv_InquireSCSI, \
	scsidrv_InquireBus, \
	scsidrv_CheckDev, \
	scsidrv_RescanBus, \
	scsidrv_Open, \
	scsidrv_Close, \
	scsidrv_Error, \
	scsidrv_Install, \
	scsidrv_Deinstall, \
	scsidrv_GetCmd, \
	scsidrv_SendData, \
	scsidrv_GetData, \
	scsidrv_SendStatus, \
	scsidrv_SendMsg, \
	scsidrv_GetMsg, \
	scsidrv_InstallNewDriver, \
}

/* the complete kernel entry
 */
struct kentry
{
	unsigned char	major;		/* FreeMiNT major version */
	unsigned char	minor;		/* FreeMiNT minor version */
	unsigned char	patchlevel;	/* FreeMiNT patchlevel */
	unsigned char	beta;		/* FreeMiNT beta ident */

	unsigned char	version_major;	/* kentry major version */
	unsigned char	version_minor;	/* kentry minor version */
	unsigned short	status;		/* FreeMiNT status */

	unsigned long	dos_version;	/* running GEMDOS version */

	/* OS functions */
	dos_vecs *vec_dos;		/* DOS entry points */
	bios_vecs *vec_bios;	/* BIOS entry points */
	xbios_vecs *vec_xbios;	/* XBIOS entry points */

	/* kernel exported function vectors */
	struct kentry_mch vec_mch;
	struct kentry_proc vec_proc;
	struct kentry_mem vec_mem;
	struct kentry_fs vec_fs;
	struct kentry_sockets vec_sockets;
	struct kentry_module vec_module;
	struct kentry_cnf vec_cnf;
	struct kentry_misc vec_misc;
	struct kentry_debug vec_debug;
	struct kentry_libkern vec_libkern;

	struct kentry_xfs vec_xfs;
	struct kentry_xdd vec_xdd;

	struct kentry_pcibios vec_pcibios;
	struct kentry_xhdi vec_xhdi;
	struct kentry_scsidrv vec_scsidrv;
};
# define DEFAULTS_kentry \
{ \
	MINT_MAJ_VERSION, \
	MINT_MIN_VERSION, \
	MINT_PATCH_LEVEL, \
	MINT_STATUS_IDENT, \
	\
	KENTRY_MAJ_VERSION, \
	KENTRY_MIN_VERSION, \
	0, \
	\
	0x00000040, \
	\
	&dos_tab, \
	&bios_tab, \
	&xbios_tab, \
	\
	DEFAULTS_kentry_mch, \
	DEFAULTS_kentry_proc, \
	DEFAULTS_kentry_mem, \
	DEFAULTS_kentry_fs, \
	DEFAULTS_kentry_sockets, \
	DEFAULTS_kentry_module, \
	DEFAULTS_kentry_cnf, \
	DEFAULTS_kentry_misc, \
	DEFAULTS_kentry_debug, \
	DEFAULTS_kentry_libkern, \
	DEFAULTS_kentry_xfs, \
	DEFAULTS_kentry_xdd, \
	DEFAULTS_kentry_pcibios, \
	DEFAULTS_kentry_xhdi, \
	DEFAULTS_kentry_scsidrv, \
}

# endif /* _mint_kentry_h */
