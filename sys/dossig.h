/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dossig_h
# define _dossig_h

# include "mint/mint.h"
# include "mint/signal.h"


long _cdecl p_kill (short pid, short sig);
long _cdecl p_sigaction (short sig, const struct sigaction *act, struct sigaction *oact);
long _cdecl p_signal (short sig, long handler);
long _cdecl p_sigblock (ulong mask);
long _cdecl p_sigsetmask (ulong mask);
long _cdecl p_sigpending (void);
long _cdecl p_sigpause (ulong mask);


# endif /* _dossig_h */
