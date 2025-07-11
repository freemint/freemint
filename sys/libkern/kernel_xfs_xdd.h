/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 1999-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#ifndef _libkern_kernel_xfs_xdd_h
#define _libkern_kernel_xfs_xdd_h

#include "mint/kerinfo.h"


/* Macros for kernel, bios and gemdos functions
 */

#ifndef KERNEL
#define KERNEL kernel
#endif

extern struct kerinfo *KERNEL;

#define MINT_MAJOR     (KERNEL->maj_version)
#define MINT_MINOR     (KERNEL->min_version)
#define MINT_KVERSION  (KERNEL->version)
#define DEFAULT_MODE   (KERNEL->default_perm)
#define DEFAULT_DMODE  (0755)

/*
 * dos_tab
 */

#define _p_term0         (*KERNEL->dos_tab->p_pterm0)
#define _c_conin         (*KERNEL->dos_tab->p_c_conin)
#define _c_conout        (*KERNEL->dos_tab->p_c_conout)
#define _c_auxin         (*KERNEL->dos_tab->p_c_auxin)
#define _c_auxout        (*KERNEL->dos_tab->p_c_auxout)
#define _c_prnout        (*KERNEL->dos_tab->p_c_prnout)
#define _c_rawio         (*KERNEL->dos_tab->p_c_rawio)
#define _c_rawcin        (*KERNEL->dos_tab->p_c_rawcin)
#define _c_necin         (*KERNEL->dos_tab->p_c_necin)
#define _c_conws         (*KERNEL->dos_tab->p_c_conws)
#define _c_conrs         (*KERNEL->dos_tab->p_c_conrs)
#define _c_conis         (*KERNEL->dos_tab->p_c_conis)
#define _d_setdrv        (*KERNEL->dos_tab->p_d_setdrv)

#define _c_conos         (*KERNEL->dos_tab->p_c_conos)
#define _c_prnos         (*KERNEL->dos_tab->p_c_prnos)
#define _c_auxis         (*KERNEL->dos_tab->p_c_auxis)
#define _c_auxos         (*KERNEL->dos_tab->p_c_auxos)
#define _m_addalt        (*KERNEL->dos_tab->p_m_addalt)
#define _s_realloc       (*KERNEL->dos_tab->p_s_realloc)
#define _s_lbopen        (*KERNEL->dos_tab->p_s_lbopen)  /* 1.15.3 MagiC: slb */
#define _s_lbclose       (*KERNEL->dos_tab->p_s_lbclose) /* 1.15.3 MagiC: slb */
#define _d_getdrv        (*KERNEL->dos_tab->p_d_getdrv)
#define _f_setdta        (*KERNEL->dos_tab->p_f_setdta)

#define _s_uper          (*KERNEL->dos_tab->p_s_uper)
#define _t_getdate       (*KERNEL->dos_tab->p_t_getdate)
#define _t_setdate       (*KERNEL->dos_tab->p_t_setdate)
#define _t_gettime       (*KERNEL->dos_tab->p_t_gettime)
#define _t_settime       (*KERNEL->dos_tab->p_t_settime)
#define _f_getdta        (*KERNEL->dos_tab->p_f_getdta)

#define _s_version       (*KERNEL->dos_tab->p_s_version)
#define _p_termres       (*KERNEL->dos_tab->p_p_termres)
#define _s_config        (*KERNEL->dos_tab->p_s_config)  /* MagiC: long Sconfig(short subfn, long flags); */
#define _d_free          (*KERNEL->dos_tab->p_d_free)
#define _d_create        (*KERNEL->dos_tab->p_d_create)
#define _d_delete        (*KERNEL->dos_tab->p_d_delete)
#define _d_setpath       (*KERNEL->dos_tab->p_d_setpath)
#define _f_create        (*KERNEL->dos_tab->p_f_create)
#define _f_open          (*KERNEL->dos_tab->p_f_open)
#define _f_close         (*KERNEL->dos_tab->p_f_close)
#define _f_read          (*KERNEL->dos_tab->p_f_read)

