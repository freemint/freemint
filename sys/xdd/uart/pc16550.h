/*
  UART-Register.
 */

/* The structure of the NS16550 register file. Note that in case of a collision
   between two registers, the first register is defined here and the second
   register  is defined to be the same.
 */
typedef struct
{
  volatile uchar iop;	/* 0 Receive/Transmit register (RBR, THR) */
  volatile uchar ier;	/* 1 Interrupt enable register      (IER) */
  volatile uchar fcr;	/* 2 Fifo  control register     (IIR/FCR) */
  volatile uchar lcr;	/* 3 Line  control register         (LCR) */
  volatile uchar mcr;	/* 4 Modem control register         (MCR) */
  volatile uchar lsr;	/* 5 Line  status register          (LSR) */
  volatile uchar msr;	/* 6 Modem status register          (MSR) */
  volatile uchar scr;	/* 7 Scratch register               (SCR) */
} UART;

/* iop Lesen/Schreiben	*/
#define	thr	iop
#define	rbr	iop
/* iir Interrupt Identification Register (lesen).	*/
#define	iir		fcr
/* Bank1 for Baud Rate.	*/
#define	dll	iop
#define	dlm	ier
/* Bank 3 lcr shadow.	*/
#define	sh_lcr	ier

/*
  ier Interrupt Enable Register. Bank 0, Offset 01h	
  in the UART, IrDA SIR and Sharp-IR Modes
  Bank 0, Offset 01h.
  */

/* Receiver High-Data-Level Interrupt Enable.	*/
/* 0 - Disable Receiver Data Ready Interrupt.*/
/* 1 - Enable Receiver Data Ready Interrupt. */
#define RXHDL_IE	0x01

/* Transmitter Low-Data-Level Interrupt Enable.	*/
/* 0 - Interrupt Disabled.	*/
/* 1 - Interrupt Enabled.	*/
#define	TXLDL_IE	0x02

/* Line Status Interrupt Enable.	*/
/* 0 - Disable Line Status Interrupts.	*/
/* 1 - Enable Line Status Interrupts.	*/
#define	LS_IE		0x04

/* Modem Status Interrupt Enable.	*/
#define MS_IE		0x08
/* 0 - Disable Modem Status Interrupts.	*/
/* 1 - Enable Modem Status Interrupts.	*/

/* DMA Interrupt Enable.	*/
#define	DMA_IE		0x10
/* 0 - Disable DMA terminal count Interrupt.	*/
/* 1 - Enable DMA terminal count Interrupt.	*/

/* Transmitter Empty Interrupt Enable and Pipline Load Interrupt Enable. */
/* 0 - Disable Interrupts.	*/
/* 1 - Enable Interrupts.	*/
#define	PLD_IE		0x20
#define	TXEMP_IE	0x20

/* Timer Interrupt Enable.	*/
/* 0 - Disable Timer Interrupt.	*/
/* 1 - Enable Timer Interrupt.	*/
#define	TMR_IE		0x80

/* in Fast IR (MIR and FIR) Modes.	*/
/* Line Status Interrupt Enable or Transmission	*/
/* FIFO Underrun Interrupt Enable.	*/
/* 0 - Interrupt Disabled.	*/
/* 1 - Interrupt Enabled.	*/
#define	TXUR_IE		0x04

/* Status FIFO Threshold Interrupt Enable.	*/
/* 0 - Interrupt disabled.	*/
/* 1 - Interrupt enabled.	*/
#define	SFIF_IE		0x40

/*
  iir Interrupt Identification Register. Read Cycles. Bank 0, Offset 02h
  Non-Extended Modes.
  */

/* Interrupt Pending Flag.	*/
/* 0 - There is an interrupt pending.	*/
/* 1 - No Interrupt pending.	*/
#define	IPND		0x01

#define IPR0		0x02	/* Interrupt Priority 1,0.	*/
#define	IPR1		0x04
#define	RXFT		0x08	/* Reception FIFO Time-Out.	*/

/* FIFOs Enabled.	*/
#define	FEN0		0x40	/* 0 - No FIFO enabled.	*/
#define	FEN1		0x80	/* 1 - FIFOs are enabled (bit 0 of FCR is set to 1).	*/


