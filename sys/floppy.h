/*
 * floppy.h - floppy routines
 *
 * Copyright (c) 2001 EmuTOS development team
 *
 * Adapted for FreeMiNT by Adam Klobukoski <atari@gabo.pl>
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* Note from Adamk: there was only minor changes in this file, after 
                    I'l have floppy.c cleaned up, I'll clean this up 
                    too.
*/

#ifdef FLOPPY_ROUTINES

#ifndef _floppy_h
#define _floppy_h

/* bios functions */

/* Functions hdv_boot, hdv_init, rwabd, getbpb and mediach are 
 * called using variable pointers. The five flop_* functions
 * are the implementation of these provided by floppy.c.
 */

//extern signed long flop_hdv_boot(void);
//extern void flop_hdv_init(void);

/* xbios functions */

signed short _cdecl floprd(signed long buf, signed long filler, signed short dev, signed short sect, signed short track, signed short side, signed short count); 
signed short _cdecl flopwr(signed long buf, signed long filler, signed short dev, signed short sect, signed short track, signed short side, signed short count); 
signed short _cdecl flopfmt(signed long buf, signed long filler, signed short dev, signed short spt, signed short track, signed short side, signed short interleave, unsigned long magic, signed short virgin); 
void _cdecl protobt(signed long buf, signed long serial, signed short type, signed short exec);
signed short _cdecl flopver(signed long buf, signed long filler, signed short dev, signed short sect, signed short track, signed short side, signed short count); 
signed short _cdecl floprate(signed short dev, signed short rate);

/* internal functions */

/* call hdv_boot() and execute bootsector */
//extern void do_hdv_boot(void);  

/* initialise floppy parameters */
extern void init_floppy(void);

/* floppy rwabs */
signed long _cdecl floppy_rwabs(int rwflag, void *buffer, int number, int recno, int dev, long lrecno);

/* floppy block io */
signed long floppy_block_io(DI *di, int rwflag, void *buffer, unsigned long int size, long lrecno);

/* floppy mediach */
long floppy_mediach(unsigned short drv);

/* get intel signed shorts */
//unsigned short getisigned short(unsigned char *addr);

/* lowlevel floppy_rwabs */
//signed long floppy_rw(signed short rw, signed long buf, signed short cnt, signed long recnr, signed short spt, signed short sides, signed short dev);

#endif /* _floppy_h */

#endif /* FLOPPY_ROUTINES */