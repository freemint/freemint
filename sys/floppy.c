/*
 * floppy.c - floppy routines
 *
 * Copyright (c) 2001 EmuTOS development team
 *
 * Adapted for FreeMiNT by Adam Klobukowski
 *
 * Authors:
 *	LVL	 Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.	See doc/license.txt for details.
 */

/* Note rom Adamk: This code is EXPERIMENTAL (as for FreeMiNT)
   So use with CARE!

   2 problems remain:
   - writing does not work (it does in EmuTOS so it 
     needs code syncing or bugfixing on our side - 
     anyway it will be done
   - DMA transfers does not work yet, I'll start 
     fixing it after fixing writing
*/

#ifdef FLOPPY_ROUTINES

# include "libkern/libkern.h"

# include "mint/ktypes.h"
# include "mint/arch/mfp.h"
# include "mint/arch/wd1772.h"
# include "mint/arch/psg.h"
# include "mint/errno.h"
# include "mint/block_IO.h"
# include "mint/endian.h"
# include "arch/timer.h"
# include "cookie.h"
# include "global.h"
# include "kmemory.h"
# include "console.h"
#ifdef NONBLOCKING_DMA
# include "dma.h"
#endif
# include "floppy.h"

# define flock			((short *) 0x43EL)
# define seekrate		((long *) 0x440L)
# define _fverify		((short *) 0x444L)
# define _nflops			((short *) 0x4a6L)
# define _dskbufp		((void *) 0x4c6L)

typedef struct
{
	uchar	boot_jump[3];	/* Boot strap short or near jump */
	uchar	system_id[8];	/* Name - can be used to special case partition manager volumes */

	uchar	sector_size[2];	/* bytes per logical sector */
	uchar	cluster_size;	/* sectors/cluster */
	ushort	reserved;	/* reserved sectors */
	uchar	fats;		/* number of FATs */
	uchar	dir_entries[2];	/* root directory entries */
	uchar	sectors[2];	/* number of sectors */
	uchar	media;		/* media code (unused) */
	ushort	fat_length;	/* sectors/FAT */
	ushort	secs_track;	/* sectors per track */
	ushort	heads;		/* number of heads */
	ulong	hidden;		/* hidden sectors (unused) */
	ulong	total_sect;	/* number of sectors (if sectors == 0) */

} _F_BS;

/* rwabs() modes */
#define RW_READ						 0
#define RW_WRITE						1

struct _floppy
{
	short		spt;		/* number of sectors per track */
	short		sides;		/* number of sides */
	int		valid;		/* unit valid */
	signed long	last_access;	/* used in mediach only */
	ulong		start;		/* physical start sector */
	ulong		size;		/* physical sectors */
	signed short	cur_track;	/* current track */
	signed short	rate;		/* track seek rate */
	char 		wp;		/* != 0 means write protected */
	uchar		system_id[8];
};
typedef struct _floppy FLOPPY;

#define NUMFLOPPIES	2	/* I assume maximal number of floppies is */
				/* two in one Atari compatibile, correct me */
				/* if I'm wrong. */

FLOPPY floppies[NUMFLOPPIES];

#define MFP_BASE _mfpregs	/* in mfp.h*/

#define set_sr(a)													 \
__extension__															 \
({register short retvalue __asm__("d0");	\
	short _a = (short)(a);			\
	__asm__ volatile			\
	("move.w %%sr,%%d0;				\
	move.w %1,%%sr "				\
	: "=r"(retvalue)	 /* outputs */	\
	: "d"(_a)		/* inputs  */	\
	: "d0"		 /* clobbered regs */	\
	);																				\
	retvalue;																 \
})

/*
 * WORD get_sr(void); 
 *	 returns the current value of sr. 
 */

#define get_sr()														\
__extension__															 \
({register short retvalue __asm__("d0");	\
	__asm__ volatile			\
	("move.w %%sr,%%d0 "			\
	: "=r"(retvalue)	/* outputs */	\
	:			/* inputs  */	\
	: "d0"		 /* clobbered regs */	\
	);																				\
	retvalue;																 \
})

# if 0	/* memcmp() is alredy defined inside libkern */

/* do it faster in asm */
int memcmp(const void * aa, const void * bb, ulong n)
{
	const uchar * a = aa;
	const uchar * b = bb;

	while(n && *a == *b)
	{
		n--;
		a++;
		b++;
	}
	if(n == 0) return 0;
	if(*a < *b) return -1;
	return 1;
}

