/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

# include "init.h"
# include "global.h"

# include "buildinfo/version.h"
# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/xbra.h"

# include "arch/cpu.h"		/* init_cache, cpush, setstack */
# include "arch/detect.h"
# include "arch/init_mach.h"	/* getmch */
# include "arch/intr.h"		/* new_mediach, new_rwabs, new_getbpb, same for old_ */
# include "arch/kernel.h"	/* enter_gemdos */
# include "arch/mmu.h"		/* save_mmu */
# include "arch/mprot.h"
# include "arch/syscall.h"	/* call_aes */

# include "bios.h"	
# include "block_IO.h"	/* init_block_IO */
# include "cnf.h"	/* load_config, some variables */
# include "console.h"	
# include "cookie.h"	
# include "crypt_IO.h"	/* init_crypt_IO */
# include "delay.h"	/* calibrate_delay */
# include "dos.h"	
# include "dosdir.h"	
# include "dosfile.h"	
# include "dosmem.h"	
# include "filesys.h"	/* init_filesys, s_ync, load_*, close_filesys */
# include "gmon.h"	/* monstartup */
# include "info.h"	/* welcome messages */
# include "kmemory.h"	/* kmalloc */
# include "memory.h"	/* init_mem, get_region, attach_region, restr_screen */
# include "proc.h"	/* init_proc, add_q, rm_q */
# include "signal.h"	/* post_sig */
# include "syscall_vectors.h"
# include "time.h"	
# include "timeout.h"	
# include "update.h"	/* start_sysupdate */
# include "util.h"	
# include "fatfs.h"	/* fatfs_config() */
# include "tosfs.h"	/* tos_filesys */
# include "xbios.h"	/* has_bconmap, curbconmap */


/* if the user is holding down the magic shift key, we ask before booting */
# define MAGIC_SHIFT	0x2		/* left shift */

/* magic number to show that we have captured the reset vector */
# define RES_MAGIC	0x31415926L

static long _cdecl mint_criticerr (long);
static void _cdecl do_exec_os (register long basepage);

static int gem_active;		/* 0 if AES has not started, nonzero otherwise */

# define EXEC_OS 0x4feL
static int	check_for_gem (void);
static void	run_auto_prgs (void);
static int	boot_kernel_p (void);

unsigned long bogomips[2];	/* Calculated BogoMIPS units and fraction */

/* print a additional boot message
 */

void
boot_print (const char *s)
{
	Cconws (s);	
}

# include <stdarg.h>

void
boot_printf (const char *fmt, ...)
{
	char buf [SPRINTF_MAX];
	va_list args;
	
	va_start (args, fmt);
	vsprintf (buf, sizeof (buf), fmt, args);
	va_end (args);
	
	Cconws (buf);
}


# ifndef MULTITOS
BASEPAGE *gem_base = 0;
long gem_start = 0;
# endif

/* GEMDOS pointer to current basepage */
BASEPAGE **tosbp;

/* structures for keyboard/MIDI interrupt vectors */
KBDVEC *syskey, oldkey;

xbra_vec old_criticerr;
xbra_vec old_execos;

# ifdef EXCEPTION_SIGS
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
# endif

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
	return (*curproc->criticerr)(error);
}

/*
 * if we are MultiTOS, and if we are running from the AUTO folder,
 * then we grab the exec_os vector and use that to start GEM; that
 * way programs that expect exec_os to act a certain way will still
 * work.
 * NOTE: we must use Pexec instead of p_exec here, because we will
 * be running in a user context (that of process 1, not process 0)
 */

