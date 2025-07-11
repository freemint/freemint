/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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

#ifndef _libkern_kernel_module_h
#define _libkern_kernel_module_h

#include "mint/kentry.h"
#include "buildinfo/version.h"


#ifndef MODULE_NAME
#error MODULE_NAME not defined!
#endif

#undef FUNCTION
#define FUNCTION str(MODULE_NAME)":"__FILE__","str(__LINE__)

#ifndef KENTRY
#define KENTRY kentry
#endif

extern struct kentry *kentry;

#define MINT_MAJOR         (KENTRY->major)
#define MINT_MINOR         (KENTRY->minor)
#define MINT_PATCHLEVEL    (KENTRY->patchlevel)

/*
 * vec_dos
 */

#define _p_term0         (*KENTRY->vec_dos->p_pterm0)
#define _c_conin         (*KENTRY->vec_dos->p_c_conin)
#define _c_conout        (*KENTRY->vec_dos->p_c_conout)
#define _c_auxin         (*KENTRY->vec_dos->p_c_auxin)
#define _c_auxout        (*KENTRY->vec_dos->p_c_auxout)
#define _c_prnout        (*KENTRY->vec_dos->p_c_prnout)
#define _c_rawio         (*KENTRY->vec_dos->p_c_rawio)
#define _c_rawcin        (*KENTRY->vec_dos->p_c_rawcin)
#define _c_necin         (*KENTRY->vec_dos->p_c_necin)
#define _c_conws         (*KENTRY->vec_dos->p_c_conws)
#define _c_conrs         (*KENTRY->vec_dos->p_c_conrs)
#define _c_conis         (*KENTRY->vec_dos->p_c_conis)
#define _d_setdrv        (*KENTRY->vec_dos->p_d_setdrv)

#define _c_conos         (*KENTRY->vec_dos->p_c_conos)
#define _c_prnos         (*KENTRY->vec_dos->p_c_prnos)
#define _c_auxis         (*KENTRY->vec_dos->p_c_auxis)
#define _c_auxos         (*KENTRY->vec_dos->p_c_auxos)
#define _m_addalt        (*KENTRY->vec_dos->p_m_addalt)
#define _s_realloc       (*KENTRY->vec_dos->p_s_realloc)
#define _s_lbopen        (*KENTRY->vec_dos->p_s_lbopen)  /* 1.15.3 MagiC: slb */
#define _s_lbclose       (*KENTRY->vec_dos->p_s_lbclose) /* 1.15.3 MagiC: slb */
#define _d_getdrv        (*KENTRY->vec_dos->p_d_getdrv)
#define _f_setdta        (*KENTRY->vec_dos->p_f_setdta)

#define _s_uper          (*KENTRY->vec_dos->p_s_uper)
#define _t_getdate       (*KENTRY->vec_dos->p_t_getdate)
#define _t_setdate       (*KENTRY->vec_dos->p_t_setdate)
#define _t_gettime       (*KENTRY->vec_dos->p_t_gettime)
#define _t_settime       (*KENTRY->vec_dos->p_t_settime)
#define _f_getdta        (*KENTRY->vec_dos->p_f_getdta)

#define _s_version       (*KENTRY->vec_dos->p_s_version)
#define _p_termres       (*KENTRY->vec_dos->p_p_termres)
#define _s_config        (*KENTRY->vec_dos->p_s_config)  /* MagiC: long Sconfig(short subfn, long flags); */
#define _d_free          (*KENTRY->vec_dos->p_d_free)
#define _d_create        (*KENTRY->vec_dos->p_d_create)
#define _d_delete        (*KENTRY->vec_dos->p_d_delete)
#define _d_setpath       (*KENTRY->vec_dos->p_d_setpath)
#define _f_create        (*KENTRY->vec_dos->p_f_create)
#define _f_open          (*KENTRY->vec_dos->p_f_open)
#define _f_close         (*KENTRY->vec_dos->p_f_close)
#define _f_read          (*KENTRY->vec_dos->p_f_read)

#define _f_write         (*KENTRY->vec_dos->p_f_write)
#define _f_delete        (*KENTRY->vec_dos->p_f_delete)
#define _f_seek          (*KENTRY->vec_dos->p_f_seek)
#define _f_attrib        (*KENTRY->vec_dos->p_f_attrib)
#define _m_xalloc        (*KENTRY->vec_dos->p_m_xalloc)
#define _f_dup           (*KENTRY->vec_dos->p_f_dup)
#define _f_force         (*KENTRY->vec_dos->p_f_force)
#define _d_getpath       (*KENTRY->vec_dos->p_d_getpath)
#define _m_alloc         (*KENTRY->vec_dos->p_m_alloc)
#define _m_free          (*KENTRY->vec_dos->p_m_free)
#define _m_shrink        (*KENTRY->vec_dos->p_m_shrink)
#define _p_exec          (*KENTRY->vec_dos->p_p_exec)
#define _p_term          (*KENTRY->vec_dos->p_p_term)
#define _f_sfirst        (*KENTRY->vec_dos->p_f_sfirst)
#define _f_snext         (*KENTRY->vec_dos->p_f_snext)

#define _f_rename        (*KENTRY->vec_dos->p_f_rename)
#define _f_datime        (*KENTRY->vec_dos->p_f_datime)
#define _f_lock          (*KENTRY->vec_dos->p_f_lock)

/*
 * MiNT extensions to GEMDOS
 */

#define _s_yield         (*KENTRY->vec_dos->p_s_yield)

