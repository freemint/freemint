/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Started:      2000-05-02
 * 
 */

# define _GNU_SOURCE

# include <ctype.h>
# include <fcntl.h>
# include <getopt.h>
# include <limits.h>
# include <signal.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>

# include <mintbind.h>

# include "crypt.h"
# include "io.h"

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	22
# define VER_STATUS	


/*
 * config values
 */
# define SAVEFILE_MASK	"drv_%c.sav"
# define LOGFILE_MASK	"drv_%c.log"

char SAVEFILE [128];
char LOGFILE [128];


/*
 * messages
 */

# define str(x)		_stringify (x)
# define _stringify(x)	#x

# define MSG_VERSION	\
	"v" str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS)

# define MSG_PROGRAM	\
	"Initialization tool for FreeMiNT filesystem crypto layer " \
	MSG_VERSION

# define MSG_GREET	\
	"Copyright (c) " __DATE__ " by Frank Naumann.\n"


static char *modes[] =
{
	"fast",
	"robust",
	NULL
};
# define FAST		0
# define ROBUST		1

static char *actions[] =
{
	"encipher",
	"decipher",
	"changekey",
	NULL
};
# define ENCIPHER	0
# define DECIPHER	1
# define CHANGEKEY	2

static char *ciphers[] =
{
	"blowfish",
	NULL
};
# define BLOWFISH	0


char *myname = NULL;

static int simulate = 0;
static int quiet = 0;
static int auto_passphrase_set = 1;

static int drv = -1;
static int mode = ROBUST;
static int action = ENCIPHER;
static int cipher = BLOWFISH;
static char *restart = NULL;

static char *passphrase = NULL;
static char *oldpassphrase = NULL;

static void *key = NULL;
static void *oldkey = NULL;

static char *buf = NULL;
static int32_t bufsize = 1024L * 128;

static FILE *logfile = NULL;
static int save_file = -1;
static int fh = -1;

static int64_t start_pos = 0;
static int64_t end_pos = 0;



static void
safe_exit (char *msg, ...)
{
	va_list args;
	
	va_start (args, msg);
	vprintf (msg, args);
	va_end (args);
	
	exit (1);
}

static void
emergency_exit (char *msg, ...)
{
	va_list args;
	
	if (save_file >= 0)
		close (save_file);
	
	if (logfile)
	{
		fflush (logfile);
		
		fprintf (logfile, "emergency exit\n");
		fflush (logfile);
		
		fclose (logfile);
	}
	
	if (fh >= 0)
		io_close (fh);
	
	va_start (args, msg);
	vprintf (msg, args);
	va_end (args);
	
	fputs ("\n", stdout);
	fputs ("WARNING: Filesystem is likely to be in an inconsistent state!\n", stdout);
	fputs ("You must first repair your filesystem.\n", stdout);
	
	exit (1);
}



static void
verify_encrypt_key (void)
{
	char buf1 [1024];
	char buf2 [1024];
	char c;
	
	printf ("\n");
	printf ("WARNING: THIS WILL TOTALLY ENCRYPT ANY DATA ON %c:\n", DriveToLetter(drv));
	printf ("\n");
	printf ("         IF YOU FORGET THE PASSPHRASE YOU CAN NEVER ACCESS\n");
	printf ("         OR DECRYPT THIS PARTITION AND ALL DATA ON IT!\n");
	printf ("\n");
	printf ("Please enter the passphrase for encryption.\n");
restart:
	strcpy (buf1, getpass ("passphrase [8 chars minimum]: "));
	strcpy (buf2, getpass ("passphrase verification: "));
	printf ("\n");
	
	if (strcmp (buf1, buf2))
	{
		printf ("Passphrases don't match!\nTry again.\n");
		printf ("\n");
		goto restart;
	}
	
	if (strlen (buf1) < 8)
	{
		printf ("Passphrase too short!\nTry again.\n");
		printf ("\n");
		goto restart;
	}
	
	passphrase = malloc (strlen (buf1) + 1);
	if (!passphrase)
		safe_exit ("out of memory, abort.\n");
	
	strcpy (passphrase, buf1);
	return;
	
	printf ("--`%s'--`%s'--\n", buf1, buf2);
	fflush (stdout);
	
	if (c == 'y' || c == 'Y')
		return;
	
	printf ("Aborted\n");
	exit (0);
}

