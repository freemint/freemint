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
 * Author: Konrad M. Kokoszkiewicz <fnaumann@freemint.de>
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

# include "libkern/libkern.h"	/* ksprintf() */

# include "arch/mprot.h"	/* no_mem_prot */
# include "arch/timer.h"	/* get_hz_200() */
# include "arch/tosbind.h"	/* TOS calls */

# include "debug.h"		/* out_device, debug_level */
# include "global.h"		/* sysdir */
# include "info.h"		/* info strings */
# include "init.h"		/* boot_printf() */

/* Boot menu routines. Quite lame I admit. Added 19.X.2000. */

/* WARNING: all this is executed on TOS, before MiNT initialization.
 */

/* if the user is holding down the magic shift key, we ask before booting */
# define MAGIC_SHIFT	0x2		/* left shift */

# define MENU_OPTIONS	7
# define MAX_CMD_LEN	32

short load_xfs_f = 1;		/* Flag: load XFS modules (if 1) */
short load_xdd_f = 1;		/* Flag: load XDD modules (if 1) */
short load_auto = 1;		/* Flag: load AUTO programs appearing after us (if 1) */
short save_ini = 1;		/* Flag: write new ini file while exiting bootmenu (if 1) */

static short boot_delay = 4;	/* boot delay in seconds */

int boot_kernel_p(void);
void read_ini(void);
void pause_and_ask(void);

/* Helper function for getdelim() below */
INLINE int
get_a_char(int fd)
{
	char ch;

	if (TRAP_Fread(fd, 1, &ch) == 1)
		return (int)ch;
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

static void
do_ini_step(char *arg)
{
	step_by_step = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static long
emit_ini_step(short fd)
{
	char line[MAX_CMD_LEN];

	ksprintf(line, sizeof(line), "INI_STEP=%s\n", step_by_step ? "YES" : "NO");

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
	char msg[64];

	if (!isdigit(*arg))
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s requires number as an argument!\r\n", "DEBUG_LEVEL");
		TRAP_Cconws(msg);

		return;
	}

	val = atol(arg);

	if (val < 0 || val > 4)
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s value %ld is out of range (%d-%d)\r\n", "DEBUG_LEVEL", val, (short)0, (short)4);
		TRAP_Cconws(msg);

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
	char msg[64];

	if (!isdigit(*arg))
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s requires number as an argument!\r\n", "DEBUG_DEVNO");
		TRAP_Cconws(msg);

		return;
	}

	val = atol(arg);

	if (val < 0 || val > 9)
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s value %ld is out of range (%d-%d)\r\n", "DEBUG_DEVNO", val, (short)0, (short)9);
		TRAP_Cconws(msg);

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
	char msg[64];

	if (!isdigit(*arg))
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s requires number as an argument!\r\n", "BOOT_DELAY");
		TRAP_Cconws(msg);

		return;
	}

	val = atol(arg);

	if (val < 0 || val > 59)
	{
		ksprintf(msg, sizeof(msg), "mint.ini: %s value %ld is out of range (%d-%d)\r\n", "BOOT_DELAY", val, (short)0, (short)59);
		TRAP_Cconws(msg);

		return;
	}

	boot_delay = (short)val;
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
	"XFS_LOAD=", "XDD_LOAD=", "EXE_AUTO=", "MEM_PROT=", "INI_STEP=",
	"DEBUG_LEVEL=", "DEBUG_DEVNO=", "BOOT_DELAY=",
	"INI_SAVE=", NULL
};

static typeof(do_xfs_load) *do_func[] =
{
	do_xfs_load, do_xdd_load, do_exe_auto, do_mem_prot, do_ini_step,
	do_debug_level, do_debug_devno, do_boot_delay,
	do_ini_save
};

static typeof(emit_xfs_load) *emit_func[] =
{
	emit_xfs_load, emit_xdd_load, emit_exe_auto, emit_mem_prot, emit_ini_step,
	emit_debug_level, emit_debug_devno, emit_boot_delay,
	emit_ini_save
};

/* we assume that the mint.ini file, containing the boot
 * menu defaults, is located at same place as mint.cnf is
 */

INLINE short
find_ini (char *outp, long outsize)
{
	ksprintf(outp, outsize, "%s%s", sysdir, "mint.ini");

	if (TRAP_Fsfirst(outp, 0) == 0)
		return 1;

	return 0;
}

