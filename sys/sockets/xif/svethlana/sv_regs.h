#ifndef __SV_REGS_H__
#define __SV_REGS_H__

/*Last edit by TG on 2012-02-15*/
/* added Superblitter interrupt register bits */

#ifdef _mint_ktypes_h
	typedef u_int32_t		uint32_t;
	typedef u_int32_t		uint32;
	typedef u_int16_t		uint16_t;
	typedef u_int16_t		uint16;
	typedef int32_t			int32;
	typedef int16_t			int16;
#else
	#include <stdint.h>
#endif

#define LONGREG(addr)		(*(volatile uint32_t *) addr)
#define LONGPNT(addr)		((volatile uint32_t *) addr)


#define DDR_BASE						(0xA0000000UL + 0x01000000UL)
#define DDR_SIZE						(0x08000000UL - 0x01000000UL)

/*Defines for the general purpose settings/status register (SV_CTRL)*/
#define SV_CTRL				(*(volatile uint32_t *) 0x80010000)
#define VGARESET				0x01UL 
#define DVIRESET				0x02UL
#define VGA_VSYNC				0x04UL
#define VGA_HSYNC				0x08UL
#define DVI_VSYNC				0x10UL
#define DVI_HSYNC				0x20UL
#define SV_REGS_WRITTEN_LAST	0x40UL
#define DVI_CONNECTED			0x80UL
#define MON_TYPE_MASK			0x300UL
#define MON_TYPE_MONO			0x000UL
#define MON_TYPE_RGB			0x100UL
#define MON_TYPE_VGA			0x200UL
#define MON_TYPE_TV				0x300UL
#define F030_32MHZ_VALID		0x400UL
#define F030_25MHZ_VALID		0x800UL
#define VGA_SEL_25MHZ			0x1000UL
#define VGA_SEL_CDCE_CLK		0x2000UL
#define DVI_SEL_25MHZ			0x4000UL
#define DVI_SEL_CDCE_CLK		0x8000UL
#define VGA_PROTECT				0x10000UL
#define DVI_PROTECT				0x20000UL
#define HW_REVISION				0x40000UL
#define VGA_VSYNC_INT_EN		0x80000UL
#define VGA_VSYNC_INT_FLAG		0x100000UL
#define DVI_VSYNC_INT_EN		0x200000UL
#define DVI_VSYNC_INT_FLAG		0x400000UL


#define SV_DDRCLK_PLL1_1		(*(volatile uint32_t *) 0x80010004)
#define SV_DDRCLK_PLL1_2		(*(volatile uint32_t *) 0x80010008)
#define SV_VIDEOCLK_PLL2_1		(*(volatile uint32_t *) 0x8001000C)
#define SV_VIDEOCLK_PLL2_2		(*(volatile uint32_t *) 0x80010010)

#define SV_256C_PALETTE_BASE	0xFFFF9800UL

/*JTAG control register*/
#define SV_JTAG_CTRL			(*(volatile uint32_t *) 0x80010054)
#define MAGIC_KEY				0x55550000UL
#define MAGIC_KEY_MASK			0x7FFF0000UL
#define JTAG_ENABLE				0x80000000UL
#define JTAG_TDO				0x00000001UL
#define JTAG_TDI				0x00000002UL
#define JTAG_TMS				0x00000004UL
#define JTAG_TCK				0x00000008UL


/*VGA resolution registers*/
#define SV_VGA_SCREENADDR		(*(volatile uint32_t *) 0x80010014)
#define SV_VGA_HDATA			(*(volatile uint32_t *) 0x80010018)
#define SV_VGA_HSYNC			(*(volatile uint32_t *) 0x8001001C)
#define SV_VGA_VDATA			(*(volatile uint32_t *) 0x80010020)
#define SV_VGA_VSYNC			(*(volatile uint32_t *) 0x80010024)
#define SV_VGA_LAST_HV_POS		(*(volatile uint32_t *) 0x80010028)
#define SV_VGA_DATA_AMOUNT		(*(volatile uint32_t *) 0x8001002C)
#define SV_VGA_VIDEOMODE		(*(volatile uint32_t *) 0x80010030)


