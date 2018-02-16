/*
 * The Host OS filesystem access driver - global definitions.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa of ARAnyM dev team.
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Originally taken from the STonX CVS repository.
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
 */

#ifndef _global_h_
#define _global_h_


#include "mint/mint.h"
#include "mint/dcntl.h"
#include "mint/file.h"
#include "mint/stat.h"
#include "mint/proc.h"
#include "libkern/libkern.h"

/* get the version number */
#include "dosdir.h"
#include "hostfs_nfapi.h"

/*
 * debugging stuff
 */

#if __KERNEL__ != 1
# if 0
# define HOSTFS_DEBUG      1
# endif

# ifdef HOSTFS_DEBUG

# define FORCE(x)
# define ALERT(x)       KERNEL_ALERT x
# define DEBUG(x)       KERNEL_DEBUG x
# define TRACE(x)       KERNEL_TRACE x
# define ASSERT(x)      assert x

# else

# define FORCE(x)
# define ALERT(x)       KERNEL_ALERT x
# define DEBUG(x)
# define TRACE(x)
# define ASSERT(x)      assert x

# endif

#else
#define d_cntl sys_d_cntl

#include "init.h"
#define c_conws boot_print

#include "kerinfo.h"
#define KERNEL (&kernelinfo)
#endif


#ifdef ALPHA
# define VER_ALPHABETA   "\0340"
#elif defined(BETA)
# define VER_ALPHABETA   "\0341"
#else
# define VER_ALPHABETA
#endif

#define MSG_VERSION	str (HOSTFS_XFS_VERSION) "." str (HOSTFS_NFAPI_VERSION) VER_ALPHABETA
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT       \
    "\033p HostFS Filesystem driver version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
    "½ " MSG_BUILDDATE " by ARAnyM Team\r\n"

# define MSG_ALPHA      \
    "\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA       \
    "\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_OLDMINT	\
    "\033pMiNT to old, this xfs requires at least FreeMiNT 1.15!\033q\r\n"

#define MSG_OLDKERINFO     \
    "\033pMiNT very old, this xfs wants at least FreeMiNT 1.15 with kerinfo version 2\033q\r\n"

#define MSG_FAILURE(s)	\
    "\7Sorry, hostfs.xfs NOT installed: " s "!\r\n\r\n"

#define MSG_PFAILURE(p,s) \
    "\7Sorry, hostfs.xfs NOT installed: " s "!\r\n"


#endif /* _global_h_ */

