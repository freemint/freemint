/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999 Ralph Lowinski <ralph@aquaplan.de>
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
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
 */

# include "cnf_mint.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/filedesc.h"
# include "arch/mprot.h"

# include "bios.h"
# include "block_IO.h"
# include "cnf.h"
# include "dosdir.h"
# include "dosfile.h"
# include "dosmem.h"
# include "fatfs.h"
# include "filesys.h"
# include "info.h"		/* messages */
# include "init.h"
# include "k_exec.h"
# include "k_fds.h"
# include "k_resource.h"
# include "keyboard.h"
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "update.h"
# include "xbios.h"


/* program to run at startup */
#define INIT_IS_GEM   1
#define INIT_IS_PRG   0
int init_is_gem = INIT_IS_PRG;	/* set to 1 if init_prg is GEM (ROM or external) */

char *init_prg = NULL;
char init_tail[256];
/*
 * note: init_tail is *NOT* used as a temporary stack for resets in
 * intr.spp (it was before 1.16)
 */


/*** First the command callback declarations: */

/*
 * set [+-n] ........... set option on(-) or off(+):
 *			 c - esc interpr. out off quota
 *			 q - silent output
 *			 v - print comand lines
 * alias drive path .... make a fake drive pointing at a path
 * cd dir .............. change directory/drive
 * echo message ........ print message on the screen
 * exec cmd args ....... execute a program
 * include file ........ include another config file
 * ren file1 file2 ..... rename a file
 * setenv name val ..... set up environment
 * sln file1 file2 ..... create a symbolic link
 */

static PCB_Ax	pCB_set;		/* set [+-c][+-q][+-v]	*/
static PCB_TTx	pCB_alias;		/* alias drive path	*/
static PCB_T	pCB_cd;			/* cd dir		*/
static PCB_A	pCB_echo;		/* echo message		*/
static PCB_TAx	pCB_exec;		/* exec cmd args	*/
static PCB_Tx	pCB_include;		/* include file		*/
/*     PCB_0TT	f_rename;	*/	/* ren file1 file2	*/
static PCB_TTx	pCB_setenv;		/* setenv name val	*/
/*     PCB_TT	f_symlink;	*/	/* sln file1 file2	*/

/* BUG: if you use setenv in mint.cnf, *none* of the original environment gets
 * passed to children. This is rarely a problem if mint.prg is in the auto
 * folder.
 */


/*** Now the variable callback declarations: */

/*
 * AUX=file ............ specify initial file/device for handle 2
 * BIOSBUF=[yn] ........ turn on/off bios buffer feature
 * CACHE=n ............. set buffer cache to size in kb
 * CON=file ............ specify initial file/device for handles -1, 0, 1
 * DEBUG_DEVNO=n ....... set debug device number to (decimal number) n
 * DEBUG_LEVEL=n ....... set debug level to (decimal number) n
 * FASTLOAD=[yn] ....... force FASTLOAD for all programs, if YES
 * GEM=file ............ specify AES driver
 * HIDE_B=[yn] ......... really remove drive B
 * INIT=file ........... specify boot program
 * INITIALMEM=n ........ set maximum additional TPA size for new processes
 * MAXMEM=n ............ set memory maximum per process
 * MPFLAGS=bitvector ... set flags for mem protection, bit 0: strict mode on/off
 * NEWFATFS=<drives> ... activate NEW FAT-FS for specified drives
 * PERCENTAGE=n ........ set max. percentage of cache to fill with linear reads
 * PRN=file ............ specify initial file for handle 3
 * SECURELEVEL=n ....... enables the appropriate security level, range 0-2
 * SLICES=n ............ set multitasking granularity
 * UPDATE=n ............ set sync time in seconds for the system update daemon
 * VFAT=<drives> ....... activate VFAT extension for specified drives
 * VFATLCASE=[yn] ...... force return of FAT names in lower case
 * WB_ENABLE=<drives> .. enable write back mode for specified drives
 * WRITEPROTECT=<drives> enable software write protection for specified drives
 *
 * dependant on global compile time switches:
 *
 * HARDSCROLL=n ........ set hard-scroll size to n, range 0-99
 *
 * New format:
 *
 * GEMDOS_AUX=file .............. specify initial file/device for handle 2
 * GEMDOS_CON=file .............. specify initial file/device for handles -1, 0, 1
 * GEMDOS_PRN=file .............. specify initial file for handle 3
 * FS_CACHE_SIZE=n .............. set buffer cache to size in kb
 * FS_CACHE_PERCENTAGE=n ........ set max. percentage of cache to fill with linear reads
 * FS_WB_ENABLE=<drives> ........ enable write back mode for specified drives
 * FS_WRITE_PROTECT=<drives> .... enable software write protection for specified drives
 * FS_UPDATE=n .................. set sync time in seconds for the system update daemon
 * FS_VFAT=<drives> ............. activate VFAT extension for specified drives
 * FS_VFAT_LCASE=[yn] ........... force return of FAT names in lower case
 * FS_NEWFATFS=<drives> ......... activate NEW FAT-FS for specified drives
 * KERN_BIOSBUF=[yn] ............ turn on/off bios buffer feature
 * KERN_DEBUG_DEVNO=n ........... set debug device number to (decimal number) n
 * KERN_DEBUG_LEVEL=n ........... set debug level to (decimal number) n
 * KERN_MPFLAGS=bitvector ....... set flags for mem protection, bit 0: strict mode on/off
 * KERN_SECURITY_LEVEL=n ........ enables the appropriate security level, range 0-2
 * KERN_SLICES=n ................ set multitasking granularity
 * PROC_MAXMEM=n ................ set memory maximum per process
 * TPA_FASTLOAD=[yn] ............ force FASTLOAD for all programs, if YES
 * TPA_INITIALMEM=n ............. set maximum additional TPA size for new processes
 * FDC_HIDE_B=[yn] .............. really remove drive B
 * KBD_AT_CAPS=[yn] ............. control the Caps lock key operation
 *
 * GEM=file ..................... specify AES driver
 * INIT=file .................... specify boot program
 *
 */

