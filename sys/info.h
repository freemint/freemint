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

/* bios.c */
extern const char *MSG_bios_kill;

/* debug.c */
extern const char *MSG_debug_syncing;
extern const char *MSG_debug_syncdone;
extern const char *MSG_fatal_reboot;
extern const char *MSG_system_halted;

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
# ifdef VERBOSE_BOOT
extern const char *MSG_init_mp;
extern const char *MSG_init_mp_enabled;
extern const char *MSG_init_mp_disabled;
# endif
extern const char *MSG_init_delay_loop;
extern const char *MSG_init_specify_prg;

/* keyboard.c */
extern const char *MSG_keytable_loaded;
extern const char *MSG_keytable_faulty;

/* unicode.c */
extern const char *MSG_unitable_loaded;

/* xhdi.c */
extern const char *MSG_xhdi_present;
extern const char *MSG_kerinfo_accepted;
extern const char *MSG_kerinfo_rejected;
extern const char *MSG_xhdi_absent;

# endif /* _info_h */
