typedef unsigned long cpureg;
typedef unsigned long cpuaddr;
#define PREG "0x%08lx"
#define PREG64 "0x%08lx%08lx"

#define BITS1(shift)	((cpureg)0x00000001UL << (shift))
#define BITS2(shift)	((cpureg)0x00000003UL << (shift))
#define BITS3(shift)	((cpureg)0x00000007UL << (shift))
#define BITS4(shift)	((cpureg)0x0000000fUL << (shift))
#define BITS8(shift)	((cpureg)0x000000ffUL << (shift))
#define BITS15(shift)	((cpureg)0x00007fffUL << (shift))
#define BITS16(shift)	((cpureg)0x0000ffffUL << (shift))
#define BITS24(shift)	((cpureg)0x00ffffffUL << (shift))
#define BITS28(shift)	((cpureg)0x0fffffffUL << (shift))
#define BITS30(shift)	((cpureg)0x3fffffffUL << (shift))

#define MMU_CACHE_WRITETHROUGH	0x00	/* writethrough, cacheable */
#define MMU_CACHE_COPYBACK		0x01	/* copyback, cacheable */
#define MMU_CACHE_PRECISE		0x02	/* Cache-inhibited, precise exception model */
#define MMU_CACHE_IMPRECISE		0x03	/* Cache-inhibited, imprecise exception model */

/*
 * Translate control register bit meanings (030/68851)
 *
 * x--- ---- ---- ---- ---- ---- ---- ----
 * translation: 1 = enable, 0 = disable
 *
 * ---- --x- ---- ---- ---- ---- ---- ----
 * supervisor root: 1 = enable, 0 = disable
 *
 * ---- ---x ---- ---- ---- ---- ---- ----
 * function code lookup: 1 = enable, 0 = disable
 *
 * ---- ---- xxxx ---- ---- ---- ---- ----
 * page size:
 * 1000 = 256 bytes
 * 1001 = 512 bytes
 * 1010 =  1 kB
 * 1011 =  2 kB
 * 1100 =  4 kB
 * 1101 =  8 kB
 * 1110 = 16 kB
 * 1111 = 32 kB
 *
 * ---- ---- ---- xxxx ---- ---- ---- ----
 * initial shift
 *
 * ---- ---- ---- ---- xxxx ---- ---- ----
 * number of bits for table index A
 *
 * ---- ---- ---- ---- ---- xxxx ---- ----
 * number of bits for table index B
 *
 * ---- ---- ---- ---- ---- ---- xxxx ----
 * number of bits for table index C
 *
 * ---- ---- ---- ---- ---- ----- ---- xxxx
 * number of bits for table index D
 */
#define MMU030_TC_ENABLED  			BITS1(31)	/* Translation Enable (0 = disabled) */
#define MMU030_TC_SRE				BITS1(25)
#define MMU030_TC_FCL				BITS1(24)
#define MMU030_TC_PAGESIZE_SHIFT	20
#define MMU030_TC_PAGESIZE_MASK		BITS4(MMU030_TC_PAGESIZE_SHIFT)
#define MMU030_TC_IS_SHIFT			16
#define MMU030_TC_IS_MASK			BITS4(MMU030_TC_IS_SHIFT)
#define MMU030_TC_TIA_SHIFT			12
#define MMU030_TC_TIA_MASK			BITS4(MMU030_TC_TIA_SHIFT)
#define MMU030_TC_TIB_SHIFT			8
#define MMU030_TC_TIB_MASK			BITS4(MMU030_TC_TIB_SHIFT)
#define MMU030_TC_TIC_SHIFT			4
#define MMU030_TC_TIC_MASK			BITS4(MMU030_TC_TIC_SHIFT)
#define MMU030_TC_TID_SHIFT			0
#define MMU030_TC_TID_MASK			BITS4(MMU030_TC_TID_SHIFT)
#define MMU030_TC_VALID_MASK (\
	MMU030_TC_ENABLED | \
	MMU030_TC_SRE | \
	MMU030_TC_FCL | \
	MMU030_TC_PAGESIZE_MASK | \
	MMU030_TC_IS_MASK | \
	MMU030_TC_TIA_MASK | \
	MMU030_TC_TIB_MASK | \
	MMU030_TC_TIC_MASK | \
	MMU030_TC_TID_MASK)


