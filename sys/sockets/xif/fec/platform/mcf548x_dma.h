/*
 * File:	mcf548x_dma.h
 * Purpose:	Register and bit definitions for the MCF548X
 *
 * Notes:	
 *	
 */

#ifndef __MCF548X_DMA_H__
#define __MCF548X_DMA_H__

/*********************************************************************
*
* Multi-Channel DMA (DMA)
*
*********************************************************************/

/* Register read/write macros */
#define MCF_DMA_TASKBAR    (*(vuint32*)(void*)(&__MBAR[0x008000]))
#define MCF_DMA_CP         (*(vuint32*)(void*)(&__MBAR[0x008004]))
#define MCF_DMA_EP         (*(vuint32*)(void*)(&__MBAR[0x008008]))
#define MCF_DMA_VP         (*(vuint32*)(void*)(&__MBAR[0x00800C]))
#define MCF_DMA_DIPR       (*(vuint32*)(void*)(&__MBAR[0x008014]))
#define MCF_DMA_DIMR       (*(vuint32*)(void*)(&__MBAR[0x008018]))
#define MCF_DMA_TCR0       (*(vuint16*)(void*)(&__MBAR[0x00801C]))
#define MCF_DMA_TCR1       (*(vuint16*)(void*)(&__MBAR[0x00801E]))
#define MCF_DMA_TCR2       (*(vuint16*)(void*)(&__MBAR[0x008020]))
#define MCF_DMA_TCR3       (*(vuint16*)(void*)(&__MBAR[0x008022]))
#define MCF_DMA_TCR4       (*(vuint16*)(void*)(&__MBAR[0x008024]))
#define MCF_DMA_TCR5       (*(vuint16*)(void*)(&__MBAR[0x008026]))
#define MCF_DMA_TCR6       (*(vuint16*)(void*)(&__MBAR[0x008028]))
#define MCF_DMA_TCR7       (*(vuint16*)(void*)(&__MBAR[0x00802A]))
#define MCF_DMA_TCR8       (*(vuint16*)(void*)(&__MBAR[0x00802C]))
#define MCF_DMA_TCR9       (*(vuint16*)(void*)(&__MBAR[0x00802E]))
#define MCF_DMA_TCR10      (*(vuint16*)(void*)(&__MBAR[0x008030]))
#define MCF_DMA_TCR11      (*(vuint16*)(void*)(&__MBAR[0x008032]))
#define MCF_DMA_TCR12      (*(vuint16*)(void*)(&__MBAR[0x008034]))
#define MCF_DMA_TCR13      (*(vuint16*)(void*)(&__MBAR[0x008036]))
#define MCF_DMA_TCR14      (*(vuint16*)(void*)(&__MBAR[0x008038]))
#define MCF_DMA_TCR15      (*(vuint16*)(void*)(&__MBAR[0x00803A]))
#define MCF_DMA_TCRn(x)    (*(vuint16*)(void*)(&__MBAR[0x00801C+((x)*0x002)]))
#define MCF_DMA_IMCR       (*(vuint32*)(void*)(&__MBAR[0x00805C]))
#define MCF_DMA_PTDDBG     (*(vuint32*)(void*)(&__MBAR[0x008080]))

/* Bit definitions and macros for MCF_DMA_DIPR */
#define MCF_DMA_DIPR_TASK0           (0x00000001)
#define MCF_DMA_DIPR_TASK1           (0x00000002)
#define MCF_DMA_DIPR_TASK2           (0x00000004)
#define MCF_DMA_DIPR_TASK3           (0x00000008)
#define MCF_DMA_DIPR_TASK4           (0x00000010)
#define MCF_DMA_DIPR_TASK5           (0x00000020)
#define MCF_DMA_DIPR_TASK6           (0x00000040)
#define MCF_DMA_DIPR_TASK7           (0x00000080)
#define MCF_DMA_DIPR_TASK8           (0x00000100)
#define MCF_DMA_DIPR_TASK9           (0x00000200)
#define MCF_DMA_DIPR_TASK10          (0x00000400)
#define MCF_DMA_DIPR_TASK11          (0x00000800)
#define MCF_DMA_DIPR_TASK12          (0x00001000)
#define MCF_DMA_DIPR_TASK13          (0x00002000)
#define MCF_DMA_DIPR_TASK14          (0x00004000)
#define MCF_DMA_DIPR_TASK15          (0x00008000)

