/*
 * Example of using new CPU cache controls in MiNT:
 * print out the cache status report.
 *
 * Author: Konrad Kokoszkiewicz, draco@atari.org
 *
 * Warszawa, 11.II.1999.
 *
 */

#include <stdio.h>
#include <mintbind.h>

main()
{
	long ccw, ccm, r;
	unsigned short x;

	r = Ssystem(0x0017, -1L, -1L);
	if (r < 0) {
		printf("S_CTRLCACHE not supported\n");
		return r;
	}

	ccw = Ssystem(0x0017,-1L,0L);	/* current cache state */
	ccm = Ssystem(0x0017,0L,-1L);	/* default control mask */

	printf("\nCPU cache status report:\n" \
		"------------------------\n\n");

	/* The default control mask returns a bit set
	 * for each "supported" function. If the mask turns
	 * over to be a zero, this means that the CPU
	 * offers no cache controls (i.e. it is either
	 * 68000 or 68010).
	 */

	if (ccm == 0) {
		printf("No cache control available\n");
		return 0;
	}

	/* Notice the Ssystem(0x0017,...) is designed so that if
	 * you're developing a desktop tool for controlling caches
	 * (or whatever), you needn't to do additional checks for
	 * CPU type: you are encouraged to trust the kernel.
	 */
	/* Moreover, you can actually *guess* the CPU type from the
	 * default control mask returned, because the system returns
	 * a different mask for a different CPU (except for 68000 and
	 * 68010, because both share the same zero mask).
	 */

	/* Steps:
	 * - check the bit in the default control mask to be sure
	 *   the desired feature is present at all, then
	 * - if the mask bit validates the control word bit, go ahead.
	 */

	printf("Instruction cache: ");
	if (ccm & 0x0001) {
		if (ccw & 0x0001)
			printf("present, enabled\n");
		else
			printf("present, disabled\n");
	} else
		printf("not available\n");

	printf("Data cache: ");
	if (ccm & 0x0002) {
		if (ccw & 0x0002)
			printf("present, enabled\n");
		else
			printf("present, disabled\n");
	} else
		printf("not available\n");

	printf("Branch cache: ");
	if (ccm & 0x0004) {
		if (ccw & 0x0004)
			printf("present, enabled\n");
		else
			printf("present, disabled\n");
	} else
		printf("not available\n");

	printf("Write buffer: ");
	if (ccm & 0x8000) {
		if (ccw & 0x8000)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Instruction burst: ");
	if (ccm & 0x0020) {
		if (ccw & 0x0020)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Data burst: ");
	if (ccm & 0x0040) {
		if (ccw & 0x0040)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Instruction full mode: ");
	if (ccm & 0x0100) {
		if (ccw & 0x0100)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Data full mode: ");
	if (ccm & 0x0400) {
		if (ccw & 0x0400)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Instruction read/write allocate: ");
	if (ccm & 0x0200) {
		if (ccw & 0x0200)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Data write allocate: ");
	if (ccm & 0x0080) {
		if (ccw & 0x0080)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("Data read/write allocate: ");
	if (ccm & 0x0800) {
		if (ccw & 0x0800)
			printf("available, enabled\n");
		else
			printf("available, disabled\n");
	} else
		printf("not available\n");

	printf("\n");

	return r;
}
