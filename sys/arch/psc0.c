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

#include "psc0.h"

void
board_putchar (char ch)
{
	/* Wait until space is available in the FIFO */
	while (!(MCF_UART_USR(DBUG_UART_PORT) & MCF_UART_USR_TXRDY))
		;

	/* Send the character */
	MCF_UART_UTB(DBUG_UART_PORT) = (uint8)ch;
}

int
board_getchar_present (void)
{
	return (MCF_UART_USR(DBUG_UART_PORT) & MCF_UART_USR_RXRDY);
}

char
board_getchar (void)
{
	/* Wait until character has been received */
	while (!(MCF_UART_USR(DBUG_UART_PORT) & MCF_UART_USR_RXRDY))
		;
	return MCF_UART_URB(DBUG_UART_PORT);
}

#endif
