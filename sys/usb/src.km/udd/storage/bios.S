/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010-2012
 *
 * TOS 4.04 Xbios dispatcher for the CT60/CTPCI boards
 * and USB-disk / Ram-Disk utility
 * Didier Mequignon 2005-2009, e-mail: aniplay@wanadoo.fr
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "../../config.h"
#include "vars.h"

#undef pun_ptr
#define pun_ptr _pun_ptr_usb

	.global	_max_logical_drive

#define MAX_LOGICAL_DRIVE	_max_logical_drive

//.chip 68040

	.extern _usb_stor_read,_usb_stor_write 
	.extern _cconws_from_S, _dsetdrive_from_S, _install_xhdi_driver
	.extern _product_name, _pun_ptr_usb, _pun_usb
	.extern _my_drvbits, _usb_1st_disk_drive

	.global _install_usb_stor,_uninstall_usb_stor

#if 0		
#define DEBUG_BIOS_LAYER	/* Show extra debug info into the console */
#endif
#ifdef DEBUG_BIOS_LAYER
	.global display_string,hex_long,hex_word,hex_byte,display_char,wait_key,_debug
#endif
	.data
	
/* AHDI */

#define PUN_DEV           0x1F /* device number of HD */
#define PUN_UNIT          0x07 /* Unit number */
#define PUN_SCSI          0x08 /* 1=SCSI 0=ACSI */
#define PUN_IDE           0x10 /* Falcon IDE */
#define PUN_USB           0x20 /* USB */
#define PUN_REMOVABLE     0x40 /* Removable media */
#define PUN_VALID         0x80 /* zero if valid */

#define pinfo_puns      0  // 2 bytes
#define pinfo_pun       2  // 32 bytes
#define pinfo_pstart   34  // 32 x 4 bytes
#define pinfo_cookie  162  // 4 bytes
#define pinfo_cookptr 166  // 4 bytes
#define pinfo_vernum  170  // 2 bytes
#define pinfo_maxsiz  172  // 2 bytes
#define pinfo_ptype   174  // 32 x 4 bytes
#define pinfo_psize   302  // 32 x 4 bytes
#define pinfo_flags   430  // 32 x 2 bytes, internal use: B15:swap, B7:change, B0:bootable
#define pinfo_bpb     494  // 32 x 18 bytes
#define pinfo_devnum 1070  // 32 bytes 
#define pinfo_size   1102


#ifdef DEBUG_BIOS_LAYER
debug3:	
	.asciz "hdv_rw 0x"
debug4:
	.ascii "hdv_bpb"
	.byte 13,10,0
debug5:	
	.ascii "hdv_mediach"
	.byte 13,10,0
	.align 2
#endif

	.text

text_color:
	moveq #0,D0
	move.w 0x3E86,D0         // number of planes
	cmp.l #2,D0
	bls.s .black_and_white
	move.l A0, -(SP)
	jsr _cconws_from_S                  // Cconws
	addq.l #4,SP
.black_and_white:
	rts

_install_usb_stor:
	lea -32(SP),SP
	movem.l D1-D4/A0-A3,(SP)
	move.l 36(SP),D0         // dev_num
	cmp.l #PUN_DEV,D0
	bhi .no_pinfo            // error
	move.l 40(SP),D2         // part_type
	move.l D2,D1
	and.l #0xFFFFFF,D1       // ID
	// GEMDOS
	cmp.l #0x47454D,D1       // GEM up to 16M
	beq.s .partition_ok
	cmp.l #0x42474D,D1       // BGM over 16M
	beq.s .partition_ok
	cmp.l #0x524157,D1       // RAW
	beq.s .partition_ok
	// DOS 1:FAT12, 0xB/0xC:FAT32
	cmp.l #0x4,D2            // FAT16 up to 32M
	beq.s .partition_ok
	cmp.l #0x6,D2            // FAT16 over 32M
	beq.s .partition_ok
	cmp.l #0xE,D2            // WIN95 FAT16
	beq .partition_ok
	cmp.l #0xB,D2		// FAT32
	beq.s .partition_ok
	cmp.l #0xC,D2		// FAT32
	beq.s .partition_ok
	cmp.l #0x81,D2		// MINIX
	beq.s .partition_ok
	cmp.l #0x83,D2		// EXT2/LNX
	bne .no_pinfo
