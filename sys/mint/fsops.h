/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_fsops_h
# define _mint_fsops_h

# include "kcompiler.h"
# include "ktypes.h"


typedef long fs_ino_t;

struct fcookie
{
	FILESYS	*fs;		/* filesystem that knows about this cookie */
	ushort	dev;		/* device info (e.g. Rwabs device number) */
	ushort	aux;		/* extra data that the file system may want */
	fs_ino_t	index;		/* this+dev uniquely identifies a file */
};

/* structure for opendir/readdir/closedir */
struct dirstruct
{
	fcookie fc;		/* cookie for this directory */
	ushort	index;		/* index of the current entry */
	ushort	flags;		/* flags (e.g. tos or not) */
# define TOS_SEARCH	0x01
	char	fsstuff[60];	/* anything else the file system wants */
				/* NOTE: this must be at least 45 bytes */
	DIR	*next;		/* linked together so we can close them
				 * on process termination */
	short	fd;		/* associated fd, for use with dirfd */
};

struct devdrv
{
	long _cdecl (*open)	(FILEPTR *f);
	long _cdecl (*write)	(FILEPTR *f, const char *buf, long bytes);
	long _cdecl (*read)	(FILEPTR *f, char *buf, long bytes);
	long _cdecl (*lseek)	(FILEPTR *f, long where, int whence);
	long _cdecl (*ioctl)	(FILEPTR *f, int mode, void *buf);
	long _cdecl (*datime)	(FILEPTR *f, ushort *timeptr, int rwflag);
	long _cdecl (*close)	(FILEPTR *f, int pid);
	long _cdecl (*select)	(FILEPTR *f, long proc, int mode);
	void _cdecl (*unselect)	(FILEPTR *f, long proc, int mode);
	
	/* extensions, check dev_descr.drvsize (size of DEVDRV struct) before calling:
	 * fast RAW tty byte io
	 */
	long _cdecl (*writeb)	(FILEPTR *f, const char *buf, long bytes);
	long _cdecl (*readb)	(FILEPTR *f, char *buf, long bytes);
	
	/* what about: scatter/gather io for DMA devices...
	 * long _cdecl (*writev)(FILEPTR *f, const struct iovec *iov, long cnt);
	 * long _cdecl (*readv)	(FILEPTR *f, struct iovec *iov, long cnt);
	 */
};

/*
 * init function for device drivers. This is required.
 *
 * the init function is referenced by the linker command as init_xdd.
 * The make process does not know wether the compiler adds an
 * underscore, so enforce that here
 */
struct kerinfo;
struct devdrv * _cdecl		init_xdd		(struct kerinfo *k) __asm__("init_xdd");

struct filesys
{
	/* kernel data
	 */
	FILESYS	*next;			/* link to next file system on chain */
	long	fsflags;
# define FS_KNOPARSE		0x0001	/* kernel shouldn't do parsing */
# define FS_CASESENSITIVE	0x0002	/* file names are case sensitive */
# define FS_NOXBIT		0x0004	/* if a file can be read, it can be executed */
# define FS_LONGPATH		0x0008	/* file system understands "size" argument to "getname" */
# define FS_NO_C_CACHE		0x0010	/* don't cache cookies for this filesystem */
# define FS_DO_SYNC		0x0020	/* file system has a sync function */
# define FS_OWN_MEDIACHANGE	0x0040	/* filesystem control self media change (dskchng) */
# define FS_REENTRANT_L1	0x0080	/* fs is level 1 reentrant */
# define FS_REENTRANT_L2	0x0100	/* fs is level 2 reentrant */
# define FS_EXT_1		0x0200	/* extensions level 1 - mknod & unmount */
# define FS_EXT_2		0x0400	/* extensions level 2 - additional place at the end */
# define FS_EXT_3		0x0800	/* extensions level 3 - stat & native UTC timestamps */
	
