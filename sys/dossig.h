/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dossig_h
# define _dossig_h

# include "mint/mint.h"
# include "mint/signal.h"


long _cdecl p_kill (int pid, int sig);
long _cdecl p_sigaction (int sig, const struct sigaction *act, struct sigaction *oact);
long _cdecl p_signal (int sig, long handler);
long _cdecl p_sigblock (ulong mask);
long _cdecl p_sigsetmask (ulong mask);
long _cdecl p_sigpending (void);
long _cdecl p_sigpause (ulong mask);
long _cdecl p_sigintr (ushort vec, ushort sig);

void sig_user (ushort vec);
void cancelsigintrs (void);


# endif /* _dossig_h */
