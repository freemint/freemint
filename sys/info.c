/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
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

/*
 * Set compilation time variables
 */

# include "info.h"
# include "init.h"

# include "buildinfo/build.h"
# include "cdef.h"
# include "buildinfo/version.h"

# include "libkern/libkern.h"


/* definitions for the kernel
 */

const char ostype    [] = THIRD_PARTY;
const char osrelease [] = MINT_VERS_STRING;
const char version   [] = THIRD_PARTY " " MINT_VERS_STRING;


/* definitions in automatically created build.h
 */

const ulong MiNT_version =
	  ((ulong) MINT_MAJ_VERSION << 24)
	| ((ulong) MINT_MIN_VERSION << 16)
	| ((ulong) MINT_PATCH_LEVEL << 8)
	| ((ulong) MINT_STATUS_IDENT);

const ulong MiNT_date =
	  ((ulong) BUILD_DAY << 24)
	| ((ulong) BUILD_MONTH << 16)
	| ((ulong) BUILD_YEAR);

const ulong MiNT_time =
	  ((ulong) BUILD_DAY_OF_WEEK << 24)
	| ((ulong) BUILD_HOUR << 16)
	| ((ulong) BUILD_MIN << 8)
	| ((ulong) BUILD_SEC);

const char  build_ctime   [] = BUILD_CTIME;


/* definitions in automatically created cdef.h
 */

#define str1(x) str(x)
const char COMPILER_NAME [] = str (_COMPILER_NAME);
const char COMPILER_OPTS [] = str1((_COMPILER_OPTS));
const char COMPILER_DEFS [] = str (_COMPILER_DEFS);

# ifdef __GNUC__
const char COMPILER_VERS [] = str (__GNUC__) "." str (__GNUC_MINOR__);
# else
# error Unknown compiler
# endif


/* MiNT welcome messages
 *
 * Copyright 1992,1993,1994 Atari Corp.  All Rights Reserved.
 * ===========================================================================
 */

# ifndef THIRD_PARTY

const char *greet1 =
	"\ev\r\n"
	"033p"
	" MiNT is Now TOS (" __DATE__ ")         \033q\r\n"
	" MiNT v" MINT_VERS_STRING " "
;

const char *greet2 =
	"\r\n"
	" \xbd 1990,1991,1992 Eric R. Smith\r\n"
	" MultiTOS kernel\r\n"
	" \xbd 1992,1993,1994 Atari Corporation\r\n"
	" All Rights Reserved.\r\n\033p"
	" Use this program at your own risk!    \033q\r\n"
	"\r\n"
;

# else /* THIRD_PARTY */

const char *greet1 =
	"\ev\r\n"
	"\033p"
	" This is " MINT_NAME " v" MINT_VERS_STRING "        "
;

const char *greet2 =
	"\033q\r\n"
	"compiled " __DATE__ "\r\n"
	"\r\n"
	MINT_NAME " is a modified version of MiNT\r\n"
	"\r\n"
	"MiNT \xbd 1990,1991,1992 Eric R. Smith\r\n"
	"MultiTOS kernel\r\n"
	"     \xbd 1992,1993,1994 Atari Corporation\r\n"
	"All Rights Reserved.\r\n"
	"\033p  Use this program at your own risk!  \033q\r\n"
	"\r\n"
;

# endif /* THIRD_PARTY */

/* Since this place messages can be translated.
 *
 * But don't bother adding TRACEs or DEBUGs. Add only ALERTs, FATALs and
 * messages printed out to the console via Cconws().
 *
 * Don't forget to add specific define that enables the national version,
 * like for example -DLANG_FRENCH for French version and so on.
 * It is assumed, that these defines are exclusive, i.e. you cannot
 * simultaneously do -DLANG_ENGLISH and -DLANG_FRENCH
 *
 * Only English support at the moment.
 */

# ifdef LANG_ENGLISH

