/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 *
 * Author: Frank Naumann <fnaumann@fremint.de>
 * Started: 1999-10-06
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 *
 * changes since last version:
 *
 * 1999-10-06:
 *
 * - first version
 *
 * known bugs:
 *
 * todo:
 *
 */

# include "syscall_vectors.h"

# include "arch/sig_mach.h"
# include "arch/tosbind.h"

# include "bios.h"
# include "console.h"
# include "crypt_IO.h"
# include "dosdir.h"
# include "dos.h"
# include "dosfile.h"
# include "dosmem.h"
# include "dossig.h"
# include "filesys.h"
# include "ipc_socket.h"
# include "k_exec.h"
# include "k_exit.h"
# include "k_fork.h"
# include "k_prot.h"
# include "k_resource.h"
# include "k_sysctl.h"
# include "keyboard.h"
# include "memory.h"
# include "proc.h"
# include "ptrace.h"
# include "rendez.h"
# include "signal.h"
# include "ssystem.h"
# include "sys_emu.h"
# include "sysv_msg.h"
# include "sysv_sem.h"
# include "sysv_shm.h"
# include "time.h"
# include "timeout.h"
# include "slb.h"
# include "xbios.h"


/*
 * dummy function for reserved or not supported system calls
 */

static long _cdecl sys_enosys(void)
{
	return ENOSYS;
}


/****************************************************************************/
/* BEGIN DOS initialization */

/* special Opcodes:
 *
 * - 0x16 MagiC-like shared library scheme
 * - 0x17 MagiC-like shared library scheme
 * - 0x33 reserved for MagiC - LONG Sconfig(WORD subfn, LONG flags);
 */

/*
 * FIXME:
 * this should really be a structure instead of array of pointers,
 * so we can use correct prototypes instead of ugly casts
 */
