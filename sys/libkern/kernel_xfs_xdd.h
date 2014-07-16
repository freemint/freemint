/*
 * $Id$
 * 
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

# ifndef _libkern_kernel_xfs_xdd_h
# define _libkern_kernel_xfs_xdd_h

# include "mint/kerinfo.h"


/* Macros for kernel, bios and gemdos functions
 */

# ifndef KERNEL
# define KERNEL	kernel
# endif

extern struct kerinfo *KERNEL;

# define MINT_MAJOR		(KERNEL->maj_version)
# define MINT_MINOR		(KERNEL->min_version)
# define MINT_KVERSION		(KERNEL->version)
# define DEFAULT_MODE		(KERNEL->default_perm)
# define DEFAULT_DMODE		(0755)

/*
 * dos_tab
 */

# define _p_term0		(*KERNEL->dos_tab[0x000])
# define _c_conin		(*KERNEL->dos_tab[0x001])
# define _c_conout		(*KERNEL->dos_tab[0x002])
# define _c_auxin		(*KERNEL->dos_tab[0x003])
# define _c_auxout		(*KERNEL->dos_tab[0x004])
# define _c_prnout		(*KERNEL->dos_tab[0x005])
# define _c_rawio		(*KERNEL->dos_tab[0x006])
# define _c_rawcin		(*KERNEL->dos_tab[0x007])
# define _c_necin		(*KERNEL->dos_tab[0x008])
# define _c_conws		(*KERNEL->dos_tab[0x009])
# define _c_conrs		(*KERNEL->dos_tab[0x00a])
# define _c_conis		(*KERNEL->dos_tab[0x00b])
# define _0x00c			(*KERNEL->dos_tab[0x00c])
# define _0x00d			(*KERNEL->dos_tab[0x00d])
# define _d_setdrv		(*KERNEL->dos_tab[0x00e])
# define _0x00f			(*KERNEL->dos_tab[0x00f])

# define _c_conos		(*KERNEL->dos_tab[0x010])
# define _c_prnos		(*KERNEL->dos_tab[0x011])
# define _c_auxis		(*KERNEL->dos_tab[0x012])
# define _c_auxos		(*KERNEL->dos_tab[0x013])
# define _m_addalt		(*KERNEL->dos_tab[0x014])
# define _s_realloc		(*KERNEL->dos_tab[0x015])
# define _s_lbopen		(*KERNEL->dos_tab[0x016])	/* 1.15.3 MagiC: slb */
# define _s_lbclose		(*KERNEL->dos_tab[0x017])	/* 1.15.3 MagiC: slb */
# define _0x018			(*KERNEL->dos_tab[0x018])
# define _d_getdrv		(*KERNEL->dos_tab[0x019])
# define _f_setdta		(*KERNEL->dos_tab[0x01a])
# define _0x01b			(*KERNEL->dos_tab[0x01b])
# define _0x01c			(*KERNEL->dos_tab[0x01c])
# define _0x01d			(*KERNEL->dos_tab[0x01d])
# define _0x01e			(*KERNEL->dos_tab[0x01e])
# define _0x01f			(*KERNEL->dos_tab[0x01f])

# define _s_uper		(*KERNEL->dos_tab[0x020])
# define _0x021			(*KERNEL->dos_tab[0x021])
# define _0x022			(*KERNEL->dos_tab[0x022])
# define _0x023			(*KERNEL->dos_tab[0x023])
# define _0x024			(*KERNEL->dos_tab[0x024])
# define _0x025			(*KERNEL->dos_tab[0x025])
# define _0x026			(*KERNEL->dos_tab[0x026])
# define _0x027			(*KERNEL->dos_tab[0x027])
# define _0x028			(*KERNEL->dos_tab[0x028])
# define _0x029			(*KERNEL->dos_tab[0x029])
# define _t_getdate		(*KERNEL->dos_tab[0x02a])
# define _t_setdate		(*KERNEL->dos_tab[0x02b])
# define _t_gettime		(*KERNEL->dos_tab[0x02c])
# define _t_settime		(*KERNEL->dos_tab[0x02d])
# define _0x02e			(*KERNEL->dos_tab[0x02e])
# define _f_getdta		(*KERNEL->dos_tab[0x02f])

# define _s_version		(*KERNEL->dos_tab[0x030])
# define _p_termres		(*KERNEL->dos_tab[0x031])
# define _0x032			(*KERNEL->dos_tab[0x032])
# define _0x033			(*KERNEL->dos_tab[0x033])	/* MagiC: LONG Sconfig(WORD subfn, LONG flags); */
# define _0x034			(*KERNEL->dos_tab[0x034])
# define _0x035			(*KERNEL->dos_tab[0x035])
# define _d_free		(*KERNEL->dos_tab[0x036])
# define _0x037			(*KERNEL->dos_tab[0x037])
# define _0x038			(*KERNEL->dos_tab[0x038])
# define _d_create		(*KERNEL->dos_tab[0x039])
# define _d_delete		(*KERNEL->dos_tab[0x03a])
# define _d_setpath		(*KERNEL->dos_tab[0x03b])
# define _f_create		(*KERNEL->dos_tab[0x03c])
# define _f_open		(*KERNEL->dos_tab[0x03d])
# define _f_close		(*KERNEL->dos_tab[0x03e])
# define _f_read		(*KERNEL->dos_tab[0x03f])