#define _f_write         (*KERNEL->dos_tab->p_f_write)
#define _f_delete        (*KERNEL->dos_tab->p_f_delete)
#define _f_seek          (*KERNEL->dos_tab->p_f_seek)
#define _f_attrib        (*KERNEL->dos_tab->p_f_attrib)
#define _m_xalloc        (*KERNEL->dos_tab->p_m_xalloc)
#define _f_dup           (*KERNEL->dos_tab->p_f_dup)
#define _f_force         (*KERNEL->dos_tab->p_f_force)
#define _d_getpath       (*KERNEL->dos_tab->p_d_getpath)
#define _m_alloc         (*KERNEL->dos_tab->p_m_alloc)
#define _m_free          (*KERNEL->dos_tab->p_m_free)
#define _m_shrink        (*KERNEL->dos_tab->p_m_shrink)
#define _p_exec          (*KERNEL->dos_tab->p_p_exec)
#define _p_term          (*KERNEL->dos_tab->p_p_term)
#define _f_sfirst        (*KERNEL->dos_tab->p_f_sfirst)
#define _f_snext         (*KERNEL->dos_tab->p_f_snext)

#define _f_rename        (*KERNEL->dos_tab->p_f_rename)
#define _f_datime        (*KERNEL->dos_tab->p_f_datime)
#define _f_lock          (*KERNEL->dos_tab->p_f_lock)

/*
 * MiNT extensions to GEMDOS
 */

#define _s_yield         (*KERNEL->dos_tab->p_s_yield)

#define _f_pipe          (*KERNEL->dos_tab->p_f_pipe)
#define _f_fchown        (*KERNEL->dos_tab->p_f_fchown)  /* 1.15.2 */
#define _f_fchmod        (*KERNEL->dos_tab->p_f_fchmod)  /* 1.15.2 */
#define _f_sync          (*KERNEL->dos_tab->p_f_sync)
#define _f_cntl          (*KERNEL->dos_tab->p_f_cntl)
#define _f_instat        (*KERNEL->dos_tab->p_f_instat)
#define _f_outstat       (*KERNEL->dos_tab->p_f_outstat)
#define _f_getchar       (*KERNEL->dos_tab->p_f_getchar)
#define _f_putchar       (*KERNEL->dos_tab->p_f_putchar)
#define _p_wait          (*KERNEL->dos_tab->p_p_wait)
#define _p_nice          (*KERNEL->dos_tab->p_p_nice)
#define _p_getpid        (*KERNEL->dos_tab->p_p_getpid)
#define _p_getppid       (*KERNEL->dos_tab->p_p_getppid)
#define _p_getpgrp       (*KERNEL->dos_tab->p_p_getpgrp)
#define _p_setpgrp       (*KERNEL->dos_tab->p_p_setpgrp)
#define _p_getuid        (*KERNEL->dos_tab->p_p_getuid)

#define _p_setuid        (*KERNEL->dos_tab->p_p_setuid)
#define _p_kill          (*KERNEL->dos_tab->p_p_kill)
#define _p_signal        (*KERNEL->dos_tab->p_p_signal)
#define _p_vfork         (*KERNEL->dos_tab->p_p_vfork)
#define _p_getgid        (*KERNEL->dos_tab->p_p_getgid)
#define _p_setgid        (*KERNEL->dos_tab->p_p_setgid)
#define _p_sigblock      (*KERNEL->dos_tab->p_p_sigblock)
#define _p_sigsetmask    (*KERNEL->dos_tab->p_p_sigsetmask)
#define _p_usrval        (*KERNEL->dos_tab->p_p_usrval)
#define _p_domain        (*KERNEL->dos_tab->p_p_domain)
#define _p_sigreturn     (*KERNEL->dos_tab->p_p_sigreturn)
#define _p_fork          (*KERNEL->dos_tab->p_p_fork)
#define _p_wait3         (*KERNEL->dos_tab->p_p_wait3)
#define _f_select        (*KERNEL->dos_tab->p_f_select)
#define _p_rusage        (*KERNEL->dos_tab->p_p_rusage)
#define _p_setlimit      (*KERNEL->dos_tab->p_p_setlimit)

