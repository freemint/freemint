/*
 * Ethernet driver for Coldfire based Firebee board.
 * This driver uses the DMA API provided with FireTOS.
 */


#include <osbind.h>

#include "global.h"
#include "buf.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"
#include "netinfo.h"
#include "mint/proc.h"
#include "mint/sockio.h"
#include "mint/arch/asm_spl.h"			// for spl7() locking
#include "mint/ssystem.h"
#include "mint/ktypes.h"
#include "ssystem.h"
#include "arch/cpu.h"					// for cpushi (cache control)
#include "util.h"
#include "cookie.h"

#include "platform/board.h"
#include "platform/dma.h"

#include "fec.h"
#include "am79c874.h"
#include "bcm5222.h"

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(a):(b))
#endif

#define ETH_CRC_LEN     (4)
#define ETH_MAX_FRM     (ETH_HLEN + ETH_MAX_DLEN + ETH_CRC_LEN)
#define ETH_MIN_FRM     (ETH_HLEN + ETH_MIN_DLEN + ETH_CRC_LEN)
#define ETH_MTU         (ETH_HLEN + ETH_MAX_DLEN)

/* mode parameters for firetos_read_parameter: */
#define CT60_MODE_READ 0

/* type parameters for firetos_read_parameter: */
#define CT60_MAC_ADDRESS 10L

/* macro to read FireTOS parameters (named ct60_rw_parameter within FireTOS): */
#define firetos_read_parameter(type_param, value )\
	(long)trap_14_wwll( 0xc60b, CT60_MODE_READ, type_param, 0 )

/* ------------------------ Type definitions ------------------------------ */
typedef struct s_fec_if_priv
{
    struct netif *nif;        		/* network interface */
    uint8 ch;                   	/* FEC channel (device) */
    int8 rx_dma_ch;					/* DMA channel */
    int8 tx_dma_ch;					/* DMA channel */
    TIMEOUT * rx_tout_magic;		/* root timeoute id */
    uint8 mode;						/* for promisc mode */
    uint8 rx_bd_idx;				/* circular index into next next buffer desc. to recv. */
    uint8 tx_bd_idx_next;			/* circular index into next buffer desc. to transmit   */
    uint8 tx_bd_idx_cur;			/* circular index into next tx buffer desc. to commit  */
    MCD_bufDescFec *rx_bd;			/* Receive Buffer Descriptors */
    MCD_bufDescFec *tx_bd;			/* Transmit Buffer Descriptors */
    uint8 *tx_buf_unaligned[NUM_TXBDS]; /* unaligned buffers to free on close */
    uint8 *rx_buf_unaligned[NUM_RXBDS];
} fec_if_t;


/* ------------------------ Static variables ------------------------------ */
/*
 * interface structures
 */
static struct netif if_fec[1];
static fec_if_t fecif_g[1];
MCD_bufDescFec *sram_unaligned_bds = (MCD_bufDescFec*)(__MBAR+0x13000);

/* ------------------------ Function declarations ------------------------- */
long driver_init (void);
void dbghex( char *, unsigned long val );
static uint8 fec_hash_address(const uint8 *addr);
static void fec_set_address(uint8 ch, const uint8 *pa);
static void fec_reg_dump( uint8 ch );
static void fec0_rx_timeout(struct proc *, long int arg);
static void fec0_rx_frame(void);
static void fec0_tx_frame(void);
static void fec_tx_frame(struct netif * nif);
static void fec_rx_timeout(struct netif * nif);
static void fec_rx_frame(struct netif * nif);
static void fec_rx_stop(fec_if_t *fec);
static void fec_tx_stop(fec_if_t *fec);
static inline int fec_buf_init(fec_if_t *fec);

static void inline fec_buf_flush(fec_if_t *fec);
static inline MCD_bufDescFec * fec_rx_alloc(fec_if_t *fec);
static inline MCD_bufDescFec * fec_tx_alloc(fec_if_t *fec);
static inline MCD_bufDescFec * fec_tx_free(fec_if_t *fec);


/*
 * Prototypes for our service functions
 */
static long	fec_open	(struct netif *);
static long	fec_close	(struct netif *);
static long	fec_output	(struct netif *, BUF *, const char *, short, short);
static long	fec_ioctl	(struct netif *, short, long);
static long	fec_config	(struct netif *, struct ifopt *);

extern uint32 mch_cookie( void );

/*
 * machine type detection
 */