# define _f_write		(*KERNEL->dos_tab[0x040])
# define _f_delete		(*KERNEL->dos_tab[0x041])
# define _f_seek		(*KERNEL->dos_tab[0x042])
# define _f_attrib		(*KERNEL->dos_tab[0x043])
# define _m_xalloc		(*KERNEL->dos_tab[0x044])
# define _f_dup			(*KERNEL->dos_tab[0x045])
# define _f_force		(*KERNEL->dos_tab[0x046])
# define _d_getpath		(*KERNEL->dos_tab[0x047])
# define _m_alloc		(*KERNEL->dos_tab[0x048])
# define _m_free		(*KERNEL->dos_tab[0x049])
# define _m_shrink		(*KERNEL->dos_tab[0x04a])
# define _p_exec		(*KERNEL->dos_tab[0x04b])
# define _p_term		(*KERNEL->dos_tab[0x04c])
# define _0x04d			(*KERNEL->dos_tab[0x04d])
# define _f_sfirst		(*KERNEL->dos_tab[0x04e])
# define _f_snext		(*KERNEL->dos_tab[0x04f])

# define _0x050			(*KERNEL->dos_tab[0x050])
# define _0x051			(*KERNEL->dos_tab[0x051])
# define _0x052			(*KERNEL->dos_tab[0x052])
# define _0x053			(*KERNEL->dos_tab[0x053])
# define _0x054			(*KERNEL->dos_tab[0x054])
# define _0x055			(*KERNEL->dos_tab[0x055])
# define _f_rename		(*KERNEL->dos_tab[0x056])
# define _f_datime		(*KERNEL->dos_tab[0x057])
# define _0x058			(*KERNEL->dos_tab[0x058])
# define _0x059			(*KERNEL->dos_tab[0x059])
# define _0x05a			(*KERNEL->dos_tab[0x05a])
# define _0x05b			(*KERNEL->dos_tab[0x05b])
# define _f_lock		(*KERNEL->dos_tab[0x05c])
# define _0x05d			(*KERNEL->dos_tab[0x05d])
# define _0x05e			(*KERNEL->dos_tab[0x05e])
# define _0x05f			(*KERNEL->dos_tab[0x05f])

# define _0x060			(*KERNEL->dos_tab[0x060])
# define _0x061			(*KERNEL->dos_tab[0x061])
# define _0x062			(*KERNEL->dos_tab[0x062])
# define _0x063			(*KERNEL->dos_tab[0x063])
# define _0x064			(*KERNEL->dos_tab[0x064])
# define _0x065			(*KERNEL->dos_tab[0x065])
# define _0x066			(*KERNEL->dos_tab[0x066])
# define _0x067			(*KERNEL->dos_tab[0x067])
# define _0x068			(*KERNEL->dos_tab[0x068])
# define _0x069			(*KERNEL->dos_tab[0x069])
# define _0x06a			(*KERNEL->dos_tab[0x06a])
# define _0x06b			(*KERNEL->dos_tab[0x06b])
# define _0x06c			(*KERNEL->dos_tab[0x06c])
# define _0x06d			(*KERNEL->dos_tab[0x06d])
# define _0x06e			(*KERNEL->dos_tab[0x06e])
# define _0x06f			(*KERNEL->dos_tab[0x06f])

# define _0x070			(*KERNEL->dos_tab[0x070])
# define _0x071			(*KERNEL->dos_tab[0x071])
# define _0x072			(*KERNEL->dos_tab[0x072])
# define _0x073			(*KERNEL->dos_tab[0x073])
# define _0x074			(*KERNEL->dos_tab[0x074])
# define _0x075			(*KERNEL->dos_tab[0x075])
# define _0x076			(*KERNEL->dos_tab[0x076])
# define _0x077			(*KERNEL->dos_tab[0x077])
# define _0x078			(*KERNEL->dos_tab[0x078])
# define _0x079			(*KERNEL->dos_tab[0x079])
# define _0x07a			(*KERNEL->dos_tab[0x07a])
# define _0x07b			(*KERNEL->dos_tab[0x07b])
# define _0x07c			(*KERNEL->dos_tab[0x07c])
# define _0x07d			(*KERNEL->dos_tab[0x07d])
# define _0x07e			(*KERNEL->dos_tab[0x07e])
# define _0x07f			(*KERNEL->dos_tab[0x07f])

# define _0x080			(*KERNEL->dos_tab[0x080])
# define _0x081			(*KERNEL->dos_tab[0x081])
# define _0x082			(*KERNEL->dos_tab[0x082])
# define _0x083			(*KERNEL->dos_tab[0x083])
# define _0x084			(*KERNEL->dos_tab[0x084])
# define _0x085			(*KERNEL->dos_tab[0x085])
# define _0x086			(*KERNEL->dos_tab[0x086])
# define _0x087			(*KERNEL->dos_tab[0x087])
# define _0x088			(*KERNEL->dos_tab[0x088])
# define _0x089			(*KERNEL->dos_tab[0x089])
# define _0x08a			(*KERNEL->dos_tab[0x08a])
# define _0x08b			(*KERNEL->dos_tab[0x08b])
# define _0x08c			(*KERNEL->dos_tab[0x08c])
# define _0x08d			(*KERNEL->dos_tab[0x08d])
# define _0x08e			(*KERNEL->dos_tab[0x08e])
# define _0x08f			(*KERNEL->dos_tab[0x08f])

