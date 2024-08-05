/*
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
# include "mint/ssystem.h"

# include "arch/cpu.h"		/* init_cache, cpush, setstack */
# include "arch/context.h"	/* restore_context */
# include "arch/intr.h"		/* new_mediach, new_rwabs, new_getbpb, same for old_ */
# include "arch/init_intr.h"	/* init_intr() */
# include "arch/init_mach.h"	/* */
# include "arch/info_mach.h"	/* machine_str() */
# include "arch/mmu.h"		/* save_mmu */
# include "arch/mprot.h"	/* */
# include "arch/syscall.h"	/* call_aes */
# include "arch/timer.h"	/* delay_seconds() */
# include "arch/tosbind.h"

# include "bios.h"		/* */
# include "block_IO.h"		/* init_block_IO */
# include "bootmenu.h"		/* boot_kernel_p(), read_ini() */
# include "cnf_mint.h"		/* load_config, some variables */
# include "console.h"		/* */
# include "cookie.h"		/* restr_cookie */
# include "crypt_IO.h"		/* init_crypt_IO */
# include "delay.h"		/* calibrate_delay */
# include "dos.h"		/* */
# include "dosdir.h"		/* */
# include "dosmem.h"		/* QUANTUM */
# include "filesys.h"		/* init_filesys, s_ync, close_filesys */
# ifdef FLOPPY_ROUTINES
# include "floppy.h"		/* init_floppy */
# endif
# include "gmon.h"		/* monstartup */
# include "info.h"		/* welcome messages */
# include "ipc_socketutil.h"	/* domaininit() */
# include "k_exec.h"		/* sys_pexec */
# include "k_exit.h"		/* sys_pwaitpid */
# include "k_fds.h"		/* do_open/do_close */
# include "keyboard.h"		/* init_keytbl() */
# include "kmemory.h"		/* kmalloc */
# include "memory.h"		/* init_mem, get_region, attach_region, restr_screen */
# include "mis.h"		/* startup_shell */
# include "module.h"		/* load_all_modules */
# include "pcibios.h"		/* pcibios_init() */
# include "proc.h"		/* init_proc, add_q, rm_q */
# include "signal.h"		/* post_sig */
# include "syscall_vectors.h"
# include "time.h"		/* */
# include "timeout.h"		/* */
# include "unicode.h"		/* init_unicode() */
# include "update.h"		/* start_sysupdate */
# include "util.h"		/* */
# include "xbios.h"		/* has_bconmap, curbconmap */

# ifdef OLDTOSFS
# include "fatfs.h"		/* fatfs_config() */
# include "tosfs.h"		/* tos_filesys */
# endif

# define EXEC_OS	0x4feL

# define CACHE_OFF	0x00000000L
# define CACHE_ON	CTRLCACHE_EIC | CTRLCACHE_EDC | CTRLCACHE_EBC | CTRLCACHE_IBE | \
			CTRLCACHE_DBE | CTRLCACHE_FIC | CTRLCACHE_FOC | CTRLCACHE_DPI | \
			CTRLCACHE_ESB
# define CCW_MASK	CTRLCACHE_EIC | CTRLCACHE_EDC | CTRLCACHE_EBC | CTRLCACHE_FI  | \
			CTRLCACHE_FD  | CTRLCACHE_IBE | CTRLCACHE_DBE | CTRLCACHE_FIC | \
			CTRLCACHE_FOC | CTRLCACHE_DPI |	CTRLCACHE_ESB


long _cdecl mint_criticerr (long);
void _cdecl do_exec_os (register long basepage);

void mint_thread(void *arg);

static char bootdrv[3];

/* print an additional boot message
 */
short intr_done = 0;
void
boot_print (const char *s)
{
	if (intr_done)
		sys_c_conws(s);
	else
		(void)TRAP_Cconws (s);
}

void
boot_printf (const char *fmt, ...)
{
	char buf [512];
	va_list args;

	va_start (args, fmt);
	kvsprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	boot_print(buf);
}

/* Stop and ask the user for confirmation to continue */
short step_by_step = 0;