uint32 mch_cookie( void )
{
	uint32 mch = 0;

	if (s_system (S_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0)
		mch = 0;

	return mch;
}

static unsigned char *board_get_ethaddr(unsigned char *ethaddr);

unsigned char *board_get_ethaddr(unsigned char * ethaddr)
{
    unsigned long ethaddr_part;
    ethaddr_part=(unsigned long)firetos_read_parameter(CT60_MAC_ADDRESS,0L);
    ethaddr[0] = 0;
    ethaddr[1] = 0xCF;
    ethaddr[2] = 0x54;
    ethaddr[3] = ( (ethaddr_part>>16) & 255 );
    ethaddr[4] = ( (ethaddr_part>>8) & 255 );
    ethaddr[5] = ( ethaddr_part &255 );
    return ethaddr;
}

static char * dbgbuf[128];
void dbghex( char * str, unsigned long value )
{
    ksprintf(
        (char*)&dbgbuf, "%s:\t0x%04x%04x",
        str,
        (short)((value&0xFFFF0000)>>16),
        (short)(value&0xFFFF)
    );
    KDEBUG(( (char*)&dbgbuf ));
}



/********************************************************************/
/*
 * Write a value to a PHY's MII register.
 *
 * Parameters:
 *  ch          FEC channel
 *  phy_addr    Address of the PHY.
 *  reg_addr    Address of the register in the PHY.
 *  data        Data to be written to the PHY register.
 *
 * Return Values:
 *  0 on failure
 *  1 on success.
 *
 * Please refer to your PHY manual for registers and their meanings.
 * mii_write() polls for the FEC's MII interrupt event (which should
 * be masked from the interrupt handler) and clears it. If after a
 * suitable amount of time the event isn't triggered, a value of 0
 * is returned.
 */
long
fec_mii_write(uint8 ch, uint8 phy_addr, uint8 reg_addr, uint16 data)
{
    long timeout;
    uint32 eimr;

    /*
     * Clear the MII interrupt bit
     */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_MII;

    /*
     * Write to the MII Management Frame Register to kick-off
     * the MII write
     */
    MCF_FEC_MMFR(ch) = 0
                       | MCF_FEC_MMFR_ST_01
                       | MCF_FEC_MMFR_OP_WRITE
                       | MCF_FEC_MMFR_PA((uint32)phy_addr)
                       | MCF_FEC_MMFR_RA((uint32)reg_addr)
                       | MCF_FEC_MMFR_TA_10
                       | MCF_FEC_MMFR_DATA(data);

    /*
     * Mask the MII interrupt
     */
    eimr = MCF_FEC_EIMR(ch);
    MCF_FEC_EIMR(ch) &= ~MCF_FEC_EIMR_MII;

    /*
     * Poll for the MII interrupt (interrupt should be masked)
     */
    for (timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
    {
        if (MCF_FEC_EIR(ch) & MCF_FEC_EIR_MII)
            break;
    }
    if(timeout == FEC_MII_TIMEOUT)
    {
        KDEBUG(("fec_mii_write failed!"));
        return 0;
    }

    /*
     * Clear the MII interrupt bit
     */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_MII;

    /*
     * Restore the EIMR
     */
    MCF_FEC_EIMR(ch) = eimr;

    return 1;
}
/********************************************************************/
/*
 * Read a value from a PHY's MII register.
 *
 * Parameters:
 *  ch          FEC channel
 *  phy_addr    Address of the PHY.
 *  reg_addr    Address of the register in the PHY.
 *  data        Pointer to storage for the Data to be read
 *              from the PHY register (passed by reference)
 *
 * Return Values:
 *  0 on failure
 *  1 on success.
 *
 * Please refer to your PHY manual for registers and their meanings.
 * mii_read() polls for the FEC's MII interrupt event (which should
 * be masked from the interrupt handler) and clears it. If after a
 * suitable amount of time the event isn't triggered, a value of 0
 * is returned.
 */
long
fec_mii_read(uint8 ch, uint8 phy_addr, uint8 reg_addr, uint16 *data)
{
    long timeout;

    /*
     * Clear the MII interrupt bit
     */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_MII;

    /*
     * Write to the MII Management Frame Register to kick-off
     * the MII read
     */
    MCF_FEC_MMFR(ch) = 0
                       | MCF_FEC_MMFR_ST_01
                       | MCF_FEC_MMFR_OP_READ
                       | MCF_FEC_MMFR_PA((uint32)phy_addr)
                       | MCF_FEC_MMFR_RA((uint32)reg_addr)
                       | MCF_FEC_MMFR_TA_10;

    /*
     * Poll for the MII interrupt (interrupt should be masked)
     */
    for (timeout = 0; timeout < FEC_MII_TIMEOUT; timeout++)
    {
        if (MCF_FEC_EIR(ch) & MCF_FEC_EIR_MII)
            break;
    }

    if(timeout == FEC_MII_TIMEOUT)
    {
        KDEBUG(("fec_mii_read failed!"));
        return 0;
    }

    /*
     * Clear the MII interrupt bit
     */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_MII;

    *data = (uint16)(MCF_FEC_MMFR(ch) & 0x0000FFFF);

    return 1;
}
/********************************************************************/
/*
 * Initialize the MII interface controller
 *
 * Parameters:
 *  ch      FEC channel
 *  sys_clk System Clock Frequency (in MHz)
 */
void
fec_mii_init(uint8 ch, uint32 sys_clk)
{

    /*
     * Initialize the MII clock (EMDC) frequency
     *
     * Desired MII clock is 2.5MHz
     * MII Speed Setting = System_Clock / (2.5MHz * 2)

     * (plus 1 to make sure we round up)
     */
    MCF_FEC_MSCR(ch) = MCF_FEC_MSCR_MII_SPEED((sys_clk/5)+1);

    /*
     * Make sure the external interface signals are enabled
     */
    if (ch == 1)
        MCF_GPIO_PAR_FECI2CIRQ |= 0
                                  | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDC_EMDC
                                  | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDIO_EMDIO;
    else
        MCF_GPIO_PAR_FECI2CIRQ |= 0
                                  | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDC
                                  | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDIO;
}

/********************************************************************/
/*
 * Generate the hash table settings for the given address
 *
 * Parameters:
 *  addr    48-bit (6 byte) Address to generate the hash for
 *
 * Return Value:
 *  The 6 most significant bits of the 32-bit CRC result
 */
static uint8 fec_hash_address(const uint8 *addr)
{
    uint32 crc;
    uint8 byte;
    int i, j;
    crc = 0xFFFFFFFF;
    for(i=0; i<6; ++i)
    {
        byte = addr[i];
        for(j=0; j<8; j++)
        {
            if((byte & 0x01)^(crc & 0x01))
            {
                crc >>= 1;
                crc = crc ^ 0xEDB88320;
            }
            else
                crc >>= 1;
            byte >>= 1;
        }
    }
    return(uint8)(crc >> 26);
}

/********************************************************************/
/*
 * Set the Physical (Hardware) Address and the Individual Address
 * Hash in the selected FEC
 *
 * Parameters:
 *  ch  FEC channel
 *  pa  Physical (Hardware) Address for the selected FEC
 */
static void fec_set_address(uint8 ch, const uint8 *pa)
{
    uint8 crc;
    /* Set the Physical Address */
    MCF_FEC_PALR(ch) = (uint32)(((uint32)pa[0]<<24) | ((uint32)pa[1]<<16) | (pa[2]<<8) | pa[3]);
    MCF_FEC_PAUR(ch) = (uint32)(((uint32)pa[4]<<24) | ((uint32)pa[5]<<16));
    /* Calculate and set the hash for given Physical Address
       in the  Individual Address Hash registers */
    crc = fec_hash_address(pa);
    if(crc >= 32)
        MCF_FEC_IAUR(ch) |= (uint32)(1 << (crc - 32));
    else
        MCF_FEC_IALR(ch) |= (uint32)(1 << crc);
}


/********************************************************************/
/*
 * Display some of the registers for debugging
 *
 * Parameters:
 *  ch      FEC channel
 */
void
fec_reg_dump(uint8 ch)
{
    if(ch == 0)
        KDEBUG(("------------- FEC0"));
    else
        KDEBUG(("------------- FEC1"));

    dbghex("EIR (Interrupt Event)", MCF_FEC_EIR(ch) );
    dbghex("EIMR (Interrupt Mask)", MCF_FEC_EIMR(ch) );
    dbghex("ECR (Ethernet Control)", MCF_FEC_ECR(ch) );
    dbghex("MMFR (MMFR)", MCF_FEC_MMFR(ch) );
    dbghex("MSCR (MII Speed Control)", MCF_FEC_MSCR(ch) );
    dbghex("MIBC (MIB Control/Status)", MCF_FEC_MIBC(ch) );
    dbghex("RCR (Receive Control)", MCF_FEC_RCR(ch) );
    dbghex("RHASH (Receive Hash)", MCF_FEC_R_HASH(ch) );
    dbghex("TCR (Transmit Control)", MCF_FEC_TCR(ch) );
    dbghex("PALR (Physical Address Low)", MCF_FEC_PALR(ch) );
    dbghex("PAHR (Physical Address High)", MCF_FEC_PAUR(ch) );
    dbghex("OPD (Opcode / Pause Duration)", MCF_FEC_OPD(ch) );
    dbghex("IAUR (Individual Address Upper )", MCF_FEC_IAUR(ch) );
    dbghex("IALR (Individual Address Lower )", MCF_FEC_IALR(ch) );
    dbghex("GAUR (Group Address Upper )", MCF_FEC_GAUR(ch) );
    dbghex("GALR (Group Address Lower)", MCF_FEC_GALR(ch) );
    dbghex("FECTFWR (Transmit FIFO Watermark)", MCF_FEC_FECTFWR(ch) );
    dbghex("FECRFSR (Receive FIFO Status)", MCF_FEC_FECRFSR(ch) );
    dbghex("FECRFCR (Receive FIFO Control)", MCF_FEC_FECRFCR(ch) );
    dbghex("FECRLRFP (Receive FIFO Last Read Frame Pointer)", MCF_FEC_FECRLRFP(ch) );
    dbghex("FECRLWFP (Receive FIFO Last Write Frame Pointer)", MCF_FEC_FECRLWFP(ch) );
    dbghex("FECRFAR (Receive FIFO Alarm)", MCF_FEC_FECRFAR(ch) );
    dbghex("FECRFRP (Receive FIFO Read Pointer)", MCF_FEC_FECRFRP(ch) );
    dbghex("FECRFWP (Receive FIFO Write Pointer)", MCF_FEC_FECRFWP(ch));
    dbghex("FECTFSR (Transmit FIFO Status)", MCF_FEC_FECTFSR(ch));
    dbghex("FECTFCR (Transmit FIFO Control)", MCF_FEC_FECTFCR(ch));
    dbghex("FECTLRFP (Transmit FIFO Last Read Frame Pointer)", MCF_FEC_FECTLRFP(ch));
    dbghex("FECTLWFP (Transmit FIFO Last Write Frame Pointer)", MCF_FEC_FECTLWFP(ch));
    dbghex("FECTFAR (Transmit FIFO Alarm)", MCF_FEC_FECTFAR(ch));
    dbghex("FECTFRP (Transmit FIFO Read Pointer)", MCF_FEC_FECTFRP(ch) );
    dbghex("FECTFWP (Transmit FIFO Write Pointer )", MCF_FEC_FECTFWP(ch) );
    dbghex("FRST (FIFO Reset Register)", MCF_FEC_FRST(ch) );
    dbghex("FECCTCWR (CRC and Transmit Frame Control Word )", MCF_FEC_CTCWR(ch) );
    KDEBUG(("-------------------------------"));
}


/********************************************************************/
/*
 * Set the duplex on the selected FEC controller
 *
 * Parameters:
 *  ch      FEC channel
 *  duplex  FEC_MII_FULL_DUPLEX or FEC_MII_HALF_DUPLEX
 */
void fec_duplex(uint8 ch, uint8 duplex)
{
    switch (duplex)
    {
    case FEC_MII_HALF_DUPLEX:
        MCF_FEC_RCR(ch) |= MCF_FEC_RCR_DRT;
        MCF_FEC_TCR(ch) &= (uint32)~MCF_FEC_TCR_FDEN;
        break;
    case FEC_MII_FULL_DUPLEX:
    default:
        MCF_FEC_RCR(ch) &= (uint32)~MCF_FEC_RCR_DRT;
        MCF_FEC_TCR(ch) |= MCF_FEC_TCR_FDEN;
        break;
    }
}


/********************************************************************/
/*
 * Reset the selected FEC controller
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_reset(uint8 ch)
{
    int i;

    /* Clear any events in the FIFO status registers */
    MCF_FEC_FECRFSR(ch) = (MCF_FEC_FECRFSR_OF | MCF_FEC_FECRFSR_UF | MCF_FEC_FECRFSR_RXW | MCF_FEC_FECRFSR_FAE | MCF_FEC_FECRFSR_IP);
    MCF_FEC_FECTFSR(ch) = (MCF_FEC_FECRFSR_OF | MCF_FEC_FECRFSR_UF | MCF_FEC_FECRFSR_RXW | MCF_FEC_FECRFSR_FAE | MCF_FEC_FECRFSR_IP);
    /* Reset the FIFOs */
    MCF_FEC_FRST(ch) |= MCF_FEC_FRST_SW_RST | MCF_FEC_FRST_RST_CTL;
    MCF_FEC_FRST(ch) = 0;

    /* Set the Reset bit and clear the Enable bit */
    MCF_FEC_ECR(ch) = MCF_FEC_ECR_RESET;
    /* Wait at least 8 clock cycles */
    for(i=0; i<10; i++)
        asm(" nop");

    MCF_FEC_FECTFSR(ch) = (MCF_FEC_FECTFSR(ch) | MCF_FEC_FECTFSR_TXW );
}


/********************************************************************/
/*
 * Enable interrupts on the selected FEC
 *
 * Parameters:
 *  ch      FEC channel
 *  pri     Interrupt Priority
 *  lvl     Interrupt Level
 */
static void fec_irq_enable(uint8 ch, uint8 lvl, uint8 pri)
{
    /* Setup the appropriate ICR */
    if(ch)
        MCF_INTC_ICR38 = (uint8)(MCF_INTC_ICRn_IP(pri) | MCF_INTC_ICRn_IL(lvl));
    else
        MCF_INTC_ICR39 = (uint8)(MCF_INTC_ICRn_IP(pri) | MCF_INTC_ICRn_IL(lvl));
    /* Clear any pending FEC interrupt events */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_CLEAR_ALL;
    /* Unmask all FEC interrupts */
    MCF_FEC_EIMR(ch) = MCF_FEC_EIMR_UNMASK_ALL;
    /* Unmask the FEC interrupt in the interrupt controller */
    if(ch)
        MCF_INTC_IMRH &= ~MCF_INTC_IMRH_INT_MASK38;
    else
        MCF_INTC_IMRH &= ~MCF_INTC_IMRH_INT_MASK39;
}

/********************************************************************/
/*
 * Disable interrupts on the selected FEC
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_irq_disable(uint8 ch)
{
    /* Mask all FEC interrupts */
    MCF_FEC_EIMR(ch) = MCF_FEC_EIMR_MASK_ALL;
    /* Mask the FEC interrupt in the interrupt controller */
    if(ch)
        MCF_INTC_IMRH |= MCF_INTC_IMRH_INT_MASK38;
    else
        MCF_INTC_IMRH |= MCF_INTC_IMRH_INT_MASK39;
}

