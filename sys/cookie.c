/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * This file is free software; you can redistribute it and/or modify
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
 */

/*
 * cookie jar handling routines
 * ----------------------------
 * The "cookie jar" is an area of memory reserved by TOS for TSR's and utility
 * programs. The idea is that you put a cookie in the jar to notify people of
 * available services. The BIOS uses the cookie jar in TOS 1.6 and higher. For
 * earlier versions of TOS, the jar is always empty (unless someone added a
 * cookie before us; POOLFIX does, for example). MiNT establishes an entirely
 * new cookie jar (with the old cookies copied over) and frees it on exit (with
 * notable exception of MiNT 1.15 and up, which never exits). That's because
 * TSR's run under MiNT will no longer be accessible after MiNT exits.
 * MiNT also puts a cookie in the jar, with tag field 'MiNT' (of course)
 * and with the major version of MiNT in the high byte of the low word,
 * and the minor version in the low byte.
 *
 */

# include "cookie.h"
# include "global.h"

# include "arch/mprot.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"
# include "mint/rsvf.h"

# include "biosfs.h"	/* rsvf */
# include "memory.h"	/* get_region, attach_region */

# ifdef JAR_PRIVATE
# include "arch/user_things.h"
# include "proc.h"
# endif

# ifdef OLDTOSFS
# include "tosfs.h"
# endif

/****************************************************************************/
/* BEGIN global data */

/* TOS and MiNT cookie jars, respectively.
 */

static struct cookie *oldcookie = NULL;
static struct cookie *newcookie;

# ifdef JAR_PRIVATE
# define MASTERCOOKIES	128
static struct cookie master_jar[MASTERCOOKIES];
# else
/* memory region that hold the cookie jar
 */
static MEMREGION *newjar_region;
# endif

/* cookies to skip on MINT cookie jar
 */
static const long skiplist [] =
{
	COOKIE__CPU,
	COOKIE__FPU,
	COOKIE_RSVF,	/* hsmodem TSR */
	COOKIE_NF,	/* natfeats of emulators are not for userspace */
# ifndef OLDTOSFS
	COOKIE__FLK,
# endif
	0
};

/* END global data */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization */

void
init_cookies (void)
{
	struct cookie *cookie;
	unsigned short i = 0;
	unsigned short ncookies = 0;

	long ncsize;

	cookie = oldcookie = *CJAR;
	if (cookie)
	{
		while (cookie->tag)
		{
# ifdef OLDTOSFS
			if (cookie->tag == COOKIE__FLK)
				flk = 1;
# endif
			cookie++;
			ncookies++;
		}
	}

# ifdef JAR_PRIVATE
	/* Processes use own copies of the cookie jar located in their
	 * memory. Jar is no more global, thus the master copy of it
	 * can be located in kernel's BSS.
	 */
	ncsize = MASTERCOOKIES * sizeof(struct cookie);
	if (ncookies > MASTERCOOKIES)
		ncookies = MASTERCOOKIES;
	newcookie = master_jar;
# else
	ncsize = MIN (cookie->value, 240);	/* avoid too big tag values */
	if (ncookies > ncsize)
		ncsize = ncookies;

	/* We allocate the cookie jar in global memory so anybody can read
	 * it or write it. This code allocates at least 16 more cookies,
	 * then rounds up to a QUANTUM boundary (that's what ROUND does).
	 * Probably, nobody will have to allocate another cookie jar :-)
	 */
	ncsize = (ncsize + 16) * sizeof(struct cookie);
	ncsize = ROUND (ncsize);
	newjar_region = get_region (core, ncsize, PROT_G);
	newcookie = (struct cookie *) attach_region (rootproc, newjar_region);
# endif

	/* set the hardware detected CPU and FPU rather
	 * than trust the TOS
	 */
	newcookie[i].tag = COOKIE__CPU;
	newcookie[i].value = mcpu;
	i++;

	newcookie[i].tag = COOKIE__FPU;
	newcookie[i].value = fputype;
	i++;

	/* copy the old cookies to the new jar */
	cookie = oldcookie;

	while (ncookies)
	{
		int copy = 1;
		int j;

		/* but don't copy RSVF, MiNTs /dev is for real...
		 * (if you want to know whats in there use ls :)
		 * don't copy _CPU & _FPU too, we already installed own
		 * _CPU and _FPU cookies
		 * don't copy other ugly cookies (XHDI, SCSIDRV)
		 */
		for (j = 0; skiplist [j]; j++)
		{
			if (skiplist [j] == cookie->tag)
			{
				copy = 0;
				break;
			}
		}

		if (copy)
			newcookie[i++] = *cookie;

		cookie++;
		ncookies--;
	}

	/* install MiNT cookie
	 */
	newcookie[i].tag   = COOKIE_MiNT;
	newcookie[i].value = (MINT_MAJ_VERSION << 8) | MINT_MIN_VERSION;
	i++;

	/* install _FLK cookie to indicate that file locking works
	 */
# ifdef OLDTOSFS
	if (!flk)
	{
		newcookie[i].tag   = COOKIE__FLK;
		newcookie[i].value = 0x00000100;	/* This is version number, 1.0 :-) */
		i++;
	}
# else
	newcookie[i].tag   = COOKIE__FLK;
	newcookie[i].value = 0x00000100;
	i++;
# endif

	/* jr: install PMMU cookie if memory protection is used
	 */
	if (!no_mem_prot)
	{
		newcookie[i].tag   = COOKIE_PMMU;
		newcookie[i].value = 0;
		i++;
	}

	/* the last cookie should have a 0 tag, and a value indicating
	 * the number of slots, total
	 */
	newcookie[i].tag   = 0;
	newcookie[i].value = ncsize / sizeof(struct cookie);

	/* setup new COOKIE Jar */
# ifdef JAR_PRIVATE
	kernel_things.user_jar_p = newcookie;
# endif
	*CJAR = newcookie;
}

