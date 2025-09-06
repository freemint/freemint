/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Guido Flohr <guido@freemint.de>
 * All rights reserved.
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
 *
 * Author: Guido Flohr <guido@freemint.de>
 * Started: 1999-10-24
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Kernel system info filesystem, information retrieval routines.
 *
 * This file is way too long and the functions indent too deep.  Besides
 * it should be rewritten from scratch to implement a more object-oriented
 * approach, maybe.
 * In fact this file is not only too long but it should be split up
 * into separate files in a separate subdirectory but the kernel build
 * process currently doesn't allow this, thus making it difficult to
 * think of meaningful names.
 */

# include "kernget.h"
# include "init.h"
# include "global.h"

# if WITH_KERNFS

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "mint/credentials.h"
# include "mint/dcntl.h"
# include "mint/filedesc.h"
# include "mint/signal.h"

# include "arch/mprot.h"
# include "arch/timer.h"
# include "arch/cpu.h"

# include "biosfs.h"
# include "cookie.h"
# include "delay.h"
# include "filesys.h"
# include "info.h"
# include "kernfs.h"
# include "kmemory.h"
# include "memory.h"
# include "pipefs.h"
# include "proc.h"
# include "procfs.h"
# include "random.h"
# include "shmfs.h"
# include "time.h"
# include "timeout.h"
# include "unifs.h"
# include "util.h"
# include "xbios.h"


long
kern_get_unimplemented (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;

# define PATIENCE_PLEEZE "Not yet implemented!\n"

	UNUSED(p);
	len = sizeof PATIENCE_PLEEZE - 1;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
	{
		DEBUG (("kern_get_cmdline: virtual memory exhausted"));
		return ENOMEM;
	}

	memcpy (info->buf, PATIENCE_PLEEZE, len);
	info->len = len;

	*buffer = info;
	return 0;
}


long
kern_get_buildinfo (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 512;

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	info->len = ksprintf (info->buf, len,
			"CC: %s\nCFLAGS: %s\nCDEFS: %s\n",
			COMPILER_NAME,
			COMPILER_OPTS,
			COMPILER_DEFS
	);

	*buffer = info;
	return 0;
}

long
kern_get_bootlog (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = strlen(boot_file) + 1;

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	strcpy (info->buf, boot_file);
	info->len = len - 1;

	*buffer = info;
	return 0;
}


long
kern_get_cookiejar (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;
	ulong slots;
	ulong used = 0;
	ulong i;
	char *crs;
	union{
		ulong tagid;
		uchar name[5];
	}ut;
	ulong value;

	UNUSED(p);
	if (get_cookie (NULL, 0, &slots))
	{
		DEBUG (("kern_get_cmdline: get_cookie(0) failed!"));
		return ENOMEM;
	}

	for (i = 1; i < slots; i++)
	{
		if (get_cookie (NULL, i, &value) || (value == 0UL))
			break;

		used++;
	}

	len = 128 + used * 34;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	crs = info->buf;
	crs += ksprintf (crs, len - (crs - info->buf), "# Slots available: %4lu\n", slots);
	crs += ksprintf (crs, len - (crs - info->buf), "# Slots used:      %4lu\n", used);

	for (i = 1; i <= used; i++)
	{
		uchar pretty_print_name[9];
		uchar *to;
		int j;

		get_cookie (NULL, i, &ut.tagid );

		to = pretty_print_name;
		for (j = 0; j < 4; j++)
		{
			if (ut.name[j] < ' ')
			{
				*to++ = '^';
				*to++ = ('@' + ut.name[j]);
			}
			else
			{
				*to++ = ut.name[j];
			}
		}

		*to++ = '\0';

		/* Now calculate the value of the cookie */
		get_cookie (NULL, ut.tagid, &value);

		crs += ksprintf (crs, len - (crs - info->buf),
				"0x%08lx (%s): 0x%08lx\n",
				ut.tagid,
				pretty_print_name,
				value
		);
	}

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}