#define _t_alarm         (*KERNEL->dos_tab->p_t_alarm)
#define _p_pause         (*KERNEL->dos_tab->p_p_pause)
#define _s_ysconf        (*KERNEL->dos_tab->p_s_ysconf)
#define _p_sigpending    (*KERNEL->dos_tab->p_p_sigpending)
#define _d_pathconf      (*KERNEL->dos_tab->p_d_pathconf)
#define _p_msg           (*KERNEL->dos_tab->p_p_msg)
#define _f_midipipe      (*KERNEL->dos_tab->p_f_midipipe)
#define _p_renice        (*KERNEL->dos_tab->p_p_renice)
#define _d_opendir       (*KERNEL->dos_tab->p_d_opendir)
#define _d_readdir       (*KERNEL->dos_tab->p_d_readdir)
#define _d_rewind        (*KERNEL->dos_tab->p_d_rewind)
#define _d_closedir      (*KERNEL->dos_tab->p_d_closedir)
#define _f_xattr         (*KERNEL->dos_tab->p_f_xattr)
#define _f_link          (*KERNEL->dos_tab->p_f_link)
#define _f_symlink       (*KERNEL->dos_tab->p_f_symlink)
#define _f_readlink      (*KERNEL->dos_tab->p_f_readlink)

#define _d_cntl          (*KERNEL->dos_tab->p_d_cntl)
#define _f_chown         (*KERNEL->dos_tab->p_f_chown)
#define _f_chmod         (*KERNEL->dos_tab->p_f_chmod)
#define _p_umask         (*KERNEL->dos_tab->p_p_umask)
#define _p_semaphore     (*KERNEL->dos_tab->p_p_semaphore)
#define _d_lock          (*KERNEL->dos_tab->p_d_lock)
#define _p_sigpause      (*KERNEL->dos_tab->p_p_sigpause)
#define _p_sigaction     (*KERNEL->dos_tab->p_p_sigaction)
#define _p_geteuid       (*KERNEL->dos_tab->p_p_geteuid)
#define _p_getegid       (*KERNEL->dos_tab->p_p_getegid)
#define _p_waitpid       (*KERNEL->dos_tab->p_p_waitpid)
#define _d_getcwd        (*KERNEL->dos_tab->p_d_getcwd)
#define _s_alert         (*KERNEL->dos_tab->p_s_alert)
#define _t_malarm        (*KERNEL->dos_tab->p_t_malarm)
#define _s_uptime        (*KERNEL->dos_tab->p_s_uptime)

#define _p_trace         (*KERNEL->dos_tab->p_p_trace)
#define _m_validate      (*KERNEL->dos_tab->p_m_validate)
#define _d_xreaddir      (*KERNEL->dos_tab->p_d_xreaddir)
#define _p_seteuid       (*KERNEL->dos_tab->p_p_seteuid)
#define _p_setegid       (*KERNEL->dos_tab->p_p_setegid)
#define _p_getauid       (*KERNEL->dos_tab->p_p_getauid)
#define _p_setauid       (*KERNEL->dos_tab->p_p_setauid)
#define _p_getgroups     (*KERNEL->dos_tab->p_p_getgroups)
#define _p_setgroups     (*KERNEL->dos_tab->p_p_setgroups)
#define _t_setitimer     (*KERNEL->dos_tab->p_t_setitimer)
#define _d_chroot        (*KERNEL->dos_tab->p_d_chroot)  /* 1.15.3 */
#define _f_stat64        (*KERNEL->dos_tab->p_f_stat64)  /* f_stat */
#define _f_seek64        (*KERNEL->dos_tab->p_f_seek64)
#define _d_setkey        (*KERNEL->dos_tab->p_d_setkey)
#define _p_setreuid      (*KERNEL->dos_tab->p_p_setreuid)
#define _p_setregid      (*KERNEL->dos_tab->p_p_setregid)