# endif

/* freemint */

/*==== Introduction =======================================================*/

/*
 * This file contains all floppy-related bios and xbios routines.
 * They are stacked by sections, function of a higher level calling 
 * functions of a lower level.
 *
 * sections in this file:
 * - private prototypes
 * - internal floppy status info
 * - floppy_init
 * - disk initializations: hdv_init, hdv_boot
 * - boot-sector: protobt 
 * - boot-sector utilities: compute_cksum, intel format signed shorts
 * - xbios floprd, flopwr, flopver
 * - xbios flopfmt
 * - internal floprw, fropwtrack
 * - internal status, flopvbl
 * - low level dma and fdc registers access
 *
 */

/*
 * TODO and not implemented:
 * - mediach does not check the write-protect status
 * - on error, the geometry info should be reset to a sane state
 * - on error, should jump to critical error vector
 * - no 'virtual' disk B: mapped to disk A: when only one drive
 * - high density media not considered in flopfmt or protobt
 * - delay() should be based on some delay loop callibration
 * - reserved or hidden sectors are not guaranteed to be handled correctly
 * - ... (search for 'TODO' in the rest of this file)
 * - the unique FDC track register is probably not handled correctly
 *	 when using two drives
 * - once mediach reported != 0, it should not report zero until a new
 *	 getbpb is called.
 */


/*==== Internal defines ===================================================*/
 
#define SECT_SIZ 512
#define TRACK_SIZ 6250

static void setisigned (uchar *addr, ushort value);

/* floppy read/write */
static signed short floprw(signed long buf, signed short rw, signed short dev, signed short sect, signed short track, signed short side, signed short count); 

signed long floppy_rw(signed short rw, signed long buf, signed short cnt, signed long recnr, signed short spt, signed short sides, signed short dev);

/* floppy write track */
static signed short flopwtrack(signed long buf, signed short dev, signed short track, signed short side);

/* initialise a floppy for hdv_init */
static void flopini(signed short dev);

/* called at start and end of a floppy access. */
static void floplock(signed short dev);
static void flopunlk(signed short dev);

/* select in the PSG port A*/
static void select(signed short dev, signed short side);

/* sets the track, returns 0 or error. rate is the step rate */
static signed short set_track(signed short track, signed short rate);

/* returns 1 if the timeout elapsed before the gpip changed */
#ifdef NONBLOCKING_DMA
void timeout_gpip();	/* delay in milliseconds */
#endif
static signed short old_timeout_gpip(signed long delay);	/* delay in milliseconds */
#define TIMEOUT 1500L	 /* default one second and a half */

/* access to dma and fdc registers */
static signed short get_dma_status(void);
static signed short get_fdc_reg(signed short reg);
static void set_fdc_reg(signed short reg, signed short value);
static void set_dma_addr(ulong addr);
static void fdc_start_dma_read(signed short count);
static void fdc_start_dma_write(signed short count);

/* delay for fdc register access */
static void delay(void);

/*==== Internal floppy status =============================================*/

/* cur_dev is the current drive, or -1 if none is current.
 * the fdc track register will reflect the position of the head of the
 * current drive. 'current' does not mean 'active', because the flopvbl
 * routine may deselect the drive in the PSG port A. The routine 
 * floplock(dev) will set the new current drive.
 *
 * cur_track contains a copy of the fdc track register for the current
 * drive, or -1 to indicate that the drive does not exist.
 *
 * last_access is set to the value of the 200 Hz counter at the end of
 * last fdc command. last_access can be used by mediach, a short time 
 * elapsed indicating that the floppy was not ejected.
 * 
 * finfo[].wp is set according to the corresponding bit in the fdc 
 * controller status reg. As soon as this value is different for
 * the drive, this means that the floppy has changed.
 *
 * finfo[].rate is the seek rate (TODO: unused)
 * 
 * the flock variable in tosvars.s is used as following :
 * - floppy.c will set it before accessing to the DMA/FDC, and
 *	 clear it at the end.
 * - flopvbl will do nothing when flock is set.
 *
 * deselected is not null when no drives are selected in PSG port A.
 * it is cleared in select() and set back in flopvbl() when the drives
 * are automatically deselected when the motor on bit is cleared in the
 * FDC status reg.
 */

