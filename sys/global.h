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

# ifndef _global_h
# define _global_h

# include "mint/mint.h"


# define DEFAULT_DIRMODE	(0777)

# if 0
# define DEFAULT_MODE		(0666)
# else
# define DEFAULT_MODE		(kernelinfo.default_perm)
# endif

extern struct kerinfo kernelinfo;

extern ulong c20ms;


/* global exported variables
 */

extern long mch;		/* machine */
extern long mcpu;		/* cpu type */
extern long fputype;		/* fpu type */
extern short fpu;		/* flag if fpu is present */
extern int tosvers;		/* the underlying TOS version */
extern short falcontos;
extern int secure_mode;
extern int screen_boundary;
extern int flk;
extern int FalconVideo;
extern short ste_video;

# define MAXLANG 6	/* languages supported */
extern int gl_lang;

# ifdef OLDTOSFS
extern long gemdos_version;
# endif

# ifdef VM_EXTENSION
extern int vm_in_use;
extern ulong st_left_after_vm;
# endif


typedef struct kbdvbase KBDVEC;
struct kbdvbase
{
	long	midivec;
	long	vkbderr;
	long	vmiderr;
	long	statvec;
	long	mousevec;
	long	clockvec;
	long	joyvec;
	long	midisys;
	long	ikbdsys;
	short	drvstat;	/* Non-zero if a packet is currently transmitted. */
};

extern KBDVEC *syskey;


# endif /* _global_h */
