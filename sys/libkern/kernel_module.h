/*
 * $Id$
 * 
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

# ifndef _libkern_kernel_module_h
# define _libkern_kernel_module_h

# include "mint/kentry.h"


# ifndef KENTRY
# define KENTRY	kentry
# endif

extern struct kentry *kentry;

# define MINT_MAJOR		(KENTRY->major)
# define MINT_MINOR		(KENTRY->minor)
# define MINT_PATCHLEVEL	(KENTRY->patchlevel)


/*
 * vec_dos
 */

# define _p_term0		(*KENTRY->vec_dos[0x000])
# define _c_conin		(*KENTRY->vec_dos[0x001])
# define _c_conout		(*KENTRY->vec_dos[0x002])
# define _c_auxin		(*KENTRY->vec_dos[0x003])
# define _c_auxout		(*KENTRY->vec_dos[0x004])
# define _c_prnout		(*KENTRY->vec_dos[0x005])
# define _c_rawio		(*KENTRY->vec_dos[0x006])
# define _c_rawcin		(*KENTRY->vec_dos[0x007])
# define _c_necin		(*KENTRY->vec_dos[0x008])
# define _c_conws		(*KENTRY->vec_dos[0x009])
# define _c_conrs		(*KENTRY->vec_dos[0x00a])
# define _c_conis		(*KENTRY->vec_dos[0x00b])
# define _0x00c			(*KENTRY->vec_dos[0x00c])
# define _0x00d			(*KENTRY->vec_dos[0x00d])
# define _d_setdrv		(*KENTRY->vec_dos[0x00e])
# define _0x00f			(*KENTRY->vec_dos[0x00f])

# define _c_conos		(*KENTRY->vec_dos[0x010])
# define _c_prnos		(*KENTRY->vec_dos[0x011])
# define _c_auxis		(*KENTRY->vec_dos[0x012])
# define _c_auxos		(*KENTRY->vec_dos[0x013])
# define _m_addalt		(*KENTRY->vec_dos[0x014])
# define _s_realloc		(*KENTRY->vec_dos[0x015])
# define _s_lbopen		(*KENTRY->vec_dos[0x016])	/* 1.15.3 MagiC: slb */
# define _s_lbclose		(*KENTRY->vec_dos[0x017])	/* 1.15.3 MagiC: slb */
# define _0x018			(*KENTRY->vec_dos[0x018])
# define _d_getdrv		(*KENTRY->vec_dos[0x019])
# define _f_setdta		(*KENTRY->vec_dos[0x01a])
# define _0x01b			(*KENTRY->vec_dos[0x01b])
# define _0x01c			(*KENTRY->vec_dos[0x01c])
# define _0x01d			(*KENTRY->vec_dos[0x01d])
# define _0x01e			(*KENTRY->vec_dos[0x01e])
# define _0x01f			(*KENTRY->vec_dos[0x01f])

# define _s_uper		(*KENTRY->vec_dos[0x020])
# define _0x021			(*KENTRY->vec_dos[0x021])
# define _0x022			(*KENTRY->vec_dos[0x022])
# define _0x023			(*KENTRY->vec_dos[0x023])
# define _0x024			(*KENTRY->vec_dos[0x024])
# define _0x025			(*KENTRY->vec_dos[0x025])
# define _0x026			(*KENTRY->vec_dos[0x026])
# define _0x027			(*KENTRY->vec_dos[0x027])
# define _0x028			(*KENTRY->vec_dos[0x028])
# define _0x029			(*KENTRY->vec_dos[0x029])
# define _t_getdate		(*KENTRY->vec_dos[0x02a])
# define _t_setdate		(*KENTRY->vec_dos[0x02b])
# define _t_gettime		(*KENTRY->vec_dos[0x02c])
# define _t_settime		(*KENTRY->vec_dos[0x02d])
# define _0x02e			(*KENTRY->vec_dos[0x02e])
# define _f_getdta		(*KENTRY->vec_dos[0x02f])

# define _s_version		(*KENTRY->vec_dos[0x030])
# define _p_termres		(*KENTRY->vec_dos[0x031])
# define _0x032			(*KENTRY->vec_dos[0x032])
# define _0x033			(*KENTRY->vec_dos[0x033])	/* MagiC: LONG Sconfig(WORD subfn, LONG flags); */
# define _0x034			(*KENTRY->vec_dos[0x034])
# define _0x035			(*KENTRY->vec_dos[0x035])
# define _d_free		(*KENTRY->vec_dos[0x036])
# define _0x037			(*KENTRY->vec_dos[0x037])
# define _0x038			(*KENTRY->vec_dos[0x038])
# define _d_create		(*KENTRY->vec_dos[0x039])
# define _d_delete		(*KENTRY->vec_dos[0x03a])
# define _d_setpath		(*KENTRY->vec_dos[0x03b])
# define _f_create		(*KENTRY->vec_dos[0x03c])
# define _f_open		(*KENTRY->vec_dos[0x03d])
# define _f_close		(*KENTRY->vec_dos[0x03e])
# define _f_read		(*KENTRY->vec_dos[0x03f])

