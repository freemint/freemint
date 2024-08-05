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

char const greet1[] =
	"\ev\r\n"
	"033p"
	" MiNT is Now TOS (" __DATE__ ")         \033q\r\n"
	" MiNT v" MINT_VERS_STRING " "
;

char const greet2[] =
	"\r\n"
	" \xbd 1990,1991,1992 Eric R. Smith\r\n"
	" MultiTOS kernel\r\n"
	" \xbd 1992,1993,1994 Atari Corporation\r\n"
	" All Rights Reserved.\r\n\033p"
	" Use this program at your own risk!    \033q\r\n"
	"\r\n"
;

# else /* THIRD_PARTY */

char const greet1[] =
	"\ev\r\n"
	"\033p"
	" This is " MINT_NAME " v" MINT_VERS_STRING "        "
;

char const greet2[] =
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
char const memprot_warning[] =
	"\033p"
	"             *** WARNING ***              \033q\r\n"
	"You have turned off the memory protection.\r\n"
	"This is not recommended and may not be\r\n"
	"supported in the future.\r\n"
	"\r\n"
;
# endif

# ifdef DEV_RANDOM
char const random_greet[] =
	"\033p High quality random number generator \033q\r\n"
	" courtesy of Theodore Ts'o.\r\n"
	" Copyright \xbd 1994-1998 Theodore Ts'o\r\n"
	" All Rights Reserved.\r\n"
	" See the file COPYRAND for details.\r\n"
	"\r\n"
;
# endif

# ifdef CRYPTO_CODE
char const crypto_greet[] =
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

# endif /* LANG_ENGLISH */
