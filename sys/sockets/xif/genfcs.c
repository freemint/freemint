/*
 *	FCS table generator (appendix B.2 of RFC 1331)
 */

# include <stdio.h>
# include <sys/types.h>

# define P	0x8408

int
main (void)
{
	ulong b, v, i;
	
	printf ("static unsigned short fcstab[256] = {");
	for (b = 0;; )
	{
		if (b % 8 == 0)
			printf ("\n\t");
		
		v = b;
		for (i = 8; i--; )
			v = (v & 1) ? ((v >> 1) ^ P) : (v >> 1);
		
		printf ("0x%04x", v & 0xffff);
		if (++b == 256)
			break;
		
		printf (", ");
	}
	printf ("\n};\n");
	
	return 0;
}