static void
verify_decrypt_key (void)
{
	char buf1 [1024];
	char buf2 [1024];
	
	printf ("\n");
	printf ("WARNING: THIS WILL TOTALLY DECRYPT ANY DATA ON %c:\n", DriveToLetter(drv));
	printf ("\n");
	printf ("         IF YOU USE THE WRONG PASSPHRASE OR DECRYPT A NOT\n");
	printf ("         ENCRYPTED PARTITION YOU'LL DESTROY ALL YOUR DATA!\n");
	printf ("\n");
	printf ("Please enter the passphrase for decryption.\n");
restart:
	strcpy (buf1, getpass ("passphrase [8 chars minimum]: "));
	strcpy (buf2, getpass ("passphrase verification: "));
	printf ("\n");
	
	if (strcmp (buf1, buf2))
	{
		printf ("Passphrases dosn't match!\nTry again.\n");
		printf ("\n");
		goto restart;
	}
	
	if (strlen (buf1) < 8)
	{
		printf ("Passphrase too short!\nTry again.\n");
		printf ("\n");
		goto restart;
	}
	
	passphrase = malloc (strlen (buf1) + 1);
	if (!passphrase)
		safe_exit ("out of memory, abort.\n");
	
	strcpy (passphrase, buf1);
	return;
}

static void
verify_change_key (void)
{
	printf ("Aborted\n");
	exit (0);
}



static void
version (void)
{
	puts (MSG_PROGRAM);
	puts (MSG_GREET);
}

static void
usage (void)
{
	puts (MSG_PROGRAM);
	puts (MSG_GREET);
	
	printf (
"Usage: \
	%s [options] device ... \
device: something like d: or L: \
useful options: \
    -a# [or --action #]:  select action (encipher, decipher, changekey) \
                          default is encipher \
    -b# [or --buffer #]:  specify buffer size in kb \
                          default is %ikb \
    -c# [or --cipher #]:  select cipher algorithm (blowfish) \
                          default is blowfish \
    -m# [or --mode #]:    select mode (robust, fast) \
                          default is robust \
    -h  [or --help]:      print this message \
    -q  [or --quiet]:     be quiet \
    -r# [or --restart #]: restart interrupted session, parameter \
                          is the generated *.sav file in robust mode \
    -t  [or --simulate]:  testing mode, simulate action without any write \
    -v  [or --version]:   print version \
dangerous options, \
for professionals only: \
    -e# [or --end #]:     set end position for action \
                          default is partition end \
    -s# [or --start #]:   set start position for action \
                          default is partition start (512) \
", basename (myname), bufsize / 1024);
	
	fflush (stdout);
}


static void default_sig_handler (int signum);
static void sigint_handler (int signum);
static void doit (const char *rescuefile);

