/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Holger Schulz <HolgerSchulz@t-online.de>
 * All rights reserved.
 *
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * This file is free software  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation  either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program  if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * emulator for unimplemented FPU and integer instructions of the 68060
 * version 0.05 from 15.03.2004
 */

#include "mint/asmdefs.h"
#define fpsr REG(fpsr)
#define fpcr REG(fpcr)
#define sfc REG(sfc)
#define dfc REG(dfc)
#define vbr REG(vbr)

XBRA      = 0x58425241              /* "XBRA" */
XBRA_ID   = 0x4d435350              /* "MCSP" */

XBRA_FPSP = 0x46505350              /* "FPSP" */
XBRA_AB40 = 0x41423430              /* "AB40" */
XBRA_MCSP = 0x4d435350              /* "MCSP" */

C__CPU    = 0x5f435055              /* "_CPU" */
C_CT60    = 0x43543630              /* "CT60" */

    .text

    .globl _init

/*
 * not a C symbol: this is directly referenced in the Makefile
 */
_init:

/*
 * GEMDOS startup code:
 * get basepage, calculate program size, and shrink memory
 */
    movea.l 4(sp),a5
    move.l  12(a5),d6       /*   size of text segment */
    add.l   20(a5),d6       /* + size of data segment */
    add.l   28(a5),d6       /* + size of bss segment */
    addi.l  #0x100,d6       /* + size of BASEPAGE */
/* Setup a (very small) stack in the commandline */
    lea     256(a5),a7

    /* Free not required memory */

    move.l  d6,-(sp)
    move.l  a5,-(sp)
    clr.w   -(sp)
    move.w  #74,-(sp)       /* Mshrink */
    trap    #1
    lea     12(sp),sp

    clr.l   -(sp)           /* switch to supervisor mode */
    move.w  #32,-(sp)       /* Super */
    trap    #1
    addq.l  #6,sp
    move.l  d0,d7           /* save old Super-Stack-Pointer */

    movec    vbr,a1

/*
 * check cookie for CPU
 * - exit if no '060 found
 * - also check for CT60, which has the FPSP in ROM,
 *   but does not use XBRA for the vectors.
 */
    movea.l 0x5A0,a5        /* cookie jar */
    clr.l   d1
cpu_ident:
    move.l  (a5)+,d0        /* id */
    beq.b   cpu_ident_end
    move.l  (a5)+,d2        /* value */
    cmp.l   #C_CT60,d0
    beq.b   is_ct60
    cmp.l   #C__CPU,d0      /* '_CPU' ? */
    bne.b   cpu_ident
    move.l  d2,d1
    bra.s   cpu_ident
cpu_ident_end:
    cmp.l   #60,d1          /* 68060 ? */
    beq.b   cpu_060

/* keinen 68060 mit CPU gefunden */

no_cpu060:
    lea no_060_msg(pc),a0
print_and_exit:
    bsr print
exit:
    move.l  d7,-(sp)        /* alter SSP */
    move.w  #32,-(sp)       /* Super */
    trap    #1
    addq.l  #6,sp

    clr.w   -(sp)           /* Pterm(0) */
    trap    #1

is_ct60:
    lea ct60_msg(pc),a0
    bra.s print_and_exit

already_installed:
    lea already_installed_msg(pc),a0
    bra.s print_and_exit

/* ---------------------------------------------------------------------------------- */
/* entry for the 68060 CPU                                                            */
/* ---------------------------------------------------------------------------------- */
cpu_060:

    move.l   0xf4(a1),a0
    cmp.l    #XBRA,-12(a0)
    bne.s    doinstall
    /*
     * skip if any other similar package already installed
     */
    cmp.l    #XBRA_ID,-8(a0)
    beq      already_installed
    cmp.l    #XBRA_AB40,-8(a0)
    beq      already_installed
    cmp.l    #XBRA_FPSP,-8(a0)
    beq      already_installed

