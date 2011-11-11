

#ifndef _MCD_API_H
#define _MCD_API_H

/*
 * Portability typedefs
 */
typedef signed long s32;
typedef unsigned long u32;
typedef signed short s16;
typedef unsigned short u16;
typedef signed char s8;
typedef unsigned char u8;



/* Chained buffer descriptor */
typedef volatile struct MCD_bufDesc_struct MCD_bufDesc;
struct MCD_bufDesc_struct {
   u32 flags;         /* flags describing the DMA */
   u32 csumResult;    /* checksum from checksumming performed since last checksum reset */
   s8  *srcAddr;      /* the address to move data from */
   s8  *destAddr;     /* the address to move data to */
   s8  *lastDestAddr; /* the last address written to */
   u32 dmaSize;       /* the number of bytes to transfer independent of the transfer size */
   MCD_bufDesc *next; /* next buffer descriptor in chain */
   u32 info;          /* private information about this descriptor;  DMA does not affect it */
};

/* Progress Query struct */
typedef volatile struct MCD_XferProg_struct {
   s8 *lastSrcAddr;         /* the most-recent or last, post-increment source address */
   s8 *lastDestAddr;        /* the most-recent or last, post-increment destination address */
   u32  dmaSize;            /* the amount of data transferred for the current buffer */
   MCD_bufDesc *currBufDesc;/* pointer to the current buffer descriptor being DMAed */
} MCD_XferProg;


/* FEC buffer descriptor */
typedef volatile struct MCD_bufDescFec_struct {
    u16 statCtrl;
    u16 length;
    u32 dataPointer;
} MCD_bufDescFec;


typedef struct dma_cookie
{
    s32 version; /* 0x0101 for example */
    s32 magic; /* 'DMAC' */
    s32 (*dma_set_initiator)(s32 initiator);
    u32 (*dma_get_initiator)(s32 requestor);
    void (*dma_free_initiator)(s32 requestor);
    s32 (*dma_set_channel)(s32 requestor, void (*handler)(void));
    s32 (*dma_get_channel)(s32 requestor);
    void (*dma_free_channel)(s32 requestor);
    void (*dma_clear_channel)(s32 channel);
    s32 (*MCD_startDma)(long channel, s8 *srcAddr, u32 srcIncr, s8 *destAddr, u32 destIncr, u32 dmaSize, u32 xferSize, u32 initiator, long priority, u32 flags, u32 funcDesc);
    s32 (*MCD_dmaStatus)(s32 channel);
    s32 (*MCD_XferProgrQuery)(s32 channel, MCD_XferProg *progRep);
    s32 (*MCD_killDma)(s32 channel);
    s32 (*MCD_continDma)(s32 channel);
    s32 (*MCD_pauseDma)(s32 channel);
    s32 (*MCD_resumeDma)(s32 channel);
    s32 (*MCD_csumQuery)(s32 channel, u32 *csum);
	void *(*dma_alloc)(ulong size);
    long (*dma_free)(void * mem);
} DMACF_COOKIE;

/********************************************************************/


/********************************************************************/

/*
 * Create identifiers for each initiator/requestor
 */
#define DMA_ALWAYS      (0)
#define DMA_DSPI_RX     (1)
#define DMA_DSPI_TX     (2)
#define DMA_DREQ0       (3)
#define DMA_PSC0_RX     (4)
#define DMA_PSC0_TX     (5)
#define DMA_USBEP0      (6)
#define DMA_USBEP1      (7)
#define DMA_USBEP2      (8)
#define DMA_USBEP3      (9)
#define DMA_PCI_TX      (10)
#define DMA_PCI_RX      (11)
#define DMA_PSC1_RX     (12)
#define DMA_PSC1_TX     (13)
#define DMA_I2C_RX      (14)
#define DMA_I2C_TX      (15)
#define DMA_FEC0_RX     (16)
#define DMA_FEC0_TX     (17)
#define DMA_FEC1_RX     (18)
#define DMA_FEC1_TX     (19)
#define DMA_DREQ1       (20)
#define DMA_CTM0        (21)
#define DMA_CTM1        (22)
#define DMA_CTM2        (23)
#define DMA_CTM3        (24)
#define DMA_CTM4        (25)
#define DMA_CTM5        (26)
#define DMA_CTM6        (27)
#define DMA_CTM7        (28)
#define DMA_USBEP4      (29)
#define DMA_USBEP5      (30)
#define DMA_USBEP6      (31)
#define DMA_PSC2_RX     (32)
#define DMA_PSC2_TX     (33)
#define DMA_PSC3_RX     (34)
#define DMA_PSC3_TX     (35)
#define DMA_FEC_RX(x)   ((x == 0) ? DMA_FEC0_RX : DMA_FEC1_RX)
#define DMA_FEC_TX(x)   ((x == 0) ? DMA_FEC0_TX : DMA_FEC1_TX)

/********************************************************************/



/*
 * Number of DMA channels
 */
