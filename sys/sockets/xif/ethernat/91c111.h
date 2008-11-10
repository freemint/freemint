/*
 * Ethernat driver for FreeMiNT.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Henrik Gilda
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

#ifndef __91C111_H__
#define __91C111_H__

#define LANREGBASE					0x80000000

#define LANADDR_BANK					0x8000000e

/*Bank 0*/
#define LANADDR_TCR					0x80000000
#define LANADDR_EPH_STATUS			0x80000002
#define LANADDR_RCR					0x80000004
#define LANADDR_COUNTER				0x80000006
#define LANADDR_MIR					0x80000008
#define LANADDR_RPCR					0x8000000A

/*Bank 1*/
#define LANADDR_CONFIG				0x80000000
#define LANADDR_BASE					0x80000002
#define LANADDR_IA01					0x80000004
#define LANADDR_IA23					0x80000006
#define LANADDR_IA45					0x80000008
#define LANADDR_GP					0x8000000A
#define LANADDR_CONTROL				0x8000000C

/*Bank 2*/
#define LANADDR_MMU					0x80000000
#define LANADDR_PNR					0x80000002
#define LANADDR_FIFO					0x80000004
#define LANADDR_POINTER				0x80000006
#define LANADDR_DATA					0x80000008
#define LANADDR_INTERRUPT			0x8000000C

/*Bank 3*/
#define LANADDR_MT01					0x80000000
#define LANADDR_MT23					0x80000002
#define LANADDR_MT45					0x80000004
#define LANADDR_MT67					0x80000006
#define LANADDR_MGMT					0x80000008
#define LANADDR_REVISION			0x8000000A
#define LANADDR_ERCV					0x8000000C


typedef	volatile	unsigned char	VUBYTE;
typedef	volatile	unsigned long	VULONG;
typedef	volatile	unsigned short	VUWORD;

typedef VUBYTE* pVUBYTE;
typedef VULONG* pVULONG;
typedef VUWORD* pVUWORD;


/* MII Management bits */
#define MII_MDO	0x0100
#define MII_MDI	0x0200
#define MII_MCLK	0x0400
#define MII_MDOE	0x0800

/*
	Some functions for manipulating the PHY registers using the MGMT
	register
*/
void	MII_write_reg(short regad, short value);
short MII_read_reg(short regad);
short MII_bit_cycle(short MDO_MDOE_mask);
void	Eth_reset(void);
void Eth_AutoNeg(void);
void	Waitms(long amount);

static inline short Eth_set_bank(short bank)
{
	short tmp;
	
	tmp = ((*(pVUWORD)LANADDR_BANK) >> 8);				//bank nr was in upper byte
	*(pVUWORD)LANADDR_BANK = (bank << 8) | 0x33;
	return tmp;
}

#endif /*__91C111_H_*/
