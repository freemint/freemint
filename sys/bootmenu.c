/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Boot menu and mint.ini routines.
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
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Bootfile-option and minor changes: Helmut Karlowski 2012
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

# include "libkern/libkern.h"	/* ksprintf() */

# include "arch/mprot.h"	/* no_mem_prot */
# include "arch/timer.h"	/* get_hz_200() */
# include "arch/tosbind.h"	/* TOS calls */

# include "bootmenu.h"
# include "debug.h"		/* out_device, debug_level */
# include "global.h"		/* sysdir */
# include "info.h"		/* info strings */
# include "init.h"		/* boot_printf() */

/* Boot menu routines. Quite lame I admit. Added 19.X.2000. */

/* WARNING: all this is executed on TOS, before MiNT initialization.
 */

/* if the user is holding down the magic shift key, we ask before booting */
# define MAGIC_SHIFT	0x2		/* left shift */

# define MAX_CMD_LEN	32

short load_xfs_f = 1;		/* Flag: load XFS modules (if 1) */
short load_xdd_f = 1;		/* Flag: load XDD modules (if 1) */
short load_auto = 1;		/* Flag: load AUTO programs appearing after us (if 1) */

static short save_ini = 1;	/* Flag: write new ini file while exiting bootmenu (if 1) */
static short boot_delay = 1;	/* boot delay in seconds */
static const char *mint_ini = "mint.ini";
static short use_cmdline = 0;	/* use command line (only provided by EmuTOS if bootstrapped */
static char  *argsptr = 0L;


/* Helper function for getdelim() below */
INLINE int
get_a_char(int fd)
{
	char ch;

	if (fd > 0) {
		/* read mint.ini character */
		if (TRAP_Fread(fd, 1, &ch) == 1)
			return (int)ch;
	} else {
		/* next args character */
		if ( *argsptr )
			return *argsptr++;
	}
	return -1;
}

/* Simplified version of getdelim(): read data into specified buffer
 * until the specified delimiter is met, or EOF occurs. When the buffer
 * is full, it continues to read the file, but does not store the
 * information anywhere.
 *
 * Does not place the delimiter into the buffer.
 */
INLINE int
getdelim(char *buf, long n, int terminator, int fd)
{
	int ch;
	long len = 0;

	while ((ch = get_a_char(fd)) != -1)
	{
		if (ch == terminator)
			break;
		if ((len + 1) < n)
			buf[len++] = (char)ch;
	}

	buf[len] = 0;

	if (ch == -1)
	{
		if (len == 0)	/* Nothing read before EOF in this line */
			return -1;
		/* Pretend success otherwise */
	}

	return 0;
}

static const char *debug_levels[] =
{
	"(none)",
	"(ALERT)",
	"(ALERT, DEBUG)",
	"(ALERT, DEBUG, TRACE)",
	"(ALERT, DEBUG, TRACE, TRACELOW)"
};

static const char *debug_devices[] =
{
#define DEBUGDEV 9
	"(PRN:, printer)",
	"(AUX:, modem)",
	"(CON:, console)",
	"(MID:, midi)",
	"(KBD:, keyboard)",
	"(RAW:, raw console)",
	"", "", "", ""
};

static const char *write_boot_levels[] =
{
# ifdef ARANYM
	"(none)",
	"(File)",
	"(File/Host-Console)"
# define WBOOTLVL 2
# else
	"(no)",
	"(yes)"
# define WBOOTLVL 1
# endif
};

/* Pairs of functions handling each keyword. The do_xxx_yyyy() function
 * is called when the mint.ini is read; the `arg' points to the line buffer
 * containing the command, right after the '=' character.
 *
 * The emit_xxx_yyyy() function is called when the mint.ini is written out.
 * It is responsible for emitting the properly formatted keyword with
 * arguments into the specified file descriptor (fd).
 *
 */
