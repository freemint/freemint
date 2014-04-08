/*
 *      install.c: install functions for USB storage under TOS/FreeMiNT
 *      
 * 		Based on the assembler code by David Galvez (2010-2012), which
 *		was itself extracted from the USB disk utility assembler code by
 *		Didier Mequignon (2005-2009).
 *
 *      Copyright 2014 Roger Burrows <rfburrows@ymail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#include "../../global.h"
#include "xhdi.h"					/* for PUN_XXX */

/*
 * the following should be in headers for other modules
 */
#define MAX_LOGSEC_SIZE		16384
#define MAX_FAT12_CLUSTERS  4078    /* architectural constants */
#define MAX_FAT16_CLUSTERS  65518

#define RWFLAG			0x0001		/* for Rwabs() 'mode' */
#define NOTRANSLATE		0x0008

#define bootdev			*(short *)0x446
#define drvbits			*(unsigned long *)0x4C2

void install_vectors(void);								//vectors.S

#define DEFAULT_SECTOR_SIZE	2048						//usb_storage.c
unsigned long usb_stor_read(long device,unsigned long blknr,
		unsigned long blkcnt,void *buffer);				//usb_storage.c
unsigned long usb_stor_write(long device,unsigned long blknr,
		unsigned long blkcnt,const void *buffer);		//usb_storage.c

long install_xhdi_driver(void);							//xhdi.c
extern PUN_INFO pun_usb;								//xhdi.c
extern unsigned long my_drvbits;						//xhdi.c
extern char *product_name[MAX_LOGICAL_DRIVE];			//xhdi.c
/*
 * end of stuff for other headers
 */


/*
 * these should be in e.g. install.h
 */
long install_usb_stor(long dev_num,unsigned long part_type,unsigned long part_offset,
					unsigned long part_size,char *vendor,char *revision,char *product);
long uninstall_usb_stor(long logdrv);
BPB *usb_getbpb(long logdrv);
long usb_mediach(long logdrv);
long usb_rwabs(long logdrv,long start,long count,void *buffer,long mode);


/*
 * extended FAT12/FAT16 bootsector
 */
typedef struct {
  /*   0 */	uchar bra[2];
  /*   2 */	uchar loader[6];
  /*   8 */	uchar serial[3];
  /*   b */	uchar bps[2];		/* bytes per sector */
  /*   d */	uchar spc;		/* sectors per cluster */
  /*   e */	uchar res[2];		/* number of reserved sectors */
  /*  10 */	uchar fat;		/* number of FATs */
  /*  11 */	uchar dir[2];		/* number of DIR root entries */
  /*  13 */	uchar sec[2];		/* total number of sectors */
  /*  15 */	uchar media;		/* media descriptor */
  /*  16 */	uchar spf[2];		/* sectors per FAT */
  /*  18 */	uchar spt[2];		/* sectors per track */
  /*  1a */	uchar sides[2];		/* number of sides */
  /*  1c */	uchar hid[4];		/* number of hidden sectors (earlier: 2 bytes) */
  /*  20 */	uchar sec2[4];		/* total number of sectors (if not in sec) */
  /*  24 */	uchar ldn;		/* logical drive number */
  /*  25 */	uchar dirty;		/* dirty filesystem flags */
  /*  26 */	uchar ext;		/* extended signature */
  /*  27 */	uchar serial2[4];	/* extended serial number */
  /*  2b */	uchar label[11];	/* volume label */
  /*  36 */	uchar fstype[8];	/* file system type */
  /*  3e */	uchar data[0x1c0];
  /* 1fe */	uchar cksum[2];
} FAT16_BS;

/*
 * various text values access as longs
 */
#define AHDI		0x41484449L		/* 'AHDI' */
#define GEM		0x0047454dL		/* '\0GEM' */
#define BGM		0x0042474dL		/* '\0BGM' */
#define RAW		0x00524157L		/* '\0RAW' */
#define MINIX		0x00004481L
#define LNX		0x00004483L

/*
 * DOS & Linux partition ids that we support
 */