/********************************************************************/
/*
 * Enable interrupts on the selected FEC
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_enable(uint8 ch)
{
    fec_irq_enable(ch, FEC_INTC_LVL, ch ? FEC1_INTC_PRI : FEC0_INTC_PRI);
    /* Enable the transmit and receive processing */
    MCF_FEC_ECR(ch) |= MCF_FEC_ECR_ETHER_EN;
}


/********************************************************************/
/*
 * Disable interrupts on the selected FEC
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_disable(uint8 ch)
{
    /* Mask the FEC interrupt in the interrupt controller */
    fec_irq_disable(ch);
    /* Disable the FEC channel */
    MCF_FEC_ECR(ch) &= ~MCF_FEC_ECR_ETHER_EN;
}

/********************************************************************/
/*
 * Initialize the selected FEC
 *
 * Parameters:
 *  ch      FEC channel
 *  mode    External interface mode (RMII, MII, 7-wire, or internal loopback)
 *  pa      Physical (Hardware) Address for the selected FEC
 */
static void fec_init( fec_if_t *fec, uint8 mode, uint8 duplex, const uint8 *pa)
{
	uint8 ch = fec->ch;
    /* Enable all the external interface signals */
    if(mode == FEC_MODE_7WIRE)
    {
        if(ch)
            MCF_GPIO_PAR_FECI2CIRQ |= MCF_GPIO_PAR_FECI2CIRQ_PAR_E17;
        else
            MCF_GPIO_PAR_FECI2CIRQ |= MCF_GPIO_PAR_FECI2CIRQ_PAR_E07;
    }
    else if (mode == FEC_MODE_MII)
    {
        if(ch)
            MCF_GPIO_PAR_FECI2CIRQ |= (MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDC_EMDC | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MDIO_EMDIO
                                       | MCF_GPIO_PAR_FECI2CIRQ_PAR_E1MII | MCF_GPIO_PAR_FECI2CIRQ_PAR_E17);
        else
            MCF_GPIO_PAR_FECI2CIRQ |= (MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDC | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MDIO
                                       | MCF_GPIO_PAR_FECI2CIRQ_PAR_E0MII | MCF_GPIO_PAR_FECI2CIRQ_PAR_E07);
    }
    /* Clear the Individual and Group Address Hash registers */
    MCF_FEC_IALR(ch) = 0;
    MCF_FEC_IAUR(ch) = 0;
    MCF_FEC_GALR(ch) = 0;
    MCF_FEC_GAUR(ch) = 0;
    /* Set the Physical Address for the selected FEC */
    fec_set_address(ch, pa);
    /* Mask all FEC interrupts */
    MCF_FEC_EIMR(ch) = MCF_FEC_EIMR_MASK_ALL;
    /* Clear all FEC interrupt events */
    MCF_FEC_EIR(ch) = MCF_FEC_EIR_CLEAR_ALL;
    /* Initialize the Receive Control Register */
    MCF_FEC_RCR(ch) = MCF_FEC_RCR_MAX_FL(ETH_MAX_FRM)
#ifdef FEC_PROMISCUOUS
                      | MCF_FEC_RCR_PROM
#endif
                      | MCF_FEC_RCR_FCE;
    if(mode == FEC_MODE_MII)
        MCF_FEC_RCR(ch) |= MCF_FEC_RCR_MII_MODE;
    else if(mode == FEC_MODE_LOOPBACK)
        MCF_FEC_RCR(ch) |= (MCF_FEC_RCR_LOOP | MCF_FEC_RCR_PROM);
    /* Set the duplex */
    if(mode == FEC_MODE_LOOPBACK)
        /* Loopback mode must operate in full-duplex */
        fec_duplex(ch, FEC_MII_FULL_DUPLEX);
    else
        fec_duplex(ch, duplex);

	if( fec->mode & FEC_IMODE_PROMISCUOUS ){
		MCF_FEC_RCR(ch) |= MCF_FEC_RCR_PROM;
	}

    /* Set Rx FIFO alarm and granularity */
    MCF_FEC_FECRFCR(ch) = MCF_FEC_FECRFCR_FRM | MCF_FEC_FECRFCR_RXW_MSK | MCF_FEC_FECRFCR_GR(7);
    MCF_FEC_FECRFAR(ch) = MCF_FEC_FECRFAR_ALARM(768);
    /* Set Tx FIFO watermark, alarm and granularity */
    MCF_FEC_FECTFCR(ch) = MCF_FEC_FECTFCR_FRM | MCF_FEC_FECTFCR_TXW_MSK | MCF_FEC_FECTFCR_GR(7);

    MCF_FEC_FECTFAR(ch) = MCF_FEC_FECTFAR_ALARM(256);
    MCF_FEC_FECTFWR(ch) = MCF_FEC_FECTFWR_X_WMRK_256;

    /* Enable the transmitter to append the CRC */
    MCF_FEC_CTCWR(ch) = (uint32)(MCF_FEC_CTCWR_TFCW | MCF_FEC_CTCWR_CRC);
}

