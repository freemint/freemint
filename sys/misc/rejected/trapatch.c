/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes* for a detailed log of changes.
 */

/*
 * begin:	1999-06
 * last change: 1999-07-23
 * 
 * Author: J”rg Westheide - <joerg_westheide@su.maus.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include "trapatch.h"
# include "stubs.h"

# include "kmemory.h"


/*
 * This is the table of DOS functions used by the TraPatch based trap handler.
 * 
 * NOTE: This table is different from dos_tab defined in dos.c and 
 *       it's content is _not_ _compatible_ with dos_tab's!
 * 
 * To add new functions, replace 'unused' with the stub for new routine or
 * append it to the table and increase DOS_MAX.
 * NOTE: Do not forget to add your function to the kernel interface
 *       (dos_tab in dos.c)
 */


/* all unused entries do a jump towards the rom
 */

extern long _cdecl jump_to_rom_gemdos (void);

# define unused jump_to_rom_gemdos

 
Func dos_table [DOS_MAX] =
{
        /* 0x000 */		stub_p_term0,
	/* 0x001 */		stub_c_conin,
	/* 0x002 */		stub_c_conout,
	/* 0x003 */		stub_c_auxin,
	/* 0x004 */		stub_c_auxout,
	/* 0x005 */		stub_c_prnout,
	/* 0x006 */		stub_c_rawio,
	/* 0x007 */		stub_c_rawcin,
	/* 0x008 */		stub_c_necin,
	/* 0x009 */		stub_c_conws,
	/* 0x00a */		stub_c_conrs,
	/* 0x00b */		stub_c_conis,
	/* 0x00c */		unused,
	/* 0x00d */		unused,
	/* 0x00e */		stub_d_setdrv,
	/* 0x00f */		unused,
	
	/* 0x010 */		stub_c_conos,
	/* 0x011 */		stub_c_prnos,
	/* 0x012 */		stub_c_auxis,
	/* 0x013 */		stub_c_auxos,
	/* 0x014 */		stub_m_addalt,
	/* 0x015 */		stub_s_realloc,
	/* 0x016 */		stub_s_lbopen,		/* 1.15.3 MagiC slb */
	/* 0x017 */		stub_s_lbclose,		/* 1.15.3 MagiC slb */
	/* 0x018 */		unused,
	/* 0x019 */		stub_d_getdrv,
	/* 0x01a */		stub_f_setdta,
	/* 0x01b */		unused,
	/* 0x01c */		unused,
	/* 0x01d */		unused,
	/* 0x01e */		unused,
	/* 0x01f */		unused,
	
	/* 0x020 */		stub_s_uper,
	/* 0x021 */		unused,
	/* 0x022 */		unused,
	/* 0x023 */		unused,
	/* 0x024 */		unused,
	/* 0x025 */		unused,
	/* 0x026 */		unused,
	/* 0x027 */		unused,
	/* 0x028 */		unused,
	/* 0x029 */		unused,
	/* 0x02a */		stub_t_getdate,
	/* 0x02b */	(Func)	stub_t_setdate,
	/* 0x02c */		stub_t_gettime,
	/* 0x02d */	(Func)	stub_t_settime,
	/* 0x02e */		unused,
	/* 0x02f */		stub_f_getdta,
	
	/* 0x030 */		stub_s_version,
	/* 0x031 */		stub_p_termres,
	/* 0x032 */		unused,
	/* 0x033 */		unused,			/* MagiC's  LONG Sconfig(WORD subfn, LONG flags); */
	/* 0x034 */		unused,
	/* 0x035 */		unused,
	/* 0x036 */		stub_d_free,
	/* 0x037 */		unused,
	/* 0x038 */		unused,
	/* 0x039 */		stub_d_create,
	/* 0x03a */		stub_d_delete,
	/* 0x03b */		stub_d_setpath,
	/* 0x03c */		stub_f_create,
	/* 0x03d */		stub_f_open,
	/* 0x03e */		stub_f_close,
	/* 0x03f */		stub_f_read,
	
	/* 0x040 */		stub_f_write,
	/* 0x041 */		stub_f_delete,
	/* 0x042 */		stub_f_seek,
	/* 0x043 */		stub_f_attrib,
	/* 0x044 */		stub_m_xalloc,
	/* 0x045 */		stub_f_dup,
	/* 0x046 */		stub_f_force,
	/* 0x047 */		stub_d_getpath,
	/* 0x048 */		stub_m_alloc,
	/* 0x049 */		stub_m_free,
	/* 0x04a */		stub_m_shrink,
	/* 0x04b */		stub_p_exec,
	/* 0x04c */		stub_p_term,
	/* 0x04d */		unused,
	/* 0x04e */		stub_f_sfirst,
	/* 0x04f */		stub_f_snext,
	
	/* 0x050 */		unused,
	/* 0x051 */		unused,
	/* 0x052 */		unused,
	/* 0x053 */		unused,
	/* 0x054 */		unused,
	/* 0x055 */		unused,
	/* 0x056 */		stub_f_rename,
	/* 0x057 */		stub_f_datime,
	/* 0x058 */		unused,
	/* 0x059 */		unused,
	/* 0x05a */		unused,
	/* 0x05b */		unused,
	/* 0x05c */		stub_f_lock,
	/* 0x05d */		unused,
	/* 0x05e */		unused,
	/* 0x05f */		unused,
	
	/* 0x060 */		unused,
	/* 0x061 */		unused,
	/* 0x062 */		unused,
	/* 0x063 */		unused,
	/* 0x064 */		unused,
	/* 0x065 */		unused,
	/* 0x066 */		unused,
	/* 0x067 */		unused,
	/* 0x068 */		unused,
	/* 0x069 */		unused,
	/* 0x06a */		unused,
	/* 0x06b */		unused,
	/* 0x06c */		unused,
	/* 0x06d */		unused,
	/* 0x06e */		unused,
	/* 0x06f */		unused,
	
	/* 0x070 */		unused,
	/* 0x071 */		unused,
	/* 0x072 */		unused,
	/* 0x073 */		unused,
	/* 0x074 */		unused,
	/* 0x075 */		unused,
	/* 0x076 */		unused,
	/* 0x077 */		unused,
	/* 0x078 */		unused,
	/* 0x079 */		unused,
	/* 0x07a */		unused,
	/* 0x07b */		unused,
	/* 0x07c */		unused,
	/* 0x07d */		unused,
	/* 0x07e */		unused,
	/* 0x07f */		unused,
	
	/* 0x080 */		unused,
	/* 0x081 */		unused,
	/* 0x082 */		unused,
	/* 0x083 */		unused,
	/* 0x084 */		unused,
	/* 0x085 */		unused,
	/* 0x086 */		unused,
	/* 0x087 */		unused,
	/* 0x088 */		unused,
	/* 0x089 */		unused,
	/* 0x08a */		unused,
	/* 0x08b */		unused,
	/* 0x08c */		unused,
	/* 0x08d */		unused,
	/* 0x08e */		unused,
	/* 0x08f */		unused,
	
	/* 0x090 */		unused,
	/* 0x091 */		unused,
	/* 0x092 */		unused,
	/* 0x093 */		unused,
	/* 0x094 */		unused,
	/* 0x095 */		unused,
	/* 0x096 */		unused,
	/* 0x097 */		unused,
	/* 0x098 */		unused,
	/* 0x099 */		unused,
	/* 0x09a */		unused,
	/* 0x09b */		unused,
	/* 0x09c */		unused,
	/* 0x09d */		unused,
	/* 0x09e */		unused,
	/* 0x09f */		unused,
	
	/* 0x0a0 */		unused,
	/* 0x0a1 */		unused,
	/* 0x0a2 */		unused,
	/* 0x0a3 */		unused,
	/* 0x0a4 */		unused,
	/* 0x0a5 */		unused,
	/* 0x0a6 */		unused,
	/* 0x0a7 */		unused,
	/* 0x0a8 */		unused,
	/* 0x0a9 */		unused,
	/* 0x0aa */		unused,
	/* 0x0ab */		unused,
	/* 0x0ac */		unused,
	/* 0x0ad */		unused,
	/* 0x0ae */		unused,
	/* 0x0af */		unused,
	
	/* 0x0b0 */		unused,
	/* 0x0b1 */		unused,
	/* 0x0b2 */		unused,
	/* 0x0b3 */		unused,
	/* 0x0b4 */		unused,
	/* 0x0b5 */		unused,
	/* 0x0b6 */		unused,
	/* 0x0b7 */		unused,
	/* 0x0b8 */		unused,
	/* 0x0b9 */		unused,
	/* 0x0ba */		unused,
	/* 0x0bb */		unused,
	/* 0x0bc */		unused,
	/* 0x0bd */		unused,
	/* 0x0be */		unused,
	/* 0x0bf */		unused,
	
	/* 0x0c0 */		unused,
	/* 0x0c1 */		unused,
	/* 0x0c2 */		unused,
	/* 0x0c3 */		unused,
	/* 0x0c4 */		unused,
	/* 0x0c5 */		unused,
	/* 0x0c6 */		unused,
	/* 0x0c7 */		unused,
	/* 0x0c8 */		unused,
	/* 0x0c9 */		unused,
	/* 0x0ca */		unused,
	/* 0x0cb */		unused,
	/* 0x0cc */		unused,
	/* 0x0cd */		unused,
	/* 0x0ce */		unused,
	/* 0x0cf */		unused,
	
	/* 0x0d0 */		unused,
	/* 0x0d1 */		unused,
	/* 0x0d2 */		unused,
	/* 0x0d3 */		unused,
	/* 0x0d4 */		unused,
	/* 0x0d5 */		unused,
	/* 0x0d6 */		unused,
	/* 0x0d7 */		unused,
	/* 0x0d8 */		unused,
	/* 0x0d9 */		unused,
	/* 0x0da */		unused,
	/* 0x0db */		unused,
	/* 0x0dc */		unused,
	/* 0x0dd */		unused,
	/* 0x0de */		unused,
	/* 0x0df */		unused,
	
	/* 0x0e0 */		unused,
	/* 0x0e1 */		unused,
	/* 0x0e2 */		unused,
	/* 0x0e3 */		unused,
	/* 0x0e4 */		unused,
	/* 0x0e5 */		unused,
	/* 0x0e6 */		unused,
	/* 0x0e7 */		unused,
	/* 0x0e8 */		unused,
	/* 0x0e9 */		unused,
	/* 0x0ea */		unused,
	/* 0x0eb */		unused,
	/* 0x0ec */		unused,
	/* 0x0ed */		unused,
	/* 0x0ee */		unused,
	/* 0x0ef */		unused,
	
	/* 0x0f0 */		unused,
	/* 0x0f1 */		unused,
	/* 0x0f2 */		unused,
	/* 0x0f3 */		unused,
	/* 0x0f4 */		unused,
	/* 0x0f5 */		unused,
	/* 0x0f6 */		unused,
	/* 0x0f7 */		unused,
	/* 0x0f8 */		unused,
	/* 0x0f9 */		unused,
	/* 0x0fa */		unused,
	/* 0x0fb */		unused,
	/* 0x0fc */		unused,
	/* 0x0fd */		unused,
	/* 0x0fe */		unused,
	
	/*
	 * MiNT extensions to GEMDOS
	 */
	
	/* 0x0ff */		stub_s_yield,
	
	/* 0x100 */		stub_f_pipe,
	/* 0x101 */		stub_f_fchown,		/* 1.15.2 */
	/* 0x102 */		stub_f_fchmod,		/* 1.15.2 */
	/* 0x103 */		unused, 		/* f_sync */
	/* 0x104 */		stub_f_cntl,
	/* 0x105 */		stub_f_instat,
	/* 0x106 */		stub_f_outstat,
	/* 0x107 */		stub_f_getchar,
	/* 0x108 */		stub_f_putchar,
	/* 0x109 */		stub_p_wait,
	/* 0x10a */		stub_p_nice,
	/* 0x10b */		stub_p_getpid,
	/* 0x10c */		stub_p_getppid,
	/* 0x10d */		stub_p_getpgrp,
	/* 0x10e */		stub_p_setpgrp,
	/* 0x10f */		stub_p_getuid,
	
	/* 0x110 */		stub_p_setuid,
	/* 0x111 */		stub_p_kill,
	/* 0x112 */		stub_p_signal,
	/* 0x113 */		stub_p_vfork,
	/* 0x114 */		stub_p_getgid,
	/* 0x115 */		stub_p_setgid,
	/* 0x116 */		stub_p_sigblock,
	/* 0x117 */		stub_p_sigsetmask,
	/* 0x118 */		stub_p_usrval,
	/* 0x119 */		stub_p_domain,
	/* 0x11a */		stub_p_sigreturn,
	/* 0x11b */		stub_p_fork,
	/* 0x11c */		stub_p_wait3,
	/* 0x11d */		stub_f_select,
	/* 0x11e */		stub_p_rusage,
	/* 0x11f */		stub_p_setlimit,
	
	/* 0x120 */		stub_t_alarm,
	/* 0x121 */		stub_p_pause,
	/* 0x122 */		stub_s_ysconf,
	/* 0x123 */		stub_p_sigpending,
	/* 0x124 */		stub_d_pathconf,
	/* 0x125 */		stub_p_msg,
	/* 0x126 */		stub_f_midipipe,
	/* 0x127 */		stub_p_renice,
	/* 0x128 */		stub_d_opendir,
	/* 0x129 */		stub_d_readdir,
	/* 0x12a */		stub_d_rewind,
	/* 0x12b */		stub_d_closedir,
	/* 0x12c */		stub_f_xattr,
	/* 0x12d */		stub_f_link,
	/* 0x12e */		stub_f_symlink,
	/* 0x12f */		stub_f_readlink,
	
	/* 0x130 */		stub_d_cntl,
	/* 0x131 */		stub_f_chown,
	/* 0x132 */		stub_f_chmod,
	/* 0x133 */		stub_p_umask,
	/* 0x134 */		stub_p_semaphore,
	/* 0x135 */		stub_d_lock,
	/* 0x136 */		stub_p_sigpause,
	/* 0x137 */		stub_p_sigaction,
	/* 0x138 */		stub_p_geteuid,
	/* 0x139 */		stub_p_getegid,
	/* 0x13a */		stub_p_waitpid,
	/* 0x13b */		stub_d_getcwd,
	/* 0x13c */		stub_s_alert,
	/* 0x13d */		stub_t_malarm,
	/* 0x13e */	(Func)	stub_p_sigintr,
	/* 0x13f */		stub_s_uptime,
	
	/* 0x140 */		unused,			/* stub_s_trapatch */
	/* 0x141 */		unused,
	/* 0x142 */		stub_d_xreaddir,
	/* 0x143 */		stub_p_seteuid,
	/* 0x144 */		stub_p_setegid,
	/* 0x145 */		stub_p_getauid,
	/* 0x146 */		stub_p_setauid,
	/* 0x147 */		stub_p_getgroups,
	/* 0x148 */		stub_p_setgroups,
	/* 0x149 */		stub_t_setitimer,
	/* 0x14a */		stub_d_chroot,		/* 1.15.3 */
	/* 0x14b */		unused,			/* stub_f_stat */
	/* 0x14c */		unused,
	/* 0x14d */		unused,
 	/* 0x14e */		stub_p_setreuid,
 	/* 0x14f */		stub_p_setregid,
 	
	/* 0x150 */		stub_s_ync,
	/* 0x151 */		stub_s_hutdown,
	/* 0x152 */		stub_d_readlabel,
	/* 0x153 */		stub_d_writelabel,
	/* 0x154 */		stub_s_system,
	/* 0x155 */		stub_t_gettimeofday,
	/* 0x156 */		stub_t_settimeofday,
	/* 0x157 */		unused,			/* t_adjtime */
	/* 0x158 */		stub_p_getpriority,
	/* 0x159 */		stub_p_setpriority,
	/* 0x15a */		unused,
	/* 0x15b */		unused,
	/* 0x15c */		unused,
	/* 0x15d */		unused,
	/* 0x15e */		unused,
	/* 0x15f */		unused
};


