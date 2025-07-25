/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_kerinfo_h
# define _mint_kerinfo_h

# ifndef __KERNEL__
# error not a KERNEL source
# endif

# include "kcompiler.h"
# include "ktypes.h"
# include "block_IO.h"		/* eXtended kernelinterface */
# include "biosvecs.h"
# include "dosvecs.h"

struct basepage;
struct nf_ops;

#define MOD_LOADED	1

#define MODCLASS_XIF	1
#define MODCLASS_XDD	2
#define MODCLASS_XFS	3
#define MODCLASS_KM	4
#define MODCLASS_KMDEF	5


/* kerinfo - kernel interface table
 * --------------------------------
 *
 * All functions should be called using the GCC calling conventions:
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
 * ushort, ulong - unsigned version of short, long
 *
 */

struct kerinfo
{
	ushort	maj_version;	/* kernel version number */
	ushort	min_version;	/* minor kernel version number */
	ushort	default_perm;	/* default file permissions */
	ushort	version;	/* version number */


	/* OS functions
	 * ------------
	 * NOTE: these tables are definitely READ ONLY!!!!
	 */
	bios_vecs	*bios_tab;	/* pointer to the BIOS entry points */
	dos_vecs	*dos_tab;	/* pointer to the GEMDOS entry points */


	/* media change vector
	 * -------------------
	 * call this if a device driver detects a disk
	 * change during a read or write operation. The parameter is the BIOS
	 * device number of the disk that changed.
	 */
	void	_cdecl (*drvchng)(ushort);

	/* Debugging stuff
	 * ---------------
	 *
	 * trace - informational messages
	 * debug - error messages
	 * alert - really serious errors
	 * fatal - fatal errors
	 */
	void	_cdecl (*trace)(const char *, ...) __attribute__((format(printf, 1, 2)));
	void	_cdecl (*debug)(const char *, ...) __attribute__((format(printf, 1, 2)));
	void	_cdecl (*alert)(const char *, ...) __attribute__((format(printf, 1, 2)));
	EXITING _cdecl (*fatal)(const char *, ...) NORETURN __attribute__((format(printf, 1, 2)));


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
	 */
	void *	_cdecl (*kmalloc)(ulong);
	void	_cdecl (*kfree)(void *);
	void *	_cdecl (*umalloc)(ulong);
	void	_cdecl (*ufree)(void *);


	/* utility functions for string manipulation
	 * -----------------------------------------
	 *
	 * like the normal library functions
	 *
	 * sprintf:
	 * --------
	 * kernel sprintf, like library sprintf but not so heavy; floating
	 * point formats are not supported!
	 * Also this sprintf will put at most SPRINTF_MAX characters into
	 * the output string.
	 */
	int	_cdecl (*kstrnicmp)(const char *, const char *, int);
	int	_cdecl (*kstricmp)(const char *, const char *);
	char *	_cdecl (*kstrlwr)(char *);
	char *	_cdecl (*kstrupr)(char *);
	int	_cdecl (*ksprintf)(char *, const char *, ...);


	/* utility functions for manipulating time
	 * ---------------------------------------
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
	void	_cdecl (*millis_time)(ulong, short *);
	long	_cdecl (*unixtime)(ushort, ushort);
	long	_cdecl (*dostime)(long);


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
	void	_cdecl (*nap)(unsigned);
	int	_cdecl (*sleep)(int que, long cond);
	void	_cdecl (*wake)(int que, long cond);
	void	_cdecl (*wakeselect)(long param);


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
	int	_cdecl (*denyshare)(FILEPTR *, FILEPTR *);
	LOCK *	_cdecl (*denylock)(ushort pid, LOCK *, LOCK *);


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
	TIMEOUT * _cdecl (*addtimeout)(long, void _cdecl (*)(PROC *, long arg));
	void	_cdecl (*canceltimeout)(TIMEOUT *);
	TIMEOUT * _cdecl (*addroottimeout)(long, void _cdecl (*)(PROC *, long arg), ushort);
	void	_cdecl (*cancelroottimeout)(TIMEOUT *);


	/* miscellaneous other things
	 * --------------------------
	 *
	 * interrupt safe kill and wake
	 *
	 * ikill(p, sig)
	 * iwake(que, cond, pid)
	 */
	long	_cdecl (*ikill)(int, ushort);
	void	_cdecl (*iwake)(int, long, short);

	/* 1.15 extensions */
	BIO	*bio;		/* buffered block I/O, see block_IO.doc */

	/* version 1 extension */
	struct timeval	*xtime;		/* pointer to current kernel time - UTC */

	long	res;

	/* version 2 extension
	 * pointers are valid if != NULL
	 */

	long	_cdecl (*add_rsvfentry)(char *, char, char);
	long	_cdecl (*del_rsvfentry)(char *);
	long	_cdecl (*killgroup)(int, ushort, int);

	/* easy to use DMA interface, see dma.c for more details */
	DMA	*dma;

	/* for udelay timing loop */
	ulong	*loops_per_sec;

	/* lookup cookies in original TOS cookiejar */
	long	_cdecl (*get_toscookie)(ulong tag, ulong *val);

	/* XXX temporary until new kernel interface */
	void	_cdecl (*so_register)(short, struct dom_ops *);
	void	_cdecl (*so_unregister)(short);
	long	_cdecl (*so_release)(struct socket *);
	void	_cdecl (*so_sockpair)(struct socket *, struct socket *);
	long	_cdecl (*so_connect)(struct socket *, struct socket *, short, short, short);
	long	_cdecl (*so_accept)(struct socket *, struct socket *, short);
	long	_cdecl (*so_create)(struct socket **, short, short, short);
	long	_cdecl (*so_dup)(struct socket **, struct socket *);
	void	_cdecl (*so_free)(struct socket *);

	/* load safely additional modules */
	void	_cdecl (*load_modules)(const char *extension, long (*loader)(struct basepage *, const char *)); //, short *, short *));

	/* fork/leave a kernel thread */
	long	_cdecl (*kthread_create)(void (*func)(void *), void *arg, struct proc **np, const char *fmt, ...);
	void	_cdecl (*kthread_exit)(short code);

	/* allocate DMA buffer (from ST-RAM with cache mode set to cmode
	 */
	void *	_cdecl (*dmabuf_alloc)(ulong size, short cmode);

	/* nf operation vector if available
	 * on this machine; NULL otherwise
	 */
	struct nf_ops *nf_ops;

	/* return the number of milliseconds remaining to preemption time.
	 */
	ulong	_cdecl	(*remaining_proc_time)(void);

	/* reserved, set to 0 */
	long	res2 [1];
};


struct dev_descr
{
	DEVDRV	*driver;
	short	dinfo;
	short	flags;
	struct tty *tty;
	long	drvsize;		/* size of DEVDRV struct */
	long	fmode;
	BDEVMAP	*bdevmap;
	int	bdev;
	int	reserved;
};


struct fs_descr
{
	FILESYS	*file_system;
	short	dev_no;    	/* this is filled in by MiNT if arg == FS_MOUNT */
	long	flags;
	long	reserved[4];
};


# endif /* _mint_kerinfo_h */
