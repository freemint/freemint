/* This File is part of 'fsck' copyright S.N. Henson
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# include "fs.h"
# include "fsck.h"
# include "global.h"
# include "io.h"
# include "common.h"
# include "xhdi.h"


# define PROGNAME	"fsck.minix"
# define VERSION	"0.22"
# define DATE		"2001-01-18"

static void
usage (void)
{
	printf ("Filesystem consistency checker. Copyright S.N. Henson 1992,1993,1994.\n");
	printf (PROGNAME " version " VERSION " (fnaumann@freemint.de, " DATE ")\n");
	
	printf ("Usage:
	" PROGNAME " [options] device ...
device: something like k: or F
useful options:
    -f            : force filesystem check even if it's clean
    -y   [or -Y]  : answer all questions with 'yes'
    -n   [or -N]  : don't repair anything, just report errors
    -p            : non-interactive mode, only do non-destructive repair
    -d # [or -D #]: use # as directory increment
    -i #,#,#,...  : print out pathnames of the # inode list
    -s            : print out summary after check
    -S            : same as -s but some more infos
");
	
	exit (1);
}

int
main (int argc, char **argv)
{
	int force = 0;
	int rw = 1;
	int c;
	
	if (init_XHDI ())
		fatal ("XHDI interface not found");
	
	opterr = 0;
	while ((c = getopt (argc, argv, "fpyYnNd:D:sSRi:z:e")) != EOF)
	{
		switch (c)
		{
			case 'f':
			{
				force = 1;
				break;
			}
			case 'y':
			case 'Y':
			{
				ally = 1;
				break;
			}
			case 'n':
			case 'N':
			{
				rw = 0;
				alln = 1;
				break;
			}
			case 'd':
			case 'D':
			{
				incr = atoi (optarg);
				if ((incr < 1) || (incr > 16) || NPOW2 (incr))
				{
					fprintf (stderr, "Invalid Increment Value\n");
					exit (1);
				}
				break;
			}
			case 's':
			{
				info = 1;
				break;
			}
			case 'S':
			{
				info = 2;
				break;
			}
			case 'R':
			{
				badroot = 1;
				break;
			}
			case 'i':
			{
				comma_parse (optarg, &inums);
				break;
			}
			case 'z':
			{
				comma_parse (optarg, &zinums);
				break;
			}
# if notyet
			case 'u':
			{
				ul = malloc (sizeof (llist));
				if (!ul) fatal ("Out of Memory");
				ul->member = (long) strdup (optarg);
				ul->next = unlist;
				unlist = ul;
				break;
			}
# endif
			case 'p':
			{
				preen = 1;
				break;
			}
			case 'e':
			{
				no_size = 1;
				break;
			}
			case '?':
			{
				usage ();
				break;
			}
		}
	}
	
	if ((argc - optind != 1) || opterr)
		usage	();
	
	if (preen && (ally || alln))
	{
		fprintf (stderr, "-p option cannot be mixed with -n or -y\n");
		exit (1);
	}
	
	if (preen)
		ally = 1;
	
	if (badroot && !incr)
	{
		fprintf (stderr, "'-R' option needs '-d'\n");
		exit (1);
	}
	
	drvnam = strdup (argv [optind]);
	
	if (init_device (argv [optind], rw))
	{
		fprintf (stderr, "Can't Open Device %s\n", argv[optind]);
		exit (1);
	}
	
	read_tables ();
	
	if ((Super->s_state & MINIX_VALID_FS)
		&& !(Super->s_state & MINIX_ERROR_FS))
	{
		printf ("%s: filesystem clean, ", argv[optind]);
		if (!force)
		{
			printf ("skip check.\n");
			goto end;
		}
		else
			printf ("check forced.\n");
	}
	else
		printf ("%s: filesystem not cleanly unmounted, check forced.\n", argv[optind]);
	
	if (version)
		do_fsck2 ();
	else
		do_fsck1 ();
	
	if (!(Super->s_state & MINIX_VALID_FS)
		|| (Super->s_state & MINIX_ERROR_FS))
	{
		char mod_bak;
		
		printf ("%s: mark filesystem clean!\n", argv[optind]);
		
		Super->s_state |= MINIX_VALID_FS;
		Super->s_state &= ~MINIX_ERROR_FS;
		
		mod_bak = modified;
		write_zone (1, Super);
		modified = mod_bak;
	}
	
end:
	showinfo ();
	close_device ();
	
	exit (0);
}
