/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dos_h
# define _dos_h

# include "mint/mint.h"
# include "mint/proc.h"

# define SHUT_POWER	0
# define SHUT_BOOT	1
# define SHUT_COLD	2
# define SHUT_HALT	3

long _cdecl sys_s_version	(void);
long _cdecl sys_s_uper		(long new_ssp);
long _cdecl sys_s_yield		(void);
long _cdecl sys_p_getpid	(void);
long _cdecl sys_p_getppid	(void);
long _cdecl sys_p_getpgrp	(void);
long _cdecl sys_p_setpgrp	(int pid, int newgrp);
long _cdecl sys_p_getauid	(void);
long _cdecl sys_p_setauid	(int id);
long _cdecl sys_p_usrval	(long arg);
long _cdecl sys_p_umask		(ushort mode);
long _cdecl sys_p_domain	(int arg);
long _cdecl sys_p_pause		(void);
long _cdecl sys_t_alarm		(long x);
long _cdecl sys_t_malarm	(long x);
long _cdecl sys_t_setitimer	(int which, long *interval, long *value, long *ointerval, long *ovalue);
long _cdecl sys_s_ysconf	(int which);
long _cdecl sys_s_alert		(const char *msg);
long _cdecl sys_s_uptime	(ulong *cur_uptim, ulong loadave[3]);
long _cdecl sys_s_hutdown	(long restart);

# endif /* _dos_h */