static void _cdecl
do_exec_os (long basepage)
{
	register long r;

	/* if the user didn't specify a startup program, jump to the ROM */
	if (!init_prg)
	{
		register void _cdecl (*f)(long);
		f = (void _cdecl (*)(long)) old_execos.next;
		(*f)(basepage);
		Pterm0();
	}
	else
	{
		/* we have to set a7 to point to lower in our TPA;
		 * otherwise we would bus error right after the Mshrink call!
		 */
		setstack(basepage+500L);
# if defined(__TURBOC__) && !defined(__MINT__)
		Mshrink (0, (void *)basepage, 512L);
# else
		Mshrink ((void *)basepage, 512L);
# endif
		r = Pexec(200, (char *) init_prg, init_tail, init_env);
		Pterm ((int)r);
	}
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
	
	syskey = (KBDVEC *) Kbdvbase ();
	oldkey = *syskey;
	
	new_xbra_install (&old_ikbd, (long) (&syskey->ikbdsys), new_ikbd);
	
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
	
# ifdef TRAPS_PRIVATE
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
# else
	new_xbra_install (&old_dos, 0x84L, mint_dos);		/* trap #1, GEMDOS */
	new_xbra_install (&old_bios, 0xb4L, mint_bios);		/* trap #13, BIOS */
	new_xbra_install (&old_xbios, 0xb8L, mint_xbios);	/* trap #14, XBIOS */
# endif
	
	new_xbra_install (&old_timer, 0x400L, mint_timer);
	xbra_install (&old_criticerr, 0x404L, mint_criticerr);
	new_xbra_install (&old_5ms, 0x114L, mint_5ms);
	new_xbra_install (&old_vbl, 4*0x1cL, mint_vbl);
	
	new_xbra_install (&old_resvec, 0x042aL, reset);
	old_resval = *((long *)0x426L);
	*((long *) 0x426L) = RES_MAGIC;
	
	spl (savesr);
	
	/* set up signal handlers */
	xbra_install (&old_bus, 8L, new_bus);
# ifdef EXCEPTION_SIGS
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
# endif
	
	/* set up disk vectors */
	new_xbra_install (&old_mediach, 0x47eL, new_mediach);
	new_xbra_install (&old_rwabs, 0x476L, new_rwabs);
	new_xbra_install (&old_getbpb, 0x472L, new_getbpb);
	olddrvs = *((long *) 0x4c2L);
	
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

# ifdef VM_EXTENSION
extern void Reset_mmu(void);	/* in vm.s */
# endif

void
restr_intr (void)
{
	ushort savesr;
	int i;
	
	savesr = spl7 ();
	*syskey = oldkey;	/* restore keyboard vectors */
	*tosbp = _base;		/* restore GEMDOS basepage pointer */
	
	restr_cookies ();
	
	*((long *) 0x008L) = (long) old_bus.next;
	
# ifdef EXCEPTION_SIGS
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
# endif
	
	*((long *) 0x084L) = old_dos;
	*((long *) 0x0b4L) = old_bios;
	*((long *) 0x0b8L) = old_xbios;
	*((long *) 0x408L) = old_term;
	*((long *) 0x404L) = (long) old_criticerr.next;
	*((long *) 0x114L) = old_5ms;
	*((long *) 0x400L) = old_timer;
	*((long *) 0x070L) = old_vbl;
	*((long *) 0x426L) = old_resval;
	*((long *) 0x42aL) = old_resvec;
	*((long *) 0x476L) = old_rwabs;
	*((long *) 0x47eL) = old_mediach;
	*((long *) 0x472L) = old_getbpb;
	*((long *) 0x4c2L) = olddrvs;
	
# ifdef VM_EXTENSION
	if (vm_in_use)
		Reset_mmu ();
# endif
	
	spl (savesr);
}

# ifdef AUTO_FIX
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
# if 1
			/* DEBUGGING: */
			boot_printf ("[MiNT is named \\AUTO\\%s]\r\n", p);
# endif
		}
	}
}
# endif

/* Boot menu routines. Quite lame I admit. Added 19.X.2000. */

# ifdef BOOT_MENU
static short load_xfs;
static short load_xdd;
static short load_auto;
static short save_ini;
static const char *ini_keywords[] =
		{ "XFS_LOAD", "XDD_LOAD", \
		  "AUTOLOAD", "MEM_PROT", "INI_SAVE" };