/*
 * Translate control register bit meanings (040/060)
 */
									/* Bit 0 is zero */
#define MMU040_TC_DUI_SHIFT			1
#define MMU040_TC_DUI_MASK			BITS2(MMU040_TC_DUI_SHIFT)	/* Default UPA Bits (instruction cache, 060 only) */
#define MMU040_TC_DCI_SHIFT			3
#define MMU040_TC_DCI_MASK			BITS2(MMU040_TC_DCI_SHIFT)	/* Default cache Mode (instruction cache, 060 only) */
#define MMU040_TC_DWO         		BITS1(5)					/* Default write protect (data cache, 060 only) */
#define MMU040_TC_DUO_SHIFT			6
#define MMU040_TC_DUO_MASK			BITS2(MMU040_TC_DUO_SHIFT)	/* Default UPA Bits (data cache, 060 only) */
#define MMU040_TC_DCO_SHIFT			8
#define MMU040_TC_DCO_MASK			BITS2(MMU040_TC_DCO_SHIFT)	/* Default cache mode (data cache, 060 only) */
#define MMU040_TC_FITC    			BITS1(10)					/* 1/2-Cache Mode (instruction ATC, 060 only) */
#define MMU040_TC_FOTC     			BITS1(11)					/* 1/2-Cache Mode (data ATC, 060 only) */
#define MMU040_TC_NAI     			BITS1(12)					/* No Allocate Mode (instruction ATC, 060 only) */
#define MMU040_TC_NAD     			BITS1(13)					/* No Allocate Mode (data ATC, 060 only) */
#define MMU040_TC_PAGESIZE			BITS1(14)					/* Page size select (0 = 4Kb, 1 = 8Kb) */
#define MMU040_TC_ENABLED  			BITS1(15)					/* Translation Enable (0 = disabled) */
#define MMU040_TC_VALID_MASK (MMU040_TC_ENABLED | MMU040_TC_PAGESIZE)
#define MMU060_TC_VALID_MASK (MMU040_TC_VALID_MASK | \
	MMU040_TC_DUI_MASK | \
	MMU040_TC_DCI_MASK | \
	MMU040_TC_DWO | \
	MMU040_TC_DUO_MASK | \
	MMU040_TC_DCO_MASK | \
	MMU040_TC_FITC | \
	MMU040_TC_FOTC | \
	MMU040_TC_NAI | \
	MMU040_TC_NAD)


/*
 * Transparent Translation Register definitions (040/060)
 */
#define MMU040_TT_ENABLED				BITS1(15)
#define MMU040_TT_SFIELD_ANY			BITS1(14)
#define MMU040_TT_SFIELD_SUPER			BITS1(13)
#define MMU040_TT_UX_SHIFT				8
#define MMU040_TT_UX_MASK				BITS2(MMU040_TT_UX_SHIFT)
#define MMU040_TT_CACHE_SHIFT			5
#define MMU040_TT_CACHE_MASK			BITS2(MMU040_TT_CACHE_SHIFT)
#define MMU040_TT_WRITE_PROTECT			BITS1(2)
#define MMU040_TT_VALID_MASK (\
	MMU040_TT_ENABLED | \
	MMU040_TT_SFIELD_ANY | \
	MMU040_TT_SFIELD_SUPER | \
	MMU040_TT_UX_MASK | \
	MMU040_TT_CACHE_MASK | \
	MMU040_TT_WRITE_PROTECT)

/*
 * Transparent Translation Register definitions (030)
 */
