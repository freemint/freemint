/* defines and type declarations to access the */
/* lance of Riebl Card (Plus), TUM, PAMs EMEGA, PAMs VME (TT) */

#define u_char  unsigned char
#define u_short unsigned short
#define u_long  unsigned long

#define MFP_GPIP	*(volatile char *)0xFFFFFA01L

typedef u_char HADDR[6];	/* 6-byte hardware address of lance */

#define DRX	0x0001
#define DTX	0x0002
#define LOOP	0x0004
#define DTCR	0x0008
#define COLL	0x0010
#define DRTY	0x0020
#define INTL	0x0040
#define PROM	0x8000

typedef struct			/* pointer to descriptor ringbuffer */
{
	u_short drp_lo;
	u_char	len;
	u_char	drp_hi;
} DRP;


typedef struct lnc_init		/* initblock of lance */
{
	u_short	mode;		/* mode word */
	HADDR	haddr;		/* hardware address of lance */
	u_long	laf[2];		/* logical adress filter */
	DRP		rdrp;	/* receive ring descr. pointer */
	DRP		tdrp;	/* xmit ring descr. pointer */
} LNCINIT;



typedef struct		/* transmit message descriptor block */
{
	u_short	ladr;	/* TMD0 */
	u_short tmd1;
	u_short	tmd2;
	u_short tmd3;
} TMD;

	/* TMD1 */
#define	ENP	0x0100
#define STP	0x0200
#define	DEF	0x0400
#define ONE	0x0800
#define	MORE	0x1000
#define	ERR	0x4000
#define OWN 	0x8000

#define OWN_CHIP	 OWN
#define OWN_HOST	!OWN

#define TDR	0x03FF		/* TMD3 */
#define RTRY	0x0400
#define LCAR	0x0800
#define LCOL	0x1000
#define UFLO	0x4000
#define BUFF3	0x8000

typedef struct			/* receive message descriptor block */
{
	u_short	ladr;		/* RMD0 */
	u_short	rmd1;
	u_short rmd2;
	short	mcnt;		/* RMD3 */
} RMD;

	/* RMD1 + bits in tmd1 */
#define BUFF	0x0400
#define CRC	0x0800
#define OFLO	0x1000
#define FRAM	0x2000
	
#define BCNT	0x0FFF		/* RMD2 */
#define ONES	0xF000


#define CSR0	0	/* register address in register address port */
#define CSR1	1
#define CSR2	2
#define CSR3	3


#define CSR0_INIT	0x0001		/* bits of CSR0 */
#define CSR0_STRT	0x0002
#define CSR0_STOP	0x0004
#define CSR0_TDMD	0x0008
#define CSR0_TXON	0x0010
#define CSR0_RXON	0x0020
#define CSR0_INEA	0x0040
#define CSR0_INTR	0x0080
#define CSR0_IDON	0x0100
#define CSR0_TINT	0x0200
#define CSR0_RINT	0x0400
#define CSR0_MERR	0x0800
#define CSR0_MISS	0x1000
#define CSR0_CERR	0x2000
#define CSR0_BABL	0x4000
#define CSR0_ERR	0x8000

#define CSR3_BCON	0x0001		/* bits of CSR3 */
#define CSR3_ACON	0x0002
#define CSR3_BSWP	0x0004


#define HBI  0x1A
#define BERR 0x02

#ifdef PAMs_INTERN

#define RCP_RDP (int *)0xFECFFFF0L	/* register data port addr */
#define RCP_RAP	(int *)0xFECFFFF2L	/* register address port addr */
#define LANCEIVEC 0x1D			/* Beim MEGA ist IPL 3, bei VME
					   IPL 5 codiert */
#define LANCEIVECREG (char *)0xFECFFFF7L
#define LANCE_EEPROM (char *)0xFECFFFFDL
#define LANCE_MEM (char *)0xFECFFFFFL
#define RCP_MEMBOT (char *)0xFECF0000L	/* start of memory on PAMs card */

#else

#define LANCEIVECREG (char *)(RCP_MEMBOT+0xFFFF)
#define RIEBL

#ifdef MEGA_STE

#define RCP_RDP (int *)0xC0FFF0L	/* register data port addr */
#define RCP_RAP	(int *)0xC0FFF2L	/* register address port addr */
#define LANCEIVEC 0x50
#define RCP_MEMBOT (char *)0xC10000L	/* start of memory on rieblcard */

#else

#ifdef ATARI_TT

#define RCP_RDP (int *)0xFE00FFF0L	/* register data port addr */
#define RCP_RAP	(int *)0xFE00FFF2L	/* register address port addr */
#define LANCEIVEC 0x50
#define RCP_MEMBOT (char *)0xFE010000L	/* start of memory on rieblcard */

#else

#ifdef SPECIAL /* Riebl MEGA ST with swapped A20 <-> A21 */

#define RCP_RDP (int *)0xFFFF7000L	/* register data port addr */
#define RCP_RAP (int *)0xFFFF7002L	/* register address port addr */
#define LANCEIVEC 0x1d
#define RCP_MEMBOT (char *)0xFED00000L	/* start of memory on rieblcard */

#else

#ifdef MEGA_ST

#define RCP_RDP (int *)0xFFFF7000L	/* register data port addr */
#define RCP_RAP	(int *)0xFFFF7002L	/* register address port addr */
#define LANCEIVEC 0x1d
#define RCP_MEMBOT (char *)0xFEE00000L	/* start of memory on rieblcard */

#endif
#endif
#endif
#endif
#endif

#define RECVBUFFS	32			/* max number of received packets */
#define XMITBUFFS	4			/* max number of unsent packets */
#define RECVRLEN	(0x05 << 5)
#define XMITRLEN	(0x02 << 5)


#define MAXPKTLEN	1518

#define write_register(r,v)	*RCP_RAP=(r);*RCP_RDP=(v)
#define read_register(r,v)	*RCP_RAP=r;v=*RCP_RDP

typedef struct
{
	HADDR	et_dest;
	HADDR	et_src;
	u_short	et_type;
	char	et_data[MAXPKTLEN-14];
} PKTBUF;


typedef struct
{
	LNCINIT	init;
	TMD		tmd[XMITBUFFS];		/* transmit descriptor blocks */
	RMD		rmd[RECVBUFFS];		/* receive descriptor blocks */
} LNCMEM;


#define MAXPKT	40
#define INIT (((LNCMEM *)RCP_MEMBOT)->init)
#define PRMD (((LNCMEM *)RCP_MEMBOT)->rmd)
#define PTMD (((LNCMEM *)RCP_MEMBOT)->tmd)
#define PPKT (PKTBUF *)((u_long)(RCP_MEMBOT + sizeof(LNCMEM) + 7) & ~7ul)

