/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * XBIOS replacement routines
 *
 */

 
 
struct gsxb_device gd; 
 
 
 /* Xbios function 96 (0x60) */
void _cdecl
sys_gsxb_dsp_doblock(char *idata, long isize, char *odata, long osize)
{
}

/* Xbios function 97 (0x61) */
void _cdecl
sys_gsxb_dsp_blkhandshake(char *idata, long isize, char *odata, long osize)
{
}

/* Xbios function 98 (0x62) */
void _cdecl
sys_gsxb_dsp_blkunpacked(char *idata, long isize, char *odata, long osize)
{
}

/* Xbios function 99 (0x63) */
void _cdecl
sys_gsxb_dsp_instream(char *idata, long block_size, long num_blocks, long *blocks_done)
{
}

/* Xbios function 100 (0x64) */
void _cdecl
sys_gsxb_dsp_outstream(char *odata, long block_size, long num_blocks, long *blocks_done)
{
}

/* Xbios function 101 (0x65) */
void _cdecl
sys_gsxb_dsp_iostream(char *idata, char *odata, long block_isize, long block_osize, long num_blocks, long *blocks_done)
{
}

/* Xbios function 102 (0x66) */
void _cdecl
sys_gsxb_dsp_removeinterrupts(short mask)
{
}

/* Xbios function 103 (0x67) */
short _cdecl
sys_gsxb_dsp_getwordsize(void)
{
}

/* Xbios function 104 (0x68) */
short _cdecl
sys_gsxb_dsp_lock(void)
{
}

/* Xbios function 105 (0x69) */
void _cdecl
sys_gsxb_dsp_unlock(void)
{
}

/* Xbios function 106 (0x6a) */
void _cdecl
sys_gsxb_dsp_available(long *xavail, long *yavail)
{
}

/* Xbios function 107 (0x6b) */
short _cdecl
sys_gsxb_dsp_reserve(long xreserve, long yreserve)
{
}

/* Xbios function 108 (0x6c) */
short _cdecl
sys_gsxb_dsp_loadprog(char *file, short ability, char *buf)
{
}

/* Xbios function 109 (0x6d) */
void _cdecl
sys_gsxb_dsp_execprog(char *codeptr, long codesize, short ability)
{
}

/* Xbios function 110 (0x6e) */
void _cdecl
sys_gsxb_dsp_execboot(char *codeptr, long codesize, short ability)
{
}

/* Xbios function 111 (0x6f) */
long _cdecl
sys_gsxb_dsp_lodtobinary(char *file, char *codeptr)
{
}

/* Xbios function 112 (0x70) */
void _cdecl
sys_gsxb_dsp_triggerhc(short vector)
{
}

/* Xbios function 113 (0x71) */
short _cdecl
sys_gsxb_dsp_requestuniqueability(void)
{
}

/* Xbios function 114 (0x72) */
short _cdecl
sys_gsxb_dsp_getprogability(void)
{
}

/* Xbios function 115 (0x73) */
void _cdecl
sys_gsxb_dsp_flushsubroutines(void)
{
}

/* Xbios function 116 (0x74) */
short _cdecl
sys_gsxb_dsp_loadsubroutines(char *ptr, long size, short ability)
{
}

/* Xbios function 117 (0x75) */
short _cdecl
sys_gsxb_dsp_inqsubrability(short ability)
{
}

/* Xbios function 118 (0x76) */
short _cdecl
sys_gsxb_dsp_runsubroutine(short handle)
{
}

/* Xbios function 119 (0x77) */
short _cdecl
sys_gsxb_dsp_hf0(short flag)
{
}

/* Xbios function 120 (0x78) */
short _cdecl
sys_gsxb_dsp_hf1(short flag)
{
}

/* Xbios function 121 (0x79) */
short _cdecl
sys_gsxb_dsp_hf2(void)
{
}

/* Xbios function 122 (0x7a) */
short _cdecl
sys_gsxb_dsp_hf3(void)
{
}

