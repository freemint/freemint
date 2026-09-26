/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
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
#include <mntent.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#include "mount.h"
#include "nfsmnt3.h"


#define LOCKED  "\\etc\\mtab~"

#define VERBOSE

#define UPDATE_MOUNT   0
#define UPDATE_UNMOUNT 1


char *commandname = "mount_nfs3";

/* common option values, set here the default values */

int verbose = 0;
int readonly = 0;
int unmount = 0;
int without_mtab = 0;
int fake_mtab = 0;

char optionstr[64+1] = "";
char noopt = 1;

char whatmsg[] =
"@(#)mount for nfs v3, derived from the nfs v2 mount tool, " __DATE__;


/* Compare two path names, treating '/' and '\\' as the same separator and
 * any run of separators as one.
 *
 * \etc\mtab holds the directory with its backslashes doubled, so
 * getmntent() hands back "\\nfs3\\falcon" where our own name is
 * "\nfs3\falcon". A plain strcmp() misses that: the unmount then found
 * no entry and never called Dcntl() -- silently, reporting success --
 * and the table grew one stale line per mount because the filter below
 * never matched either.
 */
# define IS_SEP(c)	((c) == '/' || (c) == '\\')

static int
same_path (const char *a, const char *b)
{
	while (*a && *b)
	{
		if (IS_SEP (*a) && IS_SEP (*b))
		{
			while (IS_SEP (a[1]))
				a++;
			while (IS_SEP (b[1]))
				b++;
		}
		else if (*a != *b)
			return 0;

		a++;
		b++;
	}

	/* A trailing separator does not make two paths different -- one
	 * gets typed as often as not.
	 */
	while (IS_SEP (*a))
		a++;
	while (IS_SEP (*b))
		b++;

	return *a == '\0' && *b == '\0';
}

static void
usage (void)
{
	printf ("%s usage:\n", commandname);
	printf ("  %s [ -rvnf ] { -o option } host:/remotedir localdir\n", commandname);
	printf ("  %s -u [ -vn ] localdir\n", commandname);
	printf ("\n");
	printf ("  localdir has to be below u:\\nfs3, which is where the\n");
	printf ("  nfs3.xfs driver installs itself.\n");
}

/* Append to optionstr without running past its end. The original code
 * used unchecked strcat()/_ltoa() into a 65 byte buffer.
 */
static void
add_opt (const char *text)
{
	size_t have = strlen (optionstr);
	size_t room = sizeof (optionstr) - 1 - have;

	if (room == 0)
		return;

	strncpy (optionstr + have, text, room);
	optionstr[sizeof (optionstr) - 1] = '\0';
}

static void
add_opt_num (const char *name, long value)
{
	char buf[32];

	_ltoa (value, buf, 10);
	add_opt (name);
	add_opt (buf);
}

/* Length of the option token starting at s, i.e. up to the next comma
 * or the end of the string.
 */
static size_t
opt_len (const char *s)
{
	const char *comma = strchr (s, ',');

	return comma ? (size_t) (comma - s) : strlen (s);
}

static void
parse_option (char *s)
{
	char *p;

	while (*s)
	{
		if (*s == ',')
			s += 1;

		if (!*s)
			break;

		/* Make sure p is always defined: the original fell through
		 * the final else without setting it and then did s = p.
		 */
		p = s + opt_len (s);

		if (!noopt)
			add_opt (",");

		if (!strncmp (s, "ro", 2))
		{
			add_opt ("ro");
			readonly = 1;
			p = s + 2;
		}
		else if (!strncmp (s, "rw", 2))
		{
			add_opt ("rw");
			readonly = 0;
			p = s + 2;
		}
		else if (!strncmp (s, "nosuid", 6))
		{
			add_opt ("nosuid");
			nosuid = 1;
			p = s + 6;
		}
		else if (!strncmp (s, "suid", 4))
		{
			add_opt ("suid");
			nosuid = 0;
			p = s + 4;
		}
		else if (!strncmp (s, "proto=tcp", 9))
		{
			add_opt ("proto=tcp");
			transport = 1;
			p = s + 9;
		}
		else if (!strncmp (s, "proto=udp", 9))
		{
			add_opt ("proto=udp");
			transport = 0;
			p = s + 9;
		}
		else if (!strncmp (s, "tcp", 3))
		{
			add_opt ("tcp");
			transport = 1;
			p = s + 3;
		}
		else if (!strncmp (s, "udp", 3))
		{
			add_opt ("udp");
			transport = 0;
			p = s + 3;
		}
		else if (!strncmp (s, "rsize=", 6))
		{
			rsize = strtol (&s[6], &p, 10);
			add_opt_num ("rsize=", rsize);
		}
		else if (!strncmp (s, "wsize=", 6))
		{
			wsize = strtol (&s[6], &p, 10);
			add_opt_num ("wsize=", wsize);
		}
		else if (!strncmp (s, "timeo=", 6))
		{
			timeo = strtol (&s[6], &p, 10);
			add_opt_num ("timeo=", timeo);
		}
		else if (!strncmp (s, "retrans=", 8))
		{
			retrans = strtol (&s[8], &p, 10);
			add_opt_num ("retrans=", retrans);
		}
		else if (!strncmp (s, "port=", 5))
		{
			port = strtol (&s[5], &p, 10);
			add_opt_num ("port=", port);
		}
		else if (!strncmp (s, "noac", 4))
		{
			add_opt ("noac");
			noac = 1;
			p = s + 4;
		}
		else if (!strncmp (s, "soft", 4))
		{
			add_opt ("soft");
			soft = 1;
			p = s + 4;
		}
		else if (!strncmp (s, "intr", 4))
		{
			add_opt ("intr");
			intr = 1;
			p = s + 4;
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
			add_opt_num ("actimeo=", actimeo);
		}
		else if (!strncmp (s, "vers=", 5) || !strncmp (s, "nfsvers=", 8))
		{
			/* accepted and ignored: this tool only speaks v3 */
			long v = strtol (strchr (s, '=') + 1, &p, 10);

			if (v != 3)
				fprintf (stderr, "%s: only NFS version 3 is "
					 "supported, ignoring vers=%ld\n",
					 commandname, v);
		}
		else
		{
			fprintf (stderr, "%s: unknown option '%.*s', ignoring it.\n",
				 commandname, (int) opt_len (s), s);
		}

		noopt = 0;
		s = p;
	}
}

