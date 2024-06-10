/*
 * V2ExpEth ethernet driver for FreeMiNT.
 *
 * Author:
 *  Mark Duckworth
 * 
 * Contributors:
 *  Henryk Richter - Vampire V2ExpEth Driver for AmigaOS
 *  Christian Vogelgsaang
 *  Guido Socher
 *  Original ENC28J60 Driver: Pascal Stang
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "global.h"
#include "buf.h"
#include "cookie.h"
#include "inet4/if.h"
#include "inet4/ip.h"
#include "inet4/arp.h"
#include "inet4/ifeth.h"
#include "netinfo.h"
#include "v2expeth.h"
#include "arch/tosbind.h"
#include "mint/sockio.h"
#include "mint/osbind.h"
#include "mint/ssystem.h"
#include "enc28j60.h"
#include "mint/arch/asm_spl.h"

#define V2SDNET_DEBUG	0

static char dbgbuf[256];
#define DPRINT(...)			{ ksprintf(dbgbuf, __VA_ARGS__); c_conws("v2expeth: "); c_conws(dbgbuf); c_conws("\n\r"); KERNEL_DEBUG(dbgbuf); }
#if V2SDNET_DEBUG > 0
#define DWARN(...)			DPRINT(__VA_ARGS__)
#else
#define DWARN(...)			
#endif
#if V2SDNET_DEBUG > 1
#define DDBG(...)			DPRINT(__VA_ARGS__)
#else
#define DDBG(...)			
#endif

#define V2_SDCARD_DATA				(*(volatile u_int8_t*)0xde0010)
#define V2_SDCARD_CFG_W				(*(volatile u_int16_t*)0xde0014)
#define V2_SDCARD_STATUS_R			(*(volatile u_int32_t*)0xde0060)
#define V2_SDCARD_CFG_SPEED			(*(volatile u_int16_t*)0xde001c)
#define V2_SDCARD_INT_ENABLE		(*(volatile u_int32_t*)0xde0018)

#define V2_SDCARD_SPEED 0x20
#define V2_SDCARD_CS 1
#define V2_SDCARD_CSn 0

#define RXSTART_INIT        0x0000  /*  start of RX buffer, room for 4 packets */
#define RXSTOP_INIT         0x19FF  /*  end of RX buffer */
#define TXSTART_INIT        0x1A00  /*  start of TX buffer, room for 1 packet */
#define TXSTOP_INIT         0x1FFF  /*  end of TX buffer */

/* PHIE Register */
#define PHIE_PGEIE	(1<<1)
#define PHIE_LNKIE	(1<<4)

#define SANITY_CHECK_INTERVAL 512
#define _OPT_READ_CONST = 0xff

/* result values */
#define PIO_OK            0
#define PIO_NOT_FOUND     1
#define PIO_TOO_LARGE     2
#define PIO_IO_ERR        3

/* init flags */
#define PIO_INIT_FULL_DUPLEX    1
#define PIO_INIT_LOOP_BACK      2
#define PIO_INIT_BROAD_CAST     4
#define PIO_INIT_FLOW_CONTROL   8
#define PIO_INIT_MULTI_CAST     16
#define PIO_INIT_PROMISC        32

/* status flags */
#define PIO_STATUS_VERSION      0
#define PIO_STATUS_LINK_UP      1 
#define PIO_STATUS_FULLDPX      2

/* control ids */
#define PIO_CONTROL_FLOW        0

struct enc28j60_initvars {
	u_int8_t  	mac[6];
	u_int16_t	have_recv;
	u_int8_t	macon1;
	u_int8_t	macon2;
	u_int8_t	macon3;
	u_int8_t	macon4;
};
static struct enc28j60_initvars initvars;

/*
 * Prototypes for our service functions
 */
static long	v2expeth_open(struct netif *);
static long	v2expeth_close(struct netif *);
static long	v2expeth_output(struct netif *, BUF *, const char *, short, short);
static long	v2expeth_ioctl(struct netif *, short, long);
static long	v2expeth_config(struct netif *, struct ifopt *);
void v2expeth_int_handler(void);
void _cdecl v2expeth_recv(void);

static struct netif if_v2expeth;
static volatile u_int32_t alignmem = 0;
static volatile u_int32_t upcfg = 0;
static u_int8_t Enc28j60Bank;