#define NCHANNELS 16

/*
 * PTD contrl reg bits
 */
#define PTD_CTL_TSK_PRI         0x8000
#define PTD_CTL_COMM_PREFETCH   0x0001

/*
 * Task Control reg bits and field masks
 */
#define TASK_CTL_EN             0x8000
#define TASK_CTL_VALID          0x4000
#define TASK_CTL_ALWAYS         0x2000
#define TASK_CTL_INIT_MASK      0x1f00
#define TASK_CTL_ASTRT          0x0080
#define TASK_CTL_HIPRITSKEN     0x0040
#define TASK_CTL_HLDINITNUM     0x0020
#define TASK_CTL_ASTSKNUM_MASK  0x000f

/*
 * Priority reg bits and field masks
 */
#define PRIORITY_HLD            0x80
#define PRIORITY_PRI_MASK       0x07

/*
 * Debug Control reg bits and field masks
 */
#define DBG_CTL_BLOCK_TASKS_MASK    0xffff0000
#define DBG_CTL_AUTO_ARM            0x00008000
#define DBG_CTL_BREAK               0x00004000
#define DBG_CTL_COMP1_TYP_MASK      0x00003800
#define DBG_CTL_COMP2_TYP_MASK      0x00000070
#define DBG_CTL_EXT_BREAK           0x00000004
#define DBG_CTL_INT_BREAK           0x00000002

/*
 * PTD Debug reg selector addresses
 * This reg must be written with a value to show the contents of
 * one of the desired internal register.
 */
#define PTD_DBG_REQ             0x00 /* shows the state of 31 initiators */
#define PTD_DBG_TSK_VLD_INIT    0x01 /* shows which 16 tasks are valid and
                                        have initiators asserted */


/*
 * General return values
 */
#define MCD_OK                   0
#define MCD_ERROR               -1
#define MCD_TABLE_UNALIGNED     -2
#define MCD_CHANNEL_INVALID     -3

/*
 * MCD_initDma input flags
 */
#define MCD_RELOC_TASKS         0x00000001
#define MCD_NO_RELOC_TASKS      0x00000000
#define MCD_COMM_PREFETCH_EN    0x00000002  /* Commbus Prefetching - MCF547x/548x ONLY */

/*
 * MCD_dmaStatus Status Values for each channel
 */
#define MCD_NO_DMA  1 /* No DMA has been requested since reset */
#define MCD_IDLE    2 /* DMA active, but the initiator is currently inactive */
#define MCD_RUNNING 3 /* DMA active, and the initiator is currently active */
#define MCD_PAUSED  4 /* DMA active but it is currently paused */
#define MCD_HALTED  5 /* the most recent DMA has been killed with MCD_killTask() */
#define MCD_DONE    6 /* the most recent DMA has completed. */


/*
 * Constants for the funcDesc parameter
 */
/* Byte swapping: */
#define MCD_NO_BYTE_SWAP    0x00045670  /* to disable byte swapping. */
#define MCD_BYTE_REVERSE    0x00076540  /* to reverse the bytes of each u32 of the DMAed data. */
#define MCD_U16_REVERSE     0x00067450  /* to reverse the 16-bit halves of
                                           each 32-bit data value being DMAed.*/
#define MCD_U16_BYTE_REVERSE    0x00054760 /* to reverse the byte halves of each
                                            16-bit half of each 32-bit data value DMAed */
#define MCD_NO_BIT_REV  0x00000000  /* do not reverse the bits of each byte DMAed. */
#define MCD_BIT_REV     0x00088880  /* reverse the bits of each byte DMAed */

/* CRCing: */
#define MCD_CRC16       0xc0100000  /* to perform CRC-16 on DMAed data. */
#define MCD_CRCCCITT    0xc0200000  /* to perform CRC-CCITT on DMAed data. */
#define MCD_CRC32       0xc0300000  /* to perform CRC-32 on DMAed data. */
#define MCD_CSUMINET    0xc0400000  /* to perform internet checksums on DMAed data.*/
#define MCD_NO_CSUM     0xa0000000  /* to perform no checksumming. */

#define MCD_FUNC_NOEU1 (MCD_NO_BYTE_SWAP | MCD_NO_BIT_REV | MCD_NO_CSUM)
#define MCD_FUNC_NOEU2 (MCD_NO_BYTE_SWAP | MCD_NO_CSUM)

/*
 * Constants for the flags parameter
 */
#define MCD_TT_FLAGS_RL   0x00000001 /* Read line */
#define MCD_TT_FLAGS_CW   0x00000002 /* Combine Writes */
#define MCD_TT_FLAGS_SP   0x00000004 /* Speculative prefetch(XLB) MCF547x/548x ONLY  */
#define MCD_TT_FLAGS_MASK 0x000000ff
#define MCD_TT_FLAGS_DEF  (MCD_TT_FLAGS_RL | MCD_TT_FLAGS_CW)