/* we assume that the new mint.ini file, containing the boot
 * menu defaults, is located at same place as mint.cnf is
 */

static char *
find_ini(void)
{
	extern char *cnf_path_1, *cnf_path_2, *cnf_path_3;

	if (Fsfirst(cnf_path_1, 0) == 0)
		return "mint.ini";
	if (Fsfirst(cnf_path_2, 0) == 0)
		return "\\multitos\\mint.ini";
	if (Fsfirst(cnf_path_3, 0) == 0)
		return "\\mint\\mint.ini";

	return 0;
}

static char *
find_char(char *s, char c)
{
	short f = 0;

	while (*s) {
		if ((*s == '\n') || (*s == '\r'))
			break;
		if (*s == c) {
			f = 1;
			break;
		}
		s++;
	}
	if (!f)
		return 0;
	return s;
}

static short
whether_yes(char *s)
{
	s = find_char(s, '=');
	if ((long)s == 0)
		return 0;
	s = find_char(s, 'Y');	/* don't add 'y' here, see below */
	if ((long)s == 0)
		return 0;
	return 1;
}

static void
read_ini(void)
{
	DTABUF *dta;
	char *buf, *s, *ini_file = find_ini();
	long r, x, len;
	short inihandle, options[5] = { 1, 1, 1, 1, 1 };

	if ((long)ini_file == 0)
		goto initialize;

	/* Figure out the file's length. Wish I had Fstat() here :-( */
	dta = (DTABUF *)Fgetdta();
	r = Fsfirst(ini_file, 0);
	if (r < 0)
		goto initialize;	/* No such file, probably */
	len = dta->dta_size;
	if (len < 10)
		goto initialize;	/* proper mint.ini can't be so short */
	len++;

	buf = (char *)Mxalloc(len, 0x0003);
	if ((long)buf == -32L)	
		buf = (char *)Malloc(len);	/* No Mxalloc()? */
	if ((long)buf == 0)
		goto initialize;	/* Out of memory or such */
	bzero(buf, len);

	inihandle = Fopen(ini_file, 0);
	if (inihandle < 0)
		goto exit;
	r = Fread(inihandle, len, buf);
	if (r < 0)
		goto close;

	strupr(buf);
	for (x = 0; x < 5; x++) {
		s = strstr(buf, ini_keywords[x]);
		if ((long)s)
			options[x] = whether_yes(s);
	}

close:
	(void)Fclose(inihandle);
exit:
	Mfree((long)buf);

initialize:
	load_xfs = options[0];
	load_xdd = options[1];
	load_auto = options[2];
	no_mem_prot = !options[3];
	save_ini = options[4];
}

static void
write_ini(short *options)
{
	extern char *ini_warn;
	short inihandle;
	char *ini_file = find_ini(), temp[256];
	long r, x, l;

	if ((long)ini_file == 0)
		return;
 
	inihandle = Fcreate(ini_file, 0);
	if (inihandle < 0)
		return;

	options++;		/* Ignore the first one :-) */

	l = strlen(ini_warn);
	r = Fwrite(inihandle, l, ini_warn);
	if ((r < 0) || (r != l)) {
		r = -1;
		goto close;
	}

	for (x = 0; x < 5; x++) {
		ksprintf(temp, 256, "%s=%s\n", \
			ini_keywords[x], options[x] ? "YES" : "NO");
		l = strlen(temp);
		r = Fwrite(inihandle, l, temp);
		if ((r < 0) || (r != l)) {
			r = -1;
			break;
		}
	}
close:
	(void)Fclose(inihandle);

	if (r < 0)
		(void)Fdelete(ini_file);
}		
# endif

