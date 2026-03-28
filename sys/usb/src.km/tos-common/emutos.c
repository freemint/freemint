/* etos.c - EmuTOS detection functions for USB TOS drivers
 *
 * Copyright (C) 2026 The FreeMiNT development team

 * This file is distributed under the GPL, version 2.
 * See /COPYING.GPL for details.
 */

#include  "../global.h"

#define ETOS_MAGIC	0x45544F53
#define OSXH_MAGIC	0x4F535848

/* Protoypes */
long InqEmuTOS (void);

typedef struct _osxh
{
	unsigned long	magic;		/* magic number "OSXH" */
	unsigned long	lengh;		/* extension length */
	unsigned char	major;		/* version major */
	unsigned char	minor;		/* version minor */
	unsigned char	fix;		/* version fix */
	unsigned char	official;	/* verison offcial - 0 is offical */
	unsigned long	version_string;	/* offset to version string (from ROM start) */
	unsigned long	features;	/* feature flags */
	unsigned short	machine;	/* machine flags */
	unsigned short	processor;	/* processor flags */
	unsigned char	bootdelay;	/* cold-boot delay */
} OSXH;

typedef struct _osheader
{
	unsigned short	os_entry;	/* BRAnch instruction to Reset-handler  */
	unsigned short	os_version;	/* TOS version number */
	void		*reseth;	/* Pointer to Reset-handler */
	void		*os_beg;	/* Base address of the operating system */
	void		*os_end;	/* First byte not used by the OS  */
	long		os_rsvl;	/* Reserved */
	void		*os_magic;	/* GEM memory-usage parameter block */
	long		os_date;	/* TOS date (English !) in BCD format */
	unsigned short	os_conf;	/* Various configuration bits */
	unsigned short	os_dosdate;	/* TOS date in GEMDOS format */

	/* The following components are available only as of TOS Version 1.02 (Blitter-TOS) */
	char		**p_root;	/* Base address of the GEMDOS pool */
	char		**pkbshift;	/* Pointer to BIOS Kbshift variable
					(for TOS 1.00 see Kbshift) */
	void		**p_run;	/* Address of the variables containing
					   a pointer to the current GEMDOS
					   process. */
	char		*p_rsv2;	/* Reserved */
					/* If EmuTOS is present, then 'ETOS'*/
	long		dummy;		/* branch instruction at _main */
	OSXH		osxh;		/* OSHEADER EmuTOS extension */
} OSHEADER;

/* Are we running on EmuTOS? */
short EmuTOS = -1;

static OSHEADER* get_osheader(void)
{
	long *osheader_sv = (long *)0x4f2;
	OSHEADER *osheaderp = (OSHEADER *)*osheader_sv;

	return osheaderp;
}

static long get_version(OSHEADER *O)
{
	long version = 0L;

	version |= ((long) O->osxh.major << 24);
	version |= ((long) O->osxh.minor << 16);
	version |= ((long) O->osxh.fix << 8);
	version |= (long) O->osxh.official;

	return version;
}

/* The following code test whether EmuTOS is installed, and returns
 * one of the following values:
 *
 * -1 = No EmuTOS present
 *  0 = EmuTOS present and version < 1.0.0
 * >0 = EmuTOS present with the BCD-coded version number.
 */
long InqEmuTOS (void)
{
	OSHEADER *O;
	long r = -1;

	O = (OSHEADER *) Supexec (get_osheader);

	if (O->p_rsv2 == (char *)ETOS_MAGIC) {
		if (O->osxh.magic == OSXH_MAGIC)
			r = get_version(O);
		else r = 0;
	}

	return r;
}
