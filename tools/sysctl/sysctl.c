/* $Id$ */

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../../sys/mint/sysctl.h"
#include "fparseln.h"


struct ctlname topname[] = CTL_NAMES;
struct ctlname kernname[] = CTL_KERN_NAMES;
struct ctlname hwname[] = CTL_HW_NAMES;
struct ctlname debugname[CTL_DEBUG_MAXID];

char names[BUFSIZ];

struct list {
	struct	ctlname *list;
	int	size;
};
struct list toplist = { topname, CTL_MAXID };
struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ kernname, KERN_MAXID },	/* CTL_KERN */
	{ hwname, HW_MAXID },		/* CTL_HW */
	{ 0, 0 },			/* CTL_MACHDEP */
	{ 0, CTL_DEBUG_MAXID },		/* CTL_DEBUG */
	{ 0, 0},
};

/*
 * Variables requiring special processing.
 */
#define	BOOTTIME	0x00000002

/*
 * A dummy type for limits, which requires special parsing
 */
#define CTLTYPE_LIMIT	((~0x1) << 31)

int main __P((int, char *[]));

static void listall __P((const char *, struct list *));
static void parse __P((char *, int));
static void debuginit __P((void));
static int findname __P((char *, char *, char **, struct list *));
static void usage __P((void));

int Aflag, aflag, nflag, wflag;
char *myname;

int
main(int argc, char *argv[])
{
	char *fn = NULL;
	int ch, lvl1;

	myname = argv[0];

	while ((ch = getopt(argc, argv, "Aaf:nw")) != -1) {
		switch (ch) {

		case 'A':
			Aflag = 1;
			break;

		case 'a':
			aflag = 1;
			break;

		case 'f':
			fn = optarg;
			wflag = 1;
			break;

		case 'n':
			nflag = 1;
			break;

		case 'w':
			wflag = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (Aflag || aflag) {
		debuginit();
		for (lvl1 = 1; lvl1 < CTL_MAXID; lvl1++)
			listall(topname[lvl1].ctl_name, &secondlevel[lvl1]);
		return 0;
	}

	if (fn) {
		FILE *fp;
		char *l;
		
		fp = fopen(fn, "r");
		if (fp == NULL) {
			err(1, "%s", fn);
		} else {
			for (; (l = fparseln(fp, NULL, NULL, NULL, 0)) != NULL;
			    free(l)) {
				if (*l)
					parse(l, 1);
			}
			fclose(fp);
		}
	} else {
		if (argc == 0)
			usage();
		while (argc-- > 0)
			parse(*argv++, 1);
	}
	return 0;
}

/*
 * List all variables known to the system.
 */
static void
listall(const char *prefix, struct list *lp)
{
	int lvl2;
	char *cp, name[BUFSIZ];

	if (lp->list == 0)
		return;
	strcpy(name, prefix);
	cp = &name[strlen(name)];
	*cp++ = '.';
	for (lvl2 = 0; lvl2 < lp->size; lvl2++) {
		if (lp->list[lvl2].ctl_name == 0)
			continue;
		strcpy(cp, lp->list[lvl2].ctl_name);
		parse(name, Aflag);
	}
}

/*
 * Parse a name into a MIB entry.
 * Lookup and print out the MIB entry if it exists.
 * Set a new value if requested.
 */
static void
parse(char *string, int flags)
{
	int indx, type, len;
	int special = 0;
	void *newval = 0;
	int intval, newsize = 0;
	quad_t quadval;
	size_t size;
	struct list *lp;
	int mib[CTL_MAXNAME];
	char *cp, *bufp, buf[BUFSIZ];

	bufp = buf;
	snprintf(buf, BUFSIZ, "%s", string);
	if ((cp = strchr(string, '=')) != NULL) {
		if (!wflag)
			errx(2, "Must specify -w to set variables");
		*strchr(buf, '=') = '\0';
		*cp++ = '\0';
		while (isspace((unsigned char) *cp))
			cp++;
		newval = cp;
		newsize = strlen(cp);
	}
	if ((indx = findname(string, "top", &bufp, &toplist)) == -1)
		return;
	mib[0] = indx;
	if (indx == CTL_DEBUG)
		debuginit();
//	if (mib[0] == CTL_PROC) {
//		type = CTLTYPE_NODE;
//		len = 1;
//	} else
	{
		lp = &secondlevel[indx];
		if (lp->list == 0) {
			warnx("Class `%s' is not implemented",
			topname[indx].ctl_name);
			return;
		}
		if (bufp == NULL) {
			listall(topname[indx].ctl_name, lp);
			return;
		}
		if ((indx = findname(string, "second", &bufp, lp)) == -1)
			return;
		mib[1] = indx;
		type = lp->list[indx].ctl_type;
		len = 2;
	}
	switch (mib[0]) {

	case CTL_KERN:
		switch (mib[1]) {
		case KERN_BOOTTIME:
			special |= BOOTTIME;
			break;
		}
		break;

	case CTL_HW:
		break;


	case CTL_DEBUG:
		mib[2] = CTL_DEBUG_VALUE;
		len = 3;
		break;

//	case CTL_VENDOR:
//	case CTL_USER:
//	case CTL_DDB:
		break;
	default:
		warnx("Illegal top level value: %d", mib[0]);
		return;
	
	}
	if (bufp) {
		warnx("Name %s in %s is unknown", bufp, string);
		return;
	}
	if (newsize > 0) {
		switch (type) {
		case CTLTYPE_INT:
			intval = atoi(newval);
			newval = &intval;
			newsize = sizeof intval;
			break;

#if 0
		case CTLTYPE_LIMIT:
			if (strcmp(newval, "unlimited") == 0) {
				quadval = RLIM_INFINITY;
				newval = &quadval;
				newsize = sizeof quadval;
				break;
			}
#endif
			/* FALLTHROUGH */
		case CTLTYPE_QUAD:
			sscanf(newval, "%lld", (long long *)&quadval);
			newval = &quadval;
			newsize = sizeof quadval;
			break;
		}
	}
	size = BUFSIZ;
	if (sysctl(mib, len, buf, &size, newsize ? newval : 0, newsize) == -1) {
		if (flags == 0)
			return;
		switch (errno) {
		case EOPNOTSUPP:
			warnx("The value of %s is not available", string);
			return;
		case ENOTDIR:
			warnx("The specification of %s is incomplete",
			    string);
			return;
		case ENOMEM:
			warnx("The type %s is unknown to this program",
			    string);
			return;
		default:
			warn("sysctl() for %s failed", string);
			return;
		}
	}
	if (special & BOOTTIME) {
		struct timeval *btp = (struct timeval *)buf;
		time_t boottime;

		if (!nflag) {
			boottime = btp->tv_sec;
			/* ctime() provides the trailing newline */
			printf("%s = %s", string, ctime(&boottime));
		} else
			printf("%ld\n", (long) btp->tv_sec);
		return;
	}
		
	switch (type) {
	case CTLTYPE_INT:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%d\n", *(int *)buf);
		} else {
			if (!nflag)
				printf("%s: %d -> ", string, *(int *)buf);
			printf("%d\n", *(int *)newval);
		}
		return;

	case CTLTYPE_STRING:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%s\n", buf);
		} else {
			if (!nflag)
				printf("%s: %s -> ", string, buf);
			printf("%s\n", (char *) newval);
		}
		return;

	case CTLTYPE_LIMIT:
#define PRINTF_LIMIT(lim) { \
if ((lim) == RLIM_INFINITY) \
	printf("unlimited");\
else \
	printf("%lld", (long long)(lim)); \
}

		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
//			PRINTF_LIMIT((long long)(*(quad_t *)buf));
		} else {
			if (!nflag) {
				printf("%s: ", string);
//				PRINTF_LIMIT((long long)(*(quad_t *)buf));
				printf(" -> ");
			}
//			PRINTF_LIMIT((long long)(*(quad_t *)newval));
		}
		printf("\n");
		return;
