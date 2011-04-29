/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
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

#define old_pun_ptr 	0x516

#undef pun_ptr
#define pun_ptr pun_ptr_usb

	.global	_max_logical_drive

#define MAX_LOGICAL_DRIVE	_max_logical_drive

//.chip 68040


	.global _install_usb_stor,_uninstall_usb_stor,_usb_stor_read,_usb_stor_write,_usb_1st_disk_drive
	.global _kmalloc_from_S, _cconws_from_S, _dsetdrive_from_S, _XHDINewCookie_from_S

//#define DEBUG_BIOS_LAYER	/* Show extra debug info into the console */ 
//#ifdef DEBUG_BIOS_LAYER
//	.global display_string,hex_long,hex_word,hex_byte,display_char,wait_key,_debug
//#endif

	.data

/* XHDI */

#define XH_DL_SECSIZ 0 // maximal sector size (BIOS level)
#define XH_DL_MINFAT 1 // minimal number of FATs
#define XH_DL_MAXFAT 2 // maximal number of FATs
#define XH_DL_MINSPC 3 // sectors per cluster minimal
#define XH_DL_MAXSPC 4 // sectors per cluster maximal
#define XH_DL_CLUSTS 5 // maximal number of clusters of a 16 bit FAT
#define XH_DL_MAXSEC 6 // maximal number of sectors
#define XH_DL_DRIVES 7 // maximal number of BIOS drives supported by the DOS

	
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
#define pinfo_bpb     494  // 32 x 32 bytes
#define pinfo_devnum 1518  // 32 bytes 
#define pinfo_size   1550


#ifdef DEBUG_BIOS_LAYER
//debug1:	.asciz "XBIOS #0x"
//debug2:	.asciz "Setscreen 0x"
debug3:	
	.asciz "hdv_rw 0x"
debug4:
	.ascii "hdv_bpb"
	.byte 13,10,0
debug5:	
	.ascii "hdv_mediach"
	.byte 13,10,0
//debug6:	.asciz "Vsetmode 0x"
//debug7:	.asciz "ValidMode 0x"
//debug8:	.asciz "Gettime 0x
debug132:
	.asciz "XHDI XHReadWrite 0x"
debug133:
	.ascii "XHDI XHInqTarget2"
	.byte 13,10,0
debug134:	.ascii "XHDI XHInqDev"
	.byte 13,10,0
debug135:	.ascii "XHDI XHInqDriver"
	.byte 13,10,0
debug136:	.ascii "XHDI XHInqDev2"
	.byte 13,10,0
debug137:	.ascii "XHDI XHDOSLimits"
	.byte 13,10,0
debug138:	.ascii "GALVEZ DEBUG"		/* Galvez: DEBUG */
	.byte 13,10,0
debug139:	.ascii "XHDI XHReadWrite"	/* Galvez: DEBUG */
	.byte 13,10,0
	.align 2
#endif


//	dc.l 0x58425241          // XBRA
//	dc.l 0x5F504349          // _PCI

text_color:
	moveq #0,D0
	move.w 0x3E86,D0         // number of planes
	cmp.l #2,D0
	bls.s .black_and_white
	pea (A0)
	move.w #9,-(SP)
	jsr _cconws_from_S                  // Cconws
	addq.l #6,SP
.black_and_white:
	rts
	
install_xbra:	// A0: handler, D0: vector, D1: ID 
	lea -28(SP),SP
	movem.l D1-D3/A0-A3,(SP)
	moveq #0,D3
	move.w D0,D3             // vector
	move.l A0,A3             // handler
	move.l D1,-(SP)
	move.l #18,-(SP)         // size
	jsr _kmalloc_from_S
	addq.l #4,SP
	move.l (SP)+,D1
	tst.l D0
	beq.s .error_xbra
	move.l D0,A0
	move.l #0x58425241,(A0)+ // XBRA
	move.l D1,(A0)+
	clr.l (A0)+	
	move.w #0x4EF9,(A0)+     // JMP
	move.l A3,(A0)+          // handler
	lea -10(A0),A0

	cpusha BC

	move.l D3,A1
	move.l (A1),D0
	move.l D0,(A0)+          // old vector
	move.l A0,(A1)           // JMP, new vector
