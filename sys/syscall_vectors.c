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
 * begin:	1999-10-06
 * last change: 1999-10-06
 * 
 * Author: Frank Naumann <fnaumann@fremint.de>
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

# include "bios.h"
# include "console.h"
# include "crypt_IO.h"
# include "dosdir.h"
# include "dos.h"
# include "dosfile.h"
# include "dosmem.h"
# include "dossig.h"
# include "filesys.h"
# include "k_sysctl.h"
# include "memory.h"
# include "proc.h"
# include "ptrace.h"
# include "rendez.h"
# include "resource.h"
# include "signal.h"
# include "ssystem.h"
# include "sys_emu.h"
# include "time.h"
# include "timeout.h"
# include "slb.h"
# include "xbios.h"

# include <osbind.h>


/*
 * dummy function for reserved or not supported system calls
 */

static long _cdecl
enosys (void)
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

# define DOS_MAX 0x160
ushort dos_max = DOS_MAX;

Func dos_tab [DOS_MAX] =
{
        /* 0x000 */		p_term0,
	/* 0x001 */		c_conin,
	/* 0x002 */		c_conout,
	/* 0x003 */		c_auxin,
	/* 0x004 */		c_auxout,
	/* 0x005 */		c_prnout,
	/* 0x006 */		c_rawio,
	/* 0x007 */		c_rawcin,
	/* 0x008 */		c_necin,
	/* 0x009 */		c_conws,
	/* 0x00a */		c_conrs,
	/* 0x00b */		c_conis,
	/* 0x00c */		NULL,
	/* 0x00d */		NULL,
	/* 0x00e */		d_setdrv,
	/* 0x00f */		NULL,
	
	/* 0x010 */		c_conos,
	/* 0x011 */		c_prnos,
	/* 0x012 */		c_auxis,
	/* 0x013 */		c_auxos,
	/* 0x014 */		m_addalt,
	/* 0x015 */		s_realloc,
	/* 0x016 */		s_lbopen,	/* 1.15.3, MagiC 5 */
	/* 0x017 */		s_lbclose,	/* 1.15.3, MagiC 5 */
	/* 0x018 */		NULL,
	/* 0x019 */		d_getdrv,
	/* 0x01a */		f_setdta,
	/* 0x01b */		NULL,
	/* 0x01c */		NULL,
	/* 0x01d */		NULL,
	/* 0x01e */		NULL,
	/* 0x01f */		NULL,
	
	/* 0x020 */		s_uper,
	/* 0x021 */		NULL,
	/* 0x022 */		NULL,
	/* 0x023 */		NULL,
	/* 0x024 */		NULL,
	/* 0x025 */		NULL,
	/* 0x026 */		NULL,
	/* 0x027 */		NULL,
	/* 0x028 */		NULL,
	/* 0x029 */		NULL,
	/* 0x02a */		t_getdate,
	/* 0x02b */	(Func)	t_setdate,
	/* 0x02c */		t_gettime,
	/* 0x02d */	(Func)	t_settime,
	/* 0x02e */		NULL,
	/* 0x02f */		f_getdta,
	
	/* 0x030 */		s_version,
	/* 0x031 */		p_termres,
	/* 0x032 */		NULL,
	/* 0x033 */		enosys,		/* MagiC: Sconfig() */
	/* 0x034 */		NULL,
	/* 0x035 */		NULL,
	/* 0x036 */		d_free,
	/* 0x037 */		NULL,
	/* 0x038 */		NULL,
	/* 0x039 */		d_create,
	/* 0x03a */		d_delete,
	/* 0x03b */		d_setpath,
	/* 0x03c */		f_create,
	/* 0x03d */		f_open,
	/* 0x03e */		f_close,
	/* 0x03f */		f_read,
	
	/* 0x040 */		f_write,
	/* 0x041 */		f_delete,
	/* 0x042 */		f_seek,
	/* 0x043 */		f_attrib,
	/* 0x044 */		m_xalloc,
	/* 0x045 */		f_dup,
	/* 0x046 */		f_force,
	/* 0x047 */		d_getpath,
	/* 0x048 */		m_alloc,
	/* 0x049 */		m_free,
	/* 0x04a */		m_shrink,
	/* 0x04b */		p_exec,
	/* 0x04c */		p_term,
	/* 0x04d */		NULL,
	/* 0x04e */		f_sfirst,
	/* 0x04f */		f_snext,
	
	/* 0x050 */		NULL,
	/* 0x051 */		NULL,
	/* 0x052 */		NULL,
	/* 0x053 */		NULL,
	/* 0x054 */		NULL,
	/* 0x055 */		NULL,
	/* 0x056 */		f_rename,
	/* 0x057 */		f_datime,
	/* 0x058 */		NULL,
	/* 0x059 */		NULL,
	/* 0x05a */		NULL,
	/* 0x05b */		NULL,
	/* 0x05c */		f_lock,
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
	
	/*
	 * MiNT extensions to GEMDOS
	 */
	
	/* 0x0ff */		s_yield,
	
	/* 0x100 */		f_pipe,
	/* 0x101 */		f_fchown,	/* 1.15.2  */
	/* 0x102 */		f_fchmod,	/* 1.15.2  */
	/* 0x103 */		sys_fsync, 	/* 1.15.10 */
	/* 0x104 */		f_cntl,
	/* 0x105 */		f_instat,
	/* 0x106 */		f_outstat,
	/* 0x107 */		f_getchar,
	/* 0x108 */		f_putchar,
	/* 0x109 */		p_wait,
	/* 0x10a */		p_nice,
	/* 0x10b */		p_getpid,
	/* 0x10c */		p_getppid,
	/* 0x10d */		p_getpgrp,
	/* 0x10e */		p_setpgrp,
	/* 0x10f */		p_getuid,
	
	/* 0x110 */		p_setuid,
	/* 0x111 */		p_kill,
	/* 0x112 */		p_signal,
	/* 0x113 */		p_vfork,
	/* 0x114 */		p_getgid,
	/* 0x115 */		p_setgid,
	/* 0x116 */		p_sigblock,
	/* 0x117 */		p_sigsetmask,
	/* 0x118 */		p_usrval,
	/* 0x119 */		p_domain,
	/* 0x11a */		p_sigreturn,
	/* 0x11b */		p_fork,
	/* 0x11c */		p_wait3,
	/* 0x11d */		f_select,
	/* 0x11e */		p_rusage,
	/* 0x11f */		p_setlimit,
	
	/* 0x120 */		t_alarm,
	/* 0x121 */		p_pause,
	/* 0x122 */		s_ysconf,
	/* 0x123 */		p_sigpending,
	/* 0x124 */		d_pathconf,
	/* 0x125 */		p_msg,
	/* 0x126 */		f_midipipe,
	/* 0x127 */		p_renice,
	/* 0x128 */		d_opendir,
	/* 0x129 */		d_readdir,
	/* 0x12a */		d_rewind,
	/* 0x12b */		d_closedir,
	/* 0x12c */		f_xattr,
	/* 0x12d */		f_link,
	/* 0x12e */		f_symlink,
	/* 0x12f */		f_readlink,
	
	/* 0x130 */		d_cntl,
	/* 0x131 */		f_chown,
	/* 0x132 */		f_chmod,
	/* 0x133 */		p_umask,
	/* 0x134 */		p_semaphore,
	/* 0x135 */		d_lock,
	/* 0x136 */		p_sigpause,
	/* 0x137 */		p_sigaction,
	/* 0x138 */		p_geteuid,
	/* 0x139 */		p_getegid,
	/* 0x13a */		p_waitpid,
	/* 0x13b */		d_getcwd,
	/* 0x13c */		s_alert,
	/* 0x13d */		t_malarm,
	/* 0x13e */	(Func)	p_sigintr,
	/* 0x13f */		s_uptime,
	
	/* 0x140 */	(Func)	p_trace,	/* 1.15.11 */
	/* 0x141 */		m_validate,	/* 1.15.11 */
	/* 0x142 */		d_xreaddir,
	/* 0x143 */		p_seteuid,
	/* 0x144 */		p_setegid,
	/* 0x145 */		p_getauid,
	/* 0x146 */		p_setauid,
	/* 0x147 */		p_getgroups,
	/* 0x148 */		p_setgroups,
	/* 0x149 */		t_setitimer,
	/* 0x14a */		d_chroot,	/* 1.15.3  */
	/* 0x14b */		f_stat64,	/* 1.15.4  */
	/* 0x14c */		f_seek64,	/* 1.15.10 */
	/* 0x14d */		d_setkey,	/* 1.15.4  */
 	/* 0x14e */		p_setreuid,
 	/* 0x14f */		p_setregid,
 	
	/* 0x150 */		s_ync,
	/* 0x151 */		s_hutdown,
	/* 0x152 */		d_readlabel,
	/* 0x153 */		d_writelabel,
	/* 0x154 */		s_system,
	/* 0x155 */		t_gettimeofday,
	/* 0x156 */		t_settimeofday,
	/* 0x157 */		enosys,		/* t_adjtime */
	/* 0x158 */		p_getpriority,
	/* 0x159 */		p_setpriority,
	/* 0x15a */		f_poll,		/* 1.15.10 */
	/* 0x15b */		enosys,		/* reserved */
	/* 0x15c */		enosys,		/* reserved */
	/* 0x15d */		enosys,		/* reserved */
	/* 0x15e */	(Func)	sys_p_sysctl,	/* 1.15.11 */
	/* 0x15f */	(Func)	sys_emu		/* 1.15.8, interface emulation */
	
	/* 0x160 */		/* DOS_MAX */
};

