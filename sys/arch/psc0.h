/*
 * ColdFire psc0 basic I/O routines
 *
 * This is inspired from Freescale dBUG sources which have no copyright.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See /COPYING.GPL for details.
 */

/* This file is needed until we have a proper device driver module for the ColdFire
 * psc0 serial interface. These functions are used mainly by the kernel's debug
 * facilities.
 */

#ifdef __mcoldfire__

#ifndef _psc0_h
#define _psc0_h

typedef unsigned char   uint8;          /*  Unsigned byte       */

/* This MBAR address is only valid when any of the flavours of the BaS are present,
 * if running on Freescale dBUG this values must be set accordingly. Ideally we
 * should get the value from some kind of API implented by EmuTOS/FireTOS.
 */
#define				__MBAR ((unsigned char*)0xff000000)
#define MCF_UART_USR(x)		(*(volatile uint8 *)(void*)(&__MBAR[0x008604+((x)*0x100)]))
#define MCF_UART_USR_RXRDY	(0x01)
#define MCF_UART_USR_TXRDY	(0x04)
#define MCF_UART_URB(x)		(*(volatile uint8 *)(void*)(&__MBAR[0x00860C+((x)*0x100)]))
#define MCF_UART_UTB(x)		(*(volatile uint8 *)(void*)(&__MBAR[0x00860C+((x)*0x100)]))

#define DBUG_UART_PORT		0        /* PSC channel used as terminal */

/* Prototypes */
void board_putchar (char ch);
int board_getchar_present (void);
char board_getchar (void);

#endif /* _psc0_h */
#endif /* __mcoldfire__ */