/* END initialization */
/****************************************************************************/

/****************************************************************************/
/* BEGIN cookie manipulation */

long
get_toscookie (ulong tag, ulong *val)
{
	struct cookie *cookie = oldcookie;

	if (!cookie)
		/* not initialized yet */
		cookie = *CJAR;

	if (cookie)
	{
		while (cookie->tag)
		{
			if (cookie->tag == tag)
			{
				*val = cookie->value;
				return 0;
			}

			cookie++;
		}
	}

	*val = 0;
	return 1;
}

long
set_toscookie (ulong tag, ulong val)
{
	struct cookie *cookie = oldcookie;

	if (!cookie)
		/* not initialized yet */
		cookie = *CJAR;

	if (cookie)
	{
		while (cookie->tag)
		{
			if (cookie->tag == tag)
			{
				cookie->value = val;
				return 0;
			}

			cookie++;
		}
	}

	return 1;
}

/* find cookie cookiep->tag and return its value in
 * cookiep->value, return E_OK (0)
 * return ERROR (-1) if not found
 */

long
get_cookie (struct cookie *cj, ulong tag, ulong *ret)
{
# ifdef JAR_PRIVATE
	struct user_things *ut;
	struct cookie *cjar;
# else
	struct cookie *cjar = *CJAR;
# endif
	ushort slotnum = 0;		/* number of already taken slots */
# ifdef DEBUG_INFO
	char asc [5];
	*(long *) asc = tag;
	asc [4] = '\0';
# endif

	DEBUG (("get_cookie(): tag=%08lx (%s) ret=%08lx", tag, asc, ret));

# ifdef JAR_PRIVATE
	ut = curproc->p_mem->tp_ptr;
	cjar = ut->user_jar_p;
# endif

	if (cj)
		cjar = cj;

	/* If tag == 0, we return the value of NULL slot
	 */
	if (tag == 0)
	{
		DEBUG (("get_cookie(): searching for NULL slot"));
		while (cjar->tag)
			cjar++;

		/* If ret is a zero, the value is returned in the d0, otherwise
		 * the ret is considered a pointer where the value should be put to
		 */
		if (ret != 0)
		{
			DEBUG (("exit get_cookie(): NULL value written to %08x", ret));
			*ret = cjar->value;
			return E_OK;
		}
		else
		{
			DEBUG (("exit get_cookie(): NULL value returned in d0"));
			return cjar->value;
		}

	}

	/* if the high word of tag is zero, this is the slot number
	 * to look at. The first slot is number 1.
	 */
	if ((tag & 0xffff0000UL) == 0)
	{
		DEBUG (("get_cookie(): looking for entry number %d", tag));
		while (cjar->tag)
		{
			cjar++;
			slotnum++;
		}

		slotnum++;
		if (tag > slotnum)
		{
			DEBUG (("get_cookie(): entry number too big"));
			return EINVAL;
		}

		cjar = *CJAR;
		slotnum = 1;
		while (slotnum != tag)
		{
			slotnum++;
			cjar++;
		}

		if (ret)
		{
			DEBUG (("get_cookie(): tag returned at %08lx", ret));
			*ret = cjar->tag;
			return E_OK;
		}
		else
		{
			DEBUG (("get_cookie(): tag returned in d0"));
			return cjar->tag;
		}
	}

	/* all other values of tag mean tag id to search for */

	TRACE (("get_cookie(): searching for tag %08lx", tag));
	while (cjar->tag)
	{
		if (cjar->tag == tag)
		{
			if (ret)
			{
				DEBUG (("get_cookie(): value returned at %08lx", ret));
				*ret = cjar->value;
				return E_OK;
			}
			else
			{
				DEBUG (("get_cookie(): value returned in d0"));
				return cjar->value;
			}
		}

		cjar++;
	}

	DEBUG (("get_cookie(): lookup failed"));
	return EERROR;
}

/* add cookie cookiep->tag to cookie list or change it's value
 * if already existing
 */

