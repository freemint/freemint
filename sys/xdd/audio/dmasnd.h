/*
 * dmasnd.h -- STe DMA sound registers
 */

# ifndef _dmasnd_h
# define _dmasnd_h

# define DMActl	(*(volatile unsigned short *)0xffff8900)
# define  DMA_OFF	0x00
# define  DMA_PLAY	0x01
# define  DMA_LOOP	0x02

# define DMAbah	(*(volatile unsigned char *)0xffff8903)
# define DMAbam	(*(volatile unsigned char *)0xffff8905)
# define DMAbal	(*(volatile unsigned char *)0xffff8907)

# define DMAach	(*(volatile unsigned char *)0xffff8909)
# define DMAacm	(*(volatile unsigned char *)0xffff890b)
# define DMAacl	(*(volatile unsigned char *)0xffff890d)

# define DMAeah	(*(volatile unsigned char *)0xffff890f)
# define DMAeam	(*(volatile unsigned char *)0xffff8911)
# define DMAeal	(*(volatile unsigned char *)0xffff8913)

# define DMAsmc	(*(volatile unsigned short *)0xffff8920)
# define  DMA_6KHZ	0x00
# define  DMA_12KHZ	0x01
# define  DMA_25KHZ	0x02
# define  DMA_50KHZ	0x03
# define  DMA_MONO	0x80

# define DMASetBase(bufstart) \
	{ \
		DMAbah=(char)((long)(bufstart) >> 16); \
		DMAbam=(char)((long)(bufstart) >> 8); \
		DMAbal=(char)((long)(bufstart) >> 0); \
	}

# define DMASetEnd(bufend) \
	{ \
		DMAeah=(char)((long)(bufend) >> 16); \
		DMAeam=(char)((long)(bufend) >> 8); \
		DMAeal=(char)((long)(bufend) >> 0); \
	}

# define MWdata	(*(volatile unsigned short *)0xffff8922)
# define MWmask	(*(volatile unsigned short *)0xffff8924)

# define MW_DEV		0x0400	/* LMC device # */
# define MW_VAL		0x07ff	/* mask for 9 bit commands */

# define MW_VOL		0x00c0	/* set volume */
# define  MW_VOL_VALS	40

# define MW_LVOL	0x0140	/* set left volume */
# define MW_RVOL	0x0100	/* set right volume */
# define  MW_BAL_VALS	20

# define MW_TREB	0x0080	/* set treble amplification */
# define MW_BASS	0x0040	/* set bass amplification */
# define  MW_AMP_VALS	12

# define MW_MIX		0x0000	/* set PSG / DMA mix */
# define  MW_MIX_12DB	0x0000	/*   -12db / 0db */
# define  MW_MIX_1_1	0x0001	/*     0db / 0db */
# define  MW_MIX_NOPSG	0x0002	/*       - / 0db */

# define MW_SCALE(v,n)	((v)*(n)/100)
# define MW_CHECK(v)	((v) >= 0 && (v) <= 100)

# define MWwrite(v) \
	{ \
		while (MWmask != MW_VAL); \
		MWdata = (v) | MW_DEV; \
	}

# endif /* _dmasnd_h */