static int
boot_kernel_p (void)
{
# ifdef BOOT_MENU
	extern const char *startmenu;	/* in info.c */
	char menu[512];
	short option[6];
	long c = 0;
	static struct yn_message
	{
		const char *message;	/* message to print */
		char	yes_let;	/* letter to hit for yes */
		char	no_let;		/* letter to hit for no */
	}
	/* Please change messages to appropriate languages */
	boot_it [MAXLANG] =
	{
		{ "Display the boot menu? (y)es (n)o ",  'y', 'n' },	/* English */
		{ "Das Bootmenu anzeigen? (j)a (n)ein ", 'j', 'n' },	/* German */
		{ "Display the boot menu? (o)ui (n)on ", 'o', 'n' },	/* French */
		{ "Display the boot menu? (y)es (n)o ",  'y', 'n' },	/* reserved */
		{ "¨Display the boot menu? (s)i (n)o ",  's', 'n' },	/* Spanish, upside down ? is 168 dec. */
		{ "Display the boot menu? (s)i (n)o ",   's', 'n' }	/* Italian */
	};
	
	struct yn_message *msg = &boot_it [gl_lang];
	int y;
	
	Cconws (msg->message);
	y = (int) Cconin ();
	if (tolower (y) == msg->no_let)
		return 1;

	/* English only from here, sorry */

	option[0] = 1;			/* Load MiNT or not */
	option[1] = load_xfs;		/* Load XFS or not */
	option[2] = load_xdd;		/* Load XDD or not */
	option[3] = load_auto;		/* Load AUTO or not */
	option[4] = !no_mem_prot;	/* Use memprot or not */
	option[5] = save_ini;

	for (;;) {
		ksprintf(menu, 512, startmenu, \
			option[0] ? "yes\r\n" : "no\r\n", \
			option[1] ? "yes\r\n" : "no\r\n", \
			option[2] ? "yes\r\n" : "no\r\n", \
			option[3] ? "yes\r\n" : "no\r\n", \
			option[4] ? "yes\r\n" : "no\r\n", \
			option[5] ? "yes\r\n" : "no\r\n" );
		Cconws(menu);
wait:
		c = Crawcin();
		c &= 0x7f;
		switch(c)
		{
			case 0x03:
				return 1;
			case 0x0a:
			case 0x0d:
				load_xfs = option[1];
				load_xdd = option[2];
				load_auto = option[3];
				no_mem_prot = !option[4];
				save_ini = option[5];
				if (save_ini)
					write_ini(option);
				return (int)option[0];
			case '0':
				option[5] = option[5] ? 0 : 1;
				break;
			case '1':
				option[0] = option[0] ? 0 : 1;
				break;
			case '2':
				option[1] = option[1] ? 0 : 1;
				break;
			case '3':
				option[2] = option[2] ? 0 : 1;
				break;
			case '4':
				option[3] = option[3] ? 0 : 1;
				break;
			case '5':
				option[4] = option[4] ? 0 : 1;
				break;
			default:
				goto wait;
		}
	}
	return 1;	/* not reached */

# else
	/* ask the user whether wants to boot MultiTOS;
 	 * returns 1 if yes, 0 if no
 	 */
	/* "boot MiNT?" messages, in various langauges:
	 */
	static struct yn_message
	{
		const char *message;	/* message to print */
		char	yes_let;	/* letter to hit for yes */
		char	no_let;		/* letter to hit for no */
	}
	boot_it [MAXLANG] =
	{
		{ "Load " MINT_NAME "?   (y)es (n)o ",     'y', 'n' },
		{ MINT_NAME " laden?   (j)a (n)ein ",      'j', 'n' },
		{ "Charger " MINT_NAME "?   (o)ui (n)on ", 'o', 'n' },
		{ "Load " MINT_NAME "?   (y)es (n)o ",     'y', 'n' },	/* reserved */
		{ "¨Cargar " MINT_NAME "?   (s)i (n)o ",   's', 'n' },	/* upside down ? is 168 dec. */
		{ "Carica " MINT_NAME "?   (s)i (n)o ",    's', 'n' }
	};
	
	struct yn_message *msg = &boot_it [gl_lang];
	int y;
	
	Cconws (msg->message);
	y = (int) Cconin ();
	if (tolower (y) == msg->yes_let)
		return 1;
	else
		return 0;
# endif
}


