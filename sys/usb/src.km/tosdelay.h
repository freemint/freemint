/*
 *  tosdelay.h
 *
 *  Copyright (c) 2018 Claude Labelle
 *
 *  For license details see COPYING.GPL
 */

#ifndef tosdelay_h
#define tosdelay_h

//#include "portab.h"

/*
 * this is the value to pass to the inline function delay_loop()
 * to get a delay of 1 millisecond.  other delays may be obtained
 * by multiplying or dividing as appropriate.  when calculating
 * shorter delays, rounding up is not necessary: because of the
 * instructions used in the loop (see below), the number of loops
 * executed is one more than this count (iff count >= 0).
 */
extern ulong loopcount_1_msec;

/*
 * function prototypes
 */
static inline void set_tos_delay(void);
static inline void init_delay(void);
static inline void calibrate_delay(void);
static inline int getmCPU(void);

/*
 * Loops for the specified count; for a 1 millisecond delay on the
 * current system, use the value in the global 'loopcount_1_msec'.
 */

#define delay_loop(count)                                      \
	__extension__                                          \
	({                                                     \
		ulong _count = (count);                        \
		__asm__ volatile                               \
		(                                              \
			"0:\n\t"                               \
			"subq.l #1,%0\n\t"                     \
			"jpl    0b"                            \
			:                   /* outputs */      \
			: "d"(_count)       /* inputs  */      \
			: "cc", "memory"    /* clobbered */    \
		);                                             \
	})

#endif /* tosdelay_h */
