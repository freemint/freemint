/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# ifndef _mint_mem_h
# define _mint_mem_h

# include "ktypes.h"
# include "file.h"


typedef struct memregion MEMREGION;

/** @struct memregion
 * Description of a memory region.
 */
struct memregion
{
        unsigned long	loc;		///< Base of memory region.
        unsigned long	len;		///< Length of memory region.
        long		links;		///< Number of users of region.
        unsigned short	mflags;		///< E.g. which map this came from.
	short		reservd;
        MEMREGION *save;	///< Used to save inactive shadows.
        MEMREGION *shadow;	///< Ring of shadows or 0.
        MEMREGION *next;	///< Next region in memory map.
};

# define M_CORE		0x0001	///< Region came from core map.
# define M_ALT		0x0002	///< Region came from alt map.
# define M_SWAP		0x0004	///< Region came from swap map.
# define M_KER		0x0008	///< Region came from kernel map.
# define M_MAP		0x000f	///< AND with this to pick out map
/* obsolete M_SHTEXT	0x0010	 * region is a shared text region */
/* obsolete M_SHTEXT_T	0x0020	 * `sticky bit' for shared text regions */
# define M_FSAVED	0x0040	///< Region is saved memory of a forked process
# define M_SHARED	0x0080	///< Region is shared memory region
# define M_KEEP		0x0100	///< don't free region on process termination
                     /* 0x0200  unused */
                     /* 0x0400  unused */
                     /* 0x0800  unused */
# define M_UMALLOC	0x1000	///< Region used by umalloc
# define M_KMALLOC	0x4000	///< Region used by kmalloc
# define M_SEEN		0x8000	///< flag for memused() to find links

/*
 * Here's the deal with memory bits:
 *
 * Mxalloc(long size, int mode) takes these fields in 'mode': BITS 0-2 hold
 * values 0-3 for the old GEMDOS mode argument.  If bit 3 is on, then only
 * F_PROTMODE (bits 4-7) counts, and it encodes the desired protection mode
 * to change a block to. Else F_PROTMODE is the desired protection mode for
 * the new block you're allocating.  In either case, F_PROTMODE turns into
 * a PROT_* thusly: if it's zero, you get the F_PROTMODE value from curproc's
 * prgflags. Else you get (F_PROTMODE >> F_PROTSHIFT)-1.
 *
 * The 0x4000 bit is carried along into get_region (but not from there into
 * mark_region) and, if set, causes M_KEEP to be set in the region's
 * mflags.
 */

/** @struct fileheader
 * Structure of exectuable program headers.
 */
struct fileheader
{
	short	fmagic;
	long	ftext;
	long	fdata;
	long	fbss;
	long	fsym;
	long	reserved;
	long	flag;
	short	reloc;
};

typedef struct fileheader FILEHEAD;

# define GEMDOS_MAGIC 0x601a

/* flags for curproc->memflags */
/* also used for program headers PRGFLAGS */
# define F_FASTLOAD	0x01		/* don't zero heap */
# define F_ALTLOAD	0x02		/* OK to load in alternate ram */
# define F_ALTALLOC	0x04		/* OK to malloc from alt. ram */
# define F_SMALLTPA	0x08		/* used in MagiC: TPA can be allocated
					 * as specified in the program header
					 * rather than the biggest free memory
					 * block */
# define F_MEMFLAGS	0xf0		/* reserved for future use */
/* obsolete F_SHTEXT	0x800		 * program's text may be shared */

# define F_MINALT	0xf0000000L	/* used to decide which type of RAM to load in */

# define F_ALLOCZERO	0x2000		/* zero mem, for bugged (GEM...) programs */

/* Bit in Mxalloc's arg for "don't auto-free this memory" */
# define F_KEEP		0x4000

# define F_OS_SPECIAL	0x8000		/* mark as a special process */

# define F_SINGLE_TASK	0x0010000		/* XaAES: if set (in p_flags) it's "single-task-mode" */
# define F_DONT_STOP	0x0020000		/* XaAES: if set do not stop when entering single-task-mode */