# ifndef MULTITOS
long GEM_memflags = F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_S | F_ALLOCZERO;
# else
long GEM_memflags = F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_S;
# endif


void
init (void)
{
	static char curpath[128];
	
	long *sysbase;
	long r;
	FILEPTR *f;
	
# ifdef VM_EXTENSION
	int disable_vm = 0;
# endif
# ifdef BOOT_MENU
	read_ini();	/* Read user defined defaults */
# endif	
	/* greetings (placed here 19960610 cpbs to allow me to get version
	 * info by starting MINT.PRG, even if MiNT's already installed.)
	 */
	boot_print (greet1);
	boot_print (greet2);
	
	/* figure out what kind of machine we're running on:
	 * - biosfs wants to know this
	 * - also sets no_mem_prot
	 * - 1992-06-25 kbad put it here so memprot_warning can be intelligent
	 */
	if (getmch ())
	{
		boot_print ("Hit any key to continue.\r\n");
		(void) Cconin ();
		Pterm0 ();
	}
	
# ifdef ONLY030
# ifndef MMU040
	if (mcpu < 20)
# else
	if (mcpu < 40)
# endif
	{
# ifndef MMU040
		boot_print ("\r\nThis version of MiNT requires a 68020-68060.\r\n");
# else
		boot_print ("\r\nThis version of MiNT requires a 68040-68060.\r\n");
# endif
		boot_print ("Hit any key to continue.\r\n");
		(void) Cconin ();
		Pterm0 ();
	}
# endif
	
	/* Ask the user if s/he wants to boot MiNT */
	if ((Kbshift (-1) & MAGIC_SHIFT) == MAGIC_SHIFT)
	{
		long yn = boot_kernel_p ();
		boot_print ("\r\n");
		if (!yn)
			Pterm0 ();
	}
	
# if 0
	/* if less than 1 megabyte free, turn off memory protection */
	if (Mxalloc (-1L, 3) < ONE_MEG && !no_mem_prot)
	{
		boot_print (insuff_mem_warning);
		no_mem_prot = 1;
	}
# endif
# if 0
	/* look for ourselves as \AUTO\MINTNP.PRG; if so, we turn memory
	 * protection off
	 */
	if (!no_mem_prot && Fsfirst ("\\AUTO\\MINTNP.PRG", 0) == 0)
		no_mem_prot = 1;
# endif
	
# ifdef OLDTOSFS
	/* Get GEMDOS version from ROM for later use by our own Sversion() */
	gemdos_version = Sversion ();
# endif	
	
	/* check for GEM -- this must be done from user mode */
	gem_active = check_for_gem ();
	
# ifdef AUTO_FIX
# ifdef BOOT_MENU
	if (!gem_active)
		get_my_name();
# else
	/* only useful if we're in the AUTO folder... */
	if (!gem_active)
	{
		get_my_name ();
		
# ifdef VM_EXTENSION		
		/* turn off vm and mem prot if name
		 * matches "MINTNM*.*" or "MNTNM*.*"
		 */
		if (!disable_vm)
		{
			if (strncmp (my_name, "MINTNM", 6) == 0
				|| strncmp (my_name, "MNTNM", 5) == 0)
			{
				disable_vm = 1;
				no_mem_prot = 1;
			}
		}
# endif		
		/* Turn off memory protection if our name matches either
		 * "MINTN*.PRG" or "MNTN*.PRG":
		 * (will enable vm)
		 */
		if (!no_mem_prot)
		{
				if (strncmp (my_name, "MINTN", 5) == 0
					|| strncmp (my_name, "MNTN", 4) == 0)
				{
					no_mem_prot = 1;
				}
		}
	}
# endif
# endif
# ifdef VERBOSE_BOOT
	boot_print ("Memory protection ");
	if (no_mem_prot)
		boot_print ("disabled\r\n");
	else
		boot_print ("enabled\r\n");
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
	
	(void) Super (0L);
	
	if (!no_mem_prot)
		save_mmu ();		/* save current MMU setup */
	
	/* get GEMDOS pointer to current basepage
	 * 
	 * 0x4f2 points to the base of the OS; here we can find the OS compilation
	 * date, and (in newer versions of TOS) where the current basepage pointer
	 * is kept; in older versions of TOS, it's at 0x602c
	 */
	sysbase = *((long **)(0x4f2L));	/* gets the RAM OS header */
	sysbase = (long *)sysbase[2];	/* gets the ROM one */

	tosvers = (int)(sysbase[0] & 0x0000ffff);
	falcontos = (tosvers >= 0x0400 && tosvers <= 0x0404) || (tosvers >= 0x0700);
	if (tosvers == 0x100)
	{
		if ((sysbase[7] & 0xfffe0000L) == 0x00080000L)
			tosbp = (BASEPAGE **) 0x873cL;	/* SPANISH ROM */
		else
			tosbp = (BASEPAGE **) 0x602cL;
		kbshft = (char *) 0x0e1bL;
	}
	else
	{
		tosbp = (BASEPAGE **) sysbase[10];
		kbshft = (char *) sysbase[9];
	}

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
		if (has_bconmap)
			bconmap2 = (BCONMAP2_T *) Bconmap (-2);
	}
	
