#include <stdio.h>
#include <mintbind.h>

/* in delay5.spp */

extern long calibrate(const long passes);
extern void delay5(long t);

long delayfactor;				/* holds the calibration result */

/* The bigger the PASSES value, the more accurate
 * is the calibration loop, but also the more time
 * it takes to calibrate (this on F030 lasts for 5 secs).
 */

#define	PASSES	100000000L

void
init_delay(void)
{
	long f;

	f = calibrate(PASSES);			/* must be called in supervisor */

	delayfactor = (PASSES / f) / 5;		/* one millisecond */
	delayfactor += 100;			/* round */
	delayfactor /= 200;			/* 5 microseconds */
}

/* Test routine
 *
 * The `delayfactor' holds the number of loop passes needed
 * to waste 5 microseconds.
 *
 * On Falcon this is 10, on TT 20. I don't know for ST, but I guess
 * this would be 2, which (out of timing table calculation) is 
 * equal to 4.5 microseconds, not 5. At the other hand, calling 
 * the function on 68000 takes another 4.5 microseconds by 
 * itself, so for delay values smaller than 10 the delay will be 
 * too long, it becomes accurate at about 10, and for bigger 
 * values delays will be shorter than expected.
 *
 * The result of the test is usually bigger than desired 10 seconds.
 * This is so because of multitasking (the test runs in user mode).
 * Inside the kernel measured timings should be much more accurate.
 *
 */

int
main()
{
	long tv1[2], tv2[2];
	long oldstack = 0;

	printf("Calibrating delay loop... ");
	fflush(stdout);
	Tgettimeofday(&tv1, 0L);
	Supexec(init_delay);
	Tgettimeofday(&tv2, 0L);

	printf("done: delay factor %ld (need %ld sec)\n", delayfactor, tv2[0]-tv1[0]);

	printf("Checking loop... ");
	fflush(stdout);

	Tgettimeofday(&tv1, 0L);

	if (!Super (1L))
		oldstack = Super (0L);
	delay5(2000000);		/* = 10 seconds */
	if (oldstack)
		Super (oldstack);

	Tgettimeofday(&tv2, 0L);

	printf("done: %ld sec, %ld usec.\n", tv2[0]-tv1[0], tv2[1]-tv1[1]);
	return 0;
}
