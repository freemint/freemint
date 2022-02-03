/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
        A RealTek RTL-8139 Fast Ethernet driver for Linux.

        Maintained by Jeff Garzik <jgarzik@mandrakesoft.com>
        Copyright 2000,2001 Jeff Garzik

        Much code comes from Donald Becker's rtl8139.c driver,
        versions 1.13 and older.  This driver was originally based
        on rtl8139.c version 1.07.  Header of rtl8139.c version 1.13:

        -----<snip>-----

                Written 1997-2001 by Donald Becker.
                This software may be used and distributed according to the
                terms of the GNU General Public License (GPL), incorporated
                herein by reference.  Drivers based on or derived from this
                code fall under the GPL and must retain the authorship,
                copyright and license notice.  This file is not a complete
                program and may only be used when the entire operating
                system is licensed under the GPL.

                This driver is for boards based on the RTL8129 and RTL8139
                PCI ethernet chips.

                The author may be reached as becker@scyld.com, or C/O Scyld
                Computing Corporation 410 Severn Ave., Suite 210 Annapolis
                MD 21403

                Support and updates available at
                http://www.scyld.com/network/rtl8139.html

                Twister-tuning table provided by Kinston
                <shangh@realtek.com.tw>.

        -----<snip>-----
        This software may be used and distributed according to the terms
        of the GNU General Public License, incorporated herein by reference.

        Contributors:

                Donald Becker - he wrote the original driver, kudos to him!
                (but please don't e-mail him for support, this isn't his driver)

                Tigran Aivazian - bug fixes, skbuff free cleanup

                Martin Mares - suggestions for PCI cleanup

                David S. Miller - PCI DMA and softnet updates

                Ernst Gill - fixes ported from BSD driver

                Daniel Kobras - identified specific locations of
                        posted MMIO write bugginess

                Gerard Sharp - bug fix, testing and feedback

                David Ford - Rx ring wrap fix

                Dan DeMaggio - swapped RTL8139 cards with me, and allowed me
                to find and fix a crucial bug on older chipsets.

                Donald Becker/Chris Butterworth/Marcus Westergren -
                Noticed various Rx packet size-related buglets.

                Santiago Garcia Mantinan - testing and feedback

                Jens David - 2.2.x kernel backports

                Martin Dennett - incredibly helpful insight on undocumented
                features of the 8139 chips

                Jean-Jacques Michel - bug fix

                Tobias Ringström - Rx interrupt status checking suggestion

                Andrew Morton - Clear blocked signals, avoid
                buffer overrun setting current->comm.

*/

static inline unsigned short swap_short(unsigned short val)
{
  return((val >> 8) | (val << 8));
}

static inline unsigned long swap_long(unsigned long val)
{
	return( (unsigned long)swap_short((unsigned short)(val >> 16)) | ((unsigned long)swap_short((unsigned short)val) << 16));
}

static inline unsigned short cpu_to_le16(unsigned short val)
{
  return(swap_short(val));
}
static inline unsigned long cpu_to_le32(unsigned long val)
{
  return(swap_long(val));
}

#define le16_to_cpu cpu_to_le16
#define le32_to_cpu cpu_to_le32

#define	readb(x)		  (*((volatile unsigned char *)(x)))
#define writeb(a, b)	(*((volatile unsigned char *)(b)) = ((volatile unsigned char)a))
#if defined CONFIG_RTL_BIG_ENDIAN
#define	readw(x)		  (*((volatile unsigned short *)(x)))
#define writew(a, b)	(*((volatile unsigned short *)(b)) = ((volatile unsigned short)a))
#define	readl(x)		  (*((volatile unsigned long *)(x)))
#define writel(a, b)	(*((volatile unsigned long *)(b)) = ((volatile unsigned long)a))
#else
#define readw(x)		  swap_short((*((volatile unsigned short *)(x))))
#define writew(a, b)	(*((volatile unsigned short *)(b)) = swap_short(((volatile unsigned short)a)))
#define readl(x)		  swap_long((*((volatile unsigned long *)(x))))
#define writel(a, b)	(*((volatile unsigned long *)(b)) = swap_long(((volatile unsigned long)a)))
#endif

