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

#include <mintbind.h>
#include "91c111.h"

pVUWORD const LANREG_BANK = (pVUWORD) LANADDR_BANK;

pVUWORD const LANREG_TCR = (pVUWORD) LANADDR_TCR;
pVUWORD const LANREG_EPH_STATUS = (pVUWORD) LANADDR_EPH_STATUS;
pVUWORD const LANREG_RCR = (pVUWORD) LANADDR_RCR;
pVUWORD const LANREG_COUNTER = (pVUWORD) LANADDR_COUNTER;
pVUWORD const LANREG_MIR = (pVUWORD) LANADDR_MIR;
pVUWORD const LANREG_RPCR = (pVUWORD) LANADDR_RPCR;

pVUWORD const LANREG_CONFIG = (pVUWORD) LANADDR_CONFIG;
pVUWORD const LANREG_BASE = (pVUWORD) LANADDR_BASE;
pVUWORD const LANREG_IA01 = (pVUWORD) LANADDR_IA01;
pVUWORD const LANREG_IA23 = (pVUWORD) LANADDR_IA23;
pVUWORD const LANREG_IA45 = (pVUWORD) LANADDR_IA45;
pVUWORD const LANREG_GP = (pVUWORD) LANADDR_GP;
pVUWORD const LANREG_CONTROL = (pVUWORD) LANADDR_CONTROL;

pVUWORD const LANREG_MMU = (pVUWORD) LANADDR_MMU;
pVUWORD const LANREG_PNR = (pVUWORD) LANADDR_PNR;
pVUWORD const LANREG_FIFO = (pVUWORD) LANADDR_FIFO;
pVUWORD const LANREG_POINTER = (pVUWORD) LANADDR_POINTER;
pVULONG const LANREG_DATA = (pVULONG) LANADDR_DATA;
pVUWORD const LANREG_INTERRUPT = (pVUWORD) LANADDR_INTERRUPT;

pVUWORD const LANREG_MT01 = (pVUWORD) LANADDR_MT01;
pVUWORD const LANREG_MT23 = (pVUWORD) LANADDR_MT23;
pVUWORD const LANREG_MT45 = (pVUWORD) LANADDR_MT45;
pVUWORD const LANREG_MT67 = (pVUWORD) LANADDR_MT67;
pVUWORD const LANREG_MGMT = (pVUWORD) LANADDR_MGMT;
pVUWORD const LANREG_REVISION = (pVUWORD) LANADDR_REVISION;
pVUWORD const LANREG_ERCV = (pVUWORD) LANADDR_ERCV;

pVULONG const LANREG_TIMER200 = (pVULONG) 0x4ba;


void MII_write_reg(short regad, short value)
{
	short i;
	short	oldbank;

	/* Switch to Bank 3 */
	oldbank = *LANREG_BANK;
	*LANREG_BANK = 0x0300;

	/* First output the Idle-sequence (32 1's) */
	for(i=0;i<32;i++)
	{
		MII_bit_cycle(MII_MDO|MII_MDOE);
	}

	/* Then the 2-bit start code "01" */
	MII_bit_cycle(MII_MDOE);
	MII_bit_cycle(MII_MDO|MII_MDOE);

	/* Then Read flag (=0, no read) */
	MII_bit_cycle(MII_MDOE);

	/* The Write flag (=1, do a write) */
	MII_bit_cycle(MII_MDO|MII_MDOE);

	/* The PHY address, the internal PHY has address "00000" */
	for (i=0;i<5;i++)
		MII_bit_cycle(MII_MDOE);

	/* The PHY register address, 5 bits long */
	for (i=0;i<5;i++) {
		if (regad & 0x10) {
			MII_bit_cycle(MII_MDO|MII_MDOE);	/* Output '1' if bit 4 is '1' */
		}
		else {
			MII_bit_cycle(MII_MDOE);
		}
		regad = (regad << 1);					/* shift left to get next bit next time */
	}

	/* The TA bits ("10") */
	MII_bit_cycle(MII_MDO|MII_MDOE);
	MII_bit_cycle(MII_MDOE);

	/* The Data bits */
	for (i=0;i<16;i++) {
		if (value & 0x8000) {
			MII_bit_cycle(MII_MDO|MII_MDOE);	/* Output '1' if bit 15 is '1' */
		}
		else {
			MII_bit_cycle(MII_MDOE);
		}
		value = (value << 1);					/* shift left to get next bit next time */
	}

	/* Restore the old Bank */
	*LANREG_BANK = oldbank;
}