/********************************************************************/
/*
 * Start the FEC Rx DMA task
 *
 * Parameters:
 *  ch      FEC channel
 *  rxbd    First Rx buffer descriptor in the chain
 */
static void fec_rx_start(struct netif * nif, s8 *rxbd, uint8 pri)
{
    fec_if_t *fecif = (fec_if_t *)nif->data;
    uint8 ch = fecif->ch;
    uint32 initiator;

    if( dma_get_channel( DMA_FEC_RX(ch) ) != -1 )
    {
        fec_rx_stop(fecif);
    }
    /* Make the initiator assignment */
    dma_set_initiator(DMA_FEC_RX(ch));
    /* Grab the initiator number */
    initiator = dma_get_initiator(DMA_FEC_RX(ch));
    /* Determine the DMA channel running the task for the selected FEC */
    fecif->rx_dma_ch = dma_set_channel(DMA_FEC_RX(ch), ch ? NULL : fec0_rx_frame);
#if RX_BUFFER_SIZE != 2048
#error "RX_BUFFER_SIZE must be set to 2048 for safe FEC operation"
#endif
    MCD_startDma(fecif->rx_dma_ch, (s8*)rxbd, 0, MCF_FEC_FECRFDR_ADDR(ch), 0, RX_BUFFER_SIZE,
                          0, initiator, (long)pri,
                          (uint32)(MCD_FECRX_DMA | MCD_INTERRUPT | MCD_TT_FLAGS_CW | MCD_TT_FLAGS_RL | MCD_TT_FLAGS_SP),
                          (uint32)(MCD_NO_CSUM | MCD_NO_BYTE_SWAP)
                         );
}

