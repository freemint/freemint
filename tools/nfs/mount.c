/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : mount.c
 *        do an nfs mount
 */

#include <mintbind.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <support.h>
#include <limits.h>

#include <sys/stat.h>

#include "mount.h"
#include "mntent.h"
#include "nfsmnt.h"


#define LOCKED  "\\etc\\mtab~"

#define VERBOSE

#define UPDATE_MOUNT   0
#define UPDATE_UNMOUNT 1


char *commandname = "mount";

/* common option values, set here the default values */

int verbose = 0;
int readonly = 0;
int unmount = 0;
int without_mtab = 0;
int fake_mtab = 0;

char optionstr[MNTMAXSTR+1] = "";
char noopt = 1;

char whatmsg[] =
"@(#)mount for nfs, Copyright Ulrich Kuehn, " __DATE__;


static void
usage (void)
{
	printf ("%s usage:\n", commandname);
	printf ("  mount [ -rvnf ] { -o option } remotedir localdir\n");
	printf ("  mount -u [ vn ] localdir\n");
}

static void
parse_option (char *s)
{
	char *p;

	while (*s)
	{
		if (*s == ',')
			s += 1;
		
		if (!noopt)
			strcat (optionstr, ",");
		
		if (!strncmp(s, "ro", 2))
		{
			strcat (optionstr, "ro");
			readonly = 1;
			p = s+2;
		}
		else if (!strncmp (s, "rw", 2))
		{
			strcat (optionstr, "rw");
			readonly = 0;
			p = s+2;
		}
		else if (!strncmp (s, "nosuid", 6))
		{
			strcat (optionstr, "nosuid");
			nosuid = 1;
			p = s+6;
		}
		else if (!strncmp (s, "suid", 4))
		{
			strcat (optionstr, "suid");
			nosuid = 0;
			p = s+4;
		}
		else if (!strncmp (s, "rsize=", 6))
		{
			rsize = strtol (&s[6], &p, 10);
			strcat (optionstr, "rsize=");
			_ltoa (rsize, &optionstr[strlen (optionstr)], 10);
		}
		else if (!strncmp (s, "wsize=", 6))
		{
			wsize = strtol (&s[6], &p, 10);
			strcat (optionstr, "wsize=");
			_ltoa (wsize, &optionstr[strlen (optionstr)], 10);
		}
		else if (!strncmp (s, "timeo=", 6))
		{
			timeo = strtol (&s[6], &p, 10);
			strcat (optionstr, "timeo=");
			_ltoa (timeo, &optionstr[strlen (optionstr)], 10);
		}
		else if (!strncmp (s, "retrans=", 8))
		{
			retrans = strtol(&s[8], &p, 10);
			strcat (optionstr, "retrans=");
			_ltoa (retrans, &optionstr[strlen (optionstr)], 10);
		}
		else if (!strncmp (s, "port=", 5))
		{
			port = strtol (&s[5], &p, 10);
			strcat (optionstr, "port=");
			_ltoa (port, &optionstr[strlen (optionstr)], 10);
		}
		else if (!strncmp (s, "acregmin=", 9))
		{
			/* not supported yet */
			strtol (&s[9], &p, 10);
		}
		else if (!strncmp (s, "acregmax=", 9))
		{
			/* not supported yet */
			strtol (&s[9], &p, 10);
		}
		else if (!strncmp (s, "acdirmin=", 9))
		{
			/* not supported yet */
			strtol (&s[9], &p, 10);
		}
		else if (!strncmp (s, "actimeo=", 8))
		{
			actimeo = strtol (&s[8], &p, 10);
			strcat (optionstr, "actimeo=");
			_ltoa (actimeo, &optionstr[strlen (optionstr)], 10);
		}
		else
			fprintf (stderr, "unknown option '%s', ignoring it.\n", s);
		
		noopt = 0;
		s = p;
	}
}

static long
update_mtab (int mode, char *filesys, char *dir, char *type, char *opt, int freq, int pass)
{
	FILE *fp;
	
	if (mode == UPDATE_MOUNT)
	{
		struct mntent mnt;
		
		fp = setmntent (MOUNTED, "a+");
		if (!fp)
		{
			fprintf (stderr, "%s: could not update %s\n", commandname, MOUNTED);
			return 1;
		}
		
		mnt.mnt_fsname = filesys;
		mnt.mnt_dir = dir;
		mnt.mnt_type = type;
		mnt.mnt_opts = opt;
		mnt.mnt_freq = freq;
		mnt.mnt_passno = pass;
		
		addmntent (fp, &mnt);
		endmntent (fp);
		
		return 0;
	}
	else if (mode == UPDATE_UNMOUNT)
	{
		FILE *fl;
		struct mntent *mnt;
		
		fp = setmntent (MOUNTED, "r");
		if (!fp)
		{
			fprintf (stderr, "%s: could not update mount table\n", commandname);
			return 1;
		}
		
		fl = setmntent (LOCKED, "a+");
		if (!fl)
		{
			fprintf (stderr, "%s: could not update mount table\n", commandname);
			return 1;
		}
		
		while ((mnt = getmntent (fp)) != NULL)
		{
			if ( (strcmp (mnt->mnt_dir, dir) != 0) &&
			     (strcmp (mnt->mnt_fsname, dir) != 0) )
				addmntent (fl, mnt);
		}
		
		endmntent (fp);
		endmntent (fl);
		
		rename (LOCKED, MOUNTED);
		
		return 0;
	}
	
	return 1;
}