.error_xbra:
	tst.l D0
	movem.l (SP),D1-D3/A0-A3
	lea 28(SP),SP
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
//	tst.l ___mint		// Galvez: NOT MiNT? then go out
//	beq .invalid_partition_type
	cmp.l #0xE,D2            // WIN95 FAT16
	beq .partition_ok
	cmp.l #0xB,D2		// FAT32
	beq.s .partition_ok
	cmp.l #0xC,D2		// FAT32
	bne .partition_ok
	cmp.l #0x81,D2		// MINIX
	bne .partition_ok
	cmp.l #0x83,D2		// EXT2/LNX
	bne .partition_ok
.partition_ok:
	move.l old_pun_ptr,A2	// Galvez: we need it to update hd driver pun struct
	move.l pun_ptr,D0
	bne.s .pinfo_ok
	move.l #pinfo_size,-(SP)
	jsr _kmalloc_from_S
	addq.l #4,SP
	move.l D0,pun_ptr
	beq.s .no_pinfo
	move.l D0,A3
	clr.w pinfo_puns(A3)
	move.w #0x0300,D0
	move.w D0,pinfo_vernum(A3)
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
	lea pinfo_pstart(A3),A0
	lea pinfo_size(A3),A1
.clrpun:
	clr.w -(A1)
	cmp.l A0,A1
	bgt.s .clrpun
	move.l A3,D0             // pun_ptr
.pinfo_ok:
	move.l D0,A3             // pun_ptr 
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
	move.l 48(SP),D3         // part_size
	move.l 44(SP),D1         // part_offset
	move.l 40(SP),D2         // part_type
	move.l 36(SP),D0         // dev_num
	move.b D0,pinfo_devnum(A3,D4.l)
	move.l D2,pinfo_ptype(A3,D4.l*4)
	move.l D4,D2
	or.l #PUN_USB,D2
	move.b D2,pinfo_pun(A3,D4.l)
	cmp.l #15,D4			// Galvez: update AHDI pun struct:
	bgt .ahdi_part_num_limit	// Galvez: if logical part. > 16
	move.b D2,pinfo_pun(A2,D4.l)	// Galvez: don't update pun struct,
	move.w D0,-(SP)			// Galvez: to avoid corruption.
	move.w pinfo_puns(A2),D0	// Galvez: update hd driver pun stuct
	addq.l #1,D0			// Galvez: othewise MiNT only handles 16MB
	move.w D0,pinfo_puns(A2)	// Galvez: when not using XHDI
	move.w (SP)+,D0			// Galvez: see BLOCK_IO.c (MiNT sources)
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
	move.l #0x5F555342,D1    // _USB
	lea det_hdv_bpb_usb(PC),A0
	move.w #hdv_bpb,D0
	bsr install_xbra
	move.l D0,old_hdv_bpb_usb
	lea det_hdv_rw_usb(PC),A0
	move.w #hdv_rw,D0
	bsr install_xbra	
	move.l D0,old_hdv_rw_usb
	lea det_hdv_mediach_usb(PC),A0
	move.w #hdv_mediach,D0
	bsr install_xbra
	move.l D0,old_hdv_mediach_usb
	move.l cookie,D0
	beq.s .no_cookie_jar
	move.l D0,A0
	move.l #0x58484449,D1    // XHDI
.find_cookie_jar:
	tst.l (A0)
	beq.s .cookie_slot_free
	cmp.l (A0),D1
	beq.s .cookie_found
	addq.l #8,A0
	bra.s .find_cookie_jar