# define _f_write		(*KENTRY->vec_dos[0x040])
# define _f_delete		(*KENTRY->vec_dos[0x041])
# define _f_seek		(*KENTRY->vec_dos[0x042])
# define _f_attrib		(*KENTRY->vec_dos[0x043])
# define _m_xalloc		(*KENTRY->vec_dos[0x044])
# define _f_dup			(*KENTRY->vec_dos[0x045])
# define _f_force		(*KENTRY->vec_dos[0x046])
# define _d_getpath		(*KENTRY->vec_dos[0x047])
# define _m_alloc		(*KENTRY->vec_dos[0x048])
# define _m_free		(*KENTRY->vec_dos[0x049])
# define _m_shrink		(*KENTRY->vec_dos[0x04a])
# define _p_exec		(*KENTRY->vec_dos[0x04b])
# define _p_term		(*KENTRY->vec_dos[0x04c])
# define _0x04d			(*KENTRY->vec_dos[0x04d])
# define _f_sfirst		(*KENTRY->vec_dos[0x04e])
# define _f_snext		(*KENTRY->vec_dos[0x04f])

# define _0x050			(*KENTRY->vec_dos[0x050])
# define _0x051			(*KENTRY->vec_dos[0x051])
# define _0x052			(*KENTRY->vec_dos[0x052])
# define _0x053			(*KENTRY->vec_dos[0x053])
# define _0x054			(*KENTRY->vec_dos[0x054])
# define _0x055			(*KENTRY->vec_dos[0x055])
# define _f_rename		(*KENTRY->vec_dos[0x056])
# define _f_datime		(*KENTRY->vec_dos[0x057])
# define _0x058			(*KENTRY->vec_dos[0x058])
# define _0x059			(*KENTRY->vec_dos[0x059])
# define _0x05a			(*KENTRY->vec_dos[0x05a])
# define _0x05b			(*KENTRY->vec_dos[0x05b])
# define _f_lock		(*KENTRY->vec_dos[0x05c])
# define _0x05d			(*KENTRY->vec_dos[0x05d])
# define _0x05e			(*KENTRY->vec_dos[0x05e])
# define _0x05f			(*KENTRY->vec_dos[0x05f])

# define _0x060			(*KENTRY->vec_dos[0x060])
# define _0x061			(*KENTRY->vec_dos[0x061])
# define _0x062			(*KENTRY->vec_dos[0x062])
# define _0x063			(*KENTRY->vec_dos[0x063])
# define _0x064			(*KENTRY->vec_dos[0x064])
# define _0x065			(*KENTRY->vec_dos[0x065])
# define _0x066			(*KENTRY->vec_dos[0x066])
# define _0x067			(*KENTRY->vec_dos[0x067])
# define _0x068			(*KENTRY->vec_dos[0x068])
# define _0x069			(*KENTRY->vec_dos[0x069])
# define _0x06a			(*KENTRY->vec_dos[0x06a])
# define _0x06b			(*KENTRY->vec_dos[0x06b])
# define _0x06c			(*KENTRY->vec_dos[0x06c])
# define _0x06d			(*KENTRY->vec_dos[0x06d])
# define _0x06e			(*KENTRY->vec_dos[0x06e])
# define _0x06f			(*KENTRY->vec_dos[0x06f])

# define _0x070			(*KENTRY->vec_dos[0x070])
# define _0x071			(*KENTRY->vec_dos[0x071])
# define _0x072			(*KENTRY->vec_dos[0x072])
# define _0x073			(*KENTRY->vec_dos[0x073])
# define _0x074			(*KENTRY->vec_dos[0x074])
# define _0x075			(*KENTRY->vec_dos[0x075])
# define _0x076			(*KENTRY->vec_dos[0x076])
# define _0x077			(*KENTRY->vec_dos[0x077])
# define _0x078			(*KENTRY->vec_dos[0x078])
# define _0x079			(*KENTRY->vec_dos[0x079])
# define _0x07a			(*KENTRY->vec_dos[0x07a])
# define _0x07b			(*KENTRY->vec_dos[0x07b])
# define _0x07c			(*KENTRY->vec_dos[0x07c])
# define _0x07d			(*KENTRY->vec_dos[0x07d])
# define _0x07e			(*KENTRY->vec_dos[0x07e])
# define _0x07f			(*KENTRY->vec_dos[0x07f])

