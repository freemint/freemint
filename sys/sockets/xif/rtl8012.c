/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * Copyright 2000 Vassilis Papathanassiou
 * All rights reserved.
 * 
 * Driver for Realtek's RTL8012 pocket lan adaptor connected to the cartridge
 * port via my hardware (adapter hardware description, schematics, links
 * etc can be found at http://users.otenet.gr/~papval).
 * 
 * Based on DE600 sources and skeleton by Kay Roemer.
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
 * 
 * 
 * begin:	2000-07-24
 * last change:	2000-07-24
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "global.h"
# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "rtl8012.h"
# include "rtl8012_vblint.h"
# include "netinfo.h"

# include "arch/timer.h"
# include "mint/asm.h"
# include "mint/delay.h"
# include "mint/mdelay.h"
# include "mint/sockio.h"
# include "mint/ssystem.h"
# include "cookie.h"

# include <mint/osbind.h>


# ifdef NO_DELAY
# define udelay(x)	/* on 68000 we don't need to delay */
# define mdelay(x)	/* on 68000 we don't need to delay */
# endif

# define wait5ms	mdelay (5)


/* there must be a better place for that */
volatile long *hz_200 = _hz_200;

# undef INLINE
# define INLINE static


/*
 * version
 */

# define VER_MAJOR	1
# define VER_MINOR	10
# define VER_STATUS	


/*
 * debugging stuff
 */

# if 1
# define XIF_DEBUG
# endif


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p RTL8012 ethernet romport driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 2000 by Vassilis Papathanassiou.\r\n" \
	"\275 2000-2010 by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_CPU_000	\
	"\033pThis driver work only on 68000!\033q\r\n"

# define MSG_CPU_020	\
	"\033pThis driver require at least a 68020!\033q\r\n"

# define MSG_PROBE	\
	"\033pNo Adapter found!\033q\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, driver NOT installed - initialization failed!\r\n\r\n"


INLINE uchar	RdNib			(ushort rreg);
INLINE void	WrNib			(uchar wreg, uchar wdata);
INLINE void	WrByte			(uchar wreg, uchar wdata);
INLINE uchar	RdBytEP			(void);

static void	rtl8012_stop		(void);
static void	rtl8012_active		(void);
static void	rtl8012_doreset		(void);
static int	rtl8012_ramtest		(void);
static void	rtl8012_sethw		(void);
static long	rtl8012_probe		(struct netif *);
static long	rtl8012_reset		(struct netif *);

static long _cdecl rtl8012_open		(struct netif *);
static long _cdecl rtl8012_close	(struct netif *);
static long _cdecl rtl8012_output	(struct netif *, BUF *, const char *, short, short);
static long _cdecl rtl8012_ioctl	(struct netif *, short, long);

static void rtl8012_install_int		(void);
INLINE void rtl8012_recv_packet		(struct netif *);

void rtl8012_int (void);


/* hardware NIC address */
static uchar address[6] = { 0x00, 0x00, 0xe8, 0xe5, 0xb6, 0xc2 };

/* Avoid input from interrupt while outputing */
static volatile int in_use = 0;

/* fast conversion table for byte writing */
static ushort Table [256];

INLINE void
NIC_make_table (void)
{
	int i;
	
	for (i = 0; i < 256; i++)
	{
		int x = i;
		
		if (x & 1)
			x |= 0x100;
		
		Table[i] = x & ~1;
	}
}

/* 
 * general register read
 * - 'End Of Read' command is not issued
 */	
INLINE uchar
RdNib (ushort rreg)
{
	register uchar c;
	
	*(volatile uchar *) (0xfa0000 + Table[EOC+rreg]);
	*(volatile uchar *) (0xfa0000 + Table[RdAddr+rreg]);
	
	/* Read nib */
# if 1
	//*(volatile ushort *) 0xfa0000;
	c = *(volatile ushort *) 0xfa0000;
# else
	*(volatile uchar *) 0xfa0001;
	c = *(volatile uchar *) 0xfa0001;
# endif
	
	*(volatile uchar *) (0xfa0000 + Table[EOC+rreg]);
	
	return c & 0x0f;
}

/* 
 * general register write
 * - 'End Of Write' is issued
 */
INLINE void
WrNib (uchar reg, uchar value)
{
	register ulong x;
	
	/* End Of Write */
	*(volatile uchar *) (0xfa0000 + Table[EOC|reg]);
	
	
	x = WrAddr | reg;
	
	/* Prepare address */
	*(volatile uchar *) (0xfa0000 + Table[x]);
	*(volatile uchar *) (0xfa0000 + Table[x]);
	
	x &= 0xf0;
	x |= value;
	
	/* Write address */
	*(volatile uchar *) (0xfa0000 + Table[x]);
	
	x = value + 0x80;
	
	/* Write data */
	*(volatile uchar *) (0xfa0000 + Table[x]);
	*(volatile uchar *) (0xfa0000 + Table[x]);
	
	
	/* End Of Write */
	*(volatile uchar *) (0xfa0000 + Table[EOC|x]); 
}

INLINE void
WrByte (uchar wreg, uchar wdata)
{
	WrNib (wreg, wdata & 0x0f);
	WrNib (wreg+HNib, HNib+((wdata >> 4) & 0x0f));
}

INLINE uchar
RdBytEP (void)
{
	register uchar x;
	register uchar c;
	
	/* get lo nib */
	//x = *(volatile ushort *) 0xfa0000;
	x = *(volatile uchar *) 0xfa0001;
	
	c = x & 0x0f;
	
	/* CLK down */
	*(volatile uchar *) 0xfb0000;
	
	//nop
	
	/* get hi nib */
	//x = *(volatile ushort *) 0xfa0000;
	x = *(volatile uchar *) 0xfa0001;
	
	c |= (x << 4) & 0xf0;
	
	//nop
	
	/* CLK up */
	*(volatile uchar *) (0xfb0000 + 0x2000);
	
	return c;
}

/* DMACSB down, INITB,CLK up */
# define CSB_DN_CLK_UP	({ *(volatile uchar *) (0xfb0000 + 0x2000); udelay (1); })
//# define CSB_DN_CLK_UP	*(volatile uchar *) (0xfb0000 + 0x2000)

/* DMACSB,INITB,CLK up ie P14,P16,P17 up */
# define P14_16_17_UP	({ *(volatile uchar *) (0xfb0000 + 0xe000); udelay (1); })
//# define P14_16_17_UP	*(volatile uchar *) (0xfb0000 + 0xe000)


/*
 * stop the adapter from receiving packets
 */
static void
rtl8012_stop (void)
{
	/* Set 'No Acception' */
	WrNib (CMR2+HNib, HNib);
	
	/*  disable Tx & Rx */
	WrNib (CMR1+HNib, HNib);
}

static void
rtl8012_active (void)
{
	/* acception mode broadcast and physical */
	WrNib (CMR2+HNib, HNib+RxMode);
	
	/* enable Rx & Tx */
	WrNib (CMR1+HNib, HNib+EPLC_RxE+EPLC_TxE);
}

static void
rtl8012_doreset (void)
{
	ushort sr = splhigh ();
	uchar i;
	
	DEBUG (("rtl8012: reset"));
	
	P14_16_17_UP;
	
	WrNib (MODSEL, 0);
	WrNib (MODSEL+HNib, HNib+2);
	
	udelay (10);
	
	i = RdNib (MODSEL+HNib);
	DEBUG (("rtl8012: reset -> 1: %i [expected 2]", i));
	i = RdNib (MODSEL+HNib);
	DEBUG (("rtl8012: reset -> 1: %i [expected 2]", i));
	i = RdNib (MODSEL+HNib);
	DEBUG (("rtl8012: reset -> 1: %i [expected 2]", i));
	//if (i != 2)
	//	return;
	
	WrNib (CMR1+HNib, HNib+EPLC_RST);
	
	do {
		udelay (1);
		i = RdNib (CMR1+HNib);
	}
	while (i & 0x4);
	
	/* Clear ISR */
	WrNib (ISR, EPLC_ROK+EPLC_TER+EPLC_TOK);
	WrNib (ISR+HNib, HNib+EPLC_RBER);
	
	//udelay (10);
	
	i = RdNib (CMR2+HNib);		/* must be 2 (EPLC_AM1) */
	DEBUG (("rtl8012: reset -> 2: %i [expected 2]", i));
	i = RdNib (CMR2+HNib);		/* must be 2 (EPLC_AM1) */
	DEBUG (("rtl8012: reset -> 2: %i [expected 2]", i));
	i = RdNib (CMR2+HNib);		/* must be 2 (EPLC_AM1) */
	DEBUG (("rtl8012: reset -> 2: %i [expected 2]", i));
	
# ifdef XIF_DEBUG
	i = RdNib (CMR1);
	i |= (RdNib (CMR1+HNib) << 4);
	DEBUG (("rtl8012: reset CMR1 0x%x", i));
	
	i = RdNib (CMR2);
	i |= (RdNib (CMR2+HNib) << 4);
	DEBUG (("rtl8012: reset CMR2 0x%x", i));
# endif
	
	spl (sr);
}

static int
rtl8012_ramtest (void)
{
# define SIZE 256
	ushort test[SIZE];
	ushort verify[SIZE];
	
	int i;
	int r;
	
	DEBUG (("rtl8012: ramtest"));
	
	for (i = 0; i < SIZE; i++)
		test[i] = i | (i << 8);
	
	P14_16_17_UP;
	
	/* Mode A */
	WrNib (MODSEL, 0);
	
	/* 0-450ns */
	WrNib (MODDEL, 0);
	
	/* 1.disable Tx & Rx */
	WrNib (CMR1+HNib, HNib);
	
	/* 2.enter RAM test mode */
	WrNib (CMR2, EPLC_RAMTST);
	
	/* 3.disable Tx & Rx */
	WrNib (CMR1+HNib, HNib);
	
	/* 4.enable Tx & Rx */
	WrNib (CMR1+HNib, HNib+EPLC_RxE+EPLC_TxE);
	
	//udelay (10);
	
	i = RdNib (CMR1+HNib);
	DEBUG (("rtl8012: ramtest -> 1: %i", i));
	i = RdNib (CMR1+HNib);
	DEBUG (("rtl8012: ramtest -> 1: %i", i));
	i = RdNib (CMR1+HNib);
	DEBUG (("rtl8012: ramtest -> 1: %i", i));
	
	/* Mode C to Write test pattern */
	WrNib (MODSEL+HNib, HNib+3);
	
	CSB_DN_CLK_UP;
	
	for (i = 0; i < SIZE; i++)
	{
		register ushort word = test[i];
		
		/* 1st byte */
		*(volatile uchar *) (0xfa0000 + Table[((word >> 8) & 0xff)]);
		
		/* CLK down */
		*(volatile uchar *) (0xfb0000);
		
		/* 2nd byte */
		*(volatile uchar *) (0xfa0000 + Table[(word & 0xff)]);
		
		/* CLK up */
		*(volatile uchar *) (0xfb0000 + 0x2000);
	}
	
	P14_16_17_UP;
	
	WrNib (CMR1, WRPAC);
	
	/* Mode A */
	WrNib (MODSEL, 0);
	
	/* 6.disable Tx & Rx */
	WrNib (CMR1+HNib, HNib);
	
	/* 7.enable Tx & Rx */
	WrNib (CMR1+HNib, HNib+EPLC_RxE+EPLC_TxE);
	
	i = RdNib (CMR1+HNib);
	WrNib (CMR1, RDPAC);
	
	CSB_DN_CLK_UP;
	
	/* Mode C to Read test pattern */
	WrNib (MODSEL+HNib, HNib+1);
	
	for (i = 0; i < SIZE; i++)
	{
		register ushort x;
		
		x = RdBytEP ();
		x <<= 8;
		x |= RdBytEP ();
		
		verify[i] = x;
	}
	
	//nop
	//nop
	
	P14_16_17_UP;
	
	/* 9.disable Tx & Rx before leaving RAM test mode */
	WrNib (CMR1+HNib, HNib);
	
	/* 10.leave RAM test mode */
	WrNib (CMR2, EPLC_CMR2Null);
	
# if 0
	/* Accept physical and broadcast */
	WrNib (CMR2+HNib, HNib+RxMode);
	
	/* Enable Rx TEST ONLY!!! */
	WrNib (CMR1+HNib, HNib+EPLC_RxE+EPLC_TxE);
# endif
	
	r = 0;
	for (i = 0; i < SIZE; i++)
	{
		if (test[i] != verify[i])
		{
			DEBUG (("rtl8012: ramtest -> %i [%i, %i]", i, test[i], verify[i]));
			r = 1;
		}
	}
	
	return r;
}

# if 0
;-------------------------------------------------------------------------
;	To read Ethernet node id and store in buffer 'ID_addr'
;	Assume : 9346 is used
;
;	entry : nothing!
;
;	exit  : Hopefuly the 6 bytes MAC in the buffer :-)
;
;	Note: If ID= 12 34 56 78 9A BC,  then the read bit sequence will be
;		 0011-0100 0001-0010 0111-1000 0101-0110 .......
;		 ( 3 - 4     1 - 2     7 - 8	 5 - 6	 ..... )
;
;-------------------------------------------------------------------------
# endif
static void GetNodeID (uchar *buf);