static const unsigned long valid_dos[] = { 0x04, 0x06, 0x0e, 0x0b, 0x0c, 0x81, 0x83, 0 };

unsigned short vectors_installed = 0;
char boot_sector[DEFAULT_SECTOR_SIZE];

/*
 *	local function prototypes
 */
static void build_bpb(BPB *bpbptr,void *bs);
static void display_error(long dev_num,char *vendor,char *revision,char *product,char *msg);
static void display_installed(long dev_num,char *vendor,char *revision,char *product,int logdrv);
static void display_usb(long dev_num,char *vendor,char *revision,char *product);
static unsigned long getilong(uchar *byte);
static unsigned short getiword(uchar *byte);
static long valid_drive(long logdrv);

#define DEBUGGING_ROUTINES
#ifdef DEBUGGING_ROUTINES
void hex_nybble(int n);
void hex_byte(uchar n);
void hex_word(ushort n);
void hex_long(ulong n);

void hex_nybble(int n)
{
	char c;

	c = (n > 9) ? 'A'+n-10 : '0'+n;
	c_conout(c);
}

void hex_byte(uchar n)
{
	hex_nybble(n>>4);
	hex_nybble(n&0x0f);
}

void hex_word(ushort n)
{
	hex_byte(n>>8);
	hex_byte(n&0xff);
}

void hex_long(ulong n)
{
	hex_word(n>>16);
	hex_word(n&0xffff);
};
#endif

/*
 *	convert Intel (little-endian) values to big-endian
 */
static unsigned short getiword(uchar *byte)
{
	union {
		char c[2];
		unsigned short w;
	} u;

	u.c[1] = *byte++;
	u.c[0] = *byte;

	return u.w;
}

static unsigned long getilong(uchar *byte)
{
	union {
		char c[4];
		unsigned long l;
	} u;

	u.c[3] = *byte++;
	u.c[2] = *byte++;
	u.c[1] = *byte++;
	u.c[0] = *byte;

	return u.l;
}

/*
 *	build the BPB from the boot sector
 *
 *	if it is neither FAT12 nor FAT16, we set 'recsiz' to zero
 */
static void build_bpb(BPB *bpbptr, void *bs)
{
	FAT16_BS *dosbs = (FAT16_BS*)bs;
	unsigned short bps, spf;
	unsigned long res, sectors, clusters;

	bpbptr->recsiz = 0;						/* default to invalid size */

	/*
	 * check for valid FAT16
	 */
	bps = getiword(dosbs->bps);				/* bytes per sector */
	if (bps == 0)
		return;

	bpbptr->recsiz = bps;
	bpbptr->clsiz = dosbs->spc;				/* sectors per cluster */
	bpbptr->clsizb = dosbs->spc * bps;
	bpbptr->rdlen = (32L*getiword(dosbs->dir)+bps-1) / bps;/* root dir len, rounded up */

	res = getiword(dosbs->res);				/* reserved sectors */
	if (res == 0)							/* shouldn't happen:          */
		res = 1;							/*  we use the TOS assumption */
	spf = getiword(dosbs->spf);				/* sectors per fat */
	bpbptr->fsiz = spf;
	bpbptr->fatrec = res + spf;				/* start of fat #2 */
	bpbptr->datrec = res + dosbs->fat*spf + bpbptr->rdlen;	/* start of data */

	sectors = getiword(dosbs->sec);			/* old sector count */
	if (!sectors) 							/* zero => more than 65535, */
		sectors = getilong(dosbs->sec2);		/*  so use new sector count */
	clusters = (sectors - bpbptr->datrec) / dosbs->spc;	/* number of clusters */
	if (clusters > MAX_FAT16_CLUSTERS) {
		bpbptr->recsiz = 0;
		return;
	}
	bpbptr->numcl = clusters;
	if (clusters > MAX_FAT12_CLUSTERS)
		bpbptr->bflags = 1;					/* FAT16 */
}

static void display_usb(long dev_num,char *vendor,char *revision,char *product)
{
	c_conws("USB ");
	c_conout('0'+dev_num);		/* we assume dev_num <= 9 */
	c_conws(".0 ");
	c_conws(vendor);
	c_conout(' ');
	c_conws(revision);
	c_conout(' ');
	c_conws(product);
}