#define _s_ync           (*KERNEL->dos_tab->p_s_ync)
#define _s_hutdown       (*KERNEL->dos_tab->p_s_hutdown)
#define _d_readlabel     (*KERNEL->dos_tab->p_d_readlabel)
#define _d_writelabel    (*KERNEL->dos_tab->p_d_writelabel)
#define _s_system        (*KERNEL->dos_tab->p_s_system)
#define _t_gettimeofday  (*KERNEL->dos_tab->p_t_gettimeofday)
#define _t_settimeofday  (*KERNEL->dos_tab->p_t_settimeofday)
#define _t_adjtime       (*KERNEL->dos_tab->p_t_adjtime)
#define _p_getpriority   (*KERNEL->dos_tab->p_p_getpriority)
#define _p_setpriority   (*KERNEL->dos_tab->p_p_setpriority)
#define _f_poll          (*KERNEL->dos_tab->p_f_poll)
#define _f_writev        (*KERNEL->dos_tab->p_f_writev)
#define _f_readv         (*KERNEL->dos_tab->p_f_readv)
#define _f_fstat         (*KERNEL->dos_tab->p_f_fstat)
#define _p_sysctl        (*KERNEL->dos_tab->p_p_sysctl)
#define _s_emulation     (*KERNEL->dos_tab->p_s_emulation)

#define _f_socket        (*KERNEL->dos_tab->p_f_socket)
#define _f_socketpair    (*KERNEL->dos_tab->p_f_socketpair)
#define _f_accept        (*KERNEL->dos_tab->p_f_accept)
#define _f_connect       (*KERNEL->dos_tab->p_f_connect)
#define _f_bind          (*KERNEL->dos_tab->p_f_bind)
#define _f_listen        (*KERNEL->dos_tab->p_f_listen)
#define _f_recvmsg       (*KERNEL->dos_tab->p_f_recvmsg)
#define _f_sendmsg       (*KERNEL->dos_tab->p_f_sendmsg)
#define _f_recvfrom      (*KERNEL->dos_tab->p_f_recvfrom)
#define _f_sendto        (*KERNEL->dos_tab->p_f_sendto)
#define _f_setsockopt    (*KERNEL->dos_tab->p_f_setsockopt)
#define _f_getsockopt    (*KERNEL->dos_tab->p_f_getsockopt)
#define _f_getpeername   (*KERNEL->dos_tab->p_f_getpeername)
#define _f_getsockname   (*KERNEL->dos_tab->p_f_getsockname)
#define _f_shutdown      (*KERNEL->dos_tab->p_f_shutdown)

#define _p_shmget        (*KERNEL->dos_tab->p_p_shmget)
#define _p_shmctl        (*KERNEL->dos_tab->p_p_shmctl)
#define _p_shmat         (*KERNEL->dos_tab->p_p_shmat)
#define _p_shmdt         (*KERNEL->dos_tab->p_p_shmdt)
#define _p_semget        (*KERNEL->dos_tab->p_p_semget)
#define _p_semctl        (*KERNEL->dos_tab->p_p_semctl)
#define _p_semop         (*KERNEL->dos_tab->p_p_semop)
#define _p_semconfig     (*KERNEL->dos_tab->p_p_semconfig)
#define _p_msgget        (*KERNEL->dos_tab->p_p_msgget)
#define _p_msgctl        (*KERNEL->dos_tab->p_p_msgctl)
#define _p_msgsnd        (*KERNEL->dos_tab->p_p_msgsnd)
#define _p_msgrcv        (*KERNEL->dos_tab->p_p_msgrcv)
#define _m_access        (*KERNEL->dos_tab->p_m_access)
#define _m_map           (*KERNEL->dos_tab->p_m_map)
#define _m_unmap         (*KERNEL->dos_tab->p_m_unmap)

#define _f_chown16       (*KERNEL->dos_tab->p_f_chown16)
#define _f_chdir         (*KERNEL->dos_tab->p_f_chdir)
#define _f_opendir       (*KERNEL->dos_tab->p_f_opendir)
#define _f_dirfd         (*KERNEL->dos_tab->p_f_dirfd)

INLINE long c_conws(const char *str)
{ return _c_conws(str); }

INLINE long c_conout(const int c)
{ return _c_conout(c); }

INLINE long f_setdta(DTABUF *dta)
{ return _f_setdta(dta); }

INLINE long t_getdate(void)
{ return _t_getdate(); }

INLINE long _cdecl t_gettime(void)
{ return _t_gettime(); }

INLINE struct dtabuf *f_getdta(void)
{ return _f_getdta(); }

INLINE long d_setpath(const char *path)
{ return _d_setpath(path); }

INLINE long f_create(const char *name, int attrib)
{ return _f_create(name, attrib); }

INLINE long f_open(const char *name, int mode)
{ return _f_open(name, mode); }

