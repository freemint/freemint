/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# include "cnf.h"
# include "global.h"

# include "bios.h"
# include "dos.h"
# include "dosdir.h"
# include "dosfile.h"
# include "dosmem.h"
# include "kmemory.h"
# include "filesys.h"
# include "fatfs.h"
# include "block_IO.h"
# include "string.h"
# include "welcome.h"


/* move to global.h? */
extern short forcefastload;
extern int secure_mode;


/* program to run at startup */
# ifdef MULTITOS
int init_is_gem = 1;	/* set to 1 if init_prg is GEM */
# else
int init_is_gem = 0;	/* set to 1 if init_prg is GEM */
# endif

const char *init_prg = NULL;
char *init_env = NULL;
char init_tail[256];
/*
 * note: init_tail is also used as a temporary stack for resets in
 * intr.spp
 */


static void	do_file	(int fd);
static void	do_line	(char *line);
static void	doset	(char *name, char *val);


void
load_config (void)
{
	long fd;
	
	fd = f_open ("mint.cnf", O_RDONLY);
	if (fd < 0) fd = f_open("\\mint\\mint.cnf", O_RDONLY);
	if (fd < 0) fd = f_open("\\multitos\\mint.cnf", O_RDONLY);
	if (fd < 0) return;
	
	boot_print ("\r\nReading MiNT.CNF ...\r\n\r\n");
	do_file (fd);
	f_close (fd);
}


# undef BUF
# undef LINE

# define BUF 512
# define LINE 256

static void
do_file (int fd)
{
	long r;
	char buf[BUF+1], c;
	char line[LINE+1];
	char *from;
	int count = 0;

 	buf[BUF] = 0;
	from = &buf[BUF];
	line[LINE] = 0;

	for(;;)
	{
		c = *from++;
		if (!c)
		{
			r = f_read(fd, (long)BUF, buf);
			if (r <= 0) break;
			buf[r] = 0;
			from = buf;
		}
		else if (c == '\r')
		{
			continue;
		}
		else if (c == '\n')
		{
			line[count] = 0;
			do_line(line);
			count = 0;
		}
		else
		{
			if (count < LINE)
			{
				line[count++] = c;
			}
		}
	}
	if (count)
	{
		line[count] = 0;
		do_line(line);
	}
}

/*
 * Execute a line from the config file
 * 
 * echo message		-- print a message on the screen
 * alias drive path	-- make a fake drive pointing at a path
 * cd dir		-- change directory/drive
 * exec cmd args	-- execute a program
 * setenv name val	-- set up environment
 * sln file1 file2	-- create a symbolic link
 * ren file1 file2	-- rename a file
 *
 * BUG: if you use setenv in mint.cnf, *none* of the original environment
 * gets passed to children. This is rarely a problem if mint.prg is
 * in the auto folder.
 */