.cookie_found:
	move.l 4(A0),D0
	move.l D0,old_xhdi
	clr.l -(SP)              	// XHGetVersion
	move.l D0,A0
	jsr (A0)
	addq.l #4,SP
	move.l D0,old_xhdi_version
	move.l old_xhdi,A0
	lea xhdi(PC),A1
	move.l A1,-(SP)			
	jsr _XHDINewCookie_from_S	// Galvez: XHNewCookie
	add.l #4,SP
	tst.l D0
	beq.s .no_cookie_jar
	move.l A1,4(A0)			// Galvez: Replace cookie "by-hand"			
	bra.s .no_cookie_jar
.cookie_slot_free:
	move.l 4(A0),12(A0)      	// copy size
	lea xhdi(PC),A1
	move.l A1,(A0)+
	clr.l (A0)
	clr.l old_xhdi
	move.l #0x120,D0         	//  protocol version
	move.l D0,old_xhdi_version
.no_cookie_jar:
	move.w (SP)+,D0
	move.w D0,SR		// restore interrupts
.usb_1st_drive_ok:
	move.l _dskbufp,A0       // boot sector 
	lea pinfo_bpb(A3),A1
	move.l D4,D2             // logical drive 
	asl.l #5,D2              // * 32 
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
	clr.w 18(A1)
	clr.l 20(A1)
	clr.l 24(A1)
	clr.l 28(A1)	
	move.l _drvbits,D0
	bset D4,D0
	move.l D0,_drvbits
	move.l my_drvbits,D0	// drives handle by us
	bset D4,D0
	move.l D0,my_drvbits
	moveq #2,D0              // drive C 
	cmp.l D4,D0
	bne.s .no_set_drive_usb
	move.w D0,_bootdev
//	movem.l D0-D2/A0-A2,-(SP)
	move.w D0,-(SP)
	jsr _dsetdrv_from_S
	addq.l #2,SP
//	movem.l (SP)+,D0-D2/A0-A2
.no_set_drive_usb:
	move.l 36(SP),D0         // USB devnum
	movem.l 52(SP),A1/A2/A3  // vendor / revision / product
	move.l product_name,A0      // save product name pointer for XHDI
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

//	add.l _usb_1st_disk_drive,D3 	// BIOS storage device number
	cmp.l #15,D3			// Galvez: update AHDI pun struct:
	bgt .ahdi_part_num_limit_2	// Galvez: if logical part. > 16
	move.l old_pun_ptr,A2		// Galvez: we need it to update hd driver pun struct
	moveq #-1,D0			// Galvez: don't update pun struct,
	move.b D0,pinfo_pun(A2,D3.l)	// Galvez: to avoid corruption.
	move.w pinfo_puns(A2),D0	// Galvez: update hd driver pun stuct
	subq.w #1,D0			// Galvez: othewise MiNT only handles 16MB
	move.w D0,pinfo_puns(A2)	// Galvez: when not using XHDI
					// Galvez: see BLOCK_IO.c (MiNT sources)
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
	move.l my_drvbits,D0
	bclr D3,D0
	move.l D0,my_drvbits
	move.l D3,D0
.error:
	movem.l (SP),D1-D4/A0-A3
	lea 32(SP),SP
	rts

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

	// XHDI
	
	dc.l 0x27011992
	
xhdi:
	link A6,#0
	movem.l D1-A5,-(SP)
	moveq #0,D1
	move.w 8(A6),D1

	move.w D1,D0
#ifdef DEBUG_BIOS_LAYER	
	move.w D0, -(SP)
	jsr hex_word		/* Galvez: DEBUG. XHDI function */
	move.w (SP)+,D0
#endif	
	moveq #-32,D0            // invalid function
	cmp.l #18,d1
	bcc.s .bad_xhdi
	move.w SR,D0             // supervisor only
	moveq #-1,d0             // error
	move.w tab_xhdi(PC,D1.l*2),D1
	jsr tab_xhdi(PC,D1.W)
	
