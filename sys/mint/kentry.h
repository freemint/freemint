/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _mint_kentry_h
# define _mint_kentry_h

# ifndef __KERNEL__
# error not a KERNEL source
# endif

# include <stdarg.h>

# include "kcompiler.h"
# include "ktypes.h"

/* forward declarations */
struct basepage;
struct bio;
struct dma;
struct file;
struct ilock;
struct mfp;
struct nf_ops;
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
 * (2) registers d0-d2 and a0-a2 are scratch registers and may be modified
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
#define KENTRY_VERSION_MAJOR	0
#define KENTRY_VERSION_MINOR	0


/* hardware dependant vector
 */
struct kentry_mch
{
	long		cpu;	/* our actual cpu type */
	short		fpu;	/* our actual fpu type */
	short		vdo;	/* video system */
	long		mch;	/* the machine we running */

	short		lang;	/* language preference */
	short		res;	/* reserved */

	unsigned long	c20ms;	/* linear 20 ms counter */

	/* the (important) MFP base address */
	struct mfp	*_mfpbase;

	/* interrupt vector */
	/* Func	intr [256]; */

	/* CPU dependant functions */
	void	_cdecl	(*cpush)(const void *base, long size);
};
#define DEFAULTS_kentry_mch \
{ \
	0, \
	0, \
	0, \
	0, \
	\
	0, \
	0, \
	\
	/* c20ms */ 0, \
	\
	/* _mfpbase */ NULL, \
	\
	cpush, \
}


/* process related functions
 */
struct kentry_proc
{
	/* utility functions for dealing with pauses, or for putting processes
	 * to sleep
	 */
	void	_cdecl	(*nap)(unsigned);
	int	_cdecl	(*sleep)(int que, long cond);
	void	_cdecl	(*wake)(int que, long cond);
	void	_cdecl	(*wakeselect)(struct proc *p);

	/* miscellaneous other things */
	long	_cdecl	(*ikill)(int, unsigned short);
	void	_cdecl	(*iwake)(int que, long cond, short pid);
	long	_cdecl	(*killgroup)(int, unsigned short, int);

	/* functions for adding/cancelling timeouts */
	struct timeout *_cdecl (*addtimeout)(long, void (*)());
	void _cdecl (*canceltimeout)(struct timeout *);
	struct timeout *_cdecl (*addroottimeout)(long, void (*)(), unsigned short);
	void _cdecl (*cancelroottimeout)(struct timeout *);

	/* fork/leave a kernel thread */
	long	_cdecl (*kthread_create)(void (*func)(void *), void *arg, struct proc **np, const char *fmt, ...);
	void	_cdecl (*kthread_exit)(short code);
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
	\
	addtimeout_curproc, \
	canceltimeout, \
	addroottimeout, \
	cancelroottimeout, \
	\
	kthread_create, \
	kthread_exit, \
}


/* memory management related
 */
struct kentry_mem
{
	/* memory vector
	 */

	void *	_cdecl (*kcore)(unsigned long size, const char *func);
	void *	_cdecl (*kmalloc)(unsigned long size, const char *func);
	void	_cdecl (*kfree)(void *place, const char *func);
	
	void *	_cdecl (*dmabuf_alloc)(unsigned long size, short cmode, const char *func);

	void *	_cdecl (*umalloc)(unsigned long size, const char *func);
	void	_cdecl (*ufree)(void *place, const char *func);
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
}


/* filesystem related
 */
struct kentry_fs
{
	/* extended filesystem vector
	 */

	unsigned short default_perm;	/* default file permissions */

	/* media change vector */
	void	_cdecl (*drvchng)(unsigned short drv, const char *function);

	/* file system utility functions */
	int	_cdecl (*denyshare)(struct file *, struct file *);
	LOCK *	_cdecl (*denylock)(struct ilock *, struct ilock *);