doinstall:
    lea        new_int_instr(pc),a0
    move.l     0xf4(a1),-4(a0)                  /* save old unimplemented integer vector */
    move.l     a0,0xf4(a1)                      /* set new unimplemented integer vector */

    dc.l    0x4e7a0808      /* movec.l  PCR, d0 */
    btst    #16,d0
    bne set_res             /* EC or LC */

    /* die FPU-Vektoren werden nur gesetzt, wenn auch eine da ist! */

    lea        new_fline(pc),a0
    move.l     0x2c(a1),-4(a0)                  /* save old linef vector */
    move.l     a0,0x2c(a1)                      /* set new linef vector */

    /* Floating-Point SNAN */
    lea     _top_fpsp+0x80+0x00(pc),a0
    move.l  a0,0xd8

    /* Floating-Point Operand Error */
    lea     _top_fpsp+0x80+0x08(pc),a0
    move.l  a0,0xD0

    /* Floating-Point Overflow */
    lea     _top_fpsp+0x80+0x10(pc),a0
    move.l  a0,0xD4

    /* Floating-Point Underflow */
    lea     _top_fpsp+0x80+0x18(pc),a0
    move.l  a0,0xCC

    /* Floating-Point Divide-by-Zero */
    lea     _top_fpsp+0x80+0x20(pc),a0
    move.l  a0,0xC8

    /* Floating-Point Inexact Result */
    lea     _top_fpsp+0x80+0x28(pc),a0
    move.l  a0,0xC4

    /* Floating-Point Unimplemented Data Type */
    lea     _top_fpsp+0x80+0x38(pc),a0
    move.l  a0,0xDC

    /* Unimplemented Effective Address */
    lea     _top_fpsp+0x80+0x40(pc),a0
    move.l  a0,0xF0

    fmove.l #0,fpcr

set_res:                    /* Programm resident halten und beenden */

    lea text_01(pc),a0      /* Startmeldung */
    bsr print               /* ausgeben */

    move.l  d7,-(sp)        /* in alten Modus zurueckschalten */
    move.w  #32,-(sp)       /* Super */
    trap    #1
    addq.l  #6,sp

    clr.w   -(sp)           /* kein Fehler */
    move.l  d6,-(sp)        /* Anzahl residenter Bytes */
    move.w  #49,-(sp)       /* Ptermres */
    trap    #1

/* ------------------------------------------------------------------------------- */
/* print a message                                                                 */
/* IN  : a0    address of text                                                     */
/* ------------------------------------------------------------------------------- */
print:
    move.l  a0,-(sp)
    move.w  #9,-(sp)        /* Cconws */
    trap    #1
    addq.l  #6,sp
    rts

text_01:
    .ascii  "*********************************************"
    .dc.b  13,10
    .ascii  " Unimplemented Integer Instruction and       "
    .dc.b  13,10
    .ascii  " Floating Point Handler Version 0.05         "
    .dc.b  13,10
    .ascii  " Production Release P1.01 - October 10, 1994 "
    .dc.b  13,10
    .ascii  " M68060 Software Package                     "
    .dc.b  13,10
    .ascii  " Copyright (c) 1993, 1994 Motorola Inc.      "
    .dc.b  13,10
    .ascii  " All rights reserved                         "
    .dc.b  13,10
    .ascii  "*********************************************"
    .dc.b  13,10,0

no_060_msg:
    .ascii "No 060 CPU detected, FPSP not installed!"
    .dc.b  13,10,0

ct60_msg:
    .ascii "CT60 detected, FPSP not installed!"
    .dc.b  13,10,0

already_installed_msg:
    .ascii "FPSP already installed!"
    .dc.b  13,10,0

    .even

    .dc.l XBRA
    .dc.l XBRA_ID
    .dc.l 0
new_fline:
    bra _top_fpsp+0x80+0x30

    .dc.l XBRA
    .dc.l XBRA_ID
    .dc.l 0
new_int_instr:
    bra _top_isp+0x80

/* ------------------------------------------------------------------------------------------- */
/* Hier beginnen die Routinen zum Zugriff aus dem ISP                                          */
/* ------------------------------------------------------------------------------------------- */