/********************************************************************/
/*
 * Continue the Rx DMA task
 *
 * This routine is called after the DMA task has halted after
 * encountering an Rx buffer descriptor that wasn't marked as
 * ready. There is no harm in calling the DMA continue routine
 * if the DMA is not halted.
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_rx_continue(uint8 ch)
{
    int channel;
    /* Determine the DMA channel running the task for the selected FEC */
    channel = dma_get_channel(DMA_FEC_RX(ch));
    /* Continue/restart the DMA task */
    if(channel != -1)
        MCD_continDma(channel);
}

/********************************************************************/
/*
 * Stop all frame receptions on the selected FEC
 *
 * Parameters:
 *  ch  FEC channel
 */
static void fec_rx_stop(fec_if_t *fec)
{

    uint32 mask;
    long channel;

    /* Save off the EIMR value */
    KDEBUG(("fec_rx_stop"));

    mask = MCF_FEC_EIMR(fec->ch);
    /* Mask all interrupts */
    MCF_FEC_EIMR(fec->ch) = 0;
    /* Determine the DMA channel running the task for the selected FEC */
    channel = dma_get_channel( DMA_FEC_RX(fec->ch) );

    /* Kill the FEC Tx DMA task */
    if(channel != -1)
        MCD_killDma(channel);

    /* Free up the FEC requestor from the software maintained initiator list */
    dma_free_initiator(DMA_FEC_RX(fec->ch));

    /* Free up the DMA channel */
    dma_free_channel(DMA_FEC_RX(fec->ch));

    /* Restore the interrupt mask register value */
    MCF_FEC_EIMR(fec->ch) = mask;

    fec->rx_dma_ch = -1;
    KDEBUG(("fec_rx_stop done"));
}



/********************************************************************/
static void fec_rx_timeout(struct netif * nif)
{
    fec_if_t *fecif = (fec_if_t *)nif->data;
    uint8 ch = fecif->ch;
    MCD_bufDescFec *dmaBuf;
    BUF		*b;
    int keep;
    int rlen;

    while((dmaBuf = fec_rx_alloc(fecif)) != NULL)
    {
        /* Check the Receive Frame Status Word for errors
         - The L bit should always be set
         - No undefined bits should be set
         - The upper 5 bits of the length should be cleared */
        if(!(dmaBuf->statCtrl & MCD_FEC_END_FRAME) || (dmaBuf->statCtrl & 0x0608) || (dmaBuf->length & 0xF800))
            keep = FALSE;
        else if(dmaBuf->statCtrl & RX_BD_ERROR)
            keep = FALSE;
        else
            keep = TRUE;
        if(keep)
        {
            rlen = MIN( ETH_MAX_FRM, dmaBuf->length );
            b = buf_alloc ( rlen +100, 50, BUF_NORMAL );
            if(!b)
            {
                KDEBUG(("buf_alloc failed!"));
                nif->in_errors++;
            }
            else
            {
                b->dend = b->dstart + rlen;
                memcpy( b->dstart, (void*)dmaBuf->dataPointer, b->dend - b->dstart );

                // Pass packet to upper layers
                if (nif->bpf)
                    bpf_input (nif, b);

                // enqueue packet
                if(!if_input(nif, b, 0, eth_remove_hdr(b) ))
                    nif->in_packets++;
                else
                    nif->in_errors++;
            }
        }
        else
        {
            KDEBUG(("drop frame\r\n"));
            nif->in_errors++;
        }
        /* Re-initialize the buffer descriptor and notify DMA API*/
        dmaBuf->length = RX_BUFFER_SIZE;
        dmaBuf->statCtrl &= (MCD_FEC_WRAP | MCD_FEC_INTERRUPT);
        dmaBuf->statCtrl |= MCD_FEC_BUF_READY;
        fec_rx_continue(ch);
    }
    fecif->rx_tout_magic = 0;
}

/*
 * Receive Frame interrupt
 */
static void fec_rx_frame(struct netif * nif)
{
    fec_if_t *fecif = (fec_if_t *)nif->data;

    // when processing the frame directly, I get Access Faults and other
    // strange stuff.  So this just triggers an timeout.
    // Maybe the reason for the fault is nested interrupts? Dunno.
    if( fecif->rx_tout_magic == 0 )
        fecif->rx_tout_magic = addroottimeout(0, fec0_rx_timeout, 1 );
}


/********************************************************************/
/*
 * Trasmit Frame interrupt handler - this handler is called when the
 * DMA FEC Tx task generates an interrupt
 *
 * Parameters:
 *  ch      FEC channel
 */
static void fec_tx_frame(struct netif * nif)
{
    fec_if_t * fi = (fec_if_t *)nif->data;
    MCD_bufDescFec * dmaBuf;

    dmaBuf = fec_tx_free(fi);
    if( dmaBuf != NULL )
    {
        dmaBuf->length = 0;
        nif->out_packets++;
    }
}

static void fec0_rx_frame(void)
{
    fec_rx_frame(&if_fec[0]);
}

static void fec0_rx_timeout( struct proc * p, long int arg )
{
    fec_rx_timeout( &if_fec[0] );
}




/********************************************************************/
/*
 * Start the FEC Tx DMA task
 *
 * Parameters:
 *  ch      FEC channel
 *  txbd    First Tx buffer descriptor in the chain
 */
