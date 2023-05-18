/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1991,1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * page-table data structures
 *
 *
 * The root pointer points to a list of pointers to top-level pointer tables.
 *
 * Each entry in a pointer table points to another pointer table or to
 * a page table, or is a page descriptor.
 *
 * Since, initially, the logical address space is the physical address space,
 * we only need to worry about 26MB plus 32K for I/O space.
 *
 * Since we want some pages to be supervisor-accessible, but we don't want
 * a whole separate table for that, we use long-format descriptors.
 *
 * Initial memory map:
 *
 * 0	 - membot: S (supervisor only)
 * membot	 - memtop: P (protected TPA)
 * memtop	 - phystop: G (screen)
 * phystop	 - $00E00000: bus error
 * $00E00000- $00E3FFFF: G (ROM)
 * $00E40000- $00FF7FFF: bus error
 * $00FF8000- $00FFFFFF: G (mostly S: I/O space, but that's done in hardware)
 * $01000000- ramtop: P ($04000000 under falcon/CT2)
 * ramtop	 - $7FFFFFFF: G (A32/D32 VME, supervisor only, cacheable)
 * $80000000- $FEFFFFFF: G (A32/D32 VME, supervisor only, non cacheable)
 * $FFxxxxxx	      just like $00xxxxxx.
 *
 * Here's a final choice of layouts: IS=0, PS=13 (8K), TIA=4, TIB=4, TIC=4,
 * TID=7.  This lets us map out entire unused megabytes at level C, and gives
 * us an 8K page size, which is the largest the '040 can deal with.
 *
 * This code implements 4+4+4+7, as follows:
 *
 * tbl_a
 *     0 -> tbl_b0
 *     1-7 -> Cacheable direct (VME) page descriptors
 *     8-E -> Non-cacheable direct (VME) page descriptors
 *     F -> tbl_bf
 *
 * tbl_b0 table: 16 entries (assumes only 16MB of TT RAM)
 *     0 -> tbl_c00 (16MB of ST RAM address space)
 *     1 -> tbl_c01 (16MB of TT RAM address space)
 *     2-F -> cacheable direct (VME) page descriptors
 *
 * tbl_bF table: 16 entries (deals with $FF mapping to $00)
 *     0-E -> Non-cacheable direct (VME) page descriptors
 *     F -> tbl_c00 (16MB of ST RAM address space, repeated here as $FF)
 *
 * tbl_c00 table: ST RAM address space (example assuming 4MB ST RAM)
 *     0-3 -> RAM page tables
 *     4-D -> invalid
 *     E -> direct map, cache enable (ROM)
 *     F -> direct map, cache inhibit (I/O)
 *
 * For each 16MB containing any TT RAM, there's a tbl_c.  Within those,
 * for each MB that actually has TT RAM, there's another table, containing
 * 128 RAM page tables.  Where there isn't RAM, there are "global"
 * pages, to let the hardware bus error or not as it sees fit.
 *
 * One RAM page table is allocated per megabyte of real RAM; each table has
 * 128 entries, which is 8K per page.  For a TT with 4MB ST RAM and 4MB TT RAM
 * that's 8K in page tables.  You can cut this down by not allocating page
 * tables for which the entire megabyte is not accessible (i.e. it's all
 * private memory and it's not YOUR private memory).
 *
 * You have one of these per process.  When somebody loads into G or S memory
 * or leaves it, you have to go through the page tables of every process
 * updating S bits (for S) and DT (for G) bits.
 *
 * The top levels are small & easy so replicating them once per process
 * doesn't really hurt us.
 *
 */

# include "mprot.h"
# include "global.h"

# include "libkern/libkern.h"

# include "cpu.h"
# include "kmemory.h"
# include "memory.h"
# include "mmu.h"
# include "cookie.h"
# include "mmudefs.h"
# include "arch/halt.h"


#if defined(WITH_MMU_SUPPORT) && defined(M68030)

#define TIA_BITS 4
#define TIB_BITS 4
#define TIC_BITS 4
#define TID_BITS 7
#define PAGE_SIZE_SHIFT 13
/* 0+4+4+4+7+13 == 32 */
#define PHYS_PAGESIZE (1UL << PAGE_SIZE_SHIFT)

#define TIA_SHIFT (TIB_BITS + TIC_BITS + TID_BITS + PAGE_SIZE_SHIFT)
#define TIB_SHIFT (TIC_BITS + TID_BITS + PAGE_SIZE_SHIFT)
#define TIC_SHIFT (TID_BITS + PAGE_SIZE_SHIFT)
#define TID_SHIFT (PAGE_SIZE_SHIFT)

/* TBL_SIZE is the size in entries of the A, B, and C level tables */
# define TBLA_SIZE	(1 << TIA_BITS)
# define TBLB_SIZE	(1 << TIB_BITS)
# define TBLC_SIZE	(1 << TIC_BITS)
# define TBLD_SIZE	(1 << TID_BITS)
# define TBLA_SIZE_BYTES	(TBLA_SIZE * sizeof (long_desc))
# define TBLB_SIZE_BYTES	(TBLB_SIZE * sizeof (long_desc))
# define TBLC_SIZE_BYTES	(TBLC_SIZE * sizeof (long_desc))
# define TBLD_SIZE_BYTES	(TBLD_SIZE * sizeof (long_desc))

#define TIA_MASK (TBLA_SIZE - 1)
#define TIB_MASK (TBLB_SIZE - 1)
#define TIC_MASK (TBLC_SIZE - 1)
#define TID_MASK (TBLD_SIZE - 1)

/* the memory area that an entry in a level C table describes (1MB) */
#define TBLC_MEMSIZE (1UL << TIC_SHIFT)
/* the memory area that an entry in a level B table describes (16MB) */
#define TBLB_MEMSIZE (1UL << TIB_SHIFT)
/* the memory area that an entry in a level A table describes (256MB) */
#define TBLA_MEMSIZE (1UL << TIA_SHIFT)

#define TTRAM_START 0x01000000UL


#if 0
#define MP_DEBUG(x) DEBUG(x)
#else
#define MP_DEBUG(x)
#endif

#define DEBUG_MMU_TREE 0

/*
 * You can turn this whole module off, and the stuff in context.s,
 * by setting no_mem_prot to 1.
 */

int no_mem_prot = 0;
long page_table_size = 0L;
ulong mem_prot_flags = 0L; /* Bitvector, currently only bit 0 is used */

/*
 * PMMU stuff
 */

/*
 * This is one global TC register that is copied into every process'
 * context, even though it never changes.  It's also used by the
 * functions that dump page tables.
 */

static tc_reg tc;

/* mint_top_* get used in mem.c also */
static ulong mint_top_tt;
static ulong mint_top_st;

/* offset for CT2 tt-ram normally to 0 */
ulong offset_tt_ram;

/* number of megabytes of TT RAM (rounded up, if necessary */
int tt_mbytes;

/*
 * global_mode_table: one byte per page in the system.  Initially all pages
 * are set to "global" but then the TPA pages are set to "invalid" in
 * init_mem.  This has to be allocated and initialized in init_tables,
 * when you know how much memory there is.  You need a byte per page,
 * from zero to the end of TT RAM, including the space between STRAM
 * and TTRAM.  That is, you need 16MB/pagesize plus (tt_mbytes/pagesize)
 * bytes here.
 */

static unsigned char *global_mode_table = 0L;

/*
 * prototype descriptors; field u1 must be all ones, other u? are zero.
 * This is just the first long of a full descriptor; the ".page_type" part
 * of the union.  These are initialized by init_tables.
 *
 * The proto_page_type table yields the value to stuff into the page_type
 * field of a new process' page table.  It is the "non-owner" mode for
 * a page with the corresponding value in global_mode_table.
 */

static page_type g_page;
static page_type g_ci_page;
static page_type s_ci_page;
static page_type s_page;
static page_type readable_page;
static page_type invalid_page;
static page_type page_ptr;

static page_type *const proto_page_type[] =	{
	&invalid_page,	/* private:      PROT_P */
	&g_page,		/* global:       PROT_G */
	&s_page,	 	/* super:        PROT_S */
	&readable_page,	/* private/read: PROT_PR */
	&invalid_page   /* invalid:      PROT_I */
};


/*
 * Init_tables: called sometime in initialization.  We set up some
 * constants here, but that's all.  The first new_proc call will set up the
 * page table for the root process and switch it in; from then on, we're
 * always under some process' control.
 *
 * The master page-mode table is initialized here, and some constants like
 * the size needed for future page tables.
 *
 * One important constant initialized here is page_table_size, which is
 * the amount of memory required per page table.  new_proc allocates
 * this much memory for each process' page table.  This number will be
 * 1K/megabyte plus page table overhead.  There are TBL_PAGES_OFFS
 * tables at TBL_SIZE_BYTES each before the main tables begin; then
 * there is 1024 bytes per megabyte of memory being mapped.
 */

void
init_tables(void)
{
	long global_mode_table_size;
	unsigned int b_tables;
	unsigned int c_tables;
	unsigned int d_tables;

	if (no_mem_prot)
	{
		page_table_size = 0L;
		return;
	}

	TRACE(("init_tables"));

#define phys_top_tt (*(ulong *)0x5a4L)

	offset_tt_ram = 0;

	if (phys_top_tt <= TTRAM_START)
		mint_top_tt = 0;
	else
	{
		mint_top_tt = phys_top_tt;
		if (machine == machine_ct2)
		{
			offset_tt_ram = 0x03000000L;
			DEBUG (("init_tables: Falcon CT2 -> offset 0x%lx", offset_tt_ram));
		}
	}

#define phys_top_st (*(ulong *)0x42eL)
	mint_top_st = phys_top_st;

	/*
	 * if TT-RAM ends beyond 256MB, we will need additional LEVEL B+C tables
	 */
	if (mint_top_tt)
	{
		b_tables = (unsigned int) ((mint_top_tt + TBLA_MEMSIZE - 1) / TBLA_MEMSIZE) - 1;
		c_tables = (unsigned int) ((mint_top_tt - TTRAM_START + TBLB_MEMSIZE - 1) / TBLB_MEMSIZE);
		d_tables = (unsigned int) ((mint_top_tt - TTRAM_START + TBLC_MEMSIZE - 1) / TBLC_MEMSIZE);
	} else
	{
		b_tables = 0;
		c_tables = 0;
		d_tables = 0;
	}
	tt_mbytes = d_tables;

	/* add the number of d_tables needed for ST memory */
	d_tables += (unsigned int) ((mint_top_st + TBLC_MEMSIZE - 1) / TBLC_MEMSIZE);

	/*
	 * page table size: room for A table, B0 table, BF table, STRAM C
	 * table, one TTRAM C table per 16MB (or fraction) of TTRAM, and 1024
	 * bytes per megabyte.
	 */

	page_table_size =
		TBLA_SIZE_BYTES +                            /* level A table */
		TBLB_SIZE_BYTES + TBLB_SIZE_BYTES +          /* B0 and BF tables */
		TBLC_SIZE_BYTES +                            /* level C table for STRAM (always exactly one needed) */
		b_tables * TBLB_SIZE_BYTES +				 /* level B tables for TTRAM */
		c_tables * TBLC_SIZE_BYTES +				 /* level C tables for TTRAM */
		d_tables * TBLD_SIZE_BYTES; 				 /* level D page descriptors */

	global_mode_table_size = ((TBLB_MEMSIZE / PHYS_PAGESIZE) +
				 (((ulong)tt_mbytes * TBLC_MEMSIZE) / PHYS_PAGESIZE));

	global_mode_table = kmalloc(global_mode_table_size);

	assert(global_mode_table);

	TRACELOW(("mint_top_st is $%lx; mint_top_tt is $%lx, b_tables is %u, c_tables is %u, d_tables is %u",
		mint_top_st, mint_top_tt, b_tables, c_tables, d_tables));
	TRACELOW(("page_table_size is %ld, global_mode_table_size %ld",
		page_table_size,
		global_mode_table_size));

	g_page.limit = 0x7fff;	/* set nonzero fields: disabled limit */
	g_page.unused1 = 0x3f;	/* ones in this reserved field */
	g_page.unused2 = 0;
	g_page.s = 0;
	g_page.unused3 = 0;
	g_page.ci = 0;
	g_page.unused4 = 0;
	g_page.m = 1;		/* set m and u to 1 so CPU won't do writes */
	g_page.u = 1;
	g_page.wp = 0;		/* not write-protected */
	g_page.dt = MMU030_DESCR_TYPE_PAGE;		/* descriptor type 1: page descriptor */

	g_ci_page = g_page;		/* global page, non-cacheable */
	g_ci_page.ci = 1;

	s_ci_page = g_page;		/* supervisor accessible page, non-cacheable */
	s_ci_page.s = 1;
	s_ci_page.ci = 1;

	readable_page = g_page;	/* a page which is globally readable */
	readable_page.wp = 1;	/* but write protected */

	s_page = g_page;		/* a page which is globally accessible */
	s_page.s = 1;		/* if you're supervisor */

	invalid_page = g_page;
	invalid_page.dt = MMU030_DESCR_TYPE_INVALID;

	page_ptr = g_page;
	page_ptr.m = 0;		/* this must be zero in page pointers */
	page_ptr.dt = MMU030_DESCR_TYPE_VALID8;

	tc.enable = 1;
	tc.zeros = 0;
	tc.sre = 0;
	tc.fcl = 0;
	tc.is = 0;
	tc.tia = TIA_BITS;
	tc.tib = TIB_BITS;
	tc.tic = TIC_BITS;
	tc.tid = TID_BITS;			/* 0+4+4+4+7+13 == 32 */
	tc.ps = PAGE_SIZE_SHIFT;	/* 8K page size */

	/* set the whole global_mode_table to "global" */
	memset(global_mode_table,PROT_G,global_mode_table_size);
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
		set up owner modes
	}
	else {
		set up non-owner modes
	}

	mark_pages(pagetbl,start,len,modes);
	}

 */

/*
                                       invalid---v
                                 private/gr---v  |
                               super-------v  |  |
                           global-------v  |  |  |
                       private-------v  |  |  |  |
                                     |  |  |  |  |
*/
static const ushort other_dt[] =   { 0, 1, 1, 1, 0 };
static const ushort other_s[] =    { 0, 0, 1, 0, 0 };
static const ushort other_wp[] =   { 0, 0, 0, 1, 0 };


/*
 * get_page_cookie: return a cookie representing the protection status
 * of some memory.
 *
 * Returns ((wp << 3) | (s << 2) | (dt) | 0x8000) when it wins.
 * Returns 1 if the pages are not all controlled, 0 if they're not all the same.
 */

static short
get_page_cookie(long_desc *base_tbl,ulong start,ulong len)
{
	int a_index, b_index, c_index, d_index;
	long_desc *tbl, *tbl_a, *tbl_b, *tbl_c;
	int dt, s, wp;
	ulong offset;

	if (start < mint_top_st) {
		/* start is in ST RAM; fail if not entirely in ST RAM */
		if (start+len > mint_top_st) {
			return 1;
		}
	}
	else if (start >= TTRAM_START && start < mint_top_tt) {
		/* start is in TT RAM; fail if not entirely in TT RAM */
		if (start+len > mint_top_tt) {
			return 1;
		}
	}

	/*
	 * a_index is the 256MB number of the page.
	 * b_index is the 16MB number within that 256MB (0-15).
	 * c_index is the 1MB number within that 16MB (0-15)
	 * d_index is the 8K number within that 1MB (0-127).
	 */

	a_index = (int)(start >> TIA_SHIFT) & TIA_MASK;
	b_index = (int)(start >> TIB_SHIFT) & TIB_MASK;
	c_index = (int)(start >> TIC_SHIFT) & TIC_MASK;
	d_index = (int)(start >> TID_SHIFT) & TID_MASK;

	if ((ulong)base_tbl >= TTRAM_START)
		offset = offset_tt_ram;
	else
		offset = 0;

	/* precompute the table addresses */
	tbl_a = (long_desc *)((ulong)&base_tbl[a_index] - offset);
	tbl_b = (long_desc *)((ulong)&tbl_a->tbl_address[b_index] - offset);
	tbl_c = (long_desc *)((ulong)&tbl_b->tbl_address[c_index] - offset);
	tbl = (long_desc *)((ulong)&tbl_c->tbl_address[d_index] - offset);

	dt = tbl->page_type.dt;
	wp = tbl->page_type.wp;
	s = tbl->page_type.s;

	for (;;) {
		/* quickly loop through the 1MB-block */
		for (; len && tbl < (long_desc *)((ulong)&tbl_c->tbl_address[TBLD_SIZE] - offset); tbl++)
		{
			if ((tbl->page_type.dt != dt) ||
			    (tbl->page_type.s != s) ||
			    (tbl->page_type.wp != wp)) {
				/* fail because it's not all the same protection */
				return 0;
			}
			len -= PHYS_PAGESIZE;
		}

		if (len == 0L)
			break;

		/* step to the next d-table */
		tbl_c++;
		/* if crossing a 16MB boundary, get the next c-table */
		if (tbl_c == (long_desc *)((ulong)&tbl_b->tbl_address[TBLC_SIZE] - offset))
		{
			tbl_b++;
			/* if crossing a 256MB boundary, get the next b-table */
			if (tbl_b == (long_desc *)((ulong)&tbl_b->tbl_address[TBLB_SIZE] - offset))
			{
				tbl_a++;
				tbl_b = (long_desc *)((ulong)tbl_a->tbl_address - offset);
            }
			tbl_c = (long_desc *)((ulong)tbl_b->tbl_address - offset);
		}
		tbl = (long_desc *)((ulong)tbl_c->tbl_address - offset);
	}
	/* we passed -- all the pages in question have the same prot. status */
	return (wp << 3) | (s << 2) | dt | 0x8000;
}

static void
mark_pages(long_desc *base_tbl,ulong start,ulong len,
	   ushort dt_val, ushort s_val, ushort wp_val)
{
	int a_index, b_index, c_index, d_index;
	long_desc *tbl, *tbl_a, *tbl_b, *tbl_c;
	ulong oldlen,offset;

	if (no_mem_prot)
		return;

	oldlen = len;

	/*
	 * a_index is the 256MB number of the page.
	 * b_index is the 16MB number within that 256MB (0-15).
	 * c_index is the 1MB number within that 16MB (0-15)
	 * d_index is the 8K number within that 1MB (0-127).
	 */

	a_index = (int)(start >> TIA_SHIFT) & TIA_MASK;
	b_index = (int)(start >> TIB_SHIFT) & TIB_MASK;
	c_index = (int)(start >> TIC_SHIFT) & TIC_MASK;
	d_index = (int)(start >> TID_SHIFT) & TID_MASK;

	if ((ulong)base_tbl >= TTRAM_START)
		offset = offset_tt_ram;
	else
		offset = 0;

	/* precompute the table addresses */
	tbl_a = (long_desc *)((ulong)&base_tbl[a_index] - offset);
	tbl_b = (long_desc *)((ulong)&tbl_a->tbl_address[b_index] - offset);
	tbl_c = (long_desc *)((ulong)&tbl_b->tbl_address[c_index] - offset);
	tbl = (long_desc *)((ulong)&tbl_c->tbl_address[d_index] - offset);

#ifdef MEMPROT_SHORTCUT
	/*
	 * Take a shortcut here: we're done if first page of the region is
	 * already right.
	 */
	/* I don't think this shortcut is a good idea, since while we
	 * are doing Mshrink or Srealloc we may very well have a region
	 * with mixed page types -- ERS
	 */

	if (tbl->page_type.dt == dt_val &&
	    tbl->page_type.s == s_val &&
	    tbl->page_type.wp == wp_val) {
/*
	TRACE(("mark_pages start=%lx len=%lx a:%d b:%d c:%d d:%d (same)",
		start,len,a_index,b_index,c_index,d_index));
*/
		return;
	}

#endif /* MEMPROT_SHORTCUT */
/*
	MP_DEBUG(("mark_pages start=%lx len=%lx a:%d b:%d c:%d d:%d",start,len,a_index,b_index,c_index,d_index));
*/

	for (;;)
	{
		/* quickly loop through the 1MB-block */
		for (; len && tbl < (long_desc *)((ulong)&tbl_c->tbl_address[TBLD_SIZE] - offset); tbl++)
		{
			tbl->page_type.dt = dt_val;
			tbl->page_type.s = s_val;
			tbl->page_type.wp = wp_val;
			len -= PHYS_PAGESIZE;
		}

		if (len == 0L)
			break;

		/* get the next d-table */
		tbl_c++;
		/* if crossing a 16MB boundary, get the next c-table */
		if (tbl_c == (long_desc *)((ulong)&tbl_b->tbl_address[TBLC_SIZE] - offset))
		{
			tbl_b++;
			/* if crossing a 256MB boundary, get the next b-table */
			if (tbl_b == (long_desc *)((ulong)&tbl_b->tbl_address[TBLB_SIZE] - offset))
			{
				tbl_a++;
				tbl_b = (long_desc *)((ulong)tbl_a->tbl_address - offset);
            }
			tbl_c = (long_desc *)((ulong)tbl_b->tbl_address - offset);
		}
		tbl = (long_desc *)((ulong)tbl_c->tbl_address - offset);
	}

	flush_mmu();

	/* On the '020 & '030 we have a logical cache, i.e. the DC & IC are on
	 * the CPU side of the MMU, hence on an MMU context switch we must flush
	 * them too. On the '040, by comparison, we have a physical cache, i.e.
	 * the DC & IC are on the memory side of the MMU, so no DC/IC cache flush
	 * is needed.
	 */
	cpush((void *)start, oldlen);
}

/* get_prot_mode(r): returns the type of protection region r
 * has
 */

int
get_prot_mode(MEMREGION *r)
{
	ulong start;

	if (no_mem_prot)
		return PROT_G;
	start = r->loc;
	start >>= PAGE_SIZE_SHIFT;
	return global_mode_table[start];
}

void
mark_region(MEMREGION *region, short mode, short cmode __attribute__((unused)))
{
	ulong start;
	ulong len;
	ulong i;
	ushort dt_val, s_val, wp_val;
	PROC *proc;
	MEMREGION **mr;

	if (no_mem_prot)
		return;

	start = region->loc;
	len = region->len;

	MP_DEBUG(("mark_region %lx len %lx mode %d",start,len,mode));

	/* Don't do anything if init_tables() has not yet finished */
	if (global_mode_table == 0L)
		return;

#if 0 /* this should not occur any more */
	if (mode == PROT_NOCHANGE) {
	mode = global_mode_table[(start >> PAGE_SIZE_SHIFT)];
	}
#else
	assert(mode != PROT_NOCHANGE);
#endif

	/* mark the global page table */

	memset(&global_mode_table[start >> PAGE_SIZE_SHIFT],mode,(len >> PAGE_SIZE_SHIFT));

	for (proc = proclist; proc; proc = proc->gl_next) {
		if (proc->wait_q == ZOMBIE_Q || proc->wait_q == TSR_Q)
			continue;
		if (!proc->p_mem)
			continue;
		assert(proc->p_mem->page_table);
		if (mode == PROT_I || mode == PROT_G) {
			/* everybody gets the same flags */
			goto notowner;
		}
		if (proc->p_mem->memflags & F_OS_SPECIAL) {
			/* you're special; you get owner flags */
			MP_DEBUG(("mark_region: pid %d is an OS special!",proc->pid));
			goto owner;
		}
		if ((mr = proc->p_mem->mem) != 0) {
			for (i = 0; i < proc->p_mem->num_reg; i++, mr++) {
				if (*mr == region) {
					MP_DEBUG(("mark_region: pid %d is an owner",proc->pid));
	owner:
					dt_val = MMU030_DESCR_TYPE_PAGE;
					s_val = 0;
					wp_val = 0;
					goto gotvals;
				}
			}
		}

	notowner:

/* if you get here you're not an owner, or mode is G or I */
	MP_DEBUG(("mark_region: pid %d gets non-owner modes",proc->pid));

		dt_val = other_dt[mode];
		s_val = other_s[mode];
		wp_val = other_wp[mode];

	gotvals:
		mark_pages(proc->p_mem->page_table,start,len,dt_val,s_val,wp_val);
	}
}

/* special version of mark_region, used for attaching (mode == PROT_P)
   and detaching (mode == PROT_I) a memory region to/from a process. */
void
mark_proc_region(struct memspace *p_mem, MEMREGION *region, short mode, short pid __attribute__((unused)))
{
	ulong start;
	ulong len;
	ushort dt_val, s_val, wp_val;
	short global_mode;

	if (no_mem_prot)
		return;

	start = region->loc;
	len = region->len;
	MP_DEBUG(("mark_proc_region %lx len %lx mode %d for pid %d",
		  start, len, mode, pid));

	global_mode = global_mode_table[(start >> PAGE_SIZE_SHIFT)];

	assert(p_mem && p_mem->page_table);
	if (global_mode == PROT_I || global_mode == PROT_G)
		mode = global_mode;
	else {
		if (p_mem->memflags & F_OS_SPECIAL) {
			/* you're special; you get owner flags */
			MP_DEBUG(("mark_proc_region: pid %d is an OS special!",pid));
			goto owner;
		}
		if (mode == PROT_P) {
			MP_DEBUG(("mark_proc_region: pid %d is an owner",pid));
owner:
			dt_val = MMU030_DESCR_TYPE_PAGE;
			s_val = 0;
			wp_val = 0;
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
	MP_DEBUG(("mark_proc_region: pid %d gets non-owner modes",pid));

	dt_val = other_dt[mode];
	s_val = other_s[mode];
	wp_val = other_wp[mode];

gotvals:
	mark_pages(p_mem->page_table,start,len,dt_val,s_val,wp_val);
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
 * this is an error.  Mode is only really valid if (mode & 0x8000).
 */

int
prot_temp(ulong loc, ulong len, short mode)
{
	int cookie;

	if (no_mem_prot)
		return -1;

	/* round start down to the previous page and len up to the next one. */
	len += loc & MASKBITS;
	loc &= ~MASKBITS;
	len = ROUND(len);

	if (mode == 0 || mode == 1) return 0;	/* do nothing */
	if (mode == -1) {
		assert (curproc->p_mem && curproc->p_mem->page_table);
		cookie = get_page_cookie(curproc->p_mem->page_table,loc,len);

		/* if not all controlled, return status */
		if (cookie == 0 || cookie == 1) return cookie;

		mark_pages(curproc->p_mem->page_table,loc,len,1,0,0);

		return cookie;
	}
	else {
		mark_pages(curproc->p_mem->page_table,loc,len,
			   mode&3,(mode&4)>>2,(mode&8)>>3);
		return 0;
	}
}

#if DEBUG_MMU_TREE
#include "k_fds.h"
#include "mmudump030.c"
#endif

/*
 * init_page_table: fill in the page table for the indicated process. The
 * master page map is consulted for the modes of all pages, and the memory
 * region data structures are consulted to see if this process is the owner
 * of any of those tables.
 *
 * This also sets crp and tc in both ctxts of the process.  If this is the
 * first call, then the CPU tc is cleared, the TT0 and TT1 regs are zapped,
 * and then this proc's crp and tc are loaded into it.
 */

static short mmu_is_set_up = 0;

void
init_page_table (PROC *proc, struct memspace *p_mem)
{
	long_desc *tptr;
	long_desc *tbl_a;		/* top-level table */
	long_desc *tbl_b0;		/* second level, handles $0 nybble */
	long_desc *tbl_b;
	long_desc *tbl_bf;		/* handles $F nybble */
	long_desc *tbl_c;		/* temp pointer to start of 16MB */
	ulong p, q, r;
	int i, j, k, a;
	unsigned int b_tables;
	long g;
	MEMREGION **mr;
	ulong offset;
#if DEBUG_MMU_TREE
	struct mmuinfo info;
#endif

	if (no_mem_prot)
		return;

	assert (p_mem->page_table);

	TRACELOW(("init_page_table (p_mem = %lx)", (unsigned long)p_mem));

	tptr = p_mem->page_table;
	if ((ulong) tptr >= TTRAM_START)
		offset = offset_tt_ram;
	else
		offset = 0;

	if (mint_top_tt)
	{
		b_tables = (unsigned int) ((mint_top_tt + TBLA_MEMSIZE - 1) / TBLA_MEMSIZE) - 1;
	} else
	{
		b_tables = 0;
	}

	tbl_a = tptr;
	tptr += TBLA_SIZE;
	tbl_b0 = tptr;
	tptr += TBLB_SIZE;
	tptr += b_tables * TBLB_SIZE;
	tbl_bf = tptr;
	tptr += TBLB_SIZE;
	tbl_c = tptr;
	tptr += TBLC_SIZE;

	/*
	 * table A indexes by the first nybble: $0 and $F refer to their tables,
	 * $1-$7 are TT-RAM above 256MB, or uncontrolled, cacheable; $8-$E are uncontrolled, ci.
	 * (uncontrolled actually means "supervisor protected", as there's no point
	 * having it fully accessible, especially because the Falcon "mirrors" RAM
	 * every 16 MB, as (IIRC) the address bus is only 24 bit wide - Gryf)
	 */

	tbl_a[0].page_type = page_ptr;
	tbl_a[0].tbl_address = (long_desc *)((ulong)tbl_b0 + offset);

	for (i=1; i < (TBLA_SIZE - 1); i++) {
		if (i < 8)
		{
			/* this will be fixed below, if that region contains TTRAM */
			tbl_a[i].page_type = s_page;
		} else
		{
			tbl_a[i].page_type = s_ci_page;
		}
		tbl_a[i].tbl_address = (long_desc *)((ulong)i << TIA_SHIFT);
	}

	/* $F entry of table A refers to table BF */
	tbl_a[TBLA_SIZE - 1].page_type = page_ptr;
	tbl_a[TBLA_SIZE - 1].tbl_address = (long_desc *)((ulong)tbl_bf + offset);

	/*
	 * table B0: entry 0 is $00, the 16MB of ST address space.
	 */

	tbl_b0[0].page_type = page_ptr;
	tbl_b0[0].tbl_address = (long_desc *)((ulong)tbl_c + offset);

	/* for each megabyte that is RAM, allocate a table */
	for (i = 0, g = 0, p = 0; p < mint_top_st; i++, p += TBLC_MEMSIZE) {
		tbl_c[i].page_type = page_ptr;
		tbl_c[i].tbl_address = (long_desc *)((ulong)tptr + offset);

		/* for each page in this megabyte, write a page entry */
		for (q = p, j = 0; j < TBLD_SIZE && q < mint_top_st; j++, q += PHYS_PAGESIZE, g++) {
			tptr->page_type = *proto_page_type[global_mode_table[g]];
			tptr->tbl_address = (long_desc *)q;
			tptr++;
		}
		for ( ; j < TBLD_SIZE; j++, q += PHYS_PAGESIZE, g++) {
			/* fill in the rest of this MB */
			tptr->page_type = invalid_page;
			tptr->tbl_address = (long_desc *)q;
			tptr++;
		}
	}

	/* now for each megabyte from mint_top_st to ROM, mark global */
	for ( ; p < 0x00E00000L; i++, p += TBLC_MEMSIZE) {
		tbl_c[i].page_type = g_page;
		tbl_c[i].tbl_address = (long_desc *)p;
	}

	/* fill in the E and F tables: 00Ex is ROM, 00Fx is I/O  */
	tbl_c[i].page_type = g_page;
	tbl_c[i].tbl_address = (long_desc *)p;
	i++, p += TBLC_MEMSIZE;
	tbl_c[i].page_type = g_ci_page;
	tbl_c[i].tbl_address = (long_desc *)p;

	/* Done with tbl_c for 0th 16MB; go on to TT RAM */

/*
	structure:

	for (i = each 16MB that has any TT RAM in it)
	allocate a table tbl_c, point tbl_b0[i] at it
	for (j = each 1MB that is RAM)
		allocate a table, point tbl_c[j] at it
		for (k = each page in the megabyte)
		fill in tbl_c[j][k] with page entry from global_mode_table
	for (j = the rest of the 16MB)
		set tbl_c[j] to "supervisor, cacheable"

	for (i = the rest of the 16MBs from here to $7F)
	set tbl_b0[i] to "supervisor, cacheable"
*/

	/* i counts 16MBs */
	a = 0;
	tbl_b = tbl_b0;
	for (i = 1, p = TTRAM_START, g = TTRAM_START / PHYS_PAGESIZE;
	     p < mint_top_tt;
	     )
	{
		tbl_b[i].page_type = page_ptr;
		tbl_b[i].tbl_address = (long_desc *)((ulong)tptr + offset);
		tbl_c = tptr;
		tptr += TBLC_SIZE;

		/* j counts MBs */
		for (j = 0, q = p; j < TBLC_SIZE && q < mint_top_tt; q += TBLC_MEMSIZE, j++) {
			tbl_c[j].page_type = page_ptr;
			tbl_c[j].tbl_address = (long_desc *)((ulong)tptr + offset);
			/* k counts pages (8K) */
			for (r = q, k = 0; k < TBLD_SIZE && r < mint_top_tt; k++, r += PHYS_PAGESIZE, g++) {
				tptr->page_type = *proto_page_type[global_mode_table[g]];
				tptr->tbl_address = (long_desc *)(r + offset_tt_ram);
				tptr++;
			}
			for ( ; k < TBLD_SIZE; k++, r += PHYS_PAGESIZE, g++) {
				/* fill in the rest of this MB */
				tptr->page_type = invalid_page;
				tptr->tbl_address = (long_desc *)(r + offset_tt_ram);
				tptr++;
			}
		}
		for ( ; j < TBLC_SIZE; j++, q += TBLC_MEMSIZE, g += TBLD_SIZE) {
			/* fill in the rest of this 16MB */
			tbl_c[j].page_type = s_page;
			tbl_c[j].tbl_address = (long_desc *)(q + offset_tt_ram);
		}
		i++;
		p += TBLB_MEMSIZE;
		if (i == TBLB_SIZE)
		{
			if (p < mint_top_tt)
			{
				a++;
				tbl_b += TBLB_SIZE;
				i = 0;
				assert(a < (0x80000000UL / TBLA_MEMSIZE));
				tbl_a[a].page_type = page_ptr;
				tbl_a[a].tbl_address = (long_desc *)((ulong)tbl_b + offset);
			}
		}
	}
	assert((ulong)tptr == (ulong)p_mem->page_table + page_table_size);
	assert(tbl_b == (tbl_b0 + b_tables * TBLB_SIZE));

	/* fill in the rest of $00-$0F as supervisor, but cacheable */
	for ( ; i < TBLB_SIZE; i++, p += TBLB_MEMSIZE) {
		if (a == 0)
			tbl_b[i].page_type = s_page;
		else
			tbl_b[i].page_type = invalid_page;
		tbl_b[i].tbl_address = (long_desc *)(p + offset_tt_ram);
	}

	/* done with TT RAM in table b0; do table bf */

	/*
	 * Table BF: translates addresses starting with $F.  First 15 are
	 * uncontrolled, cacheable; last one translates $FF, which
	 * shadows $00 (the 16MB ST address space).  The rest
	 * are uncontrolled, not cacheable.
	 */

	for (i=0; i < (TBLB_SIZE - 1); i++) {
		tbl_bf[i].page_type = g_ci_page;
		tbl_bf[i].tbl_address = (long_desc *)(((ulong)i << TIB_SHIFT) | 0xf0000000L);
	}
	tbl_bf[TBLB_SIZE - 1] = tbl_b0[0];

	proc->ctxt[0].crp.limit = 0x7fff;	/* disable limit function */
	proc->ctxt[0].crp.dt = MMU030_DESCR_TYPE_VALID8;		/* points to valid 8-byte entries */
	proc->ctxt[0].crp.tbl_address = (long_desc *)((ulong)tbl_a + offset);
	proc->ctxt[1].crp = proc->ctxt[0].crp;
	proc->ctxt[0].tc = tc;
	proc->ctxt[1].tc = tc;

#if DEBUG_MMU_TREE
	if (!mmu_is_set_up)
	{
		{
			info.fp = NULL;
			if (FP_ALLOC(rootproc, &info.fp) == 0)
				do_open(&info.fp, "C:\\mmudbg.txt", (O_WRONLY | O_CREAT | O_TRUNC), 0, NULL);
			mmu_printf(&info, "MMU TREE (before mark_pages)\r\n");
			init_mmu_info_030(&info, proc->ctxt[0].tc);
			if ((ulong)proc->ctxt[0].crp.tbl_address >= TTRAM_START)
				info.offset_tt_ram = offset_tt_ram;
			else
				info.offset_tt_ram = 0;
			print_tc_info_030(&info);
			print_rp_info_030(&info, "CRP   ", (const cpuaddr *)&proc->ctxt[0].crp);

			mmu030_print_tree(&info, (const cpuaddr *)&proc->ctxt[0].crp);
			mmu_printf(&info, "\r\n\r\n");
		}
	}
#endif

	/*
	 * OK, memory tables are now there as if you're a non-owner of every
	 * page.  Now for each region you ARE an owner of, mark with owner
	 * modes.
	 */

	mr = p_mem->mem;
	for (i = 0; i < p_mem->num_reg; i++, mr++) {
		if (*mr) {
			mark_pages (p_mem->page_table,(*mr)->loc,(*mr)->len,1,0,0);
		}
	}
	if (p_mem->tp_reg)
		mark_pages (p_mem->page_table,p_mem->tp_reg->loc,p_mem->tp_reg->len,1,0,0);

	if (!mmu_is_set_up)
	{
		DEBUG (("init_page_table: call set_mmu"));

		set_mmu(proc->ctxt[0].crp,proc->ctxt[0].tc);
		mmu_is_set_up = 1;
#if DEBUG_MMU_TREE
		{
			int did_mark = FALSE;

			mr = p_mem->mem;
			for (i = 0; i < p_mem->num_reg; i++, mr++) {
				if (*mr)
					did_mark = TRUE;
			}
			if (p_mem->tp_reg)
				did_mark = TRUE;
			if (did_mark)
			{
				init_mmu_info_030(&info, proc->ctxt[0].tc);
				if ((ulong)proc->ctxt[0].crp.tbl_address >= TTRAM_START)
					info.offset_tt_ram = offset_tt_ram;
				else
					info.offset_tt_ram = 0;
				mmu_printf(&info, "MMU TREE (after mark_pages)\r\n");
				print_tc_info_030(&info);
				print_rp_info_030(&info, "CRP   ", (const cpuaddr *)&proc->ctxt[0].crp);

				mmu030_print_tree(&info, (const cpuaddr *)&proc->ctxt[0].crp);
			} else
			{
				mmu_printf(&info, "MMU TREE (after mark_pages; did not mark anything)\r\n");
			}
			if (info.fp)
				do_close(rootproc, info.fp);
		}
#endif

	}
}

/*
 * This routine is called when procfs detects that a process wants to be an
 * OS SPECIAL.  The AES, SCRENMGR, and DESKTOP do this, and so does FSMGDOS
 * and possibly some other stuff. It has to re-mark every page in that
 * process' page table based on its new special status. The "special
 * status" is "you get global access to all of memory" and "everybody
 * gets Super access to yours."  It is the caller's responsibility
 * to set proc's memflags, usually to (F_OS_SPECIAL | F_PROT_S).
 */

void
mem_prot_special(PROC *proc)
{
	MEMREGION **mr;
	unsigned int i;
	unsigned char mode;

	if (no_mem_prot)
		return;

	TRACE(("mem_prot_special(pid %d)",proc->pid));
	assert (proc->p_mem && proc->p_mem->page_table);

	/*
	 * This marks ALL memory, allocated or not, as accessible. When memory
	 * is freed even F_OS_SPECIAL processes lose access to it. So one or
	 * the other of these is a bug, depending on how you want it to work.
	 */
	mark_pages(proc->p_mem->page_table,0,mint_top_st,1,0,0);
	if (mint_top_tt) {
		mark_pages(proc->p_mem->page_table,
			   TTRAM_START,
			   mint_top_tt - TTRAM_START,
			   1,0,0);
	}

	/*
	 * In addition, mark all the pages the process already owns as "super"
	 * in all other processes, but only if they are still marked "private".
	 * Thus the "special" process can access all of memory, and any process can
	 * access the "special" process' memory when in super mode.
	 */

	mr = proc->p_mem->mem;

	for (i = 0; i < proc->p_mem->num_reg; i++, mr++) {
		if (*mr) {
			mode = global_mode_table[((*mr)->loc >> PAGE_SIZE_SHIFT)];
			if (mode == PROT_P)
				mark_region(*mr, PROT_S, 0);
			else
			{
				MP_DEBUG(("mem_prot_special: Not marking region at %lx as super", (*mr)->loc));
			}
		}
	}
}

#include "mprot.x"

#ifdef DEBUG_INFO
/*----------------------------------------------------------------------------
 * DEBUGGING SECTION
 *--------------------------------------------------------------------------*/

static void
_dump_tree(long_desc *tbl, int level)
{
	int i, j;
	long_desc *p;
	ulong offset;
	static const char spaces[9] = "        ";

	if ((ulong) tbl->tbl_address >= TTRAM_START)
		offset = offset_tt_ram;
	else
		offset = 0;

	/* print the level and display the table descriptor */
	FORCE("\r%s s:%x wp:%x dt:%x a:%08lx",
	&spaces[8-(level*2)],
	tbl->page_type.s,
	tbl->page_type.wp,
	tbl->page_type.dt,
	(unsigned long)((ulong)tbl->tbl_address));

	if (tbl->page_type.dt == MMU030_DESCR_TYPE_VALID8) {
		if (level == 0) {
			j = (1 << tc.tia);
		}
		else if (level == 1) {
			j = (1 << tc.tib);
		}
		else if (level == 2) {
			j = (1 << tc.tic);
		}
		else {
			j = (1 << tc.tid);
		}

		++level;
		p = (long_desc *)((ulong)tbl->tbl_address - offset);
		for (i=0; i<j; i++, p++) {
			_dump_tree(p,level);
		}
	}
}
#endif

static const char modesym[] = { 'p', 'g', 's', 'r', 'i' };

void
QUICKDUMP(void)
{
	char outstr[33];
	ulong i, j, end;

	if (no_mem_prot)
		return;

	FORCE("STRAM global table:");
	outstr[32] = '\0';
	end = mint_top_st / PHYS_PAGESIZE;
	for (i = 0; i < end; i += 32) {
		for (j=0; j<32; j++) {
			outstr[j] = modesym[global_mode_table[j+i]];
		}
		FORCE("%08lx: %s",i*PHYS_PAGESIZE,outstr);
	}

	if (mint_top_tt) {
		FORCE("TTRAM global table:");
		end = mint_top_tt / PHYS_PAGESIZE;
		for (i = TTRAM_START / PHYS_PAGESIZE; i < end; i += 32) {
			for (j=0; j<32; j++) {
			outstr[j] = modesym[global_mode_table[j+i]];
			}
			FORCE("%08lx: %s",i*PHYS_PAGESIZE,outstr);
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

void
BIG_MEM_DUMP(int bigone, PROC *proc)
{
#ifdef DEBUG_INFO
	char linebuf[128];
	char *lp = linebuf;
	long len = sizeof(linebuf);
	MEMREGION *mp, **mr, **map;
	PROC *p;
	ulong loc;
	short owner;
	unsigned int i;
	short first;


	if (no_mem_prot) return;

	for (map = core; map != 0; ((map == core) ? (map = alt) : (map = 0))) {
		FORCE("Annotated memory dump for %s",(map == core ? "core" : "alt"));
		first = 1;
		*linebuf = '\0';
		len = 0;
		for (mp = *map; mp; mp = mp->next) {
			for (loc = mp->loc; loc < (mp->loc + mp->len); loc += PHYS_PAGESIZE) {
				if (first || ((loc & 0x1ffffL) == 0)) {
					if (*linebuf) FORCE(linebuf);
					len = ksprintf(linebuf,sizeof(linebuf),"\r%08lx: ",loc);
					lp = linebuf + len;
					first = 0;
				}
				if (loc == mp->loc) {
					*lp++ = modesym[global_mode_table[loc / PHYS_PAGESIZE]];
					len++;

					for (p = proclist; p; p = p->gl_next) {
						if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
							continue;
						if (p->p_mem && p->p_mem->mem) {
							mr = p->p_mem->mem;
							for (i = 0; i < p->p_mem->num_reg; i++, mr++) {
								if (*mr == mp) {
									owner = p->pid;
									goto gotowner;
								}
							}
						}
					}
					owner = 000;
gotowner:
					len += ksprintf(lp,sizeof(linebuf) - len,"%03d",owner);
				}
				else {
					*lp++ = ' ';
					*lp++ = '-';
					*lp++ = '-';
					*lp++ = '-';
					*lp = '\0';	/* string is always null-terminated */
					len += 4;
				}
			}
		}
		FORCE(linebuf);
	}

	if (bigone) {
		long_desc tbl;

		/* fill in tbl with the only parts used at the top level */
		tbl.page_type.dt = proc->ctxt[CURRENT].crp.dt;
		tbl.tbl_address = proc->ctxt[CURRENT].crp.tbl_address;
		_dump_tree(&tbl,0);
	}
#else
	UNUSED(proc);
	UNUSED(bigone);
#endif /* DEBUG_INFO */
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
	unsigned int i;

	if (no_mem_prot) return -1;
	if (start >= (ulong)p && start+nbytes <= (ulong)(p+1))
		return -1;
	if (p == rootproc)
		goto win_and_mark;

	if (p->p_mem) {
		mr = p->p_mem->mem;
		if (mr) {
			for (i = 0; i < p->p_mem->num_reg; i++, mr++) {
				if (*mr) {
					if (((*mr)->loc <= start) &&
					   ((*mr)->loc + (*mr)->len >= start + nbytes))
						goto win_and_mark;
				}
			}
		}
	}

	return 0;	/* we don't own this memory */

win_and_mark:
	return prot_temp(start, nbytes, -1);
}

#endif