# define _0x090			(*KERNEL->dos_tab[0x090])
# define _0x091			(*KERNEL->dos_tab[0x091])
# define _0x092			(*KERNEL->dos_tab[0x092])
# define _0x093			(*KERNEL->dos_tab[0x093])
# define _0x094			(*KERNEL->dos_tab[0x094])
# define _0x095			(*KERNEL->dos_tab[0x095])
# define _0x096			(*KERNEL->dos_tab[0x096])
# define _0x097			(*KERNEL->dos_tab[0x097])
# define _0x098			(*KERNEL->dos_tab[0x098])
# define _0x099			(*KERNEL->dos_tab[0x099])
# define _0x09a			(*KERNEL->dos_tab[0x09a])
# define _0x09b			(*KERNEL->dos_tab[0x09b])
# define _0x09c			(*KERNEL->dos_tab[0x09c])
# define _0x09d			(*KERNEL->dos_tab[0x09d])
# define _0x09e			(*KERNEL->dos_tab[0x09e])
# define _0x09f			(*KERNEL->dos_tab[0x09f])

# define _0x0a0			(*KERNEL->dos_tab[0x0a0])
# define _0x0a1			(*KERNEL->dos_tab[0x0a1])
# define _0x0a2			(*KERNEL->dos_tab[0x0a2])
# define _0x0a3			(*KERNEL->dos_tab[0x0a3])
# define _0x0a4			(*KERNEL->dos_tab[0x0a4])
# define _0x0a5			(*KERNEL->dos_tab[0x0a5])
# define _0x0a6			(*KERNEL->dos_tab[0x0a6])
# define _0x0a7			(*KERNEL->dos_tab[0x0a7])
# define _0x0a8			(*KERNEL->dos_tab[0x0a8])
# define _0x0a9			(*KERNEL->dos_tab[0x0a9])
# define _0x0aa			(*KERNEL->dos_tab[0x0aa])
# define _0x0ab			(*KERNEL->dos_tab[0x0ab])
# define _0x0ac			(*KERNEL->dos_tab[0x0ac])
# define _0x0ad			(*KERNEL->dos_tab[0x0ad])
# define _0x0ae			(*KERNEL->dos_tab[0x0ae])
# define _0x0af			(*KERNEL->dos_tab[0x0af])

# define _0x0b0			(*KERNEL->dos_tab[0x0b0])
# define _0x0b1			(*KERNEL->dos_tab[0x0b1])
# define _0x0b2			(*KERNEL->dos_tab[0x0b2])
# define _0x0b3			(*KERNEL->dos_tab[0x0b3])
# define _0x0b4			(*KERNEL->dos_tab[0x0b4])
# define _0x0b5			(*KERNEL->dos_tab[0x0b5])
# define _0x0b6			(*KERNEL->dos_tab[0x0b6])
# define _0x0b7			(*KERNEL->dos_tab[0x0b7])
# define _0x0b8			(*KERNEL->dos_tab[0x0b8])
# define _0x0b9			(*KERNEL->dos_tab[0x0b9])
# define _0x0ba			(*KERNEL->dos_tab[0x0ba])
# define _0x0bb			(*KERNEL->dos_tab[0x0bb])
# define _0x0bc			(*KERNEL->dos_tab[0x0bc])
# define _0x0bd			(*KERNEL->dos_tab[0x0bd])
# define _0x0be			(*KERNEL->dos_tab[0x0be])
# define _0x0bf			(*KERNEL->dos_tab[0x0bf])

# define _0x0c0			(*KERNEL->dos_tab[0x0c0])
# define _0x0c1			(*KERNEL->dos_tab[0x0c1])
# define _0x0c2			(*KERNEL->dos_tab[0x0c2])
# define _0x0c3			(*KERNEL->dos_tab[0x0c3])
# define _0x0c4			(*KERNEL->dos_tab[0x0c4])
# define _0x0c5			(*KERNEL->dos_tab[0x0c5])
# define _0x0c6			(*KERNEL->dos_tab[0x0c6])
# define _0x0c7			(*KERNEL->dos_tab[0x0c7])
# define _0x0c8			(*KERNEL->dos_tab[0x0c8])
# define _0x0c9			(*KERNEL->dos_tab[0x0c9])
# define _0x0ca			(*KERNEL->dos_tab[0x0ca])
# define _0x0cb			(*KERNEL->dos_tab[0x0cb])
# define _0x0cc			(*KERNEL->dos_tab[0x0cc])
# define _0x0cd			(*KERNEL->dos_tab[0x0cd])
# define _0x0ce			(*KERNEL->dos_tab[0x0ce])
# define _0x0cf			(*KERNEL->dos_tab[0x0cf])