# define _0x080			(*KENTRY->vec_dos[0x080])
# define _0x081			(*KENTRY->vec_dos[0x081])
# define _0x082			(*KENTRY->vec_dos[0x082])
# define _0x083			(*KENTRY->vec_dos[0x083])
# define _0x084			(*KENTRY->vec_dos[0x084])
# define _0x085			(*KENTRY->vec_dos[0x085])
# define _0x086			(*KENTRY->vec_dos[0x086])
# define _0x087			(*KENTRY->vec_dos[0x087])
# define _0x088			(*KENTRY->vec_dos[0x088])
# define _0x089			(*KENTRY->vec_dos[0x089])
# define _0x08a			(*KENTRY->vec_dos[0x08a])
# define _0x08b			(*KENTRY->vec_dos[0x08b])
# define _0x08c			(*KENTRY->vec_dos[0x08c])
# define _0x08d			(*KENTRY->vec_dos[0x08d])
# define _0x08e			(*KENTRY->vec_dos[0x08e])
# define _0x08f			(*KENTRY->vec_dos[0x08f])

# define _0x090			(*KENTRY->vec_dos[0x090])
# define _0x091			(*KENTRY->vec_dos[0x091])
# define _0x092			(*KENTRY->vec_dos[0x092])
# define _0x093			(*KENTRY->vec_dos[0x093])
# define _0x094			(*KENTRY->vec_dos[0x094])
# define _0x095			(*KENTRY->vec_dos[0x095])
# define _0x096			(*KENTRY->vec_dos[0x096])
# define _0x097			(*KENTRY->vec_dos[0x097])
# define _0x098			(*KENTRY->vec_dos[0x098])
# define _0x099			(*KENTRY->vec_dos[0x099])
# define _0x09a			(*KENTRY->vec_dos[0x09a])
# define _0x09b			(*KENTRY->vec_dos[0x09b])
# define _0x09c			(*KENTRY->vec_dos[0x09c])
# define _0x09d			(*KENTRY->vec_dos[0x09d])
# define _0x09e			(*KENTRY->vec_dos[0x09e])
# define _0x09f			(*KENTRY->vec_dos[0x09f])

# define _0x0a0			(*KENTRY->vec_dos[0x0a0])
# define _0x0a1			(*KENTRY->vec_dos[0x0a1])
# define _0x0a2			(*KENTRY->vec_dos[0x0a2])
# define _0x0a3			(*KENTRY->vec_dos[0x0a3])
# define _0x0a4			(*KENTRY->vec_dos[0x0a4])
# define _0x0a5			(*KENTRY->vec_dos[0x0a5])
# define _0x0a6			(*KENTRY->vec_dos[0x0a6])
# define _0x0a7			(*KENTRY->vec_dos[0x0a7])
# define _0x0a8			(*KENTRY->vec_dos[0x0a8])
# define _0x0a9			(*KENTRY->vec_dos[0x0a9])
# define _0x0aa			(*KENTRY->vec_dos[0x0aa])
# define _0x0ab			(*KENTRY->vec_dos[0x0ab])
# define _0x0ac			(*KENTRY->vec_dos[0x0ac])
# define _0x0ad			(*KENTRY->vec_dos[0x0ad])
# define _0x0ae			(*KENTRY->vec_dos[0x0ae])
# define _0x0af			(*KENTRY->vec_dos[0x0af])

# define _0x0b0			(*KENTRY->vec_dos[0x0b0])
# define _0x0b1			(*KENTRY->vec_dos[0x0b1])
# define _0x0b2			(*KENTRY->vec_dos[0x0b2])
# define _0x0b3			(*KENTRY->vec_dos[0x0b3])
# define _0x0b4			(*KENTRY->vec_dos[0x0b4])
# define _0x0b5			(*KENTRY->vec_dos[0x0b5])
# define _0x0b6			(*KENTRY->vec_dos[0x0b6])
# define _0x0b7			(*KENTRY->vec_dos[0x0b7])
# define _0x0b8			(*KENTRY->vec_dos[0x0b8])
# define _0x0b9			(*KENTRY->vec_dos[0x0b9])
# define _0x0ba			(*KENTRY->vec_dos[0x0ba])
# define _0x0bb			(*KENTRY->vec_dos[0x0bb])
# define _0x0bc			(*KENTRY->vec_dos[0x0bc])
# define _0x0bd			(*KENTRY->vec_dos[0x0bd])
# define _0x0be			(*KENTRY->vec_dos[0x0be])
# define _0x0bf			(*KENTRY->vec_dos[0x0bf])