	struct bio *bio;		/* buffered block I/O */
	struct timeval *xtime;		/* pointer to current kernel time - UTC */
};
#define DEFAULTS_kentry_fs \
{ \
	DEFAULT_MODE, \
	changedrv, \
	denyshare, \
	denylock, \
	&bio, \
	&xtime, \
}


/* socket related
 */
struct kentry_sockets
{
	void	_cdecl (*so_register)(short, struct dom_ops *);
	void	_cdecl (*so_unregister)(short);
	long	_cdecl (*so_release)(struct socket *);
	void	_cdecl (*so_sockpair)(struct socket *, struct socket *);
	long	_cdecl (*so_connect)(struct socket *, struct socket *, short, short, short);
	long	_cdecl (*so_accept)(struct socket *, struct socket *, short);
	long	_cdecl (*so_create)(struct socket **, short, short, short);
	long	_cdecl (*so_dup)(struct socket **, struct socket *);
	void	_cdecl (*so_free)(struct socket *);
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
struct kentry_module
{
	void	_cdecl (*load_modules)(const char *extension, long (*loader)(struct basepage *, const char *));
};
#define DEFAULTS_kentry_module \
{ \
	load_modules, \
}


/* miscellanous things
 */
struct kentry_misc
{
	/* easy to use DMA interface, see dma.c for more details */
	struct dma *dma;

	/* for udelay timing loop */
	unsigned long *loops_per_sec;

	/* lookup cookies in original TOS cookiejar */
	long	_cdecl (*get_toscookie)(unsigned long tag, unsigned long *val);

	long	_cdecl (*add_rsvfentry)(char *, char, char);
	long	_cdecl (*del_rsvfentry)(char *);

	ulong	_cdecl (*remaining_proc_time)(void);

	/* XXX -> kentry_arch or something like this */
	struct nf_ops *nf_ops;
};
#define DEFAULTS_kentry_misc \
{ \
	&dma, \
	&loops_per_sec, \
	\
	get_toscookie, \
	\
	add_rsvfentry, \
	del_rsvfentry, \
	\
	remaining_proc_time, \
	\
	NULL, \
}


/* debug support
 */
struct kentry_debug
{
	/* Debugging stuff
	 */

	void	_cdecl (*trace)(const char *, ...);
	void	_cdecl (*debug)(const char *, ...);
	void	_cdecl (*alert)(const char *, ...);
	EXITING	_cdecl (*fatal)(const char *, ...) NORETURN;
};
#define DEFAULTS_kentry_debug \
{ \
	Trace, \
	Debug, \
	ALERT, \
	FATAL, \
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
	 */

	long	_cdecl (*vsprintf)(char *buf, long buflen, const char *fmt, va_list args);
	long	_cdecl (*sprintf)(char *dst, long buflen, const char *fmt, ...);

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
	vsprintf, \
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
	bzero, \
}


/* the complete kernel entry
 */
struct kentry
{
	unsigned short	major;		/* FreeMiNT major version */
	unsigned short	minor;		/* FreeMiNT minor version */
	unsigned short	patchlevel;	/* FreeMiNT patchlevel */
	unsigned char	version_major;	/* kentry major version */
	unsigned char	version_minor;	/* kentry minor version */
	unsigned long	dos_version;	/* running GEMDOS version */
	unsigned long	status;		/* FreeMiNT status */

	/* OS functions */
	Func *dos_vec;			/* DOS entry points */
	Func *bios_vec;			/* BIOS entry points */
	Func *xbios_vec;		/* XBIOS entry points */

	/* kernel exported function vectors */
	struct kentry_mch vec_mch;
	struct kentry_proc vec_proc;
	struct kentry_mem vec_mem;
	struct kentry_fs vec_fs;
	struct kentry_sockets vec_sockets;
	struct kentry_module vec_module;
	struct kentry_misc vec_misc;
	struct kentry_debug vec_debug;
	struct kentry_libkern vec_libkern;
};


# endif /* _mint_kentry_h */