/* write MMIO register, with flush */
/* Flush avoids rtl8139 bug w/ posted MMIO writes */
#define RTL_W8_F(reg, val8)     do { writeb((val8), (unsigned long)ioaddr + (reg)); (void)readb((unsigned long)ioaddr + (reg)); } while(0)
#define RTL_W16_F(reg, val16)   do { writew((val16), (unsigned long)ioaddr + (reg)); (void)readw((unsigned long)ioaddr + (reg)); } while(0)
#define RTL_W32_F(reg, val32)   do { writel((val32), (unsigned long)ioaddr + (reg)); (void)readl((unsigned long)ioaddr + (reg)); } while(0)

#if MMIO_FLUSH_AUDIT_COMPLETE

/* write MMIO register */
#define RTL_W8(reg, val8)       writeb((val8), (unsigned long)ioaddr + (reg))
#define RTL_W16(reg, val16)     writew((val16), (unsigned long)ioaddr + (reg))
#define RTL_W32(reg, val32)     writel((val32), (unsigned long)ioaddr + (reg))

#else

/* write MMIO register, then flush */
#define RTL_W8    RTL_W8_F
#define RTL_W16   RTL_W16_F
#define RTL_W32   RTL_W32_F

#endif /* MMIO_FLUSH_AUDIT_COMPLETE */


/* read MMIO register */
#define RTL_R8(reg)       readb((unsigned long)ioaddr + (reg))
#define RTL_R16(reg)      readw((unsigned long)ioaddr + (reg))
#define RTL_R32(reg)      readl((unsigned long)ioaddr + (reg))

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0X8139

#define PKT_BUF_SZ 1536	/* Size of each temporary Rx buffer.*/
/* Size of the in-memory receive ring. */
#define RX_BUF_LEN_IDX  2       /* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN (8192UL << RX_BUF_LEN_IDX)
#define RX_BUF_PAD 16UL
#define RX_BUF_WRAP_PAD 2048UL /* spare padding to handle lack of packet wrap */
#define RX_BUF_TOT_LEN (RX_BUF_LEN + RX_BUF_PAD + RX_BUF_WRAP_PAD)

#define MAX_ADDR_LEN    8   /* Largest hardware address length */

/* Number of Tx descriptor registers. */
#define NUM_TX_DESC     4

/* max supported ethernet frame size -- must be at least (mtu+14+4).*/
#define MAX_ETH_FRAME_SIZE      1536UL

#define RTL_MTU         (MAX_ETH_FRAME_SIZE-14-4)

/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE     MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_LEN  (TX_BUF_SIZE * NUM_TX_DESC)

/* Ethernet standard lengths in bytes */
#define ETH_ADDR_LEN    6
#define ETH_TYPE_LEN    2
#define ETH_CRC_LEN     4
#define ETH_MAX_DATA    1500
#define ETH_MIN_DATA    46
#define ETH_HDR_LEN     (ETH_ADDR_LEN * 2 + ETH_TYPE_LEN)
#define ETH_ZLEN        60 /* Min bytes in frames */

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256UL      /* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
#define RX_FIFO_THRESH  6UL       /* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST    6UL       /* Maximum PCI burst, '6' is 1024 */
#define TX_DMA_BURST    6UL       /* Maximum PCI burst, '6' is 1024 */

enum chip
{
  CH_8139 = 0,
  CH_8139_K,
  CH_8139A,
  CH_8139B,
  CH_8130,
  CH_8139C,
  CH_8139CP,
};

#define HAS_MII_XCVR  0x010000UL
#define HAS_CHIP_XCVR 0x020000UL
#define HAS_LNK_CHNG  0x040000UL

#define RTL_MIN_IO_SIZE 0x80
#define RTL8139B_IO_SIZE 256

