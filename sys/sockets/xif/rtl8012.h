/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * begin:	2000-08-02
 * last change:	2000-08-02
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _rtl8012_h
# define _rtl8012_h


/*
 *    8 Data Modes are provided
 *
 *	+--------+---------------+-------------+
 *	|  Mode  |     Read	 |    Write    |
 *	+--------+---------------+-------------+
 *	|   0	 |   LptCtrl	 |   LptData   |
 *	+--------+---------------+-------------+
 *	|   1	 |   LptCtrl	 |   LptCtrl   |
 *	+--------+---------------+-------------+
 *	|   2	 |   LptCtrl*2	 |   LptData   |
 *	+--------+---------------+-------------+
 *	|   3	 |   LptCtrl*2	 |   LptCtrl   |
 *	+--------+---------------+-------------+
 *	|   4	 |   LptData	 |   LptData   |
 *	+--------+---------------+-------------+
 *	|   5	 |   LptData	 |   LptCtrl   |
 *	+--------+---------------+-------------+
 *	|   6	 |   LptData*2	 |   LptData   |
 *	+--------+---------------+-------------+
 *	|   7	 |   LptData*2	 |   LptCtrl   |
 *	+--------+---------------+-------------+
 *	Pin 17 active with $8000
 *	Pin 14 active with $2000
 *
 */

/* PAGE 0 of EPLC registers */
# define IDR0		0x0
# define IDR1		0x1
# define IDR2		0x2
# define IDR3		0x3
# define IDR4		0x4
# define IDR5		0x5
# define TBCR0		0x6
# define TBCR1		0x7
# define TSR		0x8
# define RSR		0x9
# define ISR		0xA
# define IMR		0xB
# define CMR1		0xC
# define CMR2		0xD
# define MODSEL		0xE
# define MODDEL		0xF

# define PNR		TBCR0
# define COLR		TBCR1

/* PAGE 1 of EPLC registers */
# define MAR0		0x0
# define MAR1		0x1
# define MAR2		0x2
# define MAR3		0x3
# define MAR4		0x4
# define MAR5		0x5
# define MAR6		0x6
# define MAR7		0x7
# define PCMR		TBCR0
# define PDR		TBCR1

/* The followings define remote DMA command through LptCtrl */
# define ATFD		3		/* ATFD bit in Lpt's Control register */
					/* -> ATFD bit is added for Xircom's MUX */
# define Ctrl_LNibRead	0x8+ATFD	/* specify low  nibble */
# define Ctrl_HNibRead	0x0+ATFD	/* specify high nibble */
# define Ctrl_SelData	0x4+ATFD	/* not through LptCtrl but through LptData */
# define Ctrl_IRQEN	0x10		/* set IRQEN of lpt control register */

/* Here define constants to construct the required read/write commands */
# define WrAddr		0xC0		/* set address of EPLC write register */
# define RdAddr		0x40		/* set address of EPLC read register */

# define EOR		0x20		/* ORed to make 'end of read',set CSB=1 */
# define EOW		0xE0		/* end of write, R/WB=A/DB=CSB=1 */
# define EOC		0xE0		/* End Of r/w Command, R/WB=A/DB=CSB=1 */
# define HNib		0x10

/* EPLC register contents */
# define EPLC_RBER	0x1		/* ISR, HNib */
# define EPLC_ROK	0x4		/* ISR */
# define EPLC_TER	0x2		/* ISR */
# define EPLC_TOK	0x1		/* ISR */

# define EPLC_POV	0x4		/* RSR, HNib */
# define EPLC_BUFO	0x1		/* RSR, HNib */

# define EPLC_RST	0x4		/* CMR1, HNib */
# define EPLC_RxE	0x2		/* CMR1, HNib */
# define EPLC_TxE	0x1		/* CMR1, HNib */
# define EPLC_RETX	0x8		/* CMR1 */
# define EPLC_TRA	0x4		/* CMR1 */
# define EPLC_IRQ	0x2		/* CMR1 */
# define WRPAC		0x2		/* CMR1	*/
# define RDPAC		0x1		/* CMR1	*/
# define EPLC_BUFE	0x1		/* CMR1 */
# define EPLC_JmpPac	EPLC_BUFE

# define EPLC_AM1	0x2		/* CMR2, HNib */
# define EPLC_AM0	0x1		/* CMR2, HNib */
# define EPLC_PAGE	0x4		/* CMR2 */
# define EPLC_RAMTST	0x2		/* CMR2 */
# define EPLC_LOOPBK	0x4		/* CMR2 */
# define EPLC_IRQOUT	0x1		/* CMR2 */
# define EPLC_CMR2Null	0x0		/* CMR2 */

# define EPLC_AddrMode	0x2		/* accept physical & broadcast packets */
					/* reject error packets */
# define RxMode		EPLC_AddrMode

# define RxCount	2
# define RxLSB		3
# define RxMSB		4
# define RxStatus	5


# endif /* _rtl8012_h */