# ifndef M68000
const char *memprot_warning =
	"\033p"
	"             *** WARNING ***              \033q\r\n"
	"You have turned off the memory protection.\r\n"
	"This is not recommended and may not be\r\n"
	"supported in the future.\r\n"
	"\r\n"
;
# endif

# ifdef DEV_RANDOM
const char *random_greet =
	"\033p High quality random number generator \033q\r\n"
	" courtesy of Theodore Ts'o.\r\n"
	" Copyright \xbd 1994-1998 Theodore Ts'o\r\n"
	" All Rights Reserved.\r\n"
	" See the file COPYRAND for details.\r\n"
	"\r\n"
;
# endif

# ifdef CRYPTO_CODE
const char *crypto_greet =
	"\033p"
	"              *** WARNING! ***              "
	"\033q\r\n"
	" This software contains strong cryptography.\r\n"
	" Depending on where you live, exporting it\r\n"
	" or using might be illegal.\r\n"
	"\033p"
	"    *** Thus, do so at your own risk ***    "
	"\033q\r\n"
	"\r\n"
;
# endif

/* These must have not more than 3 characters each
 * (ls in mis.c relies on it formatting its output)
 */
# ifdef BUILTIN_SHELL
const char *months_abbr_3[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", \
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
# endif

/* ---------- bios.c ---------- */
/* Notice that this is an ALERT(), so compose the message so that
 * it could be formatted nicely
 */
# ifndef M68000
const char *MSG_bios_kill =
	"KILLED. INVALID PROTECTION MODE. "
	"Please change the protection mode "
	"to \'Super\' in the program header."
;
# endif

/* ----------- biosfs.c --------- */

const char *ERR_biosfs_aux_fptr = "Can't allocate default aux FILEPTR!";

/* ---------- block_IO.c ---------- */

const char *ERR_bio_cant_init_cache = "init_block_IO: can't initialize default cache!";

/* ---------- bootmenu.c ----------- */

/* In German this is "BootmenÅ anzeigen? (j)a (n)ein "
 * In French "Afficher le menu de dÇmarrage? (o)ui (n)on "
 */
const char *MSG_init_askmenu = "Hold down the SHIFT key to enter menu or wait %d s.\r\n";
const char *MSG_init_menu_yes = "y";
# if 0
const char *MSG_init_menu_no = "n";
# endif
const char *MSG_init_menu_yesrn = "yes\r\n";
const char *MSG_init_menu_norn = "no\r\n";

const char *MSG_init_bootmenu =
	"\033E\r\n\033p"
	"    FreeMiNT boot menu    \033q\r\n\r\n"
	"<1> Start up FreeMiNT: %s"
	"<2> Load external XFS: %s"
	"<3> Load external XDD: %s"
	"<4> Execute AUTO PRGs: %s"
# ifdef WITH_MMU_SUPPORT
	"<5> Memory protection: %s"
# endif
	"<6> Init step by step: %s"
	"<7> Debug/trace level: %d %s\r\n"
	"<8> Debug output dev.: %d %s\r\n"
	"<9> Write bootlog:     %d %s\r\n"
	"<0> Remember settings: %s\r\n"
	"[Return] accept,\r\n"
	"[Ctrl-C] cancel.\r\n"
;

const char *MSG_init_menuwarn =
	"# This file is automatically created,\n"
	"# do not edit.\n\n"
;

const char *MSG_init_unknown_cmd = "%s: unknown command '%s'\r\n";
const char *MSG_init_no_value = "%s: '%s' lacks value\r\n";
const char *MSG_init_read = "Reading `%s' ... ";
const char *MSG_init_write = "Writing `%s' ... ";
const char *MSG_init_rw_done = "done\r\n";
const char *MSG_init_rw_error = "error %ld\r\n";
const char *MSG_init_value_out_of_range = "%s: %s value %ld is out of range (%d-%d)\r\n";
const char *MSG_init_syntax_error = "%s: %s syntax error\r\n";

/* ------------ cnf.c ---------- */

const char *MSG_cnf_bad_drive = "bad drive '%c:' in alias";
const char *MSG_cnf_tos_error = "TOS error %ld while looking for '%s'";
const char *MSG_cnf_file_not_found = "file not found";
const char *MSG_cnf_error_executing = "error while attempting to execute";
const char *MSG_cnf_cannot_include = "cannot open include file '%s'";
# ifdef OLDTOSFS
const char *MSG_cnf_newfatfs = "\033pNEWFATFS active:\033q ";
# else
const char *MSG_cnf_newfatfs = "\033pNEWFATFS\033q is \033pdefault\033q filesystem for all drives!\r\n";
# endif
const char *MSG_cnf_vfat = "\033pVFAT active:\033q ";
const char *MSG_cnf_wbcache = "\033pWB CACHE active:\033q ";
const char *MSG_cnf_invalid_arg = "invalid argument line for 'set', skipped.";
const char *MSG_cnf_set_option = "option '-%c'";
const char *MSG_cnf_set_ignored = " ignored.";
const char *MSG_cnf_parser_skipped = ", skipped.";
const char *MSG_cnf_empty_file = "[%s] empty file, skipped.";
const char *MSG_cnf_cant_allocate = "[%s] can't allocate %li bytes, break.";
const char *MSG_cnf_reading_mintcnf = "\r\nReading `%s' ... ";
const char *MSG_cnf_not_successful = "not successful, break.\r\n\r\n";
const char *MSG_cnf_bytes_done = "%li bytes done.\r\n\r\n";
const char *MSG_cnf_unknown_variable = "unknown variable";
const char *MSG_cnf_syntax_error = "syntax error";
const char *MSG_cnf_keyword_not_supported = "keyword '%s' not supported yet";
const char *MSG_cnf_needs_equation = "variable '%s' needs equation";
const char *MSG_cnf_argument_for = "argument %i for '%s' ";
const char *MSG_cnf_missed = "missed";
const char *MSG_cnf_must_be_a_num = "must be a number";
const char *MSG_cnf_out_of_range = "out of range";
const char *MSG_cnf_string_too_long = "'%s': string too long (max.: %ld) (ignored)";
const char *MSG_cnf_must_be_a_bool = "must be of type boolean (y/n)";
const char *MSG_cnf_missing_quotation = "missing quotation";
const char *MSG_cnf_junk = "junk at end of line ignored.";
const char *MSG_cnf_unknown_tag = "!!! unknown tag type %04x for '%s'!!!\n";
const char *MSG_cnf_cant_open = "parse_cnf: can't open %s";

/* ---------- debug.c ---------- */

const char *MSG_debug_syncing = "Syncing...";
const char *MSG_debug_syncdone = "done.\r\n";
/* The one below is in...
 * German: "FATALER FEHLER. Das System muû neu gestartet werden.\r\n"
 * French: "ERREUR FATALE. Vous devez redÇmarrer le systäme.\r\n"
 */
const char *MSG_fatal_reboot = "FATAL ERROR. You must reboot the system.\r\n";
/* German: "System angehalten.\r\n"
 * French: "Arràt du systäme.\r\n"
 */
const char *MSG_system_halted = "System halted.\r\n";

/* ----------- dma.c ---------- */

const char *ERR_dma_start_on_inv_handle = "dma_start on invalid handle %lu";
const char *ERR_dma_end_on_inv_handle = "dma_end on invalid handle %lu";
const char *ERR_dma_end_on_unlocked_handle = "dma_end on non-locked handle %lu";
const char *ERR_dma_block_on_inv_handle = "dma_block on invalid handle %lu";
const char *ERR_dma_deblock_on_inv_handle = "dma_deblock on invalid handle %lu";
const char *ERR_dma_addroottimeout = "dma_block: addroottimeout failed!";

/* --------- dosfile.c --------- */

/* -------- filesys.c --------- */

const char *ERR_fsys_inv_fdcwd = "In changedrv() called from %s, invalid fd/cwd";
const char *MSG_fsys_files_were_open = "Files were open on the changed drive (0x%x, %s)!";
const char *MSG_fsys_syncing = "Syncing filesystems ...";
const char *MSG_fsys_syncing_done = "Syncing done";

/* ---------- gmon.c ---------- */

# ifdef PROFILING
const char *MSG_gmon_out_of_mem = "monstartup: out of memory\n";
const char *MSG_gmon_fptr_fail = "write_gmon: failed to alloc a FILEPTR";
const char *MSG_gmon_out_fail = "write_gmon: failed to open gmon.out";
const char *MSG_gmon_out_written = "written gmon.out profiling file";
const char *MSG_gmon_profiler_on = "Profiler on";
const char *MSG_gmon_profiler_off = "Profiler off";
const char *MSG_gmon_enable_error = "Profiling enable error";
# endif

/* ---------- init.c ---------- */

# if 0
const char *MSG_init_getname = "[MiNT is named \\AUTO\\%s]\r\n";
# endif

const char *MSG_init_must_be_auto = "MiNT MUST be executed from the AUTO folder!\r\n";
const char *MSG_init_no_mint_folder = "No <boot>/mint or <boot>/mint/%s folder found!\r\n";
const char *MSG_init_hitanykey = "Hit a key to continue.\r\n";
const char *MSG_init_delay_loop = "Calibrating delay loop ... ";
# ifdef VERBOSE_BOOT
# ifdef WITH_MMU_SUPPORT
const char *MSG_init_mp = "Memory protection %s\r\n";
const char *MSG_init_mp_enabled = "enabled";
const char *MSG_init_mp_disabled = "disabled";
const char *MSG_init_saving_mmu = "Saving MMU setup ...";
# endif
const char *MSG_init_kbd_desktop_nationality = "Keyboard nationality code: %d\r\nLanguage preference code: %d\r\n";
const char *MSG_init_supermode = "Entering supervisor mode ...";
const char *MSG_init_sysdrv_is = "Booting from '%c'\r\n";
const char *MSG_init_tosver_kbshft = "Resident TOS version %d.%02d%s\r\nKbshft 0x%08lx.\r\n";
const char *MSG_init_bconmap = "BIOS Bconmap() %s.\r\n";
const char *MSG_init_system = "Initializing system components:\r\n";
const char *MSG_init_domaininit = "Initializing built-in domain ops ...";
const char *MSG_init_loading_modules = "Loading external modules ...\r\n\r\n";
const char *MSG_init_starting_sysupdate = "Starting up the update daemon ...";
const char *MSG_init_pid_0 = "Starting up the idle process (pid 0) ...";
const char *MSG_init_launching_init = "Launching %s: %s ...";
const char *MSG_init_no_init_specified = "No init program specified to be started.\r\n";
const char *MSG_init_rom_AES = "No init, launching ROM AES.\r\n";
const char *MSG_init_starting_shell = "Starting up `%s' ...";
# ifdef BUILTIN_SHELL
const char *MSG_init_starting_internal_shell = "Giving up, reverting to internal shell.\r\n";
# endif
const char *MSG_init_done = " done!\r\n";
const char *MSG_init_present = "present";
const char *MSG_init_not_present = "not present";
const char *MSG_init_error = " error %ld.\r\n";
# endif

/* ---------- keyboard.c ---------- */

# ifndef NO_AKP_KEYBOARD
# ifdef VERBOSE_BOOT
const char *MSG_keytable_loading = "Installing keyboard table `%s' ...";
const char *MSG_keytable_internal = "Installing BIOS keyboard table ...";
const char *MSG_keytable_loaded = " AKP %d, ISO %ld.\r\n";
# endif
const char *MSG_keyboard_keytbl_ignored = "Keytbl(%ld, %ld, %ld) ignored!";
# endif

/* ---------- memory.c ------------- */

const char *MSG_mem_core = "Core: %lu B ";
const char *MSG_mem_lost = "(lost %lu B)\r\n";
const char *MSG_mem_alt = "Alt: %lu B\r\n";
const char *MSG_mem_noalt = "No alternative RAM.\r\n";

/* ------------ mis.c ------------- */

# ifdef BUILTIN_SHELL
const char *MSG_shell_name = "MiS v.%d.%d, the FreeMiNT internal shell,\n%s";
const char *MSG_shell_missing_arg = ": missing argument.";
const char *MSG_shell_error = "mint: %s: error %ld\n";
const char *MSG_shell_type_help = "Type `help' for help\n\n";
const char *MSG_shell_syntax_error = "mint: syntax error.\n";
const char *MSG_shell_export_help = "Usage: %s [NAME=value]\n";
const char *MSG_shell_setenv_help = "Usage: %s [NAME value]\n";
const char *MSG_shell_rm_help = "Usage: %s [-f] FILE [FILE...]\n";
const char *MSG_shell_exit_q = "Are you sure to exit and reboot (y/n)?";
const char *MSG_shell_xcmd_help = "Usage: %s [on|off]\n";
const char *MSG_shell_xcmd_info = "Extended commands are %s\n";
const char *MSG_shell_xcmd_on = "on";
const char *MSG_shell_xcmd_off = "off";
const char *MSG_shell_cp_not_dir = "%s: %s is not a directory\n";
const char *MSG_shell_cp_the_same = "`%s' and `%s' are the same file\n";
const char *MSG_shell_cd_help = "Usage: %s [NEWDIR]\n";
const char *MSG_shell_ls_help = "Usage: %s [-l|-a] [DIR]\n";
const char *MSG_shell_help = \
	"	MiS is not intended to be a regular system shell, so don't\n" \
	"	expect much. It is only a tool to fix bigger problems that\n" \
	"	prevent the system from booting normally. Basic commands:\n" \
	"\n" \
	"	cd [DIR] - change directory\n" \
	"	echo TEXT - display `text'\n" \
	"	exit - leave and reboot\n" \
	"	export [NAME=value] - set an environment variable\n" \
	"	help - display this message\n" \
	"	setenv [NAME value] - set an environment variable\n" \
	"	ver - display version information\n" \
	"	xcmd [on|off] - switch the extended command set on/off\n"
	"\n" \
	"	Extended commands (now %s):\n" \
	"\n" \
	"	chgrp DEC-GROUP FILE - change group the file belongs to\n" \
	"	chmod OCTAL-MODE FILE - change access permissions for file\n" \
	"	chown DEC-OWNER[:DEC-GROUP] FILE - change file's ownership\n" \
	"	cp SOURCE [SOURCE...] DEST - copy file\n" \
	"	ln OLD NEW - create a symlink `new' pointing to file `old'\n" \
	"	ls [-l|-a] [DIR] - display directory\n" \
	"	mv SOURCE [SOURCE...] DEST - move/rename a file\n" \
	"	rm [-f] FILE [FILE...] - delete a file\n" \
	"\n" \
	"	All other words typed are understood as names of programs\n" \
	"	to execute. In case you'd want to execute something, that\n" \
	"	has been named like one of the internal commands, use the\n" \
	"	full pathname." \
	"\n";
# endif

/* ------------ slb.c ------------- */

/* ALERTs */
const char *MSG_slb_couldnt_open = "Could not open shared library `%s'";
const char *MSG_slb_freeing_used = "Freeing shared library %s, which is still in use!";

/* ------------ umemory.c ----------- */
/* ALERT */
const char *MSG_umem_mem_corrupted = "Process corrupted its memory, killing";

/* ---------- unicode.c ---------- */

# ifdef SOFT_UNITABLE
# ifdef VERBOSE_BOOT
const char *MSG_unitable_loading = "Loading unicode table `%s' ...";
# endif
# endif

/* ---------- unifs.c --------- */

/* ALERTs */
const char *MSG_unifs_wrong_getxattr = "ERROR: wrong file system getxattr called";
const char *MSG_unifs_couldnt_match = "unifs: couldn't match a drive with a directory";
const char *MSG_unifs_fs_doesnt_match_dirs = "unifs: drive's file system doesn't match directory's";

# endif /* LANG_ENGLISH */