/* Bit definitions and macros for MCF_DMA_DIMR */
#define MCF_DMA_DIMR_TASK0           (0x00000001)
#define MCF_DMA_DIMR_TASK1           (0x00000002)
#define MCF_DMA_DIMR_TASK2           (0x00000004)
#define MCF_DMA_DIMR_TASK3           (0x00000008)
#define MCF_DMA_DIMR_TASK4           (0x00000010)
#define MCF_DMA_DIMR_TASK5           (0x00000020)
#define MCF_DMA_DIMR_TASK6           (0x00000040)
#define MCF_DMA_DIMR_TASK7           (0x00000080)
#define MCF_DMA_DIMR_TASK8           (0x00000100)
#define MCF_DMA_DIMR_TASK9           (0x00000200)
#define MCF_DMA_DIMR_TASK10          (0x00000400)
#define MCF_DMA_DIMR_TASK11          (0x00000800)
#define MCF_DMA_DIMR_TASK12          (0x00001000)
#define MCF_DMA_DIMR_TASK13          (0x00002000)
#define MCF_DMA_DIMR_TASK14          (0x00004000)
#define MCF_DMA_DIMR_TASK15          (0x00008000)

/* Bit definitions and macros for MCF_DMA_IMCR */
#define MCF_DMA_IMCR_SRC16(x)        (((x)&0x00000003)<<0)
#define MCF_DMA_IMCR_SRC17(x)        (((x)&0x00000003)<<2)
#define MCF_DMA_IMCR_SRC18(x)        (((x)&0x00000003)<<4)
#define MCF_DMA_IMCR_SRC19(x)        (((x)&0x00000003)<<6)
#define MCF_DMA_IMCR_SRC20(x)        (((x)&0x00000003)<<8)
#define MCF_DMA_IMCR_SRC21(x)        (((x)&0x00000003)<<10)
#define MCF_DMA_IMCR_SRC22(x)        (((x)&0x00000003)<<12)
#define MCF_DMA_IMCR_SRC23(x)        (((x)&0x00000003)<<14)
#define MCF_DMA_IMCR_SRC24(x)        (((x)&0x00000003)<<16)
#define MCF_DMA_IMCR_SRC25(x)        (((x)&0x00000003)<<18)
#define MCF_DMA_IMCR_SRC26(x)        (((x)&0x00000003)<<20)
#define MCF_DMA_IMCR_SRC27(x)        (((x)&0x00000003)<<22)
#define MCF_DMA_IMCR_SRC28(x)        (((x)&0x00000003)<<24)
#define MCF_DMA_IMCR_SRC29(x)        (((x)&0x00000003)<<26)
#define MCF_DMA_IMCR_SRC30(x)        (((x)&0x00000003)<<28)
#define MCF_DMA_IMCR_SRC31(x)        (((x)&0x00000003)<<30)
#define MCF_DMA_IMCR_SRC16_FEC0RX    (0x00000000)
#define MCF_DMA_IMCR_SRC17_FEC0TX    (0x00000000)
#define MCF_DMA_IMCR_SRC18_FEC0RX    (0x00000020)
#define MCF_DMA_IMCR_SRC19_FEC0TX    (0x00000080)
#define MCF_DMA_IMCR_SRC20_FEC1RX    (0x00000100)
#define MCF_DMA_IMCR_SRC21_DREQ1     (0x00000000)
#define MCF_DMA_IMCR_SRC21_FEC1TX    (0x00000400)
#define MCF_DMA_IMCR_SRC22_FEC0RX    (0x00001000)
#define MCF_DMA_IMCR_SRC23_FEC0TX    (0x00004000)
#define MCF_DMA_IMCR_SRC24_CTM0      (0x00010000)
#define MCF_DMA_IMCR_SRC24_FEC1RX    (0x00020000)
#define MCF_DMA_IMCR_SRC25_CTM1      (0x00040000)
#define MCF_DMA_IMCR_SRC25_FEC1TX    (0x00080000)
#define MCF_DMA_IMCR_SRC26_USBEP4    (0x00000000)
#define MCF_DMA_IMCR_SRC26_CTM2      (0x00200000)
#define MCF_DMA_IMCR_SRC27_USBEP5    (0x00000000)
#define MCF_DMA_IMCR_SRC27_CTM3      (0x00800000)
#define MCF_DMA_IMCR_SRC28_USBEP6    (0x00000000)
#define MCF_DMA_IMCR_SRC28_CTM4      (0x01000000)
#define MCF_DMA_IMCR_SRC28_DREQ1     (0x02000000)
#define MCF_DMA_IMCR_SRC28_PSC2RX    (0x03000000)
#define MCF_DMA_IMCR_SRC29_DREQ1     (0x04000000)
#define MCF_DMA_IMCR_SRC29_CTM5      (0x08000000)
#define MCF_DMA_IMCR_SRC29_PSC2TX    (0x0C000000)
#define MCF_DMA_IMCR_SRC30_FEC1RX    (0x00000000)
#define MCF_DMA_IMCR_SRC30_CTM6      (0x10000000)
#define MCF_DMA_IMCR_SRC30_PSC3RX    (0x30000000)
#define MCF_DMA_IMCR_SRC31_FEC1TX    (0x00000000)
#define MCF_DMA_IMCR_SRC31_CTM7      (0x80000000)
#define MCF_DMA_IMCR_SRC31_PSC3TX    (0xC0000000)

/********************************************************************/

#endif /* __MCF548X_DMA_H__ */