#define RTL8129_CAPS    HAS_MII_XCVR
#define RTL8139_CAPS    HAS_CHIP_XCVR|HAS_LNK_CHNG

typedef enum
{
  RTL8139 = 0,
  RTL8139_CB,
  SMC1211TX,
  /*MPX5030,*/
  DELTA8139,
  ADDTRON8139,
  DFE538TX,
  RTL8129,
} board_t;

/* indexed by board_t, above */
static struct
{
  const char *name;
  unsigned long hw_flags;
} board_info[] = {
  { "RealTek RTL8139CP Fast Ethernet", RTL8139_CAPS },
  { "RealTek RTL8139B PCI/CardBus", RTL8139_CAPS },
  { "SMC1211TX EZCard 10/100 (RealTek RTL8139)", RTL8139_CAPS },
/*      { MPX5030, "Accton MPX5030 (RealTek RTL8139)", RTL8139_CAPS },*/
  { "Delta Electronics 8139 10/100BaseTX", RTL8139_CAPS },
  { "Addtron Technolgy 8139 10/100BaseTX", RTL8139_CAPS },
  { "D-Link DFE-538TX (RealTek RTL8139)", RTL8139_CAPS },
  { "RealTek RTL8129", RTL8129_CAPS },
};

/* directly indexed by chip_t, above */
const static struct
{
  const char *name;
  unsigned char version; /* from RTL8139C docs */
  unsigned long RxConfigMask; /* should clear the bits supported by this chip */
} rtl_chip_info[] = {
  { "RTL-8139",
    0x40,
    0xf0fe0040UL, /* XXX copied from RTL8139A, verify */
  },
  { "RTL-8139 rev K",
    0x60,
    0xf0fe0040UL,
  },
  { "RTL-8139A",
    0x70,
    0xf0fe0040UL,
  },
  { "RTL-8139B",
    0x78,
    0xf0fc0040UL
  },
  { "RTL-8130",
    0x7C,
    0xf0fe0040UL, /* XXX copied from RTL8139A, verify */
  },
  { "RTL-8139C",
    0x74,
    0xf0fc0040UL, /* XXX copied from RTL8139B, verify */
  },
  { "RTL-8139CP",
    0x76,
    0xf0fc0040UL, /* XXX copied from RTL8139B, verify */
  },
};

/* The rest of these values should never change. */

/* Symbolic offsets to registers. */
#define MAC0            0x00   /* Ethernet hardware address. */
#define MAR0            0x08   /* Multicast filter. */
#define TxStatus0       0x10   /* Transmit status (Four 32bit registers). */
#define TxAddr0         0x20   /* Tx descriptors (also four 32bit). */
#define RxBuf           0x30
#define RxEarlyCnt      0x34
#define RxEarlyStatus   0x36
#define ChipCmd         0x37
#define RxBufPtr        0x38
#define RxBufAddr       0x3A
#define IntrMask        0x3C
#define IntrStatus      0x3E
#define TxConfig        0x40
#define ChipVersion     0x43
#define RxConfig        0x44
#define Timer           0x48   /* A general-purpose counter. */
#define RxMissed        0x4C   /* 24 bits valid, write clears. */
#define Cfg9346         0x50
#define Config0         0x51
#define Config1         0x52
#define FlashReg        0x54
#define MediaStatus     0x58
#define Config3         0x59
#define Config4         0x5A   /* absent on RTL-8139A */
#define HltClk          0x5B
#define MultiIntr       0x5C
#define TxSummary       0x60
#define BasicModeCtrl   0x62
#define BasicModeStatus 0x64
#define NWayAdvert      0x66
#define NWayLPAR        0x68
#define NWayExpansion   0x6A
  /* Undocumented registers, but required for proper operation. */
#define FIFOTMS         0x70   /* FIFO Control and test. */
#define CSCR            0x74   /* Chip Status and Configuration Register. */
#define PARA78          0x78
#define PARA7c          0x7C   /* Magic transceiver parameter register. */
#define Config5         0xD8   /* absent on RTL-8139A */