# define _0x0c0			(*KENTRY->vec_dos[0x0c0])
# define _0x0c1			(*KENTRY->vec_dos[0x0c1])
# define _0x0c2			(*KENTRY->vec_dos[0x0c2])
# define _0x0c3			(*KENTRY->vec_dos[0x0c3])
# define _0x0c4			(*KENTRY->vec_dos[0x0c4])
# define _0x0c5			(*KENTRY->vec_dos[0x0c5])
# define _0x0c6			(*KENTRY->vec_dos[0x0c6])
# define _0x0c7			(*KENTRY->vec_dos[0x0c7])
# define _0x0c8			(*KENTRY->vec_dos[0x0c8])
# define _0x0c9			(*KENTRY->vec_dos[0x0c9])
# define _0x0ca			(*KENTRY->vec_dos[0x0ca])
# define _0x0cb			(*KENTRY->vec_dos[0x0cb])
# define _0x0cc			(*KENTRY->vec_dos[0x0cc])
# define _0x0cd			(*KENTRY->vec_dos[0x0cd])
# define _0x0ce			(*KENTRY->vec_dos[0x0ce])
# define _0x0cf			(*KENTRY->vec_dos[0x0cf])

# define _0x0d0			(*KENTRY->vec_dos[0x0d0])
# define _0x0d1			(*KENTRY->vec_dos[0x0d1])
# define _0x0d2			(*KENTRY->vec_dos[0x0d2])
# define _0x0d3			(*KENTRY->vec_dos[0x0d3])
# define _0x0d4			(*KENTRY->vec_dos[0x0d4])
# define _0x0d5			(*KENTRY->vec_dos[0x0d5])
# define _0x0d6			(*KENTRY->vec_dos[0x0d6])
# define _0x0d7			(*KENTRY->vec_dos[0x0d7])
# define _0x0d8			(*KENTRY->vec_dos[0x0d8])
# define _0x0d9			(*KENTRY->vec_dos[0x0d9])
# define _0x0da			(*KENTRY->vec_dos[0x0da])
# define _0x0db			(*KENTRY->vec_dos[0x0db])
# define _0x0dc			(*KENTRY->vec_dos[0x0dc])
# define _0x0dd			(*KENTRY->vec_dos[0x0dd])
# define _0x0de			(*KENTRY->vec_dos[0x0de])
# define _0x0df			(*KENTRY->vec_dos[0x0df])

# define _0x0e0			(*KENTRY->vec_dos[0x0e0])
# define _0x0e1			(*KENTRY->vec_dos[0x0e1])
# define _0x0e2			(*KENTRY->vec_dos[0x0e2])
# define _0x0e3			(*KENTRY->vec_dos[0x0e3])
# define _0x0e4			(*KENTRY->vec_dos[0x0e4])
# define _0x0e5			(*KENTRY->vec_dos[0x0e5])
# define _0x0e6			(*KENTRY->vec_dos[0x0e6])
# define _0x0e7			(*KENTRY->vec_dos[0x0e7])
# define _0x0e8			(*KENTRY->vec_dos[0x0e8])
# define _0x0e9			(*KENTRY->vec_dos[0x0e9])
# define _0x0ea			(*KENTRY->vec_dos[0x0ea])
# define _0x0eb			(*KENTRY->vec_dos[0x0eb])
# define _0x0ec			(*KENTRY->vec_dos[0x0ec])
# define _0x0ed			(*KENTRY->vec_dos[0x0ed])
# define _0x0ee			(*KENTRY->vec_dos[0x0ee])
# define _0x0ef			(*KENTRY->vec_dos[0x0ef])

# define _0x0f0			(*KENTRY->vec_dos[0x0f0])
# define _0x0f1			(*KENTRY->vec_dos[0x0f1])
# define _0x0f2			(*KENTRY->vec_dos[0x0f2])
# define _0x0f3			(*KENTRY->vec_dos[0x0f3])
# define _0x0f4			(*KENTRY->vec_dos[0x0f4])
# define _0x0f5			(*KENTRY->vec_dos[0x0f5])
# define _0x0f6			(*KENTRY->vec_dos[0x0f6])
# define _0x0f7			(*KENTRY->vec_dos[0x0f7])
# define _0x0f8			(*KENTRY->vec_dos[0x0f8])
# define _0x0f9			(*KENTRY->vec_dos[0x0f9])
# define _0x0fa			(*KENTRY->vec_dos[0x0fa])
# define _0x0fb			(*KENTRY->vec_dos[0x0fb])
# define _0x0fc			(*KENTRY->vec_dos[0x0fc])
# define _0x0fd			(*KENTRY->vec_dos[0x0fd])
# define _0x0fe			(*KENTRY->vec_dos[0x0fe])

