/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999 Ralph Lowinski <ralph@aquaplan.de>
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
 * Author: Ralph Lowinski <ralph@aquaplan.de>
 * Started: 1998-12-22
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * changes since last version:
 *
 * 1999-06-09:
 *
 * - fix: bug in include mechanism; freed pointer is accessed
 *
 * 1999-04-13:
 *
 * Frank:
 * - new keyword: WRITEPROTECT for software write protection
 *   on filesystem level
 *
 * 1999-03-11:
 *
 * Frank:
 * - NEWFATS keyword is always known; if not supported the user
 *   get a message that the NEWFATFS is the default filesystem
 * - VFAT implies NEWFATFS for the specified drive
 *
 * 1998-12-28:
 *
 * Frank:
 * - new: optimzed messages/correct some words
 * - fix: open/read/close mechanism
 *
 * 1998-12-22:
 *
 * - initial revision
 *
 * known bugs:
 *
 * -
 *
 * todo:
 *
 *
 */

# include "cnf.h"
# include "global.h"

# include "libkern/libkern.h"

# include "mint/filedesc.h"
# include "arch/mprot.h"

# include "bios.h"
# include "biosfs.h"
# include "block_IO.h"
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
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "update.h"
# include "xbios.h"


/* program to run at startup */
#define INIT_IS_GEM   1
#define INIT_IS_PRG   0
int init_is_gem = INIT_IS_GEM;	/* set to 1 if init_prg is GEM */

char *init_prg = NULL;
char *init_env = NULL;
char init_tail[256];
/*
 * note: init_tail is *NOT* used as a temporary stack for resets in
 * intr.spp (it was before 1.16)
 */

typedef enum {
	true  = (1 == 1),
	false = (0 == 1)
}bool;

typedef union {
	int	_err;
	short	s;
	long	l;
	ulong	u;
	bool	b;
	char	*c;
}GENARG;

typedef struct {
	short a, b;
}RANGE;
#define Range( a, b )   (((long)(a)<<16)|(int)(b))

typedef struct {
	ulong opt;     /* options */
	/* */
	const char *file;   /* name of current cnf file   */
	int        line;    /* current line in file       */
	/* */
	char   *dst;    /* write to, NULL indicates an error */
	char   *src;    /* read from                         */
	/* */
	char *env_ptr;   /* temporary pointer into that environment for setenv */
	long env_len;    /* length of the environment */
}PARSINF;

#define SET(opt)     (1ul << ((opt) -'A'))

static const char *drv_list = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";
#define CHAR2DRV(c) ((int)(strrchr(drv_list, toupper((int)c & 0xff)) - drv_list))

static void parser	(FILEPTR *f, PARSINF *inf, long f_size);
static void parser_msg	(PARSINF *inf, const char *msg);

/*----------------------------------------------------------------------------*/
void
load_config (void)
{
	PARSINF inf  = { 0ul, NULL, 1, NULL, NULL, NULL, 0ul };
	XATTR xattr;
	FILEPTR *fp;
	long ret;
	char cnf_path[32];	/* should be plenty */

	ret = FP_ALLOC (rootproc, &fp);
	if (ret) return;

	strcpy(cnf_path, sysdir);
	strcat(cnf_path, "mint.cnf");

	ret = do_open (&fp, inf.file = cnf_path, O_RDONLY, 0, &xattr);

	if (!ret)
	{
		parser (fp, &inf, xattr.size);
		do_close (rootproc, fp);
	}
	else
	{
		fp->links = 0;
		FP_FREE(fp);
	}
}


/*============================================================================*/
/* The parser uses callbacks to start actions. Using the same interfaces, both
 * comands  and  variable  assignments  differ  only  in the type entry in the
 * parser's   item  table  (see  below).   The  notation  of  the  interface's
 * declaration  names  starts  with  'PCB_',  followed by one ore more letters
 * for the argument type(s):
 * 'L' is a long integer, either decimal or hex, 'B' is a boolean containing 0
 * or 1,  ' D'  is  a comma  separated  drive list as bit mask, 'T' is a token
 * without  any  spaces  (e.g.  a path),  'A'  the  rest of the line including
 * possible spaces. 'A' can only be the last parameter.
 * Note  that  'T' and 'A' are the same parameter types but has been parsed in
 * different ways.
 * Types with a following 'x' get additional parser infornation.
 *
 * These callback types are available for the parser:
 */
