/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

# include <stdarg.h>

# include "init.h"
# include "global.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/filedesc.h"
# include "mint/basepage.h"
# include "mint/xbra.h"

# include "arch/cpu.h"		/* init_cache, cpush, setstack */
# include "arch/context.h"	/* restore_context */
# include "arch/intr.h"		/* new_mediach, new_rwabs, new_getbpb, same for old_ */
# include "arch/init_mach.h"	/* */
# include "arch/kernel.h"	/* enter_gemdos */
# include "arch/mmu.h"		/* save_mmu */
# include "arch/mprot.h"	/* */
# include "arch/startup.h"	/* _base */
# include "arch/syscall.h"	/* call_aes */

# include "bios.h"	/* */
# include "block_IO.h"	/* init_block_IO */
# include "cnf.h"	/* load_config, some variables */
# include "console.h"	/* */
# include "cookie.h"	/* restr_cookie */
# include "crypt_IO.h"	/* init_crypt_IO */
# include "delay.h"	/* calibrate_delay */
# include "dos.h"	/* */
# include "dosdir.h"	/* */
# include "filesys.h"	/* init_filesys, s_ync, close_filesys */
# ifdef FLOPPY_ROUTINES
# include "floppy.h"	/* init_floppy */
# endif
# include "gmon.h"	/* monstartup */
# include "info.h"	/* welcome messages */
# include "ipc_socketutil.h" /* domaininit() */
# include "k_exec.h"	/* sys_pexec */
# include "k_exit.h"	/* sys_pwaitpid */
# include "k_fds.h"	/* do_open/do_pclose */
# include "keyboard.h"	/* init_keytbl() */
# include "kmemory.h"	/* kmalloc */
# include "memory.h"	/* init_mem, get_region, attach_region, restr_screen */
# include "mis.h"	/* startup_shell */
# include "module.h"	/* load_all_modules */
# include "proc.h"	/* init_proc, add_q, rm_q */
# include "signal.h"	/* post_sig */
# include "syscall_vectors.h"
# include "time.h"	/* */
# include "timeout.h"	/* */
# include "unicode.h"	/* init_unicode() */
# include "update.h"	/* start_sysupdate */
# include "util.h"	/* */
# include "fatfs.h"	/* fatfs_config() */
# include "tosfs.h"	/* tos_filesys */
# include "xbios.h"	/* has_bconmap, curbconmap */


/* if the user is holding down the magic shift key, we ask before booting */
# define MAGIC_SHIFT	0x2		/* left shift */

/* magic number to show that we have captured the reset vector */
# define RES_MAGIC	0x31415926L
# define EXEC_OS 0x4feL

static long _cdecl mint_criticerr (long);
static void _cdecl do_exec_os (register long basepage);

static int	boot_kernel_p (void);

void mint_thread(void *arg);

/* print an additional boot message
 */
static short use_sys = 0;

void
boot_print (const char *s)
{
	if (use_sys)
		sys_c_conws(s);
	else
		Cconws (s);
}

void
boot_printf (const char *fmt, ...)
{
	char buf [512];
	va_list args;

	va_start (args, fmt);
	vsprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	boot_print(buf);
}

/* Stop and ask the user for conformation to continue */
static short step_by_step;

static void
stop_and_ask(void)
{
	if (step_by_step)
	{
		boot_print(MSG_init_hitanykey);
		if (use_sys)
			sys_c_conin();
		else
			Cconin();
	}
}

/* structures for keyboard/MIDI interrupt vectors */
KBDVEC *syskey, oldkey;

xbra_vec old_criticerr;
xbra_vec old_execos;

/* bus error, address error, illegal instruction, etc. vectors
 */
xbra_vec old_bus;
xbra_vec old_addr;
xbra_vec old_ill;
xbra_vec old_divzero;
xbra_vec old_trace;
xbra_vec old_priv;
xbra_vec old_linef;
xbra_vec old_chk;
xbra_vec old_trapv;
xbra_vec old_mmuconf;
xbra_vec old_format;
xbra_vec old_cpv;
xbra_vec old_uninit;
xbra_vec old_spurious;
xbra_vec old_fpcp[7];
xbra_vec old_pmmuill;
xbra_vec old_pmmuacc;

long	old_term;
long	old_resval;	/* old reset validation */
long	olddrvs;	/* BIOS drive map */

/* table of processor frame sizes in _words_ (not used on MC68000) */
uchar framesizes[16] =
{
	/*0*/	0,	/* MC68010/M68020/M68030/M68040 short */
	/*1*/	0,	/* M68020/M68030/M68040 throwaway */
	/*2*/	2,	/* M68020/M68030/M68040 instruction error */
	/*3*/	2,	/* M68040 floating point post instruction */
	/*4*/	3,	/* MC68LC040/MC68EC040 unimplemented floating point instruction */
			/* or */
			/* MC68060 access error */
	/*5*/	0,	/* NOTUSED */
	/*6*/	0,	/* NOTUSED */
	/*7*/	26,	/* M68040 access error */
	/*8*/	25,	/* MC68010 long */
	/*9*/	6,	/* M68020/M68030 mid instruction */
	/*A*/	12,	/* M68020/M68030 short bus cycle */
	/*B*/	42,	/* M68020/M68030 long bus cycle */
	/*C*/	8,	/* CPU32 bus error */
	/*D*/	0,	/* NOTUSED */
	/*E*/	0,	/* NOTUSED */
	/*F*/	13	/* 68070 and 9xC1xx microcontroller address error */
};

