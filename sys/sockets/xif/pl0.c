
# include "global.h"
# include "netinfo.h"

# include "pl0.h"
# include "plip.h"

# include <osbind.h>


/* BUG: no checks for existing interrupt routines */
short
pl0_init (void)
{
	pl0_old_busy_int = Setexc (0x40, pl0_busy_int);
	
	/* enable BUSY interrupt */
	__asm__
	(
		"bset	#0, 0xfffffa09:w	\n\t"	/* IERB */
		"bset	#0, 0xfffffa15:w	\n\t"	/* IMRB */
		"bclr	#0, 0xfffffa03:w	\n\t"	/* AER */
	);
	
	pl0_set_direction (0);
	pl0_set_strobe (1);
	pl0_cli ();
	
	return 0;
}

void
pl0_open (void)
{
	pl0_sti ();
}

void
pl0_close (void)
{
	pl0_cli ();
}