# define _0x0d0			(*KERNEL->dos_tab[0x0d0])
# define _0x0d1			(*KERNEL->dos_tab[0x0d1])
# define _0x0d2			(*KERNEL->dos_tab[0x0d2])
# define _0x0d3			(*KERNEL->dos_tab[0x0d3])
# define _0x0d4			(*KERNEL->dos_tab[0x0d4])
# define _0x0d5			(*KERNEL->dos_tab[0x0d5])
# define _0x0d6			(*KERNEL->dos_tab[0x0d6])
# define _0x0d7			(*KERNEL->dos_tab[0x0d7])
# define _0x0d8			(*KERNEL->dos_tab[0x0d8])
# define _0x0d9			(*KERNEL->dos_tab[0x0d9])
# define _0x0da			(*KERNEL->dos_tab[0x0da])
# define _0x0db			(*KERNEL->dos_tab[0x0db])
# define _0x0dc			(*KERNEL->dos_tab[0x0dc])
# define _0x0dd			(*KERNEL->dos_tab[0x0dd])
# define _0x0de			(*KERNEL->dos_tab[0x0de])
# define _0x0df			(*KERNEL->dos_tab[0x0df])

# define _0x0e0			(*KERNEL->dos_tab[0x0e0])
# define _0x0e1			(*KERNEL->dos_tab[0x0e1])
# define _0x0e2			(*KERNEL->dos_tab[0x0e2])
# define _0x0e3			(*KERNEL->dos_tab[0x0e3])
# define _0x0e4			(*KERNEL->dos_tab[0x0e4])
# define _0x0e5			(*KERNEL->dos_tab[0x0e5])
# define _0x0e6			(*KERNEL->dos_tab[0x0e6])
# define _0x0e7			(*KERNEL->dos_tab[0x0e7])
# define _0x0e8			(*KERNEL->dos_tab[0x0e8])
# define _0x0e9			(*KERNEL->dos_tab[0x0e9])
# define _0x0ea			(*KERNEL->dos_tab[0x0ea])
# define _0x0eb			(*KERNEL->dos_tab[0x0eb])
# define _0x0ec			(*KERNEL->dos_tab[0x0ec])
# define _0x0ed			(*KERNEL->dos_tab[0x0ed])
# define _0x0ee			(*KERNEL->dos_tab[0x0ee])
# define _0x0ef			(*KERNEL->dos_tab[0x0ef])

# define _0x0f0			(*KERNEL->dos_tab[0x0f0])
# define _0x0f1			(*KERNEL->dos_tab[0x0f1])
# define _0x0f2			(*KERNEL->dos_tab[0x0f2])
# define _0x0f3			(*KERNEL->dos_tab[0x0f3])
# define _0x0f4			(*KERNEL->dos_tab[0x0f4])
# define _0x0f5			(*KERNEL->dos_tab[0x0f5])
# define _0x0f6			(*KERNEL->dos_tab[0x0f6])
# define _0x0f7			(*KERNEL->dos_tab[0x0f7])
# define _0x0f8			(*KERNEL->dos_tab[0x0f8])
# define _0x0f9			(*KERNEL->dos_tab[0x0f9])
# define _0x0fa			(*KERNEL->dos_tab[0x0fa])
# define _0x0fb			(*KERNEL->dos_tab[0x0fb])
# define _0x0fc			(*KERNEL->dos_tab[0x0fc])
# define _0x0fd			(*KERNEL->dos_tab[0x0fd])
# define _0x0fe			(*KERNEL->dos_tab[0x0fe])

/*
 * MiNT extensions to GEMDOS
 */

# define _s_yield		(*KERNEL->dos_tab[0x0ff])

# define _f_pipe		(*KERNEL->dos_tab[0x100])
# define _f_fchown		(*KERNEL->dos_tab[0x101])	/* 1.15.2 */
# define _f_fchmod		(*KERNEL->dos_tab[0x102])	/* 1.15.2 */
# define _0x103			(*KERNEL->dos_tab[0x103]) 	/* f_sync */
# define _f_cntl		(*KERNEL->dos_tab[0x104])
# define _f_instat		(*KERNEL->dos_tab[0x105])
# define _f_outstat		(*KERNEL->dos_tab[0x106])
# define _f_getchar		(*KERNEL->dos_tab[0x107])
# define _f_putchar		(*KERNEL->dos_tab[0x108])
# define _p_wait		(*KERNEL->dos_tab[0x109])
# define _p_nice		(*KERNEL->dos_tab[0x10a])
# define _p_getpid		(*KERNEL->dos_tab[0x10b])
# define _p_getppid		(*KERNEL->dos_tab[0x10c])
# define _p_getpgrp		(*KERNEL->dos_tab[0x10d])
# define _p_setpgrp		(*KERNEL->dos_tab[0x10e])
# define _p_getuid		(*KERNEL->dos_tab[0x10f])