INLINE long f_close(int fh)
{ return _f_close(fh); }

INLINE long f_read(int fh, long count, void *buf)
{ return _f_read(fh, count, buf); }

INLINE long f_write(int fh, long count, const void *buf)
{ return _f_write(fh, count, buf); }

INLINE long d_getpath(char *path, int drv)
{ return _d_getpath(path, drv); }

INLINE long m_xalloc(long size, short mode)
{ return _m_xalloc(size, mode); }

INLINE long f_dup(short fd)
{ return _f_dup(fd); }

INLINE long m_alloc(long size)
{ return _m_alloc(size); }

INLINE long m_free(void *block)
{ return _m_free(block); }

INLINE long m_shrink(int dummy, void *block, long size)
{ return _m_shrink(dummy, block, size); }

INLINE long p_exec(short mode, const void *ptr1, const void *ptr2, const void *ptr3)
{ return _p_exec(mode, ptr1, ptr2, ptr3); }

INLINE long p_term(short code)
{ return _p_term(code); }

INLINE long f_sfirst(const char *path, int attrib)
{ return _f_sfirst(path, attrib); }

INLINE long f_snext(void)
{ return _f_snext(); }

INLINE long s_yield(void)
{ return _s_yield(); }

INLINE long f_cntl(short fh, long arg, short cmd)
{ return _f_cntl(fh, arg, cmd); }

INLINE long p_nice(short increment)
{ return _p_nice(increment); }

INLINE long p_getpid(void)
{ return _p_getpid(); }

INLINE long p_getppid(void)
{ return _p_getppid(); }

INLINE long p_getpgrp(void)
{ return _p_getpgrp(); }

INLINE long p_setpgrp(int pid, int newgrp)
{ return _p_setpgrp(pid, newgrp); }

INLINE long p_getuid(void)
{ return _p_getuid(); }

INLINE long p_setuid(unsigned short id)
{ return _p_setuid(id); }

INLINE long p_kill(short pid, short sig)
{ return _p_kill(pid, sig); }

INLINE long p_getgid(void)
{ return _p_getgid(); }

INLINE long p_setgid(unsigned short id)
{ return _p_setgid(id); }

INLINE long p_sigblock(ulong mask)
{ return _p_sigblock(mask); }

INLINE long p_domain(int arg)
{ return _p_domain(arg); }

INLINE long f_select(unsigned short timeout, long *rfdp, long *wfdp, long *xfdp)
{ return _f_select(timeout, rfdp, wfdp, xfdp); }

INLINE long s_ysconf(int which)
{ return _s_ysconf(which); }

INLINE long d_pathconf(const char *name, int which)
{ return _d_pathconf(name, which); }

INLINE long p_msg(int mode, long _cdecl mbid, char *ptr)
{ return _p_msg(mode, mbid, ptr); }

INLINE long d_opendir(const char *path, int flags)
{ return _d_opendir(path, flags); }

INLINE long d_readdir(int len, long handle, char *buf)
{ return _d_readdir(len, handle, buf); }

INLINE long d_closedir(long handle)
{ return _d_closedir(handle); }

INLINE long f_xattr(int flag, const char *name, XATTR *xattr)
{ return _f_xattr(flag, name, xattr); }

INLINE long d_cntl(int cmd, const char *name, long arg)
{ return _d_cntl(cmd, name, arg); }

INLINE long f_chown(const char *name, int uid, int gid)
{ return _f_chown(name, uid, gid); }

INLINE long f_chmod(const char *name, unsigned short mode)
{ return _f_chmod(name, mode); }

INLINE long p_semaphore(int mode, long id, long timeout)
{ return _p_semaphore(mode, id, timeout); }

INLINE long d_lock(int mode, int drv)
{ return _d_lock(mode, drv); }

INLINE long p_geteuid(void)
{ return _p_geteuid(); }

INLINE long p_getegid(void)
{ return _p_getegid(); }

INLINE long p_seteuid(unsigned short id)
{ return _p_seteuid(id); }

INLINE long p_setegid(unsigned short id)
{ return _p_setegid(id); }

INLINE long p_getauid(void)
{ return _p_getauid(); }

INLINE long p_setauid(int id)
{ return _p_setauid(id); }

