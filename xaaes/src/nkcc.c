
#include "nkcc.h"


unsigned short
gem_to_norm (short ks, short kr)
{
	unsigned short knorm;

	knorm = nkc_tos2n (((long) (kr & 0xff) |	/* ascii Bits 0-7 */
			    (((long) kr & 0x0000ff00L) << 8L) |	/* scan Bits 16-23 */
			    ((long) (ks & 0xff) << 24L)));	/* kstate Bits 24-31 */
	return knorm;
}


int
nkc_exit(void)
{
	/* dummy */
	return 0;
}