# ifdef VM_EXTENSION
	if ((mcpu == 30) && (no_mem_prot) && (!disable_vm))
	{
		/* use vm */
		vm_in_use = 1;
	}
# endif
	
	/* initialize cache */
	init_cache ();
	DEBUG (("%s, %ld: init_cache() ok!", __FILE__, (long) __LINE__));
	
	/* initialize memory */
	init_mem ();
	DEBUG (("%s, %ld: init_mem() ok!", __FILE__, (long) __LINE__));
	
	/* Initialize high-resolution calendar time */
	init_time ();
	DEBUG (("%s, %ld: init_time() ok!", __FILE__, (long) __LINE__));
	
	/* initialize buffered block I/O */
	init_block_IO ();
	DEBUG (("%s, %ld: init_block_IO() ok!", __FILE__, (long) __LINE__));
	
	/* initialize crypto I/O layer */
	init_crypt_IO ();
	DEBUG (("%s, %ld: init_crypt_IO() ok!", __FILE__, (long) __LINE__));
	
	/* initialize the basic file systems */
	init_filesys ();
	DEBUG (("%s, %ld: init_filesys() ok!", __FILE__, (long) __LINE__));
	
	/* initialize processes */
	init_proc();
	DEBUG (("%s, %ld: init_proc() ok! (base = %lx)", __FILE__, (long) __LINE__, _base));
	
	/* initialize system calls */
	init_dos ();
	DEBUG (("%s, %ld: init_dos() ok!", __FILE__, (long) __LINE__));
	init_bios ();
	DEBUG (("%s, %ld: init_bios() ok!", __FILE__, (long) __LINE__));
	init_xbios ();
	DEBUG (("%s, %ld: init_xbios() ok!", __FILE__, (long) __LINE__));
	
	/* initialize interrupt vectors */
	init_intr ();
	DEBUG (("%s, %ld: init_intr() ok!", __FILE__, (long) __LINE__));
	
	/* set up cookie jar */
	init_cookies ();
	DEBUG (("%s, %ld: init_cookies() ok!", __FILE__, (long) __LINE__));
	
	
	/* add our pseudodrives */
	*((long *) 0x4c2L) |= PSEUDODRVS;
	
	/* we'll be making GEMDOS calls */
	enter_gemdos ();
	
	/* set up standard file handles for the current process
	 * do this here, *after* init_intr has set the Rwabs vector,
	 * so that AHDI doesn't get upset by references to drive U:
	 */
	f = do_open ("U:\\DEV\\CONSOLE", O_RDWR, 0, NULL, NULL);
	if (!f)
		FATAL ("unable to open CONSOLE device");
	
	curproc->control = f;
	curproc->handle[0] = f;
	curproc->handle[1] = f;
	f->links = 3;
	
	f = do_open ("U:\\DEV\\MODEM1", O_RDWR, 0, NULL, NULL);
	if (!f)
		FATAL ("unable to open MODEM1 device");
	
	curproc->aux = f;
	((struct tty *) f->devinfo)->aux_cnt = 1;
	f->pos = 1;	/* flag for close to --aux_cnt */
	
	if (has_bconmap)
	{
		/* If someone has already done a Bconmap call, then
		 * MODEM1 may no longer be the default
		 */
		bconmap (curbconmap);
		f = curproc->aux;	/* bconmap can change curproc->aux */
	}
	
	if (f)
	{
		curproc->handle[2] = f;
		f->links++;
	}
	
	f = do_open ("U:\\DEV\\CENTR", O_RDWR, 0, NULL, NULL);
	if (f)
	{
		curproc->handle[3] = curproc->prn = f;
		f->links = 2;
	}
	
	f = do_open ("U:\\DEV\\MIDI", O_RDWR, 0, NULL, NULL);
	if (f)
	{
		curproc->midiin = curproc->midiout = f;
		((struct tty *) f->devinfo)->aux_cnt = 1;
		f->pos = 1;	/* flag for close to --aux_cnt */
		f->links = 2;
	}
	
	
	/* print warning message if MP is turned off */
	if (no_mem_prot && mcpu > 20)
		c_conws (memprot_warning);
	
	
	/* initialize delay */
	c_conws ("Calibrating delay loop... ");
	calibrate_delay ();
	/* Round the value and print it */
	{
		char buf[128];

		bogomips[0] = (loops_per_sec + 2500) / 500000;
		bogomips[1] = ((loops_per_sec + 2500) / 5000) % 100;

		ksprintf (buf, sizeof (buf), "%lu.%02lu BogoMIPS\r\n\r\n",
			bogomips[0], bogomips[1]);
		c_conws (buf);
	}
	
	/* initialize internal xdd */
