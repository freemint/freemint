/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _stubs_h
# define _stubs_h

# ifdef __TURBOC__
# include "include\mint.h"
# else
# include "include/mint.h"
# endif


long _cdecl stub_p_term0		(void);
long _cdecl stub_c_conin		(void);
long _cdecl stub_c_conout		(void);
long _cdecl stub_c_auxin		(void);
long _cdecl stub_c_auxout		(void);
long _cdecl stub_c_prnout		(void);
long _cdecl stub_c_rawio		(void);
long _cdecl stub_c_rawcin		(void);
long _cdecl stub_c_necin		(void);
long _cdecl stub_c_conws		(void);
long _cdecl stub_c_conrs		(void);
long _cdecl stub_c_conis		(void);
long _cdecl stub_d_setdrv		(void);
long _cdecl stub_c_conos		(void);
long _cdecl stub_c_prnos		(void);
long _cdecl stub_c_auxis		(void);
long _cdecl stub_c_auxos		(void);
long _cdecl stub_m_addalt		(void);
long _cdecl stub_s_realloc		(void);
long _cdecl stub_s_lbopen		(void);
long _cdecl stub_s_lbclose		(void);
long _cdecl stub_d_getdrv		(void);
long _cdecl stub_f_setdta		(void);
long _cdecl stub_s_uper		(void);
long _cdecl stub_t_getdate		(void);
long _cdecl stub_t_setdate		(void);
long _cdecl stub_t_gettime		(void);
long _cdecl stub_t_settime		(void);
long _cdecl stub_f_getdta		(void);
long _cdecl stub_s_version		(void);
long _cdecl stub_p_termres		(void);
long _cdecl stub_d_free		(void);
long _cdecl stub_d_create		(void);
long _cdecl stub_d_delete		(void);
long _cdecl stub_d_setpath		(void);
long _cdecl stub_f_create		(void);
long _cdecl stub_f_open		(void);
long _cdecl stub_f_close		(void);
long _cdecl stub_f_read		(void);
long _cdecl stub_f_write		(void);
long _cdecl stub_f_delete		(void);
long _cdecl stub_f_seek		(void);
long _cdecl stub_f_attrib		(void);
long _cdecl stub_m_xalloc		(void);
long _cdecl stub_f_dup			(void);
long _cdecl stub_f_force		(void);
long _cdecl stub_d_getpath		(void);
long _cdecl stub_m_alloc		(void);
long _cdecl stub_m_free		(void);
long _cdecl stub_m_shrink		(void);
long _cdecl stub_p_exec		(void);
long _cdecl stub_p_term		(void);
long _cdecl stub_f_sfirst		(void);
long _cdecl stub_f_snext		(void);
long _cdecl stub_f_rename		(void);
long _cdecl stub_f_datime		(void);
long _cdecl stub_f_lock		(void);

long _cdecl stub_s_yield		(void);
long _cdecl stub_f_pipe		(void);
long _cdecl stub_f_fchown		(void);
long _cdecl stub_f_fchmod		(void);
long _cdecl stub_f_cntl		(void);
long _cdecl stub_f_instat		(void);
long _cdecl stub_f_outstat		(void);
long _cdecl stub_f_getchar		(void);
long _cdecl stub_f_putchar		(void);
long _cdecl stub_p_wait		(void);
long _cdecl stub_p_nice		(void);
long _cdecl stub_p_getpid		(void);
long _cdecl stub_p_getppid		(void);
long _cdecl stub_p_getpgrp		(void);
long _cdecl stub_p_setpgrp		(void);
long _cdecl stub_p_getuid		(void);
long _cdecl stub_p_setuid		(void);
long _cdecl stub_p_kill		(void);
long _cdecl stub_p_signal		(void);
long _cdecl stub_p_vfork		(void);
long _cdecl stub_p_getgid		(void);
long _cdecl stub_p_setgid		(void);
long _cdecl stub_p_sigblock		(void);
long _cdecl stub_p_sigsetmask		(void);
long _cdecl stub_p_usrval		(void);
long _cdecl stub_p_domain		(void);
long _cdecl stub_p_sigreturn		(void);
long _cdecl stub_p_fork		(void);
long _cdecl stub_p_wait3		(void);
long _cdecl stub_f_select		(void);
long _cdecl stub_p_rusage		(void);
long _cdecl stub_p_setlimit		(void);
long _cdecl stub_t_alarm		(void);
long _cdecl stub_p_pause		(void);
long _cdecl stub_s_ysconf		(void);
long _cdecl stub_p_sigpending		(void);
long _cdecl stub_d_pathconf		(void);
long _cdecl stub_p_msg			(void);
long _cdecl stub_f_midipipe		(void);
long _cdecl stub_p_renice		(void);
long _cdecl stub_d_opendir		(void);
long _cdecl stub_d_readdir		(void);
long _cdecl stub_d_rewind		(void);
long _cdecl stub_d_closedir		(void);
long _cdecl stub_f_xattr		(void);
long _cdecl stub_f_link		(void);
long _cdecl stub_f_symlink		(void);
long _cdecl stub_f_readlink		(void);
long _cdecl stub_d_cntl		(void);
long _cdecl stub_f_chown		(void);
long _cdecl stub_f_chmod		(void);
long _cdecl stub_p_umask		(void);
long _cdecl stub_p_semaphore		(void);
long _cdecl stub_d_lock		(void);
long _cdecl stub_p_sigpause		(void);
long _cdecl stub_p_sigaction		(void);
long _cdecl stub_p_geteuid		(void);
long _cdecl stub_p_getegid		(void);
long _cdecl stub_p_waitpid		(void);
long _cdecl stub_d_getcwd		(void);
long _cdecl stub_s_alert		(void);
long _cdecl stub_t_malarm		(void);
long _cdecl stub_p_sigintr		(void);
long _cdecl stub_s_uptime		(void);
long _cdecl stub_s_trapatch		(void);
long _cdecl stub_d_xreaddir		(void);
long _cdecl stub_p_seteuid		(void);
long _cdecl stub_p_setegid		(void);
long _cdecl stub_p_getauid		(void);
long _cdecl stub_p_setauid		(void);
long _cdecl stub_p_getgroups		(void);
long _cdecl stub_p_setgroups		(void);
long _cdecl stub_t_setitimer		(void);
long _cdecl stub_d_chroot		(void);
long _cdecl stub_p_setreuid		(void);
long _cdecl stub_p_setregid		(void);
long _cdecl stub_s_ync			(void);
long _cdecl stub_s_hutdown		(void);
long _cdecl stub_d_readlabel		(void);
long _cdecl stub_d_writelabel		(void);
long _cdecl stub_s_system		(void);
long _cdecl stub_t_gettimeofday	(void);
long _cdecl stub_t_settimeofday	(void);
long _cdecl stub_p_getpriority		(void);
long _cdecl stub_p_setpriority		(void);


# endif /* _stubs_h */