/* Xbios function 123 (0x7b) */
void _cdecl
sys_gsxb_dsp_blkwords(short *idata, long isize, short *odata, long osize)
{
}

/* Xbios function 124 (0x7c) */
void _cdecl
sys_gsxb_dsp_blkbytes(unsigned char *idata, long isize, unsigned char *odata, long osize)
{
}

/* Xbios function 125 (0x7d) */
char _cdecl
sys_gsxb_dsp_hstat(void)
{
}

/* Xbios function 126 (0x7e) */
void _cdecl
sys_gsxb_dsp_setvectors(void _cdecl (*receiver)(void), long _cdecl (*transmitter)(void))
{
}

/* Xbios function 127 (0x7f) */
void _cdecl
sys_gsxb_dsp_multblocks(long numsend, long numreceive, DSPBLOCK *sendblks, DSPBLOCK *receiveblks)
{
}

/* Xbios function 128 (0x80) */
/* If process already owns lock, return 1
 * If another process owns lock, return ESNDLOCKED
 * If no GSXB device installed, return ENODEV
 * else return error returned by device driver
 */
long _cdecl
sys_gsxb_locksnd(void)
{
	long lock;

	if (gd.dacio)
	{
		lock = gd.dacio(gd.dev_id, bd.dac_id, PCM_IOCTL_SEMAPHORE, GSXB_SEMA_GET);
		
		if (!lock)
			lock = 1;
		else if (lock == ELOCKED)
			lock = ESNDLOCKED;
	}
	else
		lock = ENODEV;

	return lock;
}

/* Xbios function 129 (0x81) */
/* If another process owns lock, return = ELOCKED
 * If no GSXB device installed, return = ENODEV
 * If this process already owns the lock, return = E_OK
 * If no lock is set, return ESNDNOTLOCK
 */
long _cdecl
sys_gsxb_unlocksnd(void)
{
	long lock;

	if (gd.dacio)
	{
		lock = gd.dacio(gd.dev_id, gd.dac_id, PCM_IOCTL_SEMAPHORE, GSXB_SEMA_REL);
		if (lock = ENOLOCK)
			lock = ESNDNOTLOCK;
	}
	else
		lock = ENODEV;

	return lock;
}

/* Xbios function 130 (0x82) */
long _cdecl
sys_gsxb_soundcmd(short mode, short data, long data2)
{
	
}

/* Xbios function 131 (0x83) */
long _cdecl
sys_gsxb_setbuffer(short mode, void *begaddr, void *endaddr)
{
}

/* Xbios function 132 (0x84) */
long _cdecl
sys_gsxb_setmode(short mode)
{
}

/* Xbios function 133 (0x85) */
long _cdecl
sys_gsxb_settracks(short play, short rec)
{
}

/* Xbios function 134 (0x86) */
long _cdecl
sys_gsxb_setmontracks(short track)
{
}

/* Xbios function 135 (0x87) */
long _cdecl
sys_gsxb_setinterrupt(short mode, short cause, void *inth)
{
}

/* Xbios function 136 (0x88) */
long _cdecl
sys_gsxb_buffoper(short mode )
{
}

/* Xbios function 137 (0x89) */
long _cdecl
sys_gsxb_dsptristate(short dspxmit, short dsprec)
{
}

/* Xbios function 138 (0x8a) */
long _cdecl
sys_gsxb_gpio(short mode, short data)
{
}

/* Xbios function 139 (0x8b) */
long _cdecl
sys_gsxb_devconnect(short src, short dst, short clk, short prescale, short proto)
{
}

/* Xbios function 140 (0x8c) */
long _cdecl
sys_gsxb_sndstatus(short reset)
{
}

/* Xbios function 141 (0x8d) */
long _cdecl
sys_gsxb_buffptr(void *sptr)
{
}

/* Xbios function 165 (0xa5) */
short _cdecl
sys_gsxb_waveplay( short flags, long rate, void *sptr, long slen)
{
}

