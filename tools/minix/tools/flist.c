/* Simple inode in use dumper */

# include <mintbind.h>
# include <stdio.h>

# define LSIZE 1024

static struct
{
	long limit;
	unsigned short lst[LSIZE];
} fl;


int
main (int argc, char **argv)
{
	long i;
	
	fl.limit = LSIZE;
	
	i = Dcntl (0x10e, argv[1], &fl);
	printf ("Returned %ld Inodes in use:\n", i);
	for (i = 0; i < LSIZE && fl.lst[i]; i++)
	{
		printf ("%u\n", fl.lst[i]);
	}
	
	return 0;
}
