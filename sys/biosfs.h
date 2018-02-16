/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _biosfs_h
# define _biosfs_h

# include "mint/mint.h"
# include "mint/emu_tos.h"
# include "mint/file.h"
# include "mint/stat.h"


extern DEVDRV bios_tdevice;
extern DEVDRV bios_ndevice;
extern DEVDRV fakedev;

extern FILESYS bios_filesys;

struct bios_tty
{
	IOREC_T		*irec;		/* From XBIOS ... */
	long		*rsel;		/* pointer to field in tty struct */
	IOREC_T		*orec;		/* Same, for output... */
	long		*wsel;		
	long		ispeed;		
	long		ospeed;		/* last speeds set */
	long		*baudmap;	
	long		maxbaud;	/* Rsconf baud word <-> bps table */
	short		*baudx;		
	struct tty	*tty;		
	long		bticks;		/* when to take a break for real */
	long		vticks;		/* ..check read buf next (vmin/speed) */
	char		clocal;		
	char		brkint;		/* flags: local mode, break == ^C */
	short		tosfd;		/* if != EUNDEV: fd to pass Fcntl()s */
	short		bdev;
	short		unused1;
};

/* internal bios file structure */
# define BNAME_MAX	13

struct bios_file
{
	char 	name[BNAME_MAX+1];	/* device name */
	DEVDRV *device;			/* device driver for device */
	short	private;		/* extra info for device driver */
	ushort	flags;			/* flags for device open */
	struct tty *tty;		/* tty structure (if appropriate) */
	struct bios_file *next;
	short	defmode;		/* default mode */
	short	lockpid;		/* owner of the lock */
	XATTR	xattr;			/* guess what... */
	long	drvsize;		/* size of DEVDRV struct */
};

#define	MAX_BTTY	4	/* 4 bios_tty structs */

extern struct bios_tty bttys[MAX_BTTY];
extern short btty_max;

extern struct bios_tty midi_btty;

extern struct tty con_tty, aux_tty, midi_tty;
extern struct tty sccb_tty, scca_tty, ttmfp_tty;

extern long rsvf;

void biosfs_init (void);

long rsvf_ioctl (int f, void *arg, int mode);
long iwrite	(int bdev, const char *buf, long bytes, int ndelay, struct bios_file *b);
long iread	(int bdev, char *buf, long bytes, int ndelay, struct bios_file *b);
long iocsbrk	(int bdev, int mode, struct bios_tty *t);

int set_auxhandle (struct proc *, int);


# endif /* _biosfs_h */
