/*
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 */

# ifndef _mint_m68k_mfp_h
# define _mint_m68k_mfp_h


# ifndef MILAN

# define REG_ABSTAND	2

typedef struct
{
	uchar	dum1;
	volatile uchar	gpip;	/*	0	0x01	3	0x03	*/
	uchar	dum2;
	volatile uchar	aer;	/*	2	0x03	7	0x07	*/
	uchar	dum3;
	volatile uchar	ddr;	/*	4	0x05	11	0x0b	*/
	uchar	dum4;
	volatile uchar	iera;	/*	6	0x07	15	0x0f	*/
	uchar	dum5;
	volatile uchar	ierb;	/*	8	0x09	19	0x13	*/
	uchar	dum6;
	volatile uchar	ipra;	/*	10	0x0b	23	0x17	*/
	uchar	dum7;
	volatile uchar	iprb;	/*	12	0x0d	27	0x1b	*/
	uchar	dum8;
	volatile uchar	isra;	/*	14	0x0f	31	0x1f	*/
	uchar	dum9;
	volatile uchar	isrb;	/*	16	0x11	35	0x23	*/
	uchar	dum10;
	volatile uchar	imra;	/*	18	0x13	39	0x27	*/
	uchar	dum11;
	volatile uchar	imrb;	/*	20	0x15	43	0x2b	*/
	uchar	dum12;
	volatile uchar	vr;	/*	22	0x17	47	0x2f	*/
	uchar	dum13;
	volatile uchar	tacr;	/*	24	0x19	51	0x33	*/
	uchar	dum14;
	volatile uchar	tbcr;	/*	26	0x1b	55	0x37	*/
	uchar	dum15;
	volatile uchar	tcdcr;	/*	28	0x1d	59	0x3b	*/
	uchar	dum16;
	volatile uchar	tadr;	/*	30	0x1f	63	0x3f	*/
	uchar	dum17;
	volatile uchar	tbdr;	/*	32	0x21	67	0x43	*/
	uchar	dum18;
	volatile uchar	tcdr;	/*	34	0x23	71	0x47	*/
	uchar	dum19;
	volatile uchar	tddr;	/*	36	0x25	75	0x4b	*/
	uchar	dum20;
	volatile uchar	scr;	/*	38	0x27	79	0x4f	*/
	uchar	dum21;
	volatile uchar	ucr;	/*	40	0x29	83	0x53	*/
	uchar	dum22;
	volatile uchar	rsr;	/*	42	0x2b	87	0x57	*/
	uchar	dum23;
	volatile uchar	tsr;	/*	44	0x2d	91	0x5b	*/
	uchar	dum24;
	volatile uchar	udr;	/*	46	0x2f	95	0x5f	*/
} MFP;

# define _mfpregs	((MFP *)(0xfffffa00L))
# define _ttmfpregs	((MFP *)(0xfffffa80L))

# else /* MILAN */

/*
 * COM 0 oder 1? = ST-MFP
 * Port		  I/O
 * RTS		= o0 NEU vorher BUSY Printer
 * DCD		= i1
 * CTS		= i2
 * DTR		= o3 NEU vorher Blitter
 * DSR		= i4 NEU vorher ACIA irqs
 * RING		= i6
 * KEYLOCK	= i7 NEU vorher monodet
 */

# define MILANDDR	0x09
# define REG_ABSTAND	4

typedef struct
{
	uchar	dum1[3];
	volatile uchar	gpip;	/*	3	0x03	*/
	uchar	dum2[3];
	volatile uchar	aer;	/*	7	0x07	*/
	uchar	dum3[3];
	volatile uchar	ddr;	/*	11	0x0b	*/
	uchar	dum4[3];
	volatile uchar	iera;	/*	15	0x0f	*/
	uchar	dum5[3];
	volatile uchar	ierb;	/*	19	0x13	*/
	uchar	dum6[3];
	volatile uchar	ipra;	/*	23	0x17	*/
	uchar	dum7[3];
	volatile uchar	iprb;	/*	27	0x1b	*/
	uchar	dum8[3];
	volatile uchar	isra;	/*	31	0x1f	*/
	uchar	dum9[3];
	volatile uchar	isrb;	/*	35	0x23	*/
	uchar	dum10[3];
	volatile uchar	imra;	/*	39	0x27	*/
	uchar	dum11[3];
	volatile uchar	imrb;	/*	43	0x2b	*/
	uchar	dum12[3];
	volatile uchar	vr;	/*	47	0x2f	*/
	uchar	dum13[3];
	volatile uchar	tacr;	/*	51	0x33	*/
	uchar	dum14[3];
	volatile uchar	tbcr;	/*	55	0x37	*/
	uchar	dum15[3];
	volatile uchar	tcdcr;	/*	59	0x3b	*/
	uchar	dum16[3];
	volatile uchar	tadr;	/*	63	0x3f	*/
	uchar	dum17[3];
	volatile uchar	tbdr;	/*	67	0x43	*/
	uchar	dum18[3];
	volatile uchar	tcdr;	/*	71	0x47	*/
	uchar	dum19[3];
	volatile uchar	tddr;	/*	75	0x4b	*/
	uchar	dum20[3];
	volatile uchar	scr;	/*	79	0x4f	*/
	uchar	dum21[3];
	volatile uchar	ucr;	/*	83	0x53	*/
	uchar	dum22[3];
	volatile uchar	rsr;	/*	87	0x57	*/
	uchar	dum23[3];
	volatile uchar	tsr;	/*	91	0x5b	*/
	uchar	dum24[3];
	volatile uchar	udr;	/*	95	0x5f	*/
} MFP;

# define _mfpregs	((MFP *)(0xffffc100L))

# endif /* MILAN */

typedef struct
{
	volatile short	sccctl;	/* a = $ffff8c80 b = $ffff8c84	*/
	volatile short	sccdat;	/* a = $ffff8c82 b = $ffff8c86	*/
} SCC;

# define scca		((SCC *) 0xff8c80L)
# define sccb		((SCC *) 0xff8c84L)

# endif /* _mint_m68k_mfp_h */