#define MCD_SINGLE_DMA  0x00000100 /* Unchained DMA */
#define MCD_CHAIN_DMA              /* TBD */
#define MCD_EU_DMA                 /* TBD */
#define MCD_FECTX_DMA   0x00001000 /* FEC TX ring DMA */
#define MCD_FECRX_DMA   0x00002000 /* FEC RX ring DMA */


/* these flags are valid for MCD_startDma and the chained buffer descriptors */
#define MCD_BUF_READY   0x80000000 /* indicates that this buffer is now under the DMA's control */
#define MCD_WRAP        0x20000000 /* to tell the FEC Dmas to wrap to the first BD */
#define MCD_INTERRUPT   0x10000000 /* to generate an interrupt after completion of the DMA. */
#define MCD_END_FRAME   0x08000000 /* tell the DMA to end the frame when transferring
                                      last byte of data in buffer */
#define MCD_CRC_RESTART 0x40000000 /* to empty out the accumulated checksum
                                      prior to performing the DMA. */

/* Defines for the FEC buffer descriptor control/status word*/
#define MCD_FEC_BUF_READY   0x8000
#define MCD_FEC_WRAP        0x2000
#define MCD_FEC_INTERRUPT   0x1000
#define MCD_FEC_END_FRAME   0x0800


/*
 * Defines for general intuitiveness
 */

#define MCD_TRUE  1
#define MCD_FALSE 0

/*
 * Three different cases for destination and source.
 */
#define MINUS1          -1
#define ZERO            0
#define PLUS1           1


/*************************************************************************/
/*
 * API function Prototypes  - see MCD_dmaApi.c for further notes
 */

/*
 * MCD_startDma starts a particular kind of DMA .
 */

#define MCD_startDma(\
    channel, srcAddr, srcIncr,\
    destAddr, destIncr, dmaSize,\
    xferSize, initiator, priority,\
    flags, funcDesc ) \
((dmac->MCD_startDma) ( \
    channel, srcAddr, srcIncr, \
    destAddr, destIncr, dmaSize, \
    xferSize, initiator, priority, \
    flags, funcDesc))


/*
 * MCD_initDma() initializes the DMA API by setting up a pointer to the DMA
 * registers, relocating and creating the appropriate task structures, and
 * setting up some global settings
 */
#define MCD_initDma(sDmaBarAddr, taskTableDest, flags) \
    (dmac->MCD_initDma)(sDmaBarAddr, taskTableDest, flags))

/*
 * MCD_dmaStatus() returns the status of the DMA on the requested channel.
 */
#define MCD_dmaStatus(ch) ((dmac->MCD_dmaStatus)(ch))

/*
 * MCD_XferProgrQuery() returns progress of DMA on requested channel
 */
#define MCD_XferProgrQuery (channel, progRep) \
    ((dmac->MCD_XferProgrQuery(channel, progRep))

/*
 * MCD_killDma() halts the DMA on the requested channel, without any
 * intention of resuming the DMA.
 */
#define MCD_killDma(channel) ((dmac->MCD_killDma)(channel))

/*
 * MCD_continDma() continues a DMA which as stopped due to encountering an
 * unready buffer descriptor.
 */
#define MCD_continDma(channel) ((dmac->MCD_continDma)(channel))

/*
 * MCD_pauseDma() pauses the DMA on the given channel ( if any DMA is
 * running on that channel).
 */
#define MCD_pauseDma (channel) ((dmac->MCD_pauseDma)( channel ))

/*
 * MCD_resumeDma() resumes the DMA on a given channel (if any DMA is
 * running on that channel).
 */
#define MCD_resumeDma(channel) ((dmac->MCD_resumeDma)(channel))


/*
 * MCD_csumQuery provides the checksum/CRC after performing a non-chained DMA
 */
#define MCD_csumQuery(channel, csum) ((dmac->MCD_csumQuery)( channel, csum))

/*
 * MCD_getCodeSize provides the packed size required by the microcoded task
 * and structures.
 */
#define MCD_getCodeSize(void) ((dmac->MCD_getCodeSize)())


/* DMA Utils: */
#define dma_set_initiator(ini) ((dmac->dma_set_initiator)(ini))
#define dma_get_initiator(ini) ((dmac->dma_get_initiator)(ini))
#define dma_free_initiator(ini) ((dmac->dma_free_initiator)(ini))
#define dma_set_channel( requestor, handler ) ((dmac->dma_set_channel)(requestor, handler))
#define dma_get_channel(requestor) ((dmac->dma_get_channel)(requestor))
#define dma_free_channel(requestor) ((dmac->dma_free_channel)(requestor))
#define dma_clear_channel(channel) ((dmac->dma_clear_channel)(channel))

#define dma_alloc(size) ((dmac->dma_alloc)(size))
#define dma_free(mem) ((dmac->dma_free)(mem))

DMACF_COOKIE * mc_dma_init( void );

extern DMACF_COOKIE * dmac;

#endif /* _MCD_API_H */

