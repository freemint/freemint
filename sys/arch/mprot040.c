/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * mprot040.c dated 00/02/15
 *
 * Author:
 * Thomas Binder
 * (gryf@hrzpub.tu-darmstadt.de)
 *
 * Purpose:
 * Memory protection routines (mainly MMU-tree handling stuff) for MC68040
 * (and MC68060, though I can't test this myself). EXPERIMENTAL! USE WITH
 * CAUTION!
 *
 * History:
 * 00/03/18: - mark_proc_region() will now restore the global mode of the
 *             region when detaching it, see comment there for details (Gryf)
 *           - mem_prot_special() only marks pages as "super" that are still
 *             "private" (Gryf)
 * 00/02/15: - If not already set, walk_page() will enable supervisor
 *             protection for the first logical page, i.e. address 0 (Gryf)
 * 99/11/23: - Changes in report_buserr() to handle the new rw-Flag set by
 *             mmu_sigbus() in intr.spp (Gryf)
 * 99/08/17: - If the size for the template tree is smaller than MAX_TEMPLATE,
 *             then four additional template trees will be generated, each of
 *             which represents one protection mode (PROT_P/PROT_I, PROT_G,
 *             PROT_S and PROT_PR). The function mark_pages() will then just
 *             copy from one of these templates, instead of masking and setting
 *             bits for each descriptor, thus making it a lot faster. (Gryf)
 * 99/07/26: - The global template tree now represents the protection found
 *             in the global mode table. This drastically speeds up
 *             init_page_table(), as it's no longer necessary to mark each page
 *             of memory according to the global mode table here. (Gryf)
 *           - Added checks in mark_region() to ensure that memory protection
 *             is completely set up before stuff is actually marked. (Gryf)
 * 99/07/25: - Removed some additions of pagesize in loops which were no longer
 *             necessary (in mark_pages(), this was even a bug, as the start
 *             address for the following cpush() got wrong) (Gryf)
 * 99/07/22: - Fixed bug in calculation of TT-megabytes (if there were e.g.
 *             48.5 MB available, init_tables() would have calculated a total
 *             of 48 MB TT-RAM, thus allocating too little memory for the
 *             global_mode_table, which in turn could lead to overflows in
 *             mark_region()) (Gryf)
 *           - Reversed the order of the history, so that the most recent
 *             changes appear first (Gryf)
 * 99/06/07: - Removed debug output that accidentally made it into the
 *             1.15.1-release (Gryf)
 * 99/04/28: - The linearity check now effectively starts at membot, as the
 *             AfterBurner's toolkit moves the area with TOS's global variables
 *             to its Fast RAM (this affects parts of the area below membot).
 * 99/04/24-
 * 99/04/25: - walk_or_copy_tree() only copies descriptors up to mint_top_st
 *             and from 0x01000000 to mint_top_tt (Gryf)
 *           - Adapted init_page_table() to respect the changes made to
 *             walk_or_copy_tree() (Gryf)
 * 99/04/14: - Now uses the new function read_phys in walk_or_copy_tree() (and
 *             all its helper functions) (Gryf)
 * 99/03/31: - Added MMU tree dumping in BIG_MEM_DUMP (Gryf)
 * 99/03/21-
 * 99/03/24: - Creation, based on original memprot.c (Gryf)
 */

/*
 * Unfortunately, the MMU found in the MC68040 (and, with minor differences,
 * in the MC68060) is quite different from the one in the MC68030, so it made
 * no sense trying to clutter the original memprot.c with countless #ifdefs.
 *
 * What is nice, though, is the fact that the MMU handling in the MC68040 is
 * quite straight-forward, the layout of the translation tree is not near as
 * flexible (or complex, if you want), which makes things easier.
 *
 * As there are quite different machines with a 040 (or higher) out there
 * (Falcons with Afterburner, Hades, Milan) which all have a different layout
 * of their physical memory, it's not possible to create the MMU tree for
 * memory protection from scratch; instead, the one found on startup has to
 * be copied and modified as needed. The code here, however, assumes that the
 * logical memory layout is as on Falcon and TT: ST-RAM begins at address 0
 * and ends at phystop, TT-RAM begins at 0x01000000 and ends at ramtop. It
 * also assumes that the memory layout found was static (i.e. no virtual
 * memory or similar stuff) and access to all unused RAM was unrestricted.
 * All this is true for the Milan, I can't check other machines myself.
 *
 * The MC68040 only allows 4K and 8K for the page size, so it's no problem to
 * use MiNT's memory management with active memory protection, as it uses 8K
 * chunks. Even if the MMU is configured for 4K blocks, it's still possible
 * to use it, as the only difference concerning MiNT's memory protection is
 * that always two pages have to be marked for one memory chunk.
 *
 * As said above, this code /should/ work with the MC68060 as well, but as I
 * not (yet) have a machine with this CPU, I can't test it myself.
 * Nevertheless, I have consulted the MC68060 user's manual, and respected
 * the important differences in MMU handling to the MC68040 (which are mainly
 * that things that were recommended for the MC68040 are now mandatory).
 *
 * Of course, this code is still experimental, so I recommend not to use it
 * in production environments. But if possible, test this code (especially
 * if you're a happy owner of a MC68060-equipped machine) and report both
 * success and failures to me or the MiNT mailing list. Thank you!
 */

# include "mprot.h"

# include "libkern/libkern.h"

# include "cpu.h"	/* cpush */
# include "kmemory.h"
# include "memory.h"
# include "mmu.h"


#ifdef MMU040

#if 0
#define MP_DEBUG(x) DEBUG(x)
#else
#define MP_DEBUG(x)
#endif

void get_mmuregs(ulong *regs);	/* in mmu040.spp */
ulong read_phys(ulong *addr);	/* dito */

/*
 * You can turn this whole module off, and the stuff in context.s,
 * by setting no_mem_prot to 1.
 */

int no_mem_prot = 0;
long page_table_size = 0L;
int page_ram_type = 3;
ulong mem_prot_flags = 0L; /* Bitvector, currently only bit 0 is used */

/*
 * PMMU stuff
 */

/*
 * This is the global copy of the MMU tree found on startup, with protection
 * information according to global_mode_table (it only contains the descriptors
 * for memory controlled by MiNT, all others point back to the original tree
 * for maximum compatibility). Every new process gets a copy of this, with the
 * necessary modifications.
 */
static ulong *global_table = 0L;

/*
 * Another four template MMU trees, one for each protection mode (PROT_I and
 * PROT_P are identical in terms of MMU flags)
 */
static ulong *prot_p_table;
static ulong *prot_g_table;
static ulong *prot_s_table;
static ulong *prot_pr_table;
static ulong *template_tables[PROT_MAX_MODE + 1];

/* Use the four template MMU trees in mark_pages()? */
static int use_templates = 0;

/* Don't use the four template MMU trees if one tree is larger than this */
#define MAX_TEMPLATE	65536L

/* What is the size of the MMU pages? */
static ulong pagesize;
#define _4k_pages	(pagesize == 4096UL)

/* How many of the three descriptor types do we have? */
static ulong root_descriptors, pointer_descriptors, page_descriptors;

/* mint_top_* get used in mem.c also */
ulong mint_top_tt;
ulong mint_top_st;

/* number of megabytes of TT RAM (rounded up, if necessary) */
int tt_mbytes;

/*
 * global_mode_table: one byte per page in the system.  Initially all pages
 * are set to "global" but then the TPA pages are set to "invalid" in
 * init_mem.  This has to be allocated and initialized in init_tables,
 * when you know how much memory there is.  You need a byte per page,
 * from zero to the end of TT RAM, including the space between STRAM
 * and TTRAM.  That is, you need 16MB/pagesize plus (tt_mbytes/pagesize)
 * bytes here (pagesize refers to MiNT's pagesize for memory, and is 8K at
 * the moment).
 */
static unsigned char *global_mode_table = 0L;

/* The values to clear/set in a page descriptor for the protection types */
#define SUPERBIT	0x80UL
#define CACHEMODEBITS	0x60UL
#define NOCACHE		0x60UL
#define READONLYBIT	0x04UL
#define PAGETYPEBITS	0x03UL
#define INVALID		0x00UL
#define RESIDENT	0x01UL
#define PROTECTIONBITS	(SUPERBIT | READONLYBIT | PAGETYPEBITS)

ulong mode_descriptors[] =
	{
		INVALID,		/* private */
		RESIDENT,		/* global */
		RESIDENT | SUPERBIT,	/* super */
		RESIDENT | READONLYBIT,	/* readonly */
		INVALID			/* invalid/free */
	};

#define FLUSH_PMMU	0x1
#define FLUSH_CACHE	0x2
#define FLUSH_BOTH	0x3

/*
 * is_mint_mem
 *
 * Helper function for walk_or_copy_tree(); checks whether a memory area
 * described by root-, pointer-, and page-index is memory controlled by MiNT,
 * i.e. completely between 0 and mint_top_st or between 0x01000000 and
 * mint_top_tt (where mint_top_st and mint_top_tt are rounded according to the
 * size of the memory area).
 *
 * BUGS: Assumes that mint_top_tt never exceeds 0xfe000000, but as this would
 * mean almost 4GB of memory (the Milan, for example, is limited to 1GB), I
 * don't think it's really a problem (especially because a lot of programs will
 * most probably already break with 2GB due to sign problems in pointer
 * handling ...)
 *
 * Input:
 * ri: Root level index of the memory area
 * pi: Pointer level index of the memory area, or -1
 * pgi: Page level index of the memory area, or -1
 *
 * Returns:
 * 0: Memory area is not controlled by MiNT
 * otherwise: Memory area is inside the correct bounds
 */
INLINE int
is_mint_mem(ulong ri, ulong pi, ulong pgi)
{
	ulong	start,
		size,
		top_st,
		top_tt;

	/* Calculate start and size of the memory area */
	start = ri << 25UL;
	size = 1UL << 25UL;		/* root level "pages" are 32MB */
	if (pi != -1)
	{
		start |= pi << 18UL;
		size = 1UL << 18UL;	/* pointer level "pages" are 256kB */
		if (pgi != -1)
		{
			start |= pgi * pagesize;
			size = pagesize;
		}
	}
	/*
	 * Decrement size to prevent overflow (i.e. that start + size doesn't
	 * get 0, which would cause wrong comparison results)
	 */
	size--;
	/*
	 * Now check whether the memory area is completely controlled by MiNT
	 */
	if (mint_top_tt)
	{
		top_tt = (mint_top_tt + size) & ~size;
		if ((start >= 0x01000000UL) && ((start + size) < top_tt))
			return(1);
	}
	top_st = (mint_top_st + size) & ~size;
	if ((start < top_st) && ((start + size) < top_st))
		return(1);
	else
		return(0);
}

/*
 * walk_page
 *
 * Helper function for walk_or_copy_tree; "traverses" a page level descriptor.
 *
 * Input:
 * mode, root, pointer, page: See walk_or_copy_tree
 * rp: Pointer to the root of the MMU tree
 * ri: Root level index for this traverse
 * pi: Pointer level index for this traverse
 * pgi: Page level index for this traverse
 */
INLINE void
walk_page(int mode, ulong *root, ulong *pointer, ulong *page,
	ulong *rp, ulong ri, ulong pi, ulong pgi)
{
	ulong	desc,
		*descp,
		*help;

	if (mode == 0)
		(*page)++;
	else
	{
		/* Get address and contents of the page descriptor */
		help = (ulong *)(read_phys(rp + ri) & ~0x1ffUL);
		help = (ulong *)(read_phys(help + pi) &
			~(_4k_pages ? 0x7fUL : 0x3fUL));
		descp = help + pgi;
		desc = read_phys(descp);
		/*
		 * If it's an indirect descriptor, read the descriptor it
		 * points to
		 */
		if ((desc & 3) == 2)
		{
			descp = (ulong *)(desc & ~3);
			desc = read_phys(descp);
			/* If it's indirect again, it's invalid */
			if ((desc & 3) == 2)
				desc = 0UL;
		}
		/* Calculate the address of the copied page descriptor */
		help = (ulong *)(*(root + ri) & ~0x1ffUL);
		help = (ulong *)(*(help + pi) & ~(_4k_pages ? 0x7fUL : 0x3fUL));
		help += pgi;
		/* Copy the descriptor */
		if (is_mint_mem(ri, pi, pgi))
		{
			*help = desc;
			
# if 0
			/* If it's the first descriptor (i.e. for logical
			 * address 0), ensure the "supervisor only" flag is
			 * set
			 */
			if ((ri + pi + pgi) == 0)
			{
				if ((*help & SUPERBIT) != SUPERBIT)
				{
FORCE("Setting supervisor protection for page 0");
					*help |= SUPERBIT;
				}
			}
# endif
		}
		else
		{
			/*
			 * For non-MiNT memory, create an indirect descriptor,
			 * pointing to the original descriptor
			 */
			*help = (ulong)descp + 2;
		}
	}
}

/*
 * walk_pointer
 *
 * Helper function for walk_or_copy_tree; traverses a pointer level descriptor.
 *
 * Input:
 * See walk_page()
 */
INLINE void
walk_pointer(int mode, ulong *root, ulong *pointer, ulong *page,
	ulong *rp, ulong ri, ulong pi)
{
	ulong		desc,
			i,
			max = _4k_pages ? 64UL : 32UL,
			*help;
	static ulong	next_pgi = 0;

	/* Get the contents of the pointer descriptor */
	help = (ulong *)(read_phys(rp + ri) & ~0x1ffUL);
	desc = read_phys(help + pi);
	if (mode == 0)
	{
		(*pointer)++;
		/* Abort if this descriptor is invalid */
		if ((desc & 3) < 2)
			return;
	}
	else
	{
		/* Calculate the address of the destination descriptor */
		help = (ulong *)(*(root + ri) & ~0x1ffUL);
		help += pi;
		/* Copy the descriptor flags */
		*help = desc & (_4k_pages ? 0x7f : 0x3f);
		/* Abort if this descriptor is invalid */
		if ((desc & 3) < 2)
			return;
		/* Otherwise, insert the page table address */
		if (is_mint_mem(ri, pi, -1))
		{
			*help |= (ulong)&page[next_pgi];
			next_pgi += max;
		}
		else
			*help = desc;
	}
	/* Now traverse the page level table */
	if (is_mint_mem(ri, pi, -1))
	{
		for (i = 0; i < max; i++)
			walk_page(mode, root, pointer, page, rp, ri, pi, i);
	}
}

/*
 * walk_root
 *
 * Helper function for walk_or_copy_tree; traverses a root level descriptor.
 *
 * Input:
 * See walk_page()
 */
INLINE void
walk_root(int mode, ulong *root, ulong *pointer, ulong *page,
	ulong *rp, ulong ri)
{
	ulong		desc,
			i;
	static ulong	next_pi = 0;

	desc = read_phys(rp + ri);
	if (mode == 0)
	{
		(*root)++;
		/* Abort if this descriptor is invalid */
		if ((desc & 3) < 2)
			return;
	}
	else
	{
		/* Copy the descriptor flags */
		*(root + ri) = desc & 0x1ff;
		/* Abort if this descriptor is invalid */
		if ((desc & 3) < 2)
			return;
		/* Otherwise, insert the pointer table address */
		if (is_mint_mem(ri, -1, -1))
		{
			*(root + ri) |= (ulong)&pointer[next_pi];
			next_pi += 128UL;
		}
		else
			*(root + ri) = desc;
	}
	/* Now traverse the pointer level table */
	if (is_mint_mem(ri, -1, -1))
	{
		for (i = 0; i < 128UL; i++)
			walk_pointer(mode, root, pointer, page, rp, ri, i);
	}
}

/*
 * walk_or_copy_tree
 *
 * Either examines the current MMU tree and calculates the memory required for
 * a copy, or copies the current MMU tree to a new location. The copy is not a
 * complete copy, as descriptors (of any level) for memory not controlled by
 * MiNT aren't duplicated, they simply point to the original table (this
 * ensures that we don't copy unnecessary stuff and don't break things like
 * hardware register emulation).
 *
 * Input:
 * mode: Examine the tree (0) or copy it (1)
 * root: Pointer to a ulong to store the number of used root level descriptors
 *       (mode == 0) or pointer to the desination root level table (mode == 1)
 * pointer: Pointer to a ulong to store the number of used pointer level
 *          descriptors (mode == 0) or pointer to the destination pointer level
 *          table (mode == 1)
 * page: Pointer to a ulong to store the number of used page level descriptors
 *       (mode == 0) or pointer to the destination page level table (mode == 1)
 *
 * Returns:
 * 0L: The tree can't be copied (mode == 0) or was successfully copied
 *     (mode == 1)
 * else: Number of bytes necessary for a copy (only for mode == 0)
 */
static long walk_or_copy_tree(int mode, ulong *root, ulong *pointer,
	ulong *page)
{
	ulong	mmuregs[7],
		i;

	/* Get the current MMU setup */
	get_mmuregs(mmuregs);
#define TC	0
#define ITT0	1
#define ITT1	2
#define DTT0	3
#define DTT1	4
#define URP	5
#define SRP	6
	/* Some sanity checks */
FORCE("MMU is %sabled", (mmuregs[TC] & 0x8000UL) ? "en" : "dis");
	if (!(mmuregs[TC] & 0x8000UL))
	{
		MP_DEBUG (("MMU isn't enabled"));
		return(0L);	/* MMU isn't enabled */
	}
	if (mmuregs[URP] != mmuregs[SRP])
	{
		MP_DEBUG (("MMU uses different URP and SRP"));
		return(0L);
	}
	for (i = ITT0; i <= DTT1; i++)
	{
		/*
		 * Ensure there's no transparent translation enabled for the
		 * user RAM area
		 */
		if (mmuregs[i] & 0x8000UL)
		{
			ulong	logbase,
				logmask,
				address;

			logmask = (mmuregs[i] & 0x00ff0000UL) << 8UL;
			logbase = mmuregs[i] & 0xff000000UL;
			logbase &= ~logmask;
			address = 0UL;
			address &= ~logmask;
			if (address == logbase)
			{
transparent_exit:
FORCE("MMU uses transparent translation for parts of the user RAM");
				MP_DEBUG (("MMU uses transparent translation "
					"for user RAM area"));
				return(0L);
			}
			address = mint_top_st & 0xff000000UL;
			address &= ~logmask;
			if (address == logbase)
				goto transparent_exit;
			if (mint_top_tt)
			{
				address = 0x01000000UL;
				address &= ~logmask;
				if (address == logbase)
					goto transparent_exit;
				address = mint_top_tt & 0xff000000UL;
				address &= ~logmask;
				if (address == logbase)
					goto transparent_exit;
			}
		}
	}
	pagesize = (mmuregs[TC] & 0x4000UL) ? 8192UL : 4096UL;
FORCE("pagesize is %lu byte", pagesize);

	/* Initialize the counters for mode == 0 */
	if (mode == 0)
		*root = *pointer = *page = 0UL;

	/* Now, actually begin traversing the MMU tree */
	for (i = 0; i < 128UL; i++)
		walk_root(mode, root, pointer, page, (ulong *)mmuregs[URP], i);
	if (mode == 0)
	{
		/*
		 * The tree needs the sum of all used descriptors multiplied
		 * by 4
		 */
		return((long)(*root + *pointer + *page) * 4L);
	}
	else
		return(0L);
}

/*
 * get_page_descriptor
 *
 * Gets a pointer to the page descriptor of a given memory address. Due to the
 * way we copied the original MMU tree, the following holds true:
 * get_page_descriptor(table, x) + 1 = get_page_descriptor(table, x + pagesize)
 * Note that this is only valid for memory controlled by MiNT, i.e. from 0 to
 * mint_top_st and from 0x01000000 to mint_top_tt.
 *
 * Input:
 * table: Pointer to the MMU table
 * addr: address of the memory area
 *
 * Returns:
 * Pointer to the corresponding page descriptor
 */
INLINE ulong
*get_page_descriptor(ulong *table, ulong addr)
{
	ulong	ri,
		pi,
		pgi,
		desc,
		*help;

	/* Calculate the indices for the three table levels */
	ri = addr >> 25UL;
	pi = (addr >> 18UL) & 0x7f;
	pgi = (addr >> (_4k_pages ? 12UL : 13UL)) &
		(_4k_pages ? 0x3f : 0x1f);
	assert(is_mint_mem(ri, pi, pgi));

	/*
	 * Now get the pointer to the page descriptor. Note that we actually
	 * search in the global template table, because the real table may
	 * well have different physical and logical memory locations, so
	 * the pointers read from the descriptors wouldn't be suitable for
	 * the next level in logical address space. Of course, we could use
	 * read_phys() here as well, but as this function is quite a hack,
	 * it' better to just use it in the initialization phase.
	 */
	desc = *(global_table + ri);
	if ((desc & 3) < 2)
	{
get_page_descriptor_fatal:
		FATAL("memory area has an invalid descriptor?!");
	}
	help = (ulong *)(desc & ~0x1ffUL);
	desc = *(help + pi);
	if ((desc & 3) < 2)
		goto get_page_descriptor_fatal;
	help = (ulong *)(desc & ~(_4k_pages ? 0x7fUL : 0x3fUL));
	/*
	 * OK, now that we've found the descriptor in the global table,
	 * substract the global table's address and add the address of the
	 * table we actually had to search in
	 */
	help = (ulong *)((ulong)help - (ulong)global_table + (ulong)table);
	return(help + pgi);
}

/*
 * init_tables()
 *
 * Set up memory protection for the MC68040. The main things done here are:
 * - calculate the size of the per-process MMU trees (page_table_size)
 * - create the template MMU tree as a copy of the current MMU tree
 * - determine which RAM types are suitable for the per-process MMU trees
 * - create the global mode page table
 *
 * The MMU itself is not influenced here, this is done by the first call to
 * init_page_table().
 */

void
init_tables(void)
{
	if (no_mem_prot)
		return;
	{
    int n_megabytes;
    long global_mode_table_size, rounded_table_size;
    ulong last, this, addr, len;
    ulong *desc;
    int i;

    TRACE(("init_tables"));

#define phys_top_tt (*(ulong *)0x5a4L)
    if (phys_top_tt <= 0x01000000L) mint_top_tt = 0;
    else mint_top_tt = phys_top_tt;

#define phys_top_st (*(ulong *)0x42eL)
    mint_top_st = phys_top_st;

    if (mint_top_tt)
    	tt_mbytes = (int)((mint_top_tt - 0x01000000L + ONE_MEG - 1L) / ONE_MEG);
    else
    	tt_mbytes = 0;

    n_megabytes = (int) ((mint_top_st / ONE_MEG) + tt_mbytes);

    /*
     * Get the page table size. This is done by traversing the current MMU
     * tree, counting the number of descriptors.
     */
    root_descriptors = pointer_descriptors = page_descriptors = 0UL;
    page_table_size = walk_or_copy_tree(0, &root_descriptors,
	&pointer_descriptors, &page_descriptors);
FORCE("root_descriptors: %lu", root_descriptors);
FORCE("pointer_descriptors: %lu", pointer_descriptors);
FORCE("page_descriptors: %lu", page_descriptors);
FORCE("memory needed for each MMU tree: %lu byte", page_table_size);

    /* If size is 0, then something is wrong with the current MMU setup */
    if (page_table_size == 0L)
    {
init_tables_fatal:
    	FATAL("Couldn't initialize memory protection. Please run MINTNP.PRG instead.");
    }


    rounded_table_size = (long)ROUND512(page_table_size);
    if (rounded_table_size <= MAX_TEMPLATE)
    {
FORCE("using template trees for each protection mode");
	use_templates = 1;
	global_table = kmalloc(5 * rounded_table_size + 511);
    }
    else
	global_table = kmalloc(page_table_size + 511);
    if (global_table == 0L)
    	goto init_tables_fatal;
    global_table = ROUND512(global_table);
    if (use_templates)
    {
	prot_p_table = (ulong *)((long)global_table + rounded_table_size);
	prot_g_table = (ulong *)((long)prot_p_table + rounded_table_size);
	prot_s_table = (ulong *)((long)prot_g_table + rounded_table_size);
	prot_pr_table = (ulong *)((long)prot_s_table + rounded_table_size);
	template_tables[PROT_P] = prot_p_table;
	template_tables[PROT_G] = prot_g_table;
	template_tables[PROT_S] = prot_s_table;
	template_tables[PROT_PR] = prot_pr_table;
	template_tables[PROT_I] = prot_p_table;
    }

    /* Create the template copy(ies) of the MMU tree. */
    walk_or_copy_tree(1, global_table, global_table + root_descriptors,
    	global_table + root_descriptors + pointer_descriptors);
    if (use_templates)
    {
	/*
	 * The additional template trees don't need correct table level
	 * descriptors, so it's save to just copy global_table four times
	 * (especially because walk_or_copy_tree must not be called more than
	 * once in copy-mode)
	 */
	quickmove(prot_p_table, global_table, page_table_size);
	quickmove(prot_g_table, global_table, page_table_size);
	quickmove(prot_s_table, global_table, page_table_size);
	quickmove(prot_pr_table, global_table, page_table_size);
    }

    /*
     * Now ensure that at least one of ST and Alternate RAM are linearly
     * mapped (we need this as the MMU trees are written to logical RAM and
     * must be contiguos in the physical RAM as well)
     */
    for (i = 0; i < 2; i++)
    {
    	if (i == 0)
    	{
	    addr = 0UL;
	    len = mint_top_st;
	}
	else
	{
	    if (mint_top_tt)
	    {
	    	addr = 0x01000000UL;
	    	len = mint_top_tt - 0x01000000UL;
	    }
	    else
	    	break;
	}
	len = (len + pagesize - 1UL) & ~(pagesize - 1UL);
	last = *get_page_descriptor(global_table, addr);
	/* If there is an invalid descriptor, exit */
	if ((last & 3) == 0)
	{
		DEBUG (("invalid descriptor, exit"));
		goto init_tables_fatal;
	}
	last &= ~(pagesize - 1UL);
	/*
	 * While we're at it, also mark the four template trees, if necessary
	 */
#define membot (*(ulong *)0x432L)
	if (use_templates && (addr >= membot))
	{
	    desc = get_page_descriptor(prot_p_table, addr);
	    *desc &= ~PROTECTIONBITS;
	    *desc |= mode_descriptors[PROT_P];
	    desc = get_page_descriptor(prot_g_table, addr);
	    *desc &= ~PROTECTIONBITS;
	    *desc |= mode_descriptors[PROT_G];
	    desc = get_page_descriptor(prot_s_table, addr);
	    *desc &= ~PROTECTIONBITS;
	    *desc |= mode_descriptors[PROT_S];
	    desc = get_page_descriptor(prot_pr_table, addr);
	    *desc &= ~PROTECTIONBITS;
	    *desc |= mode_descriptors[PROT_PR];
	}
	len -= pagesize;
	addr += pagesize;
	while (len)
	{
	    this = *get_page_descriptor(global_table, addr);
	    if ((this & 3) == 0)
	    {
	    	DEBUG (("(this & 3) == 0 -> exit"));
		goto init_tables_fatal;
	    }
	    this &= ~(pagesize - 1UL);
	    /*
	     * The last physical address must be pagesize byte before the
	     * current. Do not break the loop to ensure we made a complete
	     * check of all the descriptors needed for the memory controlled
	     * by MiNT.
	     */
	    if ((addr > membot) && ((last + pagesize) != this))
	    	page_ram_type &= ~(1 << i);
	    /*
	     * While we're at it, also mark the four template trees, if
	     * necessary
	     */
	    if (use_templates && (addr >= membot))
	    {
		desc = get_page_descriptor(prot_p_table, addr);
		*desc &= ~PROTECTIONBITS;
		*desc |= mode_descriptors[PROT_P];
		desc = get_page_descriptor(prot_g_table, addr);
		*desc &= ~PROTECTIONBITS;
		*desc |= mode_descriptors[PROT_G];
		desc = get_page_descriptor(prot_s_table, addr);
		*desc &= ~PROTECTIONBITS;
		*desc |= mode_descriptors[PROT_S];
		desc = get_page_descriptor(prot_pr_table, addr);
		*desc &= ~PROTECTIONBITS;
		*desc |= mode_descriptors[PROT_PR];
	    }
	    last = this;
	    len -= pagesize;
	    addr += pagesize;
	}
FORCE("%s RAM is %slinear", i ? "Alternate" : "ST",
    (page_ram_type & (1 << i)) ? "" : "non-");
    }
    if (!page_ram_type)
	goto init_tables_fatal;

    global_mode_table_size = ((SIXTEEN_MEG / QUANTUM) +
			    (((ulong)tt_mbytes * ONE_MEG) / QUANTUM));

    global_mode_table = kmalloc(global_mode_table_size);
    if (global_mode_table == 0L)
	goto init_tables_fatal;

    TRACELOW(("mint_top_st is $%lx; mint_top_tt is $%lx, n_megabytes is %d",
	mint_top_st, mint_top_tt, n_megabytes));
    TRACELOW(("page_table_size is %ld, global_mode_table_size %ld",
	    page_table_size, global_mode_table_size));

    /* set the whole global_mode_table to "global" */
    memset(global_mode_table,PROT_G,global_mode_table_size);
	}
}

/*
 * mark_region: mark a region of memory as having a particular type.
 * The arguments are the memory region in question and the new type.
 * If the new type is zero then the old type is preserved.  The
 * type of each page is kept in a global place for this purpose,
 * among others.
 *
 * The types are:
 *  0	private
 *  1	global
 *  2	private, but super-accessible
 *  3	private, but world readable
 *  4   invalid
 *

The idea is this:

    for (each process) {
	if (you're an owner or you're special) {
	    change the mode for an owner
	}
	else {
	    change the mode for a non-owner
	}

	mark_pages(pagetbl,start,len,mode,0);
    }

 */

/*
 * mark_pages
 *
 * Helper function for mark_region; sets all pages matching a given memory
 * area to a given protection status.
 *
 * Input:
 * table: Pointer to the MMU table
 * start: Start address of the memory area
 * len: Length of the memory area
 * mode: New protection mode (if bit 14 is set, it's the raw descriptor mode,
 *       MiNT's protection mode otherwise)
 * flush: If bit 0 is set, flush the PMMU when finished; if bit 1 is set,
 *        flush the caches
 */
INLINE void
mark_pages(ulong *table, ulong start, ulong len, short int mode, short flush)
{
	ulong	*desc,
		*templ_desc,
		origlen,
		setmode;
	int	no_templates = 0;

	if (mode & 0x4000)
	{
		/* If it's a raw mode, just filter out the 15th and 14th bit */
		setmode = (ulong)(mode & 0x3fff);
		/* We must not use the templates trees with raw modes */
		no_templates = 1;
	}
	else
	{
		/* Otherwise, convert to descriptor protection mode bits */
		setmode = mode_descriptors[mode];
	}

	origlen = len = (len + 8191UL) & ~8191UL;
	if (use_templates && !no_templates)
	{
		/* When using the template trees, just quickmove() */
		if (len)
		{
			desc = get_page_descriptor(table, start);
			templ_desc = get_page_descriptor(
				template_tables[mode], start);
			quickmove(desc, templ_desc,
				len * 4UL / pagesize);
		}
	}
	else
	{
		/* Otherwise, loop all descriptors for the memory area */
		for (desc = get_page_descriptor(table, start); len; desc++)
		{
			/* Clear the protection bits */
			*desc &= ~PROTECTIONBITS;
			/* And set the new ones */
			*desc |= setmode;
			len -= pagesize;
		}
	}
	if (flush & FLUSH_PMMU)
		flush_mmu();
	if (flush & FLUSH_CACHE)
	{
		cpush((void *)start, origlen);
		/* Also push the modified MMU table */
		cpush((void *)table, page_table_size);
	}
}

/*
 * get_page_cookie: return a cookie representing the protection status
 * of some memory.
 *
 * Returns (protection bits of the descriptor | 0xc000) on success.
 * Returns 1 if the pages are not all controlled, 0 if they're not all the same.
 */

INLINE short
get_page_cookie(ulong *table, ulong start, ulong len)
{
    ulong *desc,
          first;

    if (start < mint_top_st) {
	/* start is in ST RAM; fail if not entirely in ST RAM */
	if (start+len > mint_top_st) {
	    return 1;
	}
    }
    else if (start >= 0x01000000L && start < mint_top_tt) {
	/* start is in TT RAM; fail if not entirely in TT RAM */
	if (start+len > mint_top_tt) {
	    return 1;
	}
    }

    /* Get the protecion bits of the first page descriptor for the area */
    desc = get_page_descriptor(table, start);
    first = *desc & PROTECTIONBITS;

    len = (len + 8191UL) & ~8191UL;
    len -= pagesize;
    /* Loop through the remaining page descriptors */
    while (len)
    {
    	desc++;
    	/*
    	 * If the current descriptor has different protection mode than the
    	 * first, we fail
    	 */
    	if ((*desc & PROTECTIONBITS) != first)
    	    return(0);
    	len -= pagesize;
    }
    /* All descriptors share the same protection bits, return them */
    return(first | 0xc000UL);
}

/* get_prot_mode(r): returns the type of protection region r
 * has
 */

int
get_prot_mode(MEMREGION *r)
{
	if (no_mem_prot)
		return PROT_G;
	{
		ulong start = r->loc;
		return global_mode_table[(start >> 13)];
	}
}

void
mark_region(MEMREGION *region, short int mode)
{
	if (no_mem_prot)
		return;
	{
    ulong start = region->loc;
    ulong len = region->len;
    ulong i;
    PROC *proc;
    MEMREGION **mr;
    short int setmode;
    ulong cmpmode;
    int shortcut;
    unsigned char *tbl_ptr, *tbl_end;

    MP_DEBUG(("mark_region %lx len %lx mode %d",start,len,mode));

    /* Don't do anything if init_tables() has not yet finished */
    if ((global_table == 0L) || (global_mode_table == 0L))
	return;

#if 0 /* this should not occur any more */
    if (mode == PROT_NOCHANGE) {
	mode = global_mode_table[(start >> 13)];
    }
#else
    assert(mode != PROT_NOCHANGE);
#endif

    /*
     * First, check if the global_mode_table is already "compatible" with the
     * new mode, i.e. the MMU-protection is the same for all affected pages
     */

    shortcut = 0;
    tbl_ptr = &global_mode_table[start >> 13];
    tbl_end = tbl_ptr + (len >> 13);
    cmpmode = mode_descriptors[mode];
    while (tbl_ptr != tbl_end)
    {
	if (mode_descriptors[*tbl_ptr] != cmpmode)
	    break;
	tbl_ptr++;
    }
    if (tbl_ptr == tbl_end)
	shortcut = 1;

    /* mark the global page table */
    memset(&global_mode_table[start >> 13], mode, (len >> 13));
    /* mark the global template tree */
    mark_pages(global_table, start, len, mode, 0);

    /*
     * OK, if shortcut is set and mode is PROT_G, nothing has to be done,
     * because all processes already have global access
     */

    if (shortcut && (mode == PROT_G))
    	goto finished;

    for (proc = proclist; proc; proc = proc->gl_next) {
	if (proc->wait_q == ZOMBIE_Q || proc->wait_q == TSR_Q)
		continue;
	assert(proc->page_table);
	if (!shortcut && (mode == PROT_I || mode == PROT_G)) {
	    /* everybody gets the same flags */
	    goto notowner;
	}
	if (proc->memflags & F_OS_SPECIAL) {
	    /* you're special; you get owner flags */
	    MP_DEBUG(("mark_region: pid %d is an OS special!",proc->pid));
	    goto owner;
	}
	if ((mr = proc->mem) != 0) {
	    for (i = 0; i < proc->num_reg; i++, mr++) {
		if (*mr == region) {
		    MP_DEBUG(("mark_region: pid %d is an owner",proc->pid));
owner:
		    setmode = (mode == PROT_I) ? PROT_I : PROT_G;
		    goto gotvals;
		}
	    }
	}

notowner:
	/* if you get here you're not an owner, or mode is G or I */
	    MP_DEBUG(("mark_region: pid %d gets non-owner modes",proc->pid));

	/*
	 * If shortcut is set, we won't do anything for non-owners, as the
	 * old mode for the area is compatible (i.e. the same MMU-wise).
	 * Note that we don't shortcut for curproc, as this call to
	 * mark_region() may be the invalidation of a freed or shrinked
	 * region which curproc is no longer owner of, but still has owner
	 * rights (MMU-wise).
	 */
	if (shortcut && (proc != curproc))
	    continue;

	setmode = mode;

gotvals:
	mark_pages(proc->page_table, start, len, setmode, 0);
    }
finished:
    flush_mmu();
    cpush((void *)start, len);
	}
}

/* special version of mark_region, used for attaching (mode == PROT_P)
   and detaching (mode == PROT_I) a memory region to/from a process. */
void
mark_proc_region(PROC *proc, MEMREGION *region, short int mode)
{
	if (no_mem_prot)
		return;
	{
    ulong start = region->loc;
    ulong len = region->len;
    short global_mode;


    MP_DEBUG(("mark_proc_region %lx len %lx mode %d for pid %d",
	      start, len, mode, proc->pid));

    global_mode = global_mode_table[(start >> 13)];

    assert(proc->page_table);
    if (global_mode == PROT_I || global_mode == PROT_G)
      mode = global_mode;
    else {
	if (proc->memflags & F_OS_SPECIAL) {
	    /* you're special; you get owner flags */
	    MP_DEBUG(("mark_proc_region: pid %d is an OS special!",proc->pid));
	    goto owner;
	}
	if (mode == PROT_P) {
	    MP_DEBUG(("mark_proc_region: pid %d is an owner",proc->pid));
owner:
	    mode = PROT_G;
	    goto gotvals;
	}
	else if (mode == PROT_I)
	{
	    /*
	     * PROT_I actually means "detach region", which in turn means
	     * "restore the global table's access mode". Two things to note:
	     * - If the global mode is PROT_P, the result is the same as with
	     *   PROT_I, because for non-owners, these modes are identical
	     * - Using the global mode is not a bug, because detaching a region
	     *   not necessarily means it's being freed (see detach_region(),
	     *   which only calls mark_proc_region for regions with a link
	     *   count > 1, free_region() otherwise)
	     */
	    mode = global_mode;
	}
    }

/* if you get here you're not an owner, or mode is G or I */
    MP_DEBUG(("mark_proc_region: pid %d gets non-owner modes",proc->pid));

gotvals:
    mark_pages(proc->page_table, start, len, mode, FLUSH_BOTH);
	}
}

/*
 * prot_temp: temporarily alter curproc's access to memory.
 * Pass in a -1 to give curproc global access; returns a cookie.  Call
 * again with that cookie to return the memory to the old mode.
 * There should be no context switches or memory protection changes
 * in the meantime.
 *
 * If called with mode == -1, returns...
 *	-1 if mem prot is off -- no error, no action.
 *	 0 if the pages are not all the same.
 *	 1 if the pages are not all controlled by the page tables.
 *
 * When mode != -1, returns...
 *	0 for success (should never fail).  There is little checking.
 * Calling with mode == 0 or 1 results in zero to spoof success, but in fact
 * this is an error.  Mode is only really valid if (mode & 0xc000).
 */

int
prot_temp(ulong loc, ulong len, int mode)
{
	if (no_mem_prot)
		return -1;
	{
    int cookie;

    /* round start down to the previous page and len up to the next one. */
    len += loc & MASKBITS;
    loc &= ~MASKBITS;
    len = ROUND(len);

    if (mode == 0 || mode == 1) return 0;	/* do nothing */
    if (mode == -1) {
	cookie = get_page_cookie(curproc->page_table, loc, len);

	/* if not all controlled, return status */
	if (cookie == 0 || cookie == 1) return cookie;

	/* Otherwise, mark the pages as global */
	mark_pages(curproc->page_table, loc, len, PROT_G, FLUSH_BOTH);

	return cookie;
    }
    else {
	mark_pages(curproc->page_table, loc, len, mode, FLUSH_BOTH);
	return 0;
    }
	}
}

/*
 * init_page_table: fill in the page table for the indicated process. The
 * master page map is consulted for the modes of all pages, and the memory
 * region data structures are consulted to see if this process is the owner
 * of any of those tables.
 *
 * This also sets crp in both ctxts of the process.  If this is the first call,
 * then the MMU is initialized with the new root pointers.
 */

static short mmu_is_set_up = 0;

void
init_page_table(PROC *proc)
{
	if (no_mem_prot)
		return;
	{
    ulong *table, phystable;
    ulong addr, len, *desc;
    ulong i;
    MEMREGION **mr;

    assert(proc && proc->page_table && !((ulong)proc->page_table & 0x1ffUL));

    if (proc->pid)
        MP_DEBUG(("init_page_table(proc=%lx, pid %d)",proc,proc->pid));

    table = proc->page_table;
    /*
     * Pitfall: The rootpointer registers and the pointers inside the table
     * must be physical addresses, not logical
     */
    phystable = *get_page_descriptor(global_table,
    	(ulong)table) & ~(pagesize - 1UL);
    phystable += (ulong)table & (pagesize - 1UL);

    /* Copy the template MMU tree */
    quickmove(table, global_table, page_table_size);
    /* Adapt the pointer addresses */
    for (i = 0; i < (root_descriptors + pointer_descriptors); i++)
    {
	/* But only those really pointing into our tree */
	if (((table[i] & ~0x1ffUL) - (ulong)global_table) < page_table_size)
	{
	    table[i] -= (ulong)global_table;
	    table[i] += phystable;
	}
    }
    proc->ctxt[0].crp[0] = proc->ctxt[1].crp[0] = phystable;

    /*
     * OK, memory tables are now there as if proc's a non-owner of every
     * page.  Now for each region it IS an owner of, mark with owner
     * modes.
     */
    mr = proc->mem;
    for (i = 0; i < proc->num_reg; i++, mr++)
    {
	if (*mr)
	    mark_pages(proc->page_table, (*mr)->loc, (*mr)->len, PROT_G, 0);
    }

    /*
     * As recommended in the MC68040 user's manual, we mark the table as
     * non-cacheable (it's necessary for the MC68060, anyway)
     */
    len = (page_table_size + pagesize - 1UL) & ~(pagesize - 1UL);
    addr = (ulong)table;
    for (desc = get_page_descriptor(table, addr); len; desc++)
    {
	*desc &= ~CACHEMODEBITS;
	*desc |= NOCACHE;
	addr += pagesize;
	len -= pagesize;
    }
    cpush(table, page_table_size);

    if (!mmu_is_set_up) {
	set_mmu((ulong *)phystable);
	mmu_is_set_up = 1;
    }
	}
}

/*
 * This routine is called when procfs detects that a process wants to be an
 * OS SPECIAL.  The AES, SCRENMGR, and DESKTOP do this, and so does FSMGDOS
 * and possibly some other stuff. It has to re-mark every page in that
 * process' page table based on its new special status. The "special
 * status" is "you get global access to all of memory" and "everybody
 * gets (at least) Super access to yours."  It is the caller's responsibility
 * to set proc's memflags, usually to (F_OS_SPECIAL | F_PROT_S).
 */

void
mem_prot_special(PROC *proc)
{
	if (no_mem_prot)
		return;

	{
    MEMREGION **mr;
    int i;
    unsigned char mode;


    TRACE(("mem_prot_special(pid %d)",proc->pid));

    /*
     * This marks ALL memory, allocated or not, as accessible. When memory
     * is freed even F_OS_SPECIAL processes lose access to it. So one or
     * the other of these is a bug, depending on how you want it to work.
     */
    mark_pages(proc->page_table,
    	membot & ~8191,
    	mint_top_st - (membot & ~8191),
    	PROT_G,
    	0);
    if (mint_top_tt) {
	mark_pages(proc->page_table,
		    0x01000000L,
		    mint_top_tt - 0x01000000UL,
		    PROT_G,
		    0);
    }

    /*
     * In addition, mark all the pages the process already owns as "super"
     * in all other processes, but only if they are still marked "private".
     * Thus the "special" process can access all of memory, and any process can
     * access the "special" process' memory when in super mode.
     */

    mr = proc->mem;

    for (i=0; i < proc->num_reg; i++, mr++) {
	if (*mr) {
	    mode = global_mode_table[((*mr)->loc >> 13)];
	    if (mode == PROT_P)
		mark_region(*mr, PROT_S);
	    else
	    {
		MP_DEBUG(("mem_prot_special: Not marking region %lx - %lx as super", (*mr)->loc, (*mr)->loc + (*mr)->len - 1L));
	    }
	}
    }
	}
}
    
static const char *berr_msg[] = { 
/*  "........." */
    "private  ",
    "global   ",    /* turned into "hardware" for violation reports */
    "super    ",
    "readable ",
    "free     ",
    "hardware "	    /* used when the memory is not controlled by us */
};

static const char *rw_msg[] = {
    "??",
    "w ",
    "r ",
    "rw"
};

void
report_buserr(void)
{
	const char *type, *rw;
	short mode;
	ulong aa, pc;
	char alertbuf[5*32+16];	/* enough for an alert */
	long len;
	char *aptr;
	
	if (no_mem_prot)
		return;
	
	aa = curproc->exception_addr;
	pc = curproc->exception_pc;
	
	if ((mint_top_tt && aa < mint_top_tt) || (aa < mint_top_st))
	{
		mode = global_mode_table[(curproc->exception_addr >> 13)];
		if (mode == PROT_G)
		{
			/* page is global: obviously a hardware bus error */
			mode = 5;
		}
	}
	else
	{
		/* (addr is > mint_top_tt) set mode = 5 so we don't look for owners */
		mode = 5;
	}
	
	type = berr_msg [mode];
	rw = rw_msg [curproc->exception_access];
	
	/* construct an AES alert box for this error:
	 * | PROCESS  "buserrxx"  KILLED: |
	 * | MEMORY VIOLATION.  (PID 000) |
	 * |                              |
	 * | Type: ......... PC: pc...... |
	 * | Addr: ........  BP: ........ |
	 */
	
	/* we play games to get around 128-char max for ksprintf */
	aptr = alertbuf;
	len = sizeof (alertbuf);
	ksprintf (alertbuf, len, "[1][ PROCESS  \"%s\"  KILLED: |", curproc->name);
	
	aptr = alertbuf + strlen (alertbuf);
	len = sizeof (alertbuf) - strlen (alertbuf);
	ksprintf (aptr, len, " MEMORY VIOLATION.  (PID %03d) | |", curproc->pid);
	
	aptr = alertbuf + strlen (alertbuf);
	len = sizeof (alertbuf) - strlen (alertbuf);
	ksprintf (aptr, len, " Type: %s PC: %08lx |", type, pc);
	
	aptr = alertbuf + strlen (alertbuf);
	len = sizeof (alertbuf) - strlen (alertbuf);
	ksprintf (aptr, len, " Addr: %08lx  BP: %08lx ][ OK ]", aa, curproc->base);
	
	if (!_ALERT (alertbuf))
	{
		/* this will call _alert again, but it will just fail again */
		ALERT ("MEMORY VIOLATION: type=%s RW=%s AA=%lx PC=%lx BP=%lx",
			type, rw, aa, pc, curproc->base);
	}
	
	if (curproc->pid == 0 || curproc->memflags & F_OS_SPECIAL)
	{
		/* the system is so thoroughly hosed that anything we try will
		 * likely cause another bus error; so let's just hang up
		 */
		
		FATAL ("Operating system killed");
	}
}

/*
 * Can the process "p" access the "nbytes" long
 * block of memory starting at "start"?
 * If it would be a legal access, the current
 * process is given temporary access via
 * prot_temp.
 * Returns a cookie like the one prot_temp
 * returns; if the process shouldn't have
 * access to the memory, returns 1.
 *
 * BUG: should actually read p's page table to
 * determine access
 */

int
mem_access_for(PROC *p, ulong start, long nbytes)
{
	MEMREGION **mr;
	int i;

	if (no_mem_prot) return -1;
	if (start >= (ulong)p && start+nbytes <= (ulong)(p+1))
		return -1;
	if (p == rootproc)
		goto win_and_mark;

	mr = p->mem;
	if (mr) {
	    for (i = 0; i < p->num_reg; i++, mr++) {
		if (*mr) {
		    if (((*mr)->loc <= start) &&
			((*mr)->loc + (*mr)->len >= start + nbytes))
			    goto win_and_mark;
		}
	    }
	}

	return 0;	/* we don't own this memory */

win_and_mark:
	return prot_temp(start, nbytes, -1);
}

/* Debugging stuff */

static const char modesym[] = { 'p', 'g', 's', 'r', 'i' };

void
QUICKDUMP(void)
{
    char outstr[33];
    ulong i, j, end;

    if (no_mem_prot) return;

    FORCE("STRAM global table:");
    outstr[32] = '\0';
    end = mint_top_st / QUANTUM;
    for (i = 0; i < end; i += 32) {
	for (j=0; j<32; j++) {
	    outstr[j] = modesym[global_mode_table[j+i]];
	}
	FORCE("%08lx: %s",i*8192L,outstr);
    }

    if (mint_top_tt) {
	FORCE("TTRAM global table:");
	end = mint_top_tt / QUANTUM;
	for (i = 2048; i < end; i += 32) {
	    for (j=0; j<32; j++) {
		outstr[j] = modesym[global_mode_table[j+i]];
	    }
	    FORCE("%08lx: %s",i*8192L,outstr);
	}
    }
}

/*
 * big_mem_dump is a biggie: for each page in the system, it
 * displays the PID of the (first) owner and the protection mode.
 * The output has three chars per page, and eight chars per line.
 * The first page of a region is marked with the mode, and the
 * rest with a space.
 *
 * Logic:
    for (mp = *core; mp; mp++) {
	for (each page of this region) {
	    if (start of line) {
		output line starter;
	    }
	    if (start of region) {
		output mode of this page;
		determine owner;
		output owner;
	    }
	    else {
		output space;
		output owner;
	    }
        }
    }
 */

/*
 * dump_descriptor
 *
 * Helper function for dump_area(), dumps the settings of a given page
 * descriptor.
 *
 * Input:
 * desc: Page descriptor to dump
 * buf: Pointer to line buffer for output
 */
INLINE void
dump_descriptor(ulong desc, char *buf)
{
	_mint_strcat (buf, (desc & 0x03) ? "R" : " ");
	_mint_strcat (buf, (desc & 0x04) ? "W" : " ");
	_mint_strcat (buf, (desc & 0x08) ? "U" : " ");
	_mint_strcat (buf, (desc & 0x10) ? "M" : " ");
	
	switch (desc & CACHEMODEBITS)
	{
		case 0x00:
			_mint_strcat (buf, "Cw");
			break;
		case 0x20:
			_mint_strcat (buf, "Cc");
			break;
		case 0x40:
			_mint_strcat (buf, "Ns");
			break;
		case 0x60:
			_mint_strcat (buf, "Nc");
			break;
	}
	
	_mint_strcat (buf, (desc & 0x080) ? "S"  : " ");
	_mint_strcat (buf, (desc & 0x100) ? "U0" : "  ");
	_mint_strcat (buf, (desc & 0x200) ? "U1" : "  ");
	_mint_strcat (buf, (desc & 0x400) ? "G"  : " ");
	
	FORCE (buf);
}

/*
 * dump_area
 *
 * Helper function for BIG_MEM_DUMP, dumps the MMU table for a given memory
 * area.
 *
 * Input:
 * table: Pointer to MMU table to use
 * start: First address of area
 * end: Last address of area - 1
 * buf: Pointer to line buffer for output
 * buflen: length of the buffer
 */
INLINE void
dump_area (ulong *table, ulong start, ulong end, char *buf, long buflen)
{
	ulong begin;
	ulong addr;
	ulong this;
	ulong first;
	ulong last;
	
	addr = begin = start & ~(pagesize - 1);
	first = last = *get_page_descriptor(table, addr);
	for (addr += pagesize; addr < end; addr += pagesize)
	{
		this = *get_page_descriptor (table, addr);
		/*
		 * Only print a line when either the descriptors flags are
		 * different or there's a gap in the physical mapping. This
		 * "compresses" the output significantly, without losing any
		 * information)
		 */
		if (this != (last + pagesize))
		{
			ksprintf (buf, buflen,
				"\r%08lx - %08lx -> %08lx - %08lx  ",
				begin, addr - 1, first & ~(pagesize - 1),
				(last & ~(pagesize - 1)) + pagesize - 1);
			
			dump_descriptor (last, buf);
			
			begin = addr;
			first = this;
		}
		last = this;
	}
	
	ksprintf (buf, buflen, "\r%08lx - %08lx -> %08lx - %08lx  ",
		begin, addr - 1, first & ~(pagesize - 1),
		(last & ~(pagesize - 1)) + pagesize - 1);
	
	dump_descriptor (last, buf);
}

void
BIG_MEM_DUMP (int bigone, PROC *proc)
{
# ifdef DEBUG_INFO
	char buf[128];
	long buflen = sizeof (buf);
	char *lp = buf;
	MEMREGION *mp, **mr, **map;
	PROC *p;
	ulong loc;
	short owner;
	short i;
	short first;
	
	
	if (no_mem_prot)
		return;
	
	for (map = core; map != 0; ((map == core) ? (map = alt) : (map = 0)))
	{
		FORCE ("Annotated memory dump for %s",(map == core ? "core" : "alt"));
		
		first = 1;
		*buf = '\0';
		for (mp = *map; mp; mp = mp->next)
		{
			for (loc = mp->loc; loc < (mp->loc + mp->len); loc += EIGHT_K)
			{
				if (first || ((loc & 0x1ffffL) == 0))
				{
					if (*buf)
						FORCE (buf);
					
					ksprintf (buf, buflen, "\r%08lx: ",loc);
					lp = &buf[11];
					first = 0;
				}
				
				if (loc == mp->loc)
				{
					*lp++ = modesym[global_mode_table[loc / EIGHT_K]];
					
					for (p = proclist; p; p = p->gl_next)
					{
						if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
							continue;
						if (p->mem)
						{
							mr = p->mem;
							for (i = 0; i < p->num_reg; i++, mr++)
							{
								if (*mr == mp)
								{
									owner = p->pid;
									goto gotowner;
								}
							}
						}
					}
					
					owner = 000;
gotowner:
					ksprintf(lp, buflen, "%03d", owner);
					lp += 3;
				}
				else
				{
					*lp++ = ' ';
					*lp++ = '-';
					*lp++ = '-';
					*lp++ = '-';
					*lp = '\0';
				}
			}
		}
		
		FORCE (buf);
	}
	
	if (bigone)
	{
		ulong *table = proc->page_table;
		
		FORCE ("MMU tree for PID %d, logical address: %08lx", proc->pid, table);
		FORCE ("\rST-RAM:                                  ");
		
		dump_area (table, membot, mint_top_st, buf, buflen);
		
		if (mint_top_tt)
		{
			FORCE ("\rAlternate RAM:                           ");
			dump_area (table, 0x01000000UL, mint_top_tt, buf, buflen);
		}
	}
# else
	UNUSED (proc);
	UNUSED (bigone);
# endif /* DEBUG_INFO */
}

# endif /* MMU040 */