static PCB_T	pCB_aux;		/* AUX=file		*/
static PCB_B	pCB_biosbuf;		/* BIOSBUF=[y|n]	*/
static PCB_T	pCB_con;		/* CON=file		*/
/*     int	out_device;		 * DEBUG_DEVNO=n	*/
/*     int	debug_level;		 * DEBUG_LEVEL=n	*/
/*     short	forcefastload;		 * FASTLOAD=[yn]	*/
static PCB_ATK	pCB_gem_init;		/* GEM=file | INIT=file	*/
static PCB_B	pCB_hide_b;		/* HIDE_B=[yn]		*/
/*     ulong	initialmem;		 * INITIALMEM=n		*/
static PCB_L	pCB_maxmem;		/* MAXMEM=n		*/
/*     ulong	mem_prot_flags;		 * MPFLAGS=bitvector	*/
static PCB_Dx	pCB_newfatfs;		/* NEWFATFS=<drives>	*/
static PCB_T	pCB_prn;		/* PRN=file		*/
static PCB_L	pCL_securelevel;	/* SECURELEVEL=n	*/
/*     short	time_slice;		 * SLICES=n		*/
/*     long	sync_time;		 * UPDATE=n		*/
static PCB_Dx	pCB_vfat;		/* VFAT=<drives>	*/
static PCB_B	pCB_vfatlcase;		/* VFATLCASE=[yn]	*/
static PCB_Dx	pCB_wb_enable;		/* WB_ENABLE=<drives>	*/
static PCB_Dx	pCB_writeprotect;	/* WRITEPROTECT=<drives>*/

/* The item table, note the 'NULL' entry at the end. */

static struct parser_item parser_tab[] =
{
	/* Shell-like commands */

	{ "SET",	PI_C_A,		pCB_set		},
	{ "ALIAS",	PI_C_TT,	pCB_alias	},
	{ "CD",		PI_C_T,		pCB_cd		},
	{ "ECHO",	PI_C_A,		pCB_echo	},
	{ "EXEC",	PI_C_TA,	pCB_exec	},
	{ "INCLUDE",	PI_C_T,		pCB_include	},
	{ "REN",	PI_C_0TT,	sys_f_rename	},
	{ "SETENV",	PI_C_TT,	pCB_setenv	},
	{ "SLN",	PI_C_TT,	sys_f_symlink	},

	/* New format */

