/*
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

# ifndef _mint_m68k_mmu_h
# define _mint_m68k_mmu_h

# if !defined(M68040) && !defined(M68060)

/* macro for turning a curproc->base_table pointer into a 16-byte boundary */
# define ROUND16(ld)	((long_desc *)(((ulong)(ld) + 15) & ~15))

/* TBL_SIZE is the size in entries of the A, B, and C level tables */
# define TBL_SIZE	(16)
# define TBL_SIZE_BYTES	(TBL_SIZE * sizeof (long_desc))

typedef struct {
	short limit;
	unsigned zeros:14;
	unsigned dt:2;
	struct long_desc *tbl_address;
} crp_reg;

/* format of long descriptors, both page descriptors and table descriptors */

typedef struct {
	unsigned limit;		/* set to $7fff to disable */
	unsigned unused1:6;
	unsigned unused2:1;
	unsigned s:1;		/* 1 grants supervisor access only */
	unsigned unused3:1;
	unsigned ci:1;		/* cache inhibit: used in page desc only */
	unsigned unused4:1;
	unsigned m:1;		/* modified: used in page desc only */
	unsigned u:1;		/* accessed */
	unsigned wp:1;		/* write-protected */
	unsigned dt:2;		/* type */
} page_type;

typedef struct long_desc {
	page_type page_type;
	struct long_desc *tbl_address;
} long_desc;

typedef struct {
	unsigned enable:1;
	unsigned zeros:5;
	unsigned sre:1;
	unsigned fcl:1;
	unsigned ps:4;
	unsigned is:4;
	unsigned tia:4;
	unsigned tib:4;
	unsigned tic:4;
	unsigned tid:4;
} tc_reg;

# else /* !(M68040 || M68060) */

/* macro for turning a curproc->base_table pointer into a 512-byte boundary */
# define ROUND512(ld)	((ulong *)(((ulong)(ld) + 511) & ~511))

typedef ulong crp_reg[2];
typedef ulong tc_reg;

# endif /* !(M68040 || M68060) */


# endif /* _mint_m68k_mmu_h */