void
stop_and_ask(void)
{
	if (step_by_step == -1)
	{
		boot_print(MSG_init_hitanykey);

		if (intr_done)
			sys_c_conin();
		else
			TRAP_Cconin();
	}
	else if (step_by_step > 0)
		delay_seconds((ulong)step_by_step);
	/* else pass on */
}

/*
 * MiNT critical error handler (called from intr.S)
 */
long _cdecl
mint_criticerr (long error) /* high word is error, low is drive */
{
	/* just return with error */
	return (error >> 16);
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

static long GEM_memflags = F_FASTLOAD | F_ALTLOAD | F_ALTALLOC | F_PROT_S | F_OS_SPECIAL /*?*/;
extern int debug_level;

typedef struct _osheader
{
	ushort    os_entry;       /* BRAnch instruction to Reset-handler  */
  ushort    os_version;     /* TOS version number                   */
  void       *reseth;         /* Pointer to Reset-handler             */
  struct _osheader *os_beg;   /* Base address of the operating system */
  void       *os_end;         /* First byte not used by the OS        */
  ulong     os_rsvl;        /* Reserved                             */
 	void/*GEM_MUPB*/   *os_magic;       /* GEM memory-usage parameter block     */
  long     os_date;        /* TOS date (English !) in BCD format   */
  ushort    os_conf;        /* Various configuration bits           */
  ushort    os_dosdate;     /* TOS date in GEMDOS format            */

    /* The following components are available only as of TOS Version
       1.02 (Blitter-TOS)               */
  uchar    **p_root;         /* Base address of the GEMDOS pool      */
  uchar    **pkbshift;       /* Pointer to BIOS Kbshift variable
                                  (for TOS 1.00 see Kbshift)           */
  BASEPAGE  **p_run;          /* Address of the variables containing
                                 a pointer to the current GEMDOS
                                 process.                             */
  ulong     p_rsv2;         /* Reserved, always 'ETOS', if EmuTOS present     */
} OSHEADER;

short write_boot_file = 0;
char boot_file[48+12] = { 0 };
bool emutos = FALSE;

void
init (void)
{
	long r, *sysbase;
	OSHEADER *os;
	FILEPTR *f;

	/* greetings (placed here 19960610 cpbs to allow me to get version
	 * info by starting MINT.PRG, even if MiNT's already installed.)
	 */
	boot_print (greet1);
	boot_print (greet2);
	debug_level = ALERT_LEVEL;
	/*
	 * Initialize sysdir
	 *
	 * from 1.16 we ignore any multitos folder
	 * from 1.16 we default to \mint
	 * from 1.16 we search for \mint\<MINT_VERSION>
	 *           for example "\\mint\\1-16-0"   for 1.16.0 version
	 *                    or "\\mint\\1-16-cur" for cvs-current of 1.16 line
	 */
	if (TRAP_Dsetpath("\\mint\\" MINT_VERS_PATH_STRING) == 0)
		strcpy(sysdir, "\\mint\\" MINT_VERS_PATH_STRING "\\");
 	else if (TRAP_Dsetpath("\\mint\\") == 0)
 		strcpy(sysdir, "\\mint\\");
# ifndef BOOTSTRAPABLE
	else
	{
		/* error, no <boot>/mint or <boot>/mint/<version> directory
		 * exist
		 */
		boot_printf(MSG_init_no_mint_folder, MINT_VERS_PATH_STRING);
		boot_print(MSG_init_hitanykey);
		(void) TRAP_Cconin();
		TRAP_Pterm0();
	}
# endif

	/* check for GEM -- this must be done from user mode */
	if (check_for_gem())
	{
		boot_print(MSG_init_must_be_auto);
		boot_print(MSG_init_hitanykey);
		(void) TRAP_Cconin();
		TRAP_Pterm0();
	}

	/* Read user defined defaults before anything else so we can override them later */
	read_ini();

	/* Ask the user if s/he wants to boot MiNT */
	pause_and_ask();

	boot_print("\r\n");

	/* figure out what kind of machine we're running on:
	 * - biosfs wants to know this
	 * - also sets no_mem_prot
	 * - 1992-06-25 kbad put it here so memprot_warning can be intelligent
	 */
	if (getmch ())
	{
		boot_print(MSG_init_hitanykey);
		(void) TRAP_Cconin();
		TRAP_Pterm0();
	}

# ifdef OLDTOSFS
	/* Get GEMDOS version from ROM for later use by our own Sversion() */
	gemdos_version = TRAP_Sversion ();
# endif

	get_my_name();

# ifdef VERBOSE_BOOT
# ifdef WITH_MMU_SUPPORT
	boot_printf(MSG_init_mp, no_mem_prot ? MSG_init_mp_disabled : MSG_init_mp_enabled);
# endif
	/* These are set inside getmch() */
	boot_printf(MSG_init_kbd_desktop_nationality, gl_kbd, gl_lang);

	boot_print(MSG_init_supermode);
# endif

	(void) TRAP_Super (0L);

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif
	stop_and_ask();

# ifdef DEBUG_INFO
	{
		long usp, ssp;

		usp = get_usp();
		ssp = get_ssp();

		DEBUG(("Kernel BASE: 0x%08lx", (unsigned long)_base));
		DEBUG(("Kernel TEXT: 0x%08lx (SIZE: %ld bytes)", _base->p_tbase, _base->p_tlen));
		DEBUG(("Kernel DATA: 0x%08lx (SIZE: %ld bytes)", _base->p_dbase, _base->p_dlen));
		DEBUG(("Kernel BSS:  0x%08lx (SIZE: %ld bytes)", _base->p_bbase, _base->p_blen));
		DEBUG(("Kernel USP:  0x%08lx (FREE: %ld bytes)", usp, usp - (_base->p_bbase + _base->p_blen)));
		DEBUG(("Kernel SSP:  0x%08lx (FREE: %ld bytes)", ssp, ssp - (_base->p_bbase + _base->p_blen)));
	}
# endif
	sysdrv = *((short *) 0x446);	/* get the boot drive number */
	bootdrv[0] = drv_list[sysdrv];

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_sysdrv_is, drv_list[sysdrv]);
# endif

# ifdef WITH_MMU_SUPPORT
# ifdef VERBOSE_BOOT
	boot_print(MSG_init_saving_mmu);
# endif

	if (!no_mem_prot)
		save_mmu ();		/* save current MMU setup */

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif
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
	os = (OSHEADER*)sysbase;

	tosvers = (short)(sysbase[0] & 0x0000ffff);
	if( gl_lang == -1 )	// no _AKP found
		gl_lang = os->os_conf >> 1;
	kbshft = (tosvers == 0x100) ? (char *) 0x0e1bL : (char *)sysbase[9];
	falcontos = (tosvers >= 0x0400 && tosvers <= 0x0404) || (tosvers >= 0x0700);

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_tosver_kbshft, (tosvers >> 8) & 0xff, tosvers & 0xff, \
			falcontos ? " (FalconTOS)" : "", (long)kbshft);
