/* defines and type declarations to access the */
/* DP8390 of the PAMs DMA adaptor */

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

typedef struct
{
	u_char reserved1[0x38];
	HADDR  hwaddr;
	u_char reserved2[0x1c2];
} DMAHWADDR;

#define MIN_LEN 60

/*
 * Definitions of commands understood by the PAMs DMA adaptor.
 *
 * In general the DMA adaptor uses LUN 0, 5, 6 and 7 on one ID changeable
 * by the PAM's Net software.
 *
 * LUN 0 works as a harddisk. You can boot the PAM's Net driver there.
 * LUN 5 works as a harddisk and lets you access the RAM and some I/O HW
 *       area. In sector 0, bytes 0x38-0x3d you find the ethernet HW address
 *       of the adaptor.
 * LUN 6 works as a harddisk and lets you access the firmware ROM.
 * LUN 7 lets you send and receive packets.
 *
 * Some commands like the INQUIRY command work identical on all used LUNs.
 * 
 * UNKNOWN1 seems to read some data.
 *          Command length is 6 bytes.
 * UNKNOWN2 seems to read some data (command byte 1 must be !=0). The
 *          following bytes seem to be something like an allocation length.
 *          Command length is 6 bytes.
 * READPKT  reads a packet received by the DMA adaptor.
 *          Command length is 6 bytes.
 * WRITEPKT sends a packet transferred by the following DMA phase. The length
 *          of the packet is transferred in command bytes 3 and 4.
 *          The adaptor automatically replaces the src hw address in an ethernet
 *          packet by its own hw address.
 *          Command length is 6 bytes.
 * INQUIRY  has the same function as the INQUIRY command supported by harddisks
 *          and other SCSI devices. I lets you detect which device you found
 *          at a given address.
 *          Command length is 6 bytes.
 * START    initializes the DMA adaptor. After this command it is able to send
 *          and receive packets. There is no status byte returned!
 *          Command length is 1 byte.
 * NUMPKTS  gives back the number of received packets waiting in the queue in
 *          the status byte.
 *          Command length is 1 byte.
 * UNKNOWN3
 * UNKNOWN4 Function of these three commands is unknown.
 * UNKNOWN5 The Command length of these three commands is 1 byte.
 * DESELECT immediately deselects the DMA adaptor. May important with interrupt
 *          driven operation.
 *          Command length is 1 byte.
 * STOP     resets the DMA adaptor. After this command packets can no longer
 *          be received or transferred.
 *          Command length is 6 byte.
 */

enum {UNKNOWN1=3, READPKT=8, UNKNOWN2, WRITEPKT=10, INQUIRY=18, START, 
      NUMPKTS=22, UNKNOWN3, UNKNOWN4, UNKNOWN5, DESELECT, STOP};

#define READSECTOR  READPKT
#define WRITESECTOR WRITEPKT

u_char *inquire8="MV      PAM's NET/GK";

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

#define TIMEOUTCMD 80 /* ca. 400ms */
#define TIMEOUTDMA 2000 /* ca. 10s */
#define TIMEOUTCOUNTER 128 /* Counter for timeout function (in ms) */