static long
update_mtab (int mode, char *filesys, char *dir, char *type,
	     char *opt, int freq, int pass)
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
		int kept, dropped;

		fp = setmntent (MOUNTED, "r");
		if (!fp)
		{
			fprintf (stderr, "%s: could not update mount table\n", commandname);
			return 1;
		}

		/* "a+" appended to whatever a previous, failed run had left
		 * behind, so the table could only ever grow. Truncate.
		 */
		fl = setmntent (LOCKED, "w");
		if (!fl)
		{
			fprintf (stderr, "%s: could not write %s\n",
				 commandname, LOCKED);
			endmntent (fp);
			return 1;
		}

		kept = 0;
		dropped = 0;

		while ((mnt = getmntent (fp)) != NULL)
		{
			if (same_path (mnt->mnt_dir, dir)
			    || same_path (mnt->mnt_fsname, dir))
			{
				dropped++;
				continue;
			}

			addmntent (fl, mnt);
			kept++;
		}

		endmntent (fp);
		endmntent (fl);

		/* GEMDOS refuses to rename onto an existing file, and the
		 * return value used to be ignored -- so the rewritten table
		 * was thrown away and the stale lines stayed forever.
		 */
		if (remove (MOUNTED) != 0)
		{
			fprintf (stderr, "%s: cannot remove %s\n",
				 commandname, MOUNTED);
			remove (LOCKED);
			return 1;
		}

		if (rename (LOCKED, MOUNTED) != 0)
		{
			fprintf (stderr, "%s: cannot rename %s to %s\n",
				 commandname, LOCKED, MOUNTED);
			return 1;
		}

		if (verbose)
			printf ("%s: %s: %d entries removed, %d kept\n",
				commandname, MOUNTED, dropped, kept);

		return 0;
	}

	return 1;
}

/* Convert a name into an absolute name that can be understood by the
 * system.
 */
static char *
convert_localname (const char *s, char *d)
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

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

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
			                  mounted, dir, fstype, optionstr);

		/* update the mount table file accordingly */
		if (!without_mtab)
			return update_mtab (UPDATE_MOUNT, mounted, dir,
					    (char *) fstype, optionstr, 0, 0);
		else
			return 0;
	}
	else
	{
		FILE *fp;
		struct mntent *mnt = NULL;
		char remote[PATH_MAX+1];
		int have_remote = 0;

		if (!dir)
		{
			usage ();
			return 1;
		}

		/* The mount table is consulted only to find out which server
		 * to notify. Not finding an entry must NOT stop the unmount:
		 * the kernel is the authority on what is mounted, and a mount
		 * made before \etc was writable -- from mint.cnf, say -- has
		 * no entry at all. The old code skipped the Dcntl() in that
		 * case and still reported success.
		 */
		fp = setmntent (MOUNTED, "r");
		if (fp)
		{
			while ((mnt = getmntent (fp)) != NULL)
			{
				if (same_path (mnt->mnt_dir, dir)
				    || same_path (mnt->mnt_fsname, dir))
					break;
			}

			if (mnt)
			{
				strncpy (remote, mnt->mnt_fsname, sizeof (remote) - 1);
				remote[sizeof (remote) - 1] = '\0';
				have_remote = 1;
			}

			endmntent (fp);
		}

		if (verbose && !have_remote)
			printf ("%s: %s is not listed in %s, "
				"unmounting it anyway\n",
				commandname, dir, MOUNTED);

		r = do_nfs_unmount (have_remote ? remote : NULL, dir);
		if (r != 0)
		{
			fprintf (stderr, "%s: could not unmount %s\n",
				 commandname, dir);
			return 1;
		}

		if (verbose)
			printf ("unmounted %s\n", dir);

		/* update the mount table file */
		if (!without_mtab)
			return update_mtab (UPDATE_UNMOUNT, NULL, dir, NULL, NULL, 0, 0);
		else
			return 0;
	}

	return r;
}