#define MMU030_TT_FC_MASK_SHIFT		0
#define MMU030_TT_FC_MASK_MASK		BITS3(MMU030_TT_FC_MASK_SHIFT)
#define MMU030_TT_FC_BASE_SHIFT		4
#define MMU030_TT_FC_BASE_MASK		BITS3(MMU030_TT_FC_BASE_SHIFT)
#define MMU030_TT_RWM				BITS1(8)
#define MMU030_TT_RW				BITS1(9)
#define MMU030_TT_CI				BITS1(10)
#define MMU030_TT_ENABLED			BITS1(15)
#define MMU030_TT_ADDR_MASK_SHIFT	16
#define MMU030_TT_ADDR_MASK_MASK	BITS8(MMU030_TT_ADDR_MASK_SHIFT)
#define MMU030_TT_ADDR_BASE_SHIFT	24
#define MMU030_TT_ADDR_BASE_MASK	BITS8(MMU030_TT_ADDR_BASE_SHIFT)
#define MMU030_TT_VALID_MASK (\
	MMU030_TT_FC_MASK_MASK | \
	MMU030_TT_FC_BASE_MASK | \
	MMU030_TT_RWM | \
	MMU030_TT_RW | \
	MMU030_TT_CI | \
	MMU030_TT_ENABLED | \
	MMU030_TT_ADDR_MASK_MASK | \
	MMU030_TT_ADDR_BASE_MASK)

/* MMU Status Register (030/68851)
 *
 * ---x ---x x-xx x---
 * reserved (all 0)
 *
 * x--- ---- ---- ----
 * bus error
 *
 * -x-- ---- ---- ----
 * limit violation
 *
 * --x- ---- ---- ----
 * supervisor only
 *
 * ---- x--- ---- ----
 * write protected
 *
 * ---- -x-- ---- ----
 * invalid translation
 *
 * ---- --x- ---- ----
 * modified
 *
 * ---- ---- -x-- ----
 * transparent access
 *
 * ---- ---- ---- -xxx
 * number of levels (number of tables accessed during search)
 *
 */

#define MMU030_MMUSR_BUS_ERROR         BITS1(15)
#define MMU030_MMUSR_LIMIT_VIOLATION   BITS1(14)
#define MMU030_MMUSR_SUPER_VIOLATION   BITS1(13)
#define MMU030_MMUSR_WRITE_PROTECTED   BITS1(11)
#define MMU030_MMUSR_INVALID           BITS1(10)
#define MMU030_MMUSR_MODIFIED          BITS1( 9)
#define MMU030_MMUSR_TRANSP_ACCESS     BITS1( 6)
#define MMU030_MMUSR_NUM_LEVELS_SHIFT         0
#define MMU030_MMUSR_NUM_LEVELS_MASK   BITS3(MMU030_MMUSR_NUM_LEVELS_SHIFT)


/*
 * Descriptor bit definitions
 */
#define MMU040_DES_RESIDENT		BITS1( 0)
#define MMU040_DES_VALID		BITS1( 1)
#define MMU040_DES_WRITEPROTECT	BITS1( 2)
#define MMU040_DES_USED			BITS1( 3)
#define MMU040_DES_MODIFIED		BITS1( 4)
#define MMU040_DES_CACHE_SHIFT	       5
#define MMU040_DES_CACHE_MASK	BITS2(MMU040_DES_CACHE_SHIFT)
#define MMU040_DES_SUPER		BITS1( 7)
#define MMU040_DES_U0			BITS1( 8)
#define MMU040_DES_U1			BITS1( 9)
#define MMU040_DES_GLOBAL		BITS1(10)
#define MMU040_DES_UR0			BITS1(11)
#define MMU040_DES_UR1			BITS1(12)
#define MMU040_PAGE_UR_MASK_8	(MMU040_DES_UR0 | MMU040_DES_UR1)
#define MMU040_PAGE_UR_MASK_4	(MMU040_DES_UR0)


#define MMU040_ROOT_PTR_ADDR_MASK		0xFFFFFE00UL
#define MMU040_PTR_PAGE_ADDR_MASK_8		0xFFFFFF80UL
#define MMU040_PTR_PAGE_ADDR_MASK_4		0xFFFFFF00UL
#define MMU040_PAGE_ADDR_MASK_8			0xFFFFE000UL
#define MMU040_PAGE_ADDR_MASK_4			0xFFFFF000UL
#define MMU040_PAGE_INDIRECT_MASK		0xFFFFFFFCUL


/*
 * Descriptors (030)
 */