typedef void	(PCB_L)   (long                                    );
typedef void	(PCB_Lx)  (long,                       PARSINF *   );
typedef void	(PCB_B)   (bool                                    );
typedef void	(PCB_Bx)  (bool,                       PARSINF *   );
typedef void	(PCB_D)   (ulong                                   );
typedef void	(PCB_Dx)  (ulong,                      PARSINF *   );
typedef void	(PCB_T)   (const char *                            );
typedef PCB_T	 PCB_A                                              ;
typedef void	(PCB_Tx)  (const char *,               PARSINF *   );
typedef PCB_Tx	 PCB_Ax                                             ;
typedef void	(PCB_TT)  (const char *, const char *              );
typedef PCB_TT	 PCB_TA                                             ;
typedef void	(PCB_TTx) (const char *, const char *, PARSINF *   );
typedef PCB_TTx	 PCB_TAx                                            ;
typedef void	(PCB_0TT) (int,          const char *, const char *);
typedef void	(PCB_ATK) (const char *, const char *, long        );

/* and */
# define _NOT_SUPPORTED_  NULL


/*** First the comand callback deklarations: */

/*
 * set [+-n] ........... set option on(-) or off(+):
 *                       c - esc interpr. out off quota
 *                       q - silent output
 *                       v - print comand lines
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


/*** Now the variable callback deklarations: */

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

/*============================================================================*/
/* The  parser's  item  table contains for every keyword (comand/variable) one
 * entry  with  a key string, a type tag and a pointer to a callback. The type
 * tags  starts  all  with 'PI_' but differs then in 'C_' for comands and 'V_'
 * and  ' R_'  for variable equations. While 'R_' means that the ptr reffers a
 * variable  that  will be written directly, the other both means that the ptr
 * refers a callback.
 * The  following part (e.g. 'L', 'TA') are the same as of the interface types
 * (see above) and has to correspond with in each table entry (expect of 'x').
 *
 * These tag types are defined:
 */
typedef enum {
#define PI_C__     0x0000 /*--- comand callbacks */
        PI_C_L   = 0x0002,   /* comand gets long                      */
        PI_C_B   = 0x0003,   /* comand gets bool                      */
        PI_C_T   = 0x0004,   /* comand gets path (or token) e.g. cd   */
        PI_C_TT  = 0x0044,   /* comand gets two pathes                */
        PI_C_0TT = 0x0144,   /* comand gets zero and two pathes       */
        PI_C_A   = 0x0005,   /* comand gets line as string, e.g. echo */
        PI_C_TA  = 0x0054,   /* comand gets path and line, e.g. exec  */
        PI_C_D   = 0x0006,   /* comand gets drive list                */
#define PI_V__     0x1000 /*--- variable callbacks */
        PI_V_L   = 0x1002,   /* variable gets long                   */
        PI_V_B   = 0x1003,   /* variable gets bool                   */
        PI_V_T   = 0x1004,   /* variable gets path                   */
        PI_V_A   = 0x1005,   /* variable gets line                   */
        PI_V_ATK = 0x1254,   /* variable gets path, line plus const. */
        PI_V_D   = 0x1006,   /* variable gets drive list             */
#define PI_R__   = 0x3000 /*--- references */
        PI_R_S   = 0x3001,   /* reference gets short */
        PI_R_L   = 0x3002,   /* reference gets long  */
        PI_R_B   = 0x3003,   /* reference gets bool  */
} PITYPE;

/* The item table, note the 'NULL' entry at the end. */