long
kern_get_filesystems (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;
	FILESYS *fs;
	char *crs;

	UNUSED(p);
	for (fs = active_fs, len = 0; fs != NULL; fs = fs->next, len++)
		;

	len = len * 41 + 1;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	crs = info->buf;
	for (fs = active_fs; fs != NULL; fs = fs->next)
	{
		char namebuf [32];
		char device [12];
		char *name = "unknown";
		int i;

		strcpy (device, "nodev   ");

		if (fs == &uni_filesys)
			continue;
		else if (fs == &kern_filesys)
			name = "kern pseudo file-system";
		else if (fs == &proc_filesys)
			name = "proc pseudo file-system";
		else if (fs == &shm_filesys)
			name = "shm pseudo file-system";
		else if (fs == &pipe_filesys)
			name = "pipe pseudo file-system";
		else if (fs == &bios_filesys)
			name = "bios pseudo file-system";
		else
		{
			strcpy (device, "        ");

			for (i = 2; i < NUM_DRIVES; i++)
				if (rootproc->p_cwd->root[i].fs == fs)
					break;

			if (i < NUM_DRIVES)
			{
				fcookie dir;

				dup_cookie (&dir, &(rootproc->p_cwd->root[i]));

				if (xfs_fscntl (fs, &dir, NULL, MX_KER_XFSNAME, (long) namebuf) == E_OK)
					name = namebuf;

				release_cookie (&dir);
			}
		}

		crs += ksprintf (crs, len - (crs - info->buf), "%s%s\n", device, name);
	}

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}

long
kern_get_hz (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;

	UNUSED(p);
	/* Enough for a number */
	len = 16;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	info->len = ksprintf (info->buf, len, "%lu\n", HZ);

	*buffer = info;
	return 0;
}

/* Functions that fill our read buffers */
# define FSHIFT		11
# define FIXED_1	(1 << FSHIFT)		/* 1.0 as fixed-point */
# define FSCALE		LOAD_SCALE
# define LOAD_INT(x)	((x) >> FSHIFT)
# define LOAD_FRAC(x)	LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

long
kern_get_loadavg (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 64;
	short tasks = 0;
	short running = 0;
	int last_pid = 0; /* ??? */

	DEBUG (("kern_get_loadavg"));

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	for (p = proclist; p; p = p->gl_next)
	{
		if (p == rootproc)
			continue;

		tasks++;

		if ((p->wait_q == CURPROC_Q) || (p->wait_q == READY_Q))
			running++;
	}

	info->len = ksprintf (info->buf, len, "%lu.%03lu %lu.%03lu %lu.%03lu %d/%d %d\n",
			      LOAD_INT (avenrun[0]), LOAD_FRAC (avenrun[0]),
			      LOAD_INT (avenrun[1]), LOAD_FRAC (avenrun[1]),
			      LOAD_INT (avenrun[2]), LOAD_FRAC (avenrun[2]),
			      running, tasks, last_pid);

	*buffer = info;
	return 0;
}


/*
  /kern/meminfo
  It is NOT FULLY implemented !
  Uses the "tot_rsize( MMAP, flag)"-function.
  Layout-idea taken from LiNUX.
  Flames, Critics, ... to pralle@informatik.uni-hannover.de
 */
long
kern_get_meminfo (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 512;
	ulong i;
	char *crs;

	ulong core_total = tot_rsize( core, 1);
	ulong core_free  = tot_rsize( core, 0);
	ulong alt_total  = tot_rsize( alt, 1);
	ulong alt_free   = tot_rsize( alt, 0);
	ulong mem_total  = core_total + alt_total;
	ulong mem_free   = core_free + alt_free;

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	crs = info->buf;

	i = ksprintf( crs, len,
			 "\t  total:  \t  used:   \t  free:\n");
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "Mem:\t%10lu\t%10lu\t%10lu\n",
			 core_total + alt_total,
			 mem_total - mem_free,
			 mem_free);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "Swap:\t%10lu\t%10lu\t%10lu\n",
			 0ul, 0ul, 0ul);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "\nMemTotal:\t%7lu kB\n",
			 mem_total / ONE_K);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "MemFree:\t%7lu kB\n",
			 mem_free / ONE_K);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "FastTotal:\t%7lu kB\n",
			 alt_total / ONE_K);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "FastFree:\t%7lu kB\n",
			 alt_free / ONE_K);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "CoreTotal:\t%7lu kB\n",
			 core_total / ONE_K);
	crs += i; len -= i;

	i = ksprintf( crs, len,
			 "CoreFree:\t%7lu kB\n",
			 core_free / ONE_K);
	crs += i; len -= i;

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}


/**
 * /kern/stat
 * The processor status.
 *
 * !! It is a QUICK HACK to  make the procps-2.0.7 confident. !!
 *
 * cpu:  user_j  nice_j  sys_j  other_j
 * The _j-values are the number of jiffies the CPU spent in the user, sys, ... mode.
 *
 * Other lines will be introduced as soon as they are needed.
 */