/*DVI resolution registers*/
#define SV_DVI_SCREENADDR		(*(volatile uint32_t *) 0x80010034)
#define SV_DVI_HDATA			(*(volatile uint32_t *) 0x80010038)
#define SV_DVI_HSYNC			(*(volatile uint32_t *) 0x8001003C)
#define SV_DVI_VDATA			(*(volatile uint32_t *) 0x80010040)
#define SV_DVI_VSYNC			(*(volatile uint32_t *) 0x80010044)
#define SV_DVI_LAST_HV_POS		(*(volatile uint32_t *) 0x80010048)
#define SV_DVI_DATA_AMOUNT		(*(volatile uint32_t *) 0x8001004C)
#define SV_DVI_VIDEOMODE		(*(volatile uint32_t *) 0x80010050)



/*Defines for the VGA/DVI data amount registers (SV_xxx_DATA_AMOUNT)*/
#define VIRTDATA_MASK	0x03FF0000UL
#define VISIDATA_MASK	0x000003FFUL

/*Defines for the VGA/DVI video mode settings register (SV_xxx_VIDEOMODE)*/
#define VIDEO_MODE_MASK	0x7UL
#define VIDMOD_1B_BPL	0x0UL
#define VIDMOD_2B_BPL	0x1UL
#define VIDMOD_4B_BPL	0x2UL
#define VIDMOD_8B_BPL	0x3UL
#define VIDMOD_8B_CHY	0x4UL		/*8bit chunky*/
#define VIDMOD_16B		0x5UL		/*16bit highcolor (rrrrrggggggbbbbb)*/
#define VIDMOD_24B		0x6UL		/*24bit packed RGB mode (3byte, 3byte...)*/
#define VIDMOD_32B		0x7UL		/*24bit non-packed RGB(4byte: RGxB, RGxB...)*/
#define VIDMOD_STCOMP	0x8UL		/*ST-compatibility (no effect right now)*/
#define VIDMOD_DOUBPIX	0x10UL	/*Double pixel width (e.g 640x480->320x480)*/
#define VIDMOD_LINEDOUB	0x20UL	/*Line doubling (e.g 640x480->640x240)*/

/*The SuperBlitter registers*/
#define SB_SRC1_ADDR			(*(volatile uint32_t *) 0x80010058)
#define SB_SRC2_ADDR			(*(volatile uint32_t *) 0x8001005C)
#define SB_DEST_ADDR			(*(volatile uint32_t *) 0x80010060)
#define SB_LINE_BYTECNT		(*(volatile uint32_t *) 0x80010064)
#define SB_SRC1_LINEOFFS	(*(volatile uint32_t *) 0x80010068)
#define SB_SRC2_LINEOFFS	(*(volatile uint32_t *) 0x8001006C)
#define SB_DEST_LINEOFFS	(*(volatile uint32_t *) 0x80010070)
#define SB_NBR_LINES			(*(volatile uint32_t *) 0x80010074)
#define SB_CTRL_STATUS		(*(volatile uint32_t *) 0x80010078)
#define SB_FIFO_PORT			(*(volatile uint32_t *) 0x80010080)

#define SB_STATUS_BUSY								0x00000001UL
#define SB_CTRL_START									0x00000001UL
#define SB_CTRL_INT_ENABLE						0x00000020UL
#define SB_STATUS_DONE_INT_FLAG				0x00000040UL
/*Blit modes*/
#define SB_BLIT_STD										0x00
#define SB_BLIT_BYTE_MASK							(0x01<<1)
#define SB_BLIT_ALPHA									(0x03<<1)
#define SB_BLIT_ALPHA_CONST						(0x07<<1)

#define SB_FIFO_PORT_EMPTY						0x00000001UL
#define SB_FIFO_PORT_FULL							0x00000002UL


#define SV_VERSION										(*(volatile uint32_t *) 0x8001007C)

/*********************************** 
	Supervidel interrupt vectors
***********************************/
#define ETH_INT_VECTOR		((volatile uint32_t *) 0x00000314)
#define SB_INT_VECTOR		((volatile uint32_t *) 0x00000318)
#define SV_VGA_INT_VECTOR	((volatile uint32_t *) 0x0000031C)
#define SV_DVI_INT_VECTOR	((volatile uint32_t *) 0x00000320)



/*******************************************
Defines for the SV Videl snoop fifo
*******************************************/
//#define SNOOPFIFO	(*(volatile uint32_t *) 0x8001007C)



/*******************************************
Defines for the I2C controller in the FPGA
*******************************************/
/*Registers of the I2C-DVI controller */
#define I2CDVI_PRERlo	(*(volatile unsigned char *) 0x80011103)
#define I2CDVI_PRERhi	(*(volatile unsigned char *) 0x80011107)
#define I2CDVI_CTR		(*(volatile unsigned char *) 0x8001110B)
#define I2CDVI_TXR		(*(volatile unsigned char *) 0x8001110F)
#define I2CDVI_RXR		(*(volatile unsigned char *) 0x8001110F)
#define I2CDVI_COM		(*(volatile unsigned char *) 0x80011113)
#define I2CDVI_SR		(*(volatile unsigned char *) 0x80011113)

