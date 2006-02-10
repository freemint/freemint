/*
		MY_TOS.H
		TOS Definition Includes

		Copyright (c) Borland International 1990
		All Rights Reserved.
		
		Erweitert von Ph. Donz 27.11.1998
		Letzte nderung: 24.04.2000
*/

#if !defined(__TOS)
#define __TOS
#define __MY_TOS

#include <types.h>

#include <syserr.h>

/**** Strukturen **************************************************/
typedef struct						/* used by Cconrs */
{
		unsigned char maxlen;
		unsigned char actuallen;
		char		buffer[255];
}LINE;

typedef struct						/* used by Fsetdta, Fgetdta */
{
	char				d_reserved[21];
	unsigned char	d_attrib;
	unsigned short	d_time;
	unsigned short	d_date;
	unsigned long	d_length;
	char				d_fname[14];
}DTA;

typedef struct						/* used by Dfree */
{
	unsigned long	b_free;
	unsigned long	b_total;
	unsigned long	b_secsiz;
	unsigned long	b_clsiz;
}DISKINFO;

typedef struct baspag			/* used by Pexec */
{
	void	*p_lowtpa;
	void	*p_hitpa;
	void	*p_tbase;
	long	p_tlen;
	void	*p_dbase;
	long	p_dlen;
	void	*p_bbase;
	long	p_blen;
	DTA	*p_dta;
	struct baspag	*p_parent;
	long	p_resrvd0;
	char	*p_env;
	char	p_stdfh[6];
	char	p_resrvd1;
	char	p_curdrv;
	long	p_resrvd2[18];
	char	p_cmdlin[128];
}BASPAG;

typedef struct						/* used by Getbpb */
{
	unsigned short	recsiz;
	unsigned short	clsiz;
	short	clsizb;
	short	rdlen;
	short	fsiz;
	short	fatrec;
	short	datrec;
	unsigned short	numcl;
	short	bflags;
}BPB;

typedef struct
{
	short	time;
	short	date;
}DOSTIME;

typedef struct						/* used by Iorec */
{
	void	*ibuf;
	short	ibufsiz;
	short	ibufhd;
	short	ibuftl;
	short	ibuflow;
	short	ibufhi;
} IOREC;

typedef struct						/* used by Kbdvbase */
{
	void	(*kb_midivec)();		/*	MIDI Interrupt-Vektor		*/
	void	(*kb_vkbderr)();		/*	Tastatur Fehler-Vektor		*/
	void	(*kb_vmiderr)();		/*	MIDI Fehler-Vektor			*/
	void	(*kb_statvec)();		/*	Tastatur-Status				*/
	void	(*kb_mousevec)();		/*	Tastatur-Maus-Status			*/
	void	(*kb_clockvec)();		/*	Tastatur-Zeitgeber			*/
	void	(*kb_joyvec)();		/*	Tastatur-Joystick-Status	*/
	void	(*kb_midisys)();		/*	System-Midi-Vektor			*/
	void	(*kb_kbdsys)();		/*	Tastatur-Vektor				*/
	char	drvstat;					/*	Tastatur-Treiberstatus	
											NIE ndern solange != 0		*/
}KBDVBASE;

typedef struct						/* used by Pexec */
{
	unsigned char	length;
	char	command_tail[128];
}COMMAND;

typedef struct						/* used by Initmouse */
{
	char	topmode;
	char	buttons;
	char	x_scale;
	char	y_scale;
	short	x_max;
	short	y_max;
	short	x_start;
	short	y_start;
}MOUSE;

typedef struct						/* used by Prtblk */
{
	void	*pb_scrptr;
	short	pb_offset;
	short	pb_width;
	short	pb_height;
	short	pb_left;
	short	pb_right;
	short	pb_screz;
	short	pb_prrez;
	void	*pb_colptr;
	short	pb_prtype;
	short	pb_prport;
	void	*pb_mask;
}PBDEF;

typedef struct						/* used by Keytbl */
{
	char	*unshift;
	char	*shift;
	char	*capslock;
}KEYTAB;

typedef struct __md
{
	struct __md	*m_link;
	void	*m_start;
	long	m_length;
	BASPAG	*m_own;
}MD;

typedef struct						/* used by Getmpb */
{
	MD	*mp_mfl;
	MD	*mp_mal;
	MD	*mp_rover;
}MPB;