Func sys_mon_tab [1] =
{
	/* 0x1069 */	(Func)	Debug
};

void
init_dos (void)
{
	/* miscellaneous initialization goes here */
}

/* END DOS initialization */
/****************************************************************************/

/****************************************************************************/
/* BEGIN BIOS initialization */

# define BIOS_MAX 0x20
ushort bios_max = BIOS_MAX;

Func bios_tab [BIOS_MAX] =
{
        /* 0x000 */		enosys,		/* getmpb */
	/* 0x001 */		ubconstat,
	/* 0x002 */		ubconin,
	/* 0x003 */		ubconout,
	/* 0x004 */		rwabs,
	/* 0x005 */		setexc,
	/* 0x006 */		tickcal,
	/* 0x007 */		getbpb,
	/* 0x008 */		ubcostat,
	/* 0x009 */		mediach,
	/* 0x00a */		drvmap,
	/* 0x00b */		kbshift,
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
	
	/* 0x020 */		/* BIOS_MAX */
};

/* BIOS initialization routine: gets keyboard buffer pointers, for the
 * interrupt routine below
 */

void
init_bios (void)
{
	keyrec = (IOREC_T *) Iorec (1);
	
	init_bdevmap ();
}

/* END BIOS initialization */
/****************************************************************************/

/****************************************************************************/
/* BEGIN XBIOS initialization */