/*
 * install a new vector for address "addr", using the XBRA protocol.
 * must run in supervisor mode!
 *
 *
 * WHO DID THIS?
 * self-modifying code is a bad idea - especially when not flushing
 * CPU caches afterwards!
 *
 * The best idea would be to get rid of the separate struct xbra,
 * and instead have enough space in front of all routines that might be
 * installed in a XBRA chain, so we can just patch in the XBRA chain
 * vector. This would also get rid of one unneccessary jump (which
 * flushes the instruction pipe on '040).
 */

static void
xbra_install (xbra_vec *xv, long addr, long _cdecl (*func) ())
{
	xv->xbra_magic = XBRA_MAGIC;
	xv->xbra_id = MINT_MAGIC;
	xv->jump = JMP_OPCODE;
	xv->this = func;
	xv->next = *((struct xbra **) addr);
	*((short **) addr) = &xv->jump;

	/* ms - workaround for now */
	cpush (xv, sizeof (xv));
}

/* new XBRA installer; for now only used on VBL, 200Hz and GEMDOS timer
 * but other stuff should be subsequently made linked by this
 * instead of by the xbra_install()
 */

static void
new_xbra_install (long *xv, long addr, long _cdecl (*func) ())
{
	*xv = *(long *) addr;
	*(long *) addr = (long) func;

	/* better to be safe... */
	cpush ((long *) addr, sizeof (addr));
	cpush (xv, sizeof (xv));
}

/*
 * MiNT critical error handler; all it does is to jump through
 * the vector for the current process
 */

static long _cdecl
mint_criticerr (long error) /* high word is error, low is drive */
{
	/* just return with error */
	return (error >> 16);
}

/*
 * if we are MultiTOS, and if we are running from the AUTO folder,
 * then we grab the exec_os vector and use that to start GEM; that
 * way programs that expect exec_os to act a certain way will still
 * work.
 *
 * NOTE: we must use Pexec instead of sys_pexec here, because we will
 * be running in a user context (that of process 1, not process 0)
 */

static void _cdecl
do_exec_os (long basepage)
{
	register long r;

	/* we have to set a7 to point to lower in our TPA;
	 * otherwise we would bus error right after the Mshrink call!
	 */
	setstack(basepage + QUANTUM - 40L);
	Mshrink((void *)basepage, QUANTUM);
	r = Pexec(200, init_prg, init_tail, init_env);
	Pterm ((int)r);
}


/*
 * initialize all interrupt vectors and new trap routines
 * we also get here any TOS variables that we're going to change
 * (e.g. the pointer to the cookie jar) so that rest_intr can
 * restore them.
 */

static void
init_intr (void)
{
	ushort savesr;
	int i;
	long *syskey_aux;

	syskey = (KBDVEC *) Kbdvbase ();
	oldkey = *syskey;

	syskey_aux = (long *)syskey;
	syskey_aux--;

	if (*syskey_aux && tosvers >= 0x0200)
		new_xbra_install (&oldkeys, (long)syskey_aux, newkeys);

	old_term = (long) Setexc (0x102, -1UL);

	savesr = spl7();

	/* Take all traps; notice, that the "old" address for any trap
	 * except #1, #13 and #14 (GEMDOS, BIOS, XBIOS) is lost.
	 * It does not matter, because MiNT does not pass these
	 * calls along anyway.
	 * The main problem is, that activating this makes ROM VDI
	 * no more working, and actually any program that takes a
	 * trap before MiNT is loaded.
	 * WARNING: NVDI 5.00 uses trap #15 and isn't even polite
	 * enough to link it with XBRA.
	 */

	{
	long dummy;

	new_xbra_install (&dummy, 0x80L, unused_trap);		/* trap #0 */
	new_xbra_install (&old_dos, 0x84L, mint_dos);		/* trap #1, GEMDOS */
# if 0
	new_xbra_install (&dummy, 0x88L, unused_trap);		/* trap #2, GEM */
# endif
	new_xbra_install (&dummy, 0x8cL, unused_trap);		/* trap #3 */
	new_xbra_install (&dummy, 0x90L, unused_trap);		/* trap #4 */
	new_xbra_install (&dummy, 0x94L, unused_trap);		/* trap #5 */
	new_xbra_install (&dummy, 0x98L, unused_trap);		/* trap #6 */
	new_xbra_install (&dummy, 0x9cL, unused_trap);		/* trap #7 */
	new_xbra_install (&dummy, 0xa0L, unused_trap);		/* trap #8 */
	new_xbra_install (&dummy, 0xa4L, unused_trap);		/* trap #9 */
	new_xbra_install (&dummy, 0xa8L, unused_trap);		/* trap #10 */
	new_xbra_install (&dummy, 0xacL, unused_trap);		/* trap #11 */
	new_xbra_install (&dummy, 0xb0L, unused_trap);		/* trap #12 */
	new_xbra_install (&old_bios, 0xb4L, mint_bios);		/* trap #13, BIOS */
	new_xbra_install (&old_xbios, 0xb8L, mint_xbios);	/* trap #14, XBIOS */
# if 0
	new_xbra_install (&dummy, 0xbcL, unused_trap);		/* trap #15 */
# endif
	}

	xbra_install (&old_criticerr, 0x404L, mint_criticerr);
	new_xbra_install (&old_5ms, 0x114L, mint_5ms);

#if 0 // this should really not be necessary ... rincewind
	new_xbra_install (&old_resvec, 0x042aL, reset);
	old_resval = *((long *)0x426L);
	*((long *) 0x426L) = RES_MAGIC;
#endif

	spl (savesr);

	/* set up signal handlers */
	xbra_install (&old_bus, 8L, new_bus);
	xbra_install (&old_addr, 12L, new_addr);
	xbra_install (&old_ill, 16L, new_ill);
	xbra_install (&old_divzero, 20L, new_divzero);
	xbra_install (&old_trace, 36L, new_trace);
	xbra_install (&old_priv, 32L, new_priv);
	if (tosvers >= 0x106)
		xbra_install (&old_linef, 44L, new_linef);
	xbra_install (&old_chk, 24L, new_chk);
	xbra_install (&old_trapv, 28L, new_trapv);

	for (i = (int)(sizeof (old_fpcp) / sizeof (old_fpcp[0])); i--; )
		xbra_install (&old_fpcp[i], 192L + i * 4, new_fpcp);

	xbra_install (&old_mmuconf, 224L, new_mmu);
	xbra_install (&old_pmmuill, 228L, new_mmu);
	xbra_install (&old_pmmuacc, 232L, new_pmmuacc);
	xbra_install (&old_format, 56L, new_format);
	xbra_install (&old_cpv, 52L, new_cpv);
	xbra_install (&old_uninit, 60L, new_uninit);
	xbra_install (&old_spurious, 96L, new_spurious);

	/* set up disk vectors */
	new_xbra_install (&old_mediach, 0x47eL, new_mediach);
	new_xbra_install (&old_rwabs, 0x476L, new_rwabs);
	new_xbra_install (&old_getbpb, 0x472L, new_getbpb);
	olddrvs = *((long *) 0x4c2L);

# if 0
	/* initialize psigintr() call stuff (useful on 68010+ only)
	 */
# ifndef ONLY030
	if (mcpu > 10)
# endif
	{
		intr_shadow = kmalloc(1024);
		if (intr_shadow)
			quickmove ((char *)intr_shadow, (char *) 0x0L, 1024L);
	}
# endif

	/* we'll be making GEMDOS calls */
	enter_gemdos ();
}