long
kern_get_stat (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 64;
	ulong all_systime = 0, all_usrtime = 0, all_nice = 0;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	for (p = proclist; p; p = p->gl_next)
	{
		if (p->pid)
		{
			all_systime += p->systime;
			all_usrtime += p->usrtime;
		}
	}

# define cpus 1L

	info->len = ksprintf (info->buf, len, "cpu \t%9lu %9lu %9lu %9lu\n",
		              all_usrtime,
			      all_nice,
			      all_systime,
			      jiffies * cpus - (all_usrtime + all_nice + all_systime));

	*buffer = info;
	return 0;
}

/*
 * /kern/sysdir
 * The system directory there the kernel load the modules and
 * the configuration.
 */
long
kern_get_sysdir (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;

	UNUSED(p);
	len = strlen(sysdir) + 1;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	strcpy (info->buf, sysdir);
	info->len = len - 1;

	*buffer = info;
	return 0;
}

/*
 *
 */
long
kern_get_time (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 64;

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	synch_timers ();

	info->len = ksprintf (info->buf, len, "%ld.%06ld %ld.%06ld\n",
		              xtime.tv_sec, xtime.tv_usec,
			      xtime.tv_sec - timezone, xtime.tv_usec);

	*buffer = info;
	return 0;
}


long
kern_get_uptime (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 64;
	ulong idle = rootproc->systime + rootproc->usrtime;

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	info->len = ksprintf (info->buf, len, "%lu.%03d %lu.%03lu\n",
			      uptime,
			      1000 - uptimetick * 5,
			      idle / 1000,
			      idle % 1000);

	*buffer = info;
	return 0;
}

long
kern_get_version (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 256;
# if BETA_IDENT
	char beta_ident [] = { BETA_IDENT, '\0' };
# else
	char beta_ident [] = "";
# endif
# if !PATCH_LEVEL
	char revision [] = "";
# else
	char revision [12] = "";
	ksprintf (revision, sizeof (revision), ".%lu", (ulong) PATCH_LEVEL);
# endif

	UNUSED(p);
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	info->len = ksprintf (info->buf, len,
			"FreeMiNT version %d.%d%s%s (%s) #%u %s\n",
			(int) MINT_MAJ_VERSION,
			(int) MINT_MIN_VERSION,
			revision,
			beta_ident,
			COMPILER_NAME,
			1 /* build_serial */,
			build_ctime
	);

	*buffer = info;
	return 0;
}

long
kern_get_welcome (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 0;
	const char *greets [] =
	{
		greet1,
		greet2,
# ifdef WITH_MMU_SUPPORT
		no_mem_prot ? memprot_warning : "",
# endif
# ifdef CRYPTO_CODE
		crypto_greet,
# endif
# ifdef DEV_RANDOM
		random_greet,
# endif
		NULL
	};
	const char **greet;
	union { const char *cc; unsigned char *c; } from;
	uchar *to;

	UNUSED(p);
	for (greet = greets; *greet != NULL; greet++)
		len += strlen (*greet);

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	to = (unsigned char *)info->buf;

	for (greet = greets; *greet != NULL; greet++)
	{
		from.cc = *greet;

		while (*from.c)
		{
			/* MiNT is Now TOS */
			if (*from.c == '\r')
			{
				from.c++;
				len--;
				continue;
			}

			*to++ = *from.c++;
		}
	}

	info->len = len;

	*buffer = info;
	return 0;
}


long
kern_procdir_get_cmdline (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong argc;
	ulong len;

	while (p->real_cmdline == NULL)
	{
		p = pid2proc (p->ppid);
		if (p == NULL || p == rootproc)
		{
			*buffer = NULL;
			return 0;
		}
	}

        argc = *((ulong *) (p->real_cmdline + sizeof (argc)));
       	len = *((ulong *) (p->real_cmdline));
       	len -= (sizeof (argc)	/* argc */
       			+ (argc + 1) * sizeof (char *) /* argv */
       			+ 2); /* 2 null-termination bytes */

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	memcpy (info->buf, p->real_cmdline + sizeof (len) + sizeof (argc)
		+ (argc + 1) * sizeof (char *), len);
	info->len = len;

	*buffer = info;
	return 0;
}

