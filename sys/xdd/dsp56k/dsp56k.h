/*
 * linux/include/asm-m68k/dsp56k.h - defines and declarations for
 *                                   DSP56k device driver
 *
 * Copyright (C) 1996,1997 Fredrik Noring, lars brinkhoff & Tomas Berndtsson
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */


/* Used for uploading DSP binary code */
struct dsp56k_upload {
	long len;
	char *bin;
};

/* For the DSP host flags */
struct dsp56k_host_flags {
	ushort dir;	/* Bit field. 1 = write output bit, 0 = do nothing.
			 * 0x0000 means reading only, 0x0011 means
			 * writing the bits stored in `out' on HF0 and HF1.
			 * Note that HF2 and HF3 can only be read.
			 */
	ushort out;	/* Bit field like above. */
	ushort status;	/* Host register's current state is returned */
};

/* ioctl command codes */
#define DSP56K_UPLOAD	        ('A'<<8|1)    /* Upload DSP binary program       */
#define DSP56K_SET_TX_WSIZE	('A'<<8|2)    /* Host transmit word size (1-4)   */
#define DSP56K_SET_RX_WSIZE	('A'<<8|3)    /* Host receive word size (1-4)    */
#define DSP56K_HOST_FLAGS	('A'<<8|4)    /* Host flag registers             */
#define DSP56K_HOST_CMD         ('A'<<8|5)    /* Trig Host Command (0-31)        */