typedef struct
{
	short	(*Bconstat) ();
	long	(*Bconin) ();
	short	(*Bcostat) ();
	void	(*Bconout) ();
	long	(*Rsconf) ();
	IOREC	*iorec;
}MAPTAB;

typedef struct
{
	MAPTAB	*maptab;
	short	maptabsize;
}BCONMAP;

typedef struct
{
	unsigned long	drivemap;	/*	Tabelle mit Bits fr Meta-DOS Treiber	*/
	char	*version;				/*	Zeichenkette mit Metados-Name und Version	*/
	long	reserved[2];			/*	reserviert	*/
}METAINFO;

/* system variable _sysbase (0x4F2L) points to next structure         */
typedef struct _syshdr
{
	unsigned	os_entry;   /* $00 BRA to reset handler             */
	unsigned	os_version; /* $02 TOS version number               */
	void	*os_start;		/* $04 -> reset handler                 */
	struct _syshdr	*os_base;   /* $08 -> baseof OS                     */
	void	*os_membot;		/* $0c -> end BIOS/GEMDOS/VDI ram usage */
	void	*os_rsv1;		/* $10 << unused,reserved >>            */
	long	*os_magic;		/* $14 -> GEM memoryusage parm. block   */
	long	os_gendat;		/* $18 Date of system build($MMDDYYYY)  */
	short	os_palmode;		/* $1c OS configuration bits            */
	short	os_gendatg;		/* $1e DOS-format date of systembuild   */
/*
	The next three fields are only available in TOS versions 1.2 and
	greater
*/
	void	*_root;			/* $20 -> base of OS pool               */
	long	*kbshift;		/* $24 -> keyboard shift state variable */
	BASPAG	**_run;		/* $28 -> GEMDOS PID of current process */
	void	*p_rsv2;			/* $2c << unused, reserved >>           */
}SYSHDR;

typedef struct
{
	unsigned short mode;
	long index;
	unsigned short dev;
	unsigned short rdev;
	unsigned short nlink;
	unsigned short uid;
	unsigned short gid;
	long size;
	long blksize;
	long nblocks;
	short mtime;
	short mdate;
	short atime;
	short adate;
	short ctime;
	short cdate;
	short attr;
	short reserved2;
	long reserved3[2];
} XATTR;


/* ApplicationDescriptor	*/
typedef void	APPL;

/* ProcessDescriptor, Aufbau der Basepage	*/
typedef struct pd
{
	void	*p_lowtpa;
	void	p_hitpa;
	void	*p_tbase;
	long	p_tlen;
	void	*p_dbase;
	long	p_dlen;
	void	*p_bbase;
	long	p_blen;
	DTA	*p_dta;
	struct pd	*p_parent;
	short	p_res0;
	short	p_res1;
	char	*p_env;
	char	p_devx[6];
	char	p_res2;
	char	p_defdrv;
	long	p_res3[18];
	char	p_cmdlin[128];
}PD;

/* ProgramHeader, Programmkopf fr ausfhrbare Dateien	*/
typedef struct 
{
	short	ph_branch;		/* 0x00: mu 0x601a sein!! */
	long	ph_tlen;			/* 0x02: Lnge  des TEXT - Segments */
	long	ph_dlen;			/* 0x06: Lnge  des DATA - Segments */
	long	ph_blen;			/* 0x0a: Lnge  des BSS  - Segments */
	long	ph_slen;			/* 0x0e: Lnge  der Symboltabelle   */
	long	ph_res1;			/* 0x12: */
	long	ph_prgflags;	/* 0x16: spezielle Flags	*/
	short	ph_absflag;		/* 0x1a: falls 0 -> Relozierungsdata vorhanden	*/
}PH;

/* Memory Control Block */
typedef struct
{
	long	mcb_magic;                    /* 'ANDR' oder 'KROM' (letzter)    */
	long	mcb_len;                      /* Nettolnge                      */
	long	mcb_owner;                    /* PD *                            */
	long	mcb_prev;                     /* vorh. Block oder NULL           */
	char	mcb_data[0];
}MCB;

/* sigaction: extended POSIX signal handling facility */
struct sigaction 
{
	unsigned long	sa_handler;		/* pointer to signal handler */
	unsigned long	sa_mask;			/* additional signals masked during delivery */
	unsigned short	sa_flags;		/* signal specific flags */
/* signal flags */
};