/* Convert a name into an absolute name that can be understood by the
 * system.
 */
static char *
convert_localname (char *s, char *d)
{
	if (!s || !d)
		return NULL;
	
	/* convert into MiNTs native representation */
	unx2dos (s, d);
	
	/* get rid of the leading drive letter */
	if (d[1] == ':')
	{
		if (tolower (d[0]) == 'u')
			d += 2;
		else
		{
			/* here we have something like d:\foo, which we convert into
			 * \d\foo, so we do not need more space.
			 */
			d[1] = tolower (d[0]);
			d[0] = '\\';
		}
	}
	
	if (d[0] != '\\')
	{
		fprintf (stderr, "%s: cannot convert '%s' into absolute name.\n", commandname, s);
		exit (1);
	}
	
	return d;
}

int
main (int argc, char *argv[])
{
	long r;
	int n;
	char *mounted, *dir;
	char path[PATH_MAX+1];
	
	
	/* switch to the real root */
	Dsetdrv ('u'-'a');
	if (argv[0] && *argv[0])
		commandname = argv[0];
	
	
	/* parse options */
	mounted = NULL;
	dir = NULL;
	for (n = 1;  n < argc;  n++)
	{
		if (argv[n][0] == '-')
		{
			switch (argv[n][1])
			{
				case 'u':
				case 'U':
					unmount = 1;
					break;
				case 'r':
				case 'R':
					readonly = 1;
					strcpy (optionstr, "ro");
					noopt = 0;
					break;
				case 'o':
				case 'O':
					n++;
					if (n > argc)
					{
						usage ();
						return 1;
					}
					parse_option (argv[n]);
					break;
				case 'v':
					verbose = 1;
					break;
				case 'n':
					without_mtab = 1;
					break;
				case 'f':
					fake_mtab = 1;
				default:
					usage();
					return 1;
			}
		}
		else  /* not an option, it must be the real arguments */
		{
			if (unmount)
			{
				if (n < argc-1)
				{
					usage ();
					return 1;
				}
				dir = argv[n];
			}
			else
			{
				if (n != argc-2)
				{
					usage ();
					return 1;
				}
				mounted = argv[n];
				dir = argv[n+1];
				n += 1;
			}
		}
	}
	
	if (optionstr[0] == '\0')
	{
		/* no options set */
		strcpy (optionstr, "defaults");
	}
	
	if (!dir)
	{
		usage();
		exit(1);
	}
	
	dir = convert_localname (dir, path);
	
	r = 0;
	if (!unmount)
	{
		if (!mounted || !dir)
		{
			usage ();
			return 1;
		}
		
		if (!fake_mtab)
			r = do_nfs_mount (mounted, dir);
		else
			r = 0;
		
		if (r != 0)
		{
			fprintf (stderr, "%s: could not do NFS mount\n", commandname);
			return 1;
		}
		
		if (verbose)
			printf ("mounted %s on %s, type %s (%s)\n",
			                  mounted, dir, "nfs", optionstr);
		
		/* update the mount table file accordingly */
		if (!without_mtab)
			return update_mtab (UPDATE_MOUNT, mounted, dir, "nfs", optionstr, 0, 0);
		else
			return 0;
	}
	else
	{
		FILE *fp;
		struct mntent *mnt;
		int which;
		
		if (!dir)
		{
			usage ();
			return 1;
		}
		
		fp = setmntent (MOUNTED, "r");
		if (!fp)
		{
			fprintf (stderr, "%s: cannot open mount table\n", commandname);
			return 1;
		}
		
		which = 0;
		while ((mnt = getmntent (fp)) != NULL)
		{
			if (!strcmp (mnt->mnt_dir, dir))
			{
				which = 2;
				break;
			}
			if (!strcmp (mnt->mnt_fsname, dir))
			{
				which = 1;
				break;
			}
		}
		
		if (mnt)
		{
			r = do_nfs_unmount (mnt->mnt_fsname, mnt->mnt_dir);
			if (r != 0)
			{
				fprintf (stderr, "%s: could not do NFS unmount\n", commandname);
				return 1;
			}
		}
		
		fclose (fp);
		
		/* update the mount table file */
		if (!without_mtab)
			return update_mtab (UPDATE_UNMOUNT, NULL, dir, NULL, NULL, 0, 0);
		else
			return 0;
	}
	
	return r;
}