/* restore all interrupt vectors and trap routines
 *
 * NOTE: This is *not* the approved way of unlinking XBRA trap handlers.
 * Normally, one should trace through the XBRA chain. However, this is
 * a very unusual situation: when MiNT exits, any TSRs or programs running
 * under MiNT will no longer exist, and so any vectors that they have
 * caught will be pointing to never-never land! So we do something that
 * would normally be considered rude, and restore the vectors to
 * what they were before we ran.
 * BUG: we should restore *all* vectors, not just the ones that MiNT caught.
 */

void
restr_intr (void)
{
	ushort savesr;
	int i;
	long *syskey_aux;

	savesr = spl7 ();

	*syskey = oldkey;	/* restore keyboard vectors */

	if (tosvers >= 0x0200)
	{
		syskey_aux = (long *)syskey;
		syskey_aux--;
		*syskey_aux = (long) oldkeys;
	}

	*((long *) 0x008L) = (long) old_bus.next;

	*((long *) 0x00cL) = (long) old_addr.next;
	*((long *) 0x010L) = (long) old_ill.next;
	*((long *) 0x014L) = (long) old_divzero.next;
	*((long *) 0x020L) = (long) old_priv.next;
	*((long *) 0x024L) = (long) old_trace.next;

	if (old_linef.next)
		*((long *) 0x2cL) = (long) old_linef.next;

	*((long *) 0x018L) = (long) old_chk.next;
	*((long *) 0x01cL) = (long) old_trapv.next;

	for (i = (int)(sizeof (old_fpcp) / sizeof (old_fpcp[0])); i--; )
		((long *) 0x0c0L)[i] = (long) old_fpcp[i].next;

	*((long *) 0x0e0L) = (long) old_mmuconf.next;
	*((long *) 0x0e4L) = (long) old_pmmuill.next;
	*((long *) 0x0e8L) = (long) old_pmmuacc.next;
	*((long *) 0x038L) = (long) old_format.next;
	*((long *) 0x034L) = (long) old_cpv.next;
	*((long *) 0x03cL) = (long) old_uninit.next;
	*((long *) 0x060L) = (long) old_spurious.next;

	*((long *) 0x084L) = old_dos;
	*((long *) 0x0b4L) = old_bios;
	*((long *) 0x0b8L) = old_xbios;
	*((long *) 0x408L) = old_term;
	*((long *) 0x404L) = (long) old_criticerr.next;
	*((long *) 0x114L) = old_5ms;
#if 0	//
	*((long *) 0x426L) = old_resval;
	*((long *) 0x42aL) = old_resvec;
#endif
	*((long *) 0x476L) = old_rwabs;
	*((long *) 0x47eL) = old_mediach;
	*((long *) 0x472L) = old_getbpb;
	*((long *) 0x4c2L) = olddrvs;

	spl (savesr);
}

static char *my_name = "MINT.PRG";

static void
get_my_name (void)
{
	register BASEPAGE *bp;
	register DTABUF *dta;
	register char *p;

	/* When executing AUTO folder programs, the ROM TOS locates
	 * all programs using Fsfirst/Fsnext; by peeking into our
	 * parent's (the AUTO-execute process's) DTA area, we can
	 * find out our own name...
	 * This works with all known (to me) TOS versions prior to
	 * MultiTOS; but I don't think any other programs should use
	 * this trick, since MiNT or another OS with memory protection
	 * could already be running! <mk>
	 *
	 * Some validity checks first...
	 */

	bp = _base->p_parent;

	if (bp) dta = (DTABUF *) bp->p_dta;
	else dta = NULL;

	if (bp && dta)
	{
		p = dta->dta_name;

		/* Test if "MINT*.PRG" or "MNT*.PRG" */
		if ((strncmp (p, "MINT", 4) == 0 || strncmp (p, "MNT", 3) == 0)
			 && strncmp (p + strlen (p) - 4, ".PRG", 4) == 0)
		{
			my_name = p ;
# if 0
			/* DEBUGGING: */
			boot_printf (MSG_init_getname, p);
# endif
		}
	}
}