/* D/Fcntl(FUTIME,...) */
struct mutimbuf
{
	unsigned short	actime;          /* Zugriffszeit */
	unsigned short	acdate;
	unsigned short	modtime;         /* letzte nderung */
	unsigned short	moddate;
};

/* Sconfig(2) -> */
typedef struct
{
	char	*in_dos;						/* Adresse der DOS- Semaphore */
	short	*dos_time;					/* Adresse der DOS- Zeit      */
	short	*dos_date;					/* Adresse des DOS- Datums    */
	long	res1;							/*                            */
	long	res2;							/*                            */
	long	res3;							/* ist 0L                     */
	void	*act_pd;						/* Laufendes Programm         */
	long	res4;							/*                            */
	short	res5;							/*                            */
	void	*res6;						/*                            */
	void	*res7;						/* interne DOS- Speicherliste */
	void	(*resv_intmem)();			/* DOS- Speicher erweitern    */
	long	(*etv_critic)();			/* etv_critic des GEMDOS      */
	char	*((*err_to_str)(char e));	/* Umrechnung Code->Klartext  */
	long	res8;							/*                            */
	long	res9;							/*                            */
	long	res10;						/*                            */
}DOSVARS;

/* os_magic -> */
typedef struct
{
	long	magic;                   /* mu $87654321 sein              */
	void	*membot;                 /* Ende der AES- Variablen         */
	void	*aes_start;              /* Startadresse                    */
	long	magic2;                  /* ist 'MAGX'                      */
	long	date;                    /* Erstelldatum ttmmjjjj           */
	void	(*chgres)(short res, short txt);  /* Auflsung ndern           */
	long	(**shel_vector)(void);   /* residentes Desktop              */
	char	*aes_bootdrv;            /* von hieraus wurde gebootet      */
	short	*vdi_device;             /* vom AES benutzter VDI-Treiber   */
	void	*reservd1;
	void	*reservd2;
	void	*reservd3;
	short	version;                 /* z.B. $0201 ist V2.1             */
	short	release;                 /* 0=alpha..3=release              */
}AESVARS;

/* Cookie MagX --> */
typedef struct
{
	long	config_status;
	DOSVARS	*dosvars;
	AESVARS	*aesvars;
	void	*res1;
	void	*hddrv_functions;
	long	status_bits;             /* MagiC 3 ab 24.5.95         */
}MAGX_COOKIE;





/***** Konstanten **************************************************/

/* attributes for Fcreate/Fsfirst/Fsnext: */
#define FA_READONLY     0x01
#define FA_HIDDEN       0x02
#define FA_SYSTEM       0x04
#define FA_VOLUME       0x08
#define FA_SUBDIR       0x10
#define FA_ARCHIVE      0x20
#define FA_ATTRIB			(FA_SUBDIR|FA_READONLY|FA_HIDDEN|FA_SYSTEM)

/* Fopen modes: */
#define FO_READ         0
#define FO_WRITE        1
#define FO_RW           2

/*	Dpathconf modes:	 (siehe ST Computer 11/95)	*/
#define DP_MAXREQ			-1
#define DP_IOPEN			0
#define DP_MAXLINKS		1
#define DP_PATHMAX		2
#define DP_NAMEMAX		3
#define DP_ATOMIC			4
#define DP_TRUNC			5
#define DP_CASE			6
#define DP_MODEATTR		7
#define DP_XATTRFIELDS	8

/*	Rckgabewerte von Dpathconf fr mode=DP_TRUNC	*/
#define DP_NOTRUNC		0
#define DP_AUTOTRUNC		1
#define DP_TOSTRUNC		2

/*	Rckgabewerte von Dpathconf fr mode=DP_CASE	*/
#define DP_CASESENS		0
#define DP_CASECONV		1
#define DP_CASEINSENS	2

/*	Rckgabewerte von Dpathconf fr mode=DP_MODEATTR	*/
#define DP_FT_DIR		0x00100000L
#define DP_FT_CHR		0x00200000L
#define DP_FT_BLK		0x00400000L
#define DP_FT_REG		0x00800000L
#define DP_FT_LNK		0x01000000L
#define DP_FT_SOCK	0x02000000L
#define DP_FT_FIFO	0x04000000L
#define DP_FT_MEM		0x08000000L