struct parser_item { char *key; PITYPE type; void *cb; long dat;
} parser_tab[] = {
	{ "SET",         PI_C_A,   pCB_set            },
	{ "ALIAS",       PI_C_TT,  pCB_alias          },
	{ "CD",          PI_C_T,   pCB_cd             },
	{ "ECHO",        PI_C_A,   pCB_echo           },
	{ "EXEC",        PI_C_TA,  pCB_exec           },
	{ "INCLUDE",     PI_C_T,   pCB_include        },
	{ "REN",         PI_C_0TT, sys_f_rename       },
	{ "SETENV",      PI_C_TT,  pCB_setenv         },
	{ "SLN",         PI_C_TT,  sys_f_symlink      },
	{ "AUX",         PI_V_T,   pCB_aux            },
	{ "BIOSBUF",     PI_V_B,   pCB_biosbuf        },
	{ "CACHE",       PI_V_L,   bio_set_cache_size },
	{ "PERCENTAGE",  PI_V_L,   bio_set_percentage },
	{ "CON",         PI_V_T,   pCB_con            },
	{ "DEBUG_DEVNO", PI_R_S,   & out_device       , Range (0, 9) },
	{ "DEBUG_LEVEL", PI_R_S,   & debug_level      , Range (0, 9) },
	{ "FASTLOAD",    PI_R_B,   & forcefastload    },
	{ "GEM",         PI_V_ATK, pCB_gem_init       , INIT_IS_GEM  },
	{ "HIDE_B",      PI_V_B,   pCB_hide_b         },
	{ "INIT",        PI_V_ATK, pCB_gem_init       , INIT_IS_PRG  },
	{ "INITIALMEM",  PI_R_L,   & initialmem       },
	{ "MAXMEM",      PI_V_L,   pCB_maxmem         },
	{ "MPFLAGS",     PI_R_L,   & mem_prot_flags   },
	{ "PRN",         PI_V_T,   pCB_prn            },
	{ "SECURELEVEL", PI_V_L,   pCL_securelevel    , Range (0, 2) },
	{ "SLICES",      PI_R_S,   & time_slice       },
	{ "UPDATE",      PI_R_L,   & sync_time        },
	{ "VFAT",        PI_V_D,   pCB_vfat           },
	{ "VFATLCASE",   PI_V_B,   pCB_vfatlcase      },
	{ "WB_ENABLE",   PI_V_D,   pCB_wb_enable      },
	{ "WRITEPROTECT",PI_V_D,   pCB_writeprotect   },
	{ "NEWFATFS",    PI_V_D,   pCB_newfatfs       },

	{ NULL }
};


/*============================================================================*/
/* Now follows the callback definitions in alphabetical order
 */