#define _f_pipe          (*KENTRY->vec_dos->p_f_pipe)
#define _f_fchown        (*KENTRY->vec_dos->p_f_fchown)  /* 1.15.2 */
#define _f_fchmod        (*KENTRY->vec_dos->p_f_fchmod)  /* 1.15.2 */
#define _f_sync          (*KENTRY->vec_dos->p_f_sync)
#define _f_cntl          (*KENTRY->vec_dos->p_f_cntl)
#define _f_instat        (*KENTRY->vec_dos->p_f_instat)
#define _f_outstat       (*KENTRY->vec_dos->p_f_outstat)
#define _f_getchar       (*KENTRY->vec_dos->p_f_getchar)
#define _f_putchar       (*KENTRY->vec_dos->p_f_putchar)
#define _p_wait          (*KENTRY->vec_dos->p_p_wait)
#define _p_nice          (*KENTRY->vec_dos->p_p_nice)
#define _p_getpid        (*KENTRY->vec_dos->p_p_getpid)
#define _p_getppid       (*KENTRY->vec_dos->p_p_getppid)
#define _p_getpgrp       (*KENTRY->vec_dos->p_p_getpgrp)
#define _p_setpgrp       (*KENTRY->vec_dos->p_p_setpgrp)
#define _p_getuid        (*KENTRY->vec_dos->p_p_getuid)

#define _p_setuid        (*KENTRY->vec_dos->p_p_setuid)
#define _p_kill          (*KENTRY->vec_dos->p_p_kill)
#define _p_signal        (*KENTRY->vec_dos->p_p_signal)
#define _p_vfork         (*KENTRY->vec_dos->p_p_vfork)
#define _p_getgid        (*KENTRY->vec_dos->p_p_getgid)
#define _p_setgid        (*KENTRY->vec_dos->p_p_setgid)
#define _p_sigblock      (*KENTRY->vec_dos->p_p_sigblock)
#define _p_sigsetmask    (*KENTRY->vec_dos->p_p_sigsetmask)
#define _p_usrval        (*KENTRY->vec_dos->p_p_usrval)
#define _p_domain        (*KENTRY->vec_dos->p_p_domain)
#define _p_sigreturn     (*KENTRY->vec_dos->p_p_sigreturn)
#define _p_fork          (*KENTRY->vec_dos->p_p_fork)
#define _p_wait3         (*KENTRY->vec_dos->p_p_wait3)
#define _f_select        (*KENTRY->vec_dos->p_f_select)
#define _p_rusage        (*KENTRY->vec_dos->p_p_rusage)
#define _p_setlimit      (*KENTRY->vec_dos->p_p_setlimit)

#define _t_alarm         (*KENTRY->vec_dos->p_t_alarm)
#define _p_pause         (*KENTRY->vec_dos->p_p_pause)
#define _s_ysconf        (*KENTRY->vec_dos->p_s_ysconf)
#define _p_sigpending    (*KENTRY->vec_dos->p_p_sigpending)
#define _d_pathconf      (*KENTRY->vec_dos->p_d_pathconf)
#define _p_msg           (*KENTRY->vec_dos->p_p_msg)
#define _f_midipipe      (*KENTRY->vec_dos->p_f_midipipe)
#define _p_renice        (*KENTRY->vec_dos->p_p_renice)
#define _d_opendir       (*KENTRY->vec_dos->p_d_opendir)
#define _d_readdir       (*KENTRY->vec_dos->p_d_readdir)
#define _d_rewind        (*KENTRY->vec_dos->p_d_rewind)
#define _d_closedir      (*KENTRY->vec_dos->p_d_closedir)
#define _f_xattr         (*KENTRY->vec_dos->p_f_xattr)
#define _f_link          (*KENTRY->vec_dos->p_f_link)
#define _f_symlink       (*KENTRY->vec_dos->p_f_symlink)
#define _f_readlink      (*KENTRY->vec_dos->p_f_readlink)

#define _d_cntl          (*KENTRY->vec_dos->p_d_cntl)
#define _f_chown         (*KENTRY->vec_dos->p_f_chown)
#define _f_chmod         (*KENTRY->vec_dos->p_f_chmod)
#define _p_umask         (*KENTRY->vec_dos->p_p_umask)
#define _p_semaphore     (*KENTRY->vec_dos->p_p_semaphore)
#define _d_lock          (*KENTRY->vec_dos->p_d_lock)
#define _p_sigpause      (*KENTRY->vec_dos->p_p_sigpause)
#define _p_sigaction     (*KENTRY->vec_dos->p_p_sigaction)
#define _p_geteuid       (*KENTRY->vec_dos->p_p_geteuid)
#define _p_getegid       (*KENTRY->vec_dos->p_p_getegid)
#define _p_waitpid       (*KENTRY->vec_dos->p_p_waitpid)
#define _d_getcwd        (*KENTRY->vec_dos->p_d_getcwd)
#define _s_alert         (*KENTRY->vec_dos->p_s_alert)
#define _t_malarm        (*KENTRY->vec_dos->p_t_malarm)
#define _s_uptime        (*KENTRY->vec_dos->p_s_uptime)

#define _p_trace         (*KENTRY->vec_dos->p_p_trace)
#define _m_validate      (*KENTRY->vec_dos->p_m_validate)
#define _d_xreaddir      (*KENTRY->vec_dos->p_d_xreaddir)
#define _p_seteuid       (*KENTRY->vec_dos->p_p_seteuid)
#define _p_setegid       (*KENTRY->vec_dos->p_p_setegid)
#define _p_getauid       (*KENTRY->vec_dos->p_p_getauid)
#define _p_setauid       (*KENTRY->vec_dos->p_p_setauid)
#define _p_getgroups     (*KENTRY->vec_dos->p_p_getgroups)
#define _p_setgroups     (*KENTRY->vec_dos->p_p_setgroups)
#define _t_setitimer     (*KENTRY->vec_dos->p_t_setitimer)
#define _d_chroot        (*KENTRY->vec_dos->p_d_chroot)  /* 1.15.3 */
#define _f_stat64        (*KENTRY->vec_dos->p_f_stat64)  /* f_stat */
#define _f_seek64        (*KENTRY->vec_dos->p_f_seek64)
#define _d_setkey        (*KENTRY->vec_dos->p_d_setkey)
#define _p_setreuid      (*KENTRY->vec_dos->p_p_setreuid)
#define _p_setregid      (*KENTRY->vec_dos->p_p_setregid)