/* Get the process environment, stuff it into our read buffer and do
 * some fixup en passant.  There are some buggy programs (notably
 * desktops) that have a weird idea about the environment, using
 * strings like "FOO=\000BAR\000", i. e. they put a null-byte
 * after the equal sign.  This will get fixed, the extra null-byte
 * is skipped.
 *
 * Note that the buffer is _not_ terminated by a double null-byte.
 * The end of buffer can easily detected by the reader on the
 * return value EOF.
 */
long
kern_procdir_get_environ (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info = NULL;
	MEMREGION *baseregion = NULL;
	MEMREGION *envregion = NULL;
	long pbase, penv;
	ulong len;
	char *to;
	enum { var = 0, equal = 1, value = 2 } state = var;
	char *env;
	char *from;
	long ret = 0;


	if (!p->p_mem)
	{
		DEBUG (("kern_procdir_get_environ: pid %d: no p_mem???", p->pid));
		goto out;
	}

	pbase = (long)p->p_mem->base;

	DEBUG (("get_environ: curproc = %p, p = %p", get_curproc(), p));
	DEBUG (("get_environ: curproc->base = %p, p->base = %lx", get_curproc()->p_mem->base, pbase));
	DEBUG (("get_environ: %p, %p, %p, %p",
			proc_addr2region (get_curproc(), pbase),
			proc_addr2region (p, pbase),
			addr2mem (get_curproc(), pbase),
			addr2region (pbase)
	));

	if (!pbase)
	{
		DEBUG (("kern_procdir_get_environ: pid %d: no environment", p->pid));
		goto out;
	}

	baseregion = proc_addr2region (get_curproc(), pbase);
	if (!baseregion)
	{
		baseregion = addr2region (pbase);

		if (baseregion)
			attach_region (get_curproc(), baseregion);
		else
			DEBUG (("kern_procdir_get_environ: no baseregion found, pid %i", p->pid));
	}
	else
		/* already attached, don't detach */
		baseregion = NULL;

	penv = (long)p->p_mem->base->p_env;
	if (!penv)
	{
		DEBUG (("kern_procdir_get_environ: pid %d: no environment", p->pid));
		goto out;
	}

	envregion = proc_addr2region (get_curproc(), penv);
	if (!envregion)
	{
		envregion = addr2region (penv);
		assert (envregion);

		attach_region (get_curproc(), envregion);
	}
	else
		/* already attached, don't detach */
		envregion = NULL;

	from = env = (char *)penv;

	/* Walk the array and find out the maximum length */
  	for (;;)
  	{
# ifndef M68000 
		if (*(ushort *)from == 0)
			break;
# else
    		if (from[0] == '\0' && from[1] == '\0')
      			break;
# endif

# ifndef M68000

# define ___ARGV	0x41524756L

		if (*(ulong *)from == ___ARGV && from[4] == '=')
			break;
# else
		if (strncmp(from, "ARGV=", sizeof("ARGV=") - 1) == 0)
			break;
# endif
    		from++;
  	}

  	len = from - env;
  	if (!len)
		goto out;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
	{
		ret = ENOMEM;
		goto out;
  	}

	from = env;
	to = info->buf;
	while (from < (env + len))
	{
		if (state != equal && *from != '\000' && *from != '=')
		{
			*to++ = *from++;
			continue;
		}

		switch (state)
		{
			case var:
				if (*from == '\000')
					*to++ = *from++;
	      			else
	      			{
        				*to++ = *from++;
        				state = equal;
	      			}
      				break;
	    		case equal:
	    		{
      				if (*from == '\000')
        				from++;
	      			else
        				*to++ = *from++;

	      			state = value;
      				break;
      			}
	    		case value:
	    		{
      				if (*from == '\000')
        				state = var;
	      			*to++ = *from++;
				break;
			}
		}
	}

	info->len = to - info->buf;

out:
	if (envregion)
		detach_region (get_curproc(), envregion);
	if (baseregion)
		detach_region (get_curproc(), baseregion);

	*buffer = info;
	return ret;
}

long
kern_procdir_get_fname (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;

	len = strlen (p->fname) + 1;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	strcpy(info->buf, p->fname);
	info->len = len - 1;

	*buffer = info;
	return 0;
}