static signed short cur_dev;
static signed short cur_track;

static unsigned char deselected;

/* Pointer to FastRAM buffer if exists, otherwise 0 */
int cookie_frb;

#ifdef NONBLOCKING_DMA
ulong	dma_channel;
#endif


/*==== hdv_init and hdv_boot ==============================================*/

void init_floppy(void)
{
	long int ret;
	
	/* check for FastRAM buffer */
	if(get_cookie(COOKIE__FRB,&ret)==E_OK) cookie_frb=ret;

	/* set floppy specific stuff */
	*_fverify = 0xFF;
	*seekrate = 3;

	*_nflops = 0;
	cur_dev = -1;

	/* I'm unsure, so let flopvbl() do the work of figuring out. */
	deselected = 1;

	floppies[0].cur_track = -1;
	floppies[0].rate = *seekrate;
	floppies[1].cur_track = -1;
	floppies[1].rate = *seekrate;
	flopini(0);
	flopini(1);

#ifdef NONBLOCKING_DMA
	dma_channel = dma.get_channel();
#endif
}

static void flopini(signed short dev)
{
	signed short status;
	
	floppies[dev].valid = 0;		/* disabled by default */

//	*_dskbufp = kcore(1024UL);	/* reserve memory for buffer */

	floplock(dev);
	cur_track = -1;
	select(dev, 0);
	set_fdc_reg(FDC_CS, FDC_RESTORE);
	if(old_timeout_gpip(TIMEOUT)) {
		/* timeout */
		flopunlk(dev);
		return;
		}
	status = get_fdc_reg(FDC_CS);
	if(status & FDC_TRACK0) {
		/* got track0 signal, this means that a drive is connected */
		cur_track = 0;
		(*_nflops)++;
// freemint		drvbits |= (1<<dev);

		/* init blkdev and device with default parameters */
		floppies[dev].valid = 1;
		floppies[dev].start = 0;
		floppies[dev].size = 79;					 /* unknown size */
		floppies[dev].sides = 2; /* default geometry of 3.5" HD */
		floppies[dev].spt = 18;
		floppies[dev].valid = 1;
		floppies[dev].last_access = 0;	 /* never accessed */

	} else {
	}
	flopunlk(dev);
}

/* rwabs */
signed long _cdecl floppy_rwabs(int rwflag, void *buffer, int number, int recno, int dev, long lrecno) /* for floppy we may ignore lrecno */
{
	return (signed long)floppy_rw(rwflag, (signed long)buffer, number, recno, floppies[dev].spt, floppies[dev].sides, dev);
}

signed long floppy_block_io(DI *di, int rwflag, void *buffer, ulong size, long lrecno)
{
	signed short count;

	count = size/SECT_SIZ;

	return (signed long)floppy_rw(rwflag, (signed long)buffer, count, lrecno, floppies[di->drv].spt, floppies[di->drv].sides, di->drv);
}


#ifdef NONBLOCKING_DMA
/* THIS NEEDS FIXING */
static void floppy_interrupt(void)
{
	if((mfp->gpip & 0x20) == 0) dma.deblock(dma_channel,0);
	dma.deblock(dma_channel,(void *)1);
}

static void floppy_interrupt_asm(void)
{
	__asm__ volatile
	(
		"movem.l %%a0-%%a2/%%d0-%%d2,-(%%sp)"
		"bsr %0"
		"movem.l (%%sp)+,%%a0-%%a2/%%d0-%%d2"
		"rte"
		: 			/* output register */
		: "m"(floppy_interrupt)			/* input registers */
		 			/* clobbered */
	);
}
#endif