# define _p_setuid		(*KERNEL->dos_tab[0x110])
# define _p_kill		(*KERNEL->dos_tab[0x111])
# define _p_signal		(*KERNEL->dos_tab[0x112])
# define _p_vfork		(*KERNEL->dos_tab[0x113])
# define _p_getgid		(*KERNEL->dos_tab[0x114])
# define _p_setgid		(*KERNEL->dos_tab[0x115])
# define _p_sigblock		(*KERNEL->dos_tab[0x116])
# define _p_sigsetmask		(*KERNEL->dos_tab[0x117])
# define _p_usrval		(*KERNEL->dos_tab[0x118])
# define _p_domain		(*KERNEL->dos_tab[0x119])
# define _p_sigreturn		(*KERNEL->dos_tab[0x11a])
# define _p_fork		(*KERNEL->dos_tab[0x11b])
# define _p_wait3		(*KERNEL->dos_tab[0x11c])
# define _f_select		(*KERNEL->dos_tab[0x11d])
# define _p_rusage		(*KERNEL->dos_tab[0x11e])
# define _p_setlimit		(*KERNEL->dos_tab[0x11f])

# define _t_alarm		(*KERNEL->dos_tab[0x120])
# define _p_pause		(*KERNEL->dos_tab[0x121])
# define _s_ysconf		(*KERNEL->dos_tab[0x122])
# define _p_sigpending		(*KERNEL->dos_tab[0x123])
# define _d_pathconf		(*KERNEL->dos_tab[0x124])
# define _p_msg			(*KERNEL->dos_tab[0x125])
# define _f_midipipe		(*KERNEL->dos_tab[0x126])
# define _p_renice		(*KERNEL->dos_tab[0x127])
# define _d_opendir		(*KERNEL->dos_tab[0x128])
# define _d_readdir		(*KERNEL->dos_tab[0x129])
# define _d_rewind		(*KERNEL->dos_tab[0x12a])
# define _d_closedir		(*KERNEL->dos_tab[0x12b])
# define _f_xattr		(*KERNEL->dos_tab[0x12c])
# define _f_link		(*KERNEL->dos_tab[0x12d])
# define _f_symlink		(*KERNEL->dos_tab[0x12e])
# define _f_readlink		(*KERNEL->dos_tab[0x12f])

# define _d_cntl		(*KERNEL->dos_tab[0x130])
# define _f_chown		(*KERNEL->dos_tab[0x131])
# define _f_chmod		(*KERNEL->dos_tab[0x132])
# define _p_umask		(*KERNEL->dos_tab[0x133])
# define _p_semaphore		(*KERNEL->dos_tab[0x134])
# define _d_lock		(*KERNEL->dos_tab[0x135])
# define _p_sigpause		(*KERNEL->dos_tab[0x136])
# define _p_sigaction		(*KERNEL->dos_tab[0x137])
# define _p_geteuid		(*KERNEL->dos_tab[0x138])
# define _p_getegid		(*KERNEL->dos_tab[0x139])
# define _p_waitpid		(*KERNEL->dos_tab[0x13a])
# define _d_getcwd		(*KERNEL->dos_tab[0x13b])
# define _s_alert		(*KERNEL->dos_tab[0x13c])
# define _t_malarm		(*KERNEL->dos_tab[0x13d])
# define _p_sigintr		(*KERNEL->dos_tab[0x13e])
# define _s_uptime		(*KERNEL->dos_tab[0x13f])

# define _0x140			(*KERNEL->dos_tab[0x140])	/* s_trapatch */
# define _0x141			(*KERNEL->dos_tab[0x141])
# define _d_xreaddir		(*KERNEL->dos_tab[0x142])
# define _p_seteuid		(*KERNEL->dos_tab[0x143])
# define _p_setegid		(*KERNEL->dos_tab[0x144])
# define _p_getauid		(*KERNEL->dos_tab[0x145])
# define _p_setauid		(*KERNEL->dos_tab[0x146])
# define _p_getgroups		(*KERNEL->dos_tab[0x147])
# define _p_setgroups		(*KERNEL->dos_tab[0x148])
# define _t_setitimer		(*KERNEL->dos_tab[0x149])
# define _d_chroot		(*KERNEL->dos_tab[0x14a])	/* 1.15.3 */
# define _f_stat64		(*KERNEL->dos_tab[0x14b])	/* f_stat */
# define _f_seek64		(*KERNEL->dos_tab[0x14c])
# define _d_setkey		(*KERNEL->dos_tab[0x14d])
# define _p_setreuid		(*KERNEL->dos_tab[0x14e])
# define _p_setregid		(*KERNEL->dos_tab[0x14f])

# define _s_ync			(*KERNEL->dos_tab[0x150])
# define _s_hutdown		(*KERNEL->dos_tab[0x151])
# define _d_readlabel		(*KERNEL->dos_tab[0x152])
# define _d_writelabel		(*KERNEL->dos_tab[0x153])
# define _s_system		(*KERNEL->dos_tab[0x154])
# define _t_gettimeofday	(*KERNEL->dos_tab[0x155])
# define _t_settimeofday	(*KERNEL->dos_tab[0x156])
# define _0x157			(*KERNEL->dos_tab[0x157])	/* t_adjtime */
# define _p_getpriority		(*KERNEL->dos_tab[0x158])
# define _p_setpriority		(*KERNEL->dos_tab[0x159])
# define _f_poll		(*KERNEL->dos_tab[0x15a])
# define _f_writev		(*KERNEL->dos_tab[0x15b])
# define _f_readv		(*KERNEL->dos_tab[0x15c])
# define _f_fstat		(*KERNEL->dos_tab[0x15d])
# define _p_sysctl		(*KERNEL->dos_tab[0x15e])
# define _s_emulation		(*KERNEL->dos_tab[0x15f])

