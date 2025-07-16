/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-03-01
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * Description: Constants for ioctl() calls.
 * 
 */

# ifndef _mint_ioctl_h
# define _mint_ioctl_h


/* 
 * audio device
 * 
 * LMC configuration. Possible values range from 0 (lowest) to 100 (highest)
 * possible value.
 */
# define AIOCSVOLUME	(('A' << 8) | 0)	/* master volume */
# define AIOCSLVOLUME	(('A' << 8) | 1)	/* left channel volume */
# define AIOCSRVOLUME	(('A' << 8) | 2)	/* right channel volume */
# define AIOCSBASS	(('A' << 8) | 3)	/* bass amplification */
# define AIOCSTREBLE	(('A' << 8) | 4)	/* treble amplification */

# define AIOCRESET	(('A' << 8) | 5)	/* reset audio hardware */
# define AIOCSYNC	(('A' << 8) | 6)	/* wait until playing done */
# define AIOCGBLKSIZE	(('A' << 8) | 7)	/* get dma block size */

# define AIOCSSPEED	(('A' << 8) | 8)	/* set play or record speed */
# define AIOCGSPEED	(('A' << 8) | 9)	/* get play or record speed */
# define AIOCGCHAN	(('A' << 8) | 10)	/* get # of channels */
# define AIOCSCHAN	(('A' << 8) | 11)	/* set # of channels */

# define AIOCGFMTS	(('A' << 8) | 12)	/* get supp. sample formats */
# define AIOCSFMT	(('A' << 8) | 13)	/* set sample format */
#  define AFMT_U8	0x00000001L		/* 8 bits, unsigned */
#  define AFMT_S8	0x00000002L		/* 8 bits, signed */
#  define AFMT_ULAW	0x00000004L		/* u law encoding */
#  define AFMT_U16	0x00000008L		/* 16 bits, unsigned */
#  define AFMT_S16	0x00000010L		/* 16 bits, signed */


/*
 * file
 */

# define FSTAT		(('F'<< 8) | 0)		/* handled by kernel */
# define FIONREAD	(('F'<< 8) | 1)
# define FIONWRITE	(('F'<< 8) | 2)
# define FUTIME		(('F'<< 8) | 3)

/* structure for FUTIME */
typedef struct mutimbuf MUTIMBUF;
struct mutimbuf
{
	unsigned short actime, acdate;		/* GEMDOS format */
	unsigned short modtime, moddate;
};
/* some tools like usb/loader may have not included this */
#ifdef _mint_ktypes_h
struct mutimbuf64
{
	time64_t actime;
	time64_t modtime;
};
#endif

# define FTRUNCATE	(('F'<< 8) | 4)
# define FIOEXCEPT	(('F'<< 8) | 5)

# define FSTAT64	(('F'<< 8) | 6)		/* 1.15.4 extension, optional */
# define FUTIME_UTC32	(('F'<< 8) | 7)	/* 1.15.4 extension, optional */
# define FIONBIO	(('F'<< 8) | 8)		/* used by mintlib; reserved */
# define FUTIME_UTC64 (('F'<< 8) | 9)	/* new support for 64bit times; optional */
# define FIBMAP		(('F'<< 8) | 10)


# define FMACOPENRES	(('F'<< 8) | 72)	/* macmint/macfile.h */
# define FMACGETTYCR	(('F'<< 8) | 73)	/* macmint/macfile.h */
# define FMACSETTYCR	(('F'<< 8) | 74)	/* macmint/macfile.h */
# define FMAGICMAC	(('F'<< 8) | 75)	/* (reserved for MagiCMac) */


/*
 * kernel module
 */

# define KM_RUN		(('K'<< 8) | 1)		/* 1.16 */
# define KM_FREE		(('K'<< 8) | 2)		/* 1.17.1 */


/*
 * shared memory
 */

# define SHMGETBLK	(('M'<< 8) | 0)
# define SHMSETBLK	(('M'<< 8) | 1)


/*
 * processes
 */

# define PPROCADDR	(('P'<< 8) | 1)
# define PBASEADDR	(('P'<< 8) | 2)
# define PCTXTSIZE	(('P'<< 8) | 3)
# define PSETFLAGS	(('P'<< 8) | 4)
# define PGETFLAGS	(('P'<< 8) | 5)
# define PTRACESFLAGS	(('P'<< 8) | 6)
# define PTRACEGFLAGS	(('P'<< 8) | 7)