/*Control register bits*/
#define	EN		0x80

/*Command register bits*/
#define	STA		0x80
#define	STO		0x40
#define	RD		0x20
#define	WR		0x10
#define	ACK		0x08

/*Status register bits*/
#define	RxACK	0x80
#define	TIP		0x02



/*The I2C adresses of the DVI chip for writing and reading*/
#define TFP410_ADDR_WR	0x70
#define TFP410_ADDR_RD	0x71

/*I2C registers in the DVI chip*/
#define VEN_ID_HI		0x01
#define VEN_ID_LO		0x00
#define DEV_ID_HI		0x03
#define DEV_ID_LO		0x02
#define CTL_1_MODE	0x08
#define DE_CTL			0x33

/*DE_CTL register bits*/
#define VS_POL	0x20		//TODO: Verify this bit in the DVI datasheet!
#define HS_POL	0x10		//TODO: Verify this bit in the DVI datasheet!

/*************************************
  Ethernet MAC controller registers
**************************************/
//Define the number of packet buffers each for TX and RX
#define ETH_PKT_BUFFS		64UL

/*
Struct used for both TX and RX BD. Note that you can only access
these registers as whole longwords, so there's no point in defining
the bit fields within.
*/
typedef volatile struct
{
	volatile uint32_t	len_ctrl;
	volatile uint32_t	data_pnt;
} ETH_BD_TYPE;

#define ETH_MODER			LONGREG(0x80012000UL)
#define ETH_INT_SOURCE		LONGREG(0x80012004UL)
#define ETH_INT_MASK		LONGREG(0x80012008UL)
#define ETH_IPGT			LONGREG(0x8001200CUL)
#define ETH_IPGR1			LONGREG(0x80012010UL)
#define ETH_IPGR2			LONGREG(0x80012014UL)
#define ETH_PACKETLEN		LONGREG(0x80012018UL)
#define ETH_COLLCONF		LONGREG(0x8001201CUL)
#define ETH_TX_BD_NUM		LONGREG(0x80012020UL)
#define ETH_CTRLMODER		LONGREG(0x80012024UL)
#define ETH_MIIMODER		LONGREG(0x80012028UL)
#define ETH_MIICOMMAND		LONGREG(0x8001202CUL)
#define ETH_MIIADDRESS		LONGREG(0x80012030UL)
#define ETH_MIITX_DATA		LONGREG(0x80012034UL)
#define ETH_MIIRX_DATA		LONGREG(0x80012038UL)
#define ETH_MIISTATUS		LONGREG(0x8001203CUL)
#define ETH_MAC_ADDR0		LONGREG(0x80012040UL)
#define ETH_MAC_ADDR1		LONGREG(0x80012044UL)
#define ETH_ETH_HASH0_ADR	LONGREG(0x80012048UL)
#define ETH_ETH_HASH1_ADR	LONGREG(0x8001204CUL)
#define ETH_ETH_TXCTRL		LONGREG(0x80012050UL)

/* Eth MAC BD base */
#define ETH_BD_BASE_PNT		0x80012400UL

/*Define TX BD and put at offset 0 in the BD space*/
/*ETH_BD_TYPE * const eth_tx_bd = (ETH_BD_TYPE*)ETH_BD_BASE_PNT;*/
#define eth_tx_bd  ((ETH_BD_TYPE*)ETH_BD_BASE_PNT)

/*Define RX BD and put two slots away in the BD space*/
/*ETH_BD_TYPE * const eth_rx_bd = (ETH_BD_TYPE*)(ETH_BD_BASE_PNT + 2UL * sizeof(ETH_BD_TYPE));*/
#define eth_rx_bd  ((ETH_BD_TYPE*)(ETH_BD_BASE_PNT + ETH_PKT_BUFFS * sizeof(ETH_BD_TYPE)))

/* Data buffer pointer */
#define ETH_DATA_BASE_ADDR	0x80012800UL
#define ETH_DATA_BASE_PNT	LONGPNT(ETH_DATA_BASE_ADDR)

