/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_ktypes_h
# define _mint_ktypes_h

# ifndef __KERNEL__
# error __KERNEL__ not defined!
# endif

# include "kcompiler.h"


typedef enum { false = (0 == 1), true  = (1 == 1) } bool;

typedef unsigned char			uchar;
typedef unsigned short			ushort;
typedef unsigned long 			ulong;

/* 64 bit support */
# ifndef LLONG
# define LLONG
typedef long long			llong;
typedef unsigned long long		ullong;
# endif


/* signed basic types */
typedef signed char			__s8;
typedef signed short			__s16;
typedef signed long			__s32;
# ifdef __GNUC__
typedef signed long long		__s64;
# else
# error signed long long		__s64;
# endif

/* unsigned basic types */
typedef unsigned char			__u8;
typedef unsigned short			__u16;
typedef unsigned long			__u32;
# ifdef __GNUC__
typedef unsigned long long		__u64;
# else
# error unsigned long long		__u64;
# endif


typedef long _cdecl (*Func)();

/* forward declarations: sockets
 */
struct dom_ops;
struct iovec;
struct module_callback;
struct proc_ext;
struct socket;

/* forward declarations: file.h
 */
typedef struct fcookie		fcookie;
typedef struct timeval		TIMEVAL;
typedef struct dtabuf		DTABUF;
typedef struct dirstruct	DIR;
typedef struct xattr		XATTR;
typedef struct stat		STAT;
typedef struct file		FILEPTR;
typedef struct ilock		LOCK;
typedef struct filesys		FILESYS;
typedef struct devdrv		DEVDRV;	
typedef struct tty		TTY;

/* forward declarations: proc.h
 */
struct procwakeup;
typedef struct context		CONTEXT;
typedef struct timeout		TIMEOUT;
typedef struct proc		PROC;

/* forward declarations: misc
 */
typedef struct bf_key		BF_KEY;
typedef struct bdevmap		BDEVMAP;
typedef struct dma		DMA;
typedef struct pollfd		POLLFD;
typedef struct sizebuf		SIZEBUF;	/* sized buffer */


/* BIOS device map */
struct bdevmap
{
	long _cdecl (*instat)	(int dev);
	long _cdecl (*in)	(int dev);
	long _cdecl (*outstat)	(int dev);
	long _cdecl (*out)	(int dev, int);
	long _cdecl (*rsconf)	(int dev, int, int, int, int, int, int);
};

/* easy to use DMA transfer interface */
struct dma
{
	ulong	_cdecl (*get_channel)	(void);
	long	_cdecl (*free_channel)	(ulong);
	void	_cdecl (*dma_start)	(ulong);
	void	_cdecl (*dma_end)	(ulong);
	void *	_cdecl (*block)		(ulong, ulong, void _cdecl (*)(PROC *p));
	void	_cdecl (*deblock)	(ulong, void *);
};

# include "poll.h"

struct sizebuf
{
	ulong	len;
	char	buf [0];
};


# endif /* _mint_ktypes_h */