	/* filesystem functions
	 */
	long	_cdecl (*root)		(int drv, fcookie *fc);
	long	_cdecl (*lookup)	(fcookie *dir, const char *name, fcookie *fc);
	long	_cdecl (*creat)		(fcookie *dir, const char *name, unsigned mode, int attrib, fcookie *fc);
	DEVDRV*	_cdecl (*getdev)	(fcookie *file, long *devspecial);
	long	_cdecl (*getxattr)	(fcookie *file, XATTR *xattr);
	long	_cdecl (*chattr)	(fcookie *file, int attr);
	long	_cdecl (*chown)		(fcookie *file, int uid, int gid);
	long	_cdecl (*chmode)	(fcookie *file, unsigned mode);
	long	_cdecl (*mkdir)		(fcookie *dir, const char *name, unsigned mode);
	long	_cdecl (*rmdir)		(fcookie *dir, const char *name);
	long	_cdecl (*remove)	(fcookie *dir, const char *name);
	long	_cdecl (*getname)	(fcookie *relto, fcookie *dir, char *pathname, int size);
	long	_cdecl (*rename)	(fcookie *olddir, char *oldname, fcookie *newdir, const char *newname);
	long	_cdecl (*opendir)	(DIR *dirh, int tosflag);
	long	_cdecl (*readdir)	(DIR *dirh, char *name, int namelen, fcookie *fc);
	long	_cdecl (*rewinddir)	(DIR *dirh);
	long	_cdecl (*closedir)	(DIR *dirh);
	long	_cdecl (*pathconf)	(fcookie *dir, int which);
	long	_cdecl (*dfree)		(fcookie *dir, long *buf);
	long	_cdecl (*writelabel)	(fcookie *dir, const char *name);
	long	_cdecl (*readlabel)	(fcookie *dir, char *name, int namelen);
	long	_cdecl (*symlink)	(fcookie *dir, const char *name, const char *to);
	long	_cdecl (*readlink)	(fcookie *dir, char *buf, int len);
	long	_cdecl (*hardlink)	(fcookie *fromdir, const char *fromname, fcookie *todir, const char *toname);
	long	_cdecl (*fscntl)	(fcookie *dir, const char *name, int cmd, long arg);
	long	_cdecl (*dskchng)	(int drv, int mode);
	long	_cdecl (*release)	(fcookie *);
	long	_cdecl (*dupcookie)	(fcookie *new, fcookie *old);
	long	_cdecl (*sync)		(void);
	long	_cdecl (*mknod)		(fcookie *dir, const char *name, ulong mode);
	long	_cdecl (*unmount)	(int drv);
	long	_cdecl (*stat64)	(fcookie *file, STAT *stat);
	
	long	res1, res2, res3;	/* reserved */
	
	/* experimental extension
	 */
	ulong	lock;			/* for non-blocking DMA */
	ulong	sleepers;		/* sleepers on this filesystem */
	void	_cdecl (*block)		(FILESYS *fs, ushort dev, const char *);
	void	_cdecl (*deblock)	(FILESYS *fs, ushort dev, const char *);
};

/*
 * init function for filesystem drivers. This is required.
 *
 * the init function is referenced by the linker command as init_xfs.
 * The make process does not know wether the compiler adds an
 * underscore, so enforce that here
 */
FILESYS * _cdecl init_xfs(struct kerinfo *k) __asm__("init_xfs");

/* various character constants and defines for TTY's */
# define MiNTEOF	0x0000ff1a	/* 1a == ^Z */

/* defines for tty_read */
# define RAW		0
# define COOKED		0x1
# define NOECHO		0
# define ECHO		0x2
# define ESCSEQ		0x04		/* cursor keys, etc. get escape sequences */


/* terminal control constants (tty.sg_flags)
 */
# define T_CRMOD	0x0001
# define T_CBREAK	0x0002
# define T_ECHO		0x0004
# define T_XTABS_notyet	0x0008		/* unimplemented */
# define T_RAW		0x0010
# define T_LCASE_notyet	0x0020		/* unimplemented */

# define T_NOFLSH	0x0040		/* don't flush buffer when signals
					   are received */
# define T_TOS		0x0080
# define T_TOSTOP	0x0100
# define T_XKEY		0x0200		/* Fread returns escape sequences for
					   cursor keys, etc. */