dos_vecs dos_tab =
{
	/* 0x000 */		sys_pterm0,
	/* 0x001 */		sys_c_conin,
	/* 0x002 */		sys_c_conout,
	/* 0x003 */		sys_c_auxin,
	/* 0x004 */		sys_c_auxout,
	/* 0x005 */		sys_c_prnout,
	/* 0x006 */		sys_c_rawio,
	/* 0x007 */		sys_c_rawcin,
	/* 0x008 */		sys_c_necin,
	/* 0x009 */		sys_c_conws,
	/* 0x00a */		sys_c_conrs,
	/* 0x00b */		sys_c_conis,
	/* 0x00c */		NULL,
	/* 0x00d */		NULL,
	/* 0x00e */		sys_d_setdrv,
	/* 0x00f */		NULL,

	/* 0x010 */		sys_c_conos,
	/* 0x011 */		sys_c_prnos,
	/* 0x012 */		sys_c_auxis,
	/* 0x013 */		sys_c_auxos,
	/* 0x014 */		sys_m_addalt,
	/* 0x015 */		sys_s_realloc,
	/* 0x016 */		sys_s_lbopen,	/* 1.15.3, MagiC 5 */
	/* 0x017 */		sys_s_lbclose,	/* 1.15.3, MagiC 5 */
	/* 0x018 */		NULL,
	/* 0x019 */		sys_d_getdrv,
	/* 0x01a */		sys_f_setdta,
	/* 0x01b */		NULL,
	/* 0x01c */		NULL,
	/* 0x01d */		NULL,
	/* 0x01e */		NULL,
	/* 0x01f */		NULL,

	/* 0x020 */		sys_s_uper,
	/* 0x021 */		NULL,
	/* 0x022 */		NULL,
	/* 0x023 */		NULL,
	/* 0x024 */		NULL,
	/* 0x025 */		NULL,
	/* 0x026 */		NULL,
	/* 0x027 */		NULL,
	/* 0x028 */		NULL,
	/* 0x029 */		NULL,
	/* 0x02a */		sys_t_getdate,
	/* 0x02b */		sys_t_setdate,
	/* 0x02c */		sys_t_gettime,
	/* 0x02d */		sys_t_settime,
	/* 0x02e */		NULL,
	/* 0x02f */		sys_f_getdta,

	/* 0x030 */		sys_s_version,
	/* 0x031 */		sys_ptermres,
	/* 0x032 */		NULL,
	/* 0x033 */		sys_enosys,		/* MagiC: Sconfig() */
	/* 0x034 */		NULL,
	/* 0x035 */		NULL,
	/* 0x036 */		sys_d_free,
	/* 0x037 */		NULL,
	/* 0x038 */		NULL,
	/* 0x039 */		sys_d_create,
	/* 0x03a */		sys_d_delete,
	/* 0x03b */		sys_d_setpath,
	/* 0x03c */		sys_f_create,
	/* 0x03d */		sys_f_open,
	/* 0x03e */		sys_f_close,
	/* 0x03f */		sys_f_read,

	/* 0x040 */		sys_f_write,
	/* 0x041 */		sys_f_delete,
	/* 0x042 */		sys_f_seek,
	/* 0x043 */		sys_f_attrib,
	/* 0x044 */		sys_m_xalloc,
	/* 0x045 */		sys_f_dup,
	/* 0x046 */		sys_f_force,
	/* 0x047 */		sys_d_getpath,
	/* 0x048 */		sys_m_alloc,
	/* 0x049 */		sys_m_free,
	/* 0x04a */		sys_m_shrink,
	/* 0x04b */		sys_pexec,
	/* 0x04c */		sys_pterm,
	/* 0x04d */		NULL,
	/* 0x04e */		sys_f_sfirst,
	/* 0x04f */		sys_f_snext,

	/* 0x050 */		NULL,
	/* 0x051 */		NULL,
	/* 0x052 */		NULL,
	/* 0x053 */		NULL,
	/* 0x054 */		NULL,
	/* 0x055 */		NULL,
	/* 0x056 */		sys_f_rename,
	/* 0x057 */		sys_f_datime,
	/* 0x058 */		NULL,
	/* 0x059 */		NULL,
	/* 0x05a */		NULL,
	/* 0x05b */		NULL,
	/* 0x05c */		sys_f_lock,
	/* 0x05d */		NULL,
	/* 0x05e */		NULL,
	/* 0x05f */		NULL,

	/* 0x060 */		NULL,
	/* 0x061 */		NULL,
	/* 0x062 */		NULL,
	/* 0x063 */		NULL,
	/* 0x064 */		NULL,
	/* 0x065 */		NULL,
	/* 0x066 */		NULL,
	/* 0x067 */		NULL,
	/* 0x068 */		NULL,
	/* 0x069 */		NULL,
	/* 0x06a */		NULL,
	/* 0x06b */		NULL,
	/* 0x06c */		NULL,
	/* 0x06d */		NULL,
	/* 0x06e */		NULL,
	/* 0x06f */		NULL,

	/* 0x070 */		NULL,
	/* 0x071 */		NULL,
	/* 0x072 */		NULL,
	/* 0x073 */		NULL,
	/* 0x074 */		NULL,
	/* 0x075 */		NULL,
	/* 0x076 */		NULL,
	/* 0x077 */		NULL,
	/* 0x078 */		NULL,
	/* 0x079 */		NULL,
	/* 0x07a */		NULL,
	/* 0x07b */		NULL,
	/* 0x07c */		NULL,
	/* 0x07d */		NULL,
	/* 0x07e */		NULL,
	/* 0x07f */		NULL,

	/* 0x080 */		NULL,
	/* 0x081 */		NULL,
	/* 0x082 */		NULL,
	/* 0x083 */		NULL,
	/* 0x084 */		NULL,
	/* 0x085 */		NULL,
	/* 0x086 */		NULL,
	/* 0x087 */		NULL,
	/* 0x088 */		NULL,
	/* 0x089 */		NULL,
	/* 0x08a */		NULL,
	/* 0x08b */		NULL,
	/* 0x08c */		NULL,
	/* 0x08d */		NULL,
	/* 0x08e */		NULL,
	/* 0x08f */		NULL,

	/* 0x090 */		NULL,
	/* 0x091 */		NULL,
	/* 0x092 */		NULL,
	/* 0x093 */		NULL,
	/* 0x094 */		NULL,
	/* 0x095 */		NULL,
	/* 0x096 */		NULL,
	/* 0x097 */		NULL,
	/* 0x098 */		NULL,
	/* 0x099 */		NULL,
	/* 0x09a */		NULL,
	/* 0x09b */		NULL,
	/* 0x09c */		NULL,
	/* 0x09d */		NULL,
	/* 0x09e */		NULL,
	/* 0x09f */		NULL,

	/* 0x0a0 */		NULL,
	/* 0x0a1 */		NULL,
	/* 0x0a2 */		NULL,
	/* 0x0a3 */		NULL,
	/* 0x0a4 */		NULL,
	/* 0x0a5 */		NULL,
	/* 0x0a6 */		NULL,
	/* 0x0a7 */		NULL,
	/* 0x0a8 */		NULL,
	/* 0x0a9 */		NULL,
	/* 0x0aa */		NULL,
	/* 0x0ab */		NULL,
	/* 0x0ac */		NULL,
	/* 0x0ad */		NULL,
	/* 0x0ae */		NULL,
	/* 0x0af */		NULL,

	/* 0x0b0 */		NULL,
	/* 0x0b1 */		NULL,
	/* 0x0b2 */		NULL,
	/* 0x0b3 */		NULL,
	/* 0x0b4 */		NULL,
	/* 0x0b5 */		NULL,
	/* 0x0b6 */		NULL,
	/* 0x0b7 */		NULL,
	/* 0x0b8 */		NULL,
	/* 0x0b9 */		NULL,
	/* 0x0ba */		NULL,
	/* 0x0bb */		NULL,
	/* 0x0bc */		NULL,
	/* 0x0bd */		NULL,
	/* 0x0be */		NULL,
	/* 0x0bf */		NULL,

	/* 0x0c0 */		NULL,
	/* 0x0c1 */		NULL,
	/* 0x0c2 */		NULL,
	/* 0x0c3 */		NULL,
	/* 0x0c4 */		NULL,
	/* 0x0c5 */		NULL,
	/* 0x0c6 */		NULL,
	/* 0x0c7 */		NULL,
	/* 0x0c8 */		NULL,
	/* 0x0c9 */		NULL,
	/* 0x0ca */		NULL,
	/* 0x0cb */		NULL,
	/* 0x0cc */		NULL,
	/* 0x0cd */		NULL,
	/* 0x0ce */		NULL,
	/* 0x0cf */		NULL,

	/* 0x0d0 */		NULL,
	/* 0x0d1 */		NULL,
	/* 0x0d2 */		NULL,
	/* 0x0d3 */		NULL,
	/* 0x0d4 */		NULL,
	/* 0x0d5 */		NULL,
	/* 0x0d6 */		NULL,
	/* 0x0d7 */		NULL,
	/* 0x0d8 */		NULL,
	/* 0x0d9 */		NULL,
	/* 0x0da */		NULL,
	/* 0x0db */		NULL,
	/* 0x0dc */		NULL,
	/* 0x0dd */		NULL,
	/* 0x0de */		NULL,
	/* 0x0df */		NULL,

	/* 0x0e0 */		NULL,
	/* 0x0e1 */		NULL,
	/* 0x0e2 */		NULL,
	/* 0x0e3 */		NULL,
	/* 0x0e4 */		NULL,
	/* 0x0e5 */		NULL,
	/* 0x0e6 */		NULL,
	/* 0x0e7 */		NULL,
	/* 0x0e8 */		NULL,
	/* 0x0e9 */		NULL,
	/* 0x0ea */		NULL,
	/* 0x0eb */		NULL,
	/* 0x0ec */		NULL,
	/* 0x0ed */		NULL,
	/* 0x0ee */		NULL,
	/* 0x0ef */		NULL,

	/* 0x0f0 */		NULL,
	/* 0x0f1 */		NULL,
	/* 0x0f2 */		NULL,
	/* 0x0f3 */		NULL,
	/* 0x0f4 */		NULL,
	/* 0x0f5 */		NULL,
	/* 0x0f6 */		NULL,
	/* 0x0f7 */		NULL,
	/* 0x0f8 */		NULL,
	/* 0x0f9 */		NULL,
	/* 0x0fa */		NULL,
	/* 0x0fb */		NULL,
	/* 0x0fc */		NULL,
	/* 0x0fd */		NULL,
	/* 0x0fe */		NULL,

	/* MiNT extensions to GEMDOS
	 */

	/* 0x0ff */		sys_s_yield,

	/* 0x100 */		sys_pipe,
	/* 0x101 */		sys_f_fchown,	/* 1.15.2  */
	/* 0x102 */		sys_f_fchmod,	/* 1.15.2  */
	/* 0x103 */		sys_fsync, 		/* 1.15.10 */
	/* 0x104 */		sys_f_cntl,
	/* 0x105 */		sys_f_instat,
	/* 0x106 */		sys_f_outstat,
	/* 0x107 */		sys_f_getchar,
	/* 0x108 */		sys_f_putchar,
	/* 0x109 */		sys_pwait,
	/* 0x10a */		sys_pnice,
	/* 0x10b */		sys_p_getpid,
	/* 0x10c */		sys_p_getppid,
	/* 0x10d */		sys_p_getpgrp,
	/* 0x10e */		sys_p_setpgrp,
	/* 0x10f */		sys_pgetuid,

	/* 0x110 */		sys_psetuid,
	/* 0x111 */		sys_p_kill,
	/* 0x112 */		sys_p_signal,
	/* 0x113 */		sys_pvfork,
	/* 0x114 */		sys_pgetgid,
	/* 0x115 */		sys_psetgid,
	/* 0x116 */		sys_p_sigblock,
	/* 0x117 */		sys_p_sigsetmask,
	/* 0x118 */		sys_p_usrval,
	/* 0x119 */		sys_p_domain,
	/* 0x11a */		sys_psigreturn,
	/* 0x11b */		sys_pfork,
	/* 0x11c */		sys_pwait3,
	/* 0x11d */		sys_f_select,
	/* 0x11e */		sys_prusage,
	/* 0x11f */		sys_psetlimit,

	/* 0x120 */		sys_t_alarm,
	/* 0x121 */		sys_p_pause,
	/* 0x122 */		sys_s_ysconf,
	/* 0x123 */		sys_p_sigpending,
	/* 0x124 */		sys_d_pathconf,
	/* 0x125 */		sys_p_msg,
	/* 0x126 */		sys_f_midipipe,
	/* 0x127 */		sys_prenice,
	/* 0x128 */		sys_d_opendir,
	/* 0x129 */		sys_d_readdir,
	/* 0x12a */		sys_d_rewind,
	/* 0x12b */		sys_d_closedir,
	/* 0x12c */		sys_f_xattr,
	/* 0x12d */		sys_f_link,
	/* 0x12e */		sys_f_symlink,
	/* 0x12f */		sys_f_readlink,

	/* 0x130 */		sys_d_cntl,
	/* 0x131 */		sys_f_chown,
	/* 0x132 */		sys_f_chmod,
	/* 0x133 */		sys_p_umask,
	/* 0x134 */		sys_p_semaphore,
	/* 0x135 */		sys_d_lock,
	/* 0x136 */		sys_p_sigpause,
	/* 0x137 */		sys_p_sigaction,
	/* 0x138 */		sys_pgeteuid,
	/* 0x139 */		sys_pgetegid,
	/* 0x13a */		sys_pwaitpid,
	/* 0x13b */		sys_d_getcwd,
	/* 0x13c */		sys_s_alert,
	/* 0x13d */		sys_t_malarm,
	/* 0x13e */		sys_enosys,		/* p_sigintr(), disabled as of 1.16.0 */
	/* 0x13f */		sys_s_uptime,

	/* 0x140 */		sys_p_trace,	/* 1.15.11 */
	/* 0x141 */		sys_m_validate,	/* 1.15.11 */
	/* 0x142 */		sys_d_xreaddir,
	/* 0x143 */		sys_pseteuid,
	/* 0x144 */		sys_psetegid,
	/* 0x145 */		sys_p_getauid,
	/* 0x146 */		sys_p_setauid,
	/* 0x147 */		sys_pgetgroups,
	/* 0x148 */		sys_psetgroups,
	/* 0x149 */		sys_t_setitimer,
	/* 0x14a */		sys_d_chroot,	/* 1.15.3  */
	/* 0x14b */		sys_f_stat64,	/* 1.15.4  */
	/* 0x14c */		sys_f_seek64,	/* 1.15.10 */
	/* 0x14d */		sys_d_setkey,	/* 1.15.4  */
	/* 0x14e */		sys_psetreuid,
	/* 0x14f */		sys_psetregid,

	/* 0x150 */		sys_s_ync,
	/* 0x151 */		sys_s_hutdown,
	/* 0x152 */		sys_d_readlabel,
	/* 0x153 */		sys_d_writelabel,
	/* 0x154 */		sys_s_system,
	/* 0x155 */		sys_t_gettimeofday,
	/* 0x156 */		sys_t_settimeofday,
	/* 0x157 */		sys_t_adjtime,		/* 1.16 */
	/* 0x158 */		sys_pgetpriority,
	/* 0x159 */		sys_psetpriority,
	/* 0x15a */		sys_f_poll,		/* 1.15.10 */
	/* 0x15b */		sys_fwritev,	/* 1.16 */
	/* 0x15c */		sys_freadv,	/* 1.16 */
	/* 0x15d */		sys_ffstat,	/* 1.16 */
	/* 0x15e */		sys_p_sysctl,	/* 1.15.11 */
	/* 0x15f */		sys_emu,	/* 1.15.8, interface emulation */

	/* 0x160 */		sys_socket,	/* 1.16 */
	/* 0x161 */		sys_socketpair,	/* 1.16 */
	/* 0x162 */		sys_accept,	/* 1.16 */
	/* 0x163 */		sys_connect,	/* 1.16 */
	/* 0x164 */		sys_bind,	/* 1.16 */
	/* 0x165 */		sys_listen,	/* 1.16 */
	/* 0x166 */		sys_recvmsg,	/* 1.16 */
	/* 0x167 */		sys_sendmsg,	/* 1.16 */
	/* 0x168 */		sys_recvfrom,	/* 1.16 */
	/* 0x169 */		sys_sendto,	/* 1.16 */
	/* 0x16a */		sys_setsockopt,	/* 1.16 */
	/* 0x16b */		sys_getsockopt,	/* 1.16 */
	/* 0x16c */		sys_getpeername,/* 1.16 */
	/* 0x16d */		sys_getsockname,/* 1.16 */
	/* 0x16e */		sys_shutdown,	/* 1.16 */
	/* 0x16f */		sys_enosys,		/* reserved */

	/* 0x170 */		sys_p_shmget,
	/* 0x171 */		sys_p_shmctl,
	/* 0x172 */		sys_p_shmat,
	/* 0x173 */		sys_p_shmdt,
	/* 0x174 */		sys_p_semget,	/* not implemented */
	/* 0x175 */		sys_p_semctl,	/* not implemented */
	/* 0x176 */		sys_p_semop,	/* not implemented */
	/* 0x177 */		sys_p_semconfig,/* not implemented */
	/* 0x178 */		sys_p_msgget,	/* not implemented */
	/* 0x179 */		sys_p_msgctl,	/* not implemented */
	/* 0x17a */		sys_p_msgsnd,	/* not implemented */
	/* 0x17b */		sys_p_msgrcv,	/* not implemented */
	/* 0x17c */		sys_enosys,		/* reserved */
	/* 0x17d */		sys_m_access,	/* 1.15.12 */
	/* 0x17e */		sys_enosys,		/* sys_mmap */
	/* 0x17f */		sys_enosys,		/* sys_munmap */

	/* 0x180 */		sys_f_chown16,	/* 1.16 */
	/* 0x181 */		sys_f_chdir,	/* 1.17 */
	/* 0x182 */		sys_f_opendir,	/* 1.17 */
	/* 0x183 */		sys_f_dirfd,	/* 1.17 */
	/* 0x184 */		sys_enosys,		/* reserved */
	/* 0x185 */		sys_enosys,		/* reserved */
	/* 0x186 */		sys_enosys,		/* reserved */
	/* 0x187 */		sys_enosys,		/* reserved */
	/* 0x188 */		sys_enosys,		/* reserved */
	/* 0x189 */		sys_enosys,		/* reserved */
	/* 0x18a */		sys_enosys,		/* reserved */
	/* 0x18b */		sys_enosys,		/* reserved */
	/* 0x18c */		sys_enosys,		/* reserved */
	/* 0x18d */		sys_enosys,		/* reserved */
	/* 0x18e */		sys_enosys,		/* reserved */
	/* 0x18f */		sys_enosys,		/* reserved */
};
/* if you change the size of this table, also change DOS_MAX in syscall.S */