# endif
	/* Check and flag if we're running on EmuTOS */
	emutos = (os->p_rsv2 == 0x45544f53 ? TRUE : FALSE); /* 'ETOS' */

#ifndef __mcoldfire__
	/* Currently, ColdFire machines have trouble with Bconmap() */
	if (falcontos)
	{
		bconmap2 = (BCONMAP2_T *) TRAP_Bconmap (-2);
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
		has_bconmap = (TRAP_Bconmap (0) == 0);

		/* kludge for PAK'ed ST/STE's
		 * they have a patched TOS 3.06 and say they have Bconmap(),
		 * ... but the patched TOS crash hardly if Bconmap() is used.
		 */
		if (has_bconmap)
		{
			if (machine == machine_st || machine == machine_ste || machine == machine_megaste)
				has_bconmap = 0;
		}

		if (has_bconmap)
			bconmap2 = (BCONMAP2_T *) TRAP_Bconmap (-2);
	}
#endif

# ifdef VERBOSE_BOOT
	boot_printf(MSG_init_bconmap, has_bconmap ? MSG_init_present : MSG_init_not_present);
# endif

	stop_and_ask();

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_system);
# endif

	/* initialize cache */
# ifndef M68000
	init_cache ();
	DEBUG (("init_cache() ok!"));
# endif

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
	DEBUG (("init_proc() ok! (base = %p)", _base));

	/* initialize system calls */
	init_bios ();
	DEBUG (("init_bios() ok!"));
	init_xbios ();
	DEBUG (("init_xbios() ok!"));

	/* initialize basic keyboard stuff */
