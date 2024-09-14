/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1999-02-19
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 */

# ifndef _info_h
# define _info_h

# include "mint/mint.h"
# include "mint/fcntl.h"
# include "mint/config.h"

extern const char ostype    [];
extern const char osrelease [];
extern const char version   [];

extern const ulong MiNT_version;
extern const ulong MiNT_date;
extern const ulong MiNT_time;

extern const char  build_ctime  [];

extern const char COMPILER_NAME [];
extern const char COMPILER_OPTS [];
extern const char COMPILER_DEFS [];
extern const char COMPILER_VERS [];


#ifdef DEV_RANDOM
extern char const random_greet[];
#endif

#ifdef CRYPTO_CODE
extern char const crypto_greet[];
#endif

extern char const memprot_warning[];

extern char const greet1[];
extern char const greet2[];

/* bios.c */
/* Notice that this is an ALERT(), so compose the message so that
 * it could be formatted nicely
 */
#define MSG_bios_kill \
	"KILLED. INVALID PROTECTION MODE. " \
	"Please change the protection mode " \
	"to \'Super\' in the program header."

/* biosfs.c */
#define ERR_biosfs_aux_fptr "Can't allocate default aux FILEPTR!"

/* block_IO.c */
#define ERR_bio_cant_init_cache "init_block_IO: can't initialize default cache!"

/* bootmenu.c */

/* In German this is "BootmenÅ anzeigen? (j)a (n)ein "
 * In French "Afficher le menu de dÇmarrage? (o)ui (n)on "
 */
#define MSG_init_askmenu "Hold down the SHIFT key to enter menu or wait %d s.\r\n"
#define MSG_init_menu_yes "y"
#define MSG_init_menu_no "n"
#define MSG_init_menu_yesrn "yes\r\n"
#define MSG_init_menu_norn "no\r\n"
#ifdef WITH_MMU_SUPPORT
# define MSG_init_boot_memprot "<5> Memory protection: %s"
#else
# define MSG_init_boot_memprot
#endif
#define MSG_init_bootmenu \
	"\033E\r\n\033p" \
	"    FreeMiNT boot menu    \033q\r\n\r\n" \
	"<1> Start up FreeMiNT: %s" \
	"<2> Load external XFS: %s" \
	"<3> Load external XDD: %s" \
	"<4> Execute AUTO PRGs: %s" \
	MSG_init_boot_memprot \
	"<6> Init step by step: %s" \
	"<7> Debug/trace level: %d %s\r\n" \
	"<8> Debug output dev.: %d %s\r\n" \
	"<9> Write bootlog:     %d %s\r\n" \
	"<0> Remember settings: %s\r\n" \
	"[Return] accept,\r\n" \
	"[Ctrl-C] cancel.\r\n"
#define MSG_init_menuwarn \
	"# This file is automatically created,\n" \
	"# do not edit.\n\n"
#define MSG_init_unknown_cmd "%s: unknown command '%s'\r\n"
#define MSG_init_no_value "%s: '%s' lacks value\r\n"
#define MSG_init_read "Reading `%s' ... "
#define MSG_init_write "Writing `%s' ... "
#define MSG_init_rw_done "done\r\n"
#define MSG_init_rw_error "error %ld\r\n"
#define MSG_init_value_out_of_range "%s: %s value %ld is out of range (%d-%d)\r\n"
#define MSG_init_syntax_error "%s: %s syntax error\r\n"

