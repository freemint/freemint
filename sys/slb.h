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

# ifndef _slb_h
# define _slb_h

# include "mint/mint.h"
# include "mint/slb.h"


long _cdecl s_lbopen (char *name, char *path, long min_ver, SHARED_LIB **sl, SLB_EXEC *fn);
long _cdecl s_lbclose (SHARED_LIB *sl);
int slb_close_on_exit (int terminate);
void remove_slb (void);


# endif /* _slb_h */
