/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 *
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 *
 *
 * Copyright (c) 1983, 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *
 */

# ifdef PROFILING

# include "info.h"
# include "gmon.h"

# include "mint/file.h"
# include "mint/proc.h"

# include "libkern/libkern.h"
# include "sys/gmon_out.h"

# include "global.h"
# include "k_fds.h"
# include "kmemory.h"
# include "profil.h"

#define ffs __builtin_ffs

struct iovec
{
	void	*iov_base;
	long	iov_len;
};

static void gmon_writev (FILEPTR *f, const struct iovec *iov, long niov);
INLINE void write_hist (FILEPTR *f);
INLINE void write_call_graph (FILEPTR *f);
INLINE void write_bb_counts (FILEPTR *f);
static void write_gmon (void);

struct gmonparam _gmonparam = { GMON_PROF_OFF };

/*
 * See profil(2) where this is described:
 */
static long s_scale;
# define SCALE_1_TO_1	0x10000L

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
void
moncontrol (long mode)
{
	struct gmonparam *p = &_gmonparam;

	/* Don't change the state if we ran into an error.  */
	if (p->state == GMON_PROF_ERROR)
		return;

	if (mode)
	{
		/* start */
		profil ((void *) p->kcount, p->kcountsize, p->lowpc, s_scale);
		p->state = GMON_PROF_ON;
	}
	else
	{
		/* stop */
		profil (NULL, 0, 0, 0);
		p->state = GMON_PROF_OFF;
	}
}


void
monstartup (ulong lowpc, ulong highpc)
{
	register long o;
	char *cp;
	struct gmonparam *p = &_gmonparam;

	/*
	 * round lowpc and highpc to multiples of the density we're using
	 * so the rest of the scaling (here and in gprof) stays in ints.
	 */
	p->lowpc = ROUNDDOWN (lowpc, HISTFRACTION * sizeof (HISTCOUNTER));
	p->highpc = ROUNDUP (highpc, HISTFRACTION * sizeof (HISTCOUNTER));
	p->textsize = p->highpc - p->lowpc;
	p->kcountsize = p->textsize / HISTFRACTION;
	p->hashfraction = HASHFRACTION;
	p->log_hashfraction = -1;

	/* The following test must be kept in sync with the corresponding
	 * test in mcount.c.
	 */
#if 1
	if ((HASHFRACTION & (HASHFRACTION - 1)) == 0)
	{
		/* if HASHFRACTION is a power of two, mcount can use shifting
		 * instead of integer division.  Precompute shift amount.
		 */
		//p->log_hashfraction = ffs (p->hashfraction * sizeof (*p->froms)) - 1;
		p->log_hashfraction = ffs (HASHFRACTION * sizeof (*p->froms)) - 1;
	}
#endif
	p->fromssize = p->textsize / HASHFRACTION;
	p->tolimit = p->textsize * ARCDENSITY / 100;

	if (p->tolimit < MINARCS)
		p->tolimit = MINARCS;
	else if (p->tolimit > MAXARCS)
		p->tolimit = MAXARCS;

	p->tossize = p->tolimit * sizeof (struct tostruct);

	cp = kmalloc (p->kcountsize + p->fromssize + p->tossize);
	if (!cp)
	{
		ALERT (MSG_gmon_out_of_mem);
		p->tos = NULL;
		p->state = GMON_PROF_ERROR;
		return;
	}

	p->tos = (struct tostruct *) cp;
	cp += p->tossize;
	p->kcount = (ushort *)cp;
	cp += p->kcountsize;
	p->froms = (ushort *)cp;

	p->tos[0].link = 0;

	o = p->highpc - p->lowpc;
	if (p->kcountsize < (ulong) o)
	{
# if 0
		s_scale = ((float) p->kcountsize / o) * SCALE_1_TO_1;
# else
		/* avoid floating point operations */
		long quot = o / p->kcountsize;

		if (quot >= 0x10000)
			s_scale = 1;
		else if (quot >= 0x100)
			s_scale = 0x10000 / quot;
		else if (o >= 0x800000)
			s_scale = 0x1000000 / (o / (p->kcountsize >> 8));
		else
			s_scale = 0x1000000 / ((o << 8) / p->kcountsize);
# endif
	}
	else
		s_scale = SCALE_1_TO_1;

	moncontrol (1);
}

void
mcleanup (void)
{
	moncontrol (0);

	if (_gmonparam.state != GMON_PROF_ERROR)
		write_gmon ();
}

static void
gmon_writev (FILEPTR *f, const struct iovec *iov, long niov)
{
	long i;

	for (i = 0; i < niov; i++)
	{
		if (iov[i].iov_len >= 0)
			(*f->dev->write)(f, iov[i].iov_base, iov[i].iov_len);
		else
			break;
	}
}

INLINE void
write_hist (FILEPTR *f)
{
	uchar tag = GMON_TAG_TIME_HIST;
	struct gmon_hist_hdr thdr __attribute__ ((aligned (__alignof__ (char *))));

	if (_gmonparam.kcountsize > 0)
	{
		ulong offset = (ulong) _base + 0x100;
		struct iovec iov[3] =
		{
			{ &tag, sizeof (tag) },
			{ &thdr, sizeof (struct gmon_hist_hdr) },
			{ _gmonparam.kcount, _gmonparam.kcountsize }
		};

		thdr.low_pc.p = (char *) _gmonparam.lowpc - offset;
		thdr.high_pc.p = (char *) _gmonparam.highpc - offset;
		thdr.hist_size.s32 = (_gmonparam.kcountsize / sizeof (HISTCOUNTER));
		thdr.prof_rate.s32 = profile_frequency ();

		strncpy (thdr.dimen, "seconds", sizeof (thdr.dimen));
		thdr.dimen_abbrev = 's';

		gmon_writev (f, iov, 3);
	}
}