/*
 * MiNT extensions to GEMDOS
 */

# define _s_yield		(*KENTRY->vec_dos[0x0ff])

# define _f_pipe		(*KENTRY->vec_dos[0x100])
# define _f_fchown		(*KENTRY->vec_dos[0x101])	/* 1.15.2 */
# define _f_fchmod		(*KENTRY->vec_dos[0x102])	/* 1.15.2 */
# define _0x103			(*KENTRY->vec_dos[0x103]) 	/* f_sync */
# define _f_cntl		(*KENTRY->vec_dos[0x104])
# define _f_instat		(*KENTRY->vec_dos[0x105])
# define _f_outstat		(*KENTRY->vec_dos[0x106])
# define _f_getchar		(*KENTRY->vec_dos[0x107])
# define _f_putchar		(*KENTRY->vec_dos[0x108])
# define _p_wait		(*KENTRY->vec_dos[0x109])
# define _p_nice		(*KENTRY->vec_dos[0x10a])
# define _p_getpid		(*KENTRY->vec_dos[0x10b])
# define _p_getppid		(*KENTRY->vec_dos[0x10c])
# define _p_getpgrp		(*KENTRY->vec_dos[0x10d])
# define _p_setpgrp		(*KENTRY->vec_dos[0x10e])
# define _p_getuid		(*KENTRY->vec_dos[0x10f])

# define _p_setuid		(*KENTRY->vec_dos[0x110])
# define _p_kill		(*KENTRY->vec_dos[0x111])
# define _p_signal		(*KENTRY->vec_dos[0x112])
# define _p_vfork		(*KENTRY->vec_dos[0x113])
# define _p_getgid		(*KENTRY->vec_dos[0x114])
# define _p_setgid		(*KENTRY->vec_dos[0x115])
# define _p_sigblock		(*KENTRY->vec_dos[0x116])
# define _p_sigsetmask		(*KENTRY->vec_dos[0x117])
# define _p_usrval		(*KENTRY->vec_dos[0x118])
# define _p_domain		(*KENTRY->vec_dos[0x119])
# define _p_sigreturn		(*KENTRY->vec_dos[0x11a])
# define _p_fork		(*KENTRY->vec_dos[0x11b])
# define _p_wait3		(*KENTRY->vec_dos[0x11c])
# define _f_select		(*KENTRY->vec_dos[0x11d])
# define _p_rusage		(*KENTRY->vec_dos[0x11e])
# define _p_setlimit		(*KENTRY->vec_dos[0x11f])

# define _t_alarm		(*KENTRY->vec_dos[0x120])
# define _p_pause		(*KENTRY->vec_dos[0x121])
# define _s_ysconf		(*KENTRY->vec_dos[0x122])
# define _p_sigpending		(*KENTRY->vec_dos[0x123])
# define _d_pathconf		(*KENTRY->vec_dos[0x124])
# define _p_msg			(*KENTRY->vec_dos[0x125])
# define _f_midipipe		(*KENTRY->vec_dos[0x126])
# define _p_renice		(*KENTRY->vec_dos[0x127])
# define _d_opendir		(*KENTRY->vec_dos[0x128])
# define _d_readdir		(*KENTRY->vec_dos[0x129])
# define _d_rewind		(*KENTRY->vec_dos[0x12a])
# define _d_closedir		(*KENTRY->vec_dos[0x12b])
# define _f_xattr		(*KENTRY->vec_dos[0x12c])
# define _f_link		(*KENTRY->vec_dos[0x12d])
# define _f_symlink		(*KENTRY->vec_dos[0x12e])
# define _f_readlink		(*KENTRY->vec_dos[0x12f])

# define _d_cntl		(*KENTRY->vec_dos[0x130])
# define _f_chown		(*KENTRY->vec_dos[0x131])
# define _f_chmod		(*KENTRY->vec_dos[0x132])
# define _p_umask		(*KENTRY->vec_dos[0x133])
# define _p_semaphore		(*KENTRY->vec_dos[0x134])
# define _d_lock		(*KENTRY->vec_dos[0x135])
# define _p_sigpause		(*KENTRY->vec_dos[0x136])
# define _p_sigaction		(*KENTRY->vec_dos[0x137])
# define _p_geteuid		(*KENTRY->vec_dos[0x138])
# define _p_getegid		(*KENTRY->vec_dos[0x139])
# define _p_waitpid		(*KENTRY->vec_dos[0x13a])
# define _d_getcwd		(*KENTRY->vec_dos[0x13b])
# define _s_alert		(*KENTRY->vec_dos[0x13c])
# define _t_malarm		(*KENTRY->vec_dos[0x13d])
# define _p_sigintr		(*KENTRY->vec_dos[0x13e])
# define _s_uptime		(*KENTRY->vec_dos[0x13f])