#define _s_ync           (*KENTRY->vec_dos->p_s_ync)
#define _s_hutdown       (*KENTRY->vec_dos->p_s_hutdown)
#define _d_readlabel     (*KENTRY->vec_dos->p_d_readlabel)
#define _d_writelabel    (*KENTRY->vec_dos->p_d_writelabel)
#define _s_system        (*KENTRY->vec_dos->p_s_system)
#define _t_gettimeofday  (*KENTRY->vec_dos->p_t_gettimeofday)
#define _t_settimeofday  (*KENTRY->vec_dos->p_t_settimeofday)
#define _t_adjtime       (*KENTRY->vec_dos->p_t_adjtime)
#define _p_getpriority   (*KENTRY->vec_dos->p_p_getpriority)
#define _p_setpriority   (*KENTRY->vec_dos->p_p_setpriority)
#define _f_poll          (*KENTRY->vec_dos->p_f_poll)
#define _f_writev        (*KENTRY->vec_dos->p_f_writev)
#define _f_readv         (*KENTRY->vec_dos->p_f_readv)
#define _f_fstat         (*KENTRY->vec_dos->p_f_fstat)
#define _p_sysctl        (*KENTRY->vec_dos->p_p_sysctl)
#define _s_emulation     (*KENTRY->vec_dos->p_s_emulation)

#define _f_socket        (*KENTRY->vec_dos->p_f_socket)
#define _f_socketpair    (*KENTRY->vec_dos->p_f_socketpair)
#define _f_accept        (*KENTRY->vec_dos->p_f_accept)
#define _f_connect       (*KENTRY->vec_dos->p_f_connect)
#define _f_bind          (*KENTRY->vec_dos->p_f_bind)
#define _f_listen        (*KENTRY->vec_dos->p_f_listen)
#define _f_recvmsg       (*KENTRY->vec_dos->p_f_recvmsg)
#define _f_sendmsg       (*KENTRY->vec_dos->p_f_sendmsg)
#define _f_recvfrom      (*KENTRY->vec_dos->p_f_recvfrom)
#define _f_sendto        (*KENTRY->vec_dos->p_f_sendto)
#define _f_setsockopt    (*KENTRY->vec_dos->p_f_setsockopt)
#define _f_getsockopt    (*KENTRY->vec_dos->p_f_getsockopt)
#define _f_getpeername   (*KENTRY->vec_dos->p_f_getpeername)
#define _f_getsockname   (*KENTRY->vec_dos->p_f_getsockname)
#define _f_shutdown      (*KENTRY->vec_dos->p_f_shutdown)

#define _p_shmget        (*KENTRY->vec_dos->p_p_shmget)
#define _p_shmctl        (*KENTRY->vec_dos->p_p_shmctl)
#define _p_shmat         (*KENTRY->vec_dos->p_p_shmat)
#define _p_shmdt         (*KENTRY->vec_dos->p_p_shmdt)
#define _p_semget        (*KENTRY->vec_dos->p_p_semget)
#define _p_semctl        (*KENTRY->vec_dos->p_p_semctl)
#define _p_semop         (*KENTRY->vec_dos->p_p_semop)
#define _p_semconfig     (*KENTRY->vec_dos->p_p_semconfig)
#define _p_msgget        (*KENTRY->vec_dos->p_p_msgget)
#define _p_msgctl        (*KENTRY->vec_dos->p_p_msgctl)
#define _p_msgsnd        (*KENTRY->vec_dos->p_p_msgsnd)
#define _p_msgrcv        (*KENTRY->vec_dos->p_p_msgrcv)
#define _m_access        (*KENTRY->vec_dos->p_m_access)
#define _m_map           (*KENTRY->vec_dos->p_m_map)
#define _m_unmap         (*KENTRY->vec_dos->p_m_unmap)

#define _f_chown16       (*KENTRY->vec_dos->p_f_chown16)
#define _f_chdir         (*KENTRY->vec_dos->p_f_chdir)
#define _f_opendir       (*KENTRY->vec_dos->p_f_opendir)
#define _f_dirfd         (*KENTRY->vec_dos->p_f_dirfd)

INLINE long c_conws(const char *str)
{ return _c_conws(str); }

INLINE long c_conout(const int c)
{ return _c_conout(c); }

INLINE long d_setdrv(int drv)
{ return _d_setdrv(drv); }

INLINE long d_getdrv(void)
{ return _d_getdrv(); }

INLINE long f_setdta(DTABUF *dta)
{ return _f_setdta(dta); }

INLINE long t_getdate(void)
{ return _t_getdate(); }

INLINE long _cdecl t_gettime(void)
{ return _t_gettime(); }

INLINE struct dtabuf *f_getdta(void)
{ return _f_getdta(); }

INLINE long s_version(void)
{ return _s_version(); }

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

INLINE long f_instat(int fh)
{ return _f_instat(fh); }

INLINE long f_getchar(int fh, int mode)
{ return _f_getchar(fh, mode); }

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

INLINE long p_signal(short sig, long handler)
{ return _p_signal(sig, handler); }

INLINE long p_getgid(void)
{ return _p_getgid(); }

INLINE long p_setgid(unsigned short id)
{ return _p_setgid(id); }

INLINE long p_sigblock(ulong mask)
{ return _p_sigblock(mask); }

INLINE long p_sigsetmask(ulong mask)
{ return _p_sigsetmask(mask); }

INLINE long p_domain(int arg)
{ return _p_domain(arg); }

INLINE long f_select(unsigned short timeout, long *rfdp, long *wfdp, long *xfdp)
{ return _f_select(timeout, rfdp, wfdp, xfdp); }

INLINE long p_setlimit(short i, long v)
{ return _p_setlimit(i, v); }

INLINE long s_ysconf(int which)
{ return _s_ysconf(which); }