/* Boot menu routines. Quite lame I admit. Added 19.X.2000. */

# define MENU_OPTIONS	7

static short load_xfs_f;
static short load_xdd_f;
static short load_auto;
static short save_ini;

static const char *ini_keywords[] =
{
	"XFS_LOAD",
	"XDD_LOAD",
	"EXE_AUTO",
	"MEM_PROT",
	"INI_STEP",
	"INI_SAVE"
};

/* we assume that the mint.ini file, containing the boot
 * menu defaults, is located at same place as mint.cnf is
 */

INLINE short
find_ini (char *outp, long outsize)
{
	ksprintf(outp, outsize, "%smint.ini", sysdir);

	if (Fsfirst(outp, 0) == 0)
		return 1;

	return 0;
}

static char *
find_char (char *s, char c)
{
	while (*s && *s != '\n' && *s != '\r')
	{
		if (*s == c)
			return s;
		s++;
	}

	return NULL;
}

INLINE short
whether_yes (char *s)
{
	s = find_char(s, '=');
	if (!s)
		return 0;
	s = find_char(s, 'Y');	/* don't add 'y' here, see below */
	if (!s)
		return 0;
	return 1;
}

static void
read_ini (void)
{
	DTABUF *dta;
	char *buf, *s, ini_file[32];
	long r, x, len;
	short inihandle, options[MENU_OPTIONS-1] = { 1, 1, 1, 1, 0, 1 };

	if (!find_ini(ini_file, sizeof(ini_file)))
		goto initialize;

	/* Figure out the file's length. Wish I had Fstat() here :-( */
	dta = (DTABUF *) Fgetdta();
	r = Fsfirst(ini_file, 0);
	if (r < 0) goto initialize;	/* No such file, probably */
	len = dta->dta_size;
	if (len < 10) goto initialize;	/* proper mint.ini can't be so short */
	len++;

	buf = (char *) Mxalloc (len, 0x0003);
	if ((long)buf < 0)
		buf = (char *) Malloc (len);	/* No Mxalloc()? */
	if ((long)buf <= 0) goto initialize;	/* Out of memory or such */
	bzero (buf, len);

	inihandle = Fopen (ini_file, 0);
	if (inihandle < 0) goto exit;
	r = Fread (inihandle, len, buf);
	if (r < 0) goto close;

	strupr (buf);
	for (x = 0; x < MENU_OPTIONS-1; x++)
	{
		s = strstr (buf, ini_keywords[x]);
		if (s)
			options[x] = whether_yes (s);
	}

close:
	Fclose (inihandle);
exit:
	Mfree ((long) buf);

initialize:
	load_xfs_f = options[0];
	load_xdd_f = options[1];
	load_auto = options[2];
	no_mem_prot = !options[3];
	step_by_step = options[4];
	save_ini = options[5];
}

INLINE void
write_ini (short *options)
{
	short inihandle;
	char ini_file[32], buf[256];
	long r, x, l;

	ksprintf(ini_file, sizeof(ini_file), "%smint.ini", sysdir);

	inihandle = Fcreate (ini_file, 0);
	if (inihandle < 0)
		return;

	options++;		/* Ignore the first one :-) */

	l = strlen (MSG_init_menuwarn);
	r = Fwrite (inihandle, l, MSG_init_menuwarn);
	if ((r < 0) || (r != l))
	{
		r = -1;
		goto close;
	}

	for (x = 0; x < MENU_OPTIONS-1; x++)
	{
		ksprintf (buf, sizeof (buf), "%s=%s\n", \
			ini_keywords[x], options[x] ? "YES" : "NO");
		l = strlen (buf);
		r = Fwrite (inihandle, l, buf);
		if ((r < 0) || (r != l))
		{
			r = -1;
			break;
		}
	}
close:
	Fclose (inihandle);

	if (r < 0)
		Fdelete (ini_file);
}

static int
boot_kernel_p (void)
{
	short option[MENU_OPTIONS];
	long c = 0;

	option[0] = 1;			/* Load MiNT or not */
	option[1] = load_xfs_f;		/* Load XFS or not */
	option[2] = load_xdd_f;		/* Load XDD or not */
	option[3] = load_auto;		/* Load AUTO or not */
	option[4] = !no_mem_prot;	/* Use memprot or not */
	option[5] = step_by_step;	/* Enter stepper mode */
	option[6] = save_ini;

	for (;;)
	{
		boot_printf(MSG_init_bootmenu, \
			option[0] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[1] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[2] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[3] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[4] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[5] ? MSG_init_menu_yesrn : MSG_init_menu_norn, \
			option[6] ? MSG_init_menu_yesrn : MSG_init_menu_norn );
wait:
		c = Crawcin();
		c &= 0x7f;
		switch(c)
		{
			case 0x03:
				return 1;
			case 0x0a:
			case 0x0d:
				load_xfs_f = option[1];
				load_xdd_f = option[2];
				load_auto = option[3];
				no_mem_prot = !option[4];
				step_by_step = option[5];
				save_ini = option[6];
				if (save_ini)
					write_ini(option);
				return (int)option[0];
			case '0':
				option[6] = option[6] ? 0 : 1;
				break;
			case '1' ... '6':
				option[(c - 1) & 0x0f] = option[(c - 1) & 0x0f] ? 0 : 1;
				break;
			default:
				goto wait;
		}
	}

	return 1;	/* not reached */
}

/*
 * some global variables used to see if GEM is active
 */

static short aes_intout[64];
static short aes_dummy[64];
static short aes_globl[15];
static short aes_cntrl[6] = { 10, 0, 1, 0, 0 };

