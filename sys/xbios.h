/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _xbios_h
# define _xbios_h

# include "mint/mint.h"


extern int has_bconmap;
extern int curbconmap;

long _cdecl vsetmode (int modecode);
long _cdecl supexec (Func funcptr, long a1, long a2, long a3, long a4, long a5);
long _cdecl midiws (int, const char *);
long _cdecl uiorec (int);
long _cdecl rsconf (int, int, int, int, int, int);
long _cdecl bconmap (int);
long _cdecl cursconf (int, int);
long _cdecl dosound (const char *ptr);
long _cdecl vsetscreen (long log, long phys, int rez, int mode);
long _cdecl random (void);

void init_xrandom (void);
void init_bconmap (void);


# endif /* _xbios_h */