static void display_error(long dev_num,char *vendor,char *revision,char *product,char *msg)
{
	display_usb(dev_num,vendor,revision,product);
	c_conws(": ");
	c_conws(msg);
	c_conws("\r\n");
}

static void display_installed(long dev_num,char *vendor,char *revision,char *product,int logdrv)
{
	display_usb(dev_num,vendor,revision,product);
	c_conws(": installed as drive ");
	c_conout('A'+logdrv);
	c_conws("\r\n");
}

/*
 *	test for valid DOS or GEMDOS partition
 *	returns 1 iff valid
 */
static int valid_partition(unsigned long type)
{
	const unsigned long *valid;

	for (valid = valid_dos; *valid; valid++)
		if (type == *valid)
			return 1;

	type &= 0x00ffffffL;

	if ((type == GEM) || (type == BGM) || (type == RAW))
		return 1;

	return 0;
}

/*
 *  install the new device
 *
 *	returns -1 for error, otherwise the drive number assigned
 */
long install_usb_stor(long dev_num,unsigned long part_type,unsigned long part_offset,
					unsigned long part_size,char *vendor,char *revision,char *product)
{
	int logdrv;
	long mask;

	/*
	 * check input parameters
	 */
	if (dev_num > PUN_DEV) {
		display_error(dev_num,vendor,revision,product,"invalid device number");
		return -1L;
	}
	if (!valid_partition(part_type)) {
		display_error(dev_num,vendor,revision,product,"invalid partition type");
		return -1L;
	}

	/*
	 * install PUN_INFO if necessary
	 */
	if (pun_usb.cookie != AHDI) {
		pun_usb.cookie = AHDI;
		pun_usb.cookie_ptr = &pun_usb.cookie;
		pun_usb.puns = 0;
		pun_usb.version_num = 0x0300;
		pun_usb.max_sect_siz = MAX_LOGSEC_SIZE;
		memset(pun_usb.pun,0xff,MAX_LOGICAL_DRIVE);	/* mark all puns invalid */
	}

	/*
	 * find first free non-floppy drive
	 */
	for (logdrv = 'C'-'A', mask = (1L<<logdrv); logdrv < MAX_LOGICAL_DRIVE; logdrv++, mask<<=1)
		if (!(drvbits&mask))
			break;
	if (logdrv >= MAX_LOGICAL_DRIVE) {
		display_error(dev_num,vendor,revision,product,"no drives available");
		return -1L;
	}

	/*
	 * read in the first sector, which we'll get BPB info from
	 */
	if (usb_stor_read(dev_num,part_offset,1,boot_sector) != 1) {
		display_error(dev_num,vendor,revision,product,"boot sector not readable");
		return -1L;
	}

	/*
	 * fill in the pun_info structure
	 */
	pun_usb.puns++;

	pun_usb.pun[logdrv] = dev_num | PUN_USB;
	pun_usb.partition_start[logdrv] = part_offset;

	part_type <<= 8;					/* for XHDI, make it a 3-character string */
	if (part_type&0xff000000L)						/* i.e. GEM/BGM/RAW */
		pun_usb.ptype[logdrv] = part_type;				/* e.g. 0x47454d00 */
	else pun_usb.ptype[logdrv] = 0x00440000L | part_type;/* e.g. 0x00440600 */

	pun_usb.psize[logdrv] = part_size;
	pun_usb.flags[logdrv] = CHANGE_FLAG;
	build_bpb(&pun_usb.bpb[logdrv],(FAT16_BS *)boot_sector);

	/*
	 * update drive bits etc
	 */
	drvbits |= 1L << logdrv;
	my_drvbits |= 1L << logdrv;			/* used for XHDI */
	if (logdrv == 'C'-'A') {			/* if drive C, make it the boot drive */
		bootdev = logdrv;				/* (is this correct?)                 */
		d_setdrv(logdrv);
	}

	/*
	 * if necessary, install hdv_xxx vectors & xhdi driver
	 */
	if (!vectors_installed) {
		install_vectors();
		install_xhdi_driver();
		vectors_installed = 1;
	}

	/*
	 * save product name ptr for XHDI
	 *
	 * FIXME: assumes ptr points to a static area - is this valid?
	 */
	product_name[dev_num] = product;

	display_installed(dev_num,vendor,revision,product,logdrv);

	return logdrv;
}