/* Eth TX BD MASKS */
#define ETH_TX_BD_READY		0x00008000UL
#define ETH_TX_BD_IRQ		0x00004000UL
#define ETH_TX_BD_WRAP		0x00002000UL
#define ETH_TX_BD_PAD		0x00001000UL
#define ETH_TX_BD_CRC		0x00000800UL
#define ETH_TX_BD_UNDERRUN	0x00000100UL
#define ETH_TX_BD_RETRY		0x000000F0UL
#define ETH_TX_BD_RETLIM	0x00000008UL
#define ETH_TX_BD_LATECOL	0x00000004UL
#define ETH_TX_BD_DEFER		0x00000002UL
#define ETH_TX_BD_CARRIER	0x00000001UL

/* Eth RX BD MASKS */
#define ETH_RX_BD_EMPTY		0x00008000UL
#define ETH_RX_BD_IRQ		0x00004000UL
#define ETH_RX_BD_WRAP		0x00002000UL
#define ETH_RX_BD_MISS		0x00000080UL
#define ETH_RX_BD_OVERRUN	0x00000040UL
#define ETH_RX_BD_INVSIMB	0x00000020UL
#define ETH_RX_BD_DRIBBLE	0x00000010UL
#define ETH_RX_BD_TOOLONG	0x00000008UL
#define ETH_RX_BD_SHORT		0x00000004UL
#define ETH_RX_BD_CRCERR	0x00000002UL
#define ETH_RX_BD_LATECOL	0x00000001UL

#define ETH_MODER_RXEN		0x00000001UL
#define ETH_MODER_TXEN		0x00000002UL
#define ETH_MODER_NOPRE		0x00000004UL
#define ETH_MODER_BRO		0x00000008UL
#define ETH_MODER_IAM		0x00000010UL
#define ETH_MODER_PRO		0x00000020UL
#define ETH_MODER_IFG		0x00000040UL
#define ETH_MODER_LOOPBCK	0x00000080UL
#define ETH_MODER_NOBCKOF	0x00000100UL
#define ETH_MODER_EXDFREN	0x00000200UL
#define ETH_MODER_FULLD		0x00000400UL
#define ETH_MODER_RST		0x00000800UL
#define ETH_MODER_DLYCRCEN	0x00001000UL
#define ETH_MODER_CRCEN		0x00002000UL
#define ETH_MODER_HUGEN		0x00004000UL
#define ETH_MODER_PAD		0x00008000UL
#define ETH_MODER_RECSMALL	0x00010000UL

#define ETH_INT_TXB			0x00000001UL
#define ETH_INT_TXE			0x00000002UL
#define ETH_INT_RXB			0x00000004UL
#define ETH_INT_RXE			0x00000008UL
#define ETH_INT_BUSY		0x00000010UL
#define ETH_INT_TXC			0x00000020UL
#define ETH_INT_RXC			0x00000040UL

#define ETH_INT_MASK_TXB	0x00000001UL
#define ETH_INT_MASK_TXE	0x00000002UL
#define ETH_INT_MASK_RXF	0x00000004UL
#define ETH_INT_MASK_RXE	0x00000008UL
#define ETH_INT_MASK_BUSY	0x00000010UL
#define ETH_INT_MASK_TXC	0x00000020UL
#define ETH_INT_MASK_RXC	0x00000040UL

#define ETH_CTRLMODER_PASSALL	0x00000001UL
#define ETH_CTRLMODER_RXFLOW	0x00000002UL
#define ETH_CTRLMODER_TXFLOW	0x00000004UL

#define ETH_MIIMODER_CLKDIV			0x000000FFUL
#define ETH_MIIMODER_NOPRE			0x00000100UL
#define ETH_MIIMODER_RST			0x00000200UL

#define ETH_MIICOMMAND_SCANSTAT		0x00000001UL
#define ETH_MIICOMMAND_RSTAT		0x00000002UL
#define ETH_MIICOMMAND_WCTRLDATA	0x00000004UL

#define ETH_MIIADDRESS_FIAD_MASK	0x0000001FUL
#define ETH_MIIADDRESS_FIAD(x)		((uint32_t)((x << 0) & ETH_MIIADDRESS_FIAD_MASK))
#define ETH_MIIADDRESS_RGAD_MASK	0x00001F00UL
#define ETH_MIIADDRESS_RGAD(x)		((uint32_t)((x << 8) & ETH_MIIADDRESS_RGAD_MASK))


#define ETH_MIISTATUS_LINKFAIL		0x00000001UL
#define ETH_MIISTATUS_BUSY			0x00000002UL
#define ETH_MIISTATUS_NVALID		0x00000004UL

#define ETH_TX_CTRL_TXPAUSERQ		0x00010000UL


#endif /*__SV_REGS_H__*/