short *aes_pb[6] = { aes_cntrl, aes_globl, aes_dummy, aes_intout,
		     aes_dummy, aes_dummy };

/*
 * check for whether GEM is active; remember, this *must* be done in
 * user mode
 */

INLINE int
check_for_gem (void)
{
	call_aes(aes_pb);	/* does an appl_init */
	return aes_globl[0];
}

static long GEM_memflags = F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_S;

void
init (void)
{
	long newstamp, pause, r, *sysbase;
	FILEPTR *f;
	char curpath[128];

	/* Initialize sysdir (TOS style) */
	strcpy(sysdir, "\\");
	if (Fsfirst("\\multitos\\mint.cnf", 0) == 0)
		strcpy(sysdir, "\\multitos\\");
	else if (Fsfirst("\\mint\\mint.cnf", 0) == 0)
		strcpy(sysdir, "\\mint\\");

	read_ini();	/* Read user defined defaults */

	/* greetings (placed here 19960610 cpbs to allow me to get version
	 * info by starting MINT.PRG, even if MiNT's already installed.)
	 */
	boot_print (greet1);
	boot_print (greet2);

	/* check for GEM -- this must be done from user mode */
	if (check_for_gem())
	{
		boot_print(MSG_must_be_auto);
		boot_print(MSG_init_hitanykey);
		(void) Cconin();
		Pterm0();
	}

	/* figure out what kind of machine we're running on:
	 * - biosfs wants to know this
	 * - also sets no_mem_prot
	 * - 1992-06-25 kbad put it here so memprot_warning can be intelligent
	 */
	if (getmch ())
	{
		boot_print(MSG_init_hitanykey);
		(void) Cconin();
		Pterm0();
	}

	/* Ask the user if s/he wants to boot MiNT */
	boot_print(MSG_init_askmenu);

	pause = (Tgettime() & 0x1fL) + 2;		/* 4 second pause */
	if (pause > 29)
		pause -= 29;

	do
	{
		newstamp = Tgettime() & 0x1fL;

		if ((Kbshift (-1) & MAGIC_SHIFT) == MAGIC_SHIFT)
		{
			long yn = boot_kernel_p ();
			if (!yn)
				Pterm0 ();
			newstamp = pause;
		}
	}
	while (newstamp != pause);

	boot_print("\r\n");

# ifdef OLDTOSFS
	/* Get GEMDOS version from ROM for later use by our own Sversion() */
	gemdos_version = Sversion ();
# endif

	get_my_name();

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_mp, no_mem_prot ? MSG_init_mp_disabled : MSG_init_mp_enabled);
# endif
# ifdef VERBOSE_BOOT
	/* These are set inside getmch() */
	boot_printf(MSG_init_kbd_desktop_nationality, gl_kbd, gl_lang);
# endif

	/* get the current directory, so that we can switch back to it after
	 * the file systems are properly initialized
	 *
	 * set the current directory for the current process
	 */
	Dgetpath (curpath, 0);
	if (!*curpath)
	{
		curpath[0] = '\\';
		curpath[1] = 0;
	}

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_supermode);
# endif

	(void) Super (0L);

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	stop_and_ask();

# ifdef DEBUG_INFO
	{
		long usp, ssp;

		usp = get_usp();
		ssp = get_ssp();

		boot_printf("Kernel BP: 0x%08lx,\r\n" \
				"TEXT: 0x%08lx (SIZE: %ld B)\r\n" \
				"DATA: 0x%08lx (SIZE: %ld B)\r\n" \
				" BSS: 0x%08lx (SIZE: %ld B)\r\n" \
				" USP: 0x%08lx (SIZE: %ld B)\r\n" \
				" SSP: 0x%08lx (SIZE: %ld B)\r\n", \
				_base, \
				_base->p_tbase, _base->p_tlen, \
				_base->p_dbase, _base->p_dlen, \
				_base->p_bbase, _base->p_blen, \
				usp, usp - (_base->p_bbase + _base->p_blen), \
				ssp, ssp - (_base->p_bbase + _base->p_blen));
	}
# endif

	sysdrv = *((short *) 0x446);	/* get the boot drive number */

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_sysdrv_is, sysdrv + 'a');
	boot_print(MSG_init_saving_mmu);
# endif

	if (!no_mem_prot)
		save_mmu ();		/* save current MMU setup */

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	stop_and_ask();

	/* get GEMDOS pointer to current basepage
	 *
	 * 0x4f2 points to the base of the OS; here we can find the OS compilation
	 * date, and (in newer versions of TOS) where the current basepage pointer
	 * is kept; in older versions of TOS, it's at 0x602c
	 */
	sysbase = *((long **)(0x4f2L));	/* gets the RAM OS header */
	sysbase = (long *)sysbase[2];	/* gets the ROM one */

	tosvers = (int)(sysbase[0] & 0x0000ffff);
	kbshft = (tosvers == 0x100) ? (char *) 0x0e1bL : (char *)sysbase[9];
	falcontos = (tosvers >= 0x0400 && tosvers <= 0x0404) || (tosvers >= 0x0700);

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_tosver_kbshft, (tosvers >> 8) & 0xff, tosvers & 0xff, \
			falcontos ? " (FalconTOS)" : "", (long)kbshft);
