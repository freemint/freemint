
# include "hdio.h"
# include "pun.h"
# include "xhdi.h"

# include <string.h>

# define MFS_XFS
# ifdef MFS_XFS
# include "minixfs.h"
# include "global.h"
# define XRWABS RWABS
# define DWARN(mes,drive) ALERT("Drive %c: " mes,DriveToLetter(drive=)
# else
# define DWARN(mes,drive) fprintf(stderr,"Drive %c: " mes "\n",DriveToLetter(drive))
# define ALERT(x)	 fprintf(stderr,x "\n")
# define NEED_SUPER
# define RWABS Rwabs
# define Kmalloc malloc
# define Kfree free
# define GETBPB Getbpb
# include <osbind.h>
# include <alloc.h>
# include <stdio.h>
# include "hdio.h"

# define XRWABS(a,b,c,d,e,f) \
trap_13_wwlwwwl((short)(0x04),(short)(a),(long)(b),(short)(c),(short)(d)\
,(short)(e),(long)(f) )

# define trap_13_wwlwwwl(n, a, b, c, d, e, f)				\
({									\
	register long retvalue __asm__("d0");				\
	volatile short _a = (volatile short)(a);			\
	volatile long  _b = (volatile long) (b);			\
	volatile short _c = (volatile short)(c);			\
	volatile short _d = (volatile short)(d);			\
	volatile short _e = (volatile short)(e);			\
	volatile long  _f = (volatile long) (f);			\
	    								\
	__asm__ volatile						\
	("\
		movl	%5,%%sp@-; \
		movw    %4,%%sp@-; \
		movw    %3,%%sp@-; \
		movw    %2,%%sp@-; \
		movl    %1,%%sp@-; \
		movw    %0,%%sp@-	"					\
	:					      /* outputs */	\
	: "r"(_a), "r"(_b), "r"(_c), "r"(_d), "r"(_e), "r"(_f) /* inputs  */ \
	);								\
									\
	__asm__ volatile						\
	("\
		movw    %1,%%sp@-; \
		trap    #13;	\
		addw    #18,%%sp "					\
	: "=r"(retvalue)			/* outputs */		\
	: "g"(n)				/* inputs  */		\
	: "d0", "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */	\
	);								\
	retvalue;							\
})

# endif

/* List of error codes for get_hddinf */

char *hdd_err[] = { 
"OK",
"Bad BPB on floppy drive",				/* 1 */
"Need drive A-P for PUN_INFO",				/* 2 */
"Invalid or no PUN_INFO structure",			/* 3 */
"Invalid drive",					/* 4 */
"Physical mode disabled for ICD software",		/* 5 */
"Physical lrecno error",				/* 6 */
"XHInqDev2 failed (old XHDI version?) and bad BPB",	/* 7 */
"XHInqDev failed",					/* 8 */
"Unrecognised partition id",				/* 9 */
"XHInqTarget failed",					/* 10 */
"Unsupported physical sector size",			/* 11 */
"Invalid partition start (zero BPB?)", 			/* 12 */
"ICD software too old to fix",				/* 13 */
/* These are from set_lrecno */
"Memory allocation failure",				/* 14 */
"Unable to access last block"				/* 15 */

};


/*
 * Hard disk info. This is a general purpose routine to handle minixfs' needs
 * for hard disks. If this function returns non-zero then the partition 
 * cannot be accessed. XHDI and pun_info are used to get partition info.
 * The structure 'hdinf' is filled in as approproiate.
 *
 * If this looks awful then that's because it *is*.
 */

static char rno_xhdi,try_xhdi;
static char try_lrecno,rno_lrecno;
static char try_plrecno,rno_plrecno;

int get_hddinf(int drive, struct hdinfo *hdinf, int flag)
{
	long ret;
#ifdef NEED_SUPER
	long tstack;
	tstack=Super(0l);
	if(!((*(long *)0x4c2) & (1l<<drive))) return 4;
#endif
	ret = _get_hddinf(drive,hdinf,flag);
#ifdef NEED_SUPER
	Super(tstack);
#endif
	return ret;
}

int _get_hddinf(int drive, struct hdinfo *hdinf, int flag)
{
	_BPB *bpb;

	hdinf->major=drive;	/* May get overwritten later */

	hdinf->drive=drive;

	bpb=GETBPB(drive); 
	if( flag ) bpb=0;


	/* Step 1: if bpb OK and sector size 512 bytes or 1K we may get away
	 * with normal Rwabs.
	 */

	if( !bpb || (bpb->recsiz!=512 && bpb->recsiz!=1024) )
	{
		long tsecsiz;
		char mpid[4];

		/* OK can't use normal Rwabs: try XHDI or pun_info */

		/* Bypass this rubbish for floppies */
		if(drive < 2 ) return 1;

		/* Try and get info from pun_inf structure */
		if( no_xhdi() )
		{
			struct pun_info *pinf;
			if(drive >= MAXUNITS) return 2;
			if(!(*(long *)0x516)) return 3;
			pinf=PUN_PTR;
			if(!pinf || (PUN_VALID & pinf->pun[drive]) ) return 4;
			hdinf->scsiz = 1;

			hdinf->start = pinf->partition_start[drive];

			if(!hdinf->start) return 12;

			hdinf->size = 0;
			hdinf->minor = pinf->pun[drive];
			hdinf->major = (hdinf->minor & PUN_DEV) + 2;
			hdinf->rwmode = RW_PHYS;
			/* We want to access at least first few sectors */
			if(hdinf->start > 0xfff0)
			{
				if(no_plrecno(hdinf->major)) return 6;
				else hdinf->rwmode |= RW_LRECNO;
			}
			return 0;
		}

		hdinf->rwmode = RW_XHDI | RW_LRECNO;

		/* Hmmmm Getbpb failed or bad secsize: see what XHDI can do */

		if( XHInqDev2(drive,&hdinf->major,&hdinf->minor,&hdinf->start,
							  0,&hdinf->size,mpid) )
		{
			if(!bpb && !flag ) return 7;
			if( XHInqDev(drive,&hdinf->major,&hdinf->minor,
						    &hdinf->start,0) ) return 8;
			hdinf->size=0;
		}
		else if(!bpb && strcmp(mpid,"RAW") && strcmp(mpid,"MIX")
		   	&& strcmp(mpid,"BGM") && strcmp(mpid,"GEM") ) return 9;

		/* Get physical sector size */
		if( XHInqTarget(hdinf->major,hdinf->minor,&tsecsiz,0,0) )
								      return 10;

		if(tsecsiz==512) hdinf->scsiz=1;
		else 
		{
			if(tsecsiz==1024) hdinf->scsiz=0;
			else return 11;
		}
		return 0;
	}
	if(bpb->recsiz==512) hdinf->scsiz=1;
	else hdinf->scsiz=0;
	hdinf->size=0;
	hdinf->rwmode = RW_NORMAL;
	return 0;
}

/* This function is called after get_hddinf and is used to finalise the
 * accessibility of a partition. The 'size' parameter is the size of the
 * partition in K; this info will typically come from the super block of
 * a filesystem. 
 * Return values:
 * 0	OK
 * 1	Malloc failure.
 * 2	Can't access last block.
 * 3    Physical lrecno error.
 */

int set_lrecno(struct hdinfo *hdinf, long int size)
{
	long tsize;
	tsize=size;
	if(hdinf->scsiz) tsize <<=1;
	if( ( (hdinf->rwmode & RW_MODE) == RW_XHDI) && hdinf->size
						      && (hdinf->size < tsize) )
	{
		DWARN("Filesystem size bigger than partition size!",
								hdinf->drive);
	}
	else hdinf->size = tsize;

	hdinf->rwmode |= RW_CHECK;

	if(hdinf->rwmode & RW_LRECNO) return 0;

	switch(hdinf->rwmode & RW_MODE)
	{
		case RW_NORMAL:
		if(tsize < 0xffff)
		{
			char *tmp_buf;
			tmp_buf=Kmalloc(1024);
			if(!tmp_buf) return 14;
			/* Try to read last block */
			if(!block_rwabs(2,tmp_buf,1,size-1,hdinf)) 
			{
				Kfree(tmp_buf);
				return 0;
			}
			Kfree(tmp_buf);
#ifndef BYPASS_LOGICAL
			return 15;
#endif
		}

		if( tsize < 0xffff || no_lrecno(hdinf->major) )
		{
		/* Bad lrecno or access error, try physical mode access */
			int drive,err;
			drive = hdinf->major;
			if( (err=get_hddinf(drive,hdinf,1)) ) return err;
			else return (set_lrecno(hdinf,size));
		}
		else hdinf->rwmode |= RW_LRECNO;
		return 0;

		case RW_PHYS:
		if(tsize+hdinf->start >= 0xfffe)
		{
			if(no_plrecno(hdinf->major)) return 1;
			hdinf->size = tsize;
			hdinf->rwmode |= RW_LRECNO;
		}
		return 0;
	}

	return 1;	/* This can't happen */
}

/* Test for 'lrecno' parameter on drive 'drive': mode' is RWABS mode to use 
 * (2=logical,10=physical).
 * Return values:
 * 0 OK
 * 1 Read error.
 * 2 No lrecno recognised.
 * 3 Error reading large sector number (possibly OK if partition too small).
 * 4 Wraparound bug present.
 * 5 Allocation error.
 */

int test_lrecno(int drive, int mode)
{
	char *block_buf1,*block_buf2;
	int size;
	_BPB *bpb;
	bpb=Getbpb(drive);
	if( (mode & 8) || !bpb ) size=1024;
	else size=bpb->recsiz;

	block_buf1=Kmalloc(size<<1);
	block_buf2=block_buf1+size;
	bzero(block_buf1,size<<1);

	if(!block_buf1) return 5;

	/* read in boot sector */
	if(RWABS(mode,block_buf1,1,0,drive)) 
	{
		Kfree(block_buf1);
		return 1;
	}

	/* read it in with lrecno */	
	if( XRWABS(mode,block_buf2,1,-1,drive,0l) ) 
	{
		Kfree(block_buf1);
		return 2;
	}

	/* Compare the two */
	if(bcmp(block_buf1,block_buf2,size))
	{
		Kfree(block_buf1);
		return 2;
	}

	/* read in next sector with lrecno */
	if(XRWABS(mode,block_buf2,1,-1,drive,1l))
	{
		Kfree(block_buf1);
		return 1;
	}

	/* compare the two */
	if(!bcmp(block_buf1,block_buf2,size))
	{
		Kfree(block_buf1);
		return 2;
	}

	/* Check for lrecno bug, this causes the upper word of a long sector
	 * number to be ignored. Read in sector 0 and 0x10000, if the bug is
	 * present then these will be identical.
	 */
	bzero(block_buf2,size);

	if(XRWABS(mode,block_buf2,1,-1,drive,0x10000l))
	{
		Kfree(block_buf1);
		return 3;
	}
	else if(!bcmp(block_buf1,block_buf2,size))
	{
		Kfree(block_buf1);
		return 4;
	}

	Kfree(block_buf1);
	return 0;

}

int no_lrecno(int drv)
{
	if( !try_lrecno )
	{
		rno_lrecno = test_lrecno(drv,2) ;
		try_lrecno = 1;
	}
	return rno_lrecno;
}

int no_plrecno(int drv)
{
	if( !try_plrecno ) 
	{
		try_plrecno = 1;
		rno_plrecno = test_lrecno(drv,10) ;
	}
	return rno_plrecno;
}

int no_xhdi(void)
{
	if( !try_xhdi ) 
	{
		if( !XHGetVersion() ) rno_xhdi=1;
		try_xhdi=1;
	}
	return rno_xhdi;
}

/* 
 * This is (finally!) the I/O function hdinf uses. It reads/writes in 1K chunks
 * and calls the relevant functions according to the hdinf structure.
 */

long block_rwabs(int rw, void *buf, unsigned int num, long int recno, struct hdinfo *hdinf)
{
	if( hdinf->scsiz )
	{
		recno <<=1;
		num <<=1;
	}

	if( (hdinf->rwmode & RW_CHECK) && (recno+num > hdinf->size) )
	{
		DWARN("Attempted access outside partition",hdinf->drive);
		return -1;
	}

	switch(hdinf->rwmode & (RW_MODE|RW_LRECNO))
	{
		case RW_NORMAL:
		return RWABS(rw,buf,num,(unsigned)recno,hdinf->major);

		case RW_NORMAL | RW_LRECNO:
		return XRWABS(rw,buf,num,-1,hdinf->major,recno);

		case RW_PHYS:
		return RWABS(rw | 8,buf,num,(unsigned)(recno+hdinf->start), hdinf->major);

		case RW_PHYS | RW_LRECNO:
		return XRWABS(rw | 8,buf,num,-1,hdinf->major, recno+hdinf->start);

		case RW_XHDI | RW_LRECNO:
		return XHReadWrite(hdinf->major,hdinf->minor,rw,
						    recno+hdinf->start,num,buf);
	}
	return 1;	/* This can't happen ! */
}

#ifndef SUPER_CALL

/* Determine partition size from drive info. Check Rwabs errors ... it should
 * give an error for an attempt to read past the end of a partition. From then
 * on use a log search to find partition size.
 * If this seems awkward you are right ... certain disk software doesn't enter
 * the correct size in the boot sector (to correct a TOS bug).
 */

char *size_err[] = 
{
"OK",
"Getbpb failed",
"Allocation error",
"Driver software incompatible"
};

int get_size(int drive, long int *size)
{
	char *buf;
	_BPB *bpb;
	unsigned secnum,bitnum;
	if(! (bpb=GETBPB(drive)) ) return 1; /* Bad bpb */
	if(! (buf=Kmalloc(bpb->recsiz)) ) return 2; /* alloc error */
	
	if(!RWABS(2,buf,1,0xfffe,drive))
	{
		Kfree(buf);
		return 3;	/* Software doesn't return errors */
	}

	bitnum=0x8000;
	secnum=0;

	do
	{
		secnum |= bitnum;
		if( RWABS(2,buf,1,secnum,drive) ) secnum &=~bitnum;
		bitnum >>=1 ;
	} while(bitnum);

	secnum++;

	*size=(((long)secnum) * bpb->recsiz)>>10;
	Kfree(buf);
	return 0;

}

#endif



