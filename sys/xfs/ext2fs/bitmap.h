/*
 * Filename:     bitmap.h
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@fishpool.com).
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@cs.uni-magdeburg.de)
 * 
 * Portions copyright 1992, 1993, 1994, 1995 Remy Card (card@masi.ibp.fr)
 * and 1991, 1992 Linus Torvalds (torvalds@klaava.helsinki.fi)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _bitmap_h
# define _bitmap_h

# include "global.h"


ulong	ext2_count_free	(char *map, ulong numchars);


/* Bitmap functions for the ext2 filesystem. */

# ifdef __mc68020__

INLINE long
ext2_set_bit (long nr, void *vaddr)
{
	char retval;
	
	__asm__ __volatile__
	(
		"bfset %2@{%1,#1}; sne %0"
		: "=d" (retval)
		: "d" (nr^7), "a" (vaddr)
	);
	
	return (long) retval;
}

INLINE long
ext2_clear_bit (long nr, void *vaddr)
{
	char retval;
	
	__asm__ __volatile__
	(
		"bfclr %2@{%1,#1}; sne %0"
		: "=d" (retval)
		: "d" (nr^7), "a" (vaddr)
	);
	
	return (long) retval;
}

INLINE long
ext2_test_bit (long nr, const void *addr)
{
	char retval;
	
	__asm__ __volatile__
	(
		"bftst %2@{%1:#1}; sne %0"
		: "=d" (retval)
		: "d" (nr^7), "a" (addr)
	);
	
	return (long) retval;
}

# else

INLINE long
ext2_set_bit (long nr, void *addr)
{
	long mask, retval;
	uchar *ADDR = (uchar *) addr;
	
	ADDR += nr >> 3;
	mask = 1UL << (nr & 0x07);
	retval = (mask & *ADDR) != 0;
	*ADDR |= mask;
	
	return retval;
}

INLINE long
ext2_clear_bit (long nr, void *addr)
{
	long mask, retval;
	uchar *ADDR = (uchar *) addr;
	
	ADDR += nr >> 3;
	mask = 1UL << (nr & 0x07);
	retval = (mask & *ADDR) != 0;
	*ADDR &= ~mask;
	
	return retval;
}

INLINE long
ext2_test_bit (long nr, const void *addr)
{
	long mask;
	const uchar *ADDR = (const uchar *) addr;
	
	ADDR += nr >> 3;
	mask = 1UL << (nr & 0x07);
	
	return ((mask & *ADDR) != 0);
}

# endif


INLINE long
ext2_find_first_zero_bit (const void *vaddr, ulong size)
{
	const ulong long *p = vaddr, *addr = vaddr;
	long res;
	
	if (!size)
		return 0;
	
	size = (size >> 5) + ((size & 31UL) > 0);
	while (*p++ == ~0UL)
	{
		if (--size == 0)
			return (p - addr) << 5;
	}
	
	--p;
	for (res = 0; res < 32L; res++)
		if (!ext2_test_bit (res, p))
			break;
	
	return (p - addr) * 32L + res;
}

INLINE long
ext2_find_next_zero_bit (const void *vaddr, ulong size, ulong offset)
{
	const ulong long *addr = vaddr;
	const ulong long *p = addr + (offset >> 5);
	long bit = offset & 31L, res;
	
	if (offset >= size)
		return size;
	
	if (bit)
	{
		/* Look for zero in first longword */
		for (res = bit; res < 32L; res++)
			if (!ext2_test_bit (res, p))
				return (p - addr) * 32L + res;
		p++;
	}
	
	/* No zero yet, search remaining full bytes for a zero */
	res = ext2_find_first_zero_bit (p, size - 32UL * (p - addr));
	
	return (p - addr) * 32L + res;
}


# endif /* _bitmap_h */