/*	Rckgabewerte von Dpathconf fr mode=DP_XATTRFIELDS	*/
#define DP_INDEX		0x0001
#define DP_DEV			0x0002
#define DP_RDEV		0x0004
#define DP_NLINK		0x0008
#define DP_UID			0x0010
#define DP_GID			0x0020
#define DP_BLKSIZE	0x0040
#define DP_SIZE		0x0080
#define DP_NBLOCKS	0x0100
#define DP_ATIME		0x0200
#define DP_CTIME		0x0400
#define DP_MTIME		0x0800


/*	Mgliche Filetypen von XATTR	*/
#define S_IFMT		0xF000			/*	Maske fr File-Typs	*/
#ifndef S_IFCHR
#define S_IFCHR	0x2000
#define S_IFDIR	0x4000
#define S_IFREG	0x8000
#endif
#define S_IFFIFO	0xA000
#define S_IFMEM	0xC000
#define S_IFLNK	0xE000

/*	spezielle Bits von XATTR	*/
#define S_ISUID	0x800
#define S_ISGID	0x400
#define S_ISVTX	0x200

/*	Zugriffsrechte von XATTR	*/
#define S_IRUSR	0x100
#define S_IWUSR	0x80
#define S_IXUSR	0x40
#define S_IRGRP	0x20
#define S_IWGRP	0x10
#define S_IXGRP	0x80
#define S_IROTH	0x4
#define S_IWOTH	0x2
#define S_IXOTH	0x1

/*	Falcon Grafik-XBios	*/
#define VERTFLAG		0x100
#define STMODES		0x80
#define OVERSCAN		0x40
#define PAL				0x20
#define VGA				0x10
#define TV				0x0
#define COL80			0x08
#define COL40			0x0
#define BPS16			4
#define BPS8			3
#define BPS4			2
#define BPS2			1
#define BPS1			0
#define NUMCOLS		7

/*	Falcon Sound-Xbios	*/
#define	LTATTEN	0
#define	RTATTEN	1
#define	LTGAIN	2
#define	RTGAIN	3
#define	ADDRIN	4
#define	ADCINPUT	5
#define	SETPRESCALE	6

#define	DMAPLAY	0
#define	DSPXMIT	1
#define	EXTINP	2
#define	ADC		3
#define	DMAREC	1
#define	DSPREC	2
#define	EXTOUT	4
#define	DAC		8


/*	Fgetchar-Modes	*/
#define	CMODE_RAW		0
#define	CMODE_COOKED	1
#define	CMODE_ECHO		2


/*	Mxalloc()-Modus	*/
#define MX_STRAM	0
#define MX_TTRAM	1
#define	MX_PREFST	2
#define MX_PREFTT	3

#define MX_MPROT	1<<3;

#define MX_PRIVATE	1<<4
#define MX_GLOBAL	2<<4 			/*	globaler Speicher anfordern	*/
#define MX_SUPER	3<<4
#define MX_READABLE	4<<4

/* GEMDOS- Device- Handles */
#define HDL_CON -1                                          /* TOS */
#define HDL_AUX -2                                          /* TOS */
#define HDL_PRN -3                                          /* TOS */
#define HDL_NUL -4                                          /* KAOS 1.2 */


/* GEMDOS- Standard- Handles */
#define STDIN   0                                           /* TOS */
#define STDOUT  1                                           /* TOS */
#define STDAUX  2                                           /* TOS */
#define STDPRN  3                                           /* TOS */
#define STDERR  4                                           /* TOS */
#define STDXTRA 5                                           /* TOS */


/* GEMDOS Pexec Modes */
#define EXE_LDEX    0                                       /* TOS */
#define EXE_LD      3                                       /* TOS */
#define EXE_EX      4                                       /* TOS */
#define EXE_BASE    5                                       /* TOS */
#define EXE_EXFR    6                                       /* TOS 1.4  */
#define EXE_XBASE   7                                       /* TOS 3.01 */



/* GEMDOS (MiNT/MagiC 3) Fopen modes */
#ifndef   O_RDONLY
#define   O_RDONLY       0
#define   O_WRONLY       1
#define   O_RDWR         2
#define   O_APPEND       8      /* ????????*/
#endif

#ifdef   O_CREAT			/*	STDIO.H Definitionen lschen!	*/
#undef	O_CREAT
#undef	O_TRUNC
#undef	O_EXCL
#endif

