/*
 *	SLIP channel 0 initialization routines.
 *
 *	03/07/94, kay roemer.
 */

#include "config.h"
#include "netinfo.h"
#include "kerbind.h"
#include "util.h"
#include "osl0.h"

/* BUG: no checks done if the modem1 line really exists */
short
sl0_init (void)
{
	short sr;
	
	sl0_old_dcdint  = s_etexec ((short)0x41, (long)sl0_dcdint);
	sl0_old_ctsint  = s_etexec ((short)0x42, (long)sl0_ctsint);
	sl0_old_txerror = s_etexec ((short)0x49, (long)sl0_txerror);
	sl0_old_txint   = s_etexec ((short)0x4A, (long)sl0_txint);
	sl0_old_rxerror = s_etexec ((short)0x4B, (long)sl0_rxerror);
	sl0_old_rxint   = s_etexec ((short)0x4C, (long)sl0_rxint);
	
	/* Enable DCD interrupt */
	__asm__("bset	#1, 0xfffffa09:w	\n\t"	/* IERB */
		"bset	#1, 0xfffffa15:w	\n\t");	/* IMRB */

	/* Enable CTS interrupt */
	__asm__("bset	#2, 0xfffffa09:w	\n\t"	/* IERB */
		"bset	#2, 0xfffffa15:w	\n\t");	/* IMRB */

	/* Set the active edges to get an interrupt at the
	 * next signal change (either high->low or low->high).
	 *
	 * NOTE that DCD/CTS wire state is inverted before the MFP
	 * input. So when DCD/CTS changes from low to high the signal
	 * at MFP input changes from high to low ...
	 */
	sr = spl7 ();
	if (sl0_get_dcd ())
		__asm__("bset	#1, 0xfffffa03:w");	/* AER */
	else	__asm__("bclr	#1, 0xfffffa03:w");	/* AER */

	if (sl0_get_cts ())
		__asm__("bset	#2, 0xfffffa03:w");	/* AER */
	else	__asm__("bclr	#2, 0xfffffa03:w");	/* AER */
	spl (sr);
	return 0;
}

void
sl0_open (void)
{
	sl0_on_off = 1;
}

void
sl0_close (void)
{
	/* if sl0_on_off == 0 the interrupt routines will jump to the
	 * old interrupt routines without doing anything, thus making
	 * the normal device driver for the serial line work. */
	sl0_on_off = 0;
}