static u_int16_t gNextPacketPtr;
static u_int8_t is_full_duplex = 1;
static u_int8_t rev;

u_int8_t enc28j60_has_recv(void);
u_int8_t enc28j60_status(u_int8_t status_id, u_int8_t *value);
static void enc28j60SetBank(volatile u_int8_t address);

static inline void csdelay()
{
	asm("tst.w	0xDFF00A");
}

static void enc28j60WriteOp(volatile u_int8_t op, volatile u_int8_t address, volatile u_int8_t data)
{
	volatile u_int8_t waitdata;

	// Looks like an infinite loop but SetBank uses the ECON1 register so it will bypass
	// The other all bank registers aren't called as often so a spurious bank switch is okay
	if ((address & ADDR_MASK) < EIE)
		enc28j60SetBank(address);

	// spi enable
	V2_SDCARD_CFG_W = V2_SDCARD_CSn+V2_SDCARD_SPEED;
	csdelay();

	V2_SDCARD_DATA = op | (address & ADDR_MASK);
	waitdata = V2_SDCARD_DATA;
	V2_SDCARD_DATA = data;
	waitdata = V2_SDCARD_DATA;

	csdelay();

	//spi disable
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");

	UNUSED(waitdata);
}

static void readBuf(u_int16_t len, u_int8_t* data)
{
	volatile u_int8_t waitdata;

	// spi enable
	V2_SDCARD_CFG_W = V2_SDCARD_CSn+V2_SDCARD_SPEED;
	csdelay();

	V2_SDCARD_DATA = ENC28J60_READ_BUF_MEM;
	waitdata = V2_SDCARD_DATA;

	while (len--)
	{
		V2_SDCARD_DATA = 0xff;
		*data++ = V2_SDCARD_DATA;
	}

	//spi disable
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	UNUSED(waitdata);
}

static void enc28j60SetBank(volatile u_int8_t address)
{
	// set the bank (if needed)
	if((address & BANK_MASK) != Enc28j60Bank)
	{
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

static void writeReg(volatile u_int8_t address, volatile  u_int16_t data)
{
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data & 0xff);
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address+1, data >> 8);
}