#define MMU030_DESCR_SHIFT				0
#define MMU030_DESCR_MASK				BITS2(MMU030_DESCR_SHIFT)
#define MMU030_DESCR_TYPE_INVALID		0	/* all tables */

#define MMU030_DESCR_TYPE_EARLY_TERM	1	/* all but lowest level table */
#define MMU030_DESCR_TYPE_PAGE			1	/* only lowest level table */
#define MMU030_DESCR_TYPE_VALID4		2	/* all but lowest level table */
#define MMU030_DESCR_TYPE_INDIRECT4		2	/* only lowest level table */
#define MMU030_DESCR_TYPE_VALID8		3	/* all but lowest level table */
#define MMU030_DESCR_TYPE_INDIRECT8		3	/* only lowest level table */

#define MMU030_DESCR_WP				BITS1(2)	/* write protect; all descr */
#define MMU030_DESCR_U				BITS1(3)	/* update; all descr */
#define MMU030_DESCR_M				BITS1(4)	/* modified; only last level table */
									/* bit 5 zero */
#define MMU030_DESCR_CI				BITS1(6)	/* cache inhibit; only last level table */
									/* bit 7 zero */
/* only for long descriptors */
#define MMU030_DESCR_S				BITS1(8)	/* supervisor */
									/* bits 9-15 zero */
#define MMU030_DESCR_LIMIT_SHIFT	16
#define MMU030_DESCR_LIMIT_MASK		BITS15(MMU030_DESCR_LIMIT_SHIFT)
#define MMU030_DESCR_LOWER			BITS1(31)

#define MMU030_DESCR_VALID4_MASK	(MMU030_DESCR_MASK | MMU030_DESCR_WP | MMU030_DESCR_U | MMU030_DESCR_M | MMU030_DESCR_CI)
#define MMU030_DESCR_VALID8_MASK	(MMU030_DESCR_VALID4_MASK | MMU030_DESCR_S | MMU030_DESCR_LIMIT_MASK | MMU030_DESCR_LOWER)


/* Root Pointer Registers (SRP and CRP, 030)
 *
 * ---- ---- ---- ---- xxxx xxxx xxxx xx-- ---- ---- ---- ---- ---- ---- ---- xxxx
 * reserved, must be 0
 *
 * ---- ---- ---- ---- ---- ---- ---- ---- xxxx xxxx xxxx xxxx xxxx xxxx xxxx ----
 * table A address
 *
 * ---- ---- ---- ---- ---- ---- ---- --xx ---- ---- ---- ---- ---- ---- ---- ----
 * descriptor type
 *
 * -xxx xxxx xxxx xxxx ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
 * limit
 *
 * x--- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
 * 0 = upper limit, 1 = lower limit
 *
 */

#define MMU030_RP0_VALID_MASK	(MMU030_DESCR_LOWER | MMU030_DESCR_LIMIT_MASK | MMU030_DESCR_MASK)

#define MMU030_RP1_ADDR_SHIFT	4
#define MMU030_RP1_ADDR_MASK	BITS28(MMU030_RP1_ADDR_SHIFT)
#define MMU030_RP1_VALID_MASK	(MMU030_RP1_ADDR_MASK)

#define MMU030_DESCR_TD_ADDR_MASK BITS28(4)
#define MMU030_DESCR_PD_ADDR_MASK BITS24(8)
#define MMU030_DESCR_ID_ADDR_MASK 0xFFFFFFFC


/*
 * cache control registers:
 */
#define CACR030_ALLOC_WRITE	BITS1(13)	/* write allocate mode (68030) */
#define CACR030_ENABLE_DB	BITS1(12)	/* enable data burst (68030) */
#define CACR030_CLR_D		BITS1(11)	/* clear data cache (68030) */
#define CACR030_CLRE_D		BITS1(10)	/* clear entry in data cache (68030) */
#define CACR030_FREEZE_D	BITS1( 9)	/* freeze data cache (68030) */
#define CACR030_ENABLE_D	BITS1( 8)	/* enable data cache (68030) */
#define CACR030_ENABLE_IB	BITS1( 4)	/* enable instruction burst (68030) */
#define CACR020_CLR_I		BITS1( 3)	/* clear instruction cache (680[23]0) */
#define CACR020_CLRE_I		BITS1( 2)	/* clear entry in instruction cache (680[23]0) */
#define CACR020_FREEZE_I	BITS1( 1)	/* freeze instruction cache (680[23]0) */
#define CACR020_ENABLE_I	BITS1( 0)	/* enable instruction cache (680[23]0) */