long
kern_procdir_get_meminfo (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	char *crs;
	ulong len;
	int i;

	if (!p->p_mem || !p->p_mem->mem)
		return EBADARG;

	len = p->p_mem->num_reg * 64;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	crs = info->buf;
	crs += ksprintf (crs, len - (crs - info->buf),
		"Base:%08lx\n",
		(unsigned long) p->p_mem->base);
	for (i = 0; i < p->p_mem->num_reg; i++)
	{
		MEMREGION *m;

		m = p->p_mem->mem[i];
		if (!m)
			continue;

# define M_SHTEXT	0x0010	/* XXX */
# define M_SHTEXT_T	0x0020	/* XXX */

		crs += ksprintf (crs, len - (crs - info->buf),
				"%08lx-%08lx %3u %c%c%c%c%c%c %c %08x %02lu:%02lu %lu\n",
				(ulong) m->loc,
				(ulong) m->loc + m->len - 1,
				(unsigned) m->links,
				m->mflags & M_KEEP ? 'k' : '-',
				m->mflags & M_SHARED ? 's' : '-',
				m->mflags & M_FSAVED ? 'f' : '-',
				m->mflags & M_SHTEXT_T ? 't' : '-',
				m->mflags & M_SHTEXT ? 's' : '-',
				m->mflags & M_CORE ? 'c' :
				m->mflags & M_ALT ? 'a' :
				m->mflags & M_SWAP ? 's' :
				m->mflags & M_KER ? 'k' : '-',
				"PGSRI"[get_prot_mode(m)],
				0,
				0UL,
				0UL,
				0UL);
	}

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}

/* Try to find something like a session id
 * only used once
 */
INLINE int
get_session_id (const struct proc *p)
{
	while (p && p != rootproc)
	{
		struct proc *leader = pid2proc (p->pgrp);

		if (leader != NULL)
			return leader->pid;

		p = pid2proc (p->ppid);
	}

	return 0;
}

INLINE void
timersub (struct timeval *a, struct timeval *b, struct timeval *result)
{
	result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_usec = a->tv_usec - b->tv_usec;

	if (result->tv_usec < 0)
	{
		--result->tv_sec;
		result->tv_usec += 1000000;
	}
}

long
kern_procdir_get_stat (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	long base;
	ulong len = 512;
	char state;
	int ttypgrp = 0;
	struct timeval starttime;
	ulong startcode = 0;
	ulong endcode = 0xffffffffUL;
	ulong i;
	ulong sigign = 0;
	ulong sigcgt = 0;
	ulong bit = 1;
	TIMEOUT *t;
	TIMEOUT *first = NULL;
	long timeout = 0UL;
	unsigned short memflags = 0;

	DEBUG (("kern_procdir_get_stat: enter"));

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	switch (p->wait_q)
	{
		case CURPROC_Q:
		case READY_Q:
			state = 'R';
			break;
		case WAIT_Q:
		case TSR_Q:
		case SELECT_Q:
			state = 'S';
			break;
		case IO_Q:
			state = 'D';
			break;
		case ZOMBIE_Q:
			state = 'Z';
			break;
		case STOP_Q:
			state = 'T';
			break;
		default:
			state = '?';
			break;
	}

	if (p->p_fd && p->p_fd->control)
	{
		struct tty *tty = (struct tty *) p->p_fd->control->devinfo;
		ttypgrp = tty->pgrp;
	}

	timersub (&xtime, &rootproc->started, &starttime);

	/* FIXME: The various timeout fields in our file should report
	 * the number of microseconds that elapse before the timeout
	 * occurs.  We currently simply report the WHEN field
	 * and I'm not sure if this is correct.
	 */
	for (t = expire_list; t != NULL; t = t->next)
	{
		if (t->proc == p)
		{
			if (first == NULL)
				first = t;
			else if (t->when < first->when)
				first = t;
		}
	}

	if (first != NULL)
		timeout = first->when;

	if (p->p_mem && p->p_mem->mem)
	{
		MEMREGION *baseregion;

		base = (long)p->p_mem->base;
		memflags = p->p_mem->memflags;

		for (i = 0; i < p->p_mem->num_reg; i++)
		{
			MEMREGION *m = p->p_mem->mem[i];
			if (m && m->loc == base)
			{
				endcode = ((unsigned long)base + m->len);
				break;
			}
		}

		baseregion = proc_addr2region (get_curproc(), base);
		if (!baseregion)
		{
			baseregion = addr2region (base);

			if (baseregion)
				attach_region (get_curproc(), baseregion);
			else
				DEBUG (("kern_procdir_get_stat: no baseregion found, pid %i", p->pid));
		}
		else
			/* already attached, don't detach */
			baseregion = NULL;

		startcode = endcode - (ulong)p->p_mem->base->p_bbase - (ulong)p->p_mem->base->p_blen;

		if (baseregion)
			detach_region (get_curproc(), baseregion);
	}
	else
	{
		base = 0;
		endcode = 0;
	}

	if (p->p_sigacts)
	{
		for (i = 1; i < NSIG; i++)
		{
			switch (SIGACTION(p, i).sa_handler)
			{
				case SIG_DFL:
					break;
				case SIG_IGN:
					sigign |= bit;
					break;
				default:
					sigcgt |= bit;
					break;
			}

			bit <<= 1;
		}
	}

	DEBUG (("kern_procdir_get_stat: do ksprintf"));

	info->len = ksprintf (info->buf, len,
				"%d (%s) %c %d %d %d %d %u "
				"%lu %lu %lu %lu %lu %lu %ld %ld %d %d "
				"%ld %ld %ld %ld %lu %lu %lu %lu %lu %lu "
				"%lu %lu %lu %lu %lu %lu %lu\n",

				p->pid,
				p->name,
				state,
				p->ppid,
				p->pgrp,
				get_session_id (p),
				ttypgrp,
				memflags,

				0UL, 0UL, 0UL, 0UL, /* Page faults */
				p->systime, p->usrtime, p->chldstime, p->chldutime,
				(int) (p->slices * time_slice * 20),
				(int) -(p->pri),

				(long) timeout / 5,
				(long) p->alarmtim->when / 5,
				(long) p->itimer->timeout->when / 5,
				(long) starttime.tv_sec * 200L + (long) starttime.tv_usec / 5000L,
				(ulong) memused (p),
				(ulong) memused (p),	/* rss */
				0x7fffffffUL,		/* rlim */
				base + 256UL,
				endcode,
				startcode,

				0UL, 0UL,
				(ulong) (p->sigpending >> 1),
				(ulong) (p->p_sigmask >> 1),
				(ulong) sigign,
				(ulong) sigcgt,
				(ulong) p->wait_q
	);

	*buffer = info;
	return 0;
}