/* flags: */
# define P_ENABLE	(1 << 0)		/* enable tracing */
# ifdef NOTYETDEFINED
# define P_DOS		(1 << 1)		/* trace DOS calls - unimplemented */
# define P_BIOS		(1 << 2)		/* trace BIOS calls - unimplemented */
# define P_XBIOS	(1 << 3)		/* trace XBIOS calls - unimplemented */
# endif

# define PTRACEGO	(('P'<< 8) | 8)		/* these 4 must be together */
# define PTRACEFLOW	(('P'<< 8) | 9)
# define PTRACESTEP	(('P'<< 8) | 10)
# define PTRACE11	(('P'<< 8) | 11)
# define PLOADINFO	(('P'<< 8) | 12)
# define PFSTAT		(('P'<< 8) | 13)
# define PMEMINFO	(('P'<< 8) | 14)

struct ploadinfo
{
	/* passed */
	unsigned short fnamelen;
	
	/* returned */
	char *cmdlin /* 128 byte */, *fname;
};

struct pmeminfo
{
	/* passed */
	unsigned long mem_blocks;
	
	/* returned */
	struct memblk
	{
		unsigned long	loc;
		unsigned long   len;
		unsigned long	flags;
	} **mlist;
};


/*
 * terminals
 */

# define TIOCGETP	(('T'<< 8) | 0)
# define TIOCSETN	(('T'<< 8) | 1)
# define TIOCGETC	(('T'<< 8) | 2)
# define TIOCSETC	(('T'<< 8) | 3)
# define TIOCGLTC	(('T'<< 8) | 4)
# define TIOCSLTC	(('T'<< 8) | 5)
# define TIOCGPGRP	(('T'<< 8) | 6)
# define TIOCSPGRP	(('T'<< 8) | 7)
# define TIOCFLUSH	(('T'<< 8) | 8)
# define TIOCSTOP	(('T'<< 8) | 9)
# define TIOCSTART	(('T'<< 8) | 10)
# define TIOCGWINSZ	(('T'<< 8) | 11)
# define TIOCSWINSZ	(('T'<< 8) | 12)
# define TIOCGXKEY	(('T'<< 8) | 13)
# define TIOCSXKEY	(('T'<< 8) | 14)
# define TIOCIBAUD	(('T'<< 8) | 18)
# define TIOCOBAUD	(('T'<< 8) | 19)
# define TIOCCBRK	(('T'<< 8) | 20)
# define TIOCSBRK	(('T'<< 8) | 21)
# define TIOCGFLAGS	(('T'<< 8) | 22)
# define TIOCSFLAGS	(('T'<< 8) | 23)
# define TIOCOUTQ	(('T'<< 8) | 24)
# define TIOCSETP	(('T'<< 8) | 25)
# define TIOCHPCL	(('T'<< 8) | 26)
# define TIOCCAR	(('T'<< 8) | 27)
# define TIOCNCAR	(('T'<< 8) | 28)
# define TIOCWONLINE	(('T'<< 8) | 29)
# define TIOCSFLAGSB	(('T'<< 8) | 30)
# define TIOCGSTATE	(('T'<< 8) | 31)
# define TIOCSSTATEB	(('T'<< 8) | 32)
# define TIOCGVMIN	(('T'<< 8) | 33)
# define TIOCSVMIN	(('T'<< 8) | 34)
# define TIOCGHUPCL	(('T'<< 8) | 98)	/* mdm0 ioctl */
# define TIOCSHUPCL	(('T'<< 8) | 99)	/* mdm0 ioctl */
# define TIOCGSOFTCAR	(('T'<< 8) | 100)	/* mdm0 ioctl */
# define TIOCSSOFTCAR	(('T'<< 8) | 101)	/* mdm0 ioctl */

# define TIOCBUFFER	(('T'<< 8) | 128)	/* HSMODEM sersoftst.txt */
# define TIOCCTLMAP	(('T'<< 8) | 129)
# define TIOCCTLGET	(('T'<< 8) | 130)
# define TIOCCTLSET	(('T'<< 8) | 131)

