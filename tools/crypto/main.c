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
 * Changes:
 * 
 * 2000-05-02:
 * 
 * - inital version
 * 
 */

# include <ctype.h>
# include <fcntl.h>
# include <getopt.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# include <mintbind.h>

# include "crypt.h"
# include "io.h"

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	10
# define VER_STATUS	


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
	"Copyright (c) " __DATE__ " by Frank Naumann.\n" \
	"All rights reserved."


static void
version (void)
{
	puts (MSG_PROGRAM);
	puts (MSG_GREET);
}

static void
help (void)
{
	puts (MSG_PROGRAM);
	puts (MSG_GREET);
	puts ("");
	
	
}

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
	
	va_start (args, msg);
	vprintf (msg, args);
	va_end (args);
	
	exit (1);
}


static char *modes[] =
{
	"safe",
	"robust",
	"fast",
	NULL
};
# define SAFE		0
# define ROBUST		1
# define FAST		2

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
static int noninteractive = 0;
static int auto_passphrase_set = 1;

static int drv = -1;
static int mode = SAFE;
static int action = ENCIPHER;
static int cipher = BLOWFISH;

static char *passphrase = NULL;
static char *oldpassphrase = NULL;

static void *key = NULL;
static void *oldkey = NULL;

static char *buf = NULL;
static int32_t bufsize = 1024L * 128;

static FILE *log = NULL;
static int save_file = 0;

static int64_t start_pos = 0;
static int64_t end_pos = 0;


static void
verify_encrypt_key (void)
{
	char buf1 [1024];
	char buf2 [1024];
	char c;
	
	printf ("\n");
	printf ("WARNING: THIS WILL TOTALLY ENCRYPT ANY DATA ON %c:\n", 'A'+drv);
	printf ("\n");
	printf ("         IF YOU FORGET THE PASSPHRASE YOU CAN NEVER ACCESS\n");
	printf ("         OR DECRYPT THIS PARTITION AND ALL DATA ON IT!\n");
	printf ("\n");
	printf ("Please enter the passphrase for encryption.\n");
restart:
	strcpy (buf1, getpass("passphrase [8 chars minimum]: "));
	strcpy (buf2, getpass("passphrase verification: "));
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
		safe_exit ("out of memory, abort.");
	
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
	char c;
	
	printf ("\n");
	printf ("WARNING: THIS WILL TOTALLY DECRYPT ANY DATA ON %c:\n", 'A'+drv);
	printf ("\n");
	printf ("         IF YOU USE THE WRONG PASSPHRASE OR DECRYPT A NOT\n");
	printf ("         ENCRYPTED PARTITION YOU'LL DESTROY ALL YOUR DATA!\n");
	printf ("\n");
	printf ("Please enter the passphrase for decryption.\n");
restart:
	strcpy (buf1, getpass("passphrase [8 chars minimum]: "));
	strcpy (buf2, getpass("passphrase verification: "));
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
		safe_exit ("out of memory, abort.");
	
	strcpy (passphrase, buf1);
	
	return;
}

static void
verify_change_key (void)
{
	printf ("Aborted\n");
	exit (0);
}


static void doit (void);

int
main (int argc, char **argv)
{
	myname = argv [0];
	
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
			{ 0,		0, 0, 0		}
		};
		
		int option_index = 0;
		int c;
		
		
		c = getopt_long (argc, argv, "vhqtb:m:a:c:", long_options, &option_index);
		
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
				
				help ();
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
				help ();
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
				int32_t r = strtol (optarg, NULL, 0);
				
				if (r < 16)
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
					help ();
					exit (1);
				}
				
				break;
			}
			case '?':
				printf ("unknown option `%s'\n", optarg);
			default:
				help ();
				exit (1);
		}
	}
	
	/* parse partition specification */
	if (optind < argc)
	{
		char *s = argv[optind];
		
		if (s[1] == ':')
			drv = toupper (s[0]) - 'A';
		
		if (drv < 0 || drv > 31)
			drv = -1;
	}
	
	/* verify it */
	if (drv == -1)
		safe_exit ("invalid or missing drive specification, abort.\n");
	
	
	/*
	 * initialize IO module
	 */
	
	if (io_init ())
		safe_exit ("initialization failed, abort.\n");
	
	
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
	
	doit ();
	
	
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

static void
doit (void)
{
	int32_t p_secsize;
	int64_t size;
	int64_t todo;
	
	int32_t ret;
	
	int fh;
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
	
	/* get size of the partition */
	size = io_seek (fh, SEEK_END, 0);
	if (size < 0)
		safe_exit ("failed to determine partition size, abort.\n");
	
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
	ret = io_seek (fh, SEEK_SET, start_pos);
	if (ret < 0)
		safe_exit ("io_seek failed, abort.\n");
	
	if (mode == ROBUST)
	{
		/* open error recovery file */
		ret = open ("crypto.rec", O_CREAT|O_WRONLY|O_TRUNC, 0600);
		if (ret < 0)
		{
			perror ("open (\"crypto.rec\")");
			safe_exit ("abort.\n");
		}
		
		save_file = ret;
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
		
		ret = io_read (fh, buf, left);
		if (ret < 0)
			emergency_exit ("io_read failed, abort.\n");

		if (mode == ROBUST)
		{
			lseek (save_file, 0, SEEK_SET);
			write (save_file, buf, left);
			fsync (save_file);
		}
		
		do_cipher (recno, p_secsize);
	    
		if (!simulate)
		{
			ret = io_seek (fh, SEEK_CUR, -left);
			if (ret < 0)
				emergency_exit ("io_seek failed, abort.\n");
			
			ret = io_write (fh, buf, left);
			if (ret < 0)
				emergency_exit ("io_write failed, abort.\n");
		}
		
		todo -= left;
		
		/* feedback */
		if (!quiet)
		{
			int64_t mb_size = size / 1024 / 1024;
			int64_t mb_done = (size - todo) / 1024 / 1024;
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
		close (save_file);
	
	io_close (fh);
}
