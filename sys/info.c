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
 * begin:	1999-02-19
 * last change:	1999-02-19
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
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

ulong MiNT_version =
	  ((ulong) MAJ_VERSION << 24)
	| ((ulong) MIN_VERSION << 16)
	| ((ulong) PATCH_LEVEL << 8)
	| ((ulong) BETA_IDENT);

ulong MiNT_date =
	  ((ulong) BUILD_DAY << 24)
	| ((ulong) BUILD_MONTH << 16)
	| ((ulong) BUILD_YEAR);

ulong MiNT_time =
	  ((ulong) BUILD_DAY_OF_WEEK << 24)
	| ((ulong) BUILD_HOUR << 16)
	| ((ulong) BUILD_MIN << 8)
	| ((ulong) BUILD_SEC);

char  build_user    [] = BUILD_USER;
char  build_host    [] = BUILD_HOST;
char  build_domain  [] = BUILD_DOMAIN;
char  build_ctime   [] = BUILD_CTIME;
ulong build_serial     = BUILD_SERIAL;


/* definitions in automatically created cdef.h
 */

char  COMPILER_NAME [] = str (_COMPILER_NAME);
char  COMPILER_OPTS [] = str (_COMPILER_OPTS);
char  COMPILER_DEFS [] = str (_COMPILER_DEFS);

# ifdef __GNUC__
char  COMPILER_VERS [] = str (__GNUC__) "." str (__GNUC_MINOR__);
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

const char *startmenu =
	"\033E\r\n\033p"
	"     FreeMiNT boot menu     \033q\r\n\r\n"
	"<1> Start up FreeMiNT: %s"
	"<2> Load external XFS: %s"
	"<3> Load external XDD: %s"
	"<4> Continue the AUTO: %s"
	"<5> Memory protection: %s"
	"<0> Save default-file: %s\r\n"
	"[Return] accept,\r\n"
	"[Ctrl-C] cancel.\r\n"
;

/* Paths used by init.c and cnf.c to find out where the cnf file is */

const char *cnf_path_1 = "mint.cnf";
const char *cnf_path_2 = "\\multitos\\mint.cnf";
const char *cnf_path_3 = "\\mint\\mint.cnf";

const char *ini_warn =
	"# This file is automatically created,\n"
	"# do not edit.\n\n"
;

/* EOF */