.bad_xhdi:
	movem.l (SP)+,D1-A5
	unlk A6
	rts
#ifdef DEBUG_BIOS_LAYER
debug:
	lea debug138(PC),A0
	jsr display_string
	rts
#endif
	
tab_xhdi:
	dc.w XHGetVersion-tab_xhdi    // 0
	dc.w XHInqTarget-tab_xhdi     // 1
	dc.w XHReserve-tab_xhdi       // 2
	dc.w XHLock-tab_xhdi          // 3
	dc.w XHStop-tab_xhdi          // 4
	dc.w XHEject-tab_xhdi         // 5
	dc.w XHDrvMap-tab_xhdi        // 6
	dc.w XHInqDev-tab_xhdi        // 7
	dc.w XHInqDriver-tab_xhdi     // 8
	dc.w XHNewCookie-tab_xhdi     // 9
	dc.w XHReadWrite-tab_xhdi     // 10
	dc.w XHInqTarget2-tab_xhdi    // 11
	dc.w XHInqDev2-tab_xhdi       // 12
	dc.w XHDriverSpecial-tab_xhdi // 13
	dc.w XHGetCapacity-tab_xhdi   // 14
	dc.w XHMediumChanged-tab_xhdi // 15
	dc.w XHMiNTInfo-tab_xhdi      // 16
	dc.w XHDOSLimits-tab_xhdi     // 17
	
XHGetVersion:
#ifdef DEBUG_BIOS_LAYER
	move.l A0,-(SP)
	lea debug138(PC),A0
	jsr display_string
	move.l (SP)+,A0
#endif
	move.l #0x120,D0         //  protocol version
	move.l old_xhdi_version,D1
	cmp.l D1,D0
	bcs.s .xv1
	move.l D1,D0             // minimum version
.xv1:	
	rts
	
XHInqTarget:
	moveq #32,D2             // stringlen
	bra.s .xi1
	
XHInqTarget2:
#ifdef DEBUG_BIOS_LAYER
	move.l A0,-(SP)
	lea debug133(PC),A0
	jsr display_string
	move.l (SP)+,A0
#endif
	move.w 26(A6),D2         // stringlen
.xi1:
	tst.w 12(A6)             // minor
	bne.s .xi2
	moveq #0,D0
	move.w 10(A6),D0         // major
	cmp.w #PUN_USB,D0
	bcs.s .xi2
	cmp.w #PUN_USB+PUN_DEV,D0
	bls.s .xi3
.xi2:
	tst.l old_xhdi
	beq.s .xi7
	moveq #-15,D0            // unknown device
	rts
.xi7:
	move.l old_xhdi,-(SP)
	rts
.xi3:
	tst.l 14(A6)
	beq.s .xi8
	move.l 14(A6),A0
	move.l #512,(A0)
.xi8:
	tst.l 18(A6)
	beq.s .xi9
	move.l 18(A6),A0	 // flags
	move.l #0x00000002,(A0)  // removable
.xi9:
	move.l 22(A6),D1         // product_name
	beq.s .xi6		 // no pointer
	move.l D1,A0

	and.b #PUN_DEV,D0
	move.l product_name,A1
	move.l (A1,D0.l*4),D0    // Galvez: D0 should be the bios drive number			
	beq.s .xi6               // no pointer
	move.l D0,A1
.xi5:
	move.b (A1)+,(A0)+
	beq.s .xi4
	subq.w #1,D2
	bpl.s .xi5		// Galvez: test
.xi4:
	clr.b -1(A0)
.xi6:
	moveq #0,D0
	rts

XHReserve:
XHLock:
XHStop:
XHEject:

	tst.l old_xhdi
	beq.s .xnu1
	moveq #0,D0
	rts
.xnu1:
	move.l old_xhdi,-(SP)
	rts
	
XHDrvMap:
	tst.l old_xhdi
	beq.s .xm1
	move.l my_drvbits,d0
	rts