#define   O_COMPAT       0x00					/*	Sharing-Flags	*/
#define   O_DENYRW       0x10
#define   O_DENYW        0x20
#define   O_DENYR        0x30
#define   O_DENYNONE     0x40
#define   O_CREAT        0x200				/*	Datei wenn ntig erstellen	*/
#define   O_TRUNC        0x400				/*	Dateilnge auf 0 setzen	*/
#define   O_EXCL         0x800				/*	Existenz ?!?!	*/


/* GEMDOS Fseek Modes */
#define SEEK_SET    0                                       /* TOS */
#define SEEK_CUR    1                                       /* TOS */
#define SEEK_END    2                                       /* TOS */


/* Psemaphore */
#define PSEM_CRGET       0                                  /* MagiC 3.0 */
#define PSEM_DESTROY     1
#define PSEM_GET         2
#define PSEM_RELEASE     3


/* Dlock modes */
#define DLOCKMODE_LOCK   1
#define DLOCKMODE_UNLOCK 0
#define DLOCKMODE_GETPID 2


/* Dopendir modes */
#define DOPEN_COMPAT     1
#define DOPEN_NORMAL     0


/* Fxattr modes */
#define FXATTR_RESOLVE	0
#define FXATTR_NRESOLVE	1


/* Pdomain modes */
#define PDOM_TOS         0
#define PDOM_MINT        1


#define	NSIG		31		/* number of signals recognized */
#define	SIGNULL		0		/* not really a signal */
#define	SIGHUP		1		/* hangup signal */
#define	SIGINT		2		/* sent by ^C */
#define	SIGQUIT		3		/* quit signal */
#define	SIGILL		4		/* illegal instruction */
#define	SIGTRAP		5		/* trace trap */
#define	SIGABRT		6		/* abort signal */
#define	SIGPRIV		7		/* privilege violation */
#define	SIGFPE		8		/* divide by zero */
#define	SIGKILL		9		/* cannot be ignored */
#define	SIGBUS		10		/* bus error */
#define	SIGSEGV		11		/* illegal memory reference */
#define	SIGSYS		12		/* bad argument to a system call */
#define	SIGPIPE		13		/* broken pipe */
#define	SIGALRM		14		/* alarm clock */
#define	SIGTERM		15		/* software termination	signal */
#define	SIGURG		16		/* urgent condition on I/O channel */
#define	SIGSTOP		17		/* stop signal not from terminal */
#define	SIGTSTP		18		/* stop signal from terminal */
#define	SIGCONT		19		/* continue stopped process */
#define	SIGCHLD		20		/* child stopped or exited */
#define	SIGTTIN		21		/* read by background process */
#define	SIGTTOU		22		/* write by background process */
#define	SIGIO			23		/* I/O possible on a descriptor */
#define	SIGXCPU		24		/* CPU time exhausted */
#define	SIGXFSZ		25		/* file size limited exceeded */
#define	SIGVTALRM	26		/* virtual timer alarm */
#define	SIGPROF		27		/* profiling timer expired */
#define	SIGWINCH		28		/* window size changed */
#define	SIGUSR1		29		/* user signal 1 */
#define	SIGUSR2		30		/* user signal 2 */

#define	SIG_DFL		0
#define	SIG_IGN		1


#define SA_NOCLDSTOP	1	/* don't send SIGCHLD when child stops */


/* additional Dcntl/Fcntl Modes */
#define FSTAT					0x4600
#define FIONREAD				0x4601
#define FIONWRITE				0x4602
#define FUTIME					0x4603
#define FTRUNCATE				0x4604
#define PBASEADDR				0x5002
#define SHMGETBLK				0x4d00
#define SHMSETBLK				0x4d01

#define CDROMEJECT				(('C'<<8)|0x09)
#define CDROMPREVENTREMOVAL	(('C'<<8)|0x0e)
#define CDROMALLOWREMOVAL		(('C'<<8)|0x0f)
#define KER_DRVSTAT			0x0104	/* Kernel: Drive-Status (ab 9.9.95) */
#define KER_XFSNAME			0x0105	/* Kernel: XFS-Name (ab 15.6.96) */
#define VFAT_CNFDFLN			0x5600	/* VFAT-XFS: ab 2.1.96 */
#define VFAT_CNFLN			0x5601	/* VFAT-XFS: ab 2.1.96 */


/* Bits for <status_bits> in MAGX_COOKIE (read only!) */
#define MGXSTB_TSKMAN_ACTIVE  1    /* MagiC task manager is currently active */




