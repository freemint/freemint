/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The u:\host filesystem - global definitions.
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team.
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
#include "mint/kerinfo.h"
#include "libkern/libkern.h"

/*
 * debugging stuff
 */

#if __KERNEL__ != 1
# if 0
# define FS_DEBUG      1
# endif

# ifdef FS_DEBUG

# define ALERT(x)       KERNEL_ALERT x
# define DEBUG(x)       KERNEL_DEBUG x
# define TRACE(x)       KERNEL_TRACE x
# define ASSERT(x)      assert x

# else

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

#include "dosdir.h"
#include "kmemory.h"
#include "k_prot.h"
#include "filesys.h"
#include "dos.h"
#include "dosmem.h"
#include "proc.h"
#define p_getuid() sys_pgetuid()
#define p_geteuid() sys_pgeteuid()
#define p_getgid() sys_pgetgid()
#define p_getegid() sys_pgetegid()
#define p_getpid() sys_p_getpid()
#define m_alloc(size) sys_m_alloc(size)
#define ALERT(x) ALERT x

#endif

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	1
# define VER_STATUS


#ifdef ALPHA
# define VER_ALPHABETA   "\0340"
#elif defined(BETA)
# define VER_ALPHABETA   "\0341"
#else
# define VER_ALPHABETA
#endif

#define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) VER_ALPHABETA
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT       \
    "\033p ARAnyM u:\\host filesystem driver version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
    "½ " MSG_BUILDDATE " by Standa Opichal\r\n"

# define MSG_ALPHA      \
    "\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA       \
    "\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_OLDMINT	\
    "\033pMiNT to old, this xfs requires at least FreeMiNT 1.16!\033q\r\n"

#define MSG_FAILURE(s)	\
    "\7Sorry, aranym.xfs NOT installed: " s "!\r\n\r\n"

#define MSG_PFAILURE(p,s) \
    "\7Sorry, aranym.xfs NOT installed: " s "!\r\n"


extern long __CDECL (*nf_call)(long id, ...);


#endif /* _global_h_ */