.xm1:
	jsr old_xhdi
	or.l my_drvbits,d0
	rts

XHInqDev:
#ifdef DEBUG_BIOS_LAYER
	lea debug134(PC),A0
	jsr display_string
#endif
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 10(A6),D0         // bios_device
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .xd2
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .xd2
	moveq #0,D1
	move.b pinfo_pun(A0,D0.l),D1
	bpl.s .xd1
.xd2:	

	tst.l old_xhdi
	beq.s .xd3
	moveq #-46,D0            // invalid drive number
		
	rts
.xd3:
	move.l old_xhdi,-(SP)
	rts
.xd1:	
	move.l D1,D2
	and.l #PUN_USB+PUN_DEV,D2
	beq.s .xd2
	tst.l 12(A6)
	beq.s .xd4
	move.l 12(A6),A1         // major
	move.w D1,(A1)
.xd4:
	tst.l 16(A6)
	beq.s .xd5
	move.l 16(A6),A1         // minor
	clr.w (A1)
.xd5:
	tst.l 20(A6)
	beq.s .xd6
	move.l pinfo_pstart(A0,D0.l*4),D1
	move.l 20(A6),A1         // start_sector
	move.l D1,(A1)
.xd6:
	lea pinfo_bpb(A0),A0
	asl.l #5,D0              // * 32
	add.l A0,D0
	tst.l 24(A6)
	beq.s .xd7
	move.l 24(A6),A1         // bpb
	move.l D0,(A1)
.xd7:
	moveq #0,D0
	rts

XHInqDriver:
#ifdef DEBUG_BIOS_LAYER
	lea debug135(PC),A0
	jsr display_string
#endif	
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 10(A6),D0         // bios_device
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .xdr2
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .xdr2
	moveq #0,D1
	move.b pinfo_pun(A0,D0.l),D1
	bpl.s .xdr1
.xdr2:	
	tst.l old_xhdi
	beq.s .xdr8
	moveq #-46,D0            // invalid drive number
	rts
.xdr8:
	move.l old_xhdi,-(SP)
	rts
.xdr1:
	move.l D1,D0
	and.l #PUN_USB,D0
	beq.s .xdr2
	move.l 12(A6),D0         // name, 17 characters
	beq.s .xdr3
	move.l D0,A1
	lea message1(PC),A0
	moveq #17,D1
.xdr6:
	move.b (A0)+,D0
	beq.s .xdr7
	move.b D0,(A1)+
	subq.l #1,D1
	bpl.s .xdr6
.xdr7:
	clr.b (A1)
.xdr3:
	move.l 16(A6),D0         // version, 7 characters
	beq.s .xdr4
	move.l D0,A1
	move.b #0x34,(A1)+       // ??? TOS 4.04
	move.b #0x2E,(A1)+
	move.b #0x30,(A1)+
	move.b #0x34,(A1)+
	clr.b (A1)
.xdr4:
	move.l 20(A6),D0         // company, 17 characters
	beq.s .xdr5
	move.l D0,A1
	clr.b (A1)
.xdr5:
	move.l 24(A6),A1         // ahdi_version
	move.w pinfo_vernum(A0),(A1)
	move.l 28(A6),A1         // maxIPL
	moveq #5,D0
	move.w D0,(A1)	
	moveq #0,D0
	rts

XHReadWrite:                      // read / write physical sectors
#ifdef DEBUG_BIOS_LAYER
	lea debug139(PC),A0
	jsr display_string
#endif	

	tst.w 12(A6)             // minor
	bne.s .xr4
	moveq #0,D4
	move.w 10(A6),D4         // major
	cmp.l #PUN_USB,D4
	bcs.s .xr4
	cmp.l #PUN_USB+PUN_DEV,D4
	bls.s .xr1
.xr4:
	tst.l old_xhdi
	beq.s .xr6
	moveq #-15,D0            // unknown device
	rts