INLINE long d_pathconf(const char *name, int which)
{ return _d_pathconf(name, which); }

INLINE long p_msg(int mode, long _cdecl mbid, char *ptr)
{ return _p_msg(mode, mbid, ptr); }

INLINE long p_renice(short pid, short increment)
{ return _p_renice(pid, increment); }

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

INLINE long p_sigaction(short sig, const struct sigaction *act, struct sigaction *oact)
{ return _p_sigaction(sig, act, oact); }

INLINE long p_geteuid(void)
{ return _p_geteuid(); }

INLINE long p_getegid(void)
{ return _p_getegid(); }

INLINE long p_waitpid(short pid, short nohang, long *rusage)
{ return _p_waitpid(pid, nohang, rusage); }

INLINE long d_getcwd(char *path, int drv, int size)
{ return _d_getcwd(path, drv, size); }

INLINE long d_xreaddir(int len, long handle, char *buf, XATTR *xattr, long *xret)
{ return _d_xreaddir(len, handle, buf, xattr, xret); }

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

INLINE long f_stat64(int flag, const char *name, struct stat *st)
{ return _f_stat64(flag, name, st); }

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

INLINE long p_getpriority(short which, short who)
{ return _p_getpriority(which, who); }

INLINE long p_setpriority(short which, short who, short prio)
{ return _p_setpriority(which, who, prio); }

INLINE long p_sysctl(long *name, unsigned long namelen, void *old, unsigned long *oldlenp, const void *new, unsigned long newlen)
{ return _p_sysctl(name, namelen, old, oldlenp, new, newlen); }


/*
 * vec_bios
 */

#define _b_ubconstat     (*KENTRY->vec_bios->p_b_bconstat)
#define _b_ubconin       (*KENTRY->vec_bios->p_b_bconin)
#define _b_ubconout      (*KENTRY->vec_bios->p_b_bconout)
#define _b_rwabs         (*KENTRY->vec_bios->p_b_rwabs)
#define _b_setexc        (*KENTRY->vec_bios->p_b_setexc)
#define _b_tickcal       (*KENTRY->vec_bios->p_b_tickcal)
#define _b_getbpb        (*KENTRY->vec_bios->p_b_getbpb)
#define _b_ubcostat      (*KENTRY->vec_bios->p_b_bcostat)
#define _b_mediach       (*KENTRY->vec_bios->p_b_mediach)
#define _b_drvmap        (*KENTRY->vec_bios->p_b_drvmap)
#define _b_kbshift       (*KENTRY->vec_bios->p_b_kbshift)

INLINE long b_ubconin(int dev)
{ return _b_ubconin(dev); }

INLINE long b_ubconout(short dev, short c)
{ return _b_ubconout(dev, c); }

INLINE long b_setexc(short number, long vector)
{ return _b_setexc(number, vector); }

INLINE long b_drvmap(void)
{ return _b_drvmap(); }


/*
 * vec_xbios
 */

#define _b_getrez        (*KENTRY->vec_xbios->p_b_getrez)
#define _b_vsetscreen    (*KENTRY->vec_xbios->p_b_vsetscreen)
#define _b_midiws        (*KENTRY->vec_xbios->p_b_midiws)
#define _b_uiorec        (*KENTRY->vec_xbios->p_b_iorec)
#define _b_ursconf       (*KENTRY->vec_xbios->p_b_rsconf)

#define _b_keytbl        (*KENTRY->vec_xbios->p_b_keytbl)
#define _b_random        (*KENTRY->vec_xbios->p_b_random)
#define _b_cursconf      (*KENTRY->vec_xbios->p_b_cursconf)
#define _b_settime       (*KENTRY->vec_xbios->p_b_settime)
#define _b_gettime       (*KENTRY->vec_xbios->p_b_gettime)
#define _b_bioskeys      (*KENTRY->vec_xbios->p_b_bioskeys)

#define _b_dosound       (*KENTRY->vec_xbios->p_b_dosound)
#define _b_kbdvbase      (*KENTRY->vec_xbios->p_b_kbdvbase)
#define _b_kbrate        (*KENTRY->vec_xbios->p_b_kbrate)
#define _b_supexec       (*KENTRY->vec_xbios->p_b_supexec)
#define _b_bconmap       (*KENTRY->vec_xbios->p_b_bconmap)

INLINE short b_getrez(void)
{ return _b_getrez(); }

INLINE struct kbdvbase  *b_kbdvbase(void)
{ return _b_kbdvbase(); }

INLINE struct io_rec *b_uiorec(int dev)
{ return _b_uiorec(dev); }

INLINE long b_supexec(Func funcptr, long a1, long a2, long a3, long a4, long a5)
{ return _b_supexec(funcptr, a1, a2, a3, a4, a5); }

#include "strings.h"
/*
 * generic version check
 */

INLINE long
check_kentry_version(void)
{
	if ((MINT_MAJOR != MINT_MAJ_VERSION) || (MINT_MINOR != MINT_MIN_VERSION))
	{
		c_conws(kstrings[Wrong_FreeMiNT]);
		c_conws(kstrings[This_module_is_compiled_against]);
		c_conws(kstrings[MINT_VERS]);
		c_conws(kstrings[KSTR_NL]);
		return -1;
	}
	if ((kentry->version_major != KENTRY_MAJ_VERSION) || (kentry->version_minor != KENTRY_MIN_VERSION))
	{
		int s = 0;
		c_conws(kstrings[Wrong_kentry]);

		if (kentry->version_major == KENTRY_MAJ_VERSION)
		{
			if( kentry->version_minor > KENTRY_MIN_VERSION)
				s = 1;
		}
		else
			if (kentry->version_major > KENTRY_MAJ_VERSION)
				s = 2;
		if( s )
			c_conws(kstrings[high]);
		else
			c_conws(kstrings[low]);
		c_conws(kstrings[KSTR_NL]);
		return -1;
	}
	return 0;
}


/*
 * kentry_mch
 */

