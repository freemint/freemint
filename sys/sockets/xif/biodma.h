/* defines and type declarations to access the */
/* DP8390 of the BIODATA DMA adaptor */

#define u_char  unsigned char
#define u_short unsigned short
#define u_long  unsigned long

typedef u_char HADDR[6];	/* 6-byte hardware address of lance */

typedef struct
{
	u_long	name;
	u_long	val;
} COOKIE;

#define COOKIEBASE (*(COOKIE **)0x5a0)

typedef struct			/* DMA buffer structure for reading */
{
	u_short	header;		/* Intel format! */
	u_short	length;		/* Intel format! */
	union {
		u_char	bytes[2044];	/* data buffer   */
		u_long  longs[510];
	} data;
} DMABUFFER;

#define MIN_LEN 60

/*
 * Definitions of commands understood by the BIODATA DMA adaptor.
 *
 * TUR      stands for Test Unit Ready and is *NOT* understood by the BIONET
 *          DMA adaptor. Since reading the ethernet address does not conform
 *          to a normal ACSI access sequence there is first a test performed
 *          if a regular ACSI device answers at a given target address before
 *          the BIONET initialization sequence is executed...
 * READPKT  reads a packet received by the DMA adaptor.
 *          Command length is 6 bytes.
 * WRITEPKT sends a packet transferred by the following DMA phase.
 *          Command length is 6 bytes.
 * SETSTAT  sends a status byte to the BIONET DMA adaptor.
 * GETETH   gets the ethernet address from the BIONET DMA adaptor.
 */

enum {TUR=0, READPKT=8, WRITEPKT=10, SETSTAT=14, GETETH=15};

/* Definition for the status byte used in SETSTAT   */
/* These directly reflect some status of the DP8390 */

#define	SAVEERR		 1	/* Save error packets                        */
#define RUNNED		 2	/* Accept runned packets (packets < 64bytes) */
#define BROADCAST	 4	/* Accept broadcasts                         */
#define MULTICAST	 8	/* Accept multicasts (not implemented)       */
#define PROMISCUOUS	32	/* Use promiscuous mode (receive all pkts)   */
#define	MONITOR		64	/* Monitor mode (not implemented)            */

/* Some specials for ACSI command phase */

#define NORMAL	0
#define	BIOMODE	1

#define	NODMA	0
#define	DMA	1

/* Atari system variables */

#define VBI   0x1C
#define HZ200 (*(volatile long *)0x4baL)
#define FLOCK (*(volatile u_short *)0x43eL)

/* Locking functions */

#define LOCK 							\
	({	short __sr;					\
		u_short __flock;				\
		__sr = spl7 ();					\
		__flock = ((FLOCK == 0) ? (FLOCK = 0x1) : 0);	\
		spl (__sr);					\
		__flock;					\
	})
#define UNLOCK (FLOCK = 0)

/* Atari HW definitions */

#define ACCESS  0xFFFF8604L
#define DACCESS (*(volatile u_short *)ACCESS)
#define MODE    0xFFFF8606L
#define DMODE   (*(volatile u_short *)MODE)
#define DBOTH   (*(volatile u_long *)MODE)
#define DMAHIGH (*(volatile u_char *)0xFFFF8609L)
#define DMAMID  (*(volatile u_char *)0xFFFF860BL)
#define DMALOW  (*(volatile u_char *)0xFFFF860DL)

#define MFP_GPIP (*(volatile char *)0xFFFFFA01L)

/* Some useful functions */

#define INT      (!(MFP_GPIP & 0x20))
#define DELAY ({MFP_GPIP; MFP_GPIP; MFP_GPIP;})
#define WRITEMODE(value)					\
	({	u_short dummy = value;				\
		__asm__ volatile("movew %0, 0xFFFF8606" : : "d"(dummy));	\
		DELAY;						\
	})
#define WRITEBOTH(value1, value2)				\
	({	u_long dummy = (u_long)(value1)<<16 | (u_short)(value2);	\
		__asm__ volatile("movel %0, 0xFFFF8604" : : "d"(dummy));	\
		DELAY;						\
	})

/* Definitions for DMODE */

#define READ        0x000
#define WRITE       0x100

#define DMA_FDC     0x080
#define DMA_ACSI    0x000

#define DMA_DISABLE 0x040

#define SEC_COUNT   0x010
#define DMA_WINDOW  0x000

#define REG_ACSI    0x008
#define REG_FDC     0x000

#define A1          0x002

/* Timeout constants */

#define TIMEOUTCMD 2 /* ca. 5-10ms */
#define TIMEOUTDMA 2000 /* ca. 10s */
#define TIMEOUTCOUNTER 64 /* Counter for timeout function (in ms) */