static void writeRegByte (volatile u_int8_t address, volatile u_int8_t data)
{
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

static void writeBuffer(u_int8_t *data, u_int16_t size, u_int16_t destptr)
{
	volatile u_int8_t waitdata;
	writeReg(EWRPTL, destptr);

	// spi enable
	V2_SDCARD_CFG_W = V2_SDCARD_CSn+V2_SDCARD_SPEED;
	csdelay();

	V2_SDCARD_DATA = ENC28J60_WRITE_BUF_MEM;
	waitdata = V2_SDCARD_DATA;
	V2_SDCARD_DATA = 0x0f; // 1st byte is cotrol byte
	waitdata = V2_SDCARD_DATA;

	while (size--)
	{
		V2_SDCARD_DATA = *data++;
		waitdata = V2_SDCARD_DATA;
	}

	csdelay();

	// spi disable
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;

	// testing delay
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");

	UNUSED(waitdata);
}

static u_int8_t enc28j60ReadOp(volatile u_int8_t op, volatile u_int8_t address)
{
	volatile u_int8_t data;
   
	// spi enable
	V2_SDCARD_CFG_W = V2_SDCARD_CSn+V2_SDCARD_SPEED;
	csdelay();

	V2_SDCARD_DATA = op | (address & ADDR_MASK);
	csdelay();

	V2_SDCARD_DATA = 0;
	if (address & 0x80) // dummy read indicated
	{
		data = V2_SDCARD_DATA;
		V2_SDCARD_DATA = 0;
	}
	data = V2_SDCARD_DATA;

	// 10ns CShold time
	asm("nop;");
	//csdelay();

	// spi disable
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");

	return data;
}

static inline u_int8_t readRegByte (volatile u_int8_t address)
{
	// The ECON1 and EIR (and some others) registers are available regardless of selected bank
	if ((address & ADDR_MASK) < EIE)
		enc28j60SetBank(address);
	u_int8_t val = enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
	return val;
}

/*
  the PHY registers require special care
*/
static u_int16_t readPhyByte (u_int8_t address) 
{
    long wtime = 0xffff;
    
    writeRegByte(MIREGADR, address);
    writeRegByte(MICMD, MICMD_MIIRD);
    while (readRegByte(MISTAT) & MISTAT_BUSY)
    {
		if( (--wtime) <= 0 )
		{
			writeRegByte(MICMD, 0x00);
			initvars.have_recv = SANITY_CHECK_INTERVAL+1;
			return 0xffff;
		}
    }
    writeRegByte(MICMD, 0x00);
    return readRegByte(MIRDL+1);
}

static void writePhy (volatile u_int8_t address, volatile u_int16_t data)
{
    long wtime = 0xffff;

    writeRegByte(MIREGADR, address);
    writeReg(MIWRL, data);
    while (readRegByte(MISTAT) & MISTAT_BUSY)
    {
		if( (--wtime) <= 0 )
		{
			initvars.have_recv = SANITY_CHECK_INTERVAL+1;
			break;
		}
    }
}

static void enc28j60_setMAC( const u_int8_t macaddr[6] )
{
	/*  set mac     */
	DDBG("Setting mac to %x:%x:%x:%x:%x:%x\r\n", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	writeRegByte(MAADR5, macaddr[0]);
	writeRegByte(MAADR4, macaddr[1]);
	writeRegByte(MAADR3, macaddr[2]);
	writeRegByte(MAADR2, macaddr[3]);
	writeRegByte(MAADR1, macaddr[4]);
	writeRegByte(MAADR0, macaddr[5]);
	enc28j60SetBank(EIR);
}

static void enc28j60_recv_reset( struct enc28j60_initvars *iv, u_int8_t restart)
{
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXRST);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXRST);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ESTAT, ESTAT_TXABRT);

	gNextPacketPtr = RXSTART_INIT;
	DDBG("Reset gnp:%0x", gNextPacketPtr);
	writeReg(ERDPTL, gNextPacketPtr);
	writeReg(ERXSTL,   RXSTART_INIT);
	writeReg(ERXRDPTL, RXSTOP_INIT );
	writeReg(ERXNDL,   RXSTOP_INIT);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF|EIR_RXERIF );
	// MAX_FRAMELEN 1518
	writeReg(MAMXFLL, 1518);

	writeRegByte(MACON4, iv->macon4 ); /* DEFER bit */
	writeRegByte(MACON3, iv->macon3 );
	writeRegByte(MACON2, iv->macon2 );
	writeRegByte(MACON1, iv->macon1 );

	enc28j60_setMAC( iv->mac );

	writePhy(PHIE, PHIE_PGEIE|PHIE_LNKIE );
 	writePhy(PHLCON, 0x3c12 ); /* 3(fixed) C(LinkStat+RX Act) 1(TX Act) 2(Stretch enable) */
 
	/*  prepare flow control */
	//writeReg(EPAUSL, 20 * 100); /*  100ms */

	if( restart )
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	iv->have_recv = 0;
}

static int  enc28j60_check_config( struct enc28j60_initvars *iv )
{
	int	ret = 1;	/* def: be optimistic */

	/* check RX registers */
	if( !(readRegByte(ECON1) & ECON1_RXEN ) )
		ret = 0;
	if( (readRegByte(MACON1)&iv->macon1) != iv->macon1 )
		ret = 0;
	if( (readRegByte(MACON2)&iv->macon2) != iv->macon2 )
		ret = 0;
	if( (readRegByte(MACON3)&iv->macon3) != iv->macon3 )
		ret = 0;

	/* check TX problems */
	if( readRegByte(ESTAT) & ESTAT_TXABRT )
		ret = 0;
	if( readRegByte(EIR) & EIR_TXERIF )
		ret = 0;

	/* Verify MAC */
	if( readRegByte(MAADR5) != iv->mac[0] )
		ret = 0;
	if( readRegByte(MAADR4) != iv->mac[1] )
		ret = 0;
	if( readRegByte(MAADR3) != iv->mac[2] )
		ret = 0;
	if( readRegByte(MAADR2) != iv->mac[3] )
		ret = 0;
	if( readRegByte(MAADR1) != iv->mac[4] )
		ret = 0;
	if( readRegByte(MAADR0) != iv->mac[5] )
		ret = 0;

	return	ret;
}

