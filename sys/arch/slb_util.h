/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * slb.h dated 99/08/17
 * 
 * Author:
 * Thomas Binder
 * (gryf@hrzpub.tu-darmstadt.de)
 * 
 * Purpose:
 * Prototypes necessary for MagiC-style "share libraries".
 * 
 * History:
 * 99/07/04-
 * 99/08/17: - Creation, with pauses (Gryf)
 */

# ifndef _slb_util_h
# define _slb_util_h

# include "mint/mint.h"


long slb_open (void);
long slb_close (void);
long slb_close_and_pterm (void);
long _cdecl slb_exec (SHARED_LIB *sl, long fn, short nargs, ...);
long _cdecl slb_fast (SHARED_LIB *sl, long fn, short nargs, ...);
long _cdecl P_kill (short, short);
long _cdecl P_setpgrp (short, short);
long _cdecl P_sigsetmask (long);
long _cdecl P_domain (short);
char * _cdecl getslbpath(BASEPAGE *base);


# endif /* _slb_util_h */