#define machine           ( KENTRY->vec_mch.global->machine)
#define fputype           ( KENTRY->vec_mch.global->fputype)
#define tosvers           ( KENTRY->vec_mch.global->tosvers)
#define gl_lang           ( KENTRY->vec_mch.global->gl_lang)
#define gl_kbd            ( KENTRY->vec_mch.global->gl_kbd)
#define sysdrv            ( KENTRY->vec_mch.global->sysdrv)
#define sysdir            ( KENTRY->vec_mch.global->sysdir)
#define loops_per_sec_ptr ( KENTRY->vec_mch.loops_per_sec)
#define c20ms_ptr         ( KENTRY->vec_mch.c20ms)
#define mfpregs           ( KENTRY->vec_mch.mfpregs)
#define cpush             (*KENTRY->vec_mch.cpush)
#define cpushi            (*KENTRY->vec_mch.cpushi)
#define nf_ops            ( KENTRY->vec_mch.nf_ops)


/*
 * kentry_proc
 */

#define nap                (*KENTRY->vec_proc.nap)
#define sleep              (*KENTRY->vec_proc.sleep)
#define wake               (*KENTRY->vec_proc.wake)
#define wakeselect         (*KENTRY->vec_proc.wakeselect)

#define ikill              (*KENTRY->vec_proc.ikill)
#define iwake              (*KENTRY->vec_proc.iwake)
#define killgroup          (*KENTRY->vec_proc.killgroup)
#define raise              (*KENTRY->vec_proc.raise)

#define addtimeout         (*KENTRY->vec_proc.addtimeout)
#define canceltimeout      (*KENTRY->vec_proc.canceltimeout)
#define addroottimeout     (*KENTRY->vec_proc.addroottimeout)
#define cancelroottimeout  (*KENTRY->vec_proc.cancelroottimeout)

#define create_process     (*KENTRY->vec_proc.create_process)

#define kthread_create     (*KENTRY->vec_proc.kthread_create)
#define kthread_exit       (*KENTRY->vec_proc.kthread_exit)

#define get_curproc        (*KENTRY->vec_proc.get_curproc)
#define pid2proc           (*KENTRY->vec_proc.pid2proc)

#define lookup_extension   (*KENTRY->vec_proc.lookup_extension)
#define attach_extension   (*KENTRY->vec_proc.attach_extension)
#define detach_extension   (*KENTRY->vec_proc.detach_extension)

#define proc_setuid        (*KENTRY->vec_proc.proc_setuid)
#define proc_setgid        (*KENTRY->vec_proc.proc_setgid)

/* for sleep */
#define CURPROC_Q      0
#define READY_Q        1
#define WAIT_Q         2
#define IO_Q           3
#define ZOMBIE_Q       4
#define TSR_Q          5
#define STOP_Q         6
#define SELECT_Q       7

#define yield()        sleep(READY_Q, 0L)


/*
 * kentry_mem
 */

#define _kmalloc       (*KENTRY->vec_mem.kmalloc)
#define _kfree         (*KENTRY->vec_mem.kfree)
#define _umalloc       (*KENTRY->vec_mem.umalloc)
#define _ufree         (*KENTRY->vec_mem.ufree)

#define kmalloc(x)     _kmalloc(x, FUNCTION)
#define kfree(x)       _kfree(x, FUNCTION)
#define umalloc(x)     _umalloc(x, FUNCTION)
#define ufree(x)       _ufree(x, FUNCTION)

#define addr2mem       (*KENTRY->vec_mem.addr2mem)
#define attach_region      (*KENTRY->vec_mem.attach_region)
#define detach_region      (*KENTRY->vec_mem.detach_region)


/*
 * kentry_fs
 */

#define DEFAULT_MODE       ( KENTRY->vec_fs.default_perm)
#define DEFAULT_DMODE      ( KENTRY->vec_fs.default_dir_perm)

#define changedrv(drv)     (*KENTRY->vec_fs._changedrv)(drv, FUNCTION)

#define denyshare          (*KENTRY->vec_fs.denyshare)
#define denylock           (*KENTRY->vec_fs.denylock)

#define bio                ( KENTRY->vec_fs.bio)
#define utc                (*KENTRY->vec_fs.xtime)

#define kernel_opendir     (*KENTRY->vec_fs.kernel_opendir)
#define kernel_readdir     (*KENTRY->vec_fs.kernel_readdir)
#define kernel_closedir    (*KENTRY->vec_fs.kernel_closedir)

#define kernel_open        (*KENTRY->vec_fs.kernel_open)
#define kernel_read        (*KENTRY->vec_fs.kernel_read)
#define kernel_write       (*KENTRY->vec_fs.kernel_write)
#define kernel_lseek       (*KENTRY->vec_fs.kernel_lseek)
#define kernel_close       (*KENTRY->vec_fs.kernel_close)

#define path2cookie        (*KENTRY->vec_fs.path2cookie)
#define relpath2cookie     (*KENTRY->vec_fs.relpath2cookie)
#define release_cookie     (*KENTRY->vec_fs.release_cookie)

/*
 * kentry_sockets
 */

#define so_register    (*KENTRY->vec_sockets.so_register)
#define so_unregister  (*KENTRY->vec_sockets.so_unregister)
#define so_release     (*KENTRY->vec_sockets.so_release)
#define so_sockpair    (*KENTRY->vec_sockets.so_sockpair)
#define so_connect     (*KENTRY->vec_sockets.so_connect)
#define so_accept      (*KENTRY->vec_sockets.so_accept)
#define so_create      (*KENTRY->vec_sockets.so_create)
#define so_dup         (*KENTRY->vec_sockets.so_dup)
#define so_free        (*KENTRY->vec_sockets.so_free)


/*
 * kentry_module
 */

#define load_modules       (*KENTRY->vec_module.load_modules)
#define register_trap2     (*KENTRY->vec_module.register_trap2)


/*
 * kentry_cnf
 */

