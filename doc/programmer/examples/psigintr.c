/* 
 * Rude example of using Psigintr()
 *
 * The program outputs a period to the stdout any time you
 * press a key or move the mouse.
 *
 */

#include <signal.h>
#include <mintbind.h>

#define KEYBOARD_ACIA	0x0118L

/* Signal handler for SIGUSR1 */

static void sig()
{
	Cconws(".");
}

main()
{
	/* Install signal handler */
	Psignal(SIGUSR1, sig);		

	/* Register the keyboard ACIA interrupt as SIGUSR1 */
	Psigintr(KEYBOARD_ACIA >> 2, SIGUSR1);

	/* Sleep */
	for(;;)
		Pause();

	return 0;
}