#define F_XALLOCMODE 0x03
#define F_STONLY     0
#define F_ALTONLY    1
#define F_STPREF     2
#define F_ALTPREF    (F_ALTONLY|F_STPREF)

/* flags for curproc->memflags (that is, PRGFLAGS) and also Mxalloc mode.  */
/* (Actually, when users call Mxalloc, they add 0x10 to what you see here) */
# define F_PROTMODE	0xf0		/* protection mode bits */
# define F_PROT_P	0x00		/* no read or write */
# define F_PROT_G	0x10		/* any access OK */
# define F_PROT_S	0x20		/* any super access OK */
# define F_PROT_PR	0x30		/* any read OK, no write */
# define F_PROT_I	0x40		/* invalid page */

# define F_PROTSHIFT	4

/* actual values found in page_mode_table and used as args to alloc_region */
# define PROT_P		0
# define PROT_G		1
# define PROT_S		2
# define PROT_PR	3
# define PROT_I		4
# define PROT_MAX_MODE	4
# define PROT_PROTMODE	0xf   /* these bits are the prot mode */
# define PROT_NOCHANGE	-1

/* cache modi */
# define CM_NOCHANGE		0
# define CM_NOCACHE		1
# define CM_SERIALIZED		2
# define CM_WRITETHROUGH	3
# define CM_COPYBACK		4
# define CM_MAX_MODE		4

typedef MEMREGION **MMAP;

# ifndef M68000

/* QUANTUM: the page size for the mmu: 8K.  This is hard-coded elsewhere. */
# define QUANTUM	0x2000L

/* MiNT leaves this much memory for TOS to use (8K)
 */
# define TOS_MEM	(QUANTUM)

/* MiNT tries to keep this much memory available for the kernel and other
 * programs when a program is launched (8K)
 */
# define KEEP_MEM	(QUANTUM)

/*
 * how much memory should be allocated to the kernel? (24K)
 */
# define KERNEL_MEM	(3 * QUANTUM)

# else

/* For 68000 machines (always short of RAM) we may try to decrease
 * the page size. This should save some memory per process.
 */
#if 1
#if 0
# define QUANTUM	0x0800L
# define TOS_MEM	(QUANTUM*4)
# define KEEP_MEM	(QUANTUM*4)
# define KERNEL_MEM	(12 * QUANTUM)
#else
# define QUANTUM	0x0200L
# define TOS_MEM	(QUANTUM*8*2)
# define KEEP_MEM	(QUANTUM*8*2)
# define KERNEL_MEM	(24 * QUANTUM *2)
#endif
#else
# define QUANTUM	0x2000L

/* MiNT leaves this much memory for TOS to use (8K)
 */
# define TOS_MEM	(QUANTUM)

/* MiNT tries to keep this much memory available for the kernel and other
 * programs when a program is launched (8K)
 */
# define KEEP_MEM	(QUANTUM)

/*
 * how much memory should be allocated to the kernel? (24K)
 */
# define KERNEL_MEM	(3 * QUANTUM)

#endif

# endif

/* macro for rounding off region sizes to QUANTUM (page) boundaries */
/* there is code in mem.c that assumes it's OK to put the screen
 * in any region, so this should be at least 256 for STs (16 is OK for
 * STes, TTs, and Falcons). We actually set a variable in main.c
 * that holds the screen boundary stuff.
 */
# define MASKBITS	(QUANTUM - 1)
# define ROUND(size)	(((size) + MASKBITS) & ~MASKBITS)

/* interesting memory constants */

# define ONE_K		0x400L
# define EIGHT_K	(8L * ONE_K)
# define ONE_MEG	(ONE_K * ONE_K)
# define SIXTEEN_MEG	(16L * ONE_K * ONE_K)
# define LOG2_ONE_MEG	20
# define LOG2_16_MEG	24
# define LOG2_EIGHT_K	13


/* Constants for mem_prot_flags bitvector */
# define MPF_STRICT  1


# endif /* _mint_mem_h */