#define parse_cnf      (*KENTRY->vec_cnf.parse_cnf)
#define parse_include  (*KENTRY->vec_cnf.parse_include)
#define parser_msg     (*KENTRY->vec_cnf.parser_msg)


/*
 * kentry_misc
 */

#define dma_interface      ( KENTRY->vec_misc.dma)
#define get_toscookie      (*KENTRY->vec_misc.get_toscookie)
#define add_rsvfentry      (*KENTRY->vec_misc.add_rsvfentry)
#define del_rsvfentry      (*KENTRY->vec_misc.del_rsvfentry)
#define remaining_proc_time    (*KENTRY->vec_misc.remaining_proc_time)

#define trap_1_emu         (*KENTRY->vec_misc.trap_1_emu)
#define trap_13_emu        (*KENTRY->vec_misc.trap_13_emu)
#define trap_14_emu        (*KENTRY->vec_misc.trap_14_emu)

#define ROM_Setexc(vnum,vptr)  (void (*)(void))trap_13_emu(0x05,(short)(vnum),(long)(vptr))


/*
 * kentry_debug
 */
#define DEBUG_LEVEL        (*KENTRY->vec_debug.debug_level)
#define KERNEL_TRACE       (*KENTRY->vec_debug.trace)
#define KERNEL_DEBUG       (*KENTRY->vec_debug.debug)
#define KERNEL_ALERT       (*KENTRY->vec_debug.alert)
#define KERNEL_FATAL       (*KENTRY->vec_debug.fatal)
#define KERNEL_FORCE       (*KENTRY->vec_debug.force)


/*
 * kentry_libkern
 */

#define _ctype         ( KENTRY->vec_libkern._ctype)

#define tolower        (*KENTRY->vec_libkern.tolower)
#define toupper        (*KENTRY->vec_libkern.toupper)

#define vsprintf       (*KENTRY->vec_libkern.vsprintf)
#define sprintf        (*KENTRY->vec_libkern.sprintf)
#define getenv         (*KENTRY->vec_libkern.getenv)
#define atol           (*KENTRY->vec_libkern.atol)
#define strtonumber    (*KENTRY->vec_libkern.strtonumber)
#define strlen         (*KENTRY->vec_libkern.strlen)
#define strcmp         (*KENTRY->vec_libkern.strcmp)
#define strncmp        (*KENTRY->vec_libkern.strncmp)
#define stricmp        (*KENTRY->vec_libkern.stricmp)
#define strnicmp       (*KENTRY->vec_libkern.strnicmp)
#define strcpy         (*KENTRY->vec_libkern.strcpy)
#define strncpy        (*KENTRY->vec_libkern.strncpy)
#define strncpy_f      (*KENTRY->vec_libkern.strncpy_f)
#define strlwr         (*KENTRY->vec_libkern.strlwr)
#define strupr         (*KENTRY->vec_libkern.strupr)
#define strcat         (*KENTRY->vec_libkern.strcat)
#define strchr         (*KENTRY->vec_libkern.strchr)
#define strrchr        (*KENTRY->vec_libkern.strrchr)
#define strrev         (*KENTRY->vec_libkern.strrev)
#define strstr         (*KENTRY->vec_libkern.strstr)
#define strtol         (*KENTRY->vec_libkern.strtol)
#define strtoul        (*KENTRY->vec_libkern.strtoul)
#define memchr         (*KENTRY->vec_libkern.memchr)
#define memcmp         (*KENTRY->vec_libkern.memcmp)

#define ms_time        (*KENTRY->vec_libkern.ms_time)
#define unix2calendar  (*KENTRY->vec_libkern.unix2calendar)
#define unix2xbios     (*KENTRY->vec_libkern.unix2xbios)
#define dostime        (*KENTRY->vec_libkern.dostime)
#define unixtime       (*KENTRY->vec_libkern.unixtime)

#define blockcpy       (*KENTRY->vec_libkern.blockcpy)
#define quickcpy       (*KENTRY->vec_libkern.quickcpy)
#define quickswap      (*KENTRY->vec_libkern.quickswap)
#define quickzero      (*KENTRY->vec_libkern.quickzero)
#define memcpy         (*KENTRY->vec_libkern.memcpy)
#define memset         (*KENTRY->vec_libkern.memset)
#define bcopy          (*KENTRY->vec_libkern.bcopy)
#define bzero          (*KENTRY->vec_libkern.bzero)

/*
 * kentry_xfs
 */
#define xfs_block      (*KENTRY->vec_xfs.block)
#define xfs_deblock    (*KENTRY->vec_xfs.deblock)
#define xfs_root       (*KENTRY->vec_xfs.root)
#define xfs_lookup     (*KENTRY->vec_xfs.lookup)
#define xfs_getdev     (*KENTRY->vec_xfs.getdev)
#define xfs_getxattr   (*KENTRY->vec_xfs.getxattr)
#define xfs_chattr     (*KENTRY->vec_xfs.chattr)
#define xfs_chown      (*KENTRY->vec_xfs.chown)
#define xfs_chmode     (*KENTRY->vec_xfs.chmode)
#define xfs_mkdir      (*KENTRY->vec_xfs.mkdir)
#define xfs_rmdir      (*KENTRY->vec_xfs.rmdir)
#define xfs_creat      (*KENTRY->vec_xfs.creat)
#define xfs_remove     (*KENTRY->vec_xfs.remove)
#define xfs_getname    (*KENTRY->vec_xfs.getname)
#define xfs_rename     (*KENTRY->vec_xfs.rename)
#define xfs_opendir    (*KENTRY->vec_xfs.opendir)
#define xfs_readdir    (*KENTRY->vec_xfs.readdir)
#define xfs_rewinddir  (*KENTRY->vec_xfs.rewinddir)
#define xfs_closedir   (*KENTRY->vec_xfs.closedir)
#define xfs_pathconf   (*KENTRY->vec_xfs.pathconf)
#define xfs_dfree      (*KENTRY->vec_xfs.dfree)
#define xfs_writelabel (*KENTRY->vec_xfs.writelabel)
#define xfs_readlabel  (*KENTRY->vec_xfs.readlabel)
#define xfs_symlink    (*KENTRY->vec_xfs.symlink)
#define xfs_readlink   (*KENTRY->vec_xfs.readlink)
#define xfs_hardlink   (*KENTRY->vec_xfs.hardlink)
#define xfs_fscntl     (*KENTRY->vec_xfs.fscntl)
#define xfs_dskchng    (*KENTRY->vec_xfs.dskchng)
#define xfs_release    (*KENTRY->vec_xfs.release)
#define xfs_dupcookie  (*KENTRY->vec_xfs.dupcookie)
#define xfs_mknod      (*KENTRY->vec_xfs.mknod)
#define xfs_unmount    (*KENTRY->vec_xfs.unmount)
#define xfs_stat64     (*KENTRY->vec_xfs.stat64)