/*----------------------------------------------------------------------------*/
static void
pCB_alias (const char *drive, const char *path, PARSINF *inf)
{
	ushort drv = CHAR2DRV (*drive);

	if (drv >= NUM_DRIVES)
	{
		parser_msg  (inf, NULL);
		boot_printf (MSG_cnf_bad_drive, *drive);
		parser_msg  (NULL,NULL);
	}
	else
	{
		fcookie root_dir;
		long r = path2cookie (path, NULL, &root_dir);
		if (r)
		{
			parser_msg  (inf, NULL);
			boot_printf (MSG_cnf_tos_error, r, path);
			parser_msg  (NULL,NULL);
		}
		else
		{
			aliasdrv[drv] = root_dir.dev + 1;
			*((long *)0x4c2L) |= (1L << drv);
			release_cookie (&curproc->p_cwd->curdir[drv]);
			dup_cookie (&curproc->p_cwd->curdir[drv], &root_dir);
			release_cookie (&curproc->p_cwd->root[drv]);
			curproc->p_cwd->root[drv] = root_dir;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_aux (const char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC (rootproc, &fp);
	if (ret) return;

	ret = do_open (&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		do_close (curproc, curproc->p_fd->ofiles[2]);
		do_close (curproc, curproc->p_fd->aux);
		curproc->p_fd->aux = curproc->p_fd->ofiles[2] = fp;
		fp->links++;
		if (is_terminal (fp) && fp->fc.fs == &bios_filesys &&
		    fp->dev == &bios_tdevice &&
		    (has_bconmap ? (fp->fc.aux>=6) : (fp->fc.aux==1)))
		{
			if (has_bconmap)
				curproc->p_fd->bconmap = fp->fc.aux;
			((struct tty *)fp->devinfo)->aux_cnt++;
			fp->pos = 1;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_biosbuf (bool onNoff)
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
pCB_cd (const char *path)
{
	int drv;
	(void)sys_d_setpath(path);
	drv = CHAR2DRV(*path);
	if (path[1] == ':') (void)sys_d_setdrv(drv);
}

/*----------------------------------------------------------------------------*/
static void
pCB_con (const char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC (rootproc, &fp);
	if (ret) return;

	ret = do_open (&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		int i;
		for (i = -1; i < 2; i++)
		{
			do_close (curproc, curproc->p_fd->ofiles[i]);
			curproc->p_fd->ofiles[i] = fp;
			fp->links++;
		}

		/* correct for overdoing it */
		fp->links--;
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_echo (const char *line)
{
	boot_print (line); boot_print ("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_exec (const char *path, const char *line, PARSINF *inf)
{
	char cmdline[128];
	int i;

	i = strlen(line);
	if (i > 126) i = 126;
	cmdline[0] = i;
	strncpy(cmdline+1, line, i);
	cmdline[i+1] = 0;
	i = (int) sys_pexec (0, (char*) path, cmdline, init_env);
	if (i < 0)
	{
		parser_msg  (inf, NULL);
		boot_print  (i == -33 ? MSG_cnf_file_not_found
		                      : MSG_cnf_error_executing);
		boot_printf (" '%s'", path);
		parser_msg  (NULL,NULL);
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_gem_init (const char *path, const char *line, long val)
{
	if ((init_prg = kmalloc (strlen(path) +1)))
	{
		strcpy (init_prg, path);
		init_is_gem = val;
		strncpy (init_tail +1, line, 125);
		init_tail[126] = '\0';
		init_tail[0]   = strlen (init_tail +1);
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_hide_b (bool onNoff)
{
	if (onNoff)
	{
		*((long *)0x4c2L) &= ~2;   /* change BIOS map */
		dosdrvs &= ~2;             /* already initalized here */
	}
}

/*----------------------------------------------------------------------------*/
static void
pCL_securelevel (long level)
{
	secure_mode = level;
	fatfs_config (0, FATFS_SECURE, secure_mode);
}

/*----------------------------------------------------------------------------*/
static void
pCB_include (const char *path, PARSINF *inf)
{
	XATTR xattr;
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC (rootproc, &fp);
	if (ret) return;

	ret = do_open (&fp, path, O_RDONLY, 0, &xattr);
	if (!ret)
	{
		PARSINF include =
		{
			inf->opt,
			path,
			1,
			inf->dst,
			inf->src,
			inf->env_ptr,
			inf->env_len
		};

		parser (fp, &include, xattr.size);
		inf->env_ptr = include.env_ptr;
		inf->env_len = include.env_len;
		do_close (rootproc, fp);
	}
	else
	{
		parser_msg  (inf, NULL);
		boot_printf (MSG_cnf_cannot_include, path);
		parser_msg  (NULL, NULL);
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_maxmem (long size)
{
	if (size > 0)
		sys_psetlimit (2, size * 1024l);
}

/*----------------------------------------------------------------------------*/
static void
pCB_newfatfs (ulong list, PARSINF *inf)
{
# ifdef OLDTOSFS
	bool flag = false;
	int  drv  = 0;
	while (list) {
		if (list & 1ul) {
			if (!(inf->opt & SET('Q'))) {
				if (!flag) {
					boot_printf (MSG_cnf_newfatfs);
					flag = true;
				}
				boot_printf ("%c", drv_list[drv]);
			}
			(void) fatfs_config (drv, FATFS_DRV, ENABLE);
		}
		list >>= 1;
		drv   += 1;
	}
	if (flag) boot_printf ("\r\n");
# else
	boot_printf (MSG_cnf_newfatfs);
# endif
}

/*----------------------------------------------------------------------------*/
static void
pCB_prn (const char *path)
{
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC (rootproc, &fp);
	if (ret) return;

	ret = do_open (&fp, path, O_RDWR|O_CREAT|O_TRUNC, 0, NULL);
	if (!ret)
	{
		do_close (curproc, curproc->p_fd->ofiles[3]);
		do_close (curproc, curproc->p_fd->prn);
		curproc->p_fd->prn = curproc->p_fd->ofiles[3] = fp;
		fp->links++;
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_setenv (const char *var, const char *arg, PARSINF *inf)
{
	long env_used = inf->env_ptr - init_env;
	long var_len  = strlen(var);
	long arg_len  = strlen(arg)       +1;   /* + '\0' */
	long env_plus = var_len + arg_len +1;   /* + '='  */

	if (env_used + env_plus + 1 > inf->env_len) {
		char *new_env = (char *)m_xalloc (inf->env_len += 1024, 0x13);
		if (init_env) {
			memcpy (new_env, init_env, env_used);
			m_free ((long) init_env);
		}
		init_env     = new_env;
		inf->env_ptr = new_env + env_used;
	}
	memcpy ( inf->env_ptr,            var, var_len);
	         inf->env_ptr[var_len]  = '=';
	memcpy (&inf->env_ptr[var_len+1], arg, arg_len);
	         inf->env_ptr[env_plus] = '\0';
	inf->env_ptr += env_plus;
}

/*----------------------------------------------------------------------------*/
static void
pCB_vfat (ulong list, PARSINF *inf)
{
	bool flag = false;
	int  drv  = 0;
	while (list) {
		if (list & 1ul) {
			if (!(inf->opt & SET('Q'))) {
				if (!flag) {
					boot_printf (MSG_cnf_vfat);
					flag = true;
				}
				boot_printf ("%c", drv_list[drv]);
			}
			/* user will VFAT
			 * so we activate also the NEWFATFS here
			 */
# ifdef OLDTOSFS
			(void) fatfs_config (drv, FATFS_DRV, ENABLE);
# endif
			(void) fatfs_config (drv, FATFS_VFAT, ENABLE);
		}
		list >>= 1;
		drv   += 1;
	}
	if (flag) boot_printf ("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_vfatlcase (bool onNoff)
{
	fatfs_config (0, FATFS_VCASE, onNoff);
}

/*----------------------------------------------------------------------------*/
static void
pCB_wb_enable (ulong list, PARSINF *inf)
{
	bool flag = false;
	int  drv  = 0;
	while (list) {
		if (list & 1ul) {
			if (!(inf->opt & SET('Q'))) {
				if (!flag) {
					boot_printf (MSG_cnf_wbcache);
					flag = true;
				}
				boot_printf ("%c", drv_list[drv]);
			}
			(void) bio.config (drv, BIO_WB, ENABLE);
		}
		list >>= 1;
		drv   += 1;
	}
	if (flag) boot_printf ("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_writeprotect (ulong list, PARSINF *inf)
{
	bool flag = false;
	int  drv  = 0;
	while (list) {
		if (list & 1ul) {
			if (!(inf->opt & SET('Q'))) {
				if (!flag) {
					boot_printf ("\033pWRITEPROTECT active:\033q ");
					flag = true;
				}
				boot_printf ("%c", drv_list[drv]);
			}
			(void) bio.config (drv, BIO_WP, ENABLE);
		}
		list >>= 1;
		drv   += 1;
	}
	if (flag) boot_printf ("\r\n");
}

/*----------------------------------------------------------------------------*/
static void
pCB_set (const char *line, PARSINF *inf)
{
	ulong opt = 0ul;
	char  onNoff;
	while ((onNoff = *line)) {
		if (onNoff != '-'  &&  onNoff != '+') {
			break;
		} else {
			char c = toupper((int)*(++line) & 0xff);
			if (!isalpha(c)) break;
			if (c != 'C' && c != 'Q' && c != 'V') {
				parser_msg  (inf, NULL);
				boot_printf (MSG_cnf_set_option, c);
				parser_msg  (NULL, MSG_cnf_set_ignored);
			} else {
				if (onNoff == '-') opt |=  SET(c);
				else               opt &= ~SET(c);
			}
			while (isspace(*(++line)));
		}
	}
	if (*line  &&  *line != '#') {
		parser_msg (inf, MSG_cnf_invalid_arg);
	} else {
		inf->opt = opt;
	}
}


/*----------------------------------------------------------------------------*/
static void
p_REFF (PITYPE type, void *reff, GENARG arg)
{
	if (type == PI_R_L)  *(long*)reff = arg.l;
	else                *(short*)reff = arg.s;
}


/*============================================================================*/
/* The parser itself and its functions ...
 */

#define NUL '\0'
#define SGL '\''
#define DBL '"'
#define EOL '\n'
#define ESC '\\'   /* not 0x1B !!! */

enum { ARG_MISS, ARG_NUMB, ARG_RANG, ARG_BOOL, ARG_QUOT };


/*----------------------------------------------------------------------------*/
static void
parser_msg (PARSINF *inf, const char *msg)
{
	if (inf) {
		boot_printf ("[%s:%i] ", inf->file, inf->line);
	} else if (!msg) {
		msg = MSG_cnf_parser_skipped;
	}
	if (msg) {
		boot_print (msg);
		boot_print ("\r\n");
	}
}

/*----------------------------------------------------------------------------*/
static void
parse_spaces (PARSINF *inf)
{
	char c;

	while (isspace (c = *(inf->src)) && c != '\n') inf->src++;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_token (PARSINF *inf, bool upcase)
{
	char   *dst = inf->dst, *src = inf->src;
	char   delim = NUL;
	char   c;
	GENARG ret;ret.c = inf->dst;

	do {
		if ((c = *(src++)) == NUL) {
			--src; /* correct overread zero */

		} else if (c == SGL || c == DBL) {
			if      (delim == NUL) { delim  = c;   continue; }
			else if (delim == c)   { delim  = NUL; continue; }

		} else if (c == ESC  && ( delim == DBL  ||
		                         (delim == NUL  && (inf->opt & SET('C'))) )) {
			if (!*src || *src == EOL || (*src == '\r' && *(src+1) == EOL)) {
				c = NUL;   /* leading backslash honestly ignored :-) */

			} else if (delim != SGL) switch ( (c = *(src++)) ) {
				case 't': c = '\t';   break;
				case 'n': c = '\n';   break;
				case 'r': c = '\r';   break;
				case 'e': c = '\x1B'; break; /* special: for nicer output */
			}

		} else if (c == EOL  || (isspace(c) && delim == NUL)) {
			if (c == EOL) --src; /* correct overread eol */
			c = NUL;

		} else if (upcase) {
			c = toupper((int)c & 0xff);
		}
		*(dst++) = c;

	} while (c);

	if (src == inf->src) {
		dst     = NULL; /* ARG_MISS */
	} else if (delim != NUL) {
		ret._err = ARG_QUOT;
		dst      = NULL;
	}
	inf->dst = dst;
	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_line (PARSINF *inf)
{
	GENARG ret;ret.c = inf->dst;

	while (*(inf->src) && *(inf->src) != EOL) {
		parse_token  (inf, false);
		parse_spaces (inf);
		if (*(inf->src)) *(inf->dst -1) = ' ';
	}
	if (inf->dst == ret.c) inf->dst = ret.c +1;
	 *(inf->dst -1) = NUL;

	return ret;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_drvlst (PARSINF *inf)
{
	GENARG ret;ret.u = 0;

	while (*(inf->src)) {
		long drv = strchr (drv_list, toupper((int)*(inf->src) & 0xff)) - drv_list;
		if (drv < 0) break;
		if (drv >= NUM_DRIVES ) {
			ret._err = ARG_RANG;
			break;
		}
		ret.l |= 1ul << drv;
		inf->src++;
		parse_spaces (inf);
		if (*(inf->src) != ',') break;
		inf->src++;
		parse_spaces (inf);
	}
	return ret;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_long (PARSINF *inf, RANGE *range)
{
	char   *src      = inf->src;
	int    sign      = 1;
	GENARG ret;ret.l = 0;

	if (*src == '-') {
		sign = -1;
		src++;
	}
	while (isdigit(*src)) ret.l = (ret.l * 10) + (*(src++) - '0');
	if (src > inf->src) {
		if      (toupper((int)*src & 0xff) == 'K') { ret.l *= 1024l;      src++; }
		else if (toupper((int)*src & 0xff) == 'M') { ret.l *= 1024l*1024; src++; }
	}
	*(inf->dst++) = '\0'; /* not really necessary */

	if (src == inf->src  || (*src && !isspace(*src))) {
		ret._err = ARG_NUMB;
		inf->dst = NULL;
	} else if (range  && (range->a > ret.l  ||  ret.l > range->b)) {
		ret._err = ARG_RANG;
		inf->dst = NULL;
	} else {
		ret.l *= sign;
	}
	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_short (PARSINF *inf, RANGE *range)
{
	RANGE  s_rng = {0x8000,0x7FFF};
	GENARG ret   = parse_long (inf, (range ? range : &s_rng));
	if (inf->dst) ret.s = (short)ret.l;

	return ret;
}

/*----------------------------------------------------------------------------*/
static GENARG
parse_bool (PARSINF *inf)
{
	char *tab[]      = { "\00","\11", "\0N","\1Y", "\0NO","\1YES",
	                     "\0OFF","\1ON", "\0FALSE","\1TRUE", NULL };
	char *token      = (parse_token (inf, true)).c;
	GENARG ret;ret.b = false;

	if (!inf->dst) {
		ret._err = ((GENARG)token)._err;
	} else {
		char **p = tab;
		while (*p && strcmp (&(*p)[1], token)) p++;
		if (!*p) {
			ret._err = ARG_BOOL;
			inf->dst = NULL;
		} else {
			ret.b    = ((*p)[0] != '\0');
		}
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
static void
parser (FILEPTR *f, PARSINF *inf, long f_size)
{
	long b_len = f_size + 2;
	int  state = 1;	/* 0: skip, 1: key, 2: arg(s), 3: call */
	char *buf;

	if (f_size == 0)
	{
		if (!(inf->opt & SET ('Q')))
		{
			boot_printf (MSG_cnf_empty_file, inf->file);
		}
		return;
	}

	buf = kmalloc (b_len);
	if (!buf)
	{
		boot_printf (MSG_cnf_cant_allocate, inf->file, b_len);
		return;
	}

	inf->src = buf + b_len - f_size - 1;

	if (!(inf->opt & SET ('Q')))
	{
		boot_printf (MSG_cnf_reading_mintcnf, inf->file);
	}

	if ((*f->dev->read)(f, inf->src, f_size) != f_size)
	{
		boot_print (MSG_cnf_not_successful);
		kfree (buf);
		return;
	}
	else
	{
		if (!(inf->opt & SET ('Q')))
			boot_printf (MSG_cnf_bytes_done, f_size);
	}

	inf->src [f_size] = '\0';

	while (*(inf->src))
	{
		const struct parser_item *item    = NULL;
		GENARG                   arg[2]   = { {0}, {0} };
		int                      arg_type = 0;
		int                      arg_num  = 0;

		if (state == (1)) {   /*---------- process keyword */
			char c;

			/*--- (1.1) skip leading spaces and empty or comment lines */
			while ( (c = *(inf->src)) ) {
				if      (c == '#')    while ((c = *(++inf->src)) && c != EOL);
				if      (c == EOL)    inf->line++;
				else if (!isspace(c)) break;
				inf->src++;
			}
			if (*(inf->src) == NUL) break;   /* <eof> */

			if (inf->opt & SET('V')) {
				char save, *end = inf->src;
				while (*end && *end != '\r' && *end != '\n') end++;
				save = *end;
				*end = NUL;
				boot_printf ("%s\r\n", inf->src);
				*end = save;
			}

			/*--- (1.2) now read the keyword */
			inf->dst = buf;
			while ((c = toupper((int)*(inf->src) & 0xff)) && c != '='  && !isspace(c)) {
				*(inf->dst++) = c;
				inf->src++;
			}
			*(inf->dst++) = '\0';

			/*--- (1.3) find item */
			item = parser_tab;
			while (item->key && strcmp (buf, item->key)) item++;

			if (!item->key) {   /*--- (1.3.1) keyword not found */
				parse_spaces (inf);   /* skip to next character */
				parser_msg (inf, NULL);
				boot_print (*(inf->src) == '=' ? MSG_cnf_unknown_variable : MSG_cnf_syntax_error);
				boot_printf(" '%s'", buf);
				parser_msg (NULL,NULL);
				state = 0;

			} else if (!item->cb) {   /*--- (1.3.2) found, but not supported */
				parser_msg (inf, NULL);
				boot_printf(MSG_cnf_keyword_not_supported, item->key);
				parser_msg (NULL,NULL);
				state = 0;

			} else if (item->type & PI_V__) {   /*--- (1.3.3) check equation */
				parse_spaces (inf);   /* skip to '=' */
				if (*(inf->src) != '=') {
					parser_msg (inf, NULL);
					boot_printf(MSG_cnf_needs_equation, item->key);
					parser_msg (NULL,NULL);
					state = 0;
				} else {
					inf->src++;   /* skip the '=' */
				}
			}
			if (state) {   /*----- (1.3.4) success */
				arg_type = item->type & 0x00FF;
				inf->dst = buf;
				state    = 2;
			}
		}

		while (state == (2)) {   /*---------- process arg */

			RANGE *range = (item->dat ? (RANGE*)&item->dat : NULL);
			char  *start;

			parse_spaces (inf);   /*--- (2.1) skip leading spaces */

			start = inf->src;   /*--- (2.2) read argument */
			switch (arg_type & 0x0F) {
				case 0x01: arg[arg_num] = parse_short  (inf, range); break;
				case 0x02: arg[arg_num] = parse_long   (inf, range); break;
				case 0x03: arg[arg_num] = parse_bool   (inf);        break;
				case 0x04: arg[arg_num] = parse_token  (inf, false); break;
				case 0x05: arg[arg_num] = parse_line   (inf);        break;
				case 0x06: arg[arg_num] = parse_drvlst (inf);        break;
			}
			if (!inf->dst) {   /*--- (2.3) argument failure */
				const char *msg = MSG_cnf_missed;
				if (inf->src != start) switch (arg[arg_num]._err) {
					case ARG_NUMB: msg = MSG_cnf_must_be_a_num;		break;
					case ARG_RANG: msg = MSG_cnf_out_of_range;		break;
					case ARG_BOOL: msg = MSG_cnf_must_be_a_bool;		break;
					case ARG_QUOT: msg = MSG_cnf_missing_quotation;		break;
				}
				parser_msg (inf, NULL);
				boot_printf(MSG_cnf_argument_for, arg_num +1, item->key);
				boot_print (msg);
				parser_msg (NULL,NULL);
				state = 0;

			} else {   /*----- (2.4) success */
				arg_num++;
				state = ((arg_type >>= 4) & 0x0F ? 2 : 3);
			}
		}

		if (state == (3)) {   /*---------- handle the callback */

			union { void *_v; PCB_Lx *l; PCB_Bx *b; PCB_Dx *u; PCB_Tx *c;
			        PCB_TTx *cc; PCB_0TT *_cc; PCB_ATK *ccl;
			} cb;
			cb._v = item->cb;

			/*--- (3.1) check for following characters */

			parse_spaces (inf);   /* skip following spaces */
			if (*(inf->src)  &&  *(inf->src) != '\n'  &&  *(inf->src) != '#') {
				parser_msg (inf, MSG_cnf_junk);
				state = 0;
			}
			switch (item->type & 0xF7FF) {   /*--- (3.2) do the call */
				/* We allege that every callback can make use of the parser
				 * information and put it always on stack. If it is not necessary
				 * it will simply ignored.
				 */
					#define A0L arg[0].l
					#define A0B arg[0].b
					#define A0U arg[0].u
					#define A0C arg[0].c
					#define A1C arg[1].c
				case PI_C_L: case PI_V_L: (*cb.l  )(  A0L,             inf); break;
				case PI_C_B: case PI_V_B: (*cb.b  )(  A0B,             inf); break;
				case PI_C_D: case PI_V_D: (*cb.u  )(  A0U,             inf); break;
				case PI_C_T: case PI_V_T:
				case PI_C_A: case PI_V_A: (*cb.c  )(  A0C,             inf); break;
				case PI_C_TT:
				case PI_C_TA:             (*cb.cc )(  A0C,A1C,         inf); break;
				case PI_C_0TT:            (*cb._cc)(0,A0C,A1C             ); break;
				case PI_V_ATK:            (*cb.ccl)(  A0C,A1C,item->dat   ); break;
				case PI_R_S: case PI_R_L: case PI_R_B:
				                           p_REFF (item->type,cb._v,arg[0]); break;

				default: ALERT(MSG_cnf_unknown_tag,
					            (int)item->type, item->key);
			}
			if (state) {   /*----- (3.3) success */
				state = 1;
			}
		}

		if (state == (0)) {   /*---------- skip to end of line */

			while (*inf->src) {
				if (*(inf->src++) == '\n') {
					inf->line++;
					state = 1;
					break;
				}
			}
		}
	} /* end while */

	kfree (buf);
}