# define _f_socket		(*KERNEL->dos_tab[0x160])
# define _f_socketpair		(*KERNEL->dos_tab[0x161])
# define _f_accept		(*KERNEL->dos_tab[0x162])
# define _f_connect		(*KERNEL->dos_tab[0x163])
# define _f_bind		(*KERNEL->dos_tab[0x164])
# define _f_listen		(*KERNEL->dos_tab[0x165])
# define _f_recvmsg		(*KERNEL->dos_tab[0x166])
# define _f_sendmsg		(*KERNEL->dos_tab[0x167])
# define _f_recvfrom		(*KERNEL->dos_tab[0x168])
# define _f_sendto		(*KERNEL->dos_tab[0x169])
# define _f_setsockopt		(*KERNEL->dos_tab[0x16a])
# define _f_getsockopt		(*KERNEL->dos_tab[0x16b])
# define _f_getpeername		(*KERNEL->dos_tab[0x16c])
# define _f_getsockname		(*KERNEL->dos_tab[0x16d])
# define _f_shutdown		(*KERNEL->dos_tab[0x16e])
# define _0x16f			(*KERNEL->dos_tab[0x16f])

# define _p_shmget		(*KERNEL->dos_tab[0x170])
# define _p_shmctl		(*KERNEL->dos_tab[0x171])
# define _p_shmat		(*KERNEL->dos_tab[0x172])
# define _p_shmdt		(*KERNEL->dos_tab[0x173])
# define _p_semget		(*KERNEL->dos_tab[0x174])
# define _p_semctl		(*KERNEL->dos_tab[0x175])
# define _p_semop		(*KERNEL->dos_tab[0x176])
# define _p_semconfig		(*KERNEL->dos_tab[0x177])
# define _p_msgget		(*KERNEL->dos_tab[0x178])
# define _p_msgctl		(*KERNEL->dos_tab[0x179])
# define _p_msgsnd		(*KERNEL->dos_tab[0x17a])
# define _p_msgrcv		(*KERNEL->dos_tab[0x17b])
# define _0x17c			(*KERNEL->dos_tab[0x17c])
# define _m_access		(*KERNEL->dos_tab[0x17d])
# define _0x17e			(*KERNEL->dos_tab[0x17e])
# define _0x17f			(*KERNEL->dos_tab[0x17f])

# define _f_chown16		(*KENTRY->dos_tab[0x180])
# define _f_chdir		(*KENTRY->dos_tab[0x181])
# define _f_opendir		(*KENTRY->dos_tab[0x182])
# define _f_dirfd		(*KENTRY->dos_tab[0x183])
# define _0x184			(*KERNEL->dos_tab[0x184])
# define _0x185			(*KERNEL->dos_tab[0x185])
# define _0x186			(*KERNEL->dos_tab[0x186])
# define _0x187			(*KERNEL->dos_tab[0x187])
# define _0x188			(*KERNEL->dos_tab[0x188])
# define _0x189			(*KERNEL->dos_tab[0x189])
# define _0x18a			(*KERNEL->dos_tab[0x18a])
# define _0x18b			(*KERNEL->dos_tab[0x18b])
# define _0x18c			(*KERNEL->dos_tab[0x18c])
# define _0x18d			(*KERNEL->dos_tab[0x18d])
# define _0x18e			(*KERNEL->dos_tab[0x18e])
# define _0x18f			(*KERNEL->dos_tab[0x18f])

	/* 0x190 */		/* DOS_MAX */

INLINE long c_conws(const char *str)
{ return ((long _cdecl (*)(const char *)) _c_conws)(str); }

INLINE long c_conout(const int c)
{ return ((long _cdecl (*)(const int)) _c_conout)(c); }

INLINE long f_setdta(DTABUF *dta)
{ return ((long _cdecl (*)(DTABUF *)) _f_setdta)(dta); }

INLINE long t_getdate(void)
{ return ((long _cdecl (*)(void)) _t_getdate)(); }

INLINE long _cdecl t_gettime(void)
{ return ((long _cdecl (*)(void)) _t_gettime)(); }

INLINE long f_getdta(void)
{ return ((long _cdecl (*)(void)) _f_getdta)(); }

INLINE long d_setpath(const char *path)
{ return ((long _cdecl (*)(const char *)) _d_setpath)(path); }

INLINE long f_create(const char *name, int attrib)
{ return ((long _cdecl (*)(const char *, int)) _f_create)(name, attrib); }

INLINE long f_open(const char *name, int mode)
{ return ((long _cdecl (*)(const char *, int)) _f_open)(name, mode); }

INLINE long f_close(int fh)
{ return ((long _cdecl (*)(int)) _f_close)(fh); }

INLINE long f_read(int fh, long count, const char *buf)
{ return ((long _cdecl (*)(int, long, const char *)) _f_read)(fh, count, buf); }

INLINE long f_write(int fh, long count, char *buf)
{ return ((long _cdecl (*)(int, long, char *)) _f_write)(fh, count, buf); }