_060_real_cas:
    bra.l   _top_isp+0x80+0x08

_060_real_cas2:
    bra.l   _top_isp+0x80+0x10


/* ------------------------------------------------------------------------------------------- */
/* memory access routines for the FPSP                                                         */
/* ------------------------------------------------------------------------------------------- */

EXC_SR = 4

/*
 * _060_imem_write/_060_dmem_write()
 *
 * Writes to instruction/data memory.
 *
 * INPUTS:
 *  a0 - source address
 *  a1 - destination address
 *  d0 - number of bytes to write
 *
 * OUTPUTS:
 *  d1 - 0 = success, !0 = failure
 *
 * _060_imem_read(), _060_dmem_read():
 *
 * Reads from instruction/data memory.
 *
 * INPUTS:
 *  a0 - source address
 *  a1 - destination address
 *  d0 - number of bytes to read
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d1 - 0 = success, !0 = failure
 */

_060_imem_read:
_060_dmem_read:
    subq.l      #1,d0           /* dec count by 1 for dbra */
    btst        #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s       super_read
user_read:
    moveq.l     #1,d1
    movec       d1,sfc          /* set sfc for user space */
user_read_loop:
copyinae:
    moves.b     (a0)+,d1
    move.b      d1,(a1)+        /* copy 1 byte */
    dbra        d0,user_read_loop   /* quit if --ctr < 0 */
    clr.l       d1              /* return success */
    rts
super_read:
    move.b      (a0)+,(a1)+     /* copy 1 byte */
    dbra        d0,super_read   /* quit if --ctr < 0 */
    clr.l       d1              /* return success */
    rts


/*
 * Writes to data memory while in supervisor mode.
 *
 *  INPUTS:
 *     a0 - source address
 *     a1 - destination address
 *     d0 - number of bytes to transfer
 *     4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *  OUTPUTS:
 *     d1 - 0 = success, !0 = failure
 */
_060_dmem_write:
    subq.l      #1,d0           /* dec count by 1 for dbra */
    btst        #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s       super_write
user_write:
    moveq.l     #1,d1
    movec       d1,dfc          /* set dfc for user data space */
user_write_loop:
    move.b      (a0)+,d1        /* copy 1 byte */
copyoutae:
    moves.b     d1,(a1)+
    dbra        d0,user_write_loop  /* quit if --ctr < 0 */
    clr.l       d1              /* return success */
    rts
super_write:
    move.b      (a0)+,(a1)+     /* copy 1 byte */
    dbra        d0,super_write  /* quit if --ctr < 0 */
    clr.l       d1              /* return success */
    rts

/*
 * _060_dmem_read_byte():
 *
 * Read a data byte from memory.
 *
 * INPUTS:
 *  a0 - source address
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d0 - data byte in d0
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_read_byte:
    clr.l   d1              /* assume success */
    clr.l   d0              /* clear whole longword */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmrbs           /* supervisor */
dmrbuae:
    moves.b (a0),d0         /* fetch user byte */
    rts
dmrbs:
    move.b  (a0),d0         /* fetch byte */
    rts

/*
 * _060_dmem_write_byte():
 *
 * Write a data byte to memory.
 *
 * INPUTS:
 *  a0 - destination address
 *  d0 - data byte in d0
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_write_byte:
    clr.l   d1              /* assume success */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmwbs           /* supervisor */
dmwbuae:
    moves.b d0,(a0)         /* store user byte */
    rts
dmwbs:
    move.b  d0,(a0)         /* store super byte */
    rts

/*
 * _060_dmem_write_word():
 *
 * Write a data word to memory.
 *
 * INPUTS:
 *  a0 - destination address
 *  d0 - data word in d0
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_write_word:
    clr.l   d1              /* assume success */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmwws           /* supervisor */
dmwwuae:
    moves.w d0,(a0)         /* store user word */
    rts
dmwws:
    move.w  d0,(a0)         /* store super word */
    rts