long uninstall_usb_stor(long logdrv)
{
	if ((logdrv < 0) || (logdrv >= MAX_LOGICAL_DRIVE))
		return -1L;

	pun_usb.puns--;
	pun_usb.pun[logdrv] = 0xff;
	pun_usb.partition_start[logdrv] = 0L;	/* probably unnecessary */

	drvbits &= ~(1L<<logdrv);
	my_drvbits &= ~(1L<<logdrv);

	return 0L;
}

/*
 *	the following functions are called from the various intercepts
 */

/*
 *	validity check (paranoia, should already have been done by the caller)
 */
static long valid_drive(long logdrv)
{
	if ((logdrv < 0) || (logdrv >= MAX_LOGICAL_DRIVE))
		return 0;
	if (pun_usb.pun[logdrv]&PUN_VALID)	/* means invalid ... */
		return 0;

	if (!(pun_usb.pun[logdrv]&PUN_USB))
		return 0;

	return 1;
}

/*
 *	return BPB pointer for specified drive
 */
BPB *usb_getbpb(long logdrv)
{
	unsigned long type;
	BPB *bpbptr;

	if (!valid_drive(logdrv))
		return NULL;

	type = pun_usb.ptype[logdrv] >> 8;
	if ((type == LNX) || (type == MINIX) || (type == RAW))
		return NULL;

	bpbptr = &pun_usb.bpb[logdrv];
#ifdef TOSONLY
	/*
	 * note: we don't check for a proper Atari partition type, although we
	 * theoretically should.  this is to allow us to create TOS-readable
	 * memory sticks, by using Linux to create an Atari-format filesystem
	 * within a partition that's identified as DOS FAT16.
	 */
	if (bpbptr->recsiz == 0)	/* not FAT16 when building the BPB */
		return NULL;
	if (bpbptr->clsiz != 2)		/* TOS only likes 2-sector clusters */
		return NULL;
#endif

	return bpbptr;
}

/*
 *	perform Rwabs()
 */
long usb_rwabs(long logdrv,long start,long count,void *buffer,long mode)
{
	unsigned long rc;
	long physdev;				/* physical device */

	if (count == 0L)
		return 0;

	if ((start < 0L) || (count < 0L) || !buffer)
		return EBADR;			/* same as EBADRQ */

	if (mode & NOTRANSLATE) {		/* if physical mode, the rwabs intercept */
		physdev = logdrv & PUN_DEV;	/*  has already allowed for floppies     */
	} else {
		BPB *bpbptr;
		unsigned short phys_per_log;	/* physical sectors per logical sector */

		if (!valid_drive(logdrv))
			return ENXIO;		/* same as EDRIVE */

		bpbptr = &pun_usb.bpb[logdrv];
		if (!bpbptr->recsiz)
			return ENXIO;
		phys_per_log = bpbptr->recsiz / 512;

		start = start * phys_per_log;
		count = count * phys_per_log;
		if ((start+count) > pun_usb.psize[logdrv])
			return EBADR;

		start += pun_usb.partition_start[logdrv];
		physdev = pun_usb.pun[logdrv] & PUN_DEV;
	}

	/*
	 * set up for read or write
	 */

	if (mode&RWFLAG)
		rc = usb_stor_write(physdev,start,count,buffer);
	else 
		rc = usb_stor_read(physdev,start,count,buffer);

	return (rc==count) ? 0 : EIO;
}

/*
 *	perform Mediach()
 */
long usb_mediach(long logdrv)
{
	long rc;

	if (!valid_drive(logdrv))
		return 0;

	rc = (pun_usb.flags[logdrv]&CHANGE_FLAG) ? 2 : 0;
	pun_usb.flags[logdrv] &= ~CHANGE_FLAG;

	return rc;
}