#define CACR020_VALID_MASK (CACR020_ENABLE_I | CACR020_FREEZE_I) /* CACR020_CLR_I & CACR020_CLRE_I always read as zero */
#define CACR030_VALID_MASK (CACR020_VALID_MASK | \
	CACR030_ENABLE_IB | \
	CACR030_ENABLE_D | \
	CACR030_FREEZE_D | \
	CACR030_ENABLE_DB | \
	CACR030_ALLOC_WRITE \
	/* CACR030_CLR_D & CACR030_CLRE_D always read as zero */ \
	)

#define CACR040_ENABLE_D	BITS1(31)	/* enable data cache (680[46]0) */
#define CACR060_FREEZE_D	BITS1(30)	/* freeze data cache (68060) */
#define CACR060_ENABLE_SB	BITS1(29)	/* enable store buffer (68060) */
#define CACR060_PUSH_DPI	BITS1(28)	/* disable CPUSH invalidation (68060) */
#define CACR060_HALF_D		BITS1(27)	/* half-cache mode for data cache (68060) */
#define CACR060_ENABLE_B	BITS1(23)	/* enable branch cache (68060) */
#define CACR060_CLRA_B		BITS1(22)	/* clear all entries in branch cache (68060) */
#define CACR060_CLRU_B		BITS1(21)	/* clear user entries in branch cache (68060) */
#define CACR040_ENABLE_I	BITS1(15)	/* enable instruction cache (680[46]0) */
#define CACR060_FREEZE_I	BITS1(14)	/* freeze instruction cache (68060) */
#define CACR060_HALF_I		BITS1(13)	/* half-cache mode for instruction cache (68060) */

#define CACR040_VALID_MASK	(CACR040_ENABLE_I | CACR040_ENABLE_D)
#define CACR060_VALID_MASK (CACR040_VALID_MASK | \
	CACR060_FREEZE_D | \
	CACR060_ENABLE_SB | \
	CACR060_PUSH_DPI | \
	CACR060_HALF_D | \
	CACR060_ENABLE_B | \
	CACR060_CLRA_B | \
	CACR060_CLRU_B | \
	CACR060_FREEZE_I | \
	CACR060_HALF_I)

/*
 * PCR (processor configuration, 060 only)
 */	
#define MMU060_PCR_ESS				BITS1(0)	/* 68060 enable superscalar dispatch */
#define MMU060_PCR_DFP				BITS1(1)	/* 68060 disable floating point unit */
#define MMU060_PCR_I14_I15			BITS1(5)	/* errata I14/I15 workaround */
#define MMU060_PCR_DEBUG			BITS1(7)	/* enable debug features */
#define MMU060_PCR_REVISION_SHIFT	8
#define MMU060_PCR_REVISION_MASK	BITS8(MMU060_PCR_REVISION_SHIFT)
#define MMU060_PCR_ID_SHIFT			16
#define MMU060_PCR_ID_MASK			BITS16(MMU060_PCR_ID_SHIFT)
#define MMU060_PCR_VALID_MASK		(MMU060_PCR_ESS | MMU060_PCR_DFP | MMU060_PCR_DEBUG)

struct mmuinfo {
	long cpu;
	long fpu;
	bool mmu_enabled;
	bool mmu_valid;
	unsigned int page_size_shift;
	cpuaddr page_size;
	cpuaddr page_mask;
	tc_reg tc;
		
    /* Translation tables (030/68851) */
    struct {
        struct {
            cpureg mask;
            cpureg unused_fields_mask;
            unsigned char shift;
            unsigned char bits;
        } table[4];
        
        struct {
            cpureg mask;
        } page;
        
        unsigned char init_shift;
        unsigned char last_table;
    } translation;
};