# endif

	if (falcontos)
	{
		bconmap2 = (BCONMAP2_T *) Bconmap (-2);
		if (bconmap2->maptabsize == 1)
		{
			/* Falcon BIOS Bconmap is busted */
			bconmap2->maptabsize = 3;
		}
		has_bconmap = 1;
	}
	else
	{
		/* The TT TOS release notes are wrong;
		 * this is the real way to test for Bconmap ability
		 */
		has_bconmap = (Bconmap (0) == 0);

		/* kludge for PAK'ed ST/STE's
		 * they have a patched TOS 3.06 and say they have Bconmap(),
		 * ... but the patched TOS crash hardly if Bconmap() is used.
		 */
		if (has_bconmap)
		{
			if (mch == ST || mch == STE || mch == MEGASTE)
				has_bconmap = 0;
		}

		if (has_bconmap)
			bconmap2 = (BCONMAP2_T *) Bconmap (-2);
	}

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_bconmap, has_bconmap ? MSG_init_present : MSG_init_not_present);
# endif

	stop_and_ask();

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_system);
# endif

	/* initialize cache */
	init_cache ();
	DEBUG (("init_cache() ok!"));

	/* initialize memory */
	init_mem ();
	DEBUG (("init_mem() ok!"));

	/* Initialize high-resolution calendar time */
	init_time ();
	DEBUG (("init_time() ok!"));

	/* initialize buffered block I/O */
	init_block_IO ();
	DEBUG (("init_block_IO() ok!"));

	/* initialize floppy */
#ifdef FLOPPY_ROUTINES
	init_floppy ();
	DEBUG (("init_floppy() ok!"));
#endif

	/* initialize crypto I/O layer */
	init_crypt_IO ();
	DEBUG (("init_crypt_IO() ok!"));

	/* initialize the basic file systems */
	init_filesys ();
	DEBUG (("init_filesys() ok!"));

	/* initialize processes */
	init_proc();
	DEBUG (("init_proc() ok! (base = %lx)", _base));

	/* initialize system calls */
	init_dos ();
	DEBUG (("init_dos() ok!"));
	init_bios ();
	DEBUG (("init_bios() ok!"));
	init_xbios ();
	DEBUG (("init_xbios() ok!"));

	/* initialize basic keyboard stuff */
	init_keybd();

	/* Disable all CPU caches */
	ccw_set(0x00000000L, 0x0000c57fL);

	/* initialize interrupt vectors */
	init_intr ();
	DEBUG (("init_intr() ok!"));

	/* Enable superscalar dispatch on 68060 */
	get_superscalar();

	/* Init done, now enable/unfreeze all caches.
	 * Don't touch the write/allocate bits, though.
	 */
	ccw_set(0x0000c567L, 0x0000c57fL);

# ifdef _xx_KMEMDEBUG
	/* XXX */
	{
		extern int kmemdebug_can_init;
		kmemdebug_can_init = 1;
	}
# endif

	/* set up cookie jar */
	init_cookies();
	DEBUG(("init_cookies() ok!"));

	/* add our pseudodrives */
	*((long *) 0x4c2L) |= PSEUDODRVS;

	/* set up standard file handles for the current process
	 * do this here, *after* init_intr has set the Rwabs vector,
	 * so that AHDI doesn't get upset by references to drive U:
	 */

	r = FP_ALLOC(rootproc, &f);
	if (r) FATAL("Can't allocate fp!");

	r = do_open(&f, "u:/dev/console", O_RDWR, 0, NULL);
	if (r)
		FATAL("unable to open CONSOLE device");

	rootproc->p_fd->control = f;
	rootproc->p_fd->ofiles[0] = f; f->links++;
	rootproc->p_fd->ofiles[1] = f; f->links++;

	r = FP_ALLOC(rootproc, &f);
	if (r) FATAL("Can't allocate fp!");

	r = do_open(&f, "u:/dev/modem1", O_RDWR, 0, NULL);
	if (r)
		FATAL("unable to open MODEM1 device");

	rootproc->p_fd->aux = f;
	((struct tty *) f->devinfo)->aux_cnt = 1;
	f->pos = 1;	/* flag for close to --aux_cnt */

	if (has_bconmap)
	{
		/* If someone has already done a Bconmap call, then
		 * MODEM1 may no longer be the default
		 */
		sys_b_bconmap(curbconmap);
		f = rootproc->p_fd->aux;	/* bconmap can change rootproc->aux */
	}

	if (f)
	{
		rootproc->p_fd->ofiles[2] = f;
		f->links++;
	}

	r = FP_ALLOC(rootproc, &f);
	if (r) FATAL("Can't allocate fp!");

	r = do_open(&f, "u:/dev/centr", O_RDWR, 0, NULL);
	if (!r)
	{
		rootproc->p_fd->ofiles[3] = f;
		rootproc->p_fd->prn = f;
		f->links++;
	}

	r = FP_ALLOC(rootproc, &f);
	if (r) FATAL("Can't allocate fp!");

	r = do_open(&f, "u:/dev/midi", O_RDWR, 0, NULL);
	if (!r)
	{
		rootproc->p_fd->midiin = f;
		rootproc->p_fd->midiout = f;
		f->links++;

		((struct tty *) f->devinfo)->use_cnt++;
		((struct tty *) f->devinfo)->aux_cnt = 2;
		f->pos = 1;	/* flag for close to --aux_cnt */
	}

	/* Use internal sys_c_conws() in boot_print() since now */
	use_sys = 1;

	/* Make the sysdir MiNT-style */
	{
		char temp[32];
		short sdx = 0;

		while (sysdir[sdx])
		{
			if (sysdir[sdx] == '\\')
				sysdir[sdx] = '/';
			sdx++;
		}

		ksprintf(temp, sizeof(temp), "u:/a%s", sysdir);
		temp[3] = (char)(sysdrv + 'a');

		strcpy(sysdir, temp);
	}

	/* print the warning message if MP is turned off */
	if (no_mem_prot && mcpu > 20)
		boot_print(memprot_warning);

	stop_and_ask();

	/* initialize delay */
	boot_print(MSG_init_delay_loop);

	calibrate_delay();

	/* Round the value and print it */
	boot_printf("%lu.%02lu BogoMIPS\r\n\r\n", \
		(loops_per_sec + 2500) / 500000, ((loops_per_sec + 2500) / 5000) % 100);

	stop_and_ask();

	/* initialize internal xdd */