long
set_cookie (struct cookie *cj, ulong tag, ulong val)
{
	ushort n = 0;
# ifdef JAR_PRIVATE
	struct user_things *ut;
	struct cookie *cjar;
# else
	struct cookie *cjar = *CJAR;
# endif

# ifdef JAR_PRIVATE
	ut = curproc->p_mem->tp_ptr;
	cjar = ut->user_jar_p;
# endif

	if (cj)
		cjar = cj;

	/* 0x0000xxxx feature of GETCOOKIE may be confusing, so
	 * prevent users from using slotnumber HERE :)
	 */
	DEBUG (("entering set_cookie(): tag=%08x val=%08x", tag, val));
	if (	((tag & 0xff000000UL) == 0) ||
		((tag & 0x00ff0000UL) == 0) ||
		((tag & 0x0000ff00UL) == 0) ||
		((tag & 0x000000ffUL) == 0))
	{
		DEBUG (("set_cookie(): invalid tag id %8x", tag));
		return EINVAL;
	}

	TRACE (("set_cookie(): jar lookup"));

	while (cjar->tag)
	{
		n++;
		if (cjar->tag == tag)
		{
			cjar->value = val;
			TRACE (("set_cookie(): old entry %08x updated", tag));
			return E_OK;
		}
		cjar++;
	}

	n++;
	if (n < cjar->value)
	{
		n = cjar->value;
		cjar->tag = tag;
		cjar->value = val;

		cjar++;
		cjar->tag = 0L;
		cjar->value = n;

		TRACE (("set_cookie(): new entry"));
		return E_OK;
	}

	/* LIST exhausted :-) */

	DEBUG (("set_cookie(): unable to place an entry, jar full"));
	return ENOMEM;
}

/* remove cookie
 */

long
del_cookie (struct cookie *cj, ulong tag)
{
# ifdef JAR_PRIVATE
	struct user_things *ut;
	struct cookie *cjar;
# else
	struct cookie *cjar = *CJAR;
# endif

# ifdef JAR_PRIVATE
	ut = curproc->p_mem->tp_ptr;
	cjar = ut->user_jar_p;
# endif

	TRACE (("del_cookie: tag %lx", tag));

	if (cj)
		cjar = cj;

	while (cjar->tag)
	{
		if (cjar->tag == tag)
		{
			while (cjar->tag)
				*cjar++ = *(cjar + 1);

			TRACE (("del_cookie: tag removed from list"));
			return E_OK;
		}

		cjar++;
	}

	DEBUG (("del_cookie: tag not found!"));
	return EINVAL;
}

/* END cookie manipulation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN rsvf cookie handling */

# define RSVF_MEM	1024
# define MAX_ENTRYS	16

static MEMREGION *rsvfregion;

static RSVF *rsvfvec = NULL;
static ushort rsvfmax = 0;

static char *freepool = NULL;
static ushort freesize = 0;

long
add_rsvfentry (char *name, char portflags, char bdev)
{
	const int namelen = strlen (name) + 1;

	int flag = 1;
	int l = 0;
	int i = 0;
	RSVF *t;

	if (!rsvfvec)
	{
		rsvfregion = get_region (core, RSVF_MEM, PROT_G);
		if (!rsvfregion)
			return ENOMEM;

		bzero ((void *) rsvfregion->loc, RSVF_MEM);

		rsvfvec = (RSVF *) attach_region (rootproc, rsvfregion);
		rsvfmax = MAX_ENTRYS;

		freepool = (char *) (rsvfvec + MAX_ENTRYS);
		freesize = RSVF_MEM - (sizeof (RSVF) * MAX_ENTRYS);

		set_cookie (NULL, COOKIE_RSVF, (long) rsvfvec);
	}

	t = rsvfvec;
	while (t->data)
	{
		if (!stricmp (t->data, name))
			return EINVAL;

		if (bdev && flag && (bdev > t->bdev))
		{
			flag = 0;
			i++;
		}

		l++;
		t++;
	}

	if ((l == rsvfmax) || (namelen > freesize))
		return ENOMEM;

	if (bdev)
	{
		while (i < l)
		{
			rsvfvec [l] = rsvfvec [l-1];
			l--;
		}
	}

	strcpy (freepool, name);

	rsvfvec [l].data = freepool;
	rsvfvec [l].type = portflags;
	rsvfvec [l].res1 = 0;
	rsvfvec [l].bdev = bdev;
	rsvfvec [l].res2 = 0;

	freepool += namelen;
	freesize -= namelen;

	return E_OK;
}

long
del_rsvfentry (char *name)
{
	RSVF *t = rsvfvec;
	long r = EINVAL;

	while (t->data)
	{
		if (!stricmp (t->data, name))
			r = E_OK;

		if (r == E_OK)
			*t = *(t + 1);

		t++;
	}

	return r;
}

/* END rsvf cookie handling */
/****************************************************************************/
