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
# define ROUNDUP2(a,b)		(((a) + (b) - 1) & ~((b) - 1))		/* if b is power of two */
# define ROUNDDOWN2(a,b)	((a) & (~((b) - 1)))				/* if b is power of two */

# define ALIGN(x,a)		__ALIGN_MASK((x),(typeof(x))(a) - 1)
# define __ALIGN_MASK(x,mask)	(((x) + (mask)) & ~(mask))

/*
 * This macro is taken from U-Boot - The Universal Boot Loader.
 * (c) Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * The ALLOC_CACHE_ALIGN_BUFFER macro is used to allocate a buffer on the
 * stack that meets the minimum architecture alignment requirements for DMA.
 * Such a buffer is useful for DMA operations where flushing and invalidating
 * the cache before and after a read and/or write operation is required for
 * correct operations.
 *
 * When called the macro creates an array on the stack that is sized such
 * that:
 *
 * 1) The beginning of the array can be advanced enough to be aligned.
 *
 * 2) The size of the aligned portion of the array is a multiple of the minimum
 *    architecture alignment required for DMA.
 *
 * 3) The aligned portion contains enough space for the original number of
 *    elements requested.
 *
 * The macro then creates a pointer to the aligned portion of this array and
 * assigns to the pointer the address of the first element in the aligned
 * portion of the array.
 *
 * Calling the macro as:
 *
 *     ALLOC_CACHE_ALIGN_BUFFER(uint32_t, buffer, 1024);
 *
 * Will result in something similar to saying:
 *
 *     uint32_t    buffer[1024];
 *
 * The following differences exist:
 *
 * 1) The resulting buffer is guaranteed to be aligned to the value of
 *    ARCH_DMA_MINALIGN.
 *
 * 2) The buffer variable created by the macro is a pointer to the specified
 *    type, and NOT an array of the specified type.  This can be very important
 *    if you want the address of the buffer, which you probably do, to pass it
 *    to the DMA hardware.  The value of &buffer is different in the two cases.
 *    In the macro case it will be the address of the pointer, not the address
 *    of the space reserved for the buffer.  However, in the second case it
 *    would be the address of the buffer.  So if you are replacing hard coded
 *    stack buffers with this macro you need to make sure you remove the & from
 *    the locations where you are taking the address of the buffer.
 *
 * Note that the size parameter is the number of array elements to allocate,
 * not the number of bytes.
 *
 * This macro can not be used outside of function scope, or for the creation
 * of a function scoped static buffer.  It can not be used to create a cache
 * line aligned global buffer.
 */

#define M68K_CACHE_LINE_SIZE	16

#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)		\
	char __##name[ROUNDUP2(PAD_SIZE((size) * sizeof(type), pad), align)  \
		      + (align - 1)];					\
									\
	type *name = (type *) ALIGN((unsigned long int)__##name, align)
#define ALLOC_ALIGN_BUFFER(type, name, size, align)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, 1)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)		\
	ALLOC_ALIGN_BUFFER_PAD(type, name, size, M68K_CACHE_LINE_SIZE, pad)
#define ALLOC_CACHE_ALIGN_BUFFER(type, name, size)			\
	ALLOC_ALIGN_BUFFER(type, name, size, M68K_CACHE_LINE_SIZE)

# endif /* _mint_misc_h */