# define _0x140			(*KENTRY->vec_dos[0x140])	/* s_trapatch */
# define _0x141			(*KENTRY->vec_dos[0x141])
# define _d_xreaddir		(*KENTRY->vec_dos[0x142])
# define _p_seteuid		(*KENTRY->vec_dos[0x143])
# define _p_setegid		(*KENTRY->vec_dos[0x144])
# define _p_getauid		(*KENTRY->vec_dos[0x145])
# define _p_setauid		(*KENTRY->vec_dos[0x146])
# define _p_getgroups		(*KENTRY->vec_dos[0x147])
# define _p_setgroups		(*KENTRY->vec_dos[0x148])
# define _t_setitimer		(*KENTRY->vec_dos[0x149])
# define _d_chroot		(*KENTRY->vec_dos[0x14a])	/* 1.15.3 */
# define _f_stat64		(*KENTRY->vec_dos[0x14b])	/* f_stat */
# define _f_seek64		(*KENTRY->vec_dos[0x14c])
# define _d_setkey		(*KENTRY->vec_dos[0x14d])
# define _p_setreuid		(*KENTRY->vec_dos[0x14e])
# define _p_setregid		(*KENTRY->vec_dos[0x14f])

# define _s_ync			(*KENTRY->vec_dos[0x150])
# define _s_hutdown		(*KENTRY->vec_dos[0x151])
# define _d_readlabel		(*KENTRY->vec_dos[0x152])
# define _d_writelabel		(*KENTRY->vec_dos[0x153])
# define _s_system		(*KENTRY->vec_dos[0x154])
# define _t_gettimeofday	(*KENTRY->vec_dos[0x155])
# define _t_settimeofday	(*KENTRY->vec_dos[0x156])
# define _0x157			(*KENTRY->vec_dos[0x157])	/* t_adjtime */
# define _p_getpriority		(*KENTRY->vec_dos[0x158])
# define _p_setpriority		(*KENTRY->vec_dos[0x159])
# define _f_poll		(*KENTRY->vec_dos[0x15a])
# define _f_writev		(*KENTRY->vec_dos[0x15b])
# define _f_readv		(*KENTRY->vec_dos[0x15c])
# define _f_fstat		(*KENTRY->vec_dos[0x15d])
# define _p_sysctl		(*KENTRY->vec_dos[0x15e])
# define _s_emulation		(*KENTRY->vec_dos[0x15f])

# define _f_socket		(*KENTRY->vec_dos[0x160])
# define _f_socketpair		(*KENTRY->vec_dos[0x161])
# define _f_accept		(*KENTRY->vec_dos[0x162])
# define _f_connect		(*KENTRY->vec_dos[0x163])
# define _f_bind		(*KENTRY->vec_dos[0x164])
# define _f_listen		(*KENTRY->vec_dos[0x165])
# define _f_recvmsg		(*KENTRY->vec_dos[0x166])
# define _f_sendmsg		(*KENTRY->vec_dos[0x167])
# define _f_recvfrom		(*KENTRY->vec_dos[0x168])
# define _f_sendto		(*KENTRY->vec_dos[0x169])
# define _f_setsockopt		(*KENTRY->vec_dos[0x16a])
# define _f_getsockopt		(*KENTRY->vec_dos[0x16b])
# define _f_getpeername		(*KENTRY->vec_dos[0x16c])
# define _f_getsockname		(*KENTRY->vec_dos[0x16d])
# define _f_shutdown		(*KENTRY->vec_dos[0x16e])
# define _0x16f			(*KENTRY->vec_dos[0x16f])