# ifdef DEV_RANDOM
	c_conws (random_greet);
# endif
	
	
	/* load external modules
	 * 
	 * set path first to make sure that MiNT's directory matches
	 * GEMDOS's
	 */
	d_setpath (curpath);
	
	/* load external xdd */
# ifdef BOOT_MENU
	if (load_xdd)
# endif
		load_modules (0);
	
	/* reset curpath just to be sure */
	d_setpath (curpath);
	
	/* load external xfs */
# ifdef BOOT_MENU
	if (load_xfs)
# endif
		load_modules (1);
	
	/* reset curpath just to be sure */
	d_setpath (curpath);
	
	
	/* start system update daemon */
	start_sysupdate ();
	
	/* load the configuration file */
	load_config ();
	
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
				(fatfs_config (i, FATFS_DRV, ASK) == ENABLE))
			{
				/* We have to preserve the current directory,
				 * as d_lock() will reset it to \
				 */
				cwd[0] = i + ((i < 26) ? 'A' : '1' - 26);
				if (d_getcwd (cwd + 2, i + 1, PATH_MAX - 2) != E_OK)
				{
					continue;
				}
				if (d_lock (1, i) == E_OK)
				{
					d_lock (0, i);
					d_setpath (cwd);
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
		static char path_env[] = "PATH=\0C:\0";
		path_env[6] = curproc->curdrv + 'A';
		init_env = path_env;
	}
	
	/* if we are MultiTOS, we're running in the AUTO folder, and our INIT
	 * is in fact GEM, take the exec_os() vector. (We know that INIT
	 * is GEM if the user told us so by using GEM= instead of INIT=.)
	 */
	if (!gem_active && init_is_gem)
	{
		xbra_install (&old_execos, EXEC_OS, (long _cdecl (*)())do_exec_os);
	}
	
	/* run any programs appearing after us in the AUTO folder */
# ifdef BOOT_MENU
	if (load_auto)
# endif
		run_auto_prgs ();

	/* prepare to run the init program as PID 1. */
	set_pid_1 ();
	
# ifdef PROFILING
	/* compiled with profiling support */
	monstartup (_base->p_tbase, (_base->p_tbase + _base->p_tlen));
# endif
	
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
	if (init_prg && (!init_is_gem || gem_active))
	{
		r = p_exec (0, (char *) init_prg, init_tail, init_env);
	}
	else if (!gem_active)
	{   
		BASEPAGE *bp; int pid;
		bp = (BASEPAGE *) p_exec (7, (char *) GEM_memflags, (char *) "\0", init_env);
		bp->p_tbase = *((long *) EXEC_OS);
# ifndef MULTITOS
		if (((long *) sysbase[5])[0] == 0x87654321)
			gem_start = ((long *) sysbase[5])[2];
		gem_base = bp;
# endif
		r = p_exec (106, (char *) "GEM", bp, 0L);
		pid = (int) r;
		if (pid > 0)
		{
			do {
				r = p_wait3(0, (long *)0);
			}
			while (pid != ((r & 0xffff0000L) >> 16));
			r &= 0x0000ffff;
		}
	}
	else
	{
		boot_print ("If MiNT is run after GEM starts, you must specify a program\r\n");
		boot_print ("to run initially in MINT.CNF, with an INIT= line\r\n");
		r = 0;
	}
	
	if (r < 0 && init_prg)
	{
		boot_printf ("FATAL: couldn't run `%s'.\r\n", init_prg);
		boot_printf ("exit code: %ld\r\n", r);
	}
	
	rootproc->base = _base;
	
# ifndef DEBUG_INFO
	/* On r < 0 (error executing init) perform a halt
	 * else reboot the system. Never go back to TOS.
	 */ 
	(void) s_hutdown ((r < 0 && init_prg) ? 0 : 1);
# else
	/* With debug kernels, always halt
	 */
	(void) s_hutdown (0);
# endif
	
	/* Never returns */	
}