# ifndef NO_AKP_KEYBOARD
	init_keybd();
	DEBUG (("init_keybd() ok!"));
# endif

	/* initalize PCI-BIOS interface */
#ifdef PCI_BIOS
	if (init_pcibios())
		DEBUG (("No PCI-BIOS found"));
#endif

	/* Disable all CPU caches */
# ifndef M68000
	ccw_set(CACHE_OFF, CCW_MASK);

	DEBUG (("ccw_set() ok!"));
# endif

	/* initialize interrupt vectors */
	init_intr ();

	/* after init_intr we are in kernel
	 * trapping isn't allowed anymore; use direct calls
	 * from now on
	 */
	intr_done = 1;
	DEBUG (("init_intr() ok!"));

	/* Enable superscalar dispatch on 68060 */
# ifndef M68000
	get_superscalar();
# endif

	/* Init done, now enable/unfreeze all caches.
	 * Don't touch the write/allocate bits, though.
	 */
# ifndef M68000
	ccw_set(CACHE_ON, CCW_MASK);
# endif

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

# ifdef BOOTSTRAPABLE
	/* Bootstrapped kernel (executed directly by some loader) does
	 * not have any drive access until the init_filesys() is called.
	 * To be able to boot from a kernel built-in filesystem we check
	 * for the sysdir here again. */

	/* the sysdrv to check for the sysdir folder */
	sys_d_setdrv(sysdrv);


	if (sys_d_setpath("\\mint\\" MINT_VERS_PATH_STRING) == 0)
		strcpy(sysdir, "\\mint\\" MINT_VERS_PATH_STRING "\\");
	else if (sys_d_setpath("\\mint\\") == 0)
		strcpy(sysdir, "\\mint\\");
	else
	{
		boot_printf(MSG_init_no_mint_folder, MINT_VERS_PATH_STRING);
		step_by_step = -1; stop_and_ask(); /* wait for a key */
		sys_s_hutdown(SHUT_HALT);
	}

# endif

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

	/* create mchdir */
	{
		char *mch_str = NULL;

		switch (machine)
		{
			case machine_st:
				mch_str = "st";
				break;
			case machine_ste:
				mch_str = "ste";
				break;
			case machine_megaste:
				mch_str = "megaste";
				break;
			case machine_tt:
				mch_str = "tt";
				break;
			case machine_falcon:
				mch_str = "falcon";
				break;
			case machine_milan:
				mch_str = "milan";
				break;
			case machine_hades:
				mch_str = "hades";
				break;
			case machine_ct2:
				mch_str = "ct2";
				break;
			case machine_ct60:
				mch_str = "ct60";
				break;
			case machine_firebee:
				mch_str = "firebee";
				break;
#ifdef WITH_NATIVE_FEATURES
			case machine_emulator:
				/* only when really running on aranym */
				if (strcmp(machine_str(), "ARAnyM") == 0)
					mch_str = "aranym";
				break;
#endif
			case machine_unknown:
			default:
				/* nothing to do */
				break;
		}

		if (mch_str != NULL)
		{
			ksprintf (mchdir, sizeof(mchdir), "%s%s/", sysdir, mch_str);
		}
	}

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
	if( !write_boot_file )
	{
		rootproc->p_fd->ofiles[1] = f;
		f->links++;
	}
	else
	{
		FILEPTR *fb;

		r = FP_ALLOC(rootproc, &fb);
		if (r) FATAL("Can't allocate fp for bootlog!");

		r = ENOENT;	/* file not found */
		if (strlen(mchdir) > 0)
		{
			ksprintf (boot_file, sizeof(boot_file), "%s%s", mchdir, "boot.log");
			r = do_open( &fb, boot_file, O_RDWR|O_TRUNC|O_CREAT, 0, NULL);
		}

		if (r)
		{
			ksprintf (boot_file, sizeof(boot_file), "%s%s", sysdir, "boot.log");
			r = do_open( &fb, boot_file, O_RDWR|O_TRUNC|O_CREAT, 0, NULL);
			if (r == ECHMEDIA)
			    r = do_open( &fb, boot_file, O_RDWR|O_TRUNC|O_CREAT, 0, NULL);
		}

		if (!r)
		{
			rootproc->p_fd->ofiles[1] = fb;
		}
		else
		{
			boot_print("could not open bootlog file\r\n");
			(void)TRAP_Cconin();
			rootproc->p_fd->ofiles[1] = f;
			f->links++;
		}
	}

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

	/* print the warning message if MP is turned off */
