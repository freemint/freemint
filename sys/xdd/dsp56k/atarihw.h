/* 
 * atarihw.h (for porting the dsp56k device only ATM)
 *
 * The dsp56k device (c) Thomas Berndtsson <tobe@lysator.liu.se>
 *
 * Author:	Konrad M. Kokoszkiewicz <draco@atari.org>
 *
 * Began:	5.XII.1999.
 * Last change:	5.XII.1999.
 *
 * BUGS:
 *
 * - provides only minimum functionality just for the dsp56k device
 * - shoult be based on the original atarihw.h from Linux,
 *   instead of being guessed from the dsp68k.c source.
 *   But I have no Linux.
 *
 */

/* Falcon030 DSP host interface */

# define DSP56K_ICR_RREQ	0x01
# define DSP56K_ICR_TREQ	0x02
# define DSP56K_ICR_HF0		0x08
# define DSP56K_ICR_HF1		0x10

# define DSP56K_CVR_HV_MASK	0x1f
# define DSP56K_CVR_HC		0x80

# define DSP56K_ISR_RXDF	0x01
# define DSP56K_ISR_TXDE	0x02
# define DSP56K_ISR_TRDY	0x04
# define DSP56K_ISR_HF2		0x08
# define DSP56K_ISR_HF3		0x10

typedef struct
{
	volatile uchar icr;
	volatile uchar cvr;
	volatile uchar isr;
	volatile uchar ivr;
	union {
		volatile uchar	b[4];
		volatile ushort	w[2];
		volatile ulong	l;
	} data;
} DSP56K_HOST_INTERFACE;

typedef struct
{
	volatile uchar rd_data_reg_sel;
	volatile uchar wd_data;
} SOUND_YM;

# define sound_ym		(*(SOUND_YM *)0xffff8800L)
# define dsp56k_host_interface  (*(DSP56K_HOST_INTERFACE *)0xffffa200L)

/* end of atarihw.h */