long floppy_mediach(ushort drv)
{
	char dummy[512];
	_F_BS *bootsector=&dummy;
	int err;
	
	short sectors,secs_track,heads,total_secs;

//	bootsector=dmabuf_alloc(512,0);
	
	err = floprw((signed long)(bootsector), RW_READ, drv, 1, 0, 0, 1);

	if (err != 0) return err;

	sectors=(((short int)(bootsector->sectors[1]))<<8)+(short int)(bootsector->sectors[0]);
	secs_track=le2cpu16(bootsector->secs_track);
	heads=le2cpu16(bootsector->heads);

	floppies[drv].size = (unsigned long)(sectors/secs_track/heads);
	floppies[drv].sides = (short)heads;
	floppies[drv].spt = (short)secs_track;

	err = 2;

	if ((bootsector->system_id[0]==floppies[drv].system_id[0])&
			(bootsector->system_id[1]==floppies[drv].system_id[1])&
			(bootsector->system_id[2]==floppies[drv].system_id[2])&
			(bootsector->system_id[3]==floppies[drv].system_id[3])&
			(bootsector->system_id[4]==floppies[drv].system_id[4])&
			(bootsector->system_id[5]==floppies[drv].system_id[5])&
			(bootsector->system_id[6]==floppies[drv].system_id[6])&
			(bootsector->system_id[7]==floppies[drv].system_id[7])) err = 0;	/* the same disk */

	floppies[drv].system_id[0]=bootsector->system_id[0];
	floppies[drv].system_id[1]=bootsector->system_id[1];
	floppies[drv].system_id[2]=bootsector->system_id[2];
	floppies[drv].system_id[3]=bootsector->system_id[3];
	floppies[drv].system_id[4]=bootsector->system_id[4];
	floppies[drv].system_id[5]=bootsector->system_id[5];
	floppies[drv].system_id[6]=bootsector->system_id[6];
	floppies[drv].system_id[7]=bootsector->system_id[7];

	return err;	/* disk was changed */
}

signed long floppy_rw(signed short rw, signed long buf, signed short cnt, signed long recnr, signed short spt, signed short sides, signed short dev)
{
	signed short track;
	signed short side;
	signed short sect;
	signed short err;

	while( --cnt >= 0)
	{
		sect = (recnr % spt) + 1;
		track = recnr / spt;
		if (sides == 1)
		{
			side = 0;
		}
		else
		{
			side = track % 2;
			track /= 2;
		}
	        if (buf > 0x1000000L)
	        {
			if (cookie_frb > 0)
			{
				/* do we really need proper FRB lock? (TODO) */
				if(rw & 1)
				{
					/* writing */ 
					memcpy((void *)cookie_frb, (void *)buf, SECT_SIZ);
					err = floprw(cookie_frb, rw, dev, sect, track, side, 1);
				}
				else
				{
					/* reading */
					err = floprw(cookie_frb, rw, dev, sect, track, side, 1);
					memcpy((void *)buf, (void *)cookie_frb, SECT_SIZ);
				}
				/* proper FRB unlock (TODO) */
			}
			else
			{
				err = -1;   /* problem: can't DMA to FastRAM */
			}
		}
		else err = floprw(buf, rw, dev, sect, track, side, 1);
		
		buf += SECT_SIZ;
		recnr ++;
		if(err) return (signed long) err;
	}

	return 0;
}


/*==== xbios floprd, flopwr ===============================================*/

signed short _cdecl floprd(signed long buf, signed long filler, signed short dev, 
						signed short sect, signed short track, signed short side, signed short count)
{
	return floprw(buf, RW_READ, dev, sect, track, side, count);
}

signed short _cdecl flopwr(signed long buf, signed long filler, signed short dev, 
						signed short sect, signed short track, signed short side, signed short count)
{
	return floprw(buf, RW_WRITE, dev, sect, track, side, count);
}

/*==== xbios flopver ======================================================*/

/* TODO, in the case where both one sector cannot be read and another is
 * read wrong, what is the error return code? 
 */

signed short _cdecl flopver(signed long buf, signed long filler, signed short dev, signed short sect, signed short track, signed short side, signed short count)
{
	signed short i;
	signed short err;
	signed short outerr = 0;
	signed short *bad = (signed short *) buf;
	
	if(count <= 0) return 0;
	if(dev < 0 || dev > 1) return ENODEV;	/* unknown disk */
	for(i = 0 ; i < count ; i++)
	{
		err = floprw((signed long) _dskbufp, RW_READ, dev, sect, track, side, 1);
		if(err)
		{
			*bad++ = sect;
			outerr = err;
			continue;
		}
		if(memcmp((void *)buf, _dskbufp, (signed long) SECT_SIZ))
		{
			*bad++ = sect;
			outerr = -16;
		}
		sect ++;
	}
		
	if(outerr)
	{
		*bad = 0;
	}
	return outerr;
}

/*==== xbios flopfmt ======================================================*/