/* cnf.c */
#define MSG_cnf_bad_drive "bad drive '%c:' in alias"
#define MSG_cnf_tos_error "TOS error %ld while looking for '%s'"
#define MSG_cnf_file_not_found "file not found"
#define MSG_cnf_error_executing "error while attempting to execute"
#define MSG_cnf_cannot_include "cannot open include file '%s'"
# ifdef OLDTOSFS
#define MSG_cnf_newfatfs "\033pNEWFATFS active:\033q "
# else
#define MSG_cnf_newfatfs "\033pNEWFATFS\033q is \033pdefault\033q filesystem for all drives!\r\n"
# endif
#define MSG_cnf_vfat "\033pVFAT active:\033q "
#define MSG_cnf_wbcache "\033pWB CACHE active:\033q "
#define MSG_cnf_invalid_arg "invalid argument line for 'set', skipped."
#define MSG_cnf_set_option "option '-%c'"
#define MSG_cnf_set_ignored " ignored."
#define MSG_cnf_parser_skipped ", skipped."
#define MSG_cnf_empty_file "[%s] empty file, skipped."
#define MSG_cnf_cant_allocate "[%s] can't allocate %li bytes, break."
#define MSG_cnf_reading_mintcnf "\r\nReading `%s' ... "
#define MSG_cnf_not_successful "not successful, break.\r\n\r\n"
#define MSG_cnf_bytes_done "%li bytes done.\r\n\r\n"
#define MSG_cnf_unknown_variable "unknown variable"
#define MSG_cnf_syntax_error "syntax error"
#define MSG_cnf_keyword_not_supported "keyword '%s' not supported yet"
#define MSG_cnf_needs_equation "variable '%s' needs equation"
#define MSG_cnf_argument_for "argument %i for '%s' "
#define MSG_cnf_missed "missed"
#define MSG_cnf_must_be_a_num "must be a number"
#define MSG_cnf_out_of_range "out of range"
#define MSG_cnf_string_too_long "'%s': string too long (max.: %ld) (ignored)"
#define MSG_cnf_must_be_a_bool "must be of type boolean (y/n)"
#define MSG_cnf_missing_quotation "missing quotation"
#define MSG_cnf_junk "junk at end of line ignored."
#define MSG_cnf_unknown_tag "!!! unknown tag type %04x for '%s'!!!\n"
#define MSG_cnf_cant_open "parse_cnf: can't open %s"
#define MSG_cnf_unset_variable "%s: unset environment variable\r\n"
#define MSG_cnf_missing_brace "missing '}'\r\n"

/* halt.c */
#define MSG_debug_syncing "Syncing..."
#define MSG_debug_syncdone "done.\r\n"
/* The one below is in...
 * German: "FATALER FEHLER. Das System muû neu gestartet werden.\r\n"
 * French: "ERREUR FATALE. Vous devez redÇmarrer le systäme.\r\n"
 */
#define MSG_fatal_reboot "FATAL ERROR. You must reboot the system.\r\n"
/* German: "System angehalten.\r\n"
 * French: "Arràt du systäme.\r\n"
 */
#define MSG_system_halted "System halted.\r\n"

/* dma.c */
#define ERR_dma_start_on_inv_handle "dma_start on invalid handle %lu"
#define ERR_dma_end_on_inv_handle "dma_end on invalid handle %lu"
#define ERR_dma_end_on_unlocked_handle "dma_end on non-locked handle %lu"
#define ERR_dma_block_on_inv_handle "dma_block on invalid handle %lu"
#define ERR_dma_deblock_on_inv_handle "dma_deblock on invalid handle %lu"
#define ERR_dma_addroottimeout "dma_block: addroottimeout failed!"

/* filesys.c */
#define ERR_fsys_inv_fdcwd "In changedrv() called from %s, invalid fd/cwd"
#define MSG_fsys_files_were_open "Files were open on the changed drive (0x%x, %s)!"
#define MSG_fsys_syncing "Syncing filesystems ..."
#define MSG_fsys_syncing_done "Syncing done"

/* gmon.c */
#define MSG_gmon_out_of_mem "monstartup: out of memory\n"
#define MSG_gmon_fptr_fail "write_gmon: failed to alloc a FILEPTR"
#define MSG_gmon_out_fail "write_gmon: failed to open gmon.out"
#define MSG_gmon_out_written "written gmon.out profiling file"
#define MSG_gmon_profiler_on "Profiler on"
#define MSG_gmon_profiler_off "Profiler off"
#define MSG_gmon_enable_error "Profiling enable error"