INLINE void
write_call_graph (FILEPTR *f)
{
# define NARCS_PER_WRITEV	32
	uchar tag = GMON_TAG_CG_ARC;
	struct gmon_cg_arc_record raw_arc[NARCS_PER_WRITEV] __attribute__ ((aligned (__alignof__ (char*))));
	long from_index, to_index, from_len;
	ulong frompc;
	struct iovec iov[2 * NARCS_PER_WRITEV];
	long nfilled;

	ulong offset = (ulong) _base + 0x100;

	for (nfilled = 0; nfilled < NARCS_PER_WRITEV; ++nfilled)
	{
		iov[2 * nfilled].iov_base = &tag;
		iov[2 * nfilled].iov_len = sizeof (tag);

		iov[2 * nfilled + 1].iov_base = &raw_arc[nfilled];
		iov[2 * nfilled + 1].iov_len = sizeof (struct gmon_cg_arc_record);
	}

	nfilled = 0;
	from_len = _gmonparam.fromssize / sizeof (*_gmonparam.froms);
	for (from_index = 0; from_index < from_len; ++from_index)
	{
		if (_gmonparam.froms[from_index] == 0)
			continue;

		frompc = _gmonparam.lowpc;
		frompc += (from_index * _gmonparam.hashfraction * sizeof (*_gmonparam.froms));

		for (to_index = _gmonparam.froms[from_index];
			to_index != 0;
			to_index = _gmonparam.tos[to_index].link)
		{
			raw_arc[nfilled].from_pc.p = (char *) frompc - offset;
			raw_arc[nfilled].self_pc.p = (char *) _gmonparam.tos[to_index].selfpc - offset;
			raw_arc[nfilled].count.l = _gmonparam.tos[to_index].count;

			if (++nfilled == NARCS_PER_WRITEV)
			{
				gmon_writev (f, iov, 2 * nfilled);
				nfilled = 0;
			}
		}
	}

	if (nfilled > 0)
		gmon_writev (f, iov, 2 * nfilled);
}

/* Head of basic-block list or NULL. */
struct __bb *__bb_head = NULL;

INLINE void
write_bb_counts (FILEPTR *f)
{
	struct __bb *grp;
	uchar tag = GMON_TAG_BB_COUNT;
	long ncounts;
	long i;

	struct iovec bbhead[2] =
	{
		{ &tag, sizeof (tag) },
		{ &ncounts, sizeof (ncounts) }
	};
	struct iovec bbbody[8];
	long nfilled;

	for (i = 0; i < (sizeof (bbbody) / sizeof (bbbody[0])); i += 2)
	{
		bbbody[i].iov_len = sizeof (grp->addresses[0]);
		bbbody[i + 1].iov_len = sizeof (grp->counts[0]);
	}

	/* Write each group of basic-block info (all basic-blocks in a
	 * compilation unit form a single group).
	 */
	for (grp = __bb_head; grp; grp = grp->next)
	{
		ncounts = grp->ncounts;
		gmon_writev (f, bbhead, 2);

		for (nfilled = i = 0; i < ncounts; ++i)
		{
			if (nfilled > (sizeof (bbbody) / sizeof (bbbody[0])) - 2)
			{
				gmon_writev (f, bbbody, nfilled);
				nfilled = 0;
			}

			bbbody[nfilled++].iov_base = ( void *) &grp->addresses[i];
			bbbody[nfilled++].iov_base = &grp->counts[i];
		}

		if (nfilled > 0)
			gmon_writev (f, bbbody, nfilled);
	}
}

static void
write_gmon (void)
{
	FILEPTR *f;
	long ret;

	ret = do_open (&f, rootproc, "u:\\ram\\gmon.out", (O_WRONLY | O_CREAT | O_TRUNC), 0, NULL);
	if (ret == 0)
	{
		struct gmon_hdr ghdr __attribute__ ((aligned (__alignof__ (long))));

		/* write gmon.out header: */
		mint_bzero (&ghdr, sizeof (struct gmon_hdr));
		memcpy (&ghdr.cookie[0], GMON_MAGIC, sizeof (ghdr.cookie));
		ghdr.version.l = GMON_VERSION;
		(*f->dev->write)(f, (const char *) &ghdr, sizeof (struct gmon_hdr));

		/* write PC histogram: */
		write_hist (f);

		/* write call-graph: */
		write_call_graph (f);

		/* write basic-block execution counts: */
		write_bb_counts (f);

		do_close (rootproc, f);
	}
	else
		ALERT (MSG_gmon_out_fail);
}

void
write_profiling (void)
{
	struct gmonparam *p = &_gmonparam;
	long save = p->state;

	p->state = GMON_PROF_OFF;
	if (save == GMON_PROF_ON)
	{
		ALERT (MSG_gmon_out_written);
		write_gmon ();
	}

	p->state = save;
}

void
toogle_profiling (void)
{
	struct gmonparam *p = &_gmonparam;

	if (p->state == GMON_PROF_OFF)
	{
		moncontrol (1);
		ALERT (MSG_gmon_profiler_on);
	}
	else if (p->state == GMON_PROF_ON)
	{
		moncontrol (0);
		ALERT (MSG_gmon_profiler_off);
	}
	else
		ALERT (MSG_gmon_enable_error);
}

# endif /* PROFILING */
