/* S_CTRLCACHE in five easy steps.
 *	Examples of new CPU cache controls in MiNT.
 *
 * Author: Konrad Kokoszkiewicz, draco@atari.org
 *
 * Mode 23 of Ssystem() call provides a CPU independent cache 
 * control. You have not to worry on the processor type installed, 
 * cache modes can be requested even on 68000, but that of course 
 * has no effect. If a cache function is not supported by a 
 * particular processor, corresponding bits of the Cache Control 
 * Word are harmlessly ignored.
 *
 * Usage:
 *
 * long Ssystem(0x0017, cache_control_word, cache_control_mask);
 *
 * Bits of the Cache Control Word and Cache Control Mask
 * are defined as follows:
 *
 * 0 - instruction cache enable (if 1)
 * 1 - data cache enable
 * 2 - branch cache enable
 * 3 - instruction cache freeze
 * 4 - data cache freeze
 * 5 - instruction burst enable
 * 6 - data burst enable
 * 7 - data write allocate enable
 * 8 - instruction cache full mode enable
 * 9 - instruction cache read/write allocate enable
 * 10 - data cache full mode enable
 * 11 - data cache read/write allocate enable
 * 12 - branch cache invalidate all
 * 13 - branch cache invalidate user entries
 * 14 - CPUSH invalidate enable
 * 15 - store buffer enable
 * 16-31 - reserved, read as zeros and ignored when written.
 *
 * The Cache Control Word toggles the cache functions according to 
 * the state of bits defined above (1 = enable, 0 = disable). The 
 * corresponding bits of the Cache Control Mask validate bits of 
 * the Cache Control Word (1 = use, 0 = ignore).
 *
 * Bits 12 and 13, when set, perform branch cache invalidation
 * on a 68060 and aren't of much use (branch cache is invalidated
 * at each context switch anyways). They're always read as zeros.
 * 
 * Bits 16-31 of the Cache Control Word should be kept zeros for
 * upward compatibility.
 *
 * NOTICE: this call is suitable rather for a Control Panel module
 * or similar control program (MiNTSetter for example), than for
 * regular applications!
 *
 */

#include <stdio.h>
#include <mintbind.h>

/* Keep in mind that all this below is actually _NOT_ a program
 * to compile and use :)
 */

main()
{
	long ccw, ccm, r;

	/* Lesson 1: how to check if S_CTRLCACHE does exist */

	r = Ssystem(0x0017, -1L, -1L);
	if (r < 0) {
		printf("S_CTRLCACHE not supported\n");
		return r;
	}

	/* Lesson 2: rude examples to demonstrate the call */

	/* Note that out of the Cache Control Word and Cache Control Mask
	 * only the former is `writable', i.e. when you request some bits
	 * in the Cache Control Word to be changed, the changed word will
	 * be written to the processor and `memorized' there, so that
	 * next time you *read* the control word, it may turn over to
	 * be different than at first time. That's because the Cache Control
	 * Word keeps the current state of all supported cache functions.
	 */

	ccw = Ssystem(0x0017, -1L, 0L);	/* read the Cache Control Word */

	/* Opposite to that, the default Cache Control Mask never changes
	 * no matter what you pass as arg2 of the Ssystem() call. In fact,
	 * when you `write' the Cache Control Mask, its value only serves
	 * for validation of the Cache Control Word written simultaneously.
	 * This is the User Cache Control Mask.
	 * However, when you *read* the control mask, you receive the
	 * system's Default Cache Control Mask, which is a constant that does
	 * not validate anything, but tells you what cache functions are
	 * recognized and can be toggled using the Control Word.
	 */

	ccm = Ssystem(0x0017, 0L, -1L);	/* read the Default Control Mask */

	(void)Ssystem(0x0017, 1L, 1L);	/* enable instruction cache alone */
	(void)Ssystem(0x0017, 0L, 1L);	/* disable instruction cache alone */
	(void)Ssystem(0x0017, 1L, 0L);	/* WRONG: no effect! */
	(void)Ssystem(0x0017, 2L, 2L); 	/* enable data cache alone */
	(void)Ssystem(0x0017, 0L, 2L);	/* disable data cache alone */
	(void)Ssystem(0x0017, 3L, 3L);	/* enable both caches */
	(void)Ssystem(0x0017, 0L, 3L);	/* disable both caches */
	(void)Ssystem(0x0017, 1L, 3L);	/* enable instruction and disable data cache */

	/* Lesson 3: avoid misunderstanding */

	/* Remember, the Cache Control Mask validates ALL bits of the
	 * Cache Control Word, not only those set to 1! So if you want
	 * just to disable data cache, the following statement
	 * is actually a very BAD idea, because it disables everything:
	 */
	(void)Ssystem(0x0017, 0x0000L, 0xffffUL);	/* WRONG! */

	/* It should be rather done so: */
	(void)Ssystem(0x0017, 0x0000L, 0x0002L);	/* RIGHT! */

	/* Lesson 4: exercises */

	/* And now... what does THIS do? :) */
	(void)Ssystem(0x0017, 0x0063L, 0x0063L);

	/* Lesson 5: real life */

	/* Those were just examples for easy understanding. In real
	 * life you should first request the Default Control Mask to
	 * figure out what capabilities are there:
	 */
	ccm = Ssystem(0x0017, 0L, -1L);

	/* ... then request the current Cache Control Word ... */
	ccw = Ssystem(0x0017, -1L, 0L);

	/* ... then modify it as you wish and write back: */
	(void)Ssystem(0x0017, ccw & 0x0002, ccm & 0x0002L);

	/* Saving the initial Cache Control Word may be a good
	 * idea if the user suddenly wishes to reset caches to
	 * a `default' state:
	 */
	(void)Ssystem(0x0017, ccw, ccm);

	/* Thank you :) */

	return r;
}