.xr6:
	move.l old_xhdi,-(SP)
	rts
.xr1:
#ifdef DEBUG_BIOS_LAYER
	lea debug132(PC),A0
	jsr display_string
	move.w 14(A6),D0         // rwflag
	jsr hex_word
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.l 24(A6),D0         // buffer
	jsr hex_long
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.w 20(A6),D0         // num sectors
	jsr hex_word
	moveq #0x20,D0
	jsr display_char
	moveq #0x30,D0
	jsr display_char
	moveq #0x78,D0
	jsr display_char
	move.l 16(A6),D0         // logical sector
	jsr hex_long
	moveq #13,D0
	jsr display_char
	moveq #10,D0
	jsr display_char	
#endif
	move.l 22(A6),A0         // buffer
	moveq #0,D3
	move.w 20(A6),D3         // count
	beq .xr2                 // no sectors
        move.l 16(A6),D2         // start sector
	btst #0,15(A6)           // rwflag
	beq.s .xr7               // read
	// write
	move.l A0,-(SP)          // buffer
	move.l D3,-(SP)          // blkcnt	
	move.l D2,-(SP)          // blknr
	move.l D4,D0             // major
	and.l #PUN_DEV,D0
	move.l pun_ptr,A0	
	move.b pinfo_devnum(A0,D0.l),D0 // devnum in the USB bus
	move.l D0,-(SP)          // USB devnum
	jsr _usb_stor_write
	bra.s .xr5
.xr2:
	moveq #-1,D0             // error
	bra.s .xr3
.xr7:
	move.l A0,-(SP)          // buffer
	move.l D3,-(SP)          // blkcnt	
	move.l D2,-(SP)          // blknr
	move.l D4,D0             // major
	and.l #PUN_DEV,D0
	move.l pun_ptr,A0	
	move.b pinfo_devnum(A0,D0.l),D0 // devnum in the USB bus
	move.l D0,-(SP)          // USB devnum
	jsr _usb_stor_read
.xr5:
	lea 16(SP),SP
	tst.l D0
	seq.b D0
	ext.w D0
	ext.l D0
	bclr #0,D0               // OK or device not responding -2
.xr3:
 	rts

XHInqDev2:
#ifdef DEBUG_BIOS_LAYER
	lea debug136(PC),A0
	jsr display_string	// Galvez: changed bsr by jsr to avoid linker error
#endif
	move.l pun_ptr,A0
 	moveq #0,D0
	move.w 10(A6),D0         // bios_device
	cmp.l _usb_1st_disk_drive,D0
	bcs.s .xdd2
	cmp.l #MAX_LOGICAL_DRIVE,D0
	bcc.s .xdd2
	moveq #0,D1
	move.b pinfo_pun(A0,D0.l),D1
	bpl.s .xdd1
.xdd2:	
	tst.l old_xhdi
	beq.s .xdd4
	moveq #-46,D0            // invalid drive number
	rts
.xdd4:
	move.l old_xhdi,-(SP)
	rts
.xdd1:	
	move.w D1,D2
	and.b #PUN_USB+PUN_DEV,D2
	beq.s .xdd2
	tst.l 12(A6)
	beq.s .xdd5
	move.l 12(A6),A1         // major
	move.w D2,(A1)
.xdd5:
	tst.l 16(A6)
	beq.s .xdd6
	move.l 16(A6),A1         // minor
	clr.w (A1)
.xdd6:
	tst.l 20(A6)
	beq.s .xdd7
	move.l pinfo_pstart(A0,D0.l*4),D1
	move.l 20(A6),A1         // start_sector
	move.l D1,(A1)
.xdd7:
	tst.l 24(A6)
	beq.s .xdd8
	lea pinfo_bpb(A0),A2
	move.l D0,D1
	asl.l #5,D1              // * 32
	add.l A2,D1
	move.l 24(A6),A1         // bpb
	move.l D1,(A1)