/* HSMODEM/SerSoft compatible definitions
 * for TIOCCTLMAP/TIOCCTLGET/TIOCCTLSET
 */
# define TIOCMH_LE	0x0001		/* line enable output, output */
# define TIOCMH_DTR	0x0002		/* data terminal ready, output */
# define TIOCMH_RTS	0x0004		/* ready to send */
# define TIOCMH_CTS	0x0008		/* clear to send */
# define TIOCMH_CAR	0x0010		/* data carrier detect, input */
# define TIOCMH_CD	TIOCMH_CAR	/* alternative name */
# define TIOCMH_RNG	0x0020		/* ring indicator, input */
# define TIOCMH_RI	TIOCMH_RNG	/* alternative name */
# define TIOCMH_DSR	0x0040		/* data set ready, input */
# define TIOCMH_LEI	0x0080		/* line enable input, input */
# define TIOCMH_TXD	0x0100		/* Sendedatenleitung, output */
# define TIOCMH_RXD	0x0200		/* Empfangsdatenleitung, input */
# define TIOCMH_BRK	0x0400		/* break detected, pseudo input */
# define TIOCMH_TER	0x0800		/* send failure, pseudo input */
# define TIOCMH_RER	0x1000		/* receive failure, pseudo input */
# define TIOCMH_TBE	0x2000		/* hardware sendbuffer empty, pseudo input */
# define TIOCMH_RBF	0x4000		/* hardware receivebuffer full, pseudo input */

# define TIOCCTLSFAST	(('T'<< 8) | 132)	/* HSMODEM sersoftst.txt */
# define TIOCCTLSSLOW	(('T'<< 8) | 133)	/* HSMODEM sersoftst.txt */
# define TIONOTSEND	(('T'<< 8) | 134)
# define TIOCERROR	(('T'<< 8) | 135)

# define TIOCSCTTY	(('T'<< 8) | 245)
# define TIOCLBIS	(('T'<< 8) | 246)	/* faked by the MiNT library */
# define TIOCLBIC	(('T'<< 8) | 247)
# define TIOCMGET	(('T'<< 8) | 248)

/* bits in longword fetched by TIOCMGET: */
# define TIOCM_LE	0001 /* not supported */
# define TIOCM_DTR	0002
# define TIOCM_RTS	0004
# define TIOCM_ST	0010 /* not supported */
# define TIOCM_SR	0020 /* not supported */
# define TIOCM_CTS	0040
# define TIOCM_CAR	0100
# define TIOCM_CD	TIOCM_CAR
# define TIOCM_RNG	0200
# define TIOCM_RI	TIOCM_RNG
# define TIOCM_DSR	0400 /* not supported */

# define TIOCCDTR	(('T'<< 8) | 249)
# define TIOCSDTR	(('T'<< 8) | 250)
# define TIOCNOTTY	(('T'<< 8) | 251)
# define TIOCGETD	(('T'<< 8) | 252)  	/* not yet implemented in MiNT */
# define TIOCSETD	(('T'<< 8) | 253)
# define TIOCLGET	(('T'<< 8) | 254)
# define TIOCLSET	(('T'<< 8) | 255)

# define NTTYDISC	1


/*
 * Ext2 filesystem extensions
 */

# define EXT2_IOC_GETFLAGS	(('f'<< 8) | 1)
# define EXT2_IOC_SETFLAGS	(('f'<< 8) | 2)
# define EXT2_IOC_GETVERSION	(('v'<< 8) | 1)
# define EXT2_IOC_SETVERSION	(('v'<< 8) | 2)


/*
 * block device ioctl
 */

# define BLKGETSIZE		(('b'<< 8) | 1)
# define BLOCKSIZE		(('b'<< 8) | 2)


/*
 * cursor control
 */

# define TCURSOFF	(('c'<< 8) | 0)
# define TCURSON	(('c'<< 8) | 1)
# define TCURSBLINK	(('c'<< 8) | 2)
# define TCURSSTEADY	(('c'<< 8) | 3)
# define TCURSSRATE	(('c'<< 8) | 4)
# define TCURSGRATE	(('c'<< 8) | 5)
# define TCURSSDELAY	(('c'<< 8) | 6)		/* undocumented! */
# define TCURSGDELAY	(('c'<< 8) | 7)		/* undocumented! */


# endif /* _mint_ioctl_h */