# define T_ECHOCTL	0x0400		/* echo ctl chars as ^x */
/* 0x0800 still available */

# define T_TANDEM	0x1000
# define T_RTSCTS	0x2000
# define T_EVENP	0x4000		/* EVENP and ODDP are mutually exclusive */
# define T_ODDP		0x8000

# define TF_FLAGS	0xF000

/* some flags for TIOC[GS]FLAGS */
# define TF_BRKINT	0x0080		/* allow breaks interrupt (like ^C) */
# define TF_CAR		0x0800		/* nonlocal mode, require carrier */
# define TF_NLOCAL	TF_CAR

# define TF_STOPBITS	0x0003
# define TF_1STOP	 0x001
# define TF_15STOP	 0x002
# define TF_2STOP	 0x003

# define TF_CHARBITS	0x000C
# define TF_8BIT	 0x000
# define TF_7BIT	 0x004
# define TF_6BIT	 0x008
# define TF_5BIT	 0x00C

/* the following are terminal status flags (tty.state) */
/* (the low byte of tty.state indicates a part of an escape sequence still
 * hasn't been read by Fread, and is an index into that escape sequence)
 */
# define TS_ESC		0x00ff
# define TS_BLIND	0x0800		/* tty is `blind' i.e. has no carrier
					   (cleared in local mode) */
# define TS_HOLD	0x1000		/* hold (e.g. ^S/^Q) */
# define TS_HPCL	0x4000		/* hang up on close */
# define TS_COOKED	0x8000		/* interpret control chars */

/* structures for terminals
 */
struct tchars
{
	char	t_intrc;
	char	t_quitc;
	char	t_startc;
	char	t_stopc;
	char	t_eofc;
	char	t_brkc;
};

struct ltchars
{
	char	t_suspc;
	char	t_dsuspc;
	char	t_rprntc;
	char	t_flushc;
	char	t_werasc;
	char	t_lnextc;
};

struct sgttyb
{
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	ushort	sg_flags;
};

struct winsize
{
	short	ws_row;
	short	ws_col;
	short	ws_xpixel;
	short	ws_ypixel;
};

struct xkey
{
	short	xk_num;
	char	xk_def[8];
};

struct tty
{
	short		pgrp;		/* process group of terminal */
	short		state;		/* terminal status, e.g. stopped */
	short		use_cnt;	/* number of times terminal is open */
	short		aux_cnt;	/* number of times terminal is open as /dev/aux */
	struct sgttyb 	sg;
	struct tchars 	tc;
	struct ltchars 	ltc;
	struct winsize	wsiz;
	long		rsel;		/* selecting process for read */
	long		wsel;		/* selecting process for write */
	char		*xkey;		/* extended keyboard table */
	long		hup_ospeed;	/* saved ospeed while hanging up */
	ushort		vmin, vtime;	/* min chars, timeout for RAW reads */
	long		resrvd[1];	/* for future expansion */
};


/* number of BIOS drives */
# define NDRIVES		32
# define NUM_DRIVES		NDRIVES

# define BIOSDRV		(NUM_DRIVES)
# define PIPEDRV		(BIOSDRV + 1)
# define PROCDRV		(PIPEDRV + 1)
# ifndef NO_RAMFS
# define RAM_DRV		(PROCDRV + 1)
# define SHM_DRV		(RAM_DRV + 1)
# else
# define SHM_DRV		(PROCDRV + 1)
# endif
# define KERNDRV		(SHM_DRV + WITH_KERNFS)
# define UNI_NUM_DRVS		(KERNDRV + 1)

# define UNIDRV			('U' - 'A')
# define PSEUDODRVS		((1L << UNIDRV))

/* various fields for the "rdev" device numbers */
# define BIOS_DRIVE_RDEV 	0x0000
# define BIOS_RDEV		0x0100
# define FAKE_RDEV		0x0200
# define PIPE_RDEV		0x7e00
# define UNK_RDEV		0x7f00
# define PROC_RDEV_BASE		0xa000


# endif /* _mint_fsops_h */