static void
write_ini (void)
{
	short inihandle;
	char ini_file[32], line[64];
	long r, x, l;

	ksprintf(ini_file, sizeof(ini_file), "%s%s", sysdir, "mint.ini");

	inihandle = TRAP_Fcreate(ini_file, 0);

	if (inihandle < 0)
		return;

	l = strlen(MSG_init_menuwarn);
	r = TRAP_Fwrite(inihandle, l, MSG_init_menuwarn);
	if ((r < 0) || (r != l))
	{
		r = -1;
		goto close;
	}

	x = 0;
	while (ini_keywords[x] && r > 0)
	{
		r = emit_func[x](inihandle);
		x++;
	}

close:

	TRAP_Fclose(inihandle);

	if (r < 0)
	{
		ksprintf(line, sizeof(line), "Error %ld writing %s\r\n", r, "mint.ini");

		TRAP_Cconws(line);
		TRAP_Fdelete(ini_file);
	}
}

void
read_ini (void)
{
	char *s, ini_file[32], line[MAX_CMD_LEN];
	long x;
	short inihandle;

	if (!find_ini(ini_file, sizeof(ini_file)))
		return;

	inihandle = TRAP_Fopen(ini_file, 0);

	if (inihandle >= 0)
	{
		while (getdelim(line, sizeof(line), '\n', inihandle) != -1)
		{
			if (line[0] && (line[0] != '#') && strchr(line, '='))
			{
				strupr(line);
				x = 0;
				while (ini_keywords[x])
				{
					s = strstr(line, ini_keywords[x]);
					if (s && (s == line))
					{
						while (*s && (*s != '='))
							s++;
						while (*s && (*s == '='))
							s++;
						if (*s)
						{
							do_func[x](s);
							break;
						}
					}
					x++;
				}

				if (ini_keywords[x] == NULL)
				{
					char msg[80];

					ksprintf(msg, sizeof(msg), "mint.ini: unknown command '%s'\r\n", line);
					TRAP_Cconws(msg);
				}
			}
		}
	}
	else
		write_ini();	/* if it doesn't exist, try to create */
}

int
boot_kernel_p (void)
{
	short option[MENU_OPTIONS];
	long c = 0;

	option[0] = 1;			/* Load MiNT or not */
	option[1] = load_xfs_f;		/* Load XFS or not */
	option[2] = load_xdd_f;		/* Load XDD or not */
	option[3] = load_auto;		/* Load AUTO or not */
	option[4] = !no_mem_prot;	/* Use memprot or not */
	option[5] = step_by_step;	/* Enter stepper mode */
	option[MENU_OPTIONS-1] = save_ini;

	for (;;)
	{
		boot_printf(MSG_init_bootmenu, \
			option[0] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[1] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[2] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[3] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[4] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[5] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[MENU_OPTIONS-1] ? MSG_init_menu_yesrn : MSG_init_menu_norn );
wait:
		c = TRAP_Crawcin();
		c &= 0x7f;
		switch(c)
		{
			case 0x03:
			{
				return 1;
			}
			case 0x0a:
			case 0x0d:
			{
				load_xfs_f = option[1];
				load_xdd_f = option[2];
				load_auto = option[3];
				no_mem_prot = !option[4];
				step_by_step = option[5];
				save_ini = option[MENU_OPTIONS-1];
				if (save_ini)
					write_ini();
				return (int)option[0];
			}
			case '0':
			{
				option[MENU_OPTIONS-1] = option[MENU_OPTIONS-1] ? 0 : 1;
				break;
			}
			case '1' ... '6':
			{
				option[(c - 1) & 0x0f] = option[(c - 1) & 0x0f] ? 0 : 1;
				break;
			}
			default:
				goto wait;
		}
	}

	return 1;	/* not reached */
}

void
pause_and_ask(void)
{
	long pause, newstamp;

	if (boot_delay)
	{
		boot_printf(MSG_init_askmenu, boot_delay);

		pause = Supexec(get_hz_200);
		pause += (boot_delay * HZ);

		do
		{
			newstamp = Supexec(get_hz_200);

			if ((TRAP_Kbshift(-1) & MAGIC_SHIFT) == MAGIC_SHIFT)
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

/* EOF */