/*
 * kentry_xdd
 */
#define xdd_open       (*KENTRY->vec_xdd.open)
#define xdd_write      (*KENTRY->vec_xdd.write)
#define xdd_read       (*KENTRY->vec_xdd.read)
#define xdd_lseek      (*KENTRY->vec_xdd.lseek)
#define xdd_ioctl      (*KENTRY->vec_xdd.ioctl)
#define xdd_datime     (*KENTRY->vec_xdd.datime)
#define xdd_close      (*KENTRY->vec_xdd.close)

/*
 * kentry_pcibios
 */

#define pcibios_installed       (*KENTRY->vec_pcibios.pcibios_installed)
#define _Find_pci_device        (*KENTRY->vec_pcibios.Find_pci_device)
#define _Find_pci_classcode     (*KENTRY->vec_pcibios.Find_pci_classcode)
#define _Read_config_byte       (*KENTRY->vec_pcibios.Read_config_byte)
#define _Read_config_word       (*KENTRY->vec_pcibios.Read_config_word)
#define _Read_config_longword   (*KENTRY->vec_pcibios.Read_config_longword)
#define _Fast_read_config_byte  (*KENTRY->vec_pcibios.Fast_read_config_byte)
#define _Fast_read_config_word  (*KENTRY->vec_pcibios.Fast_read_config_word)
#define _Fast_read_config_longword  (*KENTRY->vec_pcibios.Fast_read_config_longword)
#define Write_config_byte       (*KENTRY->vec_pcibios.Write_config_byte)
#define Write_config_word       (*KENTRY->vec_pcibios.Write_config_longword)
#define Write_config_longword   (*KENTRY->vec_pcibios.Write_config_longword)
#define Hook_interrupt          (*KENTRY->vec_pcibios.Hook_interrupt)
#define Unhook_interrupt        (*KENTRY->vec_pcibios.Unhook_interrupt)
#define _Special_cycle          (*KENTRY->vec_pcibios.Special_cycle)
#define Get_routing             (*KENTRY->vec_pcibios.Get_routing)
#define Set_interrupt           (*KENTRY->vec_pcibios.Set_interrupt)
#define Get_resource            (*KENTRY->vec_pcibios.Get_resource)
#define Get_card_used           (*KENTRY->vec_pcibios.Get_card_used)
#define Set_card_used           (*KENTRY->vec_pcibios.Set_card_used)
#define Read_mem_byte           (*KENTRY->vec_pcibios.Read_mem_byte)
#define Read_mem_word           (*KENTRY->vec_pcibios.Read_mem_word)
#define Read_mem_longword       (*KENTRY->vec_pcibios.Read_mem_longword)
#define Fast_read_mem_byte      (*KENTRY->vec_pcibios.Fast_read_mem_byte)
#define Fast_read_mem_word      (*KENTRY->vec_pcibios.Fast_read_mem_word)
#define Fast_read_mem_longword  (*KENTRY->vec_pcibios.Fast_read_mem_longword)
#define _Write_mem_byte         (*KENTRY->vec_pcibios.Write_mem_byte)
#define _Write_mem_word         (*KENTRY->vec_pcibios.Write_mem_word)
#define Write_mem_longword      (*KENTRY->vec_pcibios.Write_mem_longword)
#define Read_io_byte            (*KENTRY->vec_pcibios.Read_io_byte)
#define Read_io_word            (*KENTRY->vec_pcibios.Read_io_word)
#define Read_io_longword        (*KENTRY->vec_pcibios.Read_io_longword)
#define Fast_read_io_byte       (*KENTRY->vec_pcibios.Fast_read_io_byte)
#define Fast_read_io_word       (*KENTRY->vec_pcibios.Fast_read_io_word)
#define Fast_read_io_longword   (*KENTRY->vec_pcibios.Fast_read_io_longword)
#define _Write_io_byte          (*KENTRY->vec_pcibios.Write_io_byte)
#define _Write_io_word          (*KENTRY->vec_pcibios.Write_io_word)
#define Write_io_longword       (*KENTRY->vec_pcibios.Write_io_longword)
#define Get_machine_id          (*KENTRY->vec_pcibios.Get_machine_id)
#define Get_pagesize            (*KENTRY->vec_pcibios.Get_pagesize)
#define Virt_to_bus             (*KENTRY->vec_pcibios.Virt_to_bus)
#define Bus_to_virt             (*KENTRY->vec_pcibios.Bus_to_virt)
#define Virt_to_phys            (*KENTRY->vec_pcibios.Virt_to_phys)
#define Phys_to_virt            (*KENTRY->vec_pcibios.Phys_to_virt)

typedef long (*wrap0)(void);
typedef long (*wrap1)(long);
typedef long (*wrap2)(long, long);
typedef long (*wrap3)(long, long, long);

INLINE long Find_pci_device(unsigned long id, unsigned short index)
{ wrap2 f = (wrap2) _Find_pci_device; return (*f)(id, index); }