static void
GetNodeID (uchar *buf)
{
	ushort d0, d4, d5, d6, d7;
	ushort _d4, _d6;
	
	WrNib (CMR2, EPLC_PAGE);	/* point to page 1 */
					/* issue 3 read command to */
					/* read 6 bytes id */
	d6 = 0;				
	d4 = 0x100 | 0x80;		/* start bit | READ command */
	
read_next_id_word:
	_d4 = d4;
	_d6 = d6;
	
	d0 = RdNib (PDR);
	DEBUG (("GetNodeID [1]: d4 = %x, d6= %x (d0 = %x)", d4, d6, d0));
	
	//WrNib (PCMR+HNib, 0x2);
	
	d0 = 0x100;			/* mask */
	for (d6 = 0; d6 < 9; d6++)
	{
		/* 
		 * DI = Data Input of 9346
		 * 
		 * 	  ________________
		 * CS : __|
		 * 	      ___     ___
		 * SK : ______|	 |___|	 |
		 * 	  _______ _______
		 * DI :	 X_______X_______X
		 */
		
		//d5 = %00010110;	/* d5 = 000 HNIB  0 SKB CS 1 */
		d5 = 0x4 | 0x2;		/* d5 = 000 HNIB  0 SKB CS 1 */
		
		//moveq		#0,d0
		//roxl.w	#1,d4
		//addx		d0,d5
		if (d4 & d0)
			d5 |= 0x1;	/* put DI bit to lsb of d5 */
		
		d0 >>= 1;
		
		WrNib (PCMR+HNib, d5);	/* pulls SK low and outputs DI */
		udelay (1);
		DEBUG (("1: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		DEBUG (("1: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		DEBUG (("1: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		wait5ms;		/* wait about 5ms (long but...) */
		//d5 &= ~0x4;		/* let SKB=0 */
		WrNib (PCMR+HNib, (d5 & ~0x4));	/* Pulls SK high to end this cycle */
		udelay (1);
		DEBUG (("2: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		DEBUG (("2: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		DEBUG (("2: PCMR+HNib = %x", RdNib (PCMR+HNib)));
		//wait5ms;
		
		//DEBUG (("GetNodeID [2]: d6 = %i, d5 = %x, d0 = %x", d6, d5, d0));
	}
	
	wait5ms;
	goto read_bit;
	
read_bit:
	d5 = 0;
	
	for (d7 = 0; d7 < 17; d7++)
	{
		/* read 17-bit data,
		 * order : 0, D15, D14, --- ,D0
		 * the 1st bit is the dummy
		 * bit '0', and is discarded.
		 */
		
		WrNib (PCMR+HNib, 0x4 | 0x2);	/* pull SK low */
		wait5ms;
		WrNib (PCMR+HNib, 0x2);		/* pull SK high to latch DO */
		//wait5ms;
		
		d0 = RdNib (PDR);
		//d0 = RdNib (PDR);
		//d0 = RdNib (PDR);
		
		//nop
		//nop
		
		//lsl.l	#1,d5
		d5 <<= 1;
		
		if (d0 & 0x1)
			d5++;
		
		*(volatile uchar *) 0xfa007f;	/* EORead reg */
		//wait5ms;
		
		//DEBUG (("GetNodeID [3]: d7 = %i, d0 = %x", d7, d0));
	}
	
	DEBUG (("GetNodeID [4]: d5 = %x", d5));
	goto read_bit_return;
	
read_bit_return:
	d0 = d5;
	
	*buf++ = d0;
	
	//asr.w	#8,d5
	d5 >>= 8;
	
	*buf++ = d5;
	
	//nop
	//nop
	
	//WrNib (PCMR+HNib, %00010100);	/* 000 HNib 0 SKB CS 0 */
	WrNib (PCMR+HNib, 0x4);		/* 000 HNib 0 SKB CS 0 */
					/* let CS low to end this instruction */
	
	/* 5ms for inter-instruction gap */
	wait5ms;
	
	d4 = _d4;
	d6 = _d6;
	
	d4 += 1;
	
	d6++;
	if (d6 < 3)
		goto read_next_id_word;
	
	WrNib (CMR2, EPLC_CMR2Null);	/* back to page 0 */
	
	return;
}

/*
 * Set an Ethernet Address to IDR0-5 registers and mask 0xFFFFFFFF multicast
 */
static void
rtl8012_sethw (void)
{
	WrByte (IDR0, address[0]);
	WrByte (IDR1, address[1]);
	WrByte (IDR2, address[2]);
	WrByte (IDR3, address[3]);
	WrByte (IDR4, address[4]);
	WrByte (IDR5, address[5]);
	
# ifdef XIF_DEBUG
	{
		uchar c0, c1, c2, c3, c4, c5;
		
		c0 = RdNib (IDR0);
		c0 |= (RdNib (IDR0+HNib) << 4);
		
		c1 = RdNib (IDR1);
		c1 |= (RdNib (IDR1+HNib) << 4);
		
		c2 = RdNib (IDR2);
		c2 |= (RdNib (IDR2+HNib) << 4);
		
		c3 = RdNib (IDR3);
		c3 |= (RdNib (IDR3+HNib) << 4);
		
		c4 = RdNib (IDR4);
		c4 |= (RdNib (IDR4+HNib) << 4);
		
		c5 = RdNib (IDR5);
		c5 |= (RdNib (IDR5+HNib) << 4);
		
		DEBUG (("expected: %2x:%2x:%2x:%2x:%2x:%2x", address[0], address[1], address[2], address[3], address[4], address[5]));
		DEBUG (("readen:   %2x:%2x:%2x:%2x:%2x:%2x", c0, c1, c2, c3, c4, c5));
	}
# endif
	
	/* Now set NIC to accept broadcast */
	
	WrNib (CMR2, EPLC_PAGE);	/* point to page 1 */
	
	WrByte (MAR0, 0xff);
	WrByte (MAR1, 0xff);
	WrByte (MAR2, 0xff);
	WrByte (MAR3, 0xff);
	WrByte (MAR4, 0xff);
	WrByte (MAR5, 0xff);
	WrByte (MAR6, 0xff);
	WrByte (MAR7, 0xff);
	
# ifdef XIF_DEBUG
	{
		uchar c0, c1, c2, c3, c4, c5, c6, c7;
		
		c0 = RdNib (MAR0);
		c0 |= (RdNib (MAR0+HNib) << 4);
		
		c1 = RdNib (MAR1);
		c1 |= (RdNib (MAR1+HNib) << 4);
		
		c2 = RdNib (MAR2);
		c2 |= (RdNib (MAR2+HNib) << 4);
		
		c3 = RdNib (MAR3);
		c3 |= (RdNib (MAR3+HNib) << 4);
		
		c4 = RdNib (MAR4);
		c4 |= (RdNib (MAR4+HNib) << 4);
		
		c5 = RdNib (MAR5);
		c5 |= (RdNib (MAR5+HNib) << 4);
		
		c6 = RdNib (MAR6);
		c6 |= (RdNib (MAR6+HNib) << 4);
		
		c7 = RdNib (MAR7);
		c7 |= (RdNib (MAR7+HNib) << 4);
		
		DEBUG (("broad:    %2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x", c0, c1, c2, c3, c4, c5, c6, c7));
	}
# endif
	
	WrNib (CMR2, EPLC_CMR2Null);	/* back to page 0 */
	
	WrNib (IMR, 0);			/* EPLC_ROK+EPLC_TER+EPLC_TOK */
}

static long
rtl8012_probe (struct netif *nif)
{
	uchar test[6];
	int i;
	
	/* reset adapter */
	rtl8012_doreset ();
	
	wait5ms;
	
	/* reset adapter */
	rtl8012_doreset ();
	
	wait5ms;
	
	/* now run the memory test */
	if (rtl8012_ramtest ())
		return -1;
	
	wait5ms;
	
	/* reset adapter */
	rtl8012_doreset ();
	
	wait5ms;
	
	RdNib (CMR1);
	
	GetNodeID (test);
	DEBUG (("rtl8012_probe: MAC: %2x:%2x:%2x:%2x:%2x:%2x", test[0], test[1], test[2], test[3], test[4], test[5]));
	
	/*
	 * Try to read magic code and last 3 bytes of hw
	 * address (Not possible with RTL8012 for the moment)
	 */
	for (i = 0; i < ETH_ALEN; i++)
	{
		nif->hwlocal.adr.bytes[i] = address[i];
		nif->hwbrcst.adr.bytes[i] = 0xff;
	}
	
	return 0;
}

static long
rtl8012_reset (struct netif *nif)
{
	uchar test[6];
	
	/* block out interrupt */
	in_use = 1;
	
	/* reset adapter */
	rtl8012_doreset ();
	
	GetNodeID (test);
	DEBUG (("rtl8012_reset: MAC: %2x:%2x:%2x:%2x:%2x:%2x", test[0], test[1], test[2], test[3], test[4], test[5]));
	
	/* set hw address */
	rtl8012_sethw ();
	
	/* enable Tx and Rx */
	rtl8012_active ();
	
	/* allow interrupt */
	in_use = 0;
	
	return 0;
}

static long _cdecl
rtl8012_open (struct netif *nif)
{
	rtl8012_reset (nif);
	return 0;
}

static long _cdecl
rtl8012_close (struct netif *nif)
{
	/* block out interrupt */
	in_use = 1;
	
	rtl8012_stop ();
	
	/* allow interrupt */
	in_use = 0;
	
	return 0;
}

INLINE void
send_block (uchar *buf, register ushort len)
{
	register int flag;
	register int i;
	
	WrNib (MODSEL, 0);		/* Mode A */
	WrNib (MODSEL+HNib, HNib+3);	/* Mode C to Write packet */
	
	/* start data transfer */
	CSB_DN_CLK_UP;
	
	if (len & 1)
	{
		flag = 1;
		len--;
	}
	else
		flag = 0;
	
	i = 0;
	while (i < len)
	{
		/* 1st byte */
		*(volatile uchar *) (0xfa0000 + Table[buf[i++]]);
		
		/* CLK down */
		*(volatile uchar *) (0xfb0000);
		
		/* 2nd byte */
		*(volatile uchar *) (0xfa0000 + Table[buf[i++]]);
		
		/* CLK up */
		*(volatile uchar *) (0xfb0000 + 0x2000);
	}
	
	if (flag)
	{
		*(volatile uchar *) (0xfa0000 + Table[buf[i]]);
		
		/* CLK down */
		*(volatile uchar *) (0xfb0000);
	}
	
	udelay (1);
	
	/* end of data transfer */
	P14_16_17_UP;
	
	/* Tx jump packet command */
	WrNib (CMR1, WRPAC);
	
	udelay (1);
}

/* transmitter pages */
# define TX_PAGES		2

/*
 * Some macros for managing the output queue.
 */
# define Q_EMPTY(p)		((p)->tx_used <= 0)
# define Q_FULL(p)		((p)->tx_used >= TX_PAGES)
# define Q_DEL(p)		{ (p)->tx_used--; (p)->tx_tail ^= 1; }
# define Q_ADD(p)		((p)->tx_used++)

struct rtl12
{
	ushort	tx_tail;	/* queue tail */
	ushort	tx_used;	/* no of used queue entries */
};

static long _cdecl
rtl8012_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	struct rtl12 *pr = nif->data;
	ushort len;
	BUF *nbuf;
	
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if (!nbuf)
	{
		DEBUG (("rtl8012: eth_build_hdr failed, out of memory!"));
		
		nif->out_errors++;
		return ENOMEM;
	}
	
	/* pass to upper layer */
	if (nif->bpf)
		bpf_input (nif, nbuf);
	
	if (Q_FULL (pr))
	{
		DEBUG (("rtl8012: output queue full!"));
		
		buf_deref (nbuf, BUF_NORMAL);
		return ENOMEM;
	}
	
	len = nbuf->dend - nbuf->dstart;
	
# ifdef XIF_DEBUG
	if (in_use)
		ALERT (("rtl8012: in_use already set!!!"));
# endif
	
	/* block out interrupt */
	in_use = 1;
	
	/*
	 * Store packet in 1rst or 2nd buffer
	 * It is done automaticaly for rtl80xx
	 */
	send_block ((unsigned char *)nbuf->dstart, len);
	
	if (!Q_EMPTY (pr))
	{
		long ticks = jiffies + 2;	/* normal is 1 */
		do {
			uchar stat;
			
			stat = RdNib (ISR);
			//stat |= (RdNib (ISR+HNib) << 4);
			
			/* Clear ISR */
			WrNib (ISR, EPLC_ROK+EPLC_TER+EPLC_TOK);
			WrNib (ISR+HNib, HNib+EPLC_RBER);
			
# define RX_GOOD	EPLC_ROK
# define TX_FAILED16	EPLC_TER
# define TX_OK		EPLC_TOK
			
			if (stat & TX_OK)
			{
				DEBUG (("rtl8012: TX_OK ()!"));
				Q_DEL (pr);
			}
			else if (stat == TX_FAILED16)
			{
				DEBUG (("rtl8012: TX_FAILED16!"));
				nif->collisions++;
				ticks = jiffies + 1;
			}
		}
		while (!Q_EMPTY (pr) && jiffies - ticks <= 0);
		
		/*
		 * transmission timed out
		 */
		if (!Q_EMPTY (pr))
		{
			DEBUG (("rtl8012: timeout, check network cable!"));
			Q_DEL (pr);
			
			in_use = 0;
			
			buf_deref (nbuf, BUF_NORMAL);
			rtl8012_reset (nif);
			
			nif->out_errors++;
			return ETIMEDOUT;
		}
	}
	
	Q_ADD (pr);
	
	/* Enet packets must have at least 60 bytes */
	len = MAX (len, 60);
	
	WrByte (TBCR0, len & 0xff);
	WrNib (TBCR1, len >> 8);
	WrNib (CMR1, EPLC_TRA);		/* start transmission */
	
	in_use = 0;
	
	buf_deref (nbuf, BUF_NORMAL);
	
	nif->out_packets++;
	return 0;
}

static long _cdecl
rtl8012_ioctl (struct netif *nif, short cmd, long arg)
{
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
	}
	
	return ENOSYS;
}

static struct rtl12 rtl8012_priv =
{
	tx_tail:	0,
	tx_used:	0
};

static struct netif if_RTL12 =
{
	name:		"en",
	unit:		0,
	
	flags:		IFF_BROADCAST,
	metric:		0,
	mtu:		1500,
	timer:		0,
	hwtype:		HWTYPE_ETH,
	
	hwlocal:	{ len: ETH_ALEN },
	hwbrcst:	{ len: ETH_ALEN },
	
	snd:		{ maxqlen: IF_MAXQ },
	rcv:		{ maxqlen: IF_MAXQ },
	
	open:		rtl8012_open,
	close:		rtl8012_close,
	output:		rtl8012_output,
	ioctl:		rtl8012_ioctl,
	timeout:	NULL,
	
	data:		&rtl8012_priv,
	
	maxpackets:	1			/* can receive max. 1 packet */
};
long driver_init(void);
long
driver_init (void)
{
	char message[128];
	long cpu;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	
	if ((MINT_MAJOR == 0)
		|| ((MINT_MAJOR == 1) && ((MINT_MINOR < 15) || (MINT_KVERSION < 2))))
	{
		c_conws (MSG_MINT);
		goto failure;
	}
	
	if ((s_system (S_GETCOOKIE, COOKIE__CPU, (long) &cpu) != 0)
# ifdef __M68020__
		|| (cpu < 20))
	{
		c_conws (MSG_CPU_020);
		goto failure;
	}
# else
		|| (cpu > 0))
	{
		c_conws (MSG_CPU_000);
		goto failure;
	}
# endif
	
	/* build fast conversion table */
	NIC_make_table ();
	
	/* allocate unit number */
	if_RTL12.unit = if_getfreeunit ("en");
	
	if (rtl8012_probe (&if_RTL12) < 0)
	{
		c_conws (MSG_PROBE);
		goto failure;
	}
	
	rtl8012_install_int ();
	
	/* register in upper layer */
	if_register (&if_RTL12);
	
	ksprintf (message, "interface: en%d (%x:%x:%x:%x:%x:%x)\r\n\r\n",
		if_RTL12.unit,
		  if_RTL12.hwlocal.adr.bytes[0],
		  if_RTL12.hwlocal.adr.bytes[1],
		  if_RTL12.hwlocal.adr.bytes[2],
		  if_RTL12.hwlocal.adr.bytes[3],
		  if_RTL12.hwlocal.adr.bytes[4],
		  if_RTL12.hwlocal.adr.bytes[5]);
	
	c_conws (message);
	return 0;
	
failure:
	c_conws (MSG_FAILURE);
	return 1;
}

static void
rtl8012_install_int (void)
{
# define vector(x)	(x / 4)
	old_vbl_int = Setexc (vector (0x70), (long) vbl_interrupt);
}

INLINE ushort
get_paclen (void)
{
	uchar RxBuffer [8];
	register ushort i;
	
	//i = RdNib (CMR1+HNib);
	//i = (RdNib (CMR1+HNib) << 4) | RdNib (CMR1);
	
	/* Rx jump packet command */
	WrNib (CMR1, RDPAC);
	
	udelay (1);
	
	/* start data transfer */
	CSB_DN_CLK_UP;
	
	/* Mode C to Read from adapter buffer */
	WrNib (MODSEL+HNib, HNib+1);
	
	/* readin header */
	for (i = 0; i < 8; i++)
		RxBuffer[i] = RdBytEP ();
	
	/* size of packet */
	i = RxBuffer[RxMSB] << 8;
	i |= RxBuffer[RxLSB];
	
	if (RxBuffer[RxStatus] == 0)
	{
		P14_16_17_UP;
		return 0;
	}
	
	if (i < 8)
	{
		P14_16_17_UP;
		
		/* Accept all packets */
		//WrNib (CMR2+HNib, HNib+RxMode);
		
		return 0;
	}
	
	if (i >= 0x800)
		/* A full ethernet packet plus CRC */
		i = 1536 + 4;
	
	/* 4 CRC + 1 for dbf */
	i -= 4;
	
	return i;
}

INLINE void
read_block (uchar *buf, ushort len)
{
	register int i;
	
	for (i = 0; i < len; i++)
		*buf++ = RdBytEP ();
	
	udelay (1);
	
	P14_16_17_UP;
}

/*
 * Read a packet out of the adapter and pass it to the upper layers
 */
INLINE void
rtl8012_recv_packet (struct netif *nif)
{
	ushort pktlen;
	BUF *b;
	
	/* read packet length (excluding 32 bit crc) */
	pktlen = get_paclen ();
	
	DEBUG (("rtl8012_recv_packet: %i", pktlen));
	
	//if (pktlen < 32)
	if (!pktlen)
	{
		nif->in_errors++;
	//	if (pktlen > 10000)
	//		rtl8012_reset (nif);
		return;
	}
	
	b = buf_alloc (pktlen+100, 50, BUF_ATOMIC);
	if (!b)
	{
		DEBUG (("rtl8012_recv_packet: out of mem (buf_alloc failed)"));
		nif->in_errors++;
		return;
	}
	b->dend += pktlen;
	
	read_block ((unsigned char *)b->dstart, pktlen);
	
	/* Pass packet to upper layers */
	if (nif->bpf)
		bpf_input (nif, b);
	
	/* and enqueue packet */
	if (!if_input (nif, b, 0, eth_remove_hdr (b)))
		nif->in_packets++;
	else
		nif->in_errors++;
}

INLINE int
buf_empty (void)
{
	return (RdNib (CMR1) & 0x1);
}

/*
 * Busy (vbl for now) interrupt routine
 */
void _cdecl
rtl8012_int (void)
{
	if (in_use)
		return;
	
	in_use = 1;
	
	while (!buf_empty ())
		rtl8012_recv_packet (&if_RTL12);
	
	in_use = 0;
}