# define XBIOS_MAX 0x80
ushort xbios_max = XBIOS_MAX;

Func xbios_tab [XBIOS_MAX] =
{
        /* 0x000 */		NULL,
	/* 0x001 */		NULL,
	/* 0x002 */		NULL,
	/* 0x003 */		NULL,
	/* 0x004 */		NULL,
	/* 0x005 */		vsetscreen,
	/* 0x006 */		NULL,
	/* 0x007 */		NULL,
	/* 0x008 */		NULL,
	/* 0x009 */		NULL,
	/* 0x00a */		NULL,
	/* 0x00b */		NULL,
	/* 0x00c */		midiws,
	/* 0x00d */		NULL,
	/* 0x00e */		uiorec,
	/* 0x00f */		ursconf,
	
	/* 0x010 */		NULL,
	/* 0x011 */		random,
	/* 0x012 */		NULL,
	/* 0x013 */		NULL,
	/* 0x014 */		NULL,
	/* 0x015 */		cursconf,
	/* 0x016 */	(Func)	settime,
	/* 0x017 */		gettime,
	/* 0x018 */		NULL,
	/* 0x019 */		NULL,
	/* 0x01a */		NULL,
	/* 0x01b */		NULL,
	/* 0x01c */		NULL,
	/* 0x01d */		NULL,
	/* 0x01e */		NULL,
	/* 0x01f */		NULL,
	
	/* 0x020 */		dosound,
	/* 0x021 */		NULL,
	/* 0x022 */		NULL,
	/* 0x023 */		NULL,
	/* 0x024 */		NULL,
	/* 0x025 */		NULL,
	/* 0x026 */		supexec,
	/* 0x027 */		NULL,
	/* 0x028 */		NULL,
	/* 0x029 */		NULL,
	/* 0x02a */		NULL,
	/* 0x02b */		NULL,
	/* 0x02c */		bconmap,
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
	
	/* 0x080 */		/* XBIOS_MAX */
};

void
init_xbios (void)
{
	/* init XBIOS Random() function */
	init_xrandom ();
	
	/* init bconmap stuff */
	init_bconmap ();
}

/* END XBIOS initialization */
/****************************************************************************/