static void fec_tx_start(struct netif * nif, s8 *txbd, uint8 pri)
{
    fec_if_t *fecif = (fec_if_t *)nif->data;
    uint8 ch = fecif->ch;
    uint32 initiator;

    if( dma_get_channel( DMA_FEC_TX(ch) ) != -1 )
    {
        fec_tx_stop(fecif);
    }

    /* Make the initiator assignment */
    dma_set_initiator(DMA_FEC_TX(ch));
    /* Grab the initiator number */
    initiator = dma_get_initiator(DMA_FEC_TX(ch));
    /* Determine the DMA channel running the task for the selected FEC */
    fecif->tx_dma_ch = dma_set_channel(DMA_FEC_TX(ch), ch ? NULL : fec0_tx_frame);
    /* Start the Tx DMA task */

    MCD_startDma(fecif->tx_dma_ch, txbd, 0, MCF_FEC_FECTFDR_ADDR(ch), 0, ETH_MTU, 0, initiator, (long)pri,
                          (uint32)(MCD_FECTX_DMA | MCD_INTERRUPT | MCD_TT_FLAGS_CW | MCD_TT_FLAGS_RL | MCD_TT_FLAGS_SP),
                          (uint32)(MCD_NO_CSUM | MCD_NO_BYTE_SWAP)
                         );
}

/********************************************************************/
/*
 * Stop all transmissions on the selected FEC and kill the DMA task
 *
 * Parameters:
 *  ch  FEC channel
 */
static void fec_tx_stop(fec_if_t *fec)
{
    uint32 mask;
    long channel;

    fec_reg_dump(fec->ch);
    /* Save off the EIMR value */
    mask = MCF_FEC_EIMR(fec->ch);
    /* Mask all interrupts */
    MCF_FEC_EIMR(fec->ch) = 0;
    /* If the Ethernet is still enabled... */
    if(MCF_FEC_ECR(fec->ch) & MCF_FEC_ECR_ETHER_EN)
    {
        /* Issue the Graceful Transmit Stop */
        MCF_FEC_TCR(fec->ch) |= MCF_FEC_TCR_GTS;
        /* Wait for the Graceful Stop Complete interrupt */
        int i = 0;
        while(!(MCF_FEC_EIR(fec->ch) & MCF_FEC_EIR_GRA) && i < 1000 )
        {
            if(!(MCF_FEC_ECR(fec->ch) & MCF_FEC_ECR_ETHER_EN))
                break;
            i++;
        }
        if( i==1000 )
            KDEBUG(("Gracefull stop failed!"));

        /* Clear the Graceful Stop Complete interrupt */
        MCF_FEC_EIR(fec->ch) = MCF_FEC_EIR_GRA;
    }
    /* Determine the DMA channel running the task for the selected FEC */
    channel = dma_get_channel(DMA_FEC_TX(fec->ch));

    /* Kill the FEC Tx DMA task */
    if(channel != -1)
        MCD_killDma(channel);

    /* Free up the FEC requestor from the software maintained initiator list */
    dma_free_initiator(DMA_FEC_TX(fec->ch));


    /* Free up the DMA channel */
    dma_free_channel(DMA_FEC_TX(fec->ch));

    /* Restore the interrupt mask register value */
    MCF_FEC_EIMR(fec->ch) = mask;

    fec->tx_dma_ch = -1;

    KDEBUG(("fec_tx_stop done"));
}

/********************************************************************/
/*
 * Transmit Frame interrupt handler - this handler is called when the
 * DMA FEC Tx task generates an interrupt
 *
 * Parameters:
 *  ch      FEC channel
 */

static void fec0_tx_frame(void)
{
    fec_tx_frame(&if_fec[0]);
}

/*

*/

static inline int fec_buf_init(fec_if_t *fec)
{
    int i;
    /* set up pointers to some buffer descriptors: */
    fec->rx_bd = (MCD_bufDescFec *) ALIGN4((unsigned long)sram_unaligned_bds );
    fec->tx_bd = (MCD_bufDescFec *)((long)fec->rx_bd + (sizeof(MCD_bufDescFec) * NUM_RXBDS));

    /* clear pointers, so they can be cleanly handled on alloc failure: */
    for( i=0; i<NUM_RXBDS; i++)
    {
        fec->rx_buf_unaligned[i] = NULL;
    }
    for( i=0; i<NUM_TXBDS; i++)
    {
        fec->tx_buf_unaligned[i] = NULL;
    }

    /* initialise the rx buffer descriptors: */
    for( i=0; i<NUM_RXBDS; i++)
    {
        fec->rx_bd[i].statCtrl = MCD_FEC_BUF_READY | MCD_FEC_INTERRUPT;
        fec->rx_bd[i].length = RX_BUFFER_SIZE;
        fec->rx_buf_unaligned[i] = (uint8 *)dma_alloc(RX_BUFFER_SIZE + 16);
        if( !fec->rx_buf_unaligned[i] )
            return( 1 );
        fec->rx_bd[i].dataPointer = ((uint32)(fec->rx_buf_unaligned[i]  + 15) & 0xFFFFFFF0);
    }
    /* Set wrap bit on last one: */
    fec->rx_bd[i-1].statCtrl |= MCD_FEC_WRAP;

    for( i=0; i<NUM_TXBDS; i++)
    {
        fec->tx_bd[i].statCtrl = MCD_FEC_INTERRUPT;
        fec->tx_bd[i].length = 0;
        fec->tx_buf_unaligned[i] = (uint8 *)dma_alloc(TX_BUFFER_SIZE + 16);
        if( !fec->tx_buf_unaligned[i] )
            return(1);
        fec->tx_bd[i].dataPointer = ((uint32)(fec->tx_buf_unaligned[i] + 15) & 0xFFFFFFF0);
    }
    /* Set wrap bit on last one: */
    fec->tx_bd[i-1].statCtrl |= MCD_FEC_WRAP;

    fec->rx_bd_idx = 0;
    fec->tx_bd_idx_cur = 0;
    fec->tx_bd_idx_next = 0;
    return( 0 );
}


static inline void fec_buf_flush(fec_if_t *fec)
{
    int i;
    for( i=0; i<NUM_RXBDS; i++)
    {
        if( fec->rx_buf_unaligned[i] != NULL)
        {
            dma_free( fec->rx_buf_unaligned[i] );
            fec->rx_buf_unaligned[i] = NULL;
        }
    }
    for( i=0; i<NUM_TXBDS; i++)
    {
        if( fec->tx_buf_unaligned[i] != NULL)
        {
            dma_free(fec->tx_buf_unaligned[i]);
            fec->tx_buf_unaligned[i] = NULL;
        }
    }
}