/*
 * _060_dmem_write_long():
 *
 * Write a data longword to memory.
 *
 * INPUTS:
 *  a0 - user destination address
 *  d0 - data longword in d0
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_write_long:
    clr.l   d1              /* assume success */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmwls           /* supervisor */
dmwluae:
    moves.l d0,(a0)         /* store user longword */
    rts
dmwls:
    move.l  d0,(a0)         /* store super longword */
    rts

/*
 * _060_imem_read_word():
 *
 * Read an instruction word from memory.
 *
 * INPUTS:
 *  a0 - source address
 *  4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d0 - instruction word in d0
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_read_word:
_060_imem_read_word:
    clr.l   d1              /* assume success */
    clr.l   d0              /* clear whole longword */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmrws           /* supervisor */
dmrwuae:
    moves.w (a0),d0         /* fetch user word */
    rts
dmrws:
    move.w  (a0),d0         /* fetch super word */
    rts

/*
 * _060_imem_read_long():
 *
 * Read an instruction longword from memory.
 *
 * INPUTS:
 *  a0 - source address
 * 4(a6),bit5 - 1 = supervisor mode, 0 = user mode
 *
 * OUTPUTS:
 *  d0 - instruction longword in d0
 *  d1 - 0 = success, !0 = failure
 */
_060_dmem_read_long:
_060_imem_read_long:
    clr.l   d1              /* assume success */
    btst    #5,EXC_SR(a6)   /* check for supervisor state */
    bne.s   dmrls           /* supervisor */
dmrluae:
    moves.l (a0),d0         /* fetch user longword */
    rts
dmrls:
    move.l  (a0),d0         /* fetch super longword */
    rts

_060_real_trace:
    move.l  0x24,-(sp)
    rts

_060_real_access:
    move.l  0x08,-(sp)
    rts

/*
 * _060_real_fline():
 *
 * This is the exit point for the 060FPSP when an F-Line Illegal exception
 * is encountered. Three different types of exceptions can enter the F-Line
 * exception vector number 11: FP Unimplemented Instructions, FP implemented
 * instructions when the FPU is disabled, and F-Line Illegal instructions.
 * The 060FPSP module _fpsp_fline() distinguishes between the three and acts
 * appropriately. F-Line
 * Illegals branch here.
 */
_060_real_fline:
    move.l  new_fline-4(pc),-(sp)
    rts

/*
 * _060_real_trap():
 *
 * This is the exit point for the 060FPSP when an emulated 'ftrapcc'
 * instruction discovers that the trap condition is true and it should
 * branch to the operating system handler for the trap exception vector
 * number 7.
 *
 * The sample code below simply executes an 'rte'.
 */
_060_real_trap:
    rte

/*
 * _060_fpsp_done():
 *
 * This is the main exit point for the 68060 Floating-Point
 * Software Package. For a normal exit, all 060FPSP routines call this
 * routine. The operating system can do system dependent clean-up or
 * simply execute an 'rte' as with the sample code below.
 */
_060_fpsp_done:
    rte

/*
 * _060_real_bsun():
 *
 * This is the exit point for the 060FPSP when an enabled bsun exception
 * is present. The routine below should point to the operating system handler
 * for enabled bsun exceptions. The exception stack frame is a bsun
 * stack frame.
 *
 * The sample routine below clears the exception status bit, clears the NaN
 * bit in the FPSR, and does an rte. The instruction that caused the
 * bsun will now be re-executed but with the NaN FPSR bit cleared.
 */
_060_real_bsun:
    fsave       -(sp)
    fmovem.l    fpsr,-(sp)
    andi.b      #0xfe,(sp)
    fmovem.l    (sp)+,fpsr
    lea     12(sp),sp
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_snan():
 *
 * This is the exit point for the 060FPSP when an enabled signalling NaN exception
 * is present. The routine below should point to the operating system handler
 * for enabled signalling NaN exceptions. The exception stack frame is a signalling NaN
 * stack frame. The FP state frame holds the source operand of the faulting
 * instruction.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte'.
 */