.xdd8:
	tst.l 28(A6)
	beq.s .xdd9
	move.l 28(A6),A1         // blocks
	lea pinfo_psize(A0),A2
	move.l (A2,D0.l*4),(A1)
.xdd9:
	tst.l 32(A6)
	beq.s .xdd10
	move.l 32(A6),A1         // partid
	clr.l (A1)
	move.b pinfo_ptype+3(A0,D0.l*4),D1   
	move.w #0x0044,(A1)          		// NULL+ 'D'
	move.b D1,2(A1)
.xdd10:
	moveq #0,D0
	rts
 
XHDriverSpecial:
XHGetCapacity:
XHMediumChanged:
XHMiNTInfo:
XHNewCookie:
	tst.l old_xhdi
	beq.s .xn1
	moveq #-32,D0            //  invalid function number 
	rts
.xn1:
	move.l old_xhdi,-(SP)
	rts

XHDOSLimits:
	tst.l old_xhdi
	bne.s .xn1
#ifdef DEBUG_BIOS_LAYER
	lea debug137(PC),A0
	jsr display_string
#endif
	moveq #0,D0
	move.w 10(A6),D0         // which
	cmp.l #XH_DL_SECSIZ,D0   // maximal sector size (BIOS level)
	bne.s .xl1
	move.l #0x4000,D0
	rts	
.xl1:
	cmp.l #XH_DL_MINFAT,D0   // minimal number of FATs
	bne.s .xl2
	moveq #1,D0
	rts
.xl2:
	cmp.l #XH_DL_MAXFAT,D0   // maximal number of FATs
	bne.s .xl3
	moveq #2,D0
	rts
.xl3:
	cmp.l #XH_DL_MINSPC,D0   // sectors per cluster minimal
	bne.s .xl4
	moveq #2,D0
	rts
.xl4:
	cmp.l #XH_DL_MAXSPC,D0   // sectors per cluster maximal
	bne.s .xl5
#ifdef COLDFIRE
	moveq #64,D0             // for this Coldfire version of BDOS, else 2
#else
	moveq #2,D0
#endif
	rts
.xl5:
	cmp.l #XH_DL_CLUSTS,D0   // maximal number of clusters of a 16 bit FAT
	bne.s .xl6
	move.l #0x8000,D0
	rts	
.xl6:
	cmp.l #XH_DL_MAXSEC,D0   // maximal number of sectors
	bne.S .xl7
#ifdef COLDFIRE
	move.l #0x200000,D0      // for this Coldfire version of BDOS, else 0x10000
#else
	move.l #0x10000,D0
#endif
	rts
.xl7:
	cmp.l #XH_DL_DRIVES,D0   // maximal number of BIOS drives supported by the DOS
	bne.s .xl8
	moveq #16,D0
	rts
.xl8:
	moveq #-32,D0            // invalid function number 
	rts


message1:	.ascii "TOS4.04 "
message2:	.asciz "USB  "
message2b:	.asciz ", disk installed in "


crlf:	.byte 13,10,0
error:	.asciz "No ram-disk installed, "
error1:	.ascii "no enough radeon memory"
	.byte 13,10,0
error2:	.ascii "all drives already used"
	.byte 13,10,0
error3:	.ascii "error disk name"
	.byte 13,10,0
error4:	.ascii "partition type not supported"
	.byte 13,10,0
blue:	.byte 0x1B,0x62,0x34,0
black:	.byte 0x1B,0x62,0x3F,0

	.align 2

	.lcomm pun_ptr_usb,4
	.lcomm my_drvbits,4
	.lcomm _usb_1st_disk_drive,4
	.lcomm old_hdv_bpb_usb,4
	.lcomm old_hdv_rw_usb,4
	.lcomm old_hdv_mediach_usb,4
	.lcomm old_xhdi,4
	.lcomm old_xhdi_version,4
	.lcomm product_name,4*(PUN_DEV+1)


