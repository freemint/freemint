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
 * last change: 1999-02-19
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *  
 */

# ifndef _info_h
# define _info_h

# include "mint/mint.h"


extern const char ostype    [];
extern const char osrelease [];
extern const char version   [];

extern ulong MiNT_version;
extern ulong MiNT_date;
extern ulong MiNT_time;

extern char  build_user   [];
extern char  build_host   [];
extern char  build_domain [];
extern char  build_ctime  [];
extern ulong build_serial;

extern char COMPILER_NAME [];
extern char COMPILER_OPTS [];
extern char COMPILER_DEFS [];
extern char COMPILER_VERS [];


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


# endif /* _info_h */
