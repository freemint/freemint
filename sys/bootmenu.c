/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1998, 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Konrad M. Kokoszkiewicz <fnaumann@freemint.de>
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

# include "libkern/libkern.h"	/* ksprintf() */

# include "arch/mprot.h"	/* no_mem_prot */
# include "arch/tosbind.h"	/* TOS calls */

# include "global.h"		/* sysdir */
# include "info.h"		/* info strings */
# include "init.h"		/* boot_printf() */

/* Boot menu routines. Quite lame I admit. Added 19.X.2000. */

/* WARNING: all this is executed on TOS, before MiNT initialization.
 */

# define MENU_OPTIONS	7

short load_xfs_f = 1;
short load_xdd_f = 1;
short load_auto = 1;
short save_ini = 1;

int boot_kernel_p(void);
void read_ini(void);

static int
get_a_char(int fd)
{
	char ch;

	if (TRAP_Fread(fd, 1, &ch) == 1)
		return (int)ch;
	return -1;
}

static int
getdelim(char *buf, long n, int terminator, int fd)
{
	int ch;
	long len = 0;

	while ((ch = get_a_char(fd)) != -1)
	{
		if ((len + 1) >= n)
			break;
		if (ch == terminator)
			break;
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

/* we assume that the mint.ini file, containing the boot
 * menu defaults, is located at same place as mint.cnf is
 */

INLINE short
find_ini (char *outp, long outsize)
{
	ksprintf(outp, outsize, "%smint.ini", sysdir);

	if (TRAP_Fsfirst(outp, 0) == 0)
		return 1;

	return 0;
}

static void
do_xfs_load(char *arg)
{
	load_xfs_f = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static void
do_xdd_load(char *arg)
{
	load_xdd_f = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static void
do_exe_auto(char *arg)
{
	load_auto = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static void
do_mem_prot(char *arg)
{
	no_mem_prot = (strncmp(arg, "YES", 3) == 0) ? 0 : 1;	/* reversed */
}

static void
do_ini_step(char *arg)
{
	step_by_step = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static void
do_ini_save(char *arg)
{
	save_ini = (strncmp(arg, "YES", 3) == 0) ? 1 : 0;
}

static const char *ini_keywords[] =
{
	"XFS_LOAD=", "XDD_LOAD=", "EXE_AUTO=", "MEM_PROT=", "INI_STEP=", "INI_SAVE=", NULL
};

static typeof(do_xfs_load) *func[] =
{
	do_xfs_load, do_xdd_load, do_exe_auto, do_mem_prot, do_ini_step, do_ini_save
};

void
read_ini (void)
{
	char *s, ini_file[32], line[256];
	long x;
	short inihandle;

	if (!find_ini(ini_file, sizeof(ini_file)))
		return;

	inihandle = TRAP_Fopen(ini_file, 0);

	if (inihandle >= 0)
	{
		while (getdelim(line, sizeof(line), '\n', inihandle) != -1)
		{
			if (line[0] != '#')
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
						if (*s)
							func[x](++s);
					}
					x++;
				}
			}
		}
	}
}

INLINE void
write_ini (short *options)
{
	short inihandle;
	char ini_file[32], line[64];
	long r, x, l;

	ksprintf(ini_file, sizeof(ini_file), "%smint.ini", sysdir);

	inihandle = TRAP_Fcreate(ini_file, 0);

	if (inihandle < 0)
		return;

	options++;		/* Ignore the first one :-) */

	l = strlen(MSG_init_menuwarn);
	r = TRAP_Fwrite(inihandle, l, MSG_init_menuwarn);
	if ((r < 0) || (r != l))
	{
		r = -1;
		goto close;
	}

	for (x = 0; x < MENU_OPTIONS-1; x++)
	{
		ksprintf(line, sizeof (line), "%s=%s\n", \
			ini_keywords[x], options[x] ? "YES" : "NO");
		l = strlen(line);
		r = TRAP_Fwrite(inihandle, l, line);
		if ((r < 0) || (r != l))
		{
			r = -1;
			break;
		}
	}

close:
	TRAP_Fclose(inihandle);

	if (r < 0)
		TRAP_Fdelete(ini_file);
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
	option[6] = save_ini;

	for (;;)
	{
		boot_printf(MSG_init_bootmenu, \
			option[0] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[1] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[2] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[3] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[4] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[5] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[6] ? MSG_init_menu_yesrn : MSG_init_menu_norn );
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
				save_ini = option[6];
				if (save_ini)
					write_ini(option);
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

/* EOF */