u_int8_t enc28j60_has_recv(void)
{
  u_int8_t ret = readRegByte(EPKTCNT);

  if( !ret )
  {
 	initvars.have_recv++;
	if( initvars.have_recv > SANITY_CHECK_INTERVAL )
	{	
		/* check for bad config vars and reset hw if that is the case */ 
		if( !enc28j60_check_config( &initvars ) )
		{
			DDBG("Bad config check, resetting");
			enc28j60_recv_reset( &initvars, 1 );
		}
	}
  }
  else	
  	initvars.have_recv = 0;

  return ret;
}

u_int8_t enc28j60_status(u_int8_t status_id, u_int8_t *value)
{
  switch(status_id) {
    case PIO_STATUS_VERSION:
      *value = rev;
      return PIO_OK;
    case PIO_STATUS_LINK_UP:
      *value = (readPhyByte(PHSTAT2) >> 2) & 1;
      return PIO_OK;
    case PIO_STATUS_FULLDPX:
      *value = (readRegByte(MACON3) & MACON3_FULDPX) ? 1 : 0;/*  + (((u16)readRegByte(MACON1+3))<<8); */
      return PIO_OK;
    default:
      *value = 0;
      return PIO_NOT_FOUND;
  }
}

/*
void enc28j60RegDump(void)
{
	DDBG("RevID: 0x%x\r\n", readRegByte(EREVID));
	{
		for (int i=0;i<1000;i++)
			csdelay();
	}

	printf("ECON1: 0x%x\r\n", readRegByte(ECON1));
	printf("ECON2: 0x%x\r\n", readRegByte(ECON2));
	printf("ESTAT: 0x%x\r\n", readRegByte(ESTAT));
	printf("EIR: 0x%x\r\n", readRegByte(EIR));
	printf("EIE: 0x%x\r\n", readRegByte(EIE));
	printf("\r\n");
	printf("MACON1: 0x%x\r\n", readRegByte(MACON1));
	printf("MACON2: 0x%x\r\n", readRegByte(MACON2));
	printf("MACON3: 0x%x\r\n", readRegByte(MACON3));
	printf("MACON4: 0x%x\r\n", readRegByte(MACON4));
	printf("MAADR5: 0x%x\r\n", readRegByte(MAADR5));
	printf("MAADR4: 0x%x\r\n", readRegByte(MAADR4));
	printf("MAADR3: 0x%x\r\n", readRegByte(MAADR3));
	printf("MAADR2: 0x%x\r\n", readRegByte(MAADR2));
	printf("MAADR1: 0x%x\r\n", readRegByte(MAADR1));
	printf("MAADR0: 0x%x\r\n", readRegByte(MAADR0));
	printf("\r\n");
	printf("ERXSTH: 0x%x\r\n", readRegByte(ERXSTH));
	printf("ERXSTL: 0x%x\r\n", readRegByte(ERXSTL));
	printf("ERXNDH: 0x%x\r\n", readRegByte(ERXNDH));
	printf("ERXNDL: 0x%x\r\n", readRegByte(ERXNDL));
	printf("ERXWRPTH: 0x%x\r\n", readRegByte(ERXWRPTH));
	printf("ERXWRPTL: 0x%x\r\n", readRegByte(ERXWRPTL));
	printf("ERXRDPTH: 0x%x\r\n", readRegByte(ERXRDPTH));
	printf("ERXRDPTL: 0x%x\r\n", readRegByte(ERXRDPTL));
	printf("ERXFCON: 0x%x\r\n", readRegByte(ERXFCON));
	printf("EPKTCNT: 0x%x\r\n", readRegByte(EPKTCNT));
	printf("MAMXFLH: 0x%x\r\n", readRegByte(MAMXFLH));
	printf("MAMXFLL: 0x%x\r\n", readRegByte(MAMXFLL));
	printf("\r\n");
	printf("ETXSTH: 0x%x\r\n", readRegByte(ETXSTH));
	printf("ETXSTL: 0x%x\r\n", readRegByte(ETXSTL));
	printf("ETXNDH: 0x%x\r\n", readRegByte(ETXNDH));
	printf("ETXNDL: 0x%x\r\n", readRegByte(ETXNDL));
	printf("MACLCON1: 0x%x\r\n", readRegByte(MACLCON1));
	printf("MACLCON2: 0x%x\r\n", readRegByte(MACLCON2));
	printf("MAPHSUP: 0x%x\r\n", readRegByte(MAPHSUP));
}*/