/*
 * Here follows the TraPatch code
 */

# define XBRA_ID_TraP 0x54726150L

typedef struct XBRA XBRA;
struct XBRA
{
	long	xbra_magic;
	long	xbra_id;
	XBRA	*xbra_next;
};


# define tp_version 0x19990716L
# define max_lib 4


int tp_gemdos_funcs = 100;      /* konfigurierter Wert */

Func *	trap_tabs [max_lib+1] = {NULL, NULL, NULL, NULL, NULL};
ushort	entries   [max_lib+1] = {0   , 0   , 0   , 0   , 0   };


/*Func *trap_dos_table;*/



void
init_dos_trap (void)
{
	int num_of_funcs;
	int z;
	
	if (tp_gemdos_funcs >= DOS_MAX)
		num_of_funcs = entries [TPL_GEMDOS] = tp_gemdos_funcs;
	else
		num_of_funcs = entries [TPL_GEMDOS] = DOS_MAX;
	
	trap_tabs [TPL_GEMDOS] = kmalloc (num_of_funcs * sizeof (Func));
	
	if (trap_tabs [TPL_GEMDOS] < 0)
		FATAL ("Allocating table for DOS calls failed!\n");
	
	if (tp_gemdos_funcs > DOS_MAX)
		for (z = num_of_funcs - 1; z >= DOS_MAX; z--)
			trap_tabs [TPL_GEMDOS][z] = unused;
	
	for (z = DOS_MAX - 1; z >= 0; z--)
		trap_tabs [TPL_GEMDOS][z] = dos_table [z];
}