.partition_ok:
	lea _pun_usb,A3
	move.l A3,pun_ptr
	clr.w pinfo_puns(A3)
	move.w #0x4000,D0
	move.w D0,pinfo_maxsiz(A3)
	lea pinfo_pun(A3),A0
	moveq #-1,D0
	move.w D0,(A0)+          // drives A/B
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.l D0,(A0)+
	move.w D0,(A0)
.pinfo_ok:
	moveq #2,D4              // drive C 
	move.l _drvbits,D0
.search_empty_drive_usb:
	btst D4,D0
	beq.s .drive_not_exist_usb
	addq.l #1,D4
	cmp.l #MAX_LOGICAL_DRIVE,D4
	bcs.s .search_empty_drive_usb
	bra .drive_full_usb          // all drives already used 
.no_pinfo:
	moveq #0,D0              // not installed 
	bra .end_usb_disk
.drive_not_exist_usb:
	move.w pinfo_puns(A3),D0
	addq.l #1,D0
	move.w D0,pinfo_puns(A3)
	moveq #0,D0
	bset #7,D0               // changed
	lea pinfo_flags(A3),A0
	move.l D0,(A0,D4.l*4)    // B15:swap, B7:change, B0:bootable
	lea pinfo_psize(A3),A0
	move.l 40(SP),D2         // part_type
	cmp.l #0x4,D2            // FAT16 up to 32M
	beq.s .dos_partition
	cmp.l #0x6,D2            // FAT16 over 32M
	beq.s .dos_partition
	cmp.l #0xE,D2            // WIN95 FAT16
	beq .dos_partition
	cmp.l #0xB,D2		// FAT32
	beq.s .dos_partition
	cmp.l #0xC,D2		// FAT32
	beq.s .dos_partition
	move.l D2,pinfo_ptype(A3,D4.l*4)
	jmp .end_ptype
.dos_partition:
	clr.l D3
	move.b D2,D3
	move.w #0x0044,pinfo_ptype(A3,D4.l*4)          		// NULL+ 'D'
	move.b D3,pinfo_ptype+2(A3,D4.l*4)
.end_ptype:
	move.l 48(SP),D3         // part_size
	move.l 44(SP),D1         // part_offset
	move.l 36(SP),D0         // dev_num
	move.b D0,pinfo_devnum(A3,D4.l)
	move.l D4,D2
	or.l #PUN_USB,D2
	move.b D2,pinfo_pun(A3,D4.l)
.ahdi_part_num_limit:			
	move.l D3,(A0,D4.l*4)    // size
	move.l D1,pinfo_pstart(A3,D4.l*4)	
	move.l _dskbufp,A0 
	move.l A0,-(SP)          // buffer
	move.l #1,-(SP)          // blkcnt	
	move.l D1,-(SP)          // blknr
	move.l D0,-(SP)          // devnum
	jsr _usb_stor_read
	lea 16(SP),SP
	tst.l D0
	beq .end_usb_disk        // read error
	tst.l _usb_1st_disk_drive
	bne .usb_1st_drive_ok    // hdv vectors installed
	move.l D4,_usb_1st_disk_drive
	move.w SR,D0
	move.w D0,-(SP)
	or.l #0x700,D0           // mask interrupts
	move.w D0,SR
	
	move.l #hdv_bpb,A0
	lea old_hdv_bpb_usb(PC),A1
	move.l (A0),(A1)
	lea det_hdv_bpb_usb(PC),A1
	move.l A1,(A0)
	
	move.l #hdv_rw,A0
	lea old_hdv_rw_usb(PC),A1
	move.l (A0),(A1)
	lea det_hdv_rw_usb(PC),A1
	move.l A1,(A0)

	move.l #hdv_mediach,A0
	lea old_hdv_mediach_usb(PC),A1
	move.l (A0),(A1)
	lea det_hdv_mediach_usb(PC),A1
	move.l A1,(A0)

	jsr _install_xhdi_driver
	move.w (SP)+,D0
	move.w D0,SR		// restore interrupts