#undef PRINTF_LIMIT

	case CTLTYPE_QUAD:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%lld\n", (long long)(*(quad_t *)buf));
		} else {
			if (!nflag)
				printf("%s: %lld -> ", string,
				    (long long)(*(quad_t *)buf));
			printf("%lld\n", (long long)(*(quad_t *)newval));
		}
		return;

	case CTLTYPE_STRUCT:
		warnx("%s: unknown structure returned", string);
		return;

	default:
	case CTLTYPE_NODE:
		warnx("%s: unknown type returned", string);
		return;
	}
}

/*
 * Initialize the set of debugging names
 */
static void
debuginit(void)
{
	int mib[3], loc, i;
	size_t size;

	if (secondlevel[CTL_DEBUG].list != 0)
		return;
	secondlevel[CTL_DEBUG].list = debugname;
	mib[0] = CTL_DEBUG;
	mib[2] = CTL_DEBUG_NAME;
	for (loc = 0, i = 0; i < CTL_DEBUG_MAXID; i++) {
		mib[1] = i;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		debugname[i].ctl_name = &names[loc];
		debugname[i].ctl_type = CTLTYPE_INT;
		loc += size;
	}
}

/*
 * Scan a list of names searching for a particular name.
 */
static int
findname(char *string, char *level, char **bufp, struct list *namelist)
{
	char *name;
	int i;

	if (namelist->list == 0 || (name = strsep(bufp, ".")) == NULL) {
		warnx("%s: incomplete specification", string);
		return (-1);
	}
	for (i = 0; i < namelist->size; i++)
		if (namelist->list[i].ctl_name != NULL &&
		    strcmp(name, namelist->list[i].ctl_name) == 0)
			break;
	if (i == namelist->size) {
		warnx("%s level name %s in %s is invalid",
		    level, name, string);
		return (-1);
	}
	return (i);
}

static void
usage(void)
{
	const char *progname = myname; /* getprogname(); */

	(void)fprintf(stderr, "Usage:\t%s %s\n\t%s %s\n\t%s %s\n\t%s %s\n\t%s %s\n",
	    progname, "[-n] variable ...", 
	    progname, "[-n] -w variable=value ...",
	    progname, "[-n] -a",
	    progname, "[-n] -A",
	    progname, "[-n] -f file");
	exit(1);
}