int
main (int argc, char **argv)
{
	myname = argv [0];
	
	
	/*
	 * initialize IO module
	 */
	
	if (io_init ())
		safe_exit ("initialization failed, abort.\n");
	
	
	/*
	 * parse arguments
	 */
	
	for (;;)
	{
		static struct option long_options[] =
		{
			{ "version",	0, 0, 'v'	},
			{ "help",	0, 0, 'h'	},
			{ "quiet",	0, 0, 'q'	},
			{ "simulate",	0, 0, 't'	},
			{ "buffer",	1, 0, 'b'	},
			{ "mode",	1, 0, 'm'	},
			{ "action",	1, 0, 'a'	},
			{ "cipher",	1, 0, 'c'	},
			{ "start",	1, 0, 's'	},
			{ "end",	1, 0, 'e'	},
			{ "restart",	1, 0, 'r'	},
			{ 0,		0, 0, 0		}
		};
		
		int option_index = 0;
		int c;
		
		
		c = getopt_long (argc, argv, "vhqtb:m:a:c:s:e:r:", long_options, &option_index);
		
		/* end of the options */
		if (c == -1)
			break;
		
		switch (c)
		{
			case 0:
			{
				/* If this option set a flag, do nothing else now.  */
				if (long_options[option_index].flag != 0)
					break;
				
				usage ();
				exit (1);
				
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			}
			case 'v':
			{
				version ();
				exit (0);
			}
			case 'h':
			{
				usage ();
				exit (0);
			}
			case 'q':
			{
				quiet = 1;
				break;
			}
			case 't':
			{
				simulate = 1;
				break;
			}
			case 'b':
			{
				char *check;
				int32_t r = strtol (optarg, &check, 0);
				
				if ((optarg == check) || (r < 16))
					printf ("incorrect buffersize, using default value.\n");
				else
					bufsize = 1024L * r;
				
				break;
			}
			case 'm':
			case 'a':
			case 'c':
			{
				int flag = 1;
				char **table = NULL;
				char *option = NULL;
				int *i = NULL;
				
				switch (c)
				{
					case 'm':
						table = modes;
						option = "mode";
						i = &mode;
						break;
					case 'a':
						table = actions;
						option = "action";
						i = &action;
						break;
					case 'c':
						table = ciphers;
						option = "cipher algorithm";
						i = &cipher;
						break;
				}
				
				for (*i = 0; table [*i]; (*i)++)
				{
					if (strcmp (optarg, table [*i]) == 0)
					{
						flag = 0;
						break;
					}
				}
				
				if (flag)
				{
					printf ("unknown %s `%s'\n\n", option, optarg);
					usage ();
					exit (1);
				}
				
				break;
			}
			case 's':
			case 'e':
			{
				char *check;
				int64_t r = strtoll (optarg, &check, 0);
				
				if ((optarg == check)
					|| (r == LONG_LONG_MAX)
					|| (r == LONG_LONG_MIN))
				{
					safe_exit ("can't parse start/end position, abort.\n");
				}
				else
				{
					if (c == 's')
						start_pos = r;
					else
						end_pos = r;
				}
				
				break;
			}
			case 'r':
			{
				restart = optarg;
				break;
			}
			case '?':
				printf ("unknown option `%s'\n", optarg);
			
			default:
				usage ();
				exit (1);
		}
	}
	
	/* parse partition specification */
	if (optind < argc)
	{
		char *s = argv[optind];
		
		if (s[1] == ':')
			drv = DriveFromLetter(s[0]);
		
		if (drv < 0 || drv > 31)
			drv = -1;
	}
	
	/* verify it */
	if (drv == -1)
	{
		printf ("invalid or missing drive specification, abort.\n");
		usage ();
		exit (1);
	}
	
	
	/*
	 * verify user permission
	 */
	
	if (action == ENCIPHER)
		verify_encrypt_key ();
	else if (action == DECIPHER)
		verify_decrypt_key ();
	else if (action == CHANGEKEY)
		verify_change_key ();
	else
		safe_exit ("`action' with incorrect value, internal error.");
	
	
	key = make_key (passphrase, cipher);
	oldkey = oldpassphrase ? make_key (oldpassphrase, cipher) : NULL;
	
	if (!key || (oldpassphrase && !oldkey))
		safe_exit ("out of memory, abort.\n");
	
	
	buf = malloc (bufsize);
	if (!buf)
		safe_exit ("out of memory, abort.\n");
	
	
	/*
	 * run the main routine
	 */
	
	{
		__sighandler_t sigint;
		__sighandler_t s1, s2, s3, s4, s5, s6, s7, s8, s9,
				s10, s11, s12, s13, s14, s15, s16, s17, s18, s19;
		sigset_t newmask, oldmask;
		
		/* special handler */
		sigint = signal (SIGINT, sigint_handler);
		
		/* default handler */
		s1 = signal (SIGHUP, default_sig_handler);
		s2 = signal (SIGQUIT, default_sig_handler);
		s3 = signal (SIGILL, default_sig_handler);
		s4 = signal (SIGTRAP, default_sig_handler);
		s5 = signal (SIGABRT, default_sig_handler);
		s6 = signal (SIGPRIV, default_sig_handler);
		s7 = signal (SIGFPE, default_sig_handler);
		s8 = signal (SIGBUS, default_sig_handler);
		s9 = signal (SIGSEGV, default_sig_handler);
		s10 = signal (SIGSYS, default_sig_handler);
		s11 = signal (SIGPIPE, default_sig_handler);
		s12 = signal (SIGALRM, default_sig_handler);
		s13 = signal (SIGTERM, default_sig_handler);
		s14 = signal (SIGXCPU, default_sig_handler);
		s15 = signal (SIGXFSZ, default_sig_handler);
		s16 = signal (SIGVTALRM, default_sig_handler);
		s17 = signal (SIGPROF, default_sig_handler);
		s18 = signal (SIGUSR1, default_sig_handler);
		s19 = signal (SIGUSR2, default_sig_handler);
		
		newmask = 
			sigmask (SIGTSTP) |
			sigmask (SIGTTIN) |
			sigmask (SIGTTOU);
		
		/* set new signal mask */
		sigprocmask (SIG_SETMASK, &newmask, &oldmask);
		
		doit (restart);
		
		/* restore signal mask */
		sigprocmask (SIG_SETMASK, &oldmask, NULL);
		
		/* restore handler */
		signal (SIGINT, sigint);
		
		signal (SIGHUP, s1);
		signal (SIGQUIT, s2);
		signal (SIGILL, s3);
		signal (SIGTRAP, s4);
		signal (SIGABRT, s5);
		signal (SIGPRIV, s6);
		signal (SIGFPE, s7);
		signal (SIGBUS, s8);
		signal (SIGSEGV, s9);
		signal (SIGSYS, s10);
		signal (SIGPIPE, s11);
		signal (SIGALRM, s12);
		signal (SIGTERM, s13);
		signal (SIGXCPU, s14);
		signal (SIGXFSZ, s15);
		signal (SIGVTALRM, s16);
		signal (SIGPROF, s17);
		signal (SIGUSR1, s18);
		signal (SIGUSR2, s19);
	}
	
	
	if (auto_passphrase_set)
	{
		if (action == DECIPHER)
			Dsetkey (0, drv, "", 0);
		else
			Dsetkey (0, drv, passphrase, 0);
	}
	
	
	/*
	 * done
	 */
	
	if (!quiet)
		printf ("Leave ok.\n");
	
	exit (0);
}