.usb_1st_drive_ok:
	move.l _dskbufp,A0       // boot sector 
	lea pinfo_bpb(A3),A1
	move.l D4,D2             // logical drive 
	mulu #18,D2              // * 18
	add.l D2,A1
	moveq #0,D2
	move.b 0xC(A0),D2
	asl.l #8,D2
	move.b 0xB(A0),D2        // BPS 
	move.w D2,(A1)           // sector size 
	moveq #0,D1
	move.b 0xD(A0),D1        // SPC 
	move.w D1,2(A1)          // cluster size in sectors 
	move.w D1,D0
	mulu D2,D0	
	move.w D0,4(A1)          // cluster size in bytes 
	moveq #0,D0
	move.b 0x12(A0),D0
	asl.l #8,D0
	move.b 0x11(A0),D0       // NDIRS 
	asl.l #5,D0              // * 32 

	divu D2,D0               // / sector size 

	move.w D0,6(A1)          // size directory in sectors 
	moveq #0,D2
	move.b 0x17(A0),D2
	asl.l #8,D2
	move.b 0x16(A0),D2       // SPF 
	move.w D2,8(A1)          // FAT size 
	moveq #0,D0
	move.b 0xF(A0),D0
	asl.l #8,D0
	move.b 0xE(A0),D0        // RES 
	move.l D0,D3
	add.l D2,D3              // + FAT size 
	move.w D3,10(A1)         // 1st sector of FAT2  
	moveq #0,D3
	move.b 0x10(A0),D3       // NFATS 
	mulu D2,D3               // * FAT size 
	add.l D0,D3              // + RES 
	moveq #0,D0
	move.w 6(A1),D0          // size directory in sectors 
	add.l D3,D0
	move.w D0,12(A1)         // 1st data sector
	moveq #0,D2
	move.b 0x14(A0),D2
	asl.l #8,D2
	move.b 0x13(A0),D2       // NSECTS
	bne.s .nsects_ok_usb
	lea pinfo_psize(A3),A2
	move.l (A2,D4.w*4),D2    // partition size in sectors 
	sub.l D0,D2              // - 1st data sector 
.nsects_ok_usb:
	divu D1,D2
	move.w D2,14(A1)         // total clusters 
	moveq #1,D0 
	move.w D0,16(A1)         // FAT 16 
	move.l _drvbits,D0
	bset D4,D0
	move.l D0,_drvbits
	move.l _my_drvbits,D0	// drives handle by us
	bset D4,D0
	move.l D0,_my_drvbits
	moveq #2,D0              // drive C 
	cmp.l D4,D0
	bne.s .no_set_drive_usb
	move.w D0,_bootdev
	move.w D0,-(SP)
	jsr _dsetdrv_from_S
	addq.l #2,SP
.no_set_drive_usb:
	move.l 36(SP),D0         // USB devnum
	movem.l 52(SP),A1/A2/A3  // vendor / revision / product
	lea _product_name,A0      // save product name pointer for XHDI
	move.l A3,(A0,D4.l*4)
	bsr display_drive_usb
	pea message2b(PC)
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP
	moveq #0x41,D0           // A
	add.l D4,D0
	move.w D0,-(SP)
	jsr _bconout_from_S      // Cconout
	addq.l #2,SP
	pea crlf(PC)
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP
	move.l D4,D0             // OK
	bra.s .end_usb_disk
.invalid_partition_type:
	pea error4(PC)
	bra.s .display_error_usb
.drive_full_usb:
	pea error2(PC)
.display_error_usb:
	move.l 36+4(SP),D0       // devnum 
        movem.l 52+4(SP),A1/A2/A3 // vendor / revision / product
	bsr display_drive_usb
	move.w #0x2C,-(SP)
	jsr _bconout_from_S      // Cconout
	addq.l #2,SP
	move.w #0x20,-(SP)
	jsr _bconout_from_S      // Cconout
	addq.l #2,SP
	jsr _cconws_from_S       // Cconws
	addq.l #4,SP
	moveq #0,D0              // not installed 
.end_usb_disk:
	movem.l (SP),D1-D4/A0-A3
	lea 32(SP),SP
	rts