extern BASPAG *_BasPag;
extern long _PgmSize;


long	Gemdos(short num);

/****** GEMDOS ***************************************************/
long	Pterm0(void);
long	Cconin(void);
long	Cconout(short c);
long	Cauxin(void);
long	Cauxout(short c);
long	Cprnout(short c);
long	Crawio(short w);
long	Crawcin(void);
long	Cnecin(void);
long	Cconws(const char *buf);
long	Cconrs(LINE *buf);
long	Cconis(void);
long	Dsetdrv(short drv);
long	Cconos(void);
long	Cprnos(void);
long	Cauxis(void);
long	Cauxos(void);
long	Dgetdrv(void);
long	Fsetdta(DTA *buf);
long	Super(void *stack);
long	Tgetdate(void);
long	Tsetdate(unsigned short date);
long	Tgettime(void);
long	Tsettime(unsigned short time);
long	Fgetdta(void);
long	Sversion(void);
long	Ptermres(long keepcnt, short retcode);
long	Dfree(DISKINFO *buf, short driveno);
long	Dcreate(const char *path);
long	Ddelete(const char *path);
long	Dsetpath(const char *path);
long	Fcreate(const char *filename, short attr);
long	Fopen(const char *filename, short mode);
long	Fclose(short handle);
long	Fread(short handle, long count, void *buf);
long	Fwrite(short handle, long count, void *buf);
long	Fdelete(const char *filename);
long	Fseek(long offset, short handle, short seekmode);
long	Fattrib(const char *filename, short wflag, short attrib);
long	Fdup(short handle);
long	Fforce(short stch, short nonstdh);
long	Dgetpath(char *path, short driveno);
long	Malloc(long number);
long	Mfree(void *block);
long	Mshrink(void *block, long newsiz);
long	Pexec(short mode, char *ptr1, void *ptr2, void *ptr3);
long	Pterm(short retcode);
long	Fsfirst(const char *filename, short attr);
long	Fsnext(void);
long	Frename(const char *oldname, const char *newname);
long	Fdatime(DOSTIME *timeptr, short handle, short wflag);
long	Mxalloc(long number, short mode);
long	Maddalt(void *start, long size);
long  Flock(short handle, short mode, long start, long length);

long	Syield(void);
long	Fpipe(short *ptr);
long	Fcntl(short f, long arg, short cmd);
long	Finstat(short f);
long	Foutstat(short f);
long	Fgetchar(short f, short mode);
long	Fputchar(short f, long c, short mode);
long	Pwait(void);
long	Pgetpid(void);
long	Pgetppid(void);
long	Pkill(short pid, short sig);
long	Psignal(short sig, void *handler);
long	Psigblock(long mask);
long	Psigsetmask(long mask);
long	Pusrval(long arg);
long	Pdomain(short newdom);
long	Psigreturn(void);
long	Pwait3(short flag, long *rusage);
long	Fselect(short timeout, long *rfds, long *wfds, long *xfds);
long	Pause(void);
long	Psigpending(void);
long	Dpathconf(char *name, short n);
long	Dopendir(char *name, short flag);
long	Dreaddir(short buflen, long dir, char *buf);
long	Drewinddir(long dir);
long	Dclosedir(long dir);
long	Fxattr(short flag, char *name, void *buf);
long	Flink(char *oldname, char *newname);
long	Fsymlink(char *oldname, char *newname);
long	Freadlink(short siz, char *buf, char *name);
long	Dcntl(short cmd, char *name, long arg);
long	Pumask(unsigned short mode);
long	Dlock(short mode, short drive);
long	Psigpause(long sigmask);
long	Psigaction(short sig, long act, long oact);
long	Pwaitpid(short pid, short flag, long *rusage);
long	Dgetcwd(char *path, short drive, short size);
long	Dxreaddir(short len, long dirhandle, char *buf, XATTR *xattr, long *xret);
long	Ssync(void);
long	Dreadlabel(char *path, char *label, short len);
long	Dwritelabel(char *path, char *label);
long	Ssystem(short mode, long arg1, long arg2);

typedef void *SHARED_LIB;
typedef long (*SLB_EXEC)(void , ...);
long	Slbopen(char *name, char *path, long min_ver, SHARED_LIB *sl, SLB_EXEC *fn);
long	Slbclose(SHARED_LIB *sl);
long	Srealloc(long len);