static void
default_sig_handler (int signum)
{
	if (logfile)
		fprintf (logfile, "got signal %i\n", signum);
	
	emergency_exit ("got fatal signal %i, terminating.\n", signum);
}

static void
sigint_handler (int signum)
{
	char c;
	
	printf ("\nAre you sure to interrupt this session (y/n)? ");
	scanf ("%c", &c);
	
	if (c == 'y' || c == 'Y')
		default_sig_handler (signum);
}

static void
logfile_writeheader (FILE *out)
{
	time_t t = time (NULL);
	
	fprintf (out, "# \n# %s, %s# \n", basename (myname), ctime (&t));
}

# ifndef min
# define min(a,b) ((a < b) ? a : b)
# endif

static void
do_cipher (int32_t recno, int32_t p_secsize)
{
	switch (action)
	{
		case ENCIPHER:
			encrypt_block (key, buf, bufsize, recno, p_secsize);
			break;
		case DECIPHER:
			decrypt_block (key, buf, bufsize, recno, p_secsize);
			break;
		case CHANGEKEY:
			decrypt_block (oldkey, buf, bufsize, recno, p_secsize);
			encrypt_block (key, buf, bufsize, recno, p_secsize);
			break;
	}	
}

typedef struct rescue RESCUE;
struct rescue
{
	int32_t		magic;		/* magic number */
# define RESCUE_MAGIC	0x34c7868f
	int32_t		ver;		/* 2 for now */
	int32_t		this_size;	/* size of the struct */
	time_t		time;
	
	int		action;		/* action */
	int		cipher;		/* cipher algorithm */
	int64_t		dev;
	int32_t		p_secsize;
	int32_t		p_start;
	int32_t		p_size;
	
	int64_t		pos;
	int32_t		size;
};