/*
 * run programs in the AUTO folder that appear after MINT.PRG
 * some things to watch out for:
 * (1) make sure GEM isn't active
 * (2) make sure there really is a MINT.PRG in the auto folder
 */

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

static int
check_for_gem (void)
{
	call_aes (aes_pb);	/* does an appl_init */
	return aes_globl[0];
}

static void
run_auto_prgs (void)
{
	DTABUF *dta;
	long r;
	static char pathspec[32] = "\\AUTO\\";
	short runthem = 0;	/* set to 1 after we find MINT.PRG */
	char curpath[PATH_MAX];
 	int curdriv, bootdriv;
	
	/* if the AES is running, don't check AUTO */
	if (gem_active)
		return;
	
	/* OK, now let's run through \\AUTO looking for
	 * programs...
	 */
	d_getpath (curpath,0);
	curdriv = d_getdrv ();
	
	/* We are in Supervisor mode, so we can do this */
	bootdriv = *((short *) 0x446);
	d_setdrv (bootdriv);
	d_setpath ("\\");
	
	dta = (DTABUF *) f_getdta ();
	r = f_sfirst ("\\AUTO\\*.PRG", 0);
	while (r >= 0)
	{
# ifdef AUTO_FIX
		if (strcmp (dta->dta_name, my_name) == 0)
# else
		if (!strcmp (dta->dta_name, "MINT.PRG")
			|| !strcmp (dta->dta_name, "MINTNP.PRG" ))
# endif
		{
			runthem = 1;
		}
		else if (runthem)
		{
			strcpy (pathspec+6, dta->dta_name);
			p_exec (0, pathspec, (char *)"", init_env);
		}
		r = f_snext ();
	}
	
 	d_setdrv (curdriv);
 	d_setpath (curpath);
}
