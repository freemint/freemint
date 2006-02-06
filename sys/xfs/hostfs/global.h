/*
 * $Id$
 *
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
#include "libkern/libkern.h"

/* get the version number */
#include "hostfs_nfapi.h"

/*
 * debugging stuff
 */
#ifdef DEV_DEBUG
#  define DEBUG(x)      KERNEL_ALERT x
#  define TRACE(x)      KERNEL_ALERT x
#else
#  define DEBUG(x)      KERNEL_DEBUG x
#  define TRACE(x)      KERNEL_TRACE x
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
    "� 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.\r\n" \
    "� 2000 by Chris Felsch <C.Felsch@gmx.de>\r\n"\
    "� " MSG_BUILDDATE " by ARAnyM Team\r\n\r\n"

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

extern struct kerinfo *KERNEL;


#endif /* _global_h_ */

