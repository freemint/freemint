/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1999-09-14
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _scc_h
# define _scc_h


typedef struct scc SCC;
struct scc
{
	volatile uchar	dum1;
	volatile uchar	sccctl;		/* a = 0xffff8c81 b = 0xffff8c85 */
	volatile uchar	dum2;
	volatile uchar	sccdat;		/* a = 0xffff8c83 b = 0xffff8c87 */
};

# if 1
# define scca	((SCC *) 0x00ff8c80L)
# define sccb	((SCC *) 0x00ff8c84L)
# else
# define scca	((SCC *) 0xffff8c80L)
# define sccb	((SCC *) 0xffff8c84L)
# endif

/* Control Register */

# define WR0		0x0
# define WR1		0x1
# define WR2		0x2
# define WR3		0x3
# define WR4		0x4
# define WR5		0x5		/* Transmit parameters and controls */
# define WR6		0x6
# define WR7		0x7
# define WR8		0x8		/* Transmit FIFO - sccdat */
# define WR9		0x9
# define WR10		0xa
# define WR11		0xb
# define WR12		0xc
# define WR13		0xd
# define WR14		0xe
# define WR15		0xf

# define RR0		0x0
# define RR1		0x1		/* Special Receive Condition status bits */
# define RR2		0x2
# define RR3		0x3
# define RR4		0x4
# define RR5		0x5
# define RR6		0x6
# define RR7		0x7
# define RR8		0x8		/* Receive FIFO	- sccdat */
# define RR9		0x9
# define RR10		0xa
# define RR11		0xb
# define RR12		0xc
# define RR13		0xd
# define RR14		0xe


/* write 0 */
# define NULLCODE	0x00
# define PHIGH		0x08
# define RESESI		0x10
# define SENDA		0x18
# define ENINT		0x20
# define RESINT		0x28
# define ERRRES		0x30
# define RHIUS		0x38

# define RESRXCRC	0x40
# define RESTXCRC	0x80
# define RESTXURUN	0xc0

/* write 4 */
# define PARITY_ENABLE	0x01	/* parity enable bit		*/
# define PARITY_MASK	0x02	/* parity mask			*/
# define  PARITY_ODD	0x00	/* 0 = odd			*/
# define  PARITY_EVEN	0x02	/* 1 = even			*/
# define STB_MASK	0x0C	/* stobits			*/
# define  STB_SYNCHRON	0x00	/* 00 = synchron		*/
# define  STB_ASYNC1	0x04	/* 01 = asynchron 1 stopbit	*/
# define  STB_ASYNC15	0x08	/* 10 = asynchron 1,5 stopbits	*/
# define  STB_ASYNC2	0x0C	/* 11 = asynchron 2 stopbits	*/
# define SYNC_MASK	0x30	/* syncmode			*/
# define  SYNC_MONO	0x00	/* 00 = monosync		*/
# define  SYNC_BI	0x10	/* 01 = bisync			*/
# define  SYNC_SDLCHDCL	0x20	/* 10 = SDLC/HDCL mode		*/
# define  SYNC_EXTERNAL	0x30	/* 11 = external sync		*/
# define TAKT_MASK	0xC0	/* takt				*/
# define  TAKT_1x	0x00	/* 00 =  1x data rate		*/
# define  TAKT_16x	0x40	/* 01 = 16x data rate		*/
# define  TAKT_32x	0x80	/* 10 = 32x data rate		*/
# define  TAKT_64x	0xC0	/* 11 = 64x data rate		*/

/* write 5 */
# define TXCRCEN	0x01
# define RTS		0x02
# define SDCR16		0x04
# define TXEN		0x08
# define SBRK		0x10
# define TX5BIT		0x00
# define TX6BIT		0x40
# define TX7BIT		0x20
# define TX8BIT		0x60
# define DTR		0x80

/* read 0 */
# define RXCA		0x01
# define ZC		0x02
# define TXBE		0x04
# define DCD		0x08
# define SH		0x10
# define CTS		0x20
# define TXUR		0x40
# define BRK		0x80


# endif /* _scc_h */