	{ "GEMDOS_AUX",			PI_V_T,	pCB_aux				},
	{ "GEMDOS_CON",			PI_V_T,	pCB_con				},
	{ "GEMDOS_PRN",			PI_V_T,	pCB_prn				},
	{ "FS_CACHE_SIZE",		PI_V_L,	bio_set_cache_size		},
	{ "FS_CACHE_PERCENTAGE",	PI_V_L,	bio_set_percentage		},
	{ "FS_UPDATE",			PI_R_L,	& sync_time			},
	{ "FS_VFAT",			PI_V_D,	pCB_vfat			},
	{ "FS_VFAT_LCASE",		PI_V_B,	pCB_vfatlcase			},
	{ "FS_WB_ENABLE",		PI_V_D,	pCB_wb_enable			},
	{ "FS_WRITE_PROTECT",		PI_V_D,	pCB_writeprotect		},
	{ "FS_NEWFATFS",		PI_V_D,	pCB_newfatfs			},
	{ "KERN_BIOSBUF",		PI_V_B,	pCB_biosbuf			},
	{ "KERN_DEBUG_DEVNO",		PI_R_S,	& out_device, Range(0, 9) 	},
	{ "KERN_DEBUG_LEVEL",		PI_R_S,	& debug_level, Range(0, 9) 	},
# ifdef WITH_MMU_SUPPORT
	{ "KERN_MPFLAGS",		PI_R_L,	& mem_prot_flags		},
# endif
	{ "KERN_SECURITY_LEVEL",	PI_V_L,	pCL_securelevel, Range(0, 2)	},
	{ "KERN_SLICES",		PI_R_S,	& time_slice			},
	{ "PROC_MAXMEM",		PI_V_L,	pCB_maxmem			},
	{ "TPA_FASTLOAD",		PI_R_B,	& forcefastload			},
	{ "TPA_INITIALMEM",		PI_R_L,	& initialmem			},
	{ "ALLOW_SETEXC",	PI_R_B,	& allow_setexc },
	{ "FDC_HIDE_B",			PI_V_B,	pCB_hide_b			},
# ifndef NO_AKP_KEYBOARD
	{ "KBD_AT_CAPS",		PI_R_B,	& kbd_pc_style_caps		},
# endif

	/* These two remain the same as in the old format */

	{ "GEM",	  PI_V_ATK, pCB_gem_init, {dat: INIT_IS_GEM}		},
	{ "INIT",	  PI_V_ATK, pCB_gem_init, {dat: INIT_IS_PRG}		},

	/* This is the old format */

	{ "AUX",		PI_V_T,   pCB_aux			},
	{ "BIOSBUF",		PI_V_B,   pCB_biosbuf			},
	{ "CACHE",		PI_V_L,   bio_set_cache_size		},
	{ "PERCENTAGE",		PI_V_L,   bio_set_percentage		},
	{ "CON",		PI_V_T,   pCB_con			},
	{ "DEBUG_DEVNO",	PI_R_S,   & out_device,	Range(0, 9)	},
	{ "DEBUG_LEVEL",	PI_R_S,   & debug_level, Range(0, 9)	},
	{ "FASTLOAD",		PI_R_B,   & forcefastload		},
	{ "HIDE_B",		PI_V_B,   pCB_hide_b			},
	{ "INITIALMEM",		PI_R_L,   & initialmem			},
	{ "MAXMEM",		PI_V_L,   pCB_maxmem			},
# ifdef WITH_MMU_SUPPORT
	{ "MPFLAGS",		PI_R_L,   & mem_prot_flags		},
# endif
	{ "PRN",		PI_V_T,   pCB_prn			},
	{ "SECURELEVEL",	PI_V_L,   pCL_securelevel, Range(0, 2)	},
	{ "SLICES",		PI_R_S,   & time_slice			},
	{ "UPDATE",		PI_R_L,   & sync_time			},
	{ "VFAT",		PI_V_D,   pCB_vfat			},
	{ "VFATLCASE",		PI_V_B,   pCB_vfatlcase			},
	{ "WB_ENABLE",		PI_V_D,   pCB_wb_enable			},
	{ "WRITEPROTECT",	PI_V_D,   pCB_writeprotect		},
	{ "NEWFATFS",		PI_V_D,   pCB_newfatfs			},

	{ NULL }
};

struct cnfdata
{
};

/*----------------------------------------------------------------------------*/
void
load_config(void)
{
	char cnf_path[128];
	struct cnfdata mydata;

	strcpy(cnf_path, sysdir);
	strcat(cnf_path, "mint.cnf");
	if (parse_cnf(cnf_path, parser_tab, &mydata) != 0)
	{
		/* sysdir/mint.cnf not found, try mchdir/mint.cnf */
		strcpy(cnf_path, mchdir);
		strcat(cnf_path, "mint.cnf");

		parse_cnf(cnf_path, parser_tab, &mydata);
	}
}

/*============================================================================*/
/* Now follows the callback definitions in alphabetical order
 */

#define CHAR2DRV(c) ((int)(strrchr(drv_list, toupper((int)c & 0xff)) - drv_list))