# ifdef DEV_RANDOM
	boot_print(random_greet);
# endif

	/* initialize built-in domain ops */
# ifdef VERBOSE_BOOT
	boot_print(MSG_init_domaininit);
# endif

	domaininit();

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	stop_and_ask();

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_loading_modules);
# endif

	/* load the kernel modules */
	load_all_modules(curpath, (load_xdd_f | (load_xfs_f << 1)));

	/* Make newline */
	boot_print("\r\n");

	stop_and_ask();

	/* start system update daemon */
# ifdef VERBOSE_BOOT
	boot_print(MSG_init_starting_sysupdate);
# endif
	start_sysupdate();

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	stop_and_ask();

	/* load the configuration file */
	load_config();

	stop_and_ask();

# ifdef OLDTOSFS
	/*
	 * Until the old TOSFS is completely removed, we try to trigger a media
	 * change on each drive that has NEWFATFS enabled, but is already in
	 * control of TOSFS. This is done with d_lock() and will silently fail
	 * if there are open files on a drive.
	 */
	{
		ushort	i;
		char	cwd[PATH_MAX] = "X:";

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if ((drives[i] == &tos_filesys) &&
			    (fatfs_config(i, FATFS_DRV, ASK) == ENABLE))
			{
				/* We have to preserve the current directory,
				 * as d_lock() will reset it to \
				 */
				cwd[0] = i + ((i < 26) ? 'A' : '1' - 26);
				if (d_getcwd(cwd + 2, i + 1, PATH_MAX - 2) != E_OK)
				{
					continue;
				}
				if (d_lock (1, i) == E_OK)
				{
					d_lock(0, i);
					d_setpath(cwd);
				}
			}
		}
	}
# endif

	if (init_env == 0)
		init_env = (char *) _base->p_env;

	/* empty environment?
	 * Set the PATH variable to the root of the current drive
	 */
	if (init_env[0] == 0)
	{
		static char path_env[] = "PATH=c:/";
		path_env[5] = rootproc->p_cwd->curdrv + 'a';	/* this actually means drive u: */
		init_env = path_env;
	}

	/* if we are MultiTOS, we're running in the AUTO folder, and our INIT
	 * is in fact GEM, take the exec_os() vector. (We know that INIT
	 * is GEM if the user told us so by using GEM= instead of INIT=.)
	 */
	if (init_is_gem && init_prg)
	{
		xbra_install(&old_execos, EXEC_OS, (long _cdecl (*)())do_exec_os);
	}

# ifdef PROFILING
	/* compiled with profiling support */
	monstartup(_base->p_tbase, (_base->p_tbase + _base->p_tlen));
# endif

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_pid_0);
# endif

	save_context(&(rootproc->ctxt[CURRENT]));
	save_context(&(rootproc->ctxt[SYSCALL]));

	/* zero the user registers, and set the FPU in a "clear" state */
	for (r = 0; r < 15; r++)
	{
		rootproc->ctxt[CURRENT].regs[r] = 0;
		rootproc->ctxt[SYSCALL].regs[r] = 0;
	}

	rootproc->ctxt[CURRENT].fstate[0] = 0;
	rootproc->ctxt[CURRENT].pc = (long) mint_thread;
	rootproc->ctxt[CURRENT].usp = rootproc->sysstack;
	rootproc->ctxt[CURRENT].ssp = rootproc->sysstack;
	rootproc->ctxt[CURRENT].term_vec = (long) rts;

	rootproc->ctxt[SYSCALL].fstate[0] = 0;
	rootproc->ctxt[SYSCALL].pc = (long) mint_thread;
	rootproc->ctxt[SYSCALL].usp = rootproc->sysstack;
	rootproc->ctxt[SYSCALL].ssp = (long)(rootproc->stack + ISTKSIZE);
	rootproc->ctxt[SYSCALL].term_vec = (long) rts;

	*((long *)(rootproc->ctxt[CURRENT].usp + 4)) = 0;
	*((long *)(rootproc->ctxt[SYSCALL].usp + 4)) = 0;

	restore_context(&(rootproc->ctxt[CURRENT]));

	/* not reached */
	FATAL("restore_context() returned ???");
}

/*
 * run programs in the AUTO folder that appear after MINT.PRG
 * some things to watch out for:
 * (1) make sure GEM isn't active
 * (2) make sure there really is a MINT.PRG in the auto folder
 */

INLINE void
run_auto_prgs (void)
{
	DTABUF *dta;
	long r;
 	short curdriv, runthem = 0;		/* set to 1 after we find MINT.PRG */
	char pathspec[32], curpath[PATH_MAX];

	/* OK, now let's run through \\AUTO looking for
	 * programs...
	 */
	sys_d_getpath(curpath,0);
	curdriv = sys_d_getdrv();

	sys_d_setdrv(sysdrv);	/* set above, right after Super() */
	sys_d_setpath("/");

	dta = (DTABUF *) sys_f_getdta();
	r = sys_f_sfirst("/auto/*.prg", 0);
	while (r >= 0)
	{
		if (strcmp(dta->dta_name, my_name) == 0)
		{
			runthem = 1;
		}
		else if (runthem)
		{
			ksprintf(pathspec, sizeof(pathspec), "/auto/%s", dta->dta_name);
			sys_pexec(0, pathspec, (char *)"", init_env);
		}
		r = sys_f_snext();
	}

 	sys_d_setdrv(curdriv);
 	sys_d_setpath(curpath);
}

