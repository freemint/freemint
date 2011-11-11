/*
 * File:    fec.h
 * Purpose: Driver for the Fast Ethernet Controller (FEC)
 *
 * Notes:
 */

#ifndef _FEC_H_
#define _FEC_H_


#include "platform/types.h"
#include "platform/board.h"

/* Driver meta defines: */

#define FEC_DRIVER_MAJOR_VERSION	0
#define FEC_DRIVER_MINOR_VERSION	1

#define FEC_DRIVER_DESC		"FEC Ethernet driver"
#ifndef FEC_DRIVER
 #define FEC_DRIVER		    "fec.xif"
#endif

#define FEC_DEVICE_NAME	"eth"


#ifndef FEC_CHANNEL
 #define FEC_CHANNEL	0
#endif


#define FEC_DEBUG 1
#define FEC_VERBOSE 1
#ifdef FEC_VERBOSE
#define FEC_DEBUG 1
#endif


/*
#ifdef FEC_DEBUG
#define FEC_PROMISCUOUS
#endif
*/

#ifdef FEC_DEBUG
#define    KDEBUG(x) KERNEL_DEBUG x
#else
    #define KDEBUG(x)
#endif


/* MII Speed Settings */
#define FEC_MII_10BASE_T        0
#define FEC_MII_100BASE_TX      1

/* MII Duplex Settings */
#define FEC_MII_HALF_DUPLEX     0
#define FEC_MII_FULL_DUPLEX     1

/* Timeout for MII communications */
#define FEC_MII_TIMEOUT         0x10000

/* External Interface Modes */
#define FEC_MODE_7WIRE          0
#define FEC_MODE_MII            1
#define FEC_MODE_LOOPBACK       2   /* Internal Loopback */
#define FEC_MODE_RMII           3
#define FEC_MODE_RMII_LOOPBACK  4   /* Internal Loopback */

/* Intern driver modes: */
#define FEC_IMODE_PROMISCUOUS	1
#define FEC_IMODE_10MBIT		2

/* ------------------------ Prototypes ------------------------------------ */
long fec_mii_write(uint8 a, uint8 b, uint8 c, uint16 d);
long fec_mii_read(uint8, uint8, uint8, uint16 *);
void fec_mii_init(uint8, uint32);
void fec_duplex (uint8, uint8);


/*
 * Ethernet Info
 */
#define FEC_PHY0            (0x00)
#define FEC_PHY1            (0x01)
#define FEC_PHY(x)          ((x == 0) ? FEC_PHY0 : FEC_PHY1)

#define phy_init            am79c874_init

/* Interrupt Priorities */
#define DMA_INTC_LVL 5
#define DMA_INTC_PRI 3
#define FEC_INTC_LVL 5
#define FEC0_INTC_PRI 1
#define FEC1_INTC_PRI 0

/* DMA Task Priorities */
#define FEC0TX_DMA_PRI 6
#define FEC0RX_DMA_PRI 5
#define FEC1TX_DMA_PRI 4
#define FEC1RX_DMA_PRI 3

/* Number of DMA FEC Buffers to allocate */
#define NUM_RXBDS               (20)
#define NUM_TXBDS               (20)

/* size of data bound to each desriptor: */
#define RX_BUFFER_SIZE          (2048)
#define TX_BUFFER_SIZE          (1520)

/* Error flags within RX Buffer descriptors statCtrl: */
#define RX_BD_NO		0x0010
#define RX_BD_CR		0x0004
#define RX_BD_OV		0x0002
#define RX_BD_TR		0x0001
#define RX_BD_ERROR     (RX_BD_NO | RX_BD_CR | RX_BD_OV | RX_BD_TR)

#endif /* _FEC_H_ */