static void
doit (const char *rescuefile)
{
	char *action_pre = ((action == ENCIPHER) ? "en" : ((action == DECIPHER) ? "de" : "re"));
	RESCUE rescue;
	
	int32_t p_secsize;
	int32_t p_start;
	int32_t p_size;
	int64_t size;
	int64_t todo;
	
	int32_t ret;
	int64_t ret64;
	
	int oldperc = -1;
	
	
	/* device initializations
	 */
	
	fh = io_open (drv);
	if (fh < 0)
		safe_exit ("failed to open device, abort.\n");
	
	/* get physical sector size */
	ret = io_ioctrl (fh, 1, &p_secsize);
	if (ret < 0)
		safe_exit ("failed to determine physical sector size, abort.\n");
	
	/* get partition start sector */
	ret = io_ioctrl (fh, 3, &p_start);
	if (ret < 0)
		safe_exit ("failed to determine start sector, abort.\n");
	
	/* get partition size */
	ret = io_ioctrl (fh, 2, &p_size);
	if (ret < 0)
		safe_exit ("failed to determine partition size, abort.\n");
	
	/* get size of the partition */
	size = io_seek (fh, SEEK_END, 0);
	if (size < 0)
		safe_exit ("failed to determine partition size, abort.\n");
	
	/* do restart things */
	if (restart)
	{
		/* first read rescue file informations */
		
		ret = open (rescuefile, O_RDONLY);
		if (ret < 0)
		{
			perror ("open (...)");
			safe_exit ("abort.\n");
		}
		
		save_file = ret;
		
		ret = read (save_file, &rescue, sizeof (rescue));
		if (ret != sizeof (rescue))
		{
			perror ("read (rescue)");
			safe_exit ("abort.\n");
		}
		
		if (rescue.magic != RESCUE_MAGIC)
			safe_exit ("not a rescue file, abort.\n");
		if (rescue.ver != 2)
			safe_exit ("rescue file version not supported, abort.\n");
		if (rescue.this_size != sizeof (rescue))
			safe_exit ("rescue file corrupted, abort.\n");
		
		if (rescue.dev != drv)
			safe_exit ("drv number doesn't match, abort.\n");
		if (rescue.p_secsize != p_secsize)
			safe_exit ("physical sector size doesn't match, abort.\n");
		if (rescue.p_start != p_start)
			safe_exit ("partition start doesn't match, abort.\n");
		if (rescue.p_size != p_size)
			safe_exit ("partition size doesn't match, abort.\n");
		
		if (bufsize != rescue.size)
		{
			buf = realloc (buf, rescue.size);
			if (!buf)
				safe_exit ("out of memory, abort.\n");
			
			bufsize = rescue.size;
		}
		
		ret = read (save_file, buf, rescue.size);
		if (ret != rescue.size)
		{
			perror ("read (buf)");
			safe_exit ("abort.\n");
		}
		
		close (save_file);
		save_file = -1;
		
		
		/* now write the buffer back to the filesystem */
		
		{
			char c;
			
			printf ("Restart interrupted session from %s", ctime (&rescue.time));
			printf ("on %c: [%i], starting offset %qi.\n\n", DriveToLetter(drv), drv, rescue.pos);
			printf ("With a different passphrase you destroy your data!\n");
			printf ("Are you ABSOLUTELY SURE you want to do this? (y/n) ");
			scanf ("%c", &c);
			
			if (c != 'y' && c != 'Y')
				safe_exit ("user abort.\n");
		}
		
		ret64 = io_seek (fh, SEEK_SET, rescue.pos);
		if (ret64 < 0 || ret64 != rescue.pos)
			safe_exit ("io_seek failed, abort.\n");
		
		ret = io_write (fh, buf, rescue.size);
		if (ret < 0 || ret != rescue.size)
			emergency_exit ("io_write failed, abort.\n");
		
		action = rescue.action;
		cipher = rescue.cipher;
		start_pos = rescue.pos;
	}
	
	/* verify or initialize start position */
	if (start_pos)
	{
		if ((start_pos < p_secsize) || (start_pos > size))
			safe_exit ("incorrect `start' value, abort.\n");
		
		if ((start_pos % p_secsize) != 0)
			safe_exit ("`start' value not on a sector boundary, abort.\n");
	}
	else
		start_pos = p_secsize;
	
	/* verify or initialize end position */
	if (end_pos)
	{
		if ((end_pos < start_pos) || (end_pos > size))
			safe_exit ("incorrect `end' value, abort.\n");
		
		if ((end_pos % p_secsize) != 0)
			safe_exit ("`end' value not on a sector boundary, abort.\n");
	}
	else
		end_pos = size;
	
	/* seek to start position */
	ret64 = io_seek (fh, SEEK_SET, start_pos);
	if ((ret64 < 0) || (ret64 != start_pos))
		safe_exit ("io_seek failed, abort.\n");
	
	if (mode == ROBUST)
	{
		char c = DriveToLetter(drv);
		c = tolower(c);
		sprintf (SAVEFILE, SAVEFILE_MASK, c);
		sprintf (LOGFILE, LOGFILE_MASK, c);
		
		/* open error recovery file */
		ret = open (SAVEFILE, O_CREAT|O_WRONLY|O_TRUNC, 0600);
		if (ret < 0)
		{
			perror ("open (sav)");
			safe_exit ("abort.\n");
		}
		
		save_file = ret;
		
		/* prepare rescue data */
		rescue.magic = RESCUE_MAGIC;
		rescue.ver = 2;
		rescue.this_size = sizeof (rescue);
		rescue.time = time (NULL);
		
		rescue.action = action;
		rescue.cipher = cipher;
		rescue.dev = drv;
		rescue.p_secsize = p_secsize;
		rescue.p_start = p_start;
		rescue.p_size = p_size;
		
		/* open logfile */
		logfile = fopen (LOGFILE, "w");
		if (!logfile)
		{
			perror ("fopen (logfile)");
			safe_exit ("abort.\n");
		}
		
		logfile_writeheader (logfile);
		
		fprintf (logfile, "# %scrypting drive %c: [%i]\n", action_pre, DriveToLetter(drv), drv);
		fprintf (logfile, "# using cipher %s\n", ciphers[cipher]);
		fprintf (logfile, "# \n");
		fprintf (logfile, "# -- partition info --\n");
		fprintf (logfile, "# sectorsize: %i\n", p_secsize);
		fprintf (logfile, "# startsector: %i\n", p_start);
		fprintf (logfile, "# sectors: %i\n", p_size);
		fprintf (logfile, "# \n");
		fprintf (logfile, "# run from offset %qi to %qi\n", start_pos, end_pos);
		fprintf (logfile, "# \n\n");
		fflush (logfile);
	}
	
	/* transformation loop */
	todo = end_pos - start_pos;
	while (todo)
	{
		int32_t left = min (todo, bufsize);
		int64_t pos;
		int32_t recno;
		
		pos = io_seek (fh, SEEK_CUR, 0);
		recno = pos / p_secsize;
		
		if (pos < 0)
			emergency_exit ("io_seek failed, abort.\n");
		
		if (mode == ROBUST)
			fprintf (logfile, "off %10qi  size %7i  - reading ", pos, left);
		
		ret = io_read (fh, buf, left);
		if (ret < 0)
			emergency_exit ("io_read failed, abort.\n");
		
		if (mode == ROBUST)
		{
			fputs ("ok - ", logfile);
			fputs (action_pre, logfile);
			fputs ("cipher ", logfile);
			
			rescue.pos = pos;
			rescue.size = left;
			
			lseek (save_file, 0, SEEK_SET);
			write (save_file, &rescue, sizeof (rescue));
			write (save_file, buf, left);
			fsync (save_file);
		}
		
		do_cipher (recno, p_secsize);
		
		if (mode == ROBUST)
			fputs ("ok", logfile);
		
		if (!simulate)
		{
			ret64 = io_seek (fh, SEEK_CUR, -left);
			if (ret64 < 0)
				emergency_exit ("io_seek failed, abort.\n");
			
			if (mode == ROBUST)
				fputs (" - write ", logfile);
			
			ret = io_write (fh, buf, left);
			if (ret < 0)
				emergency_exit ("io_write failed, abort.\n");
			
			if (mode == ROBUST)
				fputs ("ok", logfile);
		}
		
		if (mode == ROBUST)
		{
			fputs ("\n", logfile);
			/* fflush (logfile); */
		}
		
		todo -= left;
		
		/* feedback */
		if (!quiet)
		{
			int64_t mb_size = end_pos / 1024 / 1024;
			int64_t mb_done = (end_pos - todo) / 1024 / 1024;
			int perc = mb_done * 100 / mb_size;
			
			if (perc != oldperc)
			{
				oldperc = perc;
				
				printf ("%5Li MB / %5Li MB  -> %3i%% done\n", mb_done, mb_size, perc);
				fflush (stdout);
			}
		}
	}
	
	if (mode == ROBUST)
	{
		fputs ("\n", logfile);
		fclose (logfile);
		
		close (save_file);
		unlink (SAVEFILE);
	}
	
	io_close (fh);
}