/****** BIOS *****************************************************/
void    Getmpb(MPB *ptr);
short     Bconstat(short dev);
long    Bconin(short dev);
void    Bconout(short dev, short c);
long    Rwabs(short rwflag, void *buf, short cnt, short recnr, short dev , long lrecno);
long    Setexc(short number, void (*vec)());
long    Tickcal(void);
BPB     *Getbpb(short dev);
long    Bcostat(short dev);
long    Mediach(short dev);
long    Drvmap(void);
long    Kbshift(short mode);


/****** XBIOS ****************************************************/
void    Initmouse(short type, MOUSE *par, void *(*vec)());
void    *Ssbrk(short count);
long    Physbase(void);
long    Logbase(void);
short     Getrez(void);
void    Setscreen(void *laddr, void *paddr, short rez , short mode);
void    Setpalette(void *pallptr);
short     Setcolor(short colornum, short color);
short     Floprd(void *buf, long filler, short devno, short sectno,
               short trackno, short sideno, short count);
short     Flopwr(void *buf, long filler, short devno, short sectno,
               short trackno, short sideno, short count);
short     Flopfmt(void *buf, long filler, short devno, short spt, short trackno,
                short sideno, short interlv, long magic, short virgin);
void    Midiws(short cnt, void *ptr);
void    Mfpint(short erno, void (*vector)());
IOREC   *Iorec(short dev);
KEYTAB  *Keytbl(void *unshift, void *shift, void *capslock);
long    Random(void);
long    Rsconf(short baud, short ctr, short ucr, short rsr, short tsr, short scr);
void    Protobt(void *buf, long serialno, short disktype, short execflag);
short     Flopver(void *buf, long filler, short devno, short sectno,
                short trackno, short sideno, short count);
void    Scrdmp(void);
short     Cursconf(short func, short rate);
void    Settime(unsigned long time);
unsigned long  Gettime(void);
void    Bioskeys(void);
void    Ikbdws(short count, void *ptr);
void    Jdisint(short number);
void    Jenabint(short number);
char    Giaccess(short data, short regno);
void    Offgibit(short bitno);
void    Ongibit(short bitno);
void    Xbtimer(short timer, short control, short data, void (*vector)());
void    Dosound(void *buf);
short     Setprt(short config);
KBDVBASE *Kbdvbase(void);
short     Kbrate(short initial, short repeat);
void    Prtblk(PBDEF *par);
void    Vsync(void);
long    Supexec(long (*func)());
void    Puntaes(void);
short     Floprate(short devno, short newrate);
short     Blitmode(short mode);
short     DMAread(long sector, short count, void *buffer, short devno);
short     DMAwrite(long sector, short count, void *buffer, short devno);
short     NVMaccess(short opcode, short start, short count, void *buffer);
long    Bconmap(short devno);
void    Metainit(METAINFO *buffer);
short     EsetShift(short shftMode);
short     EgetShift(void);
short     EsetBank(short bankNum);
short     EsetColor(short colorNum, short color);
void    EsetPalette(short colorNum, short count, short *palettePtr);
void    EgetPalette(short colorNum, short count, short *palettePtr);
short     EsetGray(short swtch);
short     EsetSmear(short swtch);


/***** Falcon030-Xbios **********************************************/
/*	Grafiksystem	*/
short Vsetmode(short modecode);
short Mon_type(void);
void Vsetsync(short external);
long Vgetsize(short mode);
void Vsetrgb(short index,short count,long *array);
void Vgetrgb(short index,short count,long *array);
void Vsetmask(short andmask,short ormask);

/*	Soundsystem	*/
long Locksnd(void);
long Unlocksnd(void);
long Soundcmd(short mode, long data);
long Setbuffer(short reg,void *begaddr,void *endaddr);
long Setmode(short mode);
long Settrack(short playtracks,short rectracks);
long Setmontrack(short montracks);
long Setinterrupt(short src_inter,short cause);
long Buffoper(short mode);
long Devconnect(short src,short dst,short srcclk,short prescale,short protocol);
long Sndstatus(short reset);
long Buffptr(long *ptr);

#define	Fshrink(a)		Fwrite(a, 0L, (void *) -1L)			/* KAOS 1.2 */
#define	Mgrow(a,b)		Mshrink(a,b)								/* KAOS 1.2 */
#define	Mblavail(a)		Mshrink(a,-1L)								/* KAOS 1.2 */

#endif
/************************************************************************/