display_drive_usb:
	move.l A1,-(SP)
	moveq #0x30,D1
	add.l D1,D0	         // dev_num
	move.w D0,-(SP)        
	lea blue(PC),A0   
	bsr text_color
	pea message2(PC)         // USB-disk installed 
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
	move.w #0x2E,-(SP)
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
	move.w #0x30,-(SP)
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
	lea black(PC),A0
	bsr text_color
	move.w #0x20,-(SP)
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP    
	move.w #0x20,-(SP)
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
 	move.l A2,-(SP)
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP             
	move.w #0x20,-(SP)
	jsr _bconout_from_S      // Cconout 
	addq.l #2,SP
	move.l A3,-(SP)
	jsr _cconws_from_S       // Cconws 
	addq.l #4,SP
	rts

_uninstall_usb_stor:
	lea -32(SP),SP
	movem.l D1-D4/A0-A3,(SP)
	move.l 36(SP),D3         // partition to uninstall
	cmp.l #32,D3
	bhi .error            // error

.ahdi_part_num_limit_2:	
	move.l pun_ptr,D0
	move.l D0,A3
	moveq #-1,D0
	move.b D0, pinfo_pun(A3,D3.l)
	clr.l D0
	move.l D0,pinfo_pstart(A3,D3.l)
	move.w pinfo_puns(A3),D0
	subq.w #1,D0
	move.w D0,pinfo_puns(A3)
	move.l _drvbits,D0
	bclr D3,D0
	move.l D0,_drvbits
	move.l _my_drvbits,D0
	bclr D3,D0
	move.l D0,_my_drvbits
	move.l D3,D0
.error:
	movem.l (SP),D1-D4/A0-A3
	lea 32(SP),SP
	rts

	dc.l	0x58425241	// XBRA
	dc.l	0x5F555342	// ID: _USB
old_hdv_bpb_usb:
	dc.l	0

det_hdv_bpb_usb:
	move.l A0,-(SP)
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 4+4(SP),D0        // drive
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .dhbu2
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .dhbu2
	tst.b pinfo_pun(A0,D0.l)
	bpl.s .dhbu1
.dhbu2:
	move.l (SP)+,A0
	moveq #0,D0
	move.l old_hdv_bpb_usb,-(SP)
	rts
.dhbu1:
	move.l D1,-(SP)
	move.l pinfo_ptype(A0,D0.l*4),D1
	and.l #0xFFFFFF,D1
	cmp.l #0x524157,D1       // RAW
	beq.s .dhbu4
	cmp.l #0x81,D1       	 // MINIX
	beq.s .dhbu4
	cmp.l #0x83,D1       	 // EXT2/LNX
	bne.s .dhbu3
.dhbu4:
	move.l (SP)+,D1
	move.l (SP)+,A0
	moveq #0,D0
	rts	
.dhbu3:
	lea pinfo_bpb(A0),A0
	asl.l #5,D0              // * 32
	add.l A0,D0
#ifdef DEBUG_BIOS_LAYER
	move.l D0,-(SP)
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.l (SP),D0
	jsr hex_long
	moveq #0x20,D0
	jsr display_char
	lea debug4(PC),A0
	jsr display_string
	move.l (SP)+,D0
#endif
	move.l (SP)+,D1
	move.l (SP)+,A0
	rts

	dc.l	0x58425241	// XBRA
	dc.l	0x5F555342	// ID: _USB
old_hdv_rw_usb:
	dc.l	0

det_hdv_rw_usb:
	lea -28(SP),SP
	movem.l D1-D4/A0-A2,(SP)
#ifdef DEBUG_BIOS_LAYER
	lea debug3(PC),A0
	jsr display_string	
	move.w 4+28(SP),D0       // rwflag
	jsr hex_word
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.l 6+28(SP),D0       // buffer
	jsr hex_long
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.w 10+28(SP),D0      // num sectors
	jsr hex_word
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.w 12+28(SP),D0      // logical sector
	jsr hex_word
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.w 16+28(SP),D0      // Galvez: logical sector (lrecno)
	jsr hex_long
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.w 14+28(SP),D0      // drive
	jsr hex_word
	moveq #13,D0
	jsr display_char
	moveq #10,D0
	jsr display_char