static void
do_line (char *line)
{
	/* temporary pointer into that environment for setenv */
	static char *env_ptr;
	/* length of the environment */
	static long env_len;
	
	char *cmd, *arg1, *arg2;
	char *newenv;
	char *t;
	int i;
	char delim;

	while (*line == ' ') line++;	/* skip whitespace at start of line */
	if (*line == '#') return;	/* ignore comments */
	if (!*line) return;		/* and also blank lines */

	cmd = line;
	
	/* check for variable assignments (e.g. INIT=, etc.)
	 *
	 * AGK: note we check for spaces whilst scanning so that an environment
	 * variable may include an =, this has the unfortunate side effect that
	 * the '=' _has_ to be concatenated to the variable name (INIT etc.)
	 */
	for (t = cmd; *t && *t != ' '; t++)
	{
		if (*t == '=')
		{
			*t++ = 0;
			doset(cmd, t);
			return;
		}
	}

	/* OK, assume a regular command;
	 * break it up into 'cmd', 'arg1', arg2'
	 */
	
	while (*line && *line != ' ') line++;
	delim = ' ';
	if (*line)
	{
		*line++ = 0;
		while (*line == ' ') line++;
		if (*line == '"')
		{
			delim = '"';
			line++;
		}
	}

	if (!strcmp(cmd, "echo"))
	{
		boot_print (line); boot_print ("\r\n");
		return;
	}
	
	arg1 = line;
	while (*line && *line != delim) line++;
	delim = ' ';
	if (*line)
	{
		*line++ = 0;
		while (*line == ' ') line++;
		if (*line == '"')
		{
			delim = '"';
			line++;
		}
	}
	
	if (!strcmp(cmd, "cd"))
	{
		int drv;
		(void)d_setpath(arg1);
		drv = DriveFromLetter((*arg1);
		if (arg1[1] == ':') (void)d_setdrv(drv);
		return;
	}
	
	if (!strcmp(cmd, "exec"))
	{
		char cmdline[128];
		int i;

		i = strlen(line);
		if (i > 126) i = 126;
		cmdline[0] = i;
		strncpy(cmdline+1, line, i);
		cmdline[i+1] = 0;
		i = (int)p_exec(0, arg1, cmdline, init_env);
		if (i == -33)
		{
			FORCE("%s: file not found", arg1);
		} else if (i < 0)
		{
			FORCE("%s: error while attempting to execute", arg1);
		}
		return;
	}
	
	if (!strcmp(cmd, "setenv"))
	{
		if (strlen(arg1) + strlen(line) + 4 + (env_ptr - init_env) > env_len)
		{
			long j;

			env_len += 1024;
			newenv = (char *)m_xalloc(env_len, 0x13);
			if (init_env)
			{
				t = init_env;
				j = env_ptr - init_env;
				env_ptr = newenv;
				for (i = 0; i < j; i++)
					*env_ptr++ = *t++;
				if (init_env)
					m_free((virtaddr)init_env);
			}
			else
			{
				env_ptr = newenv;
			}
			init_env = newenv;
		}
		while (*arg1)
		{
			*env_ptr++ = *arg1++;
		}
		*env_ptr++ = '=';
		while (*line)
		{
			*env_ptr++ = *line++;
		}
		*env_ptr++ = 0;
		*env_ptr = 0;
		return;
	}
	
	if (!strcmp (cmd, "include"))
	{
		long fd = f_open (arg1, O_RDONLY);
		if (fd < 0)
		{
			ALERT ("include: cannot open file %s", arg1);
			return;
		}
		do_file ((int)fd);
		f_close ((int)fd);
		return;
	}
	
	arg2 = line;
	while (*line && *line != delim) line++;
	if (*line)
	{
		*line = 0;
	}
	
	if (!strcmp(cmd, "alias"))
	{
		int drv;
		long r;
		fcookie root_dir;
		extern int aliasdrv[];
		
		drv = DriveFromLetter(*arg1);
		if (drv < 0 || drv >= NUM_DRIVES)
		{
			ALERT("Bad drive (%c:) in alias", *arg1);
			return;
		}
		r = path2cookie (arg2, NULL, &root_dir);
		if (r)
		{
			ALERT("alias: TOS error %ld while looking for %s", r, arg2);
			return;
		}
		aliasdrv[drv] = root_dir.dev + 1;
		*((long *)0x4c2L) |= (1L << drv);
		release_cookie (&curproc->curdir[drv]);
		dup_cookie (&curproc->curdir[drv], &root_dir);
		release_cookie (&curproc->root[drv]);
		curproc->root[drv] = root_dir;
		
		return;
	}
	
	if (!strcmp(cmd, "sln"))
	{
		(void)f_symlink(arg1, arg2);
		return;
	}
	
	if (!strcmp(cmd, "ren"))
	{
		(void)f_rename(0, arg1, arg2);
		return;
	}
	
	FORCE("syntax error in mint.cnf near: %s", cmd);
}

/*
 * routines for reading the configuration file
 * we allow the following commands in the file:
 * # anything		-- comment
 * INIT=file		-- specify boot program
 * CON=file		-- specify initial file/device for handles -1, 0, 1
 * PRN=file		-- specify initial file for handle 3
 * BIOSBUF=[yn]		-- if 'n' or 'N' then turn off BIOSBUF feature
 * DEBUG_LEVEL=n	-- set debug level to (decimal number) n
 * DEBUG_DEVNO=n	-- set debug device number to (decimal number) n
 * HARDSCROLL=n		-- set hard-scroll size to n, range 0-99.
 * SLICES=nnn		-- set multitasking granularity
 * UPDATE=n   		-- set the sync time in seconds for the system update daemon
 * SECURELEVEL=n        -- enables the appropriate security level
 * SINGLEMODE=[yn]      -- allow/disallow programs to switch off the scheduler
 * FASTLOAD=[yn]	-- force FASTLOAD for all programs, if YES.
 * NEWFATFS=n,n,n,...	-- activate NEW FAT-FS for specified drives
 * VFAT=n,n,n,...	-- activate VFAT extension for specified drives
 * VFATLCASE=[yn]	-- force return of FAT names in lower case
 * WB_ENABLE=n,n,n,...	-- enable write back mode for specified drives
 * CACHE=<size in kb>	-- set buffer cache to size
 * HIDE_B=		-- really remove drive B:
 */

static void
doset (char *name, char *val)
{
	char *t;
	
	if (!strcmp (name, "GEM"))
	{
		init_is_gem = 1;
		goto setup_init;
	}
	
	if (!strcmp (name, "INIT"))
	{
		init_is_gem = 0;
setup_init:
		if (!*val) return;
		t = kmalloc (strlen (val) + 1);
		if (!t) return;
		strcpy (t, val);
		init_prg = t;
		boot_printf("doset: %s = '%s'\r\n", name, t);
		while (*t && !isspace (*t)) t++;
		
		/* get the command tail, too */
		if (*t)
		{
			*t++ = 0;
			strncpy (init_tail + 1, t, 125);
			init_tail[126] = 0;
			init_tail[0] = strlen (init_tail + 1);
		}
		return;
	}
	
	if (!strcmp (name, "CON"))
	{
		FILEPTR *f;
		int i;
		
		f = do_open (val, O_RDWR, 0, (XATTR *) 0);
		if (f)
		{
			for (i = -1; i < 2; i++)
			{
				do_close(curproc->handle[i]);
				curproc->handle[i] = f;
				f->links++;
			}
			f->links--;	/* correct for overdoing it */
		}
		return;
	}
	
	if (!strcmp (name, "PRN"))
	{
		FILEPTR *f;
		
		f = do_open (val, O_RDWR|O_CREAT|O_TRUNC, 0, (XATTR *) 0);
		if (f)
		{
			do_close(curproc->handle[3]);
			do_close(curproc->prn);
			curproc->prn = curproc->handle[3] = f;
			f->links = 2;
		}
		return;
	}
	
	if (!strcmp (name, "AUX"))
	{
		FILEPTR *f;
		
		f = do_open (val, O_RDWR|O_CREAT|O_TRUNC, 0, (XATTR *) 0);
		if (f)
		{
			extern FILESYS bios_filesys;
			extern DEVDRV bios_tdevice;

			do_close (curproc->handle[2]);
			do_close (curproc->aux);
			curproc->aux = curproc->handle[2] = f;
			f->links = 2;
			if (is_terminal (f) && f->fc.fs == &bios_filesys &&
			    f->dev == &bios_tdevice &&
			    (has_bconmap ? (f->fc.aux>=6) : (f->fc.aux==1)))
			{
				if (has_bconmap)
					curproc->bconmap = f->fc.aux;
				((struct tty *)f->devinfo)->aux_cnt++;
				f->pos = 1;
			}
		}
		return;
	}
	
	if (!strcmp (name, "BIOSBUF"))
	{
		if (*val == 'n' || *val == 'N')
		{
			extern short bconbsiz;	/* from bios.c */
			extern short bconbdev;
			
			if (bconbsiz) bflush();
			bconbdev = -1;
		}
		return;
	}
	
# if 0
	if (!strcmp (name, "SINGLEMODE"))
	{
		extern short disallow_single;		/* from ssystem2.c */
		
		disallow_single = (*val == 'n' || *val == 'N');
		return;
	}
# endif
	
	if (!strcmp (name, "FASTLOAD"))
	{
		forcefastload = (*val == 'y' || *val == 'Y');
		return;
	}
	
	if (!strcmp (name, "SECURELEVEL"))
	{
		if (*val >= '0' && *val < '3')
		{
			secure_mode = (int) atol (val);
			(void) fatfs_config (0, FATFS_SECURE, secure_mode);
		}
		else
		{
			ALERT("Bad arg to \"SECURELEVEL\" in cnf file");
		}
		return;
	}
	
	if (!strcmp (name, "DEBUG_LEVEL"))
	{
		extern int debug_level;
		if (*val >= '0' && *val <= '9')
			debug_level = (int)atol(val);
		else ALERT("Bad arg to \"DEBUG_LEVEL\" in cnf file");
		return;
	}
	
	if (!strcmp (name, "DEBUG_DEVNO"))
	{
		extern int out_device;
		if (*val >= '0' && *val <= '9')
			out_device= (int)atol(val);
		else ALERT("Bad arg to \"DEBUG_DEVNO\" in cnf file");
		return;
	}
	
# ifdef FASTTEXT
	if (!strcmp (name, "HARDSCROLL"))
	{
		int i;
		extern int hardscroll;
		
		if (!strcmp (val, "AUTO"))
		{
			hardscroll = -1;
			return;
		}
		i = *val++;
		if (i < '0' || i > '9') return;
		hardscroll = i-'0';
		i = *val;
		if (i < '0' || i > '9') return;
		hardscroll = 10*hardscroll + i - '0';
		return;
	}
# endif
	
	if (!strcmp (name, "MAXMEM"))
	{
		long r;
		
		r = atol (val) * 1024L;
		if (r > 0)
			p_setlimit(2, r);
		return;
	}
	
	if (!strcmp (name, "SLICES"))
	{
		extern short time_slice;
		
		time_slice = atol (val);
		return;
	}
	
	/* uk: set update time for system update daemon */
	if (!strcmp (name, "UPDATE"))
	{
		extern long sync_time;
		
		sync_time = atol (val);
		return;
	}
	
	if (!strcmp (name, "NEWFATFS"))
	{
# ifdef FATFS_TESTING
		int flag = 1;
		while (*val)
		{
			*val = toupper (*val);
			switch (*val)
			{
				case 'A': case 'B': case 'C': case 'D': case 'E':
				case 'F': case 'G': case 'H': case 'I': case 'J':
				case 'K': case 'L': case 'M': case 'N': case 'O':
				case 'P': case 'Q': case 'R': case 'S': case 'T':
				case 'U': case 'V': case 'W': case 'X': case 'Y':
				case 'Z': case '1': case '2': case '3': case '4':
				case '5': case '6':
				{
					char drv = DriveFromLetter(*val);
					if (drv >= 0 && drv < NUM_DRIVES)
					{
						if (flag)
						{
							boot_printf ("\033p!!! INFORMATION !!!\033q\r\n");
							boot_printf ("NEWFATFS on drive %c", DriveToLetter(drv));
							flag = 0;
						}
						else
							boot_printf (", %c", DriveToLetter(drv));
						
						(void) fatfs_config (drv, FATFS_DRV, ENABLE);
					}
				}
			}
			val++;
		}
		
		if (!flag)
			boot_printf (" active.\r\n\r\n");
# else
		boot_printf ("\033p!!! INFORMATION !!!\033q\r\n");
		boot_printf ("NEWFATFS on all drives active (Default-FS).\r\n\r\n");
# endif
		return;
	}
	
	if (!strcmp (name, "VFAT"))
	{
		while (*val)
		{
			*val = toupper (*val);
			switch (*val)
			{
				case 'A': case 'B': case 'C': case 'D': case 'E':
				case 'F': case 'G': case 'H': case 'I': case 'J':
				case 'K': case 'L': case 'M': case 'N': case 'O':
				case 'P': case 'Q': case 'R': case 'S': case 'T':
				case 'U': case 'V': case 'W': case 'X': case 'Y':
				case 'Z': case '1': case '2': case '3': case '4':
				case '5': case '6':
				{
					char drv = DriveFromLetter(*val);
					if (drv >= 0 && drv < NUM_DRIVES)
						(void) fatfs_config (drv, FATFS_VFAT, ENABLE);
				}
			}
			val++;
		}
		return;
	}
	
	if (!strcmp (name, "VFATLCASE"))
	{
		if (*val == 'y' || *val == 'Y')
			fatfs_config (0, FATFS_VCASE, ENABLE);
		else
			fatfs_config (0, FATFS_VCASE, DISABLE);
		
		return;
	}
	
	if (!strcmp (name, "WB_ENABLE"))
	{
		while (*val)
		{
			*val = toupper (*val);
			switch (*val)
			{
				case 'A': case 'B': case 'C': case 'D': case 'E':
				case 'F': case 'G': case 'H': case 'I': case 'J':
				case 'K': case 'L': case 'M': case 'N': case 'O':
				case 'P': case 'Q': case 'R': case 'S': case 'T':
				case 'U': case 'V': case 'W': case 'X': case 'Y':
				case 'Z': case '1': case '2': case '3': case '4':
				case '5': case '6':
				{
					char drv = DriveFromLetter(*val);
					if (drv >= 0 && drv < NUM_DRIVES)
						(void) bio.config (drv, BIO_WB, ENABLE);
				}
			}
			val++;
		}
		
		return;
	}
	
	if (!strcmp (name, "CACHE"))
	{
		bio_set_cache_size (atol (val));
		return;
	}
	
	if (!strcmp (name, "HIDE_B"))
	{
		extern long dosdrvs;
		
		if (*val == 'Y' || *val == 'y')
		{		
			*((long *) 0x4c2L) &= ~2;	/* change BIOS map */
			dosdrvs &= ~2;			/* already initalized here */
		}
		
		return;
	}
	
	if (!strcmp (name, "HIDE"))
	{
		extern long dosdrvs;
		
		while (*val)
		{
			*val = toupper (*val);
			switch (*val)
			{
				case 'A': case 'B': case 'C': case 'D': case 'E':
				case 'F': case 'G': case 'H': case 'I': case 'J':
				case 'K': case 'L': case 'M': case 'N': case 'O':
				case 'P': case 'Q': case 'R': case 'S': case 'T':
				case 'U': case 'V': case 'W': case 'X': case 'Y':
				case 'Z': case '1': case '2': case '3': case '4':
				case '5': case '6':
				{
					int drv = DriveFromLetter(*val);
					if (drv >= 0 && drv < NUM_DRIVES)
					{
						long bit = 1;
						
						while (drv)
						{
							bit *= 2;
							drv--;
						}
						
						*((long *)0x4c2L) &= ~bit;
						dosdrvs &= ~bit;
					}
				}
			}
			val++;
		}
		
		return;
	}

	FORCE ("Unknown variable `%s'", name);
}