long
s_trapatch (ushort mode, ushort info, ushort lib,
            ushort fnum, void *vec, void *reserved)
{
	XBRA *help;
	XBRA **old;
	int  isroot = (curproc->euid == 0);
	
	switch (mode)
	{
		case TPM_INSTALL:
		{
			if (isroot == 0)
			{
				DEBUG (("TPM_INSTALL: access denied"));
				return EACCES;
			}
			
			if (reserved != NULL)
				return ETP_illresvd;
			if (((XBRA*) vec)[-1].xbra_magic != XBRA_MAGIC)
				return ETP_no_xbra;
			if (lib > max_lib)
				return ETP_lib_illegal;
			if (fnum >= entries[lib])
				return ETP_fnum_illegal;
			
			help = (XBRA*) trap_tabs[lib][fnum];
			
			while (help[-1].xbra_next != NULL)
			{
				if (help[-1].xbra_id == XBRA_ID_TraP)
					return EERROR;
				if (help[-1].xbra_id == ((XBRA*) vec)[-1].xbra_id)
					return ETP_dupe;
				
				help = help[-1].xbra_next;
			}
			
			/* Interrupts sperren */
			((XBRA*) vec)[-1].xbra_next = help;
			trap_tabs[lib][fnum] = vec;
			/* Interrupts freigeben */
			
			return E_OK;
		}
		case TPM_DEINSTALL:
		{
			if (isroot == 0)
			{
				DEBUG (("TPM_DEINSTALL: access denied"));
				return EACCES;
			}
			
			if (reserved != NULL)
				return ETP_illresvd;
			if (lib > max_lib)
				return ETP_lib_illegal;
			if (fnum >= entries[lib])
				return ETP_fnum_illegal;
			
			help = (XBRA *) trap_tabs[lib][fnum];
			old = (XBRA **) &trap_tabs[lib][fnum];
			
			while (1)
			{
				if (help[-1].xbra_magic != XBRA_MAGIC)
					return EERROR;
				if (help[-1].xbra_id == XBRA_ID_TraP)
					return ETP_xbra_not_found;
				if (help[-1].xbra_next == NULL)
					return EERROR;
				if (help[-1].xbra_id == (long) vec)
				{
					*old = help[-1].xbra_next;
					return E_OK;
				}
				
				old = &help[-1].xbra_next;
				help = help[-1].xbra_next;
			}
			
			/* not reached */
			break;
		}
		case TPM_INFO:
		{
			switch (info)
			{
				case TPW_MAXWHAT:
				{
					return TPW_ENTRY;
				}
				case TPW_VERSION:
				{
					return tp_version;
				}
				case TPW_ENTRIES:
				{
					if (lib > max_lib)
						return ETP_lib_illegal;
					
					return entries[lib];
				}
				case TPW_ENTRY:
				{
					if (isroot == 0)
					{
						DEBUG (("TPW_ENTRY: access denied"));
						return EACCES;
					}
					
					if (lib > max_lib)
						return ETP_lib_illegal;
					if (fnum >= entries[lib])
						return ETP_fnum_illegal;
					
					return ((long) trap_tabs[lib][fnum]);
				}
				default:
				{
					return ETP_info_not_available;
				}
			}
		}
		default:
		{
			return ETP_invalid_mode;
		}
	}
	
	/* not reached */
	return EERROR;
}
