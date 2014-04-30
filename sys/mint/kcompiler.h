/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-04-18
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _mint_misc_h
# define _mint_misc_h

# ifndef __KERNEL__
# error __KERNEL__ not defined!
# endif

# include <compiler.h>

# ifndef USED
# if __GNUC__ >= 4
# define USED __attribute__((used))
# else
# define USED
# endif
# else
# error "USED already defined"
# endif

# ifndef EXITING
# define EXITING	__EXITING
# endif

# ifndef NORETURN
# define NORETURN	__NORETURN
# endif

# ifndef _cdecl
# define _cdecl		__CDECL
# endif


# ifndef NULL
# define NULL		__NULL
# endif


/* Forces a function to be always inlined.  */
#if __GNUC_PREREQ (3,2)
# define INLINE static __inline __attribute__ ((__always_inline__))
#else
# define INLINE static __inline
#endif

/* define to indicate unused variables: */
# define UNUSED(x)	(void) x


# ifndef TRUE
# define TRUE	1
# define FALSE	0
# endif


/*
 * configuration modes
 */

# ifdef DISABLE
# undef DISABLE
# endif

# ifdef ENABLE
# undef ENABLE
# endif

# define ASK		-1
# define DISABLE	 0
# define ENABLE		 1


/*
 * useful makros
 */

# define MAX(a,b)	(((a) > (b)) ? (a) : (b))
# define MIN(a,b)	(((a) > (b)) ? (b) : (a))

# define MAX3(a,b,c)	((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
# define MIN3(a,b,c)	((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

# define ABS(val)	(((val) < 0) ? -val : val)

# define ROUNDUP(a,b)	((((a) + ((b) - 1)) / (b)) * (b))
# define ROUNDDOWN(a,b)	(((a) / (b)) * (b))

# endif /* _mint_misc_h */