_060_real_snan:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_operr():
 *
 * This is the exit point for the 060FPSP when an enabled operand error exception
 * is present. The routine below should point to the operating system handler
 * for enabled operand error exceptions. The exception stack frame is an operand error
 * stack frame. The FP state frame holds the source operand of the faulting
 * instruction.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte'.
 */
_060_real_operr:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_ovfl():
 *
 * This is the exit point for the 060FPSP when an enabled overflow exception
 * is present. The routine below should point to the operating system handler
 * for enabled overflow conditions. The exception stack frame is an overflow
 * stack frame. The FP state frame holds the EXCEPTIONAL OPERAND.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte'.
 */
_060_real_ovfl:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_unfl():
 *
 * This is the exit point for the 060FPSP when an enabled underflow exception
 * is present. The routine below should point to the operating system handler
 * for enabled underflow conditions. The exception stack frame is an underflow
 * stack frame. The FP state frame holds the EXCEPTIONAL OPERAND.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte'.
 */
_060_real_unfl:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_dz():
 *
 * This is the exit point for the 060FPSP when an enabled divide-by-zero exception
 * is present. The routine below should point to the operating system handler
 * for enabled divide-by-zero exceptions. The exception stack frame is a divide-by-zero
 * stack frame. The FP state frame holds the source operand of the faulting
 * instruction.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte.'
 */
_060_real_dz:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_inex():
 *
 * This is the exit point for the 060FPSP when an enabled inexact exception
 * is present. The routine below should point to the operating system handler
 * for enabled inexact exceptions. The exception stack frame is an inexact
 * stack frame. The FP state frame holds the source operand of the faulting
 * instruction.
 *
 * The sample routine below simply clears the exception status bit and
 * does an 'rte'.
 */
_060_real_inex:
    fsave       -(sp)
    move.w      #0x6000,2(sp)
    frestore    (sp)+
    fmovem.l    #0,fpcr
    rte

/*
 * _060_real_fpu_disabled():
 *
 * This is the exit point for the 060FPSP when an FPU disabled exception is
 * encountered. Three different types of exceptions can enter the F-Line exception
 * vector number 11: FP Unimplemented Instructions, FP implemented  instructions when
 * the FPU is disabled, and F-Line Illegal instructions. The 060FPSP module
 * _fpsp_fline() distinguishes between the three and acts appropriately. FPU disabled
 * exceptions branch here.
 *
 * The sample code below enables the FPU, sets the PC field in the exception stack
 * frame to the PC of the instruction causing the exception, and does an 'rte'.
 * The execution of the instruction then proceeds with an enabled floating-point
 * unit.
 */
_060_real_fpu_disabled:
    move.l  d0,-(sp)
    dc.l    0x4e7a0808      /* movec.l pcr, d0 */
    bclr    #1,d0
    dc.l    0x4e7b0808      /* movec.l d0, pcr */
    move.l  (sp)+,d0
    move.l  12(sp),2(sp)
    fmovem.l    #0,fpcr
    rte

_060_real_chk:
    tst.b   (sp)            /* is tracing enabled? */
    bpl.b   real_chk_end    /* no */
    move.b  #0x24,0x7(sp)   /* set trace vecno */
    bra.w   _060_real_trace
real_chk_end:
    rte

_060_real_divbyzero:
    tst.b   (sp)            /* is tracing enabled? */
    bpl.b   real_divbyzero_end  /* no */
    move.b  #0x24,0x7(sp)   /* set trace vecno */
    bra.l   _060_real_trace
real_divbyzero_end:
    rte

/*
 * _060_lock_page():
 *
 * Entry point for the operating system's routine to "lock" a page
 * from being paged out. This routine is needed by the cas/cas2
 * algorithms so that no page faults occur within the "core" code
 * region. Note: the routine must lock two pages if the operand
 * spans two pages.
 * NOTE: THE ROUTINE SHOULD RETURN AN FSLW VALUE IN D0 ON FAILURE
 * SO THAT THE 060SP CAN CREATE A PROPER ACCESS ERROR FRAME.
 * Arguments:
 *  a0 = operand address
 *  d0 = `xxxxxxff -> supervisor; `xxxxxx00 -> user
 *  d1 = `xxxxxxff -> longword; `xxxxxx00 -> word
 * Expected outputs:
 *  d0 = 0 -> success; non-zero -> failure
 */