static inline MCD_bufDescFec * fec_rx_alloc(fec_if_t *fec)
{
    long i = fec->rx_bd_idx;
    /* bail out if the current rx buffer isn't owned by DMA: */
    if( fec->rx_bd[i].statCtrl & MCD_FEC_BUF_READY)
    {
        return( NULL );
    }

    /* increment the circular index */
    fec->rx_bd_idx = (uint8)((i + 1) % NUM_RXBDS);
    return(&fec->rx_bd[i]);
}

/* Return a pointer to the next empty Tx Buffer Descriptor */
static inline MCD_bufDescFec * fec_tx_alloc(fec_if_t *fec)
{
    long i = fec->tx_bd_idx_next;
    /* Check to see if the ring of BDs is full */
    if( (fec->tx_bd[i].statCtrl & MCD_FEC_BUF_READY) )
        return NULL;
    /* increment the circular index */
    fec->tx_bd_idx_next = (uint8)((fec->tx_bd_idx_next+ 1) % NUM_TXBDS);
    return(&fec->tx_bd[i]);
}

static inline MCD_bufDescFec * fec_tx_free(fec_if_t *fec)
{
    long i = fec->tx_bd_idx_cur;

    /* Check to see if the ring of BDs is empty */
    if( (fec->tx_bd[i].statCtrl & MCD_FEC_BUF_READY ) )
        return NULL;
    /* Increment the circular index */
    fec->tx_bd_idx_cur = (uint8)((fec->tx_bd_idx_cur + 1) % NUM_TXBDS);
    return (&fec->tx_bd[i]);
}


/*
 * Initialization. This is called when the driver is loaded. If you
 * link the driver with main.o and init.o then this must be called
 * driver_init() because main() calles a function with this name.
 *
 * You should probe for your hardware here, setup the interface
 * structure and register your interface.
 *
 * This function should return 0 on success and != 0 if initialization
 * fails.
 */
long
driver_init (void)
{
    fec_if_t * fi;
    static char message[100];
    extern DMACF_COOKIE * dmac;

    if( mc_dma_init() == NULL )
    {
        c_conws("Exit: failed to load DMA API!\r\n");
        KDEBUG(("Exit: failed to load DMA API!"));
        return( 1 );
    }

    /*
     * Set interface name
     */
    strcpy (if_fec[0].name, FEC_DEVICE_NAME);
    if_fec[0].unit = if_getfreeunit (FEC_DEVICE_NAME);
    if_fec[0].metric = 0;
    if_fec[0].flags = IFF_BROADCAST;
    if_fec[0].mtu = 1500;
    if_fec[0].timer = 0;
    if_fec[0].hwtype = HWTYPE_ETH;
    if_fec[0].hwlocal.len = ETH_ALEN;
    if_fec[0].hwbrcst.len = ETH_ALEN;

    /*
     * Set interface hardware and broadcast addresses. For real ethernet
     * drivers you must get them from the hardware of course!
     */
    board_get_ethaddr(if_fec[0].hwlocal.adr.bytes);
    memcpy (if_fec[0].hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

    /*
     * Set length of send and receive queue. IF_MAXQ is a good value.
     */
    if_fec[0].rcv.maxqlen = IF_MAXQ;
    if_fec[0].snd.maxqlen = IF_MAXQ;

    /*
     * Setup pointers to service functions
     */
    if_fec[0].open = fec_open;
    if_fec[0].close = fec_close;
    if_fec[0].output = fec_output;
    if_fec[0].ioctl = fec_ioctl;

    /*
     * Optional timer function that is called every 200ms.
     */
    if_fec[0].timeout = 0;

    /*
     * Number of packets the hardware can receive in fast succession,
     * 0 means unlimited.
     */
    if_fec[0].maxpackets = NUM_RXBDS;


    /*
     * Here you could attach some more data your driver may need
     */
    if_fec[0].data = (void*)&fecif_g[0];
    fi = &fecif_g[0];
    memset( if_fec[0].data, sizeof(fec_if_t), 0);
    fi->ch = 0;
    fi->rx_dma_ch = -1;
    fi->tx_dma_ch = -1;




    if (NETINFO->fname)
    {
        if( !strcmp("FEC10.XIF", NETINFO->fname) )
        {
			fi->mode |= FEC_IMODE_10MBIT;
        }

        if( !strcmp("FECP.XIF", NETINFO->fname) )
        {
			fi->mode |= FEC_IMODE_PROMISCUOUS;
        }

        if( !strcmp("FECP10.XIF", NETINFO->fname) )
        {
			fi->mode |= FEC_IMODE_PROMISCUOUS | FEC_IMODE_10MBIT;
        }
    }

    /*
     * Register the interface.
     */
    if_register (&if_fec[0]);

    ksprintf (message, "%s (%s) v%d.%d  (%s%d - %02x:%02x:%02x:%02x:%02x:%02x)\r\n",
              FEC_DRIVER_DESC,
              mch_cookie() == FALCON ? "Firebee" : "m548x",
              FEC_DRIVER_MAJOR_VERSION,
              FEC_DRIVER_MINOR_VERSION,
              FEC_DEVICE_NAME,
              if_fec[0].unit,
              if_fec[0].hwlocal.adr.bytes[0],if_fec[0].hwlocal.adr.bytes[1],
              if_fec[0].hwlocal.adr.bytes[2],if_fec[0].hwlocal.adr.bytes[3],
              if_fec[0].hwlocal.adr.bytes[4],if_fec[0].hwlocal.adr.bytes[5]
             );
    c_conws (message);
    return( 0 );
}


/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
fec_open (struct netif *nif)
{
    long res=0;
    fec_if_t * fi = (fec_if_t*)nif->data;
    uint8 speed = FEC_MII_100BASE_TX;
    uint8 duplex = FEC_MII_FULL_DUPLEX;
    uint8 mode = FEC_MODE_MII;

    if( fi->mode & FEC_IMODE_10MBIT )
	{
		speed = FEC_MII_10BASE_T;
		duplex = FEC_MII_HALF_DUPLEX;
	}

    if( fec_buf_init(fi) )
    {
        c_conws("fec_buf_init failed!\r\n");
        fec_buf_flush(fi);
        res = 1;
    }
    else
    {
        fec_reset(fi->ch);
        fec_init(fi, mode, duplex, (const uint8 *)nif->hwlocal.adr.bytes);

        /* Initialize the PHY interface */
		 if( (mode == FEC_MODE_MII) && mch_cookie() == FALCON ? (am79c874_init(fi->ch, FEC_PHY(fi->ch), speed, duplex) == 0)  :
															   (bcm5222_init(fi->ch, FEC_PHY(fi->ch), speed, duplex) == 0))
        {
            /* Flush the network buffers */
	          c_conws("ethernet PHY init failed!\r\n");
            fec_buf_flush(fi);
            res = 1;
        }
        else
        {
            /* Enable the multi-channel DMA tasks */
            KDEBUG(("starting multi-channel DMA tasks\r\n"));
            fec_rx_start(nif, (s8*)fi->rx_bd, fi->ch ? FEC1RX_DMA_PRI : FEC0RX_DMA_PRI);
            fec_tx_start(nif, (s8*)fi->tx_bd, fi->ch ? FEC1TX_DMA_PRI : FEC0TX_DMA_PRI);
            KDEBUG(("enable network interface\r\n"));
            fec_enable(fi->ch);
        }
    }
    return res;
}

static long fec_close( struct netif *nif )
{
    int level;
    fec_if_t * fi = (fec_if_t*)nif->data;
    /* Disable interrupts */
    level = spl7();
    /* Gracefully disable the receiver and transmitter */
    fec_tx_stop(fi);
    fec_rx_stop(fi);
    /* Disable FEC interrupts */
    fec_disable(fi->ch);
    /* Flush the network buffers */
    fec_buf_flush(fi);
    /* Restore interrupt level */
    spl(level);
    return( 0 );
}


/*
 * FEC output function, for general documentation look into dummyeth.c
 */

static long
fec_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
    fec_if_t * fi = (fec_if_t*)nif->data;
    BUF *nbuf;
    int len,lenpad;
    MCD_bufDescFec *dmaBuf;
    long res = E_OK;

    /* Not ready yet, failed to alloc dma channel or shutdown state? */
    if( fi->tx_dma_ch < 0 )
    {
        buf_deref( buf, BUF_NORMAL );
        return( EINTERNAL );
    }

    len = ((unsigned long)buf->dend) - ((unsigned long)buf->dstart);
    if( len > nif->mtu || len < 1 )
    {
        KDEBUG(("fec_output: packet to large/small!"));
        nif->out_errors++;
        buf_deref( buf, BUF_NORMAL );
        return( EINTERNAL );
    }

    /* Attach ethernet header with xif util function: */
    nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
    if (nbuf == 0)
    {
        KDEBUG(("fec_output: eth_buld_hdr failed!"));
        nif->out_errors++;
        res = ENOMEM;
        goto out_err0;
    }
    /* packet is resized, recalculate len: */
    len = ((unsigned long)nbuf->dend) - ((unsigned long)nbuf->dstart);

    /* FEC needs at least ETH_MIN_FRM: */
    lenpad = MAX(len, ETH_MIN_FRM );

    if( lenpad > ETH_MAX_FRM )
    {
        KDEBUG(("fec_output: frame to large!"));
        nif->out_errors++;
        res = ENOMEM;
        goto out_err1;
    }

    /* tell packet "filter" about the packet (it's more like an monitor?) */
    if (nif->bpf)
        bpf_input(nif, nbuf);

    if( (dmaBuf = fec_tx_alloc(fi)) != NULL )
    {
        if( lenpad != len )
        {
            memset( (void*)dmaBuf->dataPointer, lenpad, 0);
        }
        memcpy( (void*)dmaBuf->dataPointer, nbuf->dstart, len );
        dmaBuf->length = lenpad;
        /* Set Frame ready for transmission */
        dmaBuf->statCtrl |= ( MCD_FEC_BUF_READY | MCD_FEC_END_FRAME );
        MCD_continDma(fi->tx_dma_ch);
    }
    else
    {
        KDEBUG(("bnuf_tx_alloc failed!"));
        res = ENOMEM;
    }

out_err1:
    buf_deref( nbuf, BUF_NORMAL );
out_err0:
    return res;
}



