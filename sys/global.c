/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * begin:	1998-07
 * last change: 1998-07-02
 * 
 * Author: Frank Naumann - <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *  
 */

# include "global.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "block_IO.h"		/* bio */
# include "cookie.h"		/* add_rsvfentry, del_rsvfentry, get_toscookie */
# include "delay.h"		/* loops_per_sec */
# include "dma.h"		/* dma */
# include "filesys.h"		/* changedrv, denyshare, denylock */
# include "ipc_socketutil.h"	/* so_* */
# include "kmemory.h"		/* kmalloc, kfree, umalloc, ufree */
# include "proc.h"		/* sleep, wake, wakeselect, iwake */
# include "signal.h"		/* ikill */
# include "syscall_vectors.h"	/* bios_tab, dos_tab */
# include "time.h"		/* xtime */
# include "timeout.h"		/* nap, addtimeout, canceltimeout, addroottimeout, cancelroottimeout */


# undef DEFAULT_MODE
# define DEFAULT_MODE	(0666)

/*
 * kernel info that is passed to loaded file systems and device drivers
 */

struct kerinfo kernelinfo =
{
	MAJ_VERSION, MIN_VERSION,
	DEFAULT_MODE,
	2,
	bios_tab, dos_tab,
	changedrv,
	Trace, Debug, ALERT, FATAL,
	_kmalloc,
	_kfree,
	_umalloc,
	_ufree,
	_mint_o_strnicmp,
	_mint_o_stricmp,
	_mint_strlwr,
	_mint_strupr,
	ksprintf_old,
	ms_time, unixtime, dostime,
	nap, sleep, wake, (void _cdecl (*)(long)) wakeselect,
	denyshare, denylock,
	addtimeout_curproc, canceltimeout,
	addroottimeout, cancelroottimeout,
	ikill, iwake,
	&bio,
	&xtime,		/* version 1 extension */
	0,
	
	/* version 2
	 */
	
	add_rsvfentry,
	del_rsvfentry,
	killgroup,
	&dma,
	&loops_per_sec,
	get_toscookie,
	
	so_register,
	so_unregister,
	so_release,
	so_sockpair,
	so_connect,
	so_accept,
	so_create,
	so_dup,
	so_free,
	
	{
		0, 0, 0, 0, 0, 0, 0
	}
};

ulong c20ms = 0;


long mch = 0;		/* machine we are are running */
long mcpu;		/* processor we are running */
long fputype = 0;	/* value for cookie jar */
/*
 * AGK: for proper co-processors we must consider saving their context.
 * This variable when non-zero indicates that the BIOS considers a true
 * coprocessor to be present. We use this variable in the context switch
 * code to decide whether to attempt an FPU context save.
 */
short fpu = 0;		/* flag */

int tosvers;		/* version of TOS we're running over */

/* flag for Falcon TOS kludges: TRUE if TOS 4.00 - 4.04
 * what about TOS 4.50 in Milan? see syscall.spp
 */
short falcontos;

/*
 * if this variable is set, then we have "secure mode" enabled; that
 * means only the superuser may access some functions those may be critical
 * for system security.
 *
 */

int secure_mode = 0;

/*
 * "screen_boundary+1" tells us how screens must be positioned
 * (to a 256 byte boundary on STs, a 16 byte boundary on other
 * machines; actually, 16 bytes is conservative, 4 is probably
 * OK, but it doesn't hurt to be cautious). The +1 is because
 * we're using this as a mask in the ROUND() macro in mem.h.
 */
int screen_boundary = 255;

/*
 * variable set if someone has already installed an flk cookie
 */
int flk = 0;

/*
 * variable set to 1 if the _VDO cookie indicates Falcon style video
 */
int FalconVideo = 0;

/*
 * variable set to 1 if the _VDO cookie indicates STE style video
 */
short ste_video = 0;

/*
 * variable holds language preference
 */
int gl_lang = -1;

/* version of GEMDOS in ROM.
 * dos.c no longer directly calls Sversion()
 */
# ifdef OLDTOSFS
long gemdos_version;
# endif

# if 0

# include "mfp.h"

struct mch
{
	/* hardware dependant vector
	 */
	
	long	cpu;	/* our actual cpu type */
	short	fpu;	/* our actual fpu type */
	short	vdo;	/* video system */
	long	mch;	/* the machine we running */
	char *	mname;	/* ASCII version of mch */
	
	short	lang;	/* language preference */
	short	res;	/* reserved */
	
	ulong	c20ms;	/* linear 20 ms counter */
	
	/* the (important) MFP base address */
	MFP *	_mfpbase;
	
	/* interrupt vector */
	/* Func	intr [256]; */
	
	/* CPU dependant functions */
	void	_cdecl (*cpush)(const void *base, long size);
};

struct mch mch_ =
{
};


struct prc
{
	/* utility functions for dealing with pauses, or for putting processes
	 * to sleep
	 */
	void	_cdecl (*nap)(unsigned);
	int	_cdecl (*sleep)(int que, long cond);
	void	_cdecl (*wake)(int que, long cond);
	void	_cdecl (*wakeselect)(long param);
	
	/* miscellaneous other things */
	long	_cdecl (*ikill)(int, int);
	void	_cdecl (*iwake)(int que, long cond, short pid);
	
	/* functions for adding/cancelling timeouts */
	TIMEOUT * _cdecl (*addtimeout)(long, void (*)());
	void	_cdecl (*canceltimeout)(TIMEOUT *);
	TIMEOUT * _cdecl (*addroottimeout)(long, void (*)(), ushort);
	void	_cdecl (*cancelroottimeout)(TIMEOUT *);
};

