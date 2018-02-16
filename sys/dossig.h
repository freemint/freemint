/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dossig_h
# define _dossig_h

# include "mint/mint.h"
# include "mint/signal.h"

long _cdecl sys_p_kill (short pid, short sig);
long _cdecl sys_p_sigaction (short sig, const struct sigaction *act, struct sigaction *oact);
long _cdecl sys_p_signal (short sig, long handler);
long _cdecl sys_p_sigblock (ulong mask);
long _cdecl sys_p_sigsetmask (ulong mask);
long _cdecl sys_p_sigpending (void);
long _cdecl sys_p_sigpause (ulong mask);


# endif /* _dossig_h */