/*
 * MintNet notifies you of some noteable IOCLT's. Usually you don't
 * need to act on them because MintNet already has done so and only
 * tells you that an ioctl happened.
 *
 * One useful thing might be SIOCGLNKFLAGS and SIOCSLNKFLAGS for setting
 * and getting flags specific to your driver. For an example how to use
 * them look at slip.c
 */
static long
fec_ioctl (struct netif *nif, short cmd, long arg)
{
    struct ifreq *ifr;

    switch (cmd)
    {
    case SIOCSIFNETMASK:
    case SIOCSIFFLAGS:
    case SIOCSIFADDR:
        return 0;

    case SIOCSIFMTU:
        /*
         * Limit MTU to 1500 bytes. MintNet has alraedy set nif->mtu
         * to the new value, we only limit it here.
         */
        if (nif->mtu > ETH_MAX_DLEN)
            nif->mtu = ETH_MAX_DLEN;
        return 0;

    case SIOCSIFOPT:
        /*
         * Interface configuration, handled by fec_config()
         */
        ifr = (struct ifreq *) arg;
        return fec_config (nif, ifr->ifru.data);
    }

    return ENOSYS;
}

/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that fec_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
fec_config (struct netif *nif, struct ifopt *ifo)
{
    fec_if_t * fi = (fec_if_t*)nif->data;

# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

    if (!STRNCMP ("hwaddr"))
    {
        /*
         * Set hardware address
         */
        if (ifo->valtype != IFO_HWADDR)
            return ENOENT;
        memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
        fec_set_address( ((fec_if_t*)nif->data)->ch, nif->hwlocal.adr.bytes );
    }
    else if (!STRNCMP ("braddr"))
    {
        /*
         * Set broadcast address
         */
        if (ifo->valtype != IFO_HWADDR)
            return ENOENT;
        memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
    }
    else if (!STRNCMP ("debug"))
    {
        /*
         * turn debuggin on/off
         */
        if (ifo->valtype != IFO_INT)
            return ENOENT;
        //KDEBUG (("fec: debug level is %ld", ifo->ifou.v_long));
        if( ifo->ifou.v_long >= 2 )
        {
            fec_reg_dump( fi->ch );
        }
    }
    else if(!STRNCMP("promisc"))
    {
        // TBD restart FEC and set promisc mode
        if (ifo->valtype != IFO_STRING)
            return ENOENT;
    }
    else if (!STRNCMP ("log"))
    {
        /*
         * set log file
         */
        if (ifo->valtype != IFO_STRING)
            return ENOENT;
        //KDEBUG (("fec: log file is %s", ifo->ifou.v_string));
    }

    return ENOSYS;
}


