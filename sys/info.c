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
 * 
 * Set compilation time variables
 * 
 */  

# include "info.h"

# include "buildinfo/build.h"
# include "buildinfo/cdef.h"
# include "buildinfo/version.h"

# include "libkern/libkern.h"


/* definitions for the kernel
 */

const char ostype    [] = THIRD_PARTY;
const char osrelease [] = VERS_STRING;
const char version   [] = THIRD_PARTY " " VERS_STRING;


/* definitions in automatically created build.h
 */

const ulong MiNT_version =
	  ((ulong) MAJ_VERSION << 24)
	| ((ulong) MIN_VERSION << 16)
	| ((ulong) PATCH_LEVEL << 8)
	| ((ulong) BETA_IDENT);

const ulong MiNT_date =
	  ((ulong) BUILD_DAY << 24)
	| ((ulong) BUILD_MONTH << 16)
	| ((ulong) BUILD_YEAR);

const ulong MiNT_time =
	  ((ulong) BUILD_DAY_OF_WEEK << 24)
	| ((ulong) BUILD_HOUR << 16)
	| ((ulong) BUILD_MIN << 8)
	| ((ulong) BUILD_SEC);

const char  build_user    [] = BUILD_USER;
const char  build_host    [] = BUILD_HOST;
const char  build_domain  [] = BUILD_DOMAIN;
const char  build_ctime   [] = BUILD_CTIME;
const ulong build_serial     = BUILD_SERIAL;


/* definitions in automatically created cdef.h
 */

const char COMPILER_NAME [] = str (_COMPILER_NAME);
const char COMPILER_OPTS [] = str (_COMPILER_OPTS);
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
	"\r\n"
	"033p"
	" MiNT is Now TOS (" __DATE__ ")         \033q\r\n"
	" MiNT v" VERS_STRING " "
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
	"\r\n"
	"\033p"
	" This is " MINT_NAME " v" VERS_STRING "        "
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

/* Since this place messages can be translated
 * Don't forget to add specific define that enables the national version,
 * like for example -DLANG_FRENCH for French version and so on.
 * It is assumed, that these defines are exclusive, i.e. you cannot
 * simultaneously do -DLANG_ENGLISH and -DLANG_FRENCH
 *
 * Only English support at the moment.
 */

# ifdef LANG_ENGLISH

const char *memprot_warning =
	"\007\033p"
	"             *** WARNING ***              \033q\r\n"
	"You have turned off the memory protection.\r\n"
	"This is not recommended and may not be\r\n"
	"supported in the future.\r\n"
	"\r\n"
;

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

/* ---------- bios.c ---------- */
/* Notice that this is an ALERT(), so compose the message so that
 * it could be formatted nicely
 */
const char *MSG_bios_kill =
	"KILLED. INVALID PROTECTION MODE. "
	"Please change the protection mode "
	"to \'Super\' in the program header.";

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
const char *MSG_cnf_reading_mintcnf = "\r\nReading '%s' ... ";
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
const char *MSG_cnf_must_be_a_bool = "must be of type boolean (y/n)";
const char *MSG_cnf_missing_quotation = "missing quotation";
const char *MSG_cnf_junk = "junk at end of line ignored.";
const char *MSG_cnf_unknown_tag = "!!! unknown tag type %04x for '%s'!!!\n";

/* ---------- debug.c ---------- */

const char *MSG_debug_syncing = "Syncing...";
const char *MSG_debug_syncdone = "done.\r\n";
/* The one below is in...
 * German: "FATALER FEHLER. Das System muž neu gestartet werden.\r\n"
 * French: "ERREUR FATALE. Vous devez red‚marrer le systŠme.\r\n"
 */
const char *MSG_fatal_reboot = "FATAL ERROR. You must reboot the system.\r\n";
/* German: "System angehalten.\r\n"
 * French: "Arrˆt du systŠme.\r\n"
 */
const char *MSG_system_halted = "System halted.\r\n";

/* ---------- init.c ---------- */

# if 0
const char *MSG_init_getname = "[MiNT is named \\AUTO\\%s]\r\n";
# endif

/* In German this is "Bootmen anzeigen? (j)a (n)ein "
 * In French "Afficher le menu de d‚marrage? (o)ui (n)on "
 */
const char *MSG_init_askmenu = "Display the boot menu? (y)es (n)o ";
const char *MSG_init_menu_yes = "y";
const char *MSG_init_menu_no = "n";
const char *MSG_init_menu_yesrn = "yes\r\n";
const char *MSG_init_menu_norn = "no\r\n";

const char *MSG_init_bootmenu =
	"\033E\r\n\033p"
	"    FreeMiNT boot menu    \033q\r\n\r\n"
	"<1> Start up FreeMiNT: %s"
	"<2> Load external XFS: %s"
	"<3> Load external XDD: %s"
	"<4> Execute AUTO PRGs: %s"
	"<5> Memory protection: %s"
	"<0> Remember settings: %s\r\n"
	"[Return] accept,\r\n"
	"[Ctrl-C] cancel.\r\n"
;

const char *MSG_init_menuwarn =
	"# This file is automatically created,\n"
	"# do not edit.\n\n"
;

const char *MSG_init_hitanykey = "Hit a key to continue.\r\n";
# ifdef VERBOSE_BOOT
const char *MSG_init_mp = "Memory protection ";
const char *MSG_init_mp_enabled = "enabled\r\n";
const char *MSG_init_mp_disabled = "disabled\r\n";
# endif
const char *MSG_init_delay_loop = "Calibrating delay loop... ";
const char *MSG_init_specify_prg =
	"If MiNT is run after GEM starts, you must specify a program\r\n"
	"to run initially in mint.cnf, with an INIT= line\r\n";

/* ---------- keyboard.c ---------- */

const char *MSG_keytable_loaded = "Loaded keyboard table for AKP code %d\r\n";
const char *MSG_keytable_faulty = "Keyboard table is BAD!\r\n";

/* ---------- unicode.c ---------- */

const char *MSG_unitable_loaded = "Loaded unicode table %s\r\n";

/* ---------- xhdi.c ---------- */

const char *MSG_xhdi_present = "This system features XHDI level %x.%x (kerinfo %s)\r\n\r\n";
const char *MSG_kerinfo_accepted = "accepted";
const char *MSG_kerinfo_rejected = "rejected";
const char *MSG_xhdi_absent = "This system does not feature XHDI.\r\n\r\n";

# endif /* LANG_ENGLISH */

/* EOF */