static long
v2expeth_open (struct netif *nif)
{
	u_int16_t i = 0;
	enc28j60_recv_reset(&initvars, 1);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	/* wait a while for "LINK UP" -> break after 200 ms */
	{
		unsigned char status;
		int runs = 100;

		while( runs-- )
		{
			enc28j60_status( PIO_STATUS_LINK_UP, &status );
    		if( status )
  		  		break;

			for (i=0;i<4000;i++)
				csdelay();
		}
	}

	return 0;
}

static long
v2expeth_close (struct netif *nif)
{
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);    
	return 0;
}

void _cdecl
v2expeth_recv(void)
{
	BUF *buf; 
	short type;
	u_int16_t frame_len = 0;
	struct netif *nif = &if_v2expeth;
	u_int16_t got_size = 0;

	// spi corruption or internal data structure problem?
	while(1)
	{
		/* EIR->PKTIF would be faster but errata sheet says "unreliable" */
		u_int8_t packet_count = readRegByte(EPKTCNT);
		if(packet_count == 0xff || packet_count == 0)
		{
			// 0xff = module not connected, or 0 = no packets
			return;
		}
		/*  read chip's packet header */

		writeReg(ERDPTL, gNextPacketPtr);
		struct {
			u_int8_t nextPacketL;
			u_int8_t nextPacketH;
			u_int8_t byteCountL;
			u_int8_t byteCountH;
			u_int8_t statusL;
			u_int8_t statusH;
		} enc28j60_packet_header;

		readBuf(sizeof enc28j60_packet_header, (u_int8_t*) &enc28j60_packet_header);
		got_size = ((u_int16_t)enc28j60_packet_header.byteCountL + (((u_int16_t)enc28j60_packet_header.byteCountH)<<8)) - 4; // cut off the crc
		frame_len = got_size;
		u_int16_t nextPacketPtr = (u_int16_t)enc28j60_packet_header.nextPacketL + (((u_int16_t)enc28j60_packet_header.nextPacketH)<<8);
		//DDBG("np:0x%x gotsize:%d, framelen: %d", nextPacketPtr, got_size, frame_len);

		if ((enc28j60_packet_header.statusL & 0x80)==0)
		{
			DDBG("receive error np:0x%x, size: %d", gNextPacketPtr, got_size);

			// does the next packet pointer make sense?
			if (nextPacketPtr<RXSTART_INIT && nextPacketPtr>RXSTOP_INIT)
			{
				// nope, lets just reset entirely
				enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);  
				enc28j60_recv_reset(&initvars, 1);
				break;
			}
			else
			{
				// we got an error but the next packet pointer seems sane. Let's try to go with it
				gNextPacketPtr = nextPacketPtr;
				writeReg(ERDPTL, gNextPacketPtr);
				enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
			}
		}

		gNextPacketPtr = nextPacketPtr;

		// so we have a sane header, how about the frame length?
		if (frame_len < 14 || frame_len > 1535)
		{
			// nope, lets reset for this as well
			DWARN("Size error: %d", frame_len);
			nif->in_errors++;
			enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
			enc28j60_recv_reset(&initvars, 1);
			break;
		}

		buf = buf_alloc(frame_len + 200, 100, BUF_ATOMIC);
		if (!buf)
		{
			DWARN("Buf alloc failed");
			nif->in_errors++;
			writeReg(ERDPTL, gNextPacketPtr);
			enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);  
			continue;
		}

		buf->dend = buf->dstart + frame_len;
		/*  read packet */
		readBuf(frame_len, (u_int8_t*)buf->dstart);

		if (nif->bpf)
			bpf_input (nif, buf);

		type = eth_remove_hdr(buf);

		if(!if_input(nif, buf, 0UL, type))
			nif->in_packets++;
		else
			nif->in_errors++;

		writeReg(ERXRDPTL, gNextPacketPtr);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);  
	}
}