signed short _cdecl flopfmt(signed long buf, signed long filler, signed short dev, signed short spt, signed short track, signed short side, signed short interleave, ulong magic, signed short virgin)
{
	int i, j;
	char b1, b2;
	char *s;
	char *data;
	signed short *bad;
	signed short err;
	int n;

#define APPEND(b, count) do { n=count; while(n--) *s++ = b; } while(0) 

	if(magic != 0x87654321UL) return 0;
	if(dev < 0 || dev > 1) return ENODEV;	/* unknown disk */
	if(spt < 1 || spt > 10) return EGENERIC;	/* general error */
	s = (char *)buf;

	data = s; /* dummy, to avoid warning. data will be set in the loop */
	
	/*
	 * sector interleave factor ignored, always 1.
	 * create the image in memory. 
	 * track  ::= GAP1 record record ... record GAP5
	 * record ::= GAP2 index GAP3 data GAP4
	 */

	b1 = virgin >> 8;
	b2 = virgin;

	/* GAP1 : 60 chars 0x4E */	
	APPEND(0x4E, 60);
	
	for(i = 0 ; i < spt ; i++)
	{
		/* GAP2 */
		APPEND(0x00, 12);
		APPEND(0xF5, 3);

		/* index */
		*s++ = 0xfe;
		*s++ = track;
		*s++ = side;
		*s++ = i+1;
		*s++ = 2; /* means sector of 512 chars */
		*s++ = 0xf7;

		/* GAP3 */
		APPEND(0x4e, 22);
		APPEND(0x00, 12);
		APPEND(0xF5, 3);
		
		/* data */
		*s++ = 0xfe;
		data = s; /* the content of a sector */
		for(j = 0 ; j < SECT_SIZ ; j += 2) {
			*s++ = b1; *s++ = b2;
		}
		*s++ = 0xf7;

		/* GAP4 */
		APPEND(0x4e, 40);
	}		

	/* GAP5 : all the rest to fill to size 6250 (size of a raw track) */	
	APPEND(0x4E, TRACK_SIZ - 60 - 614 * spt);
	
#undef APPEND

	/* write the buffer to track */
	err = flopwtrack(buf, dev, track, side);
	if(err) return err;

	/* verify sectors and store bad sector numbers in buf */
	bad = (signed short *)buf;
	for(i = 0 ; i < spt ; i++)
	{
		err = flopver((signed long)data, 0L, dev, i+1, track, side, 1);
		if(err)
		{
			*bad++ = i+1;
		}
	}
	*bad = 0;
	if(bad != (signed short *)buf) {
		return EBADSEC;	/* bad sectors on format */
	}
	
	return 0;
}

/*==== xbios floprate ======================================================*/

/* sets the rate of the specified drive. 
 * rate meaning
 * 0	 6ms
 * 1	12ms
 * 2	 2ms
 * 3	 3ms
 */

signed short _cdecl floprate(signed short dev, signed short rate)
{
	signed short old;
	if(dev < 0 || dev > 1) return ENODEV;	/* unknown disk */
	old = floppies[dev].rate;
	if(rate >= 0 && rate <= 3)
	{
		floppies[dev].rate = rate;
	}
	return old;
}

/*==== internal floprw ====================================================*/

