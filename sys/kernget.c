/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * begin:	1999-10-24
 * last change: 2000-04-08
 * 
 * Author: Guido Flohr <guido@freemint.de>
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

# include "arch/mprot.h"
# include "arch/timer.h"
# include "buildinfo/version.h"
# include "libkern/libkern.h"
# include "mint/signal.h"

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
# include "resource.h"
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
	
	if (get_cookie (0, &slots))
	{
		DEBUG (("kern_get_cmdline: get_cookie(0) failed!"));
		return ENOMEM;
	}
	
	for (i = 1; i < slots; i++)
	{
		if (get_cookie (i, &value) || (value == 0UL))
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
		
		get_cookie (i, (ulong *) name);
		
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
		get_cookie (*tagid, &value);
		
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

/* This is mostly stolen from linux-m68k and should be adapted to MiNT
 */
long 
kern_get_cpuinfo (SIZEBUF **buffer)
{
	SIZEBUF *info;
	ulong len = 256, clkspeed, clkdiv, div = 0;
	extern ulong bogomips[2];
	char *cpu, *mmu, *fpuname;
	
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;

	cpu = "68000";
	fpuname = mmu = "none";

	/* Average number of cycles per instruction in __delay() */
	clkdiv = 9;	/* 8/10, 68000 is sloooooow */

	switch (mcpu)
	{
		case 10:
			cpu = "68010";
			break;
		case 20:
			cpu = "68020";
			clkdiv = 4;		/* 2/6 */
			break;
		case 30:
			cpu = mmu = "68030";
			clkdiv = 4;		/* 2/6 */
			break;
		case 40:
			cpu = mmu = "68040";
			clkdiv = 2;		/* 2/2 */
			break;
		case 60:
			cpu = mmu = "68060";
			div = 1;		/* divide, don't multiply */
			clkdiv = 2;		/* 1/0 (predicted branch) */
			break;

		/* Add more processors here */

		default:
			cpu = "680x0";
			break;
	}
	
	if (fpu)
	{
		switch (fputype >> 16)
		{
			case 0x02:
				fpuname = "68881/82";
				break;
			case 0x04:
				fpuname = "68881";
				break;
			case 0x06:
				fpuname = "68882";
				break;
			case 0x08:
				fpuname = "68040";
				break;
			case 0x10:
				fpuname = "68060";
				break;
			default:
				fpuname = "680x0";
				break;
		}
	}
	
# ifdef ONLY030
	/* Round the fractional part up to an unit,
	 * add units, then mul everything by clkdiv */
	clkspeed = ((bogomips[1] + 50)/100) + bogomips[0];

	/* mc68060 can execute more than 1 instruction in a cycle
	 * and this is the case. So we must div, not mul.
	 */
	if (div)
		clkspeed /= clkdiv;
	else
		clkspeed *= clkdiv;
# else
	/* This is:
	 * bogomips = (clkspeed - ((0.24/2) * 4)) / clkdiv
	 * The subtracted value is calculated out of the
	 * correction value on a 16 Mhz 68030 (0.24)
	 * scaled down according to the clock frequency
	 * then multiplied as many times as 68000 is
	 * slower than 68030 :-)
	 * Notice this stuff has no real meaning on ST, it has
	 * a visual purpose only.
	 */
	clkspeed = 8;
	bogomips[0] = 0;
	bogomips[1] = 83;
# endif

	info->len = ksprintf (info->buf, len,
				"CPU:\t\t%s\n"
				"MMU:\t\t%s\n"
				"FPU:\t\t%s\n"
				"Clockspeed:\t %lu MHz\n"
				"BogoMIPS:\t %lu.%02lu\n",
				cpu, mmu, fpuname, clkspeed,
				bogomips[0], bogomips[1]);
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
				if (rootproc->root[i].fs == fs)
					break;
			
			if (i < NUM_DRIVES)
			{
				fcookie dir;
				
				dup_cookie (&dir, &(rootproc->root[i]));
				
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
	char buf[32];
	ulong len;
	PROC *p;
	short tasks = 0;
	short running = 0;
# if 0
	extern int last_pid; /* in util.c */
# else
	int last_pid = 0; /* ??? */
# endif 
	
	DEBUG (("kern_get_loadavg"));
	
	for (p = proclist; p; p = p->gl_next)
	{
		if (p == rootproc)
			continue;
		
		tasks++;
		
		if ((p->wait_q == CURPROC_Q) || (p->wait_q == READY_Q))
			running++;
	}
	
	len = ksprintf (buf, sizeof (buf), "%lu.%03lu %lu.%03lu %lu.%03lu %d/%d %d\n",
			LOAD_INT (avenrun[0]), LOAD_FRAC (avenrun[0]),
			LOAD_INT (avenrun[1]), LOAD_FRAC (avenrun[1]),
			LOAD_INT (avenrun[2]), LOAD_FRAC (avenrun[2]),
			running, tasks, last_pid);
	
	info = kmalloc (sizeof (*info) + len);
	if (!info)
	{
		DEBUG (("kern_get_loadavg: virtual memory exhausted"));
		return ENOMEM;
	}
	
	/* Not null-terminated! */
	info->len = len;
	memcpy (info->buf, buf, len);
	
	*buffer = info;
	return 0;
}

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
	ulong idle = rootproc->systime + rootproc->usrtime;
	char buf[32];
	ulong len;
	
	len = ksprintf (buf, sizeof (buf), "%lu.%03d %lu.%03lu\n",
			uptime,
			1000 - uptimetick * 5,
			idle / 1000,
			idle % 1000);
	
	info = kmalloc (sizeof (*info) + len);
	if (!info)
	{
		DEBUG (("kern_get_uptime: virtual memory exhausted"));
		return ENOMEM;
	}
	
	/* Not null-terminated! */
	memcpy (info->buf, buf, len);
	info->len = len;
	
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
kern_procdir_get_cmdline (SIZEBUF **buffer, const PROC *p)
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
kern_procdir_get_environ (SIZEBUF **buffer, PROC *p)
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
			addr2mem (curproc, (virtaddr) p->base),
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
    		if (from[0] == '\0' && from[1] == '\0')
      			break;
    		
    		if (from[0] == 'A' && from[1] == 'R' && from[2] == 'G' &&
        	    from[3] == 'V' && from[4] == '=')
      			break;
    		
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
kern_procdir_get_fname (SIZEBUF **buffer, const PROC *p)
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
kern_procdir_get_meminfo (SIZEBUF **buffer, const PROC *p)
{
	SIZEBUF *info;
	char *crs;
	ulong len;
	int i;
	
	len = p->num_reg * 64;
	
	info = kmalloc (sizeof (*info) + len);
	if (!info)
		return ENOMEM;
	
	crs = info->buf;
	for (i = 0; i < p->num_reg; i++)
	{
		MEMREGION *mem = p->mem[i];
		
		crs += ksprintf (crs, len - (crs - info->buf),
				"%08x-%08x %3u %c%c%c%c%c%c %08x %02lu:%02lu %lu\n",
				(ulong) p->addr[i],
				(ulong) p->addr[i] + mem->len + 1,
				(unsigned) mem->links,
				mem->mflags & M_KEEP ? 'k' : '-',
				mem->mflags & M_SHARED ? 's' : '-',
				mem->mflags & M_FSAVED ? 'f' : '-',
				mem->mflags & M_SHTEXT_T ? 't' : '-',
				mem->mflags & M_SHTEXT ? 's' : '-',
				mem->mflags & M_CORE ? 'c' :
				mem->mflags & M_ALT ? 'a' :
				mem->mflags & M_SWAP ? 's' :
				mem->mflags & M_KER ? 'k' : '-',
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
get_session_id (const PROC *p)
{
	while (p && p != rootproc)
	{
		PROC *leader = pid2proc (p->pgrp);
		
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
kern_procdir_get_stat (SIZEBUF **buffer, PROC *p)
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
	
	if (p->control)
	{
		struct tty *tty = (struct tty *) p->control->devinfo;
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
	
	for (i = 0; i < p->num_reg; i++)
	{
		if ((void *) p->addr[i] == (void *) p->base)
		{
			MEMREGION *m = p->mem[i];
			
			if (m == NULL || m->loc != (long) p->base)
				break;
			
			endcode = ((ulong) (p->base) + m->len);
		}
	}
	
	for (i = 1; i < NSIG; i++)
	{
		switch (p->sighandle [i])
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
				p->memflags,
				
				0UL, 0UL, 0UL, 0UL, /* Page faults */
				p->systime, p->usrtime, p->chldstime, p->chldutime,
				(int) (p->slices * time_slice * 20),
				(int) -(p->pri),
				
				(long) timeout / 5,
				(long) p->alarmtim->when / 5,
				(long) p->itimer->timeout->when / 5,
				(long) starttime.tv_sec * 200L + (long) starttime.tv_usec / 5000L,
				(ulong) memused ((PROC *) p),
				(ulong) memused ((PROC *) p),  /* rss */
				0x7fffffffUL,  /* rlim */
				(ulong) (p->base) + 256UL,
				endcode,
				endcode - (ulong) p->base->p_bbase - (ulong) p->base->p_blen,
				
				0UL, 0UL,
				(ulong) (p->sigpending >> 1), 
				(ulong) (p->sigmask >> 1), 
				(ulong) sigign, 
				(ulong) sigcgt,
				(ulong) p->wait_q
	);
	
	if (baseregion)
		detach_region (curproc, baseregion);
	
	*buffer = info;
	return 0;
}

long 
kern_procdir_get_status (SIZEBUF **buffer, const PROC *p)
{
	SIZEBUF *info;
	ulong len = 512;
	char *crs;
	ulong memsize = 0;
	ulong coresize = 0;
	ulong altsize = 0;
	ulong swapsize = 0;
	ulong kersize = 0;
	ulong shtextsize = 0;
	ulong shtexttsize = 0;
	ulong fsavedsize = 0;
	ulong sharedsize = 0;
	ulong keepsize = 0;
	ulong kmallocsize = 0;
	char *state = ". Huh?";
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
	}
	
	crs += ksprintf (crs, len - (crs - info->buf), "State:\t%s\n", state);
	
	crs += ksprintf (crs, len - (crs - info->buf), "Pid:\t%d\nPPid:\t%d\nUid:\t%d\t%d\t%d\nGid:\t%d\t%d\t%d\n",
		         p->pid, p->ppid,
		         p->ruid, p->euid, p->suid,
		         p->rgid, p->egid, p->sgid);
	
	for (i = 0; i < p->num_reg; i++)
	{
		MEMREGION *mem = p->mem[i];
		
		memsize += mem->len;
		switch (mem->mflags & M_MAP)
		{
			case M_CORE:
				coresize += mem->len;
				break;
			case M_ALT:
				altsize += mem->len;
				break;
			case M_SWAP:
				swapsize += mem->len;
				break;
			case M_KER:
				kersize += mem->len;
				break;
		}
		
		if (mem->mflags & M_SHTEXT)
			shtextsize += mem->len;
		if (mem->mflags & M_SHTEXT_T)
			shtexttsize += mem->len;
		if (mem->mflags & M_FSAVED)
			fsavedsize += mem->len;
		if (mem->mflags & M_SHARED)
			sharedsize += mem->len;
		if (mem->mflags & M_KEEP)
			keepsize += mem->len;
		if (mem->mflags & M_KMALLOC)
			kmallocsize += mem->len;
	}
	
	crs += ksprintf (crs, len - (crs - info->buf),
			"VmSize:\t%8lu kB\n"
			"VmCore:\t%8lu kB\n"
			"VmAlt:\t%8lu kB\n"
			"VmSwap:\t%8lu kB\n"
			"VmKer:\t%8lu kB\n"
			"NumReg:\t%8u \n",
			memsize >> 10,
			coresize >> 10,
			altsize >> 10, 
			swapsize >> 10,
			kersize >> 10,
			(unsigned) p->num_reg
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
	
	for (i = 1; i < NSIG; i++)
	{
		switch (p->sighandle[i])
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
	
	crs += ksprintf (crs, len - (crs - info->buf),
			"SigPnd:\t%08lx\n"
			"SigBlk:\t%08lx\n"
			"SigIgn:\t%08lx\n"
			"SigCgt:\t%08lx\n",
			p->sigpending >> 1,
			p->sigmask >> 1,
			sigign >> 1,
			sigcgt >> 1
	);
	
	info->len = crs - info->buf;
	
	*buffer = info;
	return 0;
}

# endif /* WITH_KERNFS */
