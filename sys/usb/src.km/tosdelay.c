/*
 * tosdelay.c - initialise values used to provide microsecond-order delays
 * based on delay.c from EmuTOS
 *
 * note that the timings are quite imprecise (but conservative) unless
 * you are running on at least a 32MHz 68030 processor
 *
 * Copyright (C) 2013-2017 The EmuTOS development team
 *
 * Authors:
 *  RFB    Roger Burrows
 *
 * Modified by Claude Labelle to include mdelay() and udelay() functions
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See /COPYING.GPL for details.
 */

#include "tosdelay.h"

/*
 * initial 1 millisecond delay loop values
 */
#define LOOPS_68060		110000	/* 68060 timing assumes 110MHz for safety */
#define LOOPS_68030		3800	/* 68030 timing assumes 32MHz */
#define LOOPS_68000		760	/* 68000 timing assumes 16MHz */

#define CALIBRATION_TIME	100	/* target # millisecs to run calibration */

//#define COOKIE_CPU		0x5f435055L

/*
 * global variables
 */
ulong loopcount_1_msec;
ulong delay_1usec;

/*
 * function prototypes (functions in delayasm.S)
 */
ulong run_calibration(ulong loopcount);
void calibration_timer(void);

/*
 * initialise delay values
 *
 * used for initial estimate, in order for calibration to be run for a reasonable amount of time
 */
static inline void set_tos_delay(void)
{
	init_delay();
	calibrate_delay();
}
static inline void init_delay(void)
{
	unsigned int mcpu = getmCPU();
	switch(mcpu) {
		case 60:
			loopcount_1_msec = LOOPS_68060;
			//c_conws("68060 CPU.\r\n");
			break;
		case 40:
		case 30:			/* assumes 68030 */
			loopcount_1_msec = LOOPS_68030;
			//c_conws("68030/040 CPU.\r\n");
			break;
		default:			/* assumes 68000 */
			loopcount_1_msec = LOOPS_68000;
			//c_conws("68000 CPU.\r\n");
	}
}

/*
 * calibrate delay values: must only be called *after* interrupts are allowed
 *
 * NOTE: we use TimerD so we restore the RS232 stuff
 */
static inline void calibrate_delay(void)
{
	ulong loopcount, intcount;
	ulong ret;
	/*
	 * disable interrupts then run the calibration
	 */
	Jdisint(MFP_TIMERD);
	ret = Super(0L);
	loopcount = CALIBRATION_TIME * loopcount_1_msec;
	intcount = run_calibration(loopcount);
	Super(ret);
	Jenabint(MFP_TIMERD);

	/*
	 * disable interrupts then restore the RS232
	 * serial port stuff (in case we're using it)
	 */
	//Jdisint(MFP_TIMERD);
	// TO DO: will need to save and restore values
	//rsconf1(B9600, 0, 0x88, 1, 1, 0);   /* just like init_serport() */
	//Jenabint(MFP_TIMERD);

	/*
	 * intcount is the number of interrupts that occur during 'loopcount'
	 * loops.  an interrupt occurs every 1/960 sec (see delayasm.S).
	 * so the number of loops per second = loopcount/(intcount/960).
	 * so, loops per millisecond = (loopcount*960)/(intcount*1000)
	 * = (loopcount*24)/(intcount*25).
	 */
	
	if (intcount)	   /* check for valid */
		loopcount_1_msec = (loopcount * 24) / (intcount * 25);

	delay_1usec = loopcount_1_msec / 1000;
}

static inline void udelay (register ulong usecs)
{
	while (usecs--)
		delay_loop(delay_1usec);
}

static inline void mdelay (register ulong msecs)
{
	while (msecs--)
		delay_loop(loopcount_1_msec);
}

static inline int getmCPU(void)
{
	unsigned int mcpu;
	long val;
	mcpu = (getcookie(COOKIE__CPU, &val) == TRUE)?(int)(val & 0x0000FFFF):0;
	return mcpu;
}
