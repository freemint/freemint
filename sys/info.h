/*
 * $Id$
 * 
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

extern const char  build_user   [];
extern const char  build_host   [];
extern const char  build_domain [];
extern const char  build_ctime  [];
extern const ulong build_serial;

extern const char COMPILER_NAME [];
extern const char COMPILER_OPTS [];
extern const char COMPILER_DEFS [];
extern const char COMPILER_VERS [];


#ifdef DEV_RANDOM
extern const char *random_greet;
#endif

#ifdef CRYPTO_CODE
extern const char *crypto_greet;
#endif

extern const char *memprot_notice;
extern const char *memprot_warning;

extern const char *greet1;
extern const char *greet2;

extern const char *months_abbr_3[];

/* bios.c */
extern const char *MSG_bios_kill;

/* biosfs.c */
extern const char *ERR_biosfs_aux_fptr;

/* block_IO.c */
extern const char *ERR_bio_cant_init_cache;

/* cnf.c */
extern const char *MSG_cnf_bad_drive;
extern const char *MSG_cnf_tos_error;
extern const char *MSG_cnf_file_not_found;
extern const char *MSG_cnf_error_executing;
extern const char *MSG_cnf_cannot_include;
extern const char *MSG_cnf_newfatfs;
extern const char *MSG_cnf_vfat;
extern const char *MSG_cnf_wbcache;
extern const char *MSG_cnf_invalid_arg;
extern const char *MSG_cnf_set_option;
extern const char *MSG_cnf_set_ignored;
extern const char *MSG_cnf_parser_skipped;
extern const char *MSG_cnf_empty_file;
extern const char *MSG_cnf_cant_allocate;
extern const char *MSG_cnf_reading_mintcnf;
extern const char *MSG_cnf_not_successful;
extern const char *MSG_cnf_bytes_done;
extern const char *MSG_cnf_unknown_variable;
extern const char *MSG_cnf_syntax_error;
extern const char *MSG_cnf_keyword_not_supported;
extern const char *MSG_cnf_needs_equation;
extern const char *MSG_cnf_argument_for;
extern const char *MSG_cnf_missed;
extern const char *MSG_cnf_must_be_a_num;
extern const char *MSG_cnf_out_of_range;
extern const char *MSG_cnf_must_be_a_bool;
extern const char *MSG_cnf_missing_quotation;
extern const char *MSG_cnf_junk;
extern const char *MSG_cnf_unknown_tag;

/* debug.c */
extern const char *MSG_debug_syncing;
extern const char *MSG_debug_syncdone;
extern const char *MSG_fatal_reboot;
extern const char *MSG_system_halted;

/* dma.c */
extern const char *ERR_dma_start_on_inv_handle;
extern const char *ERR_dma_end_on_inv_handle;
extern const char *ERR_dma_end_on_unlocked_handle;
extern const char *ERR_dma_block_on_inv_handle;
extern const char *ERR_dma_deblock_on_inv_handle;
extern const char *ERR_dma_addroottimeout;

/* dosfile.c */
# if O_GLOBAL
extern const char *MSG_oglobal_denied;
extern const char *MSG_global_handle;
# endif

/* filesys.c */
extern const char *ERR_fsys_inv_fdcwd;
extern const char *MSG_fsys_files_were_open;
extern const char *MSG_fsys_syncing;
extern const char *MSG_fsys_syncing_done;

/* gmon.c */
# ifdef PROFILING
extern const char *MSG_gmon_out_of_mem;
extern const char *MSG_gmon_fptr_fail;
extern const char *MSG_gmon_out_fail;
extern const char *MSG_gmon_out_written;
extern const char *MSG_gmon_profiler_on;
extern const char *MSG_gmon_profiler_off;
extern const char *MSG_gmon_enable_error;
# endif

/* init.c */
extern const char *MSG_init_getname;
extern const char *MSG_init_askmenu;
extern const char *MSG_init_menu_yes;
extern const char *MSG_init_menu_no;
extern const char *MSG_init_menu_yesrn;
extern const char *MSG_init_menu_norn;
extern const char *MSG_init_bootmenu;
extern const char *MSG_init_menuwarn;
extern const char *MSG_init_hitanykey;
extern const char *MSG_must_be_auto;
# ifdef VERBOSE_BOOT
extern const char *MSG_init_mp;
extern const char *MSG_init_mp_enabled;
extern const char *MSG_init_mp_disabled;
# endif
extern const char *MSG_init_delay_loop;
extern const char *MSG_couldnt_run_init;

/* keyboard.c */
extern const char *MSG_keytable_loaded;
extern const char *MSG_keytable_faulty;

/* mis.c */
extern const char *MSG_shell_name;
extern const char *MSG_shell_missing_arg;

/* slb.c */
extern const char *MSG_slb_couldnt_open;
extern const char *MSG_slb_freeing_used;

/* unicode.c */
extern const char *MSG_unitable_loaded;
extern const char *MSG_unicode_cannot_below_128;

/* unifs.c */
extern const char *MSG_unifs_wrong_getxattr;
extern const char *MSG_unifs_couldnt_match;
extern const char *MSG_unifs_fs_doesnt_match_dirs;

/* xhdi.c */
extern const char *MSG_xhdi_present;
extern const char *MSG_kerinfo_accepted;
extern const char *MSG_kerinfo_rejected;
extern const char *MSG_xhdi_absent;

# endif /* _info_h */