static long
v2expeth_output(struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;
	u_int16_t tlen;

	ushort sr = spl7();

	nbuf = eth_build_hdr(buf, nif, hwaddr, pktype);
	if (nbuf == 0)
	{
		DWARN("send nonbuf");
		nif->out_errors++;
		spl(sr);
		return ENOMEM;
	}
	nif->out_packets++;
	
	if (nif->bpf)
		bpf_input (nif, nbuf);

	tlen = (nbuf->dend) - (nbuf->dstart);

	long maxwait = 0x1ffff; /* wait patiently */
	while (readRegByte(ECON1) & ECON1_TXRTS)
	{
		/* errata sheet issue 12 -> TXRTS never becomes 0 */
		/* ignores errata issue 13 -> frame loss due to late collision */
		/* eworks around errata sheet issue 15 -> ESTAT.LATECOL not set */
		if (readRegByte(EIR) & EIR_TXERIF) {
			enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
			enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
		}
		if( !(--maxwait) )
		{
			spl(sr);
			return -3; /* waited long enough: drop frame */
		}
	}

	writeBuffer( (u_int8_t*)nbuf->dstart, tlen, TXSTART_INIT);

	/*  initiate send */
	writeReg(ETXNDL, TXSTART_INIT+tlen);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	buf_deref (nbuf, BUF_NORMAL);
	spl(sr);

	return (0);
}

static long
v2expeth_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;
	struct sockaddr_hw *shw;
	struct enc28j60_initvars *iv = &initvars;

	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;
		case SIOCSIFHWADDR:
			ifr = (struct ifreq*) arg;
			shw = &ifr->ifru.adr.hw;
			memcpy(iv->mac, shw->shw_adr.bytes, ETH_ALEN);
			enc28j60_setMAC(iv->mac);
			memcpy(if_v2expeth.hwlocal.adr.bytes, iv->mac, ETH_ALEN);
			return 0;
		case SIOCSIFMTU:
			// Limit MTU to 1500 bytes. MintNet has already set nif->mtu to the new value, we only limit it here. 
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;
		case SIOCSIFOPT:
			// Interface configuration, handled by v2expeth_config() 
			ifr = (struct ifreq *) arg;
			return v2expeth_config (nif, ifr->ifru.data);
	}

	return ENOSYS;
}