/* END DOS initialization */
/****************************************************************************/

/****************************************************************************/
/* BEGIN BIOS initialization */

bios_vecs bios_tab =
{
	/* 0x000 */		(void _cdecl (*)(struct mpb *ptr))sys_enosys,		/* getmpb */
	/* 0x001 */		sys_b_ubconstat,
	/* 0x002 */		sys_b_ubconin,
	/* 0x003 */		sys_b_ubconout,
	/* 0x004 */		sys_b_rwabs,
	/* 0x005 */		sys_b_setexc,
	/* 0x006 */		sys_b_tickcal,
	/* 0x007 */		sys_b_getbpb,
	/* 0x008 */		sys_b_ubcostat,
	/* 0x009 */		sys_b_mediach,
	/* 0x00a */		sys_b_drvmap,
	/* 0x00b */		sys_b_kbshift,
	/* 0x00c */		NULL,
	/* 0x00d */		NULL,
	/* 0x00e */		NULL,
	/* 0x00f */		NULL,

	/* 0x010 */		NULL,
	/* 0x011 */		NULL,
	/* 0x012 */		NULL,
	/* 0x013 */		NULL,
	/* 0x014 */		NULL,
	/* 0x015 */		NULL,
	/* 0x016 */		NULL,
	/* 0x017 */		NULL,
	/* 0x018 */		NULL,
	/* 0x019 */		NULL,
	/* 0x01a */		NULL,
	/* 0x01b */		NULL,
	/* 0x01c */		NULL,
	/* 0x01d */		NULL,
	/* 0x01e */		NULL,
	/* 0x01f */		NULL,
};
/* if you change the size of this table, also change BIOS_MAX in syscall.S */