/* Non-Extended Mode Interrupt Priorities.
*_______________________________________________________________________________________________________________*
|IIR Bits|Priority|Interrupt Type|           Interrupt Source               |     Interrupt Reset Control       |
|  3210  | Level  |              |                                          |                                   |
----------------------------------------------------------------------------------------------------------------|
|  0001  |   -    |     None     |               None                       |                                   |
----------------------------------------------------------------------------------------------------------------|
|  0110  | Highest| Line Status  | Parity error, framing error, data overrun| Read Line Status Register (LSR).  |
|        |        |              | or break event.                          |                                   |
----------------------------------------------------------------------------------------------------------------|
|  0100  | Second | Receiver High| Receiver Buffer Register (RBR) full, or  | Reading the RBR or, reception     |
|        |        |   Data Level | reception FIFO level equal to or above   | FIFO level drops below threhold.  |
|        |        |              | treshold.                                |                                   |
----------------------------------------------------------------------------------------------------------------|
|  1100  | Second |Reception FIFO|At least one character is in the reception| Reading the RBR.                  |
|        |        |   Time-Out   |FIFO, and no character has been input     |                                   |
|        |        |              |to or read from the reception FIFO for    |                                   |
|        |        |              |four character times.                     |                                   |
----------------------------------------------------------------------------------------------------------------|
|  0010  | Third  | Transmitter  | Transmitter Data Register Empty.         |Reading the IIR Register (if source|
|        |        |    Ready     |                                          |of interrupt) or writing into the  |
|        |        |              |                                          |Transmitter Data Register.         |
----------------------------------------------------------------------------------------------------------------|
|  0000  | Fourth | Modem Status | Any transition on /CTS, /DSR or /DCD or  | Reading the Modem Status Register |
|        |        |              | a low to high transition on /RI.         | (MSR).                            |
*---------------------------------------------------------------------------------------------------------------*/
/* Extended Mode.	*/
/* Receive High-Data-Level Event.	*/
/* 0 - Receiver data level is not high.	*/
/* 1 - Receiver data level is high.	*/
#define	RXHDL_EV	0x01

/* Transmitter Low-Data-Event.	*/
/* 0 - Transmitter data level is not low.	*/
/* 1 - ransmitter data level is low.	*/
#define	TXLDL_EV	0x02

#define LS_EV		0x04	/* Line Status Event or	*/
#define	TXUR_EV		0x04	/* Transmitter Underrun Event.	*/

/* Modem Status Event.	*/
/* 0 - Modem Status Interrupt event.	*/
/* 1 - Disabled. MS_EV forced to 0.	*/
#define	MS_EV		0x08

#define	DMA_EV		0x10	/* DMA Event Occured.	*/

#define	TXEMP_EV	0x20	/* Transmitter Empty or	*/
#define	PLD_EV		0x20	/* Pipeline Load Event.	*/

#define	SFIF_EV		0x40	/* Status FIFO Event.	*/

#define	TMR_EV		0x80	/* Timer Event.	*/

/*
  iir Event Identification Register. Write Cycles.
   Bank 0 Offset 02h
   */
#define	FIFO_EN		0x01	/* FIFO Enable.	*/
#define	RXFR		0x02	/* Clear Receive FIFO.	*/
#define	TXFR		0x04	/* Clear Transmit FIFO.	*/


/* Receiver FIFO Threshold Level Select. */
#define RXFTH_1  0x00
#define RXFTH_4  0x40
#define RXFTH_8  0x80
#define RXFTH_14 0xC0
/*
  lcr Line Control Register or bsr Bank Select Register.
  Bank 0, Offset 03h.
  */
#define	bsr	lcr

/* Word Length Select.	*/
/* WLS1 | WLS0 | character length	*/
/*   0  |   0  | 5	*/
/*   0  |   1  | 6	*/
/*   1  |   0  | 7	*/
/*   1  |   1  | 8	*/
#define	WLS0		0x01
#define	WLS1		0x02
#define	WLS		0x03	/* Bitmaske */

/* Number of Stop Bits.	*/
#define	STB		0x04
/* 0 - One stop bit is generated. */
/* 1 - if data length 5: 1 1/2    */
/*     else            : 2        */

/* Parity Enable.	*/
/* 0 - No parity bit is used.	*/
/* 1 - A parity bit is generated.	*/
#define	PEN		0x08

/* Even Parity Enable.	*/
/* 0 - odd.	*/
/* 1 - even.	*/
#define	EPS		0x10

/* Stick Parity Select.	*/
/* PEN | EPS | STKPS | Selected Parity Bit */
/*  0  |  x  |   x   |  None	           */
/*  1  |  0  |   0   |  Odd                */
/*  1  |  1  |   0   |  Even               */
/*  1  |  0  |   1   |  Locic 1            */
/*  1  |  1  |   1   |  Locic 0            */
#define STKPS		0x20