# define _p_shmget		(*KENTRY->vec_dos[0x170])
# define _p_shmctl		(*KENTRY->vec_dos[0x171])
# define _p_shmat		(*KENTRY->vec_dos[0x172])
# define _p_shmdt		(*KENTRY->vec_dos[0x173])
# define _p_semget		(*KENTRY->vec_dos[0x174])
# define _p_semctl		(*KENTRY->vec_dos[0x175])
# define _p_semop		(*KENTRY->vec_dos[0x176])
# define _p_semconfig		(*KENTRY->vec_dos[0x177])
# define _p_msgget		(*KENTRY->vec_dos[0x178])
# define _p_msgctl		(*KENTRY->vec_dos[0x179])
# define _p_msgsnd		(*KENTRY->vec_dos[0x17a])
# define _p_msgrcv		(*KENTRY->vec_dos[0x17b])
# define _0x17c			(*KENTRY->vec_dos[0x17c])
# define _m_access		(*KENTRY->vec_dos[0x17d])
# define _0x17e			(*KENTRY->vec_dos[0x17e])
# define _0x17f			(*KENTRY->vec_dos[0x17f])

# define _0x180			(*KENTRY->vec_dos[0x180])
# define _0x181			(*KENTRY->vec_dos[0x181])
# define _0x182			(*KENTRY->vec_dos[0x182])
# define _0x183			(*KENTRY->vec_dos[0x183])
# define _0x184			(*KENTRY->vec_dos[0x184])
# define _0x185			(*KENTRY->vec_dos[0x185])
# define _0x186			(*KENTRY->vec_dos[0x186])
# define _0x187			(*KENTRY->vec_dos[0x187])
# define _0x188			(*KENTRY->vec_dos[0x188])
# define _0x189			(*KENTRY->vec_dos[0x189])
# define _0x18a			(*KENTRY->vec_dos[0x18a])
# define _0x18b			(*KENTRY->vec_dos[0x18b])
# define _0x18c			(*KENTRY->vec_dos[0x18c])
# define _0x18d			(*KENTRY->vec_dos[0x18d])
# define _0x18e			(*KENTRY->vec_dos[0x18e])
# define _0x18f			(*KENTRY->vec_dos[0x18f])

	/* 0x190 */		/* DOS_MAX */

INLINE long c_conws(const char *str)
{ return ((long _cdecl (*)(const char *)) _c_conws)(str); }

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

INLINE long p_setpriority(int which, int who, int prio)
{ return ((long _cdecl (*)(int, int, int)) _p_setpriority)(which, who, prio); }

INLINE long p_sysctl(long *name, ulong namelen, void *old, ulong *oldlenp, const void *new, ulong newlen)
{ return ((long _cdecl (*)(long *, ulong, void *, ulong *, const void *, ulong)) _p_sysctl)(name, namelen, old, oldlenp, new, newlen); }


/*
 * kentry_proc
 */

# define nap			(*KENTRY->vec_proc.nap)
# define sleep			(*KENTRY->vec_proc.sleep)
# define wake			(*KENTRY->vec_proc.wake)
# define wakeselect		(*KENTRY->vec_proc.wakeselect)

# define ikill			(*KENTRY->vec_proc.ikill)
# define iwake			(*KENTRY->vec_proc.iwake)
# define killgroup

# define addtimeout		(*KENTRY->vec_proc.addtimeout)
# define canceltimeout		(*KENTRY->vec_proc.canceltimeout)
# define addroottimeout		(*KENTRY->vec_proc.addroottimeout)
# define cancelroottimeout	(*KENTRY->vec_proc.cancelroottimeout)

# define kthread_create		(*KENTRY->vec_proc.kthread_create)
# define kthread_exit		(*KENTRY->vec_proc.kthread_exit)

/* for sleep */
# define CURPROC_Q		0
# define READY_Q		1
# define WAIT_Q			2
# define IO_Q			3
# define ZOMBIE_Q		4
# define TSR_Q			5
# define STOP_Q			6
# define SELECT_Q		7


/*
 * kentry_mem
 */

# define kmalloc(x)		(*KENTRY->vec_mem.kmalloc)(x, __FUNCTION__)
# define kfree(x)		(*KENTRY->vec_mem.kfree)(x, __FUNCTION__)

# define umalloc(x)		(*KENTRY->vec_mem.umalloc)(x, __FUNCTION__)
# define ufree(x)		(*KENTRY->vec_mem.ufree)(x, __FUNCTION__)


/*
 * kentry_fs
 */

# define DEFAULT_MODE		( KENTRY->vec_fs.default_perm)
# define DEFAULT_DMODE		( 0777)

# define changedrive(drv)	(*KENTRY->vec_fs.drvchng)(drv, __FUNCTION__)

# define denyshare		(*KENTRY->vec_fs.denyshare)
# define denylock		(*KENTRY->vec_fs.denylock)

# define bio			( KENTRY->vec_fs.bio)
# define utc			(*KENTRY->vec_fs.xtime)


/*
 * kentry_sockets
 */

