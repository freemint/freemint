/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _xbios_h
# define _xbios_h

# include "mint/mint.h"

extern int has_bconmap;
extern int curbconmap;

long _cdecl sys_b_vsetmode (int modecode);
long _cdecl sys_b_supexec (Func funcptr, long a1, long a2, long a3, long a4, long a5);
long _cdecl sys_b_midiws (int, const char *);
struct io_rec *_cdecl sys_b_uiorec (short);
long _cdecl rsconf (int, int, int, int, int, int);
long _cdecl sys_b_bconmap (short);
long _cdecl sys_b_cursconf (int, int);
long _cdecl sys_b_dosound (const char *ptr);
long _cdecl sys_b_vsetscreen (long log, long phys, int rez, int mode);
long _cdecl sys_b_random (void);

void init_xbios (void);

# endif /* _xbios_h */