_060_real_lock_page:
/*
 * _060_unlock_page():
 *
 * Entry point for the operating system's routine to "unlock" a
 * page that has been "locked" previously with _real_lock_page.
 * Note: the routine must unlock two pages if the operand spans
 * two pages.
 * Arguments:
 *  a0 = operand address
 *  d0 = `xxxxxxff -> supervisor; `xxxxxx00 -> user
 *  d1 = `xxxxxxff -> longword; `xxxxxx00 -> word
 */
_060_real_unlock_page:
    clr.l   d0
    rts

_060_isp_done:
    rte

/* ----------------------------------------------------------------------------- */
/* jump entry table into the ISP functions                                       */
/* This area must be exactly 128 bytes!                                          */
/* ----------------------------------------------------------------------------- */
_top_isp:
    dc.l    _060_real_chk         - _top_isp    /* or 0xf4 */
    dc.l    _060_real_divbyzero   - _top_isp
    dc.l    _060_real_trace       - _top_isp
    dc.l    _060_real_access      - _top_isp
    dc.l    _060_isp_done         - _top_isp
    dc.l    _060_real_cas         - _top_isp
    dc.l    _060_real_cas2        - _top_isp
    dc.l    _060_real_lock_page   - _top_isp
    dc.l    _060_real_unlock_page - _top_isp
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    _060_imem_read        - _top_isp
    dc.l    _060_dmem_read        - _top_isp
    dc.l    _060_dmem_write       - _top_isp
    dc.l    _060_imem_read_word   - _top_isp
    dc.l    _060_imem_read_long   - _top_isp
    dc.l    _060_dmem_read_byte   - _top_isp
    dc.l    _060_dmem_read_word   - _top_isp
    dc.l    _060_dmem_read_long   - _top_isp
    dc.l    _060_dmem_write_byte  - _top_isp
    dc.l    _060_dmem_write_word  - _top_isp
    dc.l    _060_dmem_write_long  - _top_isp
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0

#include "isp.sa"

/* ----------------------------------------------------------------------------- */
/* jump entry table into the FPSP functions                                      */
/* This area must be exactly 128 bytes!                                          */
/* ----------------------------------------------------------------------------- */
_top_fpsp:
    dc.l    _060_real_bsun         - _top_fpsp
    dc.l    _060_real_snan         - _top_fpsp
    dc.l    _060_real_operr        - _top_fpsp
    dc.l    _060_real_ovfl         - _top_fpsp
    dc.l    _060_real_unfl         - _top_fpsp
    dc.l    _060_real_dz           - _top_fpsp
    dc.l    _060_real_inex         - _top_fpsp
    dc.l    _060_real_fline        - _top_fpsp  /* or 0x2C */
    dc.l    _060_real_fpu_disabled - _top_fpsp
    dc.l    _060_real_trap         - _top_fpsp
    dc.l    _060_real_trace        - _top_fpsp
    dc.l    _060_real_access       - _top_fpsp
    dc.l    _060_fpsp_done         - _top_fpsp
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    _060_imem_read         - _top_fpsp
    dc.l    _060_dmem_read         - _top_fpsp
    dc.l    _060_dmem_write        - _top_fpsp
    dc.l    _060_imem_read_word    - _top_fpsp
    dc.l    _060_imem_read_long    - _top_fpsp
    dc.l    _060_dmem_read_byte    - _top_fpsp
    dc.l    _060_dmem_read_word    - _top_fpsp
    dc.l    _060_dmem_read_long    - _top_fpsp
    dc.l    _060_dmem_write_byte   - _top_fpsp
    dc.l    _060_dmem_write_word   - _top_fpsp
    dc.l    _060_dmem_write_long   - _top_fpsp
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0
    dc.l    0

#include "fpsp.sa"