/* Set Break.	*/
/* 0 - the break is disabled.	*/
/* 1 - Break enabled. see doc.	*/
#define	SBRK		0x40

/* Bank Select Enable.	*/
#define	BNKSE		0x80

/*
  mcr Modem Control Register.
  Bank 0, Offset 04h
  Non Extended UART mode.
  */
#define	DTRO		0x01	/* Data Terminal Ready.	*/
#define	RTSO		0x02	/* Request To Send.	*/
#define	RILP		0x04	/* Loopback Interrupt Request.	*/
				/* Global Interrupt Enable or	*/
#define	GI_EN		0x08	/* 0 - Interrupt disabled.	*/
				/* 1 - Interrupt enabled.	*/

/* Local Loopback Enable.	*/
#define LOOP		0x10	/* 0 - Local loopback disabled.	*/
				/* 1 - Local loopback enabled.	*/

#define	DTRO_BIT	0	/* Data Terminal Ready.	*/
#define	RTSO_BIT	1	/* Request To Send.	*/
#define	RILP_BIT	2	/* Loopback Interrupt Request.	*/
#define	GI_EN_BIT	3	/* Loopback DCD.	*/


/* Extended Mode.	*/

/* DMA Enable.	*/
#define	DMA_EN		0x04	/* 0 - DMA not enabled.	*/
				/* 1 - Enable DMA.	*/
				/* Transmission Defferral.	*/
#define	TX_DFR		0x08	/* 0 - No Transmission defferral enabled.	*/
				/* 1 - Transmission defferral enabled.	*/
				/* Send Interaction Pulse.	*/
#define	IR_PLS		0x10	/* 0 - No action.	*/
				/* 1 - An infrared pulse is transmitted.	*/
/* Operation Mode.	*/
/* MDSL2 | MDSL1 | MDSL0 | Oeration Mode	*/
/*   0   |   0   |   0   | UART Modes	*/
/*   0   |   0   |   1   | Reserved		*/
/*   0   |   1   |   0   | Sharp-IR		*/
/*   0   |   1   |   1   | IrDA SIR		*/
/*   1   |   0   |   0   | Extended IrDA MIR (1.152 Mbps)	*/
/*   1   |   0   |   1   | Extended IrDA FIR (4.0 Mbps)	*/
/*   1   |   1   |   0   | Consumer Remote Control	*/
/*   1   |   1   |   1   | Reserved		*/
#define	MDSL0		0x20
#define	MDSL1		0x40
#define	MDSL2		0x80

/*
 lsr Line Status Register.
 Bank 0 Offset 05h
 */

/* Receive Data Aviable.	*/
#define	RXDA		0x01
/* 0 - The Receive Data Register or the FIFO has been read.	
   1 - A complete incomming character has been received and
   transferred into the Receiver Data Register or FIFO.	*/

/* Overrun Error.	*/
#define OE		0x02	/* 1 - Overrun has occurred.	*/
#define	PE		0x04	/* Parity Error or	*/
#define	BAD_CRC		0x04	/* CRC Error.	*/
#define	FE		0x08	/* Framing Error or	*/
#define	PHY_ERR		0x08	/* Physical Layer Error.	*/
#define	BRK		0x10	/* Break Character Received or	*/
#define	MAX_LEN		0x10	/* Maximum Length Exceeded.	*/
#define	TXDE		0x20	/* Transmitter Data Register Empty.	*/
#define	TEMT		0x40	/* Transmitter Empty.	*/
#define	ERR_INF		0x80	/* Error in Receiver FIFO or	*/
#define	FR_END		0x80	/* Frame End.	*/
/*
  msr Modem Status Register.
  Bank 0, Offset 06h.
  */

/* Delta Clear to Send.	*/
#define	DCTS		0x01	/* 1 - /CTS state changed.	*/

/* Delta Data Set Ready.	*/
#define	DDSR		0x02	/* 1 - DSR state changed.	*/

/* Trailing Edge Ring Indicate.	*/
#define	TERRI		0x04	/* 1 - Low to high transition of /RI.	*/

/* Delta Data Carrier Detect.	*/
#define	DDCD		0x08	/* 1 - /DCD signal state changed.	*/
#define	CTSI		0x10	/* This bit returns the inverse of the /CTS input signal.	*/
#define	DSRI		0x20	/* This bit returns the inverse of the /DSR input signal.	*/
#define	RII		0x40	/* This bit returns the inverse of the /RI input signal.	*/
#define	DCDI		0x80	/* This bit returns the inverse of the /DCD input signal.	*/