short MII_read_reg(short regad)
{
	short i, tmp1, tmp2;
	short	oldbank;

	/* Switch to Bank 3 */
	oldbank = *LANREG_BANK;
	*LANREG_BANK = 0x0300;

	/* First output the Idle-sequence (32 1's) */
	for(i=0;i<32;i++)
	{
		MII_bit_cycle(MII_MDO|MII_MDOE);
	}

	/* Then the 2-bit start code "01" */
	MII_bit_cycle(MII_MDOE);
	MII_bit_cycle(MII_MDO|MII_MDOE);

	/* The Read flag (=1, do a read) */
	MII_bit_cycle(MII_MDO|MII_MDOE);

	/* Then Write flag (=0, no write) */
	MII_bit_cycle(MII_MDOE);

	/* The PHY address, the internal PHY has address "00000" */
	for (i=0;i<5;i++)
		MII_bit_cycle(MII_MDOE);

	/* The PHY register address, 5 bits long */
	for (i=0;i<5;i++) {
		if (regad & 0x10) {
			MII_bit_cycle(MII_MDO|MII_MDOE);	/* Output '1' if bit 4 is '1' */
		}
		else {
			MII_bit_cycle(MII_MDOE);
		}
		regad = (regad << 1);					/* shift left to get next bit next time */
	}

	/* The TA bits ("Z0") */
	MII_bit_cycle(0);
	MII_bit_cycle(0);		//ignore the second bit of TA too.

	/* Read the Data bits */
	tmp2 = 0;
	for (i=0;i<16;i++) {
		tmp1 = MII_bit_cycle(0);
		tmp2 = tmp2 << 1;				// Shift in a 0
		if(tmp1 & MII_MDI)
		{
			tmp2 = tmp2 | 0x0001;	//MDI = 1, shift in a '1'
		}
		//else nothing, because or'ing 0x0000 is quite unnecessary
	}

	/* Restore the old Bank */
	*LANREG_BANK = oldbank;


	return (tmp2);
}


short MII_bit_cycle(short MDO_MDOE_mask)
{
	short tmp;


	tmp = *LANREG_MGMT;									// Read before rising edge

	MDO_MDOE_mask = MDO_MDOE_mask & (~MII_MCLK);	// Write data before rising edge
	*LANREG_MGMT = MDO_MDOE_mask;

	asm("nop"::);

	MDO_MDOE_mask = MDO_MDOE_mask | MII_MCLK;		// Rising MCLK edge
	*LANREG_MGMT = MDO_MDOE_mask;

	asm("nop"::);
	asm("nop"::);
	asm("nop"::);

	MDO_MDOE_mask = MDO_MDOE_mask & (~MII_MCLK);	// Falling MCLK edge
	*LANREG_MGMT = MDO_MDOE_mask;

	asm("nop"::);
	asm("nop"::);
	asm("nop"::);

/*
	MDO_MDOE_mask = MDO_MDOE_mask & (~MII_MDI);	// Mask away MDI bit
	tmp = tmp & MII_MDI;									// Keep only MDI bit
	MDO_MDOE_mask = MDO_MDOE_mask | tmp;			// OR in the read MDI bit
*/
	
	return tmp;
}


void Eth_reset(void)
{
	short	banktmp;
	unsigned long	timer200Hz;

	banktmp = *LANREG_BANK; 					// Save old Bank nr
	*LANREG_BANK = 0x0000;						// Switch to bank 0

	/* Reset the LAN chip (Soft reset), and clear the bit again */
	*LANREG_RCR = 0x0080;
	*LANREG_RCR = 0x0000;

	/* Wait for 50 ms */
	timer200Hz = *LANREG_TIMER200;
	while (*LANREG_TIMER200 < (timer200Hz + 12));	/* 10 ticks = 50 ms */

	*LANREG_BANK = banktmp;
}


void Eth_AutoNeg(void)
{
	unsigned long	timer200Hz;
	short banktmp;


	banktmp = *LANREG_BANK; 					// Save old Bank nr
	*LANREG_BANK = 0x0000;						// Switch to bank 0

	/* Reset the LAN chip (Soft reset), and clear the bit again */
	*LANREG_RCR = 0x0080;
	*LANREG_RCR = 0x0000;

	/* Wait for 50 ms */
	timer200Hz = *LANREG_TIMER200;
	while (*LANREG_TIMER200 < (timer200Hz + 12));	/* 10 ticks = 50 ms */

	/* Enable Auto-negotiation mode in the RPCR register */
	*LANREG_RPCR = 0x0008;

	/* Reset the PHY */
	MII_write_reg(0,0x8000);

	/* Wait for another 50 ms */
	timer200Hz = *LANREG_TIMER200;
	while (*LANREG_TIMER200 < (timer200Hz + 12));	/* 10 ticks = 50 ms */

	/* Enable ANEG_EN and disable MII_DIS and enable ANEG_RST */
	MII_write_reg(0,0x3200);

	*LANREG_BANK = banktmp;
}


/*
 *	Waits for a specified number of ms, rounded down to nearest 5ms,
 *	because this function uses the 200Hz system timer
 */
void Waitms(long amount)
{
	long	timer200Hz;

	amount = amount / 5;
	timer200Hz = *LANREG_TIMER200;
	while (*LANREG_TIMER200 < (timer200Hz + amount));

}