/* init.c */
#define MSG_init_getname "[MiNT is named \\AUTO\\%s]\r\n"
#define MSG_init_hitanykey "Hit a key to continue.\r\n"
#define MSG_init_must_be_auto "MiNT MUST be executed from the AUTO folder!\r\n"
#define MSG_init_no_mint_folder "No <boot>/mint or <boot>/mint/%s folder found!\r\n"
#define MSG_init_delay_loop "Calibrating delay loop ... "
#define MSG_init_mp "Memory protection %s\r\n"
#define MSG_init_mp_enabled "enabled"
#define MSG_init_mp_disabled "disabled"
#define MSG_init_saving_mmu "Saving MMU setup ..."
#define MSG_init_kbd_desktop_nationality "Keyboard nationality code: %d\r\nLanguage preference code: %d\r\n"
#define MSG_init_supermode "Entering supervisor mode ..."
#define MSG_init_sysdrv_is "Booting from '%c'\r\n"
#define MSG_init_tosver_kbshft "Resident TOS version %d.%02d%s\r\nKbshft 0x%08lx.\r\n"
#define MSG_init_bconmap "BIOS Bconmap() %s.\r\n"
#define MSG_init_system "Initializing system components:\r\n"
#define MSG_init_domaininit "Initializing built-in domain ops ..."
#define MSG_init_loading_modules "Loading external modules ...\r\n\r\n"
#define MSG_init_starting_sysupdate "Starting up the update daemon ..."
#define MSG_init_pid_0 "Starting up the idle process (pid 0) ..."
#define MSG_init_launching_init "Launching %s: %s ..."
#define MSG_init_no_init_specified "No init program specified to be started.\r\n"
#define MSG_init_rom_AES "No init, launching ROM AES.\r\n"
#define MSG_init_starting_shell "Starting up `%s' ..."
#define MSG_init_starting_internal_shell "Giving up, reverting to internal shell.\r\n"
#define MSG_init_done " done!\r\n"
#define MSG_init_present "present"
#define MSG_init_not_present "not present"
#define MSG_init_error " error %ld.\r\n"

/* keyboard.c */
#define MSG_keytable_loading "Installing keyboard table `%s' ..."
#define MSG_keytable_internal "Installing BIOS keyboard table ..."
#define MSG_keytable_loaded " AKP %d, ISO %ld.\r\n"
#define MSG_keyboard_keytbl_ignored "Keytbl(%ld, %ld, %ld) ignored!"

/* memory.c */
#define MSG_mem_core "Core: %lu B "
#define MSG_mem_lost "(lost %lu B)\r\n"
#define MSG_mem_alt "Alt: %lu B\r\n"
#define MSG_mem_noalt "No alternative RAM.\r\n"

/* mis.c */
#define MSG_shell_name "MiS v.%d.%d, the FreeMiNT internal shell,\n%s"
#define MSG_shell_missing_arg ": missing argument."
#define MSG_shell_error "mint: %s: error %ld\n"
#define MSG_shell_type_help "Type `help' for help\n\n"
#define MSG_shell_syntax_error "mint: syntax error.\n"
#define MSG_shell_export_help "Usage: %s [NAME=value]\n"
#define MSG_shell_setenv_help "Usage: %s [NAME value]\n"
#define MSG_shell_rm_help "Usage: %s [-f] FILE [FILE...]\n"
#define MSG_shell_exit_q "Are you sure to exit and reboot (y/n)?"
#define MSG_shell_xcmd_help "Usage: %s [on|off]\n"
#define MSG_shell_xcmd_info "Extended commands are %s\n"
#define MSG_shell_xcmd_on "on"
#define MSG_shell_xcmd_off "off"
#define MSG_shell_cp_not_dir "%s: %s is not a directory\n"
#define MSG_shell_cp_the_same "`%s' and `%s' are the same file\n"
#define MSG_shell_cd_help "Usage: %s [NEWDIR]\n"
#define MSG_shell_ls_help "Usage: %s [-l|-a] [DIR]\n"
#define MSG_shell_help \
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
	"	xcmd [on|off] - switch the extended command set on/off\n" \
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
	"\n"

/* slb.c */
/* ALERTs */
#define MSG_slb_couldnt_open "Could not open shared library `%s'"
#define MSG_slb_freeing_used "Freeing shared library %s, which is still in use!"

/* umemory.c */
/* ALERT */
#define MSG_umem_mem_corrupted "Process corrupted its memory, killing"

/* unicode.c */
#define MSG_unitable_loading "Loading unicode table `%s' ..."

/* unifs.c */
/* ALERTs */
#define MSG_unifs_wrong_getxattr "ERROR: wrong file system getxattr called"
#define MSG_unifs_couldnt_match "unifs: couldn't match a drive with a directory"
#define MSG_unifs_fs_doesnt_match_dirs "unifs: drive's file system doesn't match directory's"

/* xhdi.c */
#define MSG_xhdi_present "This system features XHDI level %x.%x (kerinfo %s).\r\n\r\n"
#define MSG_kerinfo_accepted "accepted"
#define MSG_kerinfo_rejected "rejected"
#define MSG_kerinfo_unused "not used"
#define MSG_xhdi_absent "This system does not feature XHDI.\r\n\r\n"

# endif /* _info_h */