static signed short floprw(signed long buf, signed short rw, signed short dev, signed short sect, signed short track, signed short side, signed short count)
{
	signed short retry;
	signed short err;
	signed short status;
	signed short timeout;
#ifdef NONBLOCKING_DMA
	void *int_vec;
	MFP *mfp = MFP_BASE;
	uchar old_ierb, old_imrb;
#endif

	if(dev < 0 || dev > 1) return ENODEV;	/* unknown disk */
	
//	if((rw == RW_WRITE) && (track == 0) && (sect == 1) && (side == 0)) {
//		/* TODO, maybe media changed ? */
//	}
	
#ifdef NONBLOCKING_DMA
	dma.dma_start(dma_channel);
#endif
	floplock(dev);

	select(dev, side);
	err = set_track(track, floppies[dev].rate);
	if(err)
	{
		flopunlk(dev);
		return err;
	}
	for(retry = 0; retry < 2 ; retry ++)
	{
		set_fdc_reg(FDC_SR, sect);
		set_dma_addr((ulong) buf);

#ifdef NONBLOCKING_DMA
		int_vec=(* 0x11C);	/* save interrupt vector */
		(* 0x11C) = floppy_interrupt_asm;	/* install interrupt vector */
	
		old_ierb = mfp->ierb & 0x80;	/* get MFP status */
		old_imrb = mfp->imrb & 0x80;
	
		mfp->ierb |= 0x80;
		mfp->imrb |= 0x80;	/* setup MFP to run interrupt */
#endif

		if(rw == RW_READ) 
		{
			fdc_start_dma_read(count);
			set_fdc_reg(FDC_CS, FDC_READ);
		}
		else
		{
			fdc_start_dma_write(count);
			set_fdc_reg(FDC_CS, FDC_WRITE);
		}
		
#ifdef NONBLOCKING_DMA
		timeout = (signed short)dma.block(dma_channel, TIMEOUT, &timeout_gpip);
	
		mfp->ierb |= old_ierb;	/* restore MFP status */
		mfp->imrb |= old_imrb;
	
		(* 0x11C)=int_vec;	/* restore interrupt vector */
#else
		timeout = old_timeout_gpip(TIMEOUT);
#endif
		
		if(timeout)
		{
			/* timeout */
			err = EBUSY;	/* drive not ready */
			flopunlk(dev);
			return err;
		}
		status = get_dma_status();
		if(! (status & DMA_OK))
		{
			/* DMA error, retry */
			err = EGENERIC;	/* general error */
		}
		else
		{
			status = get_fdc_reg(FDC_CS);
			if((rw == RW_WRITE) && (status & FDC_WRI_PRO))
			{
				err = EROFS;	/* write protect */
				/* no need to retry */
				break;
			}
			else if(status & FDC_RNF)
			{
				err = ESECTOR;	/* sector not found */
			}
			else if(status & FDC_CRCERR)
			{
				err = ECRC;	 /* CRC error */
			}
			else if(status & FDC_LOSTDAT)
			{
				err = EBUSY;	/* drive not ready */
			}
			else
			{
				err = 0;
				break;
			}
		}
	}
	flopunlk(dev);
#ifdef NONBLOCKING_DMA
	dma.dma_end(dma_channel);
#endif
	return err;
}

/*==== internal flopwtrack =================================================*/

static signed short flopwtrack(signed long buf, signed short dev, signed short track, signed short side)
{
	signed short retry;
	signed short err;
	signed short status;
	
	if(dev < 0 || dev > 1) return ENODEV;	/* unknown disk */
	
//	if((track == 0) && (side == 0)) {
//		/* TODO, maybe media changed ? */
//	}
	
	floplock(dev);
	
	select(dev, side);
	err = set_track(track, floppies[dev].rate);
	if(err)
	{
		flopunlk(dev);
		return err;
	}
	for(retry = 0; retry < 2 ; retry ++)
	{
		set_dma_addr((ulong) buf);
		fdc_start_dma_write((TRACK_SIZ + SECT_SIZ-1) / SECT_SIZ);
		set_fdc_reg(FDC_CS, FDC_WRITETR);
	
		if(old_timeout_gpip(TIMEOUT))
		{
			/* timeout */
			err = EBUSY;	/* drive not ready */
			flopunlk(dev);
			return err;
		}
		status = get_dma_status();
		if(! (status & DMA_OK))
		{
			/* DMA error, retry */
			err = EGENERIC;	/* general error */
		}
		else
		{
			status = get_fdc_reg(FDC_CS);
			if(status & FDC_WRI_PRO)
			{
				err = EROFS;	/* write protect */
				/* no need to retry */
				break;
			}
			else if(status & FDC_LOSTDAT)
			{
				err = EBUSY;	/* drive not ready */ 
			}
			else
			{
				err = 0;
				break;
			}
		}
	}	
	flopunlk(dev);
	return err;
}

/*==== internal status, flopvbl ===========================================*/

static void floplock(signed short dev)
{
	while (*flock);	/* waiting for lock */
	*flock = 1;

	if(dev != cur_dev)
	{
		/* 
		 * the FDC has only one track register for two units.
		 * we need to save the current value, and switch 
		 */
		if(cur_dev != -1)
		{
			floppies[cur_dev].cur_track = cur_track;
		}
		cur_dev = dev;
		cur_track = floppies[cur_dev].cur_track;
		/* TODO, what if the new device is not available? */
		set_fdc_reg(FDC_TR, cur_track);
	} 
}

static void flopunlk(signed short dev)
{
	floppies[dev].last_access = *_hz_200;
	*flock = 0;
}