/*----------------------------------------------------------------------------*/
static void
pCB_alias(const char *drive, const char *path, struct parsinf *inf)
{
	unsigned short drv = CHAR2DRV(*drive);

	if (drv >= NUM_DRIVES)
	{
		parser_msg(inf, NULL);
		boot_printf(MSG_cnf_bad_drive, *drive);
		parser_msg(NULL, NULL);
	}
	else
	{
		fcookie root_dir;
		long r = path2cookie(get_curproc(), path, NULL, &root_dir);
		if (r)
		{
			parser_msg(inf, NULL);
			boot_printf(MSG_cnf_tos_error, r, path);
			parser_msg(NULL,NULL);
		}
		else
		{
			aliasdrv[drv] = root_dir.dev + 1;
			*((long *)0x4c2L) |= (1L << drv);
			release_cookie(&get_curproc()->p_cwd->curdir[drv]);
			dup_cookie(&get_curproc()->p_cwd->curdir[drv], &root_dir);
			release_cookie(&get_curproc()->p_cwd->root[drv]);
			get_curproc()->p_cwd->root[drv] = root_dir;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_aux(char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret) return;

	ret = do_open(&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		do_close(get_curproc(), get_curproc()->p_fd->ofiles[2]);
		do_close(get_curproc(), get_curproc()->p_fd->aux);
		get_curproc()->p_fd->aux = get_curproc()->p_fd->ofiles[2] = fp;
		fp->links++;
		if (is_terminal(fp) && fp->fc.fs == &bios_filesys &&
		    fp->dev == &bios_tdevice &&
		    (has_bconmap ? (fp->fc.aux>=6) : (fp->fc.aux==1)))
		{
			if (has_bconmap)
				get_curproc()->p_fd->bconmap = fp->fc.aux;
			((struct tty *)fp->devinfo)->aux_cnt++;
			fp->pos = 1;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_biosbuf(bool onNoff)
{
	if (!onNoff)
	{
		if (bconbsiz)
			bflush();

		bconbdev = -1;
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_cd(char *path)
{
	int drv;

	sys_d_setpath(path);

	drv = CHAR2DRV(*path);
	if (path[1] == ':')
		sys_d_setdrv(drv);
}

/*----------------------------------------------------------------------------*/
static void
pCB_con(char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret) return;

	ret = do_open(&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		int i;
		for (i = -1; i < 2; i++)
		{
			do_close(get_curproc(), get_curproc()->p_fd->ofiles[i]);
			get_curproc()->p_fd->ofiles[i] = fp;
			fp->links++;
		}

		/* correct for overdoing it */
		fp->links--;
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_echo(char *line)
{
	boot_print(line);
	boot_print("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_exec(const char *path, const char *line, struct parsinf *inf)
{
	union { const char *cc; char *c;} pathptr = {path};	// pathptr.cc = path;
	char cmdline[128];
	int i;

	i = strlen(line);
	if (i > 126) i = 126;
	cmdline[0] = i;
	strncpy(cmdline+1, line, i);
	cmdline[i+1] = 0;

	i = (int) sys_pexec(0,  pathptr.c, cmdline, _base->p_env);
	if (i < 0)
	{
		parser_msg(inf, NULL);
		boot_print(i == -33 ? MSG_cnf_file_not_found
				      : MSG_cnf_error_executing);
		boot_printf(" '%s'", path);
		parser_msg(NULL,NULL);
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_gem_init(const char *path, const char *line, long val)
{
	init_is_gem = val;

	boot_printf("%s '%s' '%s'\r\n", init_is_gem ? "GEM" : "INIT", path, line);
	if (stricmp(path,"ROM") == 0)
	{
		init_prg = 0;
	}
	else if ((init_prg = kmalloc(strlen(path) +1)))
	{
		strcpy(init_prg, path);
		strncpy(init_tail +1, line, 125);
		init_tail[126] = '\0';
		init_tail[0] = strlen(init_tail +1);
	}
	else
		boot_printf(" error: no mem for init_prg\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_hide_b(bool onNoff)
{
	if (onNoff)
	{
		*((long *)0x4c2L) &= ~2;	/* change BIOS map */
		dosdrvs &= ~2;			/* already initalized here */
	}
}

/*----------------------------------------------------------------------------*/
static void
pCL_securelevel(long level)
{
	secure_mode = level;
	fatfs_config(0, FATFS_SECURE, secure_mode);
}

/*----------------------------------------------------------------------------*/
static void
pCB_include(char *path, struct parsinf *inf)
{
	parse_include(path, inf, parser_tab);
}

/*----------------------------------------------------------------------------*/
static void
pCB_maxmem(long size)
{
	if (size > 0)
		sys_psetlimit(2, size * 1024l);
}

/*----------------------------------------------------------------------------*/
static void
pCB_newfatfs(unsigned long list, struct parsinf *inf)
{
# ifdef OLDTOSFS
	bool flag = false;
	int drv = 0;

	while (list)
	{
		if (list & 1ul)
		{
			if (!(inf->opt & SET('Q')))
			{
				if (!flag)
				{
					boot_printf(MSG_cnf_newfatfs);
					flag = true;
				}
				boot_printf("%c", drv_list[drv]);
			}
			fatfs_config(drv, FATFS_DRV, ENABLE);
		}
		list >>= 1;
		drv += 1;
	}
	if (flag)
		boot_printf("\r\n");
# else
	boot_printf(MSG_cnf_newfatfs);
# endif
}

/*----------------------------------------------------------------------------*/
static void
pCB_prn(char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret) return;

	ret = do_open(&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		do_close(get_curproc(), get_curproc()->p_fd->ofiles[3]);
		do_close(get_curproc(), get_curproc()->p_fd->prn);

		get_curproc()->p_fd->prn =
		get_curproc()->p_fd->ofiles[3] = fp;

		fp->links++;
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_setenv(const char *var, const char *arg, struct parsinf *inf)
{
	DEBUG(("pCB_setenv: %s=%s", var, arg));
	_mint_setenv(_base, var, arg);
}

/*----------------------------------------------------------------------------*/
static void
pCB_vfat(unsigned long list, struct parsinf *inf)
{
	bool flag = false;
	int drv = 0;

	while (list)
	{
		if (list & 1ul)
		{
			if (!(inf->opt & SET('Q')))
			{
				if (!flag)
				{
					boot_printf(MSG_cnf_vfat);
					flag = true;
				}
				boot_printf("%c", drv_list[drv]);
			}
			/* user will VFAT
			 * so we activate also the NEWFATFS here
			 */
# ifdef OLDTOSFS
			fatfs_config(drv, FATFS_DRV, ENABLE);
# endif
			fatfs_config(drv, FATFS_VFAT, ENABLE);
		}
		list >>= 1;
		drv += 1;
	}

	if (flag)
		boot_printf("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_vfatlcase(bool onNoff)
{
	fatfs_config(0, FATFS_VCASE, onNoff);
}

/*----------------------------------------------------------------------------*/
static void
pCB_wb_enable(unsigned long list, struct parsinf *inf)
{
	bool flag = false;
	int drv = 0;

	while (list)
	{
		if (list & 1ul)
		{
			if (!(inf->opt & SET('Q')))
			{
				if (!flag)
				{
					boot_printf(MSG_cnf_wbcache);
					flag = true;
				}
				boot_printf("%c", drv_list[drv]);
			}
			bio.config(drv, BIO_WB, ENABLE);
		}
		list >>= 1;
		drv += 1;
	}

	if (flag)
		boot_printf("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_writeprotect(unsigned long list, struct parsinf *inf)
{
	bool flag = false;
	int drv = 0;

	while (list)
	{
		if (list & 1ul)
		{
			if (!(inf->opt & SET('Q')))
			{
				if (!flag)
				{
					boot_printf("\033pWRITEPROTECT active:\033q ");
					flag = true;
				}
				boot_printf("%c", drv_list[drv]);
			}
			bio.config(drv, BIO_WP, ENABLE);
		}
		list >>= 1;
		drv += 1;
	}

	if (flag)
		boot_printf("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_set(char *line, struct parsinf *inf)
{
	unsigned long opt = 0UL;
	char onNoff;

	while ((onNoff = *line))
	{
		if (onNoff != '-' && onNoff != '+')
		{
			break;
		}
		else
		{
			char c = toupper((int)*(++line) & 0xff);

			if (!isalpha(c))
				break;

			if (c != 'C' && c != 'Q' && c != 'V')
			{
				parser_msg(inf, NULL);
				boot_printf(MSG_cnf_set_option, c);
				parser_msg(NULL, MSG_cnf_set_ignored);
			}
			else
			{
				if (onNoff == '-') opt |=  SET(c);
				else opt &= ~SET(c);
			}

			while (isspace(*(++line)))
				;
		}
	}

	if (*line && *line != '#')
		parser_msg(inf, MSG_cnf_invalid_arg);
	else
		inf->opt = opt;
}