/* ClearBitMasks */
#define MultiIntrClear 0xF000U
#define ChipCmdClear   0x00E2U
#define Config1Clear   ((1<<7)|(1<<6)|(1<<3)|(1<<2)|(1<<1))

/* ChipCmdBits */
#define CmdReset   0x10
#define CmdRxEnb   0x08
#define CmdTxEnb   0x04
#define RxBufEmpty 0x01

/* IntrStatus, Interrupt register bits, using my own meaningful names. */
#define PCIErr     0x8000U
#define PCSTimeout 0x4000U
#define RxFIFOOver   0x40U
#define RxUnderrun   0x20U
#define RxOverflow   0x10U
#define TxErr        0x08U
#define TxOK         0x04U
#define RxErr        0x02U
#define RxOK         0x01U

/* TxStatusBits */
#define TxHostOwns        0x2000UL
#define TxUnderrun        0x4000UL
#define TxStatOK          0x8000UL
#define TxOutOfWindow 0x20000000UL
#define TxAborted     0x40000000UL
#define TxCarrierLost 0x80000000UL

/* RxStatusBits */
#define RxMulticast 0x8000UL
#define RxPhysical  0x4000UL
#define RxBroadcast 0x2000UL
#define RxBadSymbol 0x0020UL
#define RxRunt      0x0010UL
#define RxTooLong   0x0008UL
#define RxCRCErr    0x0004UL
#define RxBadAlign  0x0002UL
#define RxStatusOK  0x0001UL

/* Bits in RxConfig. */
#define AcceptErr       0x20UL
#define AcceptRunt      0x10UL
#define AcceptBroadcast 0x08UL
#define AcceptMulticast 0x04UL
#define AcceptMyPhys    0x02UL
#define AcceptAllPhys   0x01UL

/* Bits in TxConfig. */
#define TxIFG1 (1UL << 25)     /* Interframe Gap Time */
#define TxIFG0 (1UL << 24)     /* Enabling these bits violates IEEE 802.3 */
#define TxLoopBack ((1UL << 18) | (1 << 17)) /* enable loopback test mode */
#define TxCRC (1UL << 16)      /* DISABLE appending CRC to end of Tx packets */
#define TxClearAbt (1UL << 0)  /* Clear abort (WO) */
#define TxDMAShift 8UL   /* DMA burst value (0-7) is shift this many bits */
#define TxVersionMask 0x7C800000UL /* mask out version bits 30-26, 23 */

/* Bits in Config1 */
#define Cfg1_PM_Enable   0x01
#define Cfg1_VPD_Enable  0x02
#define Cfg1_PIO         0x04
#define Cfg1_MMIO        0x08
#define Cfg1_LWAKE       0x10
#define Cfg1_Driver_Load 0x20
#define Cfg1_LED0        0x40
#define Cfg1_LED1        0x80

/* RxConfigBits */
  /* Early Rx threshold, none or X/16 */
#define RxCfgEarlyRxNone 0
#define RxCfgEarlyRxShift 24
  /* rx fifo threshold */
#define RxCfgFIFOShift 13
#define RxCfgFIFONone (7UL << RxCfgFIFOShift)
  /* Max DMA burst */
#define RxCfgDMAShift 8
#define RxCfgDMAUnlimited (7UL << RxCfgDMAShift)
  /* rx ring buffer length */
#define RxCfgRcv8K 0
#define RxCfgRcv16K (1UL << 11)
#define RxCfgRcv32K (1UL << 12)
#define RxCfgRcv64K ((1UL << 11) | (1UL << 12))
  /* Disable packet wrap at end of Rx buffer */
#define RxNoWrap (1UL << 7)

/* Twister tuning parameters from RealTek.
   Completely undocumented, but required to tune bad links. */
