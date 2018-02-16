/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _timeout_h
# define _timeout_h

# include "mint/mint.h"


extern TIMEOUT *tlist;
extern TIMEOUT *expire_list;

TIMEOUT * _cdecl addtimeout (struct proc *p, long delta, void _cdecl (*func)(struct proc *, long));
TIMEOUT * _cdecl addtimeout_curproc (long delta, void _cdecl (*func)(struct proc *, long));
TIMEOUT * _cdecl addroottimeout (long delta, void _cdecl (*func)(struct proc *, long), ushort flags);
void _cdecl cancelalltimeouts (void);
void _cdecl canceltimeout (TIMEOUT *which);
void _cdecl cancelroottimeout (TIMEOUT *which);

#if 0	/* see timeout.c */
void _cdecl timeout (void);
#endif

void checkalarms (void);
void _cdecl nap (unsigned n);


# endif /* _timeout_h */
