/*
 * $Id$
 *
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


/* Conversion table from `atarist' charset to `latin1' charset.
 * Generated mechanically by GNU recode 3.4.
 *
 * The recoding should be reversible.
 */

uchar const atarist_to_latin1 [256] =
{
      0,   1,   2,   3,   4,   5,   6,   7,     /*   0 -   7 */
      8,   9,  10,  11,  12,  13,  14,  15,     /*   8 -  15 */
     16,  17,  18,  19,  20,  21,  22,  23,     /*  16 -  23 */
     24,  25,  26,  27,  28,  29,  30,  31,     /*  24 -  31 */
     32,  33,  34,  35,  36,  37,  38,  39,     /*  32 -  39 */
     40,  41,  42,  43,  44,  45,  46,  47,     /*  40 -  47 */
     48,  49,  50,  51,  52,  53,  54,  55,     /*  48 -  55 */
     56,  57,  58,  59,  60,  61,  62,  63,     /*  56 -  63 */
     64,  65,  66,  67,  68,  69,  70,  71,     /*  64 -  71 */
     72,  73,  74,  75,  76,  77,  78,  79,     /*  72 -  79 */
     80,  81,  82,  83,  84,  85,  86,  87,     /*  80 -  87 */
     88,  89,  90,  91,  92,  93,  94,  95,     /*  88 -  95 */
     96,  97,  98,  99, 100, 101, 102, 103,     /*  96 - 103 */
    104, 105, 106, 107, 108, 109, 110, 111,     /* 104 - 111 */
    112, 113, 114, 115, 116, 117, 118, 119,     /* 112 - 119 */
    120, 121, 122, 123, 124, 125, 126, 127,     /* 120 - 127 */
    199, 252, 233, 226, 228, 224, 229, 231,     /* 128 - 135 */
    234, 235, 232, 239, 238, 236, 196, 197,     /* 136 - 143 */
    201, 230, 198, 244, 246, 242, 251, 249,     /* 144 - 151 */
    255, 214, 220, 162, 163, 165, 223, 159,     /* 152 - 159 */
    225, 237, 243, 250, 241, 209, 170, 186,     /* 160 - 167 */
    191, 190, 172, 189, 188, 161, 171, 187,     /* 168 - 175 */
    227, 245, 216, 248, 221, 181, 192, 195,     /* 176 - 183 */
    213, 168, 180, 152, 182, 169, 174, 185,     /* 184 - 191 */
    166, 193, 194, 156, 142, 143, 146, 128,     /* 192 - 199 */
    200, 144, 202, 203, 204, 205, 206, 207,     /* 200 - 207 */
    208, 157, 210, 211, 212, 184, 153, 215,     /* 208 - 215 */
    253, 217, 218, 219, 154, 167, 222, 158,     /* 216 - 223 */
    133, 160, 131, 176, 132, 134, 145, 135,     /* 224 - 231 */
    138, 130, 136, 137, 141, 173, 140, 139,     /* 232 - 239 */
    240, 177, 149, 155, 147, 164, 247, 148,     /* 240 - 247 */
    254, 151, 183, 150, 129, 178, 179, 175      /* 248 - 255 */
};