#define CSCR_LinkOKBit       0x0400U
#define CSCR_LinkChangeBit   0x0800U
#define CSCR_LinkStatusBits  0xf000U
#define CSCR_LinkDownOffCmd  0x03c0U
#define CSCR_LinkDownCmd     0xf3c0U

/* Cfg9346Bits */
#define Cfg9346_Lock   0x00
#define Cfg9346_Unlock 0xC0

/* NegotiationBits */
#define AutoNegotiationEnable  0x1000
#define AutoNegotiationRestart 0x0200
#define AutoNegoAbility10half    0x21
#define AutoNegoAbility10full    0x41
#define AutoNegoAbility100half   0x81
#define AutoNegoAbility100full  0x101

/* MediaStatusBits */
#define DuplexMode 0x0100    /* in BasicModeControlRegister */
#define Speed_10     0x08    /* in Media Status Register */

/* A few user-configurable values. */
/* media options */
#define MAX_UNITS 8
static int media[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int full_duplex[MAX_UNITS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/* MII serial management: mostly bogus for now. */
/* Read and write the MII management registers using software-generated
   serial MDIO protocol.
   The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
   "overclocking" issues. */
#define MDIO_DIR        0x80
#define MDIO_DATA_OUT   0x04
#define MDIO_DATA_IN    0x02
#define MDIO_CLK        0x01
#define MDIO_WRITE0 (MDIO_DIR)
#define MDIO_WRITE1 (MDIO_DIR | MDIO_DATA_OUT)

#define rtl8139_mdio_delay(mdio_addr)   readb(mdio_addr)
#define mdio_delay(mdio_addr)   readb(mdio_addr)

static char mii_2_8139_map[8] =
{
  BasicModeCtrl,
  BasicModeStatus,
  0,
  0,
  NWayAdvert,
  NWayLPAR,
  NWayExpansion,
  0
};

static const unsigned short rtl8139_intr_mask =
  /* PCIErr | */ PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver |  TxErr | TxOK | RxErr | RxOK;

static const unsigned long rtl8139_rx_config =
    RxCfgEarlyRxNone | RxCfgRcv32K | RxNoWrap | (RX_FIFO_THRESH << RxCfgFIFOShift) | (RX_DMA_BURST << RxCfgDMAShift);

#define TX_TIMEOUT ((1000UL*200UL)/1000UL)
#undef jiffies
#define jiffies (*_hz_200)

struct rtl8139_private
{
  long handle;
  void *mmio_addr;
  unsigned long mmio_len;
  int drv_flags;
  unsigned char *rx_ring;
  unsigned long cur_rx;    /* Index into the Rx buffer of next Rx pkt. */
  unsigned long tx_flag;
  unsigned long cur_tx;
  unsigned long dirty_tx;
  unsigned long trans_start;
  /* The saved address of a sent-in-place packet/buffer, for skfree(). */
  unsigned char *tx_buf[NUM_TX_DESC];     /* Tx bounce buffers */
  unsigned char *tx_bufs; /* Tx bounce buffer region. */
  unsigned long tx_bufs_dma;
  unsigned long rx_ring_dma;
  signed char phys[4];      /* MII device addresses. */
  unsigned short advertising;    /* NWay media advertisement */
  char twistie, twist_row, twist_col;     /* Twister tune state. */
  unsigned int full_duplex:1;     /* Full-duplex operation requested. */
  unsigned int duplex_lock:1;
  unsigned int default_port:4;    /* Last dev->if_port value. */
  unsigned int media2:4;  /* Secondary monitored media port. */
  unsigned int medialock:1;       /* Don't sense media type. */
  unsigned int mediasense:1;      /* Media sensing in progress. */
  int chipset;
/* For 8139C+ */
  int AutoNegoAbility;
/* For mintnet */
  unsigned char dev_addr[MAX_ADDR_LEN]; /* hw address   */
  long ioaddr;
  int must_free_irq;
  struct netif *nif;
/* For CTPCI */
  long (*ctpci_dma_lock)(long mode);
};

