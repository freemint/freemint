/* Minixfs translation configurer */

# include <mintbind.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

# include <mint/dcntl.h>

/* Filename translation modes
 */
# define SRCH_TOS	0x01	/* search with tosify, tos domain  */
# define SRCH_MNT	0x02	/* search with tosify, mint domain */
# define DIR_TOS	0x04	/* dir compat tosify, tos domain  */
# define DIR_MNT	0x08	/* dir compat tosify, mint domain */
# define LWR_TOS	0x10	/* lower case creat, tos domain  */
# define LWR_MNT	0x20	/* lower case creat, mint domain */
# define AEXEC_TOS	0x40	/* auto 'x', tos domain.   */
# define AEXEC_MNT	0x80	/* auto 'x', mint domain. */


/* mfsconf.c */
int	main	(int argc, char **argv);
void	usage	(void);
long	tr_get	(long arg, int chr);
char *	tr_text	(int arg);

long trans;

int
main (int argc, char **argv)
{
	char err = 0;
	char c;
	long tmp;
	
	if (argc == 1)
	{
		usage ();
	}
	
	if (Dcntl (MFS_VERIFY, argv[1], &trans) || trans != MFS_MAGIC)
	{
		fprintf (stderr, "Pathname: %s is not a minix partition.\n", argv[1]);
		exit (1);
	}
	
	Dcntl (MFS_GTRANS, argv[1], &trans);
	
	if (argc == 2)
	{
		printf ("Minix Filesystem Path %s\nConfiguration:\n", argv[1]);
		printf ("Directory Translation %s.\n", tr_text (DIR_TOS));
		printf ("Search Translation    %s.\n", tr_text (SRCH_TOS));
		printf ("Auto Exec Setting     %s.\n", tr_text (AEXEC_TOS));
		printf ("Lower Case Creation   %s.\n", tr_text (LWR_TOS));
		exit (0);
	}
	
	/* Parse command-line options */
	opterr = 0;
	while ((c = getopt (argc - 1, &argv[1], "l:L:d:D:s:S:x:X:")) != EOF)
	{
        	switch (c)
        	{
        		case 'l':
			case 'L':
			{
				trans &= ~(LWR_TOS | LWR_MNT); 
				tmp = tr_get (LWR_TOS, optarg[0]);
				if (tmp == -1) err = 1;
				else trans |= tmp;
				break;
			}
			case 'd':
			case 'D':
			{
				trans &= ~(DIR_TOS | DIR_MNT);
				tmp = tr_get (DIR_TOS, optarg[0]);
				if (tmp == -1) err = 1;
				else trans |= tmp;
				break;
			}
			case 'x':
			case 'X':
			{
				trans &= ~(AEXEC_TOS | AEXEC_MNT);
				tmp = tr_get (AEXEC_TOS, optarg[0]);
				if (tmp == -1) err = 1;
				else trans |= tmp;			
				break;
			}
			case 's':
			case 'S':
			{
				trans &= ~(SRCH_TOS | SRCH_MNT);
				tmp = tr_get (SRCH_TOS, optarg[0]);
				if (tmp == -1) err = 1;
				else trans |= tmp;
				break;
			}
			case '?':
			{
				err = 1;
				break;
			}
		}
	}
	
	if (err)
	{
		usage();
	}

	Dcntl (MFS_STRANS, argv[1], &trans);
	exit (0);
}

void
usage (void)
{
	fprintf (stderr, "Usage : mfsconfig path -[ldsx] [m|t|b|n]\n");
	fprintf (stderr, "'path' is any minix filesystem path\n");
	fprintf (stderr, "-l : Lower case creation\n");
	fprintf (stderr, "-d : Directory entry translation\n");
	fprintf (stderr, "-s : Search translation\n");
	fprintf (stderr, "-x : Exec translation\n");
	fprintf (stderr, "m : MiNT domain only\n");
	fprintf (stderr, "t : TOS domain only\n");
	fprintf (stderr, "b : Both domains (always)\n");
	fprintf (stderr, "n : Neither domain (never)\n");
	exit (1);
}

long
tr_get (long arg, int chr)
{
	switch (chr)
	{
		case 'b':
		case 'B':
		{
			return (arg | (arg << 1));
		}
		case 'n':
		case 'N':
		{
			return 0;
		}
		case 't':
		case 'T':
		{
			return arg;
		}
		case 'm':
		case 'M':
		{
			return arg << 1;
		}
		default:
		{
			return -1;
		}
	}
}

char *
tr_text (int arg)
{
	static char *tr_msg[] = { "Never", "TOS domain", "MiNT domain", "Always" };
	int index = 0;
	
	if (trans & arg) index |= 1;
	if (trans & (arg << 1)) index |= 2;
	
	return tr_msg[index];
}