# ifdef WITH_MMU_SUPPORT
	if (no_mem_prot && mcpu > 20 && !is_apollo_68080)
		boot_print(memprot_warning);

	stop_and_ask();
# endif

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

	stop_and_ask();

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

	/* set cwd to sysdir for modules */
	sys_d_setpath(sysdir);
	/* load the kernel modules */
	load_all_modules((load_xdd_f | (load_xfs_f << 1)));

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

	rootproc->ctxt[CURRENT].fstate.bytes[0] = 0;
	rootproc->ctxt[CURRENT].pc = (long) mint_thread;
	rootproc->ctxt[CURRENT].usp = rootproc->sysstack;
	rootproc->ctxt[CURRENT].ssp = rootproc->sysstack;
	rootproc->ctxt[CURRENT].term_vec = (long) rts;

	rootproc->ctxt[SYSCALL].fstate.bytes[0] = 0;
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


/* ------------------ The MiNT thread and related code --------------------- */

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
			sys_pexec(0, pathspec, (char *)"", _base->p_env);
		}
		r = sys_f_snext();
	}

 	sys_d_setdrv(curdriv);
 	sys_d_setpath(curpath);
}

/* This seems to be an overkill, but we sometimes need to set something
 * in the environment.
 */

/* Measures the size of the environment */

long
env_size(const char *var)
{
	long count = 1;			/* Trailing NULL */

	while (var && *var)
	{
		while (*var)
		{
			var++;
			count++;
		}
		var++;
		count++;
	}

	return count;
}

/* _mint_delenv() idea is borrowed from mintlib's del_env() */
void
_mint_delenv(BASEPAGE *bp, const char *strng)
{
	char *name, *var;

	/* find the tag in the environment */
	var = _mint_getenv(bp, strng);
	if (!var)
		return;

	TRACE(("%s(): delete existing %s=%s", __FUNCTION__, strng, var));

	/* if it's found, move all the other environment variables down by 1 to
   	 * delete it
         */
	var -= (strlen(strng) + 1);
	name = var + strlen(var) + 1;

	do {
		while (*name)
			*var++ = *name++;

		*var++ = *name++;
	}
	while (*name);

	*var = '\0';
}