long
kern_get_unimplemented (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len;

# define PATIENCE_PLEEZE "Not yet implemented!\n"

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
kern_get_buildinfo (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 512;

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
kern_get_cookiejar (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len;
	ulong slots;
	ulong used = 0;
	ulong i;
	char *crs;
	uchar name[5];
	ulong *tagid = (ulong *) name;
	ulong value;

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

		get_cookie (NULL, i, (ulong *) name);

		to = pretty_print_name;
		for (j = 0; j < 4; j++)
		{
			if (name[j] < ' ')
			{
				*to++ = '^';
				*to++ = ('@' + name[j]);
			}
			else
			{
				*to++ = name[j];
			}
		}

		*to++ = '\0';

		/* Now calculate the value of the cookie */
		get_cookie (NULL, *tagid, &value);

		crs += ksprintf (crs, len - (crs - info->buf),
				"0x%08lx (%s): 0x%08lx\n",
				*tagid,
				pretty_print_name,
				value
		);
	}

	info->len = crs - info->buf;

	*buffer = info;
	return 0;
}

long
kern_get_filesystems (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len;
	FILESYS *fs;
	char *crs;

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

		_mint_strcpy (device, "nodev   ");

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
			_mint_strcpy (device, "        ");

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
kern_get_hz (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len;

	/* Enough for a number */
	len = 16;

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	ksprintf (info->buf, len, "%lu", HZ);
	info->len = strlen(info->buf);

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
kern_get_loadavg (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 64;
	struct proc *p;
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
kern_get_meminfo (SIZEBUF **buffer)
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
kern_get_stat (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 64;
	struct proc *p;
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
 *
 */
long
kern_get_time (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 64;

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
kern_get_uptime (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 64;
	ulong idle = rootproc->systime + rootproc->usrtime;

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
kern_get_version (SIZEBUF **buffer)
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

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	info->len = ksprintf (info->buf, len,
			"FreeMiNT version %d.%d%s%s (%s@%s.%s) (%s) #%lu %s\n",
			(int) MAJ_VERSION,
			(int) MIN_VERSION,
			revision,
			beta_ident,
			build_user,
			build_host,
			build_domain,
			COMPILER_NAME,
			build_serial,
			build_ctime
	);

	*buffer = info;
	return 0;
}

long
kern_get_welcome (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 0;
	const char *greets [] =
	{
		greet1,
		greet2,
		no_mem_prot ? memprot_warning : "",
# ifdef CRYPTO_CODE
		crypto_greet,
# endif
# ifdef DEV_RANDOM
		random_greet,
# endif
		NULL
	};
	const char **greet;
	const uchar *from;
	uchar *to;

	for (greet = greets; *greet != NULL; greet++)
		len += _mint_strlen (*greet);

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	to = info->buf;

	for (greet = greets; *greet != NULL; greet++)
	{
		from = *greet;

		while (*from)
		{
			/* MiNT is Now TOS */
			if (*from == '\r')
			{
				from++;
				len--;
				continue;
			}

			*to++ = (char) atarist_to_latin1 [*from++];
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
kern_procdir_get_environ (SIZEBUF **buffer, struct proc *p)
{
	SIZEBUF *info = NULL;
	MEMREGION *baseregion = NULL;
	MEMREGION *envregion = NULL;
	ulong len;
	char *to;
	enum { var = 0, equal = 1, value = 2 } state = var;
	char *env;
	char *from;
	long ret = 0;


	DEBUG (("get_environ: curproc = %lx, p = %lx", curproc, p));
	DEBUG (("get_environ: curproc->base = %lx, p->base = %lx", curproc->base, p->base));
	DEBUG (("get_environ: %lx, %lx, %lx, %lx",
			proc_addr2region (curproc, (long) p->base),
			proc_addr2region (p, (long) p->base),
			addr2mem (curproc, (long) p->base),
			addr2region ((long) p->base)
	));

	if (!p->base)
	{
		DEBUG (("kern_procdir_get_environ: pid %d: no environment", p->pid));
		goto out;
	}

	baseregion = proc_addr2region (curproc, (long) p->base);
	if (!baseregion)
	{
		baseregion = addr2region ((long) p->base);

		if (baseregion)
			attach_region (curproc, baseregion);
		else
			DEBUG (("kern_procdir_get_environ: no baseregion found, pid %i", p->pid));
	}
	else
		/* already attached, don't detach */
		baseregion = NULL;

	if (!p->base->p_env)
	{
		DEBUG (("kern_procdir_get_environ: pid %d: no environment", p->pid));
		goto out;
	}

	envregion = proc_addr2region (curproc, (long) p->base->p_env);
	if (!envregion)
	{
		envregion = addr2region ((long) p->base->p_env);
		assert (envregion);

		attach_region (curproc, envregion);
	}
	else
		/* already attached, don't detach */
		envregion = NULL;

	from = env = p->base->p_env;

	/* Walk the array and find out the maximum length */
  	for (;;)
  	{
# ifdef ONLY030
		if (*(ushort *)from == 0)
			break;
# else
    		if (from[0] == '\0' && from[1] == '\0')
      			break;
# endif

# ifdef ONLY030

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
		detach_region (curproc, envregion);
	if (baseregion)
		detach_region (curproc, baseregion);

	*buffer = info;
	return ret;
}

long
kern_procdir_get_fname (SIZEBUF **buffer, const struct proc *p)
{
	SIZEBUF *info;
	ulong len;

	len = _mint_strlen (p->fname);

	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	memcpy (info->buf, p->fname, len);
	info->len = len;

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
	for (i = 0; i < p->p_mem->num_reg; i++)
	{
		MEMREGION *m;

		m = p->p_mem->mem[i];
		if (!m)
			continue;

# define M_SHTEXT	0x0010	/* XXX */
# define M_SHTEXT_T	0x0020	/* XXX */

		crs += ksprintf (crs, len - (crs - info->buf),
				"%08x-%08x %3u %c%c%c%c%c%c %08x %02lu:%02lu %lu\n",
				(ulong) p->p_mem->addr[i],
				(ulong) p->p_mem->addr[i] + m->len + 1,
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
kern_procdir_get_stat (SIZEBUF **buffer, struct proc *p)
{
	SIZEBUF *info;
	MEMREGION *baseregion;
	ulong len = 512;
	char state = '.';
	int ttypgrp = 0;
	struct timeval starttime;
	ulong endcode = 0xffffffffUL;
	ulong i;
	ulong sigign = 0;
	ulong sigcgt = 0;
	ulong bit = 1;
	TIMEOUT *t;
	TIMEOUT *first = NULL;
	long timeout = 0UL;

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
		for (i = 0; i < p->p_mem->num_reg; i++)
		{
			if ((void *) p->p_mem->addr[i] == (void *) p->base)
			{
				MEMREGION *m = p->p_mem->mem[i];

				if (m == NULL || m->loc != (long) p->base)
					break;

				endcode = ((ulong) (p->base) + m->len);
			}
		}
	}
	else
		endcode = 0;

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

	baseregion = proc_addr2region (curproc, (long) p->base);
	if (!baseregion)
	{
		baseregion = addr2region ((long) p->base);

		if (baseregion)
			attach_region (curproc, baseregion);
		else
			DEBUG (("kern_procdir_get_stat: no baseregion found, pid %i", p->pid));
	}
	else
		/* already attached, don't detach */
		baseregion = NULL;

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
				p->p_mem->memflags,

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
				(ulong) (p->base) + 256UL,
				endcode,
				endcode ? endcode - (ulong) p->base->p_bbase - (ulong) p->base->p_blen : 0,

				0UL, 0UL,
				(ulong) (p->sigpending >> 1),
				(ulong) (p->p_sigmask >> 1),
				(ulong) sigign,
				(ulong) sigcgt,
				(ulong) p->wait_q
	);

	if (baseregion)
		detach_region (curproc, baseregion);

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
kern_procdir_get_statm( SIZEBUF **buffer, PROC *p)
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


	// exchange LOG2_EIGHT_K with PAGEBITS. !!!
	// Why is it in 'kmemory.c' and not in 'kmemory.h' ?

	ksprintf (info->buf, len, "%lu %lu %lu %lu %lu %lu %lu\n",
		total >> LOG2_EIGHT_K,
		resid >> LOG2_EIGHT_K,
		share >> LOG2_EIGHT_K,
		txtrs >> LOG2_EIGHT_K,
		librs >> LOG2_EIGHT_K,
		datrs >> LOG2_EIGHT_K,
		dpage
	);
	info->len = len;

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