# define so_register		(*KENTRY->vec_sockets.so_register)
# define so_unregister		(*KENTRY->vec_sockets.so_unregister)
# define so_release		(*KENTRY->vec_sockets.so_release)
# define so_sockpair		(*KENTRY->vec_sockets.so_sockpair)
# define so_connect		(*KENTRY->vec_sockets.so_connect)
# define so_accept		(*KENTRY->vec_sockets.so_accept)
# define so_create		(*KENTRY->vec_sockets.so_create)
# define so_dup			(*KENTRY->vec_sockets.so_dup)
# define so_free		(*KENTRY->vec_sockets.so_free)


/*
 * kentry_module
 */

# define load_modules		(*KENTRY->vec_module.load_modules)


/*
 * kentry_cnf
 */

# define parse_cnf		(*KENTRY->vec_cnf.parse_cnf)
# define parse_include		(*KENTRY->vec_cnf.parse_include)
# define parser_msg		(*KENTRY->vec_cnf.parser_msg)


/*
 * kentry_misc
 */

# define dma_interface		( KENTRY->vec_misc.dma)
# define loops_per_sec_ptr	( KENTRY->vec_misc.loops_per_sec)
# define get_toscookie		(*KENTRY->vec_misc.get_toscookie)
# define add_rsvfentry		(*KENTRY->vec_misc.add_rsvfentry)
# define del_rsvfentry		(*KENTRY->vec_misc.del_rsvfentry)
# define remaining_proc_time	(*KENTRY->vec_misc.remaining_proc_time)


/*
 * kentry_debug
 */

# define KERNEL_TRACE		(*KENTRY->vec_debug.trace)
# define KERNEL_DEBUG		(*KENTRY->vec_debug.debug)
# define KERNEL_ALERT		(*KENTRY->vec_debug.alert)
# define KERNEL_FATAL		(*KENTRY->vec_debug.fatal)


/*
 * kentry_libkern
 */

# define _ctype			( KENTRY->vec_libkern._ctype)

# define tolower		(*KENTRY->vec_libkern.tolower)
# define toupper		(*KENTRY->vec_libkern.toupper)

# define vsprintf		(*KENTRY->vec_libkern.vsprintf)
# define sprintf		(*KENTRY->vec_libkern.sprintf)
# define getenv			(*KENTRY->vec_libkern.getenv)
# define atol			(*KENTRY->vec_libkern.atol)
# define strtonumber		(*KENTRY->vec_libkern.strtonumber)
# define strlen			(*KENTRY->vec_libkern.strlen)
# define strcmp			(*KENTRY->vec_libkern.strcmp)
# define strncmp		(*KENTRY->vec_libkern.strncmp)
# define stricmp		(*KENTRY->vec_libkern.stricmp)
# define strnicmp		(*KENTRY->vec_libkern.strnicmp)
# define strcpy			(*KENTRY->vec_libkern.strcpy)
# define strncpy		(*KENTRY->vec_libkern.strncpy)
# define strncpy_f		(*KENTRY->vec_libkern.strncpy_f)
# define strlwr			(*KENTRY->vec_libkern.strlwr)
# define strupr			(*KENTRY->vec_libkern.strupr)
# define strcat			(*KENTRY->vec_libkern.strcat)
# define strchr			(*KENTRY->vec_libkern.strchr)
# define strrchr		(*KENTRY->vec_libkern.strrchr)
# define strrev			(*KENTRY->vec_libkern.strrev)
# define strstr			(*KENTRY->vec_libkern.strstr)
# define strtol			(*KENTRY->vec_libkern.strtol)
# define strtoul		(*KENTRY->vec_libkern.strtoul)
# define memchr			(*KENTRY->vec_libkern.memchr)
# define memcmp			(*KENTRY->vec_libkern.memcmp)

# define ms_time		(*KENTRY->vec_libkern.ms_time)
# define unix2calendar		(*KENTRY->vec_libkern.unix2calendar)
# define unix2xbios		(*KENTRY->vec_libkern.unix2xbios)
# define dostime		(*KENTRY->vec_libkern.dostime)
# define unixtime		(*KENTRY->vec_libkern.unixtime)

# define blockcpy		(*KENTRY->vec_libkern.blockcpy)
# define quickcpy		(*KENTRY->vec_libkern.quickcpy)
# define quickswap		(*KENTRY->vec_libkern.quickswap)
# define quickzero		(*KENTRY->vec_libkern.quickzero)
# define memcpy			(*KENTRY->vec_libkern.memcpy)
# define memset			(*KENTRY->vec_libkern.memset)
# define bcopy			(*KENTRY->vec_libkern.bcopy)
# define bzero			(*KENTRY->vec_libkern.bzero)

# endif /* _libkern_kernel_module_h */