void
_mint_setenv(BASEPAGE *bp, const char *var, const char *value)
{
	char *env_str, *ne;
	long old_size, var_size;
	MEMREGION *m;

	TRACE(("%s(): %s=%s", __FUNCTION__, var, value ? value : "(null)"));

	/* If it already exists, delete it */
	_mint_delenv(bp, var);
	if (value == NULL)
		return;
	env_str = bp->p_env;

	old_size = env_size(env_str);
	var_size = strlen(var) + strlen(value) + 16;	/* '=', terminating '\0' and some space */

	m = addr2mem(get_curproc(), (long)env_str);
	if (!m) FATAL("No memory region of environment!");

	/* If there is enough place in the current environment,
	 * don't reallocate it.
	 */
	if (m->len > (old_size+var_size))
	{
		ne = env_str;

		while (*ne)
		{
			while (*ne)
				ne++;
			ne++;
		}
	}
	else
	{
		char *new_env, *es;

		new_env = (char *)sys_m_xalloc(old_size + var_size, F_ALTPREF);
		if (new_env == NULL)
		{
			DEBUG(("%s(): failed to alloc %ld bytes", __FUNCTION__, old_size + var_size));
			return;
		}

		/* just to be sure */
		mint_bzero(new_env, old_size + var_size);

		es = env_str;
		ne = new_env;

		/* Copy old env to new place */
		while (*es)
		{
			while (*es)
				*ne++ = *es++;

			*ne++ = *es++;
		}

		bp->p_env = new_env;
		sys_m_free((long)env_str);
	}

	/* Append new variable at the end */
	strcpy(ne, var);
	strcat(ne, "=");
	strcat(ne, value);

	/* and place the zero terminating the environment */
	ne += strlen(ne);
	*++ne = '\0';
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
/* This is called by new_exec_os() residing in intr.S.
 */
void _cdecl
do_exec_os (long basepage)
{
	register long r;

	/* we have to set a7 to point to lower in our TPA;
	 * otherwise we would bus error right after the Mshrink call!
	 */
	setstack(basepage + QUANTUM - 40L);
	TRAP_Mshrink((void *)basepage, QUANTUM);
	r = TRAP_Pexec(200, init_prg, init_tail, _base->p_env);
	TRAP_Pterm ((int)r);
}

void
mint_thread(void *arg)
{
	int pid;
	long r;

# ifdef VERBOSE_BOOT
	boot_print(MSG_init_done);
# endif

	DEBUG(("%s(): startup", __FUNCTION__));

	/* Delete old TOS environment, we don't inherit it */
	_base->p_env = (char *)sys_m_xalloc(QUANTUM, F_ALTPREF);

	if (_base->p_env == NULL)
		FATAL ("Can't allocate environment!");

	mint_bzero(_base->p_env, QUANTUM);

	/*
	 * Set the PATH variable to our sysdir
	 */
	_mint_setenv(_base, "PATH", sysdir);

	/* Export the sysdir to the environment */
	_mint_setenv(_base, "SYSDIR", sysdir);
	_mint_setenv(_base, "MCHDIR", mchdir[0] ? mchdir : sysdir);
	_mint_setenv(_base, "MINT_VERSION", MINT_VERS_PATH_STRING);
	_mint_setenv(_base, "BOOTDRIVE", bootdrv);

	/* Setup some common variables */
	_mint_setenv(_base, "TERM", "st52");
	_mint_setenv(_base, "SLBPATH", sysdir);

	/* These two are for the MiNTLib */
	_mint_setenv(_base, "UNIXMODE", "/brUs");
	_mint_setenv(_base, "PCONVERT", "PATH,HOME,SHELL");

# ifndef NO_AKP_KEYBOARD
	/* Load the keyboard table */
	load_keyboard_table(NULL, 0x1);
# endif

	/* Load the unicode table */
# ifdef SOFT_UNITABLE
	init_unicode();
# endif

	stop_and_ask();

	/* we default to U:\ before loading the cnf  */
	sys_d_setdrv('u' - 'a');
 	sys_d_setpath("/");

	/* load the MINT.CNF configuration file */
	load_config();

	stop_and_ask();

# ifdef OLDTOSFS
	/* This must go after load_config().
	 *
	 * Until the old TOSFS is completely removed, we try to trigger a media
	 * change on each drive that has NEWFATFS enabled, but is already in
	 * control of TOSFS. This is done with d_lock() and will silently fail
	 * if there are open files on a drive.
	 */
	{
		unsigned short i;
		char *cwd;

                cwd = (char *)sys_m_xalloc(PATH_MAX, F_ALTPREF);
                if (!cwd)
                {
			FATAL ("Can't allocate OLDTOSFS cwd!");
                }

		memset(cwd, 0, PATH_MAX);
		cwd[1] = ':';

		for (i = 0; i < NUM_DRIVES; i++)
		{
			if ((drives[i] == &tos_filesys) &&
			    (fatfs_config(i, FATFS_DRV, ASK) == ENABLE))
			{
				/* We have to preserve the current directory,
				 * as d_lock() will reset it to \
				 */
				cwd[0] = i + ((i < 26) ? 'A' : '1' - 26);
				if (sys_d_getcwd(cwd + 2, i + 1, PATH_MAX - 2) != E_OK)
				{
					continue;
				}
				if (sys_d_lock (1, i) == E_OK)
				{
					sys_d_lock(0, i);
					sys_d_setpath(cwd);
				}
			}
		}

		(void) sys_m_free((long)cwd);
	}
# endif

	/* This must go after load_config().
	 *
	 * if we are MultiTOS, we're running in the AUTO folder, and our INIT
	 * is in fact GEM, take the exec_os() vector. (We know that INIT
	 * is GEM if the user told us so by using GEM= instead of INIT=.)
	 */
	if (init_is_gem && init_prg)
	{
		boot_printf("init is gem and init_prg = %s\r\n", init_prg);
		new_xbra_install(&old_exec_os, EXEC_OS, (long _cdecl (*)())new_exec_os);
	}

	/* run any programs appearing after us in the AUTO folder */
	if (load_auto)
	{
		run_auto_prgs();
		stop_and_ask();
	}

	/* If we are starting the GEM, default to the boot drive,
	 * so it can properly load the accessories.
	 * Otherwise, default to U:\ before starting INIT.
	 */
	sys_d_setdrv(init_is_gem ? sysdrv : 'u' - 'a');
 	sys_d_setpath("/");
	stop_and_ask();

	DEBUG(( "closing bootlog, fd=%p\r\n",rootproc->p_fd->ofiles[0] ));

	if( write_boot_file )
	{
		//boot_printf( "closing bootlog, fd=%lx\r\n",rootproc->p_fd->ofiles[1] );
		r = do_close( rootproc, rootproc->p_fd->ofiles[1] );
		if( r )
			DEBUG(( "error closing bootlog:%ld\r\n", r));
		rootproc->p_fd->ofiles[1] = rootproc->p_fd->ofiles[0];
		rootproc->p_fd->ofiles[0]->links++;
		write_boot_file = 0;
	}
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
		TRACE(("%s(): init_prg is %s", __FUNCTION__, init_prg));

# ifdef VERBOSE_BOOT
		boot_printf(MSG_init_launching_init, init_is_gem ? "GEM" : "init", init_prg);
# endif
		if (!init_is_gem)
		{
# if 1
			r = sys_pexec(100, init_prg, init_tail, _base->p_env);
			DEBUG(("%s(): exec(%s) returned %ld", __FUNCTION__, init_prg, r));
# else
			r = sys_pexec(0, init_prg, init_tail, _base->p_env);
			DEBUG(("%s(): init terminated, returned %ld", __FUNCTION__, init_prg, r));
# endif
		}
		else
		{
			BASEPAGE *bp;

			bp = (BASEPAGE *)sys_pexec(7, (char *)GEM_memflags, (char *)"\0", _base->p_env);
			bp->p_tbase = *((long *) EXEC_OS);

			r = sys_pexec(106, (char *)"GEM", bp, 0L);
			DEBUG(("%s(): exec(%s) returned %ld", __FUNCTION__, init_prg, r));
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
				bp = (BASEPAGE *) sys_pexec(7, (char *) GEM_memflags, (char *) "\0", _base->p_env);
				bp->p_tbase = entry;

				r = sys_pexec(106, (char *) "GEM", bp, 0L);
				DEBUG(("%s(): exec ROM AES returned %ld", __FUNCTION__, r));
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
		path2cookie(get_curproc(), sysdir, NULL, &dir);
		ext = (dir.fs->fsflags & FS_NOXBIT) ? ".tos" : "";
		release_cookie(&dir);

		ksprintf(shellpath, sizeof(shellpath), "%ssh%s", sysdir, ext);

# ifdef VERBOSE_BOOT
		boot_printf(MSG_init_starting_shell, shellpath);
# endif
		r = sys_pexec(100, shellpath, init_tail, _base->p_env);

# ifdef VERBOSE_BOOT
		if (r > 0)
			boot_print(MSG_init_done);
		else
			boot_printf(MSG_init_error, r);
# endif

# ifdef BUILTIN_SHELL
		if (r <= 0)
		{
# ifdef VERBOSE_BOOT
			boot_print(MSG_init_starting_internal_shell);
# endif
			r = startup_shell();	/* r is the shell's pid */
		}
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
		do {
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
		}
		while (pid != ((r & 0xffff0000L) >> 16));
	}
# ifndef DEBUG_INFO
	else
		sys_s_hutdown(SHUT_HALT);		/* Everything failed. Halt. */

	/* If init program exited, reboot the system.
	 * Never go back to TOS.
	 */
	FORCE("init:sys_s_hutdown:COLD");
	(void) sys_s_hutdown(SHUT_COLD);	/* cold reboot is more efficient ;-) */
# else
	/* With debug kernels, always halt
	 */
	FORCE("init:sys_s_hutdown:HALT");
	(void) sys_s_hutdown(SHUT_HALT);
# endif

	/* Never returns */
}
