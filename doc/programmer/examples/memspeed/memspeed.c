/* memspeed.c
 *
 * a test for memory move efficiency
 *
 */

#include <stdio.h>
#include <mintbind.h>
#include <math.h>

typedef struct {
	long sec;
	long usec;
} tv;

tv t1, t2;
double r1, r2;

void time_start(void)
{
	fflush(stdout);
	Tgettimeofday(&t1, 0L);
}

void time_stop(void)
{
	Tgettimeofday(&t2, 0L);
	r1 = ((double)t2.sec + (double)t2.usec / 1000000) \
		- ((double)t1.sec + (double)t1.usec / 1000000);
	r2 = 32768 / r1;
	printf(": %f secs, %f kB/s\n", r1, r2);
}

void pass(char *ptr1, char *ptr2)
{
	extern void copy256m(char *src, char *dst, long num);
	extern void copy512m(char *src, char *dst, long num);
	extern void copy256l(char *src, char *dst, long num);
	extern void longcopy(char *src, char *dst, long num);
	extern void wordcopy(char *src, char *dst, long num);
	extern void bytecopy(char *src, char *dst, long num);

	unsigned short x;

	printf("single bytes (1 * move.b)     ");
	time_start();
	for (x = 0; x < 32; x++)
		bytecopy(ptr1, ptr2, 1048576L);
	time_stop();

	printf("single words (1 * move.w)     ");
	time_start();
	for (x = 0; x < 32; x++)
		wordcopy(ptr1, ptr2, 524288L);
	time_stop();

	printf("single longwords (1 * move.l) ");
	time_start();
	for (x = 0; x < 32; x++)
		longcopy(ptr1, ptr2, 262144L);
	time_stop();

	printf("256-byte blocks (64 * move.l) ");
	time_start();
	for (x = 0; x < 32; x++)
		copy256l(ptr1, ptr2, 4096L);
	time_stop();

	printf("256-byte blocks (12 * movem.l)");
	time_start();
	for (x = 0; x < 32; x++)
		copy256m(ptr1, ptr2, 4096L);
	time_stop();

	printf("512-byte blocks (22 * movem.l)");
	time_start();
	for (x = 0; x < 32; x++)
		copy512m(ptr1, ptr2, 2048L);
	time_stop();
}

int main()
{
	char *ptr1, *ptr2;
	unsigned long ccw, ccm;

	if (Ssystem(-1, 0L, 0L) < 0) {
		printf("Sorry, MiNT >= 1.15.0 is required.\n");
		return -32;
	}

	ptr1 = (char *)Mxalloc(2097152L, 3);
	ptr2 = ptr1 + 1048576L;

	if ((long)ptr1 == 0) {
		printf("Insufficient memory, 2 MB is required.\n");
		return -1;
	}

	printf("\nMemspeed test: copy a 32 MB across memory\n" \
		"-----------------------------------------\n\n");

	if (Ssystem(0x0017, -1L, -1L) == 0) {
		ccw = Ssystem(0x0017, -1L, 0L);
		ccm = Ssystem(0x0017, 0L, -1L);
		Ssystem(0x0017, 2L, 2L);	/* data cache on */
		printf("First pass, data cache on\n\n");
		pass(ptr1, ptr2);
		printf("\nSecond pass, data cache off\n\n");
		Ssystem(0x0017, 0L, 2L);	/* data cache off */
		pass(ptr1, ptr2);
		Ssystem(0x0017, ccw, ccm); 	/* restore caches */
	} else 
		pass(ptr1, ptr2);

	printf("\n");
	Mfree(ptr1);
	return 0;
}