struct prc prc =
{
	nap,
	sleep,
	wake,
	(void _cdecl (*)(long)) wakeselect,
	ikill,
	iwake,
	addtimeout,
	canceltimeout,
	addroottimeout,
	cancelroottimeout
};


struct mem
{
	/* memory vector
	 */
	
	void *	_cdecl (*kcore)		(ulong size);	/* ST-RAM alloc */
	void *	_cdecl (*kmalloc)	(ulong size);	/* TT-RAM alloc */
	void	_cdecl (*kfree)		(void *place);	/* memory free  */
};

struct mem mem =
{
	_kcore,
	_kmalloc,
	_kfree,
};


struct xfs
{
	/* extended filesystem vector
	 */
	
	ushort	default_perm;		/* default file permissions */
	
	/* media change vector */
	void	_cdecl (*drvchng)(ushort);
	
	/* file system utility functions */
	int	_cdecl (*denyshare)(FILEPTR *, FILEPTR *);
	LOCK *	_cdecl (*denylock)(LOCK *, LOCK *);
	
	/* 1.15 extensions */
	BIO *	bio;			/* buffered block I/O */
};

struct xfs xfs =
{
	DEFAULT_MODE,
	changedrv,
	denyshare,
	denylock,
	& bio
};


struct str
{
	/* string manipulation vector
	 */
	
	long	_cdecl (*sprintf)(char *dst, const char *fmt, ...);
	long	_cdecl (*atol)(const char *s);
	
	uchar *	_ctype;				/* character type table */
	int	_cdecl (*tolower)(int c);	/* */
	int	_cdecl (*toupper)(int c);	/* */
	
	long	_cdecl (*strlen)(const char *s);
	
	long	_cdecl (*strcmp)(const char *str1, const char *str2);
	long	_cdecl (*strncmp)(const char *str1, const char *str2, long len);
	
	long	_cdecl (*stricmp)(const char *str1, const char *str2);
	long	_cdecl (*strnicmp)(const char *str1, const char *str2, long len);
	
	char *	_cdecl (*strcpy)(char *dst, const char *src);
	char *	_cdecl (*strncpy)(char *dst, const char *src, long len);
	
	char *	_cdecl (*strlwr)(char *s);
	char *	_cdecl (*strupr)(char *s);
	
	char *	_cdecl (*strcat)(char *dst, const char *src);
	char *	_cdecl (*strrchr)(const char *str, long which);
	char *	_cdecl (*strrev)(char *s);
	
	/* unicode functions
	 */
	char	_cdecl (*uni2atari)(uchar off, uchar cp);
	void	_cdecl (*atari2uni)(uchar atari_st, uchar *dst);
};

struct str str =
{
	ksprintf,
	_mint_atol,
	_mint_ctype,
	_mint_tolower,
	_mint_toupper,
	_mint_strlen,
	_mint_strcmp,
	_mint_strncmp,
	_mint_stricmp,
	_mint_strnicmp,
	_mint_strcpy,
	_mint_strncpy,
	_mint_strlwr,
	_mint_strupr,
	_mint_strcat,
	_mint_strrchr,
	_mint_strrev,
	NULL, /*_mint_uni2atari,*/
	NULL  /*_mint_atari2uni */
};


struct hlp
{
	/* help function vector
	 */
	
	void	_cdecl (*bcopy)(void *dst, const void *src, long size);	/* quickmovb */
	void	_cdecl (*fcopy)(void *dst, void *src, long size);	/* quickmove */
	void	_cdecl (*fswap)(void *dst, void *src, long size);	/* quickswap */
	void	_cdecl (*fzero)(char *place, long blocks);		/* quickzero */
	
	void	_cdecl (*ms_time)(ulong ms, short *timeptr);		/* util.h */
	long	_cdecl (*unixtime)(ushort time, ushort date);		/* util.h */
	long	_cdecl (*dostime)(long time);				/* util.h */
};

struct hlp hlp =
{
	quickmovb,
	quickmove,
	quickswap,
	quickzero,
	ms_time,
	unixtime,
	dostime
};


struct deb
{
	/* Debugging stuff
	 */
	
	void	_cdecl (*trace)(const char *, ...);
	void	_cdecl (*debug)(const char *, ...);
	void	_cdecl (*alert)(const char *, ...);
	EXITING	_cdecl (*fatal)(const char *, ...) NORETURN;
};

struct deb deb =
{
	Trace,
	Debug,
	ALERT,
	FATAL
};


struct kernel
{
	ushort		major;		/* FreeMiNT major version */
	ushort		minor;		/* FreeMiNT minor version */
	ushort		patchlevel;	/* FreeMiNT patchlevel */
	ushort		version;	/* kernel struct version */
	ulong		dos_version;	/* running GEMDOS version */
	ulong		status;		/* FreeMiNT status */
	
	/* OS functions */
	Func		*dos_vec;	/* DOS entry points */
	Func		*bios_vec;	/* BIOS entry points */
	Func		*xbios_vec;	/* XBIOS entry points */
	
	/* */
	struct mch	*mch_vec;
	struct prc	*prc_vec;
	struct mem	*mem_vec;
	struct xfs	*xfs_vec;
	struct str	*str_vec;
	struct hlp	*hlp_vec;
	struct deb	*deb_vec;
	
	/* future expansion */
	char		reserved [40];
};

struct kernel kernel =
{
	MAJ_VERSION,
	MIN_VERSION,
	PATCH_LEVEL,
	0,
	0x00000040,
	0,
	
	dos_tab,
	bios_tab,
	xbios_tab,
	
	& mch_,
	& prc,
	& mem,
	& xfs,
	& str,
	& hlp,
	& deb
};

# endif