void flopvbl(void)
{
	unsigned short int status;
    	uchar  b;
    	unsigned short int old_sr;

	/* don't do anything if the DMA circuitry is being used */
	if(flock) return;
	/* only do something every 8th VBL */
	//done by vbl handler if(frclock & 7) return;
   
	/* TODO - read the write-protect bit in the status register for
	 * both drives
	 */
 
	/* if no drives are selected, no need to deselect them */
	if(deselected) return;
	/* read FDC status register */
	status = get_fdc_reg(FDC_CS);

	/* if the bit motor on is not set, deselect both drives */
	if((status & FDC_MOTORON) == 0)
	{
		old_sr = set_sr(0x2700);

		PSG->control = PSG_PORT_A;
		b = PSG->control;
		b |= 6;
		PSG->data = b;

		deselected = 1;
		set_sr(old_sr);
	}
}

/*==== low level register access ==========================================*/

static void select(signed short dev, signed short side)
{
	signed short old_sr;
	char a;
	
	old_sr = set_sr(0x2700);
	PSG->control = PSG_PORT_A;
	a = PSG->control;
	a &= 0xf8;
	if(dev == 0)
	{
		a |= 4;
	}
	else
	{
		a |= 2;
	}
	if(side == 0)
	{
		a |= 1;
	}	
	PSG->data = a;
	deselected = 0;
	set_sr(old_sr);
}

static signed short set_track(signed short track, signed short rate)
{
	if(track == cur_track) return 0;
	
	if(track == 0)
	{
		set_fdc_reg(FDC_CS, FDC_RESTORE | (rate & 3));
	}
	else
	{
		set_fdc_reg(FDC_DR, track);
		set_fdc_reg(FDC_CS, FDC_SEEK | (rate & 3));
	}
	if(old_timeout_gpip(TIMEOUT))
	{
		cur_track = -1;
		return ESPIPE;	/* seek error */
	}
	else
	{
		cur_track = track;
		return 0;
	}
}

/* returns 1 if the timeout (milliseconds) elapsed before gpip went low */
#ifdef NONBLOCKING_DMA
void timeout_gpip() /* for dma transfers */
{
	MFP *mfp = MFP_BASE;

	if((mfp->gpip & 0x20) == 0) dma.deblock(dma_channel,0);
	dma.deblock(dma_channel,(void *)1);
}
#endif

static signed short old_timeout_gpip(signed long delay) /* for blocking transfers */
{
	MFP *mfp = MFP_BASE;
	signed long next = *_hz_200 + delay/5;
	while(*_hz_200 < next)
	{
		if((mfp->gpip & 0x20) == 0) return 0;
	}
	return 1;
}

static signed short get_dma_status(void)
{
	signed short ret;
	WD1772->control = 0x90;
	ret = WD1772->control;
	return ret;
}

static signed short get_fdc_reg(signed short reg)
{
	signed short ret;
	WD1772->control = reg;
	delay();
	ret = WD1772->data;
	delay();
	return ret;
}

static void set_fdc_reg(signed short reg, signed short value)
{
	WD1772->control = reg;
	delay();
	WD1772->data = value;
	delay();
}

static void set_dma_addr(ulong addr)
{
	WD1772->addr_high = addr>>16;
	WD1772->addr_med = addr>>8;
	WD1772->addr_low = addr;
}

/* the fdc_start_dma_*() functions toggle the dma write bit, to
 * signal the DMA to clear its internal buffers (16 chars in input, 
 * 32 chars in output). This is done just before issuing the 
 * command to the fdc, after all fdc registers have been set.
 */

static void fdc_start_dma_read(signed short count)
{
	WD1772->control = DMA_SCREG | DMA_FDC;
	WD1772->control = DMA_SCREG | DMA_FDC | DMA_WRBIT;
	WD1772->control = DMA_SCREG | DMA_FDC;
	WD1772->data = count;
}

static void fdc_start_dma_write(signed short count)
{
	WD1772->control = DMA_SCREG | DMA_FDC | DMA_WRBIT;
	WD1772->control = DMA_SCREG | DMA_FDC;
	WD1772->control = DMA_SCREG | DMA_FDC | DMA_WRBIT;
	WD1772->data = count;
}
 
static void delay(void)
{
	signed short delay = 30;
	while(--delay);
}

#endif /* FLOPPY_ROUTINES */
