/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File : mntent.c
 *        access files containing file system information
 *
 */


# include <stdio.h>
# include <stdlib.h>
# include <support.h>
# include <ctype.h>
# include <errno.h>

# include <sys/file.h>

# include "mntent.h"



# define MAX_LINE_LEN 80


static struct mntent the_mntent;
static char the_mntbuf[MAX_LINE_LEN+2];


FILE *
setmntent (char *name, char *type)
{
	FILE *fp;
	int i;
	
	/* open the file */
	fp = fopen (name, type);
	if (!fp)
		return NULL;
	
	/* set up some static buffers etc. */
	the_mntent.mnt_fsname = NULL;
	the_mntent.mnt_dir = NULL;
	the_mntent.mnt_type = NULL;
	the_mntent.mnt_opts = NULL;
	the_mntent.mnt_freq = 0;
	the_mntent.mnt_passno = 0;
	
	for (i = 0;  i < MAX_LINE_LEN+2;  i++)
		the_mntbuf[i] = '\0';
	
	return fp;
}

int
endmntent (FILE *fp)
{
	fclose (fp);
	return 1;   /* for compatibility */
}

struct mntent *
getmntent (FILE *fp)
{
	char *p;
	
read_again:
	p = fgets (the_mntbuf, MAX_LINE_LEN, fp);
	if (p == NULL)
		return NULL;
	
	/* strip off tailing line separator */
	p = the_mntbuf;
	while (*p && (*p != '\r') && (*p != '\n'))
		p++;
	*p = '\0';
	
	/* check for comments */
	if (the_mntbuf[0] == '#')   /* if that line is a comment, do it again */
	{
		goto read_again;
	}
	
	/* now scan the line and set the mntent struct accordingly */
	p = the_mntbuf;
	while (*p && isspace(*p))   /* skip leading white spaces */
		p++;
	the_mntent.mnt_fsname = p;
	while (*p && !isspace(*p))
		p++;
	if (*p)
		*p++ = '\0';
	
	while (*p && isspace(*p))
		p++;
	the_mntent.mnt_dir = p;
	while (*p && !isspace(*p))
		p++;
	if (*p)
		*p++ = '\0';
	
	while (*p && isspace(*p))
		p++;
	the_mntent.mnt_type = p;
	while (*p && !isspace(*p))
		p++;
	if (*p)
		*p++ = '\0';
	
	while (*p && isspace(*p))
		p++;
	the_mntent.mnt_opts = p;
	while (*p && !isspace(*p))
		p++;
	if (*p)
		*p++ = '\0';
	
	while (*p && isspace(*p))
		p++;
	the_mntent.mnt_freq = (int)strtol(p, &p, 10);
	if (*p)
		p++;
	
	while (*p && isspace(*p))
		p++;
	the_mntent.mnt_passno = (int)strtol(p, &p, 10);
	
	return &the_mntent;
}

int
addmntent(FILE *fp, struct mntent *mnt)
{
	char line[MAX_LINE_LEN+3], numbuf[8];
	char *p, *s;
	long n, r;
	
	/* wait for a lock on that file */
	do {
		r = flock (fileno (fp), LOCK_EX|LOCK_NB);
	}
	while ((r < 0) && (errno == ELOCKED));
	
	if (r != 0)
		return 1;
	
	fseek (fp, 0, SEEK_END);   /* go to end of file */
	n = 0;
	p = line;
	if (mnt->mnt_fsname)
	{
		for (s = mnt->mnt_fsname; (n < MAX_LINE_LEN-1) && *s;  n++)
			*p++ = *s++;
		*p++ = ' ';
		n++;
	}
	
	if (mnt->mnt_dir)
	{
		for (s = mnt->mnt_dir; (n < MAX_LINE_LEN-1) && *s;  n++)
			*p++ = *s++;
		*p++ = ' ';
		n++;
	}
	
	if (mnt->mnt_type)
	{
		for (s = mnt->mnt_type; (n < MAX_LINE_LEN-1) && *s;  n++)
			*p++ = *s++;
		*p++ = ' ';
		n++;
	}
	
	if (mnt->mnt_opts)
	{
		for (s = mnt->mnt_opts; (n < MAX_LINE_LEN-1) && *s;  n++)
			*p++ = *s++;
		*p++ = ' ';
		n++;
	}
	
	s = _ltoa (mnt->mnt_freq, numbuf, 10);
	for ( ; (n < MAX_LINE_LEN-1) && *s;  n++)
		*p++ = *s++;
	*p++ = ' ';
	n++;
	
	s = _ltoa (mnt->mnt_passno, numbuf, 10);
	for ( ; (n < MAX_LINE_LEN-1) && *s;  n++)
		*p++ = *s++;
	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';
	
	r = fwrite (line, 1, n+2, fp);
	
	flock (fileno (fp), LOCK_UN);   /* unlock file */
	
	if (r == n+2)
		return 0;
	else
		return 1;
}

char *
hasmntopt (struct mntent *mnt, char *opt)
{
	return NULL;
}