static long
v2expeth_config (struct netif *nif, struct ifopt *ifo)
{
# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))
	
	if (!STRNCMP ("hwaddr"))
	{
		// Set hardware address
		uchar *cp;
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwlocal.adr.bytes;
		UNUSED (cp);
		DDBG("v2expeth: hwaddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	}
	else if (!STRNCMP ("braddr"))
	{
		// Set broadcast address
		uchar *cp;
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
		cp = nif->hwbrcst.adr.bytes;
		UNUSED (cp);
		DDBG("v2expeth: braddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	}
	else if (!STRNCMP ("debug"))
	{
		// turn debuggin on/off 
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DDBG("v2expeth: debug level is %ld", ifo->ifou.v_long);
	}
	else if (!STRNCMP ("log"))
	{
		// set log file 
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DDBG("v2expeth: log file is %s", ifo->ifou.v_string);
	}
	
	return ENOSYS;
}

void v2expeth_int_handler(void)
{
	ushort sr = spl7();
	if (enc28j60_has_recv())
		v2expeth_recv();
	spl(sr);
}

long 
driver_init(void)
{
	unsigned int count;
	u_int8_t mac3val;
	struct enc28j60_initvars *iv = &initvars;
	u_int16_t i;

	DPRINT("v2expeth Eth driver v0.1 (en%d)", if_v2expeth.unit);

	// default mac: 06:80:11:04:04 
	// can be changed using ifconfig hwaddr xx:xx:xx:xx:xx:xx when bringing it up 
	iv->mac[0] = 0x06;
	iv->mac[1] = 0x80;
	iv->mac[2] = 0x11;
	iv->mac[3] = 0x04;
	iv->mac[4] = 0x04;
	iv->mac[5] = 0x05;

	// spi disable
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;

	u_int16_t sdspeed = 10;
	u_int16_t minspeed = 0x0;
	u_int16_t maxspeed = 0x7f;

	if (sdspeed<minspeed)
		sdspeed = 0;

	if (sdspeed>maxspeed)
		sdspeed = maxspeed;

	V2_SDCARD_CFG_SPEED = sdspeed;

	// spi disable again?
	V2_SDCARD_CFG_W = V2_SDCARD_CS+V2_SDCARD_SPEED;

	// fixme set isfull duplex from flags
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	{
		for (i=0;i<40000;i++)
			csdelay();
	}

	// check CLKRDY bit to see if reset is complete
	count = 0;
	while (!enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY)
	{
		count++;
		if (count==0xff)
		{
			DPRINT ("Failed to get clock ready message, no hardware present?\r\n");
			return -1;
		}
	}

	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, ECON1, 0);
 
	/*  return rev */
	rev = readRegByte(EREVID);
	/*  microchip forgot to step the number on the silcon when they */
	/*  released the revision B7. 6 is now rev B7. We still have */
	/*  to see what they do when they release B8. At the moment */
	/*  there is no B8 out yet */
	if ( (rev < 1) || (rev > 10) )
	{
		DPRINT("Invalid revision %d, no hardware present?\r\n", rev);
		return -1;
	}
	if (rev > 5) ++rev;

	writeReg(ETXSTL, TXSTART_INIT);
	writeReg(ETXNDL, TXSTOP_INIT);
  
	/*  set packet filter */
	//enc28j60_broadcast_multicast_filter( flags );/* flags & (PIO_INIT_BROAD_CAST|PIO_INIT_MULTI_CAST) ); */

	/*  BIST pattern generator? */
	//writeReg(EPMM0, 0x303f);
	//writeReg(EPMCSL, 0xf7f9);

	if(is_full_duplex) {
		writeRegByte(MABBIPG, 0x15);    
		writeReg(MAIPGL, 0x0012);
	} else {
		writeRegByte(MABBIPG, 0x12);
		writeReg(MAIPGL, 0x0C12);
	}

	/*  MAC init (with flow control) */
	mac3val = MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN;
	if(is_full_duplex) {
		mac3val |= MACON3_FULDPX;
		iv->macon4=0;
	}
	else
	{
		iv->macon4 = 1<<6;	/* DEFER bit */
	}
	iv->macon3 = mac3val;
	iv->macon2 = 0x00;
	iv->macon1 = MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS;

	/*  set packet pointers */
	enc28j60_recv_reset( iv, 0 );

	/*  PHY init */
	if(is_full_duplex) {
		writePhy(PHCON1, PHCON1_PDPXMD);
		writePhy(PHCON2, 0);
	} else {
		writePhy(PHCON1, 0);
		writePhy(PHCON2, PHCON2_HDLDIS);
	}

	writePhy(PHLCON, 0x3c12 ); /* 3(fixed) C(LinkStat+RX Act) 1(TX Act) 2(Stretch enable) */

	///enc28j60SetBank(EIR);
	// disable all interrupts
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_DMAIF|EIR_LINKIF|EIR_TXIF|EIR_TXERIF|EIR_RXERIF|EIR_PKTIF);
	// global interrupt enable and packet interrupt enable
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE); /* |EIE_LINKIE|EIE_TXIE|EIE_TXERIE|EIE_RXERIE); */

	
	strcpy (if_v2expeth.name, "en");
    if_v2expeth.unit = if_getfreeunit ("en");
    if_v2expeth.metric = 0;
    if_v2expeth.mtu = 1500;
	if_v2expeth.flags = IFF_BROADCAST;
    if_v2expeth.timer = 0;
    if_v2expeth.hwtype = HWTYPE_ETH;
    if_v2expeth.hwlocal.len = ETH_ALEN;
    if_v2expeth.hwbrcst.len = ETH_ALEN;
    if_v2expeth.rcv.maxqlen = IF_MAXQ;
    if_v2expeth.snd.maxqlen = IF_MAXQ;

	// Setup pointers to service functions 
	if_v2expeth.open = v2expeth_open;
	if_v2expeth.close = v2expeth_close;
	if_v2expeth.output = v2expeth_output;
	if_v2expeth.ioctl = v2expeth_ioctl;
	// Optional timer function that is called every 200ms. 
	if_v2expeth.timeout = 0;
	// Here you could attach some more data your driver may need 
	if_v2expeth.data = 0;
	// Number of packets the hardware can receive in fast succession, 0 means unlimited. 
	if_v2expeth.maxpackets = 3;

	memcpy (if_v2expeth.hwlocal.adr.bytes, iv->mac, ETH_ALEN);
	memcpy (if_v2expeth.hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);
	
	if_register (&if_v2expeth);

	// hook into 200hz interrupt 
	u_int32_t* vec = (u_int32_t*)0x114UL;
	s_system (S_GETCOOKIE, COOKIE__5MS, (long) &vec);
	v2expeth_int_old = (void*) *vec;
	*vec = (u_int32_t) v2expeth_int;

	DDBG("int installed: %08x -> %08x (old=%08x)", (u_int32_t)vec, v2expeth_int, v2expeth_int_old);

	return 0;
}