INLINE long Find_pci_classcode(unsigned long class, unsigned short index)
{ wrap2 f = (wrap2) _Find_pci_classcode; return (*f)(class, index); }

INLINE long Read_config_byte(long handle, unsigned short reg, unsigned char *address)
{ wrap3 f = (wrap3)_Read_config_byte; return (*f)(handle, reg, (long)address); }

INLINE long Read_config_word(long handle, unsigned short reg, unsigned short *address)
{ wrap3 f = (wrap3)_Read_config_word; return (*f)(handle, reg, (long)address); }

INLINE long Read_config_longword(long handle, unsigned short reg, unsigned long *address)
{ wrap3 f = (wrap3)_Read_config_longword; return (*f)(handle, reg, (long)address); }

INLINE unsigned char Fast_read_config_byte(long handle, unsigned short reg)
{ wrap2 f = (wrap2) _Fast_read_config_byte; return (*f)(handle, reg); }

INLINE unsigned short Fast_read_config_word(long handle, unsigned short reg)
{ wrap2 f = (wrap2) _Fast_read_config_word; return (*f)(handle, reg); }

INLINE unsigned long Fast_read_config_longword(long handle, unsigned short reg)
{ wrap2 f = (wrap2) _Fast_read_config_longword; return (*f)(handle, reg); }

INLINE long Special_cycle(unsigned short bus, unsigned long data)
{ wrap2 f = (wrap2) _Special_cycle; return (*f)(bus, data); }

INLINE long Write_mem_byte(long handle, unsigned long offset, unsigned short val)
{ wrap3 f = (wrap3)_Write_mem_byte; return (*f)(handle, offset, val); }

INLINE long Write_mem_word(long handle, unsigned long offset, unsigned short val)
{ wrap3 f = (wrap3)_Write_mem_word; return (*f)(handle, offset, val); }

INLINE long Write_io_byte(long handle, unsigned long offset, unsigned short val)
{ wrap3 f = (wrap3)_Write_io_byte; return (*f)(handle, offset, val); }

INLINE long Write_io_word(long handle, unsigned long offset, unsigned short val)
{ wrap3 f = (wrap3)_Write_io_word; return (*f)(handle, offset, val); }


/*
 * kentry_xhdi
 */

#define xhgetversion       (*KENTRY->vec_xhdi.XHGetVersion)
#define xhinqtarget        (*KENTRY->vec_xhdi.XHInqTarget)
#define xhreserve          (*KENTRY->vec_xhdi.XHReserve)
#define xhlock             (*KENTRY->vec_xhdi.XHLock)
#define xhstop             (*KENTRY->vec_xhdi.XHStop)
#define xheject            (*KENTRY->vec_xhdi.XHEject)
#define xhdrvmap           (*KENTRY->vec_xhdi.XHDrvMap)
#define xhinqdev           (*KENTRY->vec_xhdi.XHInqDev)
#define xhinqdriver        (*KENTRY->vec_xhdi.XHInqDriver)
#define xhnewcookie        (*KENTRY->vec_xhdi.XHNewCookie)
#define xhreadwrite        (*KENTRY->vec_xhdi.XHReadWrite)
#define xhinqtarget        (*KENTRY->vec_xhdi.XHInqTarget)
#define xhinqdev2          (*KENTRY->vec_xhdi.XHInqDev2)
#define xhdriverspecial    (*KENTRY->vec_xhdi.XHDriverSpecial)
#define xhgetcapacity      (*KENTRY->vec_xhdi.XHGetCapacity)
#define xhmediumchaged     (*KENTRY->vec_xhdi.XHMediumChanged)
#define xhmintinfo         (*KENTRY->vec_xhdi.XHMiNTInfo)
#define xhdoslimits        (*KENTRY->vec_xhdi.XHDOSLimits)
#define xhlastaccess       (*KENTRY->vec_xhdi.XHLastAccess)
#define xhreaccess         (*KENTRY->vec_xhdi.XHReaccess)


/*
 * kentry_scsidrv
 */

#define scsidrv_In                 (*KENTRY->vec_scsidrv.scsidrv_In)
#define scsidrv_Out                (*KENTRY->vec_scsidrv.scsidrv_Out)
#define scsidrv_InquireSCSI        (*KENTRY->vec_scsidrv.scsidrv_InquireSCSI)
#define scsidrv_InquireBus         (*KENTRY->vec_scsidrv.scsidrv_InquireBus)
#define scsidrv_CheckDev           (*KENTRY->vec_scsidrv.scsidrv_CheckDev)
#define scsidrv_RescanBus          (*KENTRY->vec_scsidrv.scsidrv_RescanBus)
#define scsidrv_Open               (*KENTRY->vec_scsidrv.scsidrv_Open)
#define scsidrv_Close              (*KENTRY->vec_scsidrv.scsidrv_Close)
#define scsidrv_Error              (*KENTRY->vec_scsidrv.scsidrv_Error)
#define scsidrv_Install            (*KENTRY->vec_scsidrv.scsidrv_Install)
#define scsidrv_Deinstall          (*KENTRY->vec_scsidrv.scsidrv_Deinstall)
#define scsidrv_GetCmd             (*KENTRY->vec_scsidrv.scsidrv_GetCmd)
#define scsidrv_SendData           (*KENTRY->vec_scsidrv.scsidrv_SendData)
#define scsidrv_GetData            (*KENTRY->vec_scsidrv.scsidrv_GetData)
#define scsidrv_SendStatus         (*KENTRY->vec_scsidrv.scsidrv_SendStatus)
#define scsidrv_SendMsg            (*KENTRY->vec_scsidrv.scsidrv_SendMsg)
#define scsidrv_GetMsg             (*KENTRY->vec_scsidrv.scsidrv_GetMsg)
#define scsidrv_InstallNewDriver   (*KENTRY->vec_scsidrv.scsidrv_InstallNewDriver)

#endif /* _libkern_kernel_module_h */