#endif
	btst #3,5+28(SP)         // rwflag
	bne.s .dhru8             // physical
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 14+28(SP),D0      // drive
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .dhru8
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .dhru8
	moveq #0,D4
	move.b pinfo_pun(A0,D0.l),D4
	bpl.s .dhru1             // valid
.dhru8:
	movem.l (SP),D1-D4/A0-A2
	lea 28(SP),SP
	moveq #0,D0
	move.l old_hdv_rw_usb,-(SP)
	rts
.dhru1:
	move.l D4,D2
	and.l #PUN_USB,D2
	beq.s .dhru8             // not USB
	and.l #PUN_DEV,D4
	moveq #0,D2
	move.w 12+28(SP),D2      // logical sector
	cmp.w #0xffff,D2         // Galvez: check recno <> -1
	bne.s .dhru6		
	move.l 16+28(SP),D2      // logical sector
.dhru6:
	tst.l D2
	bmi .dhru2               // negative logical sector
	move.l 6+28(SP),D1       // buffer
	beq .dhru4               // no buffer
	move.l pinfo_pstart(A0,D0.l*4),D3
	move.l pinfo_devnum(A0,D0.l),D4	 // devnum in the USB bus
	lea pinfo_bpb(A0),A0
	asl.l #5,D0              // * 32
	add.l D0,A0
	move.w 14(A0),D0         // total clusters
	mulu.w 2(A0),D0          // cluster size in sectors
	cmp.l D0,D2              // logical sector to hight
	bcc .dhru2
	moveq #0,D0
	move.w (A0),D0           // sector size
	lsr.l #8,D0
	lsr.l #1,D0              // / 512
	move.l D1,A0             // buffer
	move.w 10+28(SP),D1      // num sectors
	beq .dhru4               // no sectors
	mulu D0,D1
	mulu.l D0,D2
	add.l D3,D2              // start sector
	move.l D1,D3             // count
	btst #0,5+28(SP)         // rwflag
	beq.s .dhru7             // read
	// write
	tst.l D2                 // logical sector
	beq.s .dhru2             // root sector
	move.l A0,-(SP)          // buffer
	move.l D3,-(SP)          // blkcnt	
	move.l D2,-(SP)          // blknr
	move.l D4,-(SP)          // USB devnum 
	jsr _usb_stor_write
	bra.s .dhru5
.dhru2:
	moveq #-1,D0             // error
	bra.s .dhru3
.dhru4:
	moveq #0,D0              // OK
	bra.s .dhru3
.dhru7:
	move.l A0,-(SP)          // buffer
	move.l D3,-(SP)          // blkcnt	
	move.l D2,-(SP)          // blknr
	move.l D4,-(SP)          // USB devnum
	jsr _usb_stor_read
.dhru5:
	lea 16(SP),SP
	tst.l D0
	seq.b D0
	ext.w D0
	ext.l D0
	bclr #0,D0               // OK or device not responding -2
.dhru3:
	movem.l (SP),D1-D4/A0-A2
	lea 28(SP),SP
 	rts

	dc.l	0x58425241	// XBRA
	dc.l	0x5F555342	// ID: _USB
old_hdv_mediach_usb:
	dc.l	0

det_hdv_mediach_usb:
	move.l A0,-(SP)
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 4+4(SP),D0        // drive
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .dhmu2
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .dhmu2
	tst.b pinfo_pun(A0,D0.l)
	bpl.s .dhmu1
.dhmu2:
	move.l (SP)+,A0
	moveq #0,D0
	move.l old_hdv_mediach_usb,-(SP)
	rts
.dhmu1:
#ifdef DEBUG_BIOS_LAYER
	move.l A0,-(SP)
	lea debug5(PC),A0
	jsr display_string
	move.l (SP)+,A0
#endif
	lea pinfo_flags(A0),A0
	add.l D0,A0
	add.l D0,A0
	bclr #7,1(A0)
	sne.b D0
	and.l #2,D0
	move.l (SP)+,A0
	rts

message2:	.asciz "USB  "
message2b:	.asciz ", disk installed in "


crlf:	.byte 13,10,0
error2:	.ascii "all drives already used"
	.byte 13,10,0
error4:	.ascii "partition type not supported"
	.byte 13,10,0
blue:	.byte 0x1B,0x62,0x34,0
black:	.byte 0x1B,0x62,0x3F,0