INLINE long d_getpath(char *path, int drv)
{ return ((long _cdecl (*)(char *, int)) _d_getpath)(path, drv); }

INLINE long m_xalloc(long size, int mode)
{ return ((long _cdecl (*)(long, int)) _m_xalloc)(size, mode); }

INLINE long f_dup(short fd)
{ return ((long _cdecl (*)(short)) _f_dup)(fd); }

INLINE long m_alloc(long size)
{ return ((long _cdecl (*)(long)) _m_alloc)(size); }

INLINE long m_free(void *block)
{ return ((long _cdecl (*)(void *)) _m_free)(block); }

INLINE long m_shrink(int dummy, long block, long size)
{ return ((long _cdecl (*)(int, long, long)) _m_shrink)(dummy, block, size); }

INLINE long p_exec(int mode, const void *ptr1, const void *ptr2, const void *ptr3)
{ return ((long _cdecl (*)(int, const void *, const void *, const void *)) _p_exec)(mode, ptr1, ptr2, ptr3); }

INLINE long p_term(int code)
{ return ((long _cdecl (*)(int)) _p_term)(code); }

INLINE long f_sfirst(const char *path, int attrib)
{ return ((long _cdecl (*)(const char *, int)) _f_sfirst)(path, attrib); }

INLINE long f_snext(void)
{ return ((long _cdecl (*)(void)) _f_snext)(); }

INLINE long s_yield(void)
{ return ((long _cdecl (*)(void)) _s_yield)(); }

INLINE long f_cntl(int fh, long arg, int cmd)
{ return ((long _cdecl (*)(int, long, int)) _f_cntl)(fh, arg, cmd); }

INLINE long p_nice(int increment)
{ return ((long _cdecl (*)(int)) _p_nice)(increment); }

INLINE long p_getpid(void)
{ return ((long _cdecl (*)(void)) _p_getpid)(); }

INLINE long p_getppid(void)
{ return ((long _cdecl (*)(void)) _p_getppid)(); }

INLINE long p_getpgrp(void)
{ return ((long _cdecl (*)(void)) _p_getpgrp)(); }

INLINE long p_setpgrp(int pid, int newgrp)
{ return ((long _cdecl (*)(int, int)) _p_setpgrp)(pid, newgrp); }

INLINE long p_getuid(void)
{ return ((long _cdecl (*)(void)) _p_getuid)(); }

INLINE long p_setuid(int id)
{ return ((long _cdecl (*)(int)) _p_setuid)(id); }

INLINE long p_kill(int pid, int sig)
{ return ((long _cdecl (*)(int, int)) _p_kill)(pid, sig); }

INLINE long p_getgid(void)
{ return ((long _cdecl (*)(void)) _p_getgid)(); }

INLINE long p_setgid(int id)
{ return ((long _cdecl (*)(int)) _p_setgid)(id); }

INLINE long p_sigblock(ulong mask)
{ return ((long _cdecl (*)(ulong)) _p_sigblock)(mask); }

INLINE long p_domain(int arg)
{ return ((long _cdecl (*)(int)) _p_domain)(arg); }

INLINE long f_select(unsigned timeout, long *rfdp, long *wfdp, long *xfdp)
{ return ((long _cdecl (*)(unsigned, long *, long *, long *)) _f_select)(timeout, rfdp, wfdp, xfdp); }

INLINE long s_ysconf(int which)
{ return ((long _cdecl (*)(int)) _s_ysconf)(which); }

INLINE long d_pathconf(const char *name, int which)
{ return ((long _cdecl (*)(const char *, int)) _d_pathconf)(name, which); }

INLINE long p_msg(int mode, long _cdecl mbid, char *ptr)
{ return ((long _cdecl (*)(int, long _cdecl, char *)) _p_msg)(mode, mbid, ptr); }

INLINE long d_opendir(const char *path, int flags)
{ return ((long _cdecl (*)(const char *, int)) _d_opendir)(path, flags); }

INLINE long d_readdir(int len, long handle, char *buf)
{ return ((long _cdecl (*)(int, long, char *)) _d_readdir)(len, handle, buf); }

INLINE long d_closedir(long handle)
{ return ((long _cdecl (*)(long)) _d_closedir)(handle); }

INLINE long f_xattr(int flag, const char *name, XATTR *xattr)
{ return ((long _cdecl (*)(int, const char *, XATTR *)) _f_xattr)(flag, name, xattr); }

INLINE long d_cntl(int cmd, const char *name, long arg)
{ return ((long _cdecl (*)(int, const char *, long)) _d_cntl)(cmd, name, arg); }

INLINE long f_chown(const char *name, int uid, int gid)
{ return ((long _cdecl (*)(const char *, int, int)) _f_chown)(name, uid, gid); }

INLINE long f_chmod(const char *name, unsigned mode)
{ return ((long _cdecl (*)(const char *, unsigned)) _f_chmod)(name, mode); }

INLINE long p_semaphore(int mode, long id, long timeout)
{ return ((long _cdecl (*)(int, long, long)) _p_semaphore)(mode, id, timeout); }

INLINE long d_lock(int mode, int drv)
{ return ((long _cdecl (*)(int, int)) _d_lock)(mode, drv); }