/**
 * This one gets information about the used pages of the process.
 * Uiuiui! WHAT A HACK !!
 * It is far away from being that what it should be.
 * I just add it to make TOP from the PROCPS-2.0.7 suit happy.
 *
 * Most of the stuff isn't meaningful ,because of NO Virtual-Memory(*)
 *
 * ATM I do not know if we have shared-stuff. I will check it, soon.
 * Anyways, we deliver:
 * - total	Total number of memory pages.
 * - resid	Number of resident (non swapped) pages.	(see (*))
 * - share	Number of shared pages.
 * - txtrs	Text resident pages.			(see (*))
 * - librs	Shared-lib resident size.		(see (*))
 * - datrs	Data resident size.
 * - dpage	Dirty pages.				(see (*))
 *
 */
long
kern_procdir_get_statm( SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 64;

	ulong total = memused(p);
	ulong resid = total;
	ulong share = 0;
	ulong txtrs = p->p_mem ? p->p_mem->txtsize : 0;
	ulong librs = 0;
	ulong datrs = resid - txtrs;		/* Maybe that will do. */
	ulong dpage = 0;

	info = kmalloc( sizeof ( *info) + len);
	if (!info)
		return ENOMEM;


	/* exchange LOG2_EIGHT_K with PAGEBITS. !!! */
	/* Why is it in 'kmemory.c' and not in 'kmemory.h' ? */

	info->len = ksprintf (info->buf, len, "%lu %lu %lu %lu %lu %lu %lu\n",
		total >> LOG2_EIGHT_K,
		resid >> LOG2_EIGHT_K,
		share >> LOG2_EIGHT_K,
		txtrs >> LOG2_EIGHT_K,
		librs >> LOG2_EIGHT_K,
		datrs >> LOG2_EIGHT_K,
		dpage
	);

	*buffer = info;
	return 0;
}