/* END BIOS initialization */
/****************************************************************************/

/****************************************************************************/
/* BEGIN XBIOS initialization */

xbios_vecs xbios_tab =
{
	/* 0x000 */		NULL,
	/* 0x001 */		NULL,
	/* 0x002 */		NULL,
	/* 0x003 */		NULL,
	/* 0x004 */		NULL,
	/* 0x005 */		sys_b_vsetscreen,
	/* 0x006 */		NULL,
	/* 0x007 */		NULL,
	/* 0x008 */		NULL,
	/* 0x009 */		NULL,
	/* 0x00a */		NULL,
	/* 0x00b */		NULL,
	/* 0x00c */		sys_b_midiws,
	/* 0x00d */		NULL,
	/* 0x00e */		sys_b_uiorec,
	/* 0x00f */		sys_b_ursconf,

	/* 0x010 */		sys_b_keytbl,
	/* 0x011 */		sys_b_random,
	/* 0x012 */		NULL,
	/* 0x013 */		NULL,
	/* 0x014 */		NULL,
	/* 0x015 */		sys_b_cursconf,
	/* 0x016 */		sys_b_settime,
	/* 0x017 */		sys_b_gettime,
	/* 0x018 */		sys_b_bioskeys,
	/* 0x019 */		NULL,
	/* 0x01a */		NULL,
	/* 0x01b */		NULL,
	/* 0x01c */		NULL,
	/* 0x01d */		NULL,
	/* 0x01e */		NULL,
	/* 0x01f */		NULL,

	/* 0x020 */		sys_b_dosound,
	/* 0x021 */		NULL,
	/* 0x022 */		sys_b_kbdvbase,
	/* 0x023 */		sys_b_kbrate,
	/* 0x024 */		NULL,
	/* 0x025 */		NULL,
	/* 0x026 */		sys_b_supexec,
	/* 0x027 */		NULL,
	/* 0x028 */		NULL,
	/* 0x029 */		NULL,
	/* 0x02a */		NULL,
	/* 0x02b */		NULL,
	/* 0x02c */		sys_b_bconmap,
	/* 0x02d */		NULL,
	/* 0x02e */		NULL,
	/* 0x02f */		NULL,

	/* 0x030 */		NULL,
	/* 0x031 */		NULL,
	/* 0x032 */		NULL,
	/* 0x033 */		NULL,
	/* 0x034 */		NULL,
	/* 0x035 */		NULL,
	/* 0x036 */		NULL,
	/* 0x037 */		NULL,
	/* 0x038 */		NULL,
	/* 0x039 */		NULL,
	/* 0x03a */		NULL,
	/* 0x03b */		NULL,
	/* 0x03c */		NULL,
	/* 0x03d */		NULL,
	/* 0x03e */		NULL,
	/* 0x03f */		NULL,

	/* 0x040 */		NULL,
	/* 0x041 */		NULL,
	/* 0x042 */		NULL,
	/* 0x043 */		NULL,
	/* 0x044 */		NULL,
	/* 0x045 */		NULL,
	/* 0x046 */		NULL,
	/* 0x047 */		NULL,
	/* 0x048 */		NULL,
	/* 0x049 */		NULL,
	/* 0x04a */		NULL,
	/* 0x04b */		NULL,
	/* 0x04c */		NULL,
	/* 0x04d */		NULL,
	/* 0x04e */		NULL,
	/* 0x04f */		NULL,

	/* 0x050 */		NULL,
	/* 0x051 */		NULL,
	/* 0x052 */		NULL,
	/* 0x053 */		NULL,
	/* 0x054 */		NULL,
	/* 0x055 */		NULL,
	/* 0x056 */		NULL,
	/* 0x057 */		NULL,
	/* 0x058 */		NULL,
	/* 0x059 */		NULL,
	/* 0x05a */		NULL,
	/* 0x05b */		NULL,
	/* 0x05c */		NULL,
	/* 0x05d */		NULL,
	/* 0x05e */		NULL,
	/* 0x05f */		NULL,

	/* 0x060 */		NULL,
	/* 0x061 */		NULL,
	/* 0x062 */		NULL,
	/* 0x063 */		NULL,
	/* 0x064 */		NULL,
	/* 0x065 */		NULL,
	/* 0x066 */		NULL,
	/* 0x067 */		NULL,
	/* 0x068 */		NULL,
	/* 0x069 */		NULL,
	/* 0x06a */		NULL,
	/* 0x06b */		NULL,
	/* 0x06c */		NULL,
	/* 0x06d */		NULL,
	/* 0x06e */		NULL,
	/* 0x06f */		NULL,

	/* 0x070 */		NULL,
	/* 0x071 */		NULL,
	/* 0x072 */		NULL,
	/* 0x073 */		NULL,
	/* 0x074 */		NULL,
	/* 0x075 */		NULL,
	/* 0x076 */		NULL,
	/* 0x077 */		NULL,
	/* 0x078 */		NULL,
	/* 0x079 */		NULL,
	/* 0x07a */		NULL,
	/* 0x07b */		NULL,
	/* 0x07c */		NULL,
	/* 0x07d */		NULL,
	/* 0x07e */		NULL,
	/* 0x07f */		NULL
};
/* if you change the size of this table, also change XBIOS_MAX in syscall.S */

/* END XBIOS initialization */
/****************************************************************************/