static void
do_xfs_load(char *arg)
{
	load_xfs_f = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static long
emit_xfs_load(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "XFS_LOAD=%s\n", load_xfs_f ? "YES" : "NO");

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_xdd_load(char *arg)
{
	load_xdd_f = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static long
emit_xdd_load(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "XDD_LOAD=%s\n", load_xdd_f ? "YES" : "NO");

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_exe_auto(char *arg)
{
	load_auto = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static long
emit_exe_auto(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "EXE_AUTO=%s\n", load_auto ? "YES" : "NO");

	return TRAP_Fwrite(fd, strlen(line), line);
}

# ifdef WITH_MMU_SUPPORT
static void
do_mem_prot(char *arg)
{
	no_mem_prot = (strncmp(arg, "YES", 3) == 0) ? 0 : 1;	/* reversed */
}

static long
emit_mem_prot(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "MEM_PROT=%s\n", no_mem_prot ? "NO" : "YES");

	return TRAP_Fwrite(fd, strlen(line), line);
}
# endif

/* INI_STEP=YES makes step_by_step equal to -1 and acts traditionally.
 * INI_STEP=NO makes step by step equal to 0 and acts traditionally
 *
 * INI_STEP=number makes a delay of 'number' seconds on each
 * step point.
 */
static void
do_ini_step(char *arg)
{
	if (!isdigit(*arg))
	{
		if (strncmp(arg, "YES", 3) == 0)
			step_by_step = -1;
		else if (strncmp(arg, "NO", 2) == 0)
			step_by_step = 0;
		else
			boot_printf(MSG_init_syntax_error, mint_ini, "INI_STEP");
	}
	else
	{
		long val;

		val = atol(arg);

		if (val < 0 || val > 10)
		{
			boot_printf(MSG_init_value_out_of_range, mint_ini,
					"INI_STEP", val, (short)0, (short)10);

			return;
		}

		step_by_step = (short)val;
	}
}

static long
emit_ini_step(short fd)
{
	char line[MAX_CMD_LEN];

	if (step_by_step == -1)
		ksprintf(line, sizeof(line), "INI_STEP=%s\n", "YES");
	else if (step_by_step == 0)
		ksprintf(line, sizeof(line), "INI_STEP=%s\n", "NO");
	else
		ksprintf(line, sizeof(line), "INI_STEP=%d\n", step_by_step);

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_ini_save(char *arg)
{
	save_ini = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static long
emit_ini_save(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "INI_SAVE=%s\n", save_ini ? "YES" : "NO");

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_debug_level(char *arg)
{
	long val;

	if (!isdigit(*arg))
	{
		boot_printf(MSG_init_syntax_error, mint_ini, "DEBUG_LEVEL");

		return;
	}

	val = atol(arg);

	if (val < 0 || val > LOW_LEVEL)
	{
		boot_printf(MSG_init_value_out_of_range, mint_ini,
				"DEBUG_LEVEL", val, (short)0, (short)LOW_LEVEL);

		return;
	}

	debug_level = (int)val;
}

static long
emit_debug_level(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "DEBUG_LEVEL=%d\n", debug_level);

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_debug_devno(char *arg)
{
	long val;

	if (!isdigit(*arg))
	{
		boot_printf(MSG_init_syntax_error, mint_ini, "DEBUG_DEVNO");

		return;
	}

	val = atol(arg);

	if (val < 0 || val > 9)
	{
		boot_printf(MSG_init_value_out_of_range, mint_ini,
			 	"DEBUG_DEVNO", val, (short)0, (short)9);

		return;
	}

	out_device = (int)val;
}

static long
emit_debug_devno(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "DEBUG_DEVNO=%d\n", out_device);

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_boot_delay(char *arg)
{
	long val;

	if (!isdigit(*arg))
	{
		boot_printf(MSG_init_syntax_error, mint_ini, "BOOT_DELAY");

		return;
	}

	val = atol(arg);

	if (val < 0 || val > 59)
	{
		boot_printf(MSG_init_value_out_of_range, mint_ini,
				"BOOT_DELAY", val, (short)0, (short)59);

		return;
	}

	boot_delay = (short)val;
}

static long
emit_write_boot(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "WRITE_BOOT=%d\n", write_boot_file );

	return TRAP_Fwrite(fd, strlen(line), line);
}

static void
do_write_boot(char *arg)
{
	write_boot_file = *arg - '0';
	if( write_boot_file < 0 || write_boot_file > 2 )
		write_boot_file = 1;
}

static long
emit_boot_delay(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "BOOT_DELAY=%d\n", boot_delay);

	return TRAP_Fwrite(fd, strlen(line), line);
}

/* Keywords. Note that it is not necessary to keep the "XXX_YYYY=" scheme,
 * this is only some sort of convenience.
 */
static const char *ini_keywords[] =
{
	"XFS_LOAD=", "XDD_LOAD=", "EXE_AUTO=",
# ifdef WITH_MMU_SUPPORT
	"MEM_PROT=",
# endif
	"INI_STEP=",
	"DEBUG_LEVEL=", "DEBUG_DEVNO=", "BOOT_DELAY=",
	"WRITE_BOOT=",
	"INI_SAVE=",
	NULL
};

static typeof(do_xfs_load) *do_func[] =
{
	do_xfs_load, do_xdd_load, do_exe_auto,
# ifdef WITH_MMU_SUPPORT
	do_mem_prot,
# endif
	do_ini_step,
	do_debug_level, do_debug_devno, do_boot_delay, do_write_boot,
	do_ini_save
};

static typeof(emit_xfs_load) *emit_func[] =
{
	emit_xfs_load, emit_xdd_load, emit_exe_auto,
# ifdef WITH_MMU_SUPPORT
	emit_mem_prot,
# endif
	emit_ini_step,
	emit_debug_level, emit_debug_devno, emit_boot_delay,
	emit_write_boot,
	emit_ini_save
};

/* we assume that the mint.ini file, containing the boot
 * menu defaults, is located at same place as mint.cnf is
 */

static void
write_ini (void)
{
	char ini_file[128];
	long r;

	ksprintf(ini_file, sizeof(ini_file), "%s%s", sysdir, mint_ini);
	boot_printf(MSG_init_write, ini_file);

	/* try to delete it */
	TRAP_Fdelete(ini_file);

	/* and create a new one */
	r = TRAP_Fcreate(ini_file, 0);
	if (r >= 0)
	{
		short inihandle = r;
		int l;

		l = strlen(MSG_init_menuwarn);
		r = TRAP_Fwrite(inihandle, l, MSG_init_menuwarn);
		if (r >= 0)
		{
			if (r == l)
			{
				int x = 0;
				while (ini_keywords[x] && r > 0)
				{
					r = emit_func[x](inihandle);
					x++;
				}
			}
			else
				r = -1;
		}

		TRAP_Fclose(inihandle);
	}

	if (r < 0)
		goto error;

	boot_printf(MSG_init_rw_done);
	return;

error:
	boot_printf(MSG_init_rw_error, r);

	/* try to delete it */
	TRAP_Fdelete(ini_file);

	stop_and_ask();
}

static void
read_ini_file (short fd, char delimiter)
{
	char *s, line[MAX_CMD_LEN];

	while (getdelim(line, sizeof(line), delimiter, fd) != -1)
	{
		if (line[0] && (line[0] != '#') && strchr(line, '='))
		{
			int x;

			strupr(line);
			x = 0;
			while (ini_keywords[x])
			{
				s = strstr(line, ini_keywords[x]);
				if (s && (s == line))
				{
					s = strchr(line, '=');
					if (s)
					{
						s++;
						do_func[x](s);
						break;
					}
					else
						boot_printf(MSG_init_no_value, mint_ini, line);
				}
				x++;
			}

			if (ini_keywords[x] == NULL)
				boot_printf(MSG_init_unknown_cmd, mint_ini, line);
		}
	}

	boot_printf(MSG_init_rw_done);
}

void
read_ini (void)
{
	char ini_file[128];
	long r, usp;

	
	/* Ozk: This code crashed on hardware that correctly denies
	 *	user accesses to the low 2Kb of system RAM. Therefore
	 *	we must switch to Super before accessing the system-
	 *	variables. Do your homework, please :-)
	 */
	usp = TRAP_Super(1L);
	if (!usp)
		usp = TRAP_Super(0L);
	else
		usp = 0L;
	
	/* check and read the command line arguments - if not empty */
	argsptr = (*(BASEPAGE**)(*((long **)(0x4f2L)))[10])->p_cmdlin;

	if ( argsptr && strlen(argsptr) > 0 ) {
		use_cmdline = 1;
		save_ini = 0;

		strcpy(ini_file, "args: ");
		strcat(ini_file, argsptr);
		if (usp)
			TRAP_Super(usp);

		boot_printf(MSG_init_read, ini_file);

		
		read_ini_file( -1, ' ');
	} else {
		if (usp)
			TRAP_Super(usp);

		ksprintf(ini_file, sizeof(ini_file), "%s%s", sysdir, mint_ini);
		boot_printf(MSG_init_read, ini_file);

		r = TRAP_Fopen(ini_file, 0);
		if (r < 0)
		{
			boot_printf(MSG_init_rw_error, r);

			/* if it doesn't exist, try to create */
			write_ini();
		} else {
			read_ini_file( r, '\n');

			TRAP_Fclose(r);
		}
	}

	boot_printf("\r\n");
}

int
boot_kernel_p (void)
{
	int option[10];
	int modified;

	option[0] = 1;			/* Load MiNT or not */
	option[1] = load_xfs_f;		/* Load XFS or not */
	option[2] = load_xdd_f;		/* Load XDD or not */
	option[3] = load_auto;		/* Load AUTO or not */
# ifdef WITH_MMU_SUPPORT
	option[4] = !no_mem_prot;	/* Use memprot or not */
# endif
	option[5] = step_by_step;	/* Enter stepper mode */
	option[6] = debug_level;
	option[7] = out_device;
	option[8] = write_boot_file;
	option[9] = save_ini;

	modified = 0;

	for (;;)
	{
		int c, off;

		boot_printf(MSG_init_bootmenu, \
			option[0] ? MSG_init_menu_yesrn : MSG_init_menu_norn,
			option[1] ? MSG_init_menu_yesrn : MSG_init_menu_norn,
			option[2] ? MSG_init_menu_yesrn : MSG_init_menu_norn,
			option[3] ? MSG_init_menu_yesrn : MSG_init_menu_norn,
# ifdef WITH_MMU_SUPPORT
			option[4] ? MSG_init_menu_yesrn : MSG_init_menu_norn,
# endif
			( option[5] == -1 ) ? MSG_init_menu_yesrn : MSG_init_menu_norn,
			option[6], debug_levels[option[6]],
			option[7], debug_devices[option[7]],
			option[8], write_boot_levels[option[8]],
			option[9] ? MSG_init_menu_yesrn : MSG_init_menu_norn);

wait:
		c = TRAP_Crawcin();
		c &= 0x7f;

		off = ((c & 0x0f) - 1);	/* not used in all cases */

		switch (c)
		{
			case 0x03:
			{
				return 1;
			}
			case 0x0d: /* return */
			{
				if (modified)
				{
					load_xfs_f   =  option[1];
					load_xdd_f   =  option[2];
					load_auto    =  option[3];
# ifdef WITH_MMU_SUPPORT
					no_mem_prot  = !option[4];
# endif
					step_by_step =  option[5];
					debug_level  =  option[6];
					out_device   =  option[7];
					write_boot_file =  option[8];
					save_ini     =  option[9];

					if (save_ini)
						write_ini();
				}

				return option[0];
			}
			case '0':
			{
				option[9] = option[9] ? 0 : 1;

				modified = 1;
				break;
			}
			case '1':
			case '2':
			case '3':
			case '4':
# ifdef WITH_MMU_SUPPORT
			case '5':
# endif
			{
				option[off] = option[off] ? 0 : 1;

				modified = 1;
				break;
			}
			case '6':
			{
				option[off] = option[off] == -1 ? 0 : -1;

				modified = 1;
				break;
			}
			case '7':
			{
				int opt = option[off];

				opt++;
				if (opt > LOW_LEVEL)
					opt = 0;

				option[off] = opt;

				modified = 1;
				break;
			}
			case '8':
			{
				int opt = option[off];

				opt++;
				if (opt > DEBUGDEV)
					opt = 0;

				option[off] = opt;

				modified = 1;
				break;
			}
			case '9':
			{
				int opt = option[off];

				opt++;
				if (opt > WBOOTLVL)
					opt = 0;

				option[off] = opt;
				modified = 1;
				break;
			}
			default:
				goto wait;
		}
	}

	/* not reached */
	return 1;
}

void
pause_and_ask(void)
{
	long pause, newstamp;

	if (boot_delay)
	{
		boot_printf(MSG_init_askmenu, boot_delay);

		pause = TRAP_Supexec(get_hz_200);
		pause += (boot_delay * HZ);

		do {
			newstamp = TRAP_Supexec(get_hz_200);
			if (TRAP_Kbshift(-1) == MAGIC_SHIFT)
			{
				long yn = boot_kernel_p ();
				if (!yn)
					TRAP_Pterm0 ();
				newstamp = pause;
			}
		}
		while (newstamp < pause);
	}
}
