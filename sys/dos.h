/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dos_h
# define _dos_h

# include "mint/mint.h"
# include "mint/proc.h"


long _cdecl s_version	(void);
long _cdecl s_uper	(long new_ssp);
long _cdecl s_yield	(void);
long _cdecl p_getpid	(void);
long _cdecl p_getppid	(void);
long _cdecl p_getpgrp	(void);
long _cdecl p_setpgrp	(int pid, int newgrp);
long _cdecl p_getauid	(void);
long _cdecl p_setauid	(int id);
long _cdecl p_usrval	(long arg);
long _cdecl p_umask	(ushort mode);
long _cdecl p_domain	(int arg);
long _cdecl p_pause	(void);
long _cdecl t_alarm	(long x);
long _cdecl t_malarm	(long x);
long _cdecl t_setitimer	(int which, long *interval, long *value, long *ointerval, long *ovalue);
long _cdecl s_ysconf	(int which);
long _cdecl s_alert	(char *msg);
long _cdecl s_uptime	(ulong *cur_uptim, ulong loadave[3]);
long _cdecl s_hutdown	(long restart);

void shutdown (void);


# endif /* _dos_h */