void
mint_thread(void *arg)
{
	int pid;
	long r;

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	/* Load the keyboard table */
	load_keytbl();

	/* Load the unicode table */
# ifdef SOFT_UNITABLE
	init_unicode();
# endif

	/* run any programs appearing after us in the AUTO folder */
	if (load_auto)
		run_auto_prgs();

	/* prepare to run the init program as PID 1. */
	set_pid_1();

	/* run the initial program
	 *
	 * if that program is in fact GEM, we start it via exec_os, otherwise
	 * we do it with Pexec.
	 * the logic is: if the user specified init_prg, and it is not
	 * GEM, then we try to execute it; if it *is* GEM (e.g. gem.sys),
	 * then we try to execute it if gem is already active, otherwise
	 * we jump through the exec_os vector (which we grabbed above) in
	 * order to start it. We *never* go through exec_os if we're not in
	 * the AUTO folder.
	 */
	if (init_prg)
	{
# ifdef VERBOSE_BOOT
		boot_printf(MSG_init_launching_init, init_is_gem ? "GEM" : "init", init_prg);
# endif
		if (!init_is_gem)
		{
# if 1
			r = sys_pexec(100, init_prg, init_tail, init_env);
# else
			r = sys_pexec(0, init_prg, init_tail, init_env);
# endif
			DEBUG(("init_prg done!"));
		}
		else
		{
			BASEPAGE *bp;

			bp = (BASEPAGE *)sys_pexec(7, (char *)GEM_memflags, (char *)"\0", init_env);
			bp->p_tbase = *((long *) EXEC_OS);

			r = sys_pexec(106, (char *)"GEM", bp, 0L);
		}
# ifdef VERBOSE_BOOT
		if (r >= 0)
			boot_print(MSG_init_done);
		else
			boot_printf(MSG_init_error, r);
# endif
	}
	else
	{	/* "GEM=ROM" sets init_prg == 0 and init_is_gem == 1 -> run rom AES */
	  	if (init_is_gem)
	  	{
	  		BASEPAGE *bp;
			long entry;
# ifdef VERBOSE_BOOT
			boot_print(MSG_init_rom_AES);
# endif
			entry = *((long *) EXEC_OS);
			bp = (BASEPAGE *) sys_pexec (7, (char *) GEM_memflags, (char *) "\0", init_env);
			bp->p_tbase = entry;

			r = sys_pexec(106, (char *) "GEM", bp, 0L);
	  	}
	  	else
	  	{
# ifdef VERBOSE_BOOT
			boot_print(MSG_init_no_init_specified);
# endif
			r = 0;
		}
	}

	/* r < 0 means an error during sys_pexec() execution (e.g. file not found);
	 * r == 0 means that mint.cnf lacks the GEM= or INIT= line.
	 *
	 * In both cases we halt the system, but before that we will want
	 * to execute some sort of shell that could help to fix minor fs problems
	 * without rebooting to TOS.
	 */
	if (r <= 0)
	{
		char *ext, shellpath[32];	/* 32 should be plenty */
		fcookie dir;

		/* Last resort: try to execute sysdir/sh.tos.
		 * For that, the absolute path must be used, because the user
		 * could have changed the current drive/dir inside mint.cnf file.
		 */
		path2cookie(sysdir, NULL, &dir);
		ext = (dir.fs->fsflags & FS_NOXBIT) ? ".tos" : "";
		release_cookie(&dir);

		ksprintf(shellpath, sizeof(shellpath), "%ssh%s", sysdir, ext);

# ifdef VERBOSE_BOOT
		boot_printf(MSG_init_starting_shell, shellpath);
# endif
		r = sys_pexec(100, shellpath, init_tail, init_env);

# ifdef VERBOSE_BOOT
		if (r > 0)
			boot_print(MSG_init_done);
		else
			boot_printf(MSG_init_error, r);
# endif

# ifdef BUILTIN_SHELL
# ifdef VERBOSE_BOOT
		boot_print(MSG_init_starting_internal_shell);
# endif
		if (r <= 0)
			r = startup_shell();	/* r is the shell's pid */
# endif
	}

	/* Here we have the code for the process 0 (MiNT itself).
	 *
	 * It waits for the init program to terminate. When it is checked that
	 * the init program is still alive, the pid 0 goes to sleep.
	 * It can only be awaken, when noone else is ready to run, sleep()
	 * in proc.c moves it to the run queue then (q.v.). In this case,
	 * instead of calling sleep() again and force the CPU to execute the
	 * loop again and again, we request it to be stopped.
	 *
	 */
	pid = (int) r;
	if (pid > 0)
	{
		do
		{
# if 1
			r = sys_pwaitpid(-1, 1, NULL);
			if (r == 0)
			{
				sleep(WAIT_Q, (long)mint_thread);
				/* LPSTOP doesn't work (at least on Milan060)
				 * if (mcpu == 60)
				 *	cpu_lpstop();
				 * else */
					cpu_stop();	/* stop and wait for an interrupt */
			}
# else
			r = sys_pwaitpid(-1, 0, NULL);
			TRACE(("sys_pwaitpid done -> %li (%li)", r, ((r & 0xffff0000L) >> 16)));
# endif
		} while (pid != ((r & 0xffff0000L) >> 16));
	}
# ifndef DEBUG_INFO
	else
		sys_s_hutdown(SHUT_HALT);		/* Everything failed. Halt. */

	/* If init program exited, reboot the system.
	 * Never go back to TOS.
	 */
	(void) sys_s_hutdown(SHUT_COLD);	/* cold reboot is more efficient ;-) */
# else
	/* With debug kernels, always halt
	 */
	(void) sys_s_hutdown(SHUT_HALT);
# endif

	/* Never returns */
}