INLINE long p_geteuid(void)
{ return ((long _cdecl (*)(void)) _p_geteuid)(); }

INLINE long p_getegid(void)
{ return ((long _cdecl (*)(void)) _p_getegid)(); }

INLINE long p_seteuid(int id)
{ return ((long _cdecl (*)(int)) _p_seteuid)(id); }

INLINE long p_setegid(int id)
{ return ((long _cdecl (*)(int)) _p_setegid)(id); }

INLINE long p_getauid(void)
{ return ((long _cdecl (*)(void)) _p_getauid)(); }

INLINE long p_setauid(int id)
{ return ((long _cdecl (*)(int)) _p_setauid)(id); }

INLINE long p_getgroups(int gidsetlen, int gidset[])
{ return ((long _cdecl (*)(int, int [])) _p_getgroups)(gidsetlen, gidset); }

INLINE long p_setgroups(int ngroups, int gidset[])
{ return ((long _cdecl (*)(int, int [])) _p_setgroups)(ngroups, gidset); }

INLINE long p_setreuid(int rid, int eid)
{ return ((long _cdecl (*)(int, int)) _p_setreuid)(rid, eid); }

INLINE long p_setregid(int rid, int eid)
{ return ((long _cdecl (*)(int, int)) _p_setregid)(rid, eid); }

INLINE long s_ync(void)
{ return ((long _cdecl (*)(void)) _s_ync)(); }

INLINE long s_hutdown(long restart)
{ return ((long _cdecl (*)(long)) _s_hutdown)(restart); }

INLINE long s_system(int mode, ulong arg1, ulong arg2)
{ return ((long _cdecl (*)(int, ulong, ulong)) _s_system)(mode, arg1, arg2); }

INLINE long p_sysctl(long *name, ulong namelen, void *old, ulong *oldlenp, const void *new, ulong newlen)
{ return ((long _cdecl (*)(long *, ulong, void *, ulong *, const void *, ulong)) _p_sysctl)(name, namelen, old, oldlenp, new, newlen); }


# define datestamp		t_getdate()
# define timestamp		t_gettime()
# define FreeMemory		m_alloc(-1L)


# define changedrive		(*KERNEL->drvchng)
# define KERNEL_TRACE		(*KERNEL->trace)
# define KERNEL_DEBUG		(*KERNEL->debug)
# define KERNEL_ALERT		(*KERNEL->alert)
# define FATAL			(*KERNEL->fatal)

# define kmalloc		(*KERNEL->kmalloc)
# define kfree			(*KERNEL->kfree)
# define umalloc		(*KERNEL->umalloc)
# define ufree			(*KERNEL->ufree)

# undef strnicmp
# define strnicmp		(*KERNEL->kstrnicmp)
# undef stricmp
# define stricmp		(*KERNEL->kstricmp)
# undef strlwr
# define strlwr			(*KERNEL->kstrlwr)
# undef strupr
# define strupr			(*KERNEL->kstrupr)
# undef ksprintf
# define ksprintf		(*KERNEL->ksprintf)

# define ms_time		(*KERNEL->millis_time)
# define unixtime		(*KERNEL->unixtime)
# define dostime		(*KERNEL->dostime)

# define nap			(*KERNEL->nap)
# define sleep			(*KERNEL->sleep)

/* for sleep */
# define CURPROC_Q		0
# define READY_Q		1
# define WAIT_Q			2
# define IO_Q			3
# define ZOMBIE_Q		4
# define TSR_Q			5
# define STOP_Q			6
# define SELECT_Q		7

# define wake			(*KERNEL->wake)
# define wakeselect		(*KERNEL->wakeselect)
# define denyshare		(*KERNEL->denyshare)
# define denylock		(*KERNEL->denylock)
# define addtimeout		(*KERNEL->addtimeout)
# define canceltimeout		(*KERNEL->canceltimeout)
# define addroottimeout		(*KERNEL->addroottimeout)
# define cancelroottimeout	(*KERNEL->cancelroottimeout)
# define ikill			(*KERNEL->ikill)
# define iwake			(*KERNEL->iwake)
# define bio			(*KERNEL->bio)
# define utc			(*KERNEL->xtime)
# define add_rsvfentry		(*KERNEL->add_rsvfentry)
# define del_rsvfentry		(*KERNEL->del_rsvfentry)
# define killgroup		(*KERNEL->killgroup)
# define dma_interface		( KERNEL->dma)
# define loops_per_sec_ptr	( KERNEL->loops_per_sec)
# define get_toscookie		(*KERNEL->get_toscookie)
# define so_register		(*KERNEL->so_register)
# define so_unregister		(*KERNEL->so_unregister)
# define so_release		(*KERNEL->so_release)
# define so_sockpair		(*KERNEL->so_sockpair)
# define so_connect		(*KERNEL->so_connect)
# define so_accept		(*KERNEL->so_accept)
# define so_create		(*KERNEL->so_create)
# define so_dup			(*KERNEL->so_dup)
# define so_free		(*KERNEL->so_free)
# define load_modules		(*KERNEL->load_modules)
# define kthread_create		(*KERNEL->kthread_create)
# define kthread_exit		(*KERNEL->kthread_exit)

# endif /* _libkern_kernel_xfs_xdd_h */