long
kern_procdir_get_status (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len = 512;
	char *crs;
	ulong memsize = 0;
	ulong coresize = 0;
	ulong altsize = 0;
	ulong swapsize = 0;
	ulong kersize = 0;
	ulong regions = 0;
	ulong shtextsize = 0;
	ulong shtexttsize = 0;
	ulong fsavedsize = 0;
	ulong sharedsize = 0;
	ulong keepsize = 0;
	ulong kmallocsize = 0;
	char *state;
	ulong sigign = 0;
	ulong sigcgt = 0;
	ulong bit = 1;
	int i;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	crs = info->buf;
	crs += ksprintf (crs, len - (crs - info->buf), "Name:\t%s\n", p->name);
	switch (p->wait_q)
	{
		case CURPROC_Q:
		case READY_Q:
			state = "R (running)";
			break;
		case WAIT_Q:
		case TSR_Q:
		case SELECT_Q:
			state = "S (sleeping)";
			break;
		case IO_Q:
			state = "D (disk sleep)";
			break;
		case ZOMBIE_Q:
			state = "Z (zombie)";
			break;
		case STOP_Q:
			state = "T (stopped)";
			break;
		default:
			state = "? (unknown)";
			break;
	}

	crs += ksprintf (crs, len - (crs - info->buf), "State:\t%s\n", state);

	assert (p->p_cred && p->p_cred->ucr);

	crs += ksprintf (crs, len - (crs - info->buf),
			 "Pid:\t%d\nPPid:\t%d\nUid:\t%d\t%d\t%d\nGid:\t%d\t%d\t%d\n",
		         p->pid, p->ppid,
		         p->p_cred->ruid, p->p_cred->ucr->euid, p->p_cred->suid,
		         p->p_cred->rgid, p->p_cred->ucr->egid, p->p_cred->sgid);

	if (p->p_mem && p->p_mem->mem)
	{
		regions = p->p_mem->num_reg;

		DEBUG (("status: Number of Regions: %8lu", regions));

		for (i = 0; i < regions; i++)
		{
			MEMREGION *m;

			m = p->p_mem->mem[i];
			if (!m)
				continue;

			memsize += m->len;
			switch (m->mflags & M_MAP)
			{
				case M_CORE:
					coresize += m->len;
					break;
				case M_ALT:
					altsize += m->len;
					break;
				case M_SWAP:
					swapsize += m->len;
					break;
				case M_KER:
					kersize += m->len;
					break;
			}

			if (m->mflags & M_SHTEXT)
				shtextsize += m->len;
			if (m->mflags & M_SHTEXT_T)
				shtexttsize += m->len;
			if (m->mflags & M_FSAVED)
				fsavedsize += m->len;
			if (m->mflags & M_SHARED)
				sharedsize += m->len;
			if (m->mflags & M_KEEP)
				keepsize += m->len;
			if (m->mflags & M_KMALLOC)
				kmallocsize += m->len;
		}
	}

	crs += ksprintf (crs, len - (crs - info->buf),
			"VmSize:\t%8lu kB\n"
			"VmCore:\t%8lu kB\n"
			"VmAlt:\t%8lu kB\n"
			"VmSwap:\t%8lu kB\n"
			"VmKer:\t%8lu kB\n"
			"NumReg:\t%8lu \n",
			memsize >> 10,
			coresize >> 10,
			altsize >> 10,
			swapsize >> 10,
			kersize >> 10,
			regions
	);

	crs += ksprintf (crs, len - (crs - info->buf),
			"ShTxt:\t%8lu kB\n"
			"ShTxtT:\t%8lu kB\n"
			"Fsav:\t%8lu kB\n"
			"Shared:\t%8lu kB\n"
			"Keep:\t%8lu kB\n"
			"KMall:\t%8lu kB\n",
			shtextsize >> 10,
			shtexttsize >> 10,
			fsavedsize >> 10,
			sharedsize >> 10,
			keepsize >> 10,
			kmallocsize >> 10
	);

	if (p->p_sigacts)
	{
		for (i = 1; i < NSIG; i++)
		{
			switch (SIGACTION(p, i).sa_handler)
			{
				case SIG_DFL:
					break;
				case SIG_IGN:
					sigign |= bit;
					break;
				default:
					sigcgt |= bit;
					break;
			}

			bit <<= 1;
		}
	}

	crs += ksprintf (crs, len - (crs - info->buf),
			"SigPnd:\t%08lx\n"
			"SigBlk:\t%08lx\n"
			"SigIgn:\t%08lx\n"
			"SigCgt:\t%08lx\n",
			p->sigpending >> 1,
			p->p_sigmask >> 1,
			sigign >> 1,
			sigcgt >> 1
	);

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}

# endif /* WITH_KERNFS */