INLINE long p_getgroups(short gidsetlen, unsigned short gidset[])
{ return _p_getgroups(gidsetlen, gidset); }

INLINE long p_setgroups(short ngroups, unsigned short gidset[])
{ return _p_setgroups(ngroups, gidset); }

INLINE long p_setreuid(unsigned short rid, unsigned short eid)
{ return _p_setreuid(rid, eid); }

INLINE long p_setregid(unsigned short rid, unsigned short eid)
{ return _p_setregid(rid, eid); }

INLINE long s_ync(void)
{ return _s_ync(); }

INLINE long s_hutdown(long restart)
{ return _s_hutdown(restart); }

INLINE long s_system(int mode, unsigned long arg1, unsigned long arg2)
{ return _s_system(mode, arg1, arg2); }

INLINE long p_sysctl(long *name, unsigned long namelen, void *old, unsigned long *oldlenp, const void *new, unsigned long newlen)
{ return _p_sysctl(name, namelen, old, oldlenp, new, newlen); }


#define datestamp      t_getdate()
#define timestamp      t_gettime()
#define FreeMemory     m_alloc(-1L)


#define changedrive        (*KERNEL->drvchng)
#define KERNEL_TRACE       (*KERNEL->trace)
#define KERNEL_DEBUG       (*KERNEL->debug)
#define KERNEL_ALERT       (*KERNEL->alert)
#define FATAL          (*KERNEL->fatal)

#define kmalloc        (*KERNEL->kmalloc)
#define kfree          (*KERNEL->kfree)
#define umalloc        (*KERNEL->umalloc)
#define ufree          (*KERNEL->ufree)

#undef strnicmp
#define strnicmp       (*KERNEL->kstrnicmp)
#undef stricmp
#define stricmp        (*KERNEL->kstricmp)
#undef strlwr
#define strlwr         (*KERNEL->kstrlwr)
#undef strupr
#define strupr         (*KERNEL->kstrupr)
#undef ksprintf
#define ksprintf       (*KERNEL->ksprintf)

#define ms_time        (*KERNEL->millis_time)
#define unixtime       (*KERNEL->unixtime)
#define dostime        (*KERNEL->dostime)

#define nap            (*KERNEL->nap)
#define sleep          (*KERNEL->sleep)

/* for sleep */
#define CURPROC_Q      0
#define READY_Q        1
#define WAIT_Q         2
#define IO_Q           3
#define ZOMBIE_Q       4
#define TSR_Q          5
#define STOP_Q         6
#define SELECT_Q       7

#define wake               (*KERNEL->wake)
#define wakeselect         (*KERNEL->wakeselect)
#define denyshare          (*KERNEL->denyshare)
#define denylock           (*KERNEL->denylock)
#define addtimeout         (*KERNEL->addtimeout)
#define canceltimeout      (*KERNEL->canceltimeout)
#define addroottimeout     (*KERNEL->addroottimeout)
#define cancelroottimeout  (*KERNEL->cancelroottimeout)
#define ikill              (*KERNEL->ikill)
#define iwake              (*KERNEL->iwake)
#define bio                (*KERNEL->bio)
#define utc                (*KERNEL->xtime)
#define add_rsvfentry      (*KERNEL->add_rsvfentry)
#define del_rsvfentry      (*KERNEL->del_rsvfentry)
#define killgroup          (*KERNEL->killgroup)
#define dma_interface      ( KERNEL->dma)
#define loops_per_sec_ptr  ( KERNEL->loops_per_sec)
#define get_toscookie      (*KERNEL->get_toscookie)
#define so_register        (*KERNEL->so_register)
#define so_unregister      (*KERNEL->so_unregister)
#define so_release         (*KERNEL->so_release)
#define so_sockpair        (*KERNEL->so_sockpair)
#define so_connect         (*KERNEL->so_connect)
#define so_accept          (*KERNEL->so_accept)
#define so_create          (*KERNEL->so_create)
#define so_dup             (*KERNEL->so_dup)
#define so_free            (*KERNEL->so_free)
#define load_modules       (*KERNEL->load_modules)
#define kthread_create     (*KERNEL->kthread_create)
#define kthread_exit       (*KERNEL->kthread_exit)

#endif /* _libkern_kernel_xfs_xdd_h */
