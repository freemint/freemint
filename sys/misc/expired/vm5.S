// VM routines for MiNT
// since nobody seemed to have the guts to make them
// I made them...
// il Profesore 1996

// Later fixes by Draco, 1997.

//tablemem organisation..
//
// table_mem   ->   +---------------------------------------+
//                  |             ....                      |
// table_start ->   +---------------------------------------+
//                  |        root pointers                  |
//                  |         short form                    |
//                  |             512 bytes                 |
// table_2_start->  +---------------------------------------+
//                  |        second level descriptors       |
//                  |          short form                   |
//                  |         64*256*4 bytes                |
//                  +---------------------------------------+
// page_info_start->|         1 L ptr to page descriptor    |
//                  |         1 L ptr to next info          |
//                  +---------------------------------------+
// swap_sector ->   |          64k used by bios patcher     |
//                  |                                       |
//                  +---------------------------------------+
//

//transparent translation is disabled for the tt-ram left
//ie from 0x01000000-0x01ffffff
//transparent translation is enabled for the vme-ram left
//ie from 0xfe000000-0xfeffffff


//tt-ram is expanded by the partition length.. this scheme
//seems to be best to get maximum compatibility.

#ifdef VM_EXTENSION	

	.globl	_Init_the_VM
	.globl	Test_if_page_fault	
	.globl	VM_Installed
	.globl	Device_sector_size
	.globl	Patch_bios_rout
	.globl	VM_start
	.globl	VM_stop
	.globl	VM_size	
	.globl	_Reset_mmu

	.globl	_old_rwabs
	.globl	_quickmovb

	.data
VM_Installed		dc.w	0xff00
In_driver		dc.w	0
Nbr_of_conflicts	dc.l	0
	.data
swap_sector		ds.l	1
save_return		ds.l	5		// save the frame
Part_size		ds.l	1
Reg_image		ds.l	2
table_start		ds.l	1
table_2_start		ds.l	1
page_info_start		ds.l	1
page_unused_chain 	ds.l	1
page_used_chain		ds.l	1
page_start		ds.l	1		// ptr to corrected
st_page_start		ds.l	1
VM_start		ds.l	1		// start of VM_mem
VM_size			ds.l	1
VM_stop			ds.l	1		// stop of VM_mem
Ret_struct		ds.l	3		// bugfix for buggy gas
Dest_adr		ds.l	1
Save_CRP		ds.l	2		// area for save of mmu regs...
Save_TC			ds.l	1
Save_TT0		ds.l	1
Save_TT1		ds.l	1
Max_trans_sect		ds.w	1
Trans_sect		ds.w	1
Sector_size		ds.w	1
Nbr_of_sectors		ds.w	1
Nbr_of_pages_on_disk	ds.w	1
Nbr_of_pages		ds.w	1
Nbr_of_st_pages		ds.w	1
Device_nbr		ds.w	1
Device_sector_size	ds.w	32
Was_page_fault		ds.w	1
Swapping		ds.w	1

	.text

_Init_the_VM:

	movem.l	d2-d7/a2-a6,-(sp)
	// setup ptr

	clr.b	Swapping
	move.l	48(sp),a0
	lea	15(a0),a0
	move.l	a0,d0
	and.w	#-16,d0
	move.l	d0,a6
	move.l	a6,table_start
	lea	4*128(a6),a0
	move.l	a0,table_2_start
	add.l	#64*256*4,a0
	move.l	a0,page_info_start

	// calc nbr of pages..

	move.l	52(sp),d0
	add.l	#4*1024-1,d0
	and.w	#-4*1024,d0		// align memory to even pages
	move.l	d0,page_start

	sub.l	52(sp),d0
	neg.l	d0
	add.l	56(sp),d0
	divu	#4*1024,d0
	move.w	d0,Nbr_of_pages		// nbr of pages

	move.l	60(sp),d0
	add.l	#4*1024-1,d0
	and.w	#-4*1024,d0		// align memory to even pages
	move.l	d0,st_page_start

	sub.l	60(sp),d0
	neg.l	d0
	add.l	64(sp),d0
	divu	#4*1024,d0		
	move.w	d0,Nbr_of_st_pages	// nbr of pages

// find the partition for VM

// This was intended to begin bootsector reading for each drive
// in physical mode, but now it is unnecessary since all disk accesses
// are now done in logical mode. I left it however cause it prevents
// the VM to run on an old hddriver... a5 is nb. used later to remove
// the found device from the puninfo structure. (Draco)

	move.l	(0x516).w,a5		// ptr to puninfo
	lea	0x52(a5),a1
	cmp.l	#0x41484449,(a1)	// check cookie //AHDI
	bne	no_part
	cmp.l	0x56(a5),a1		// check cookie ptr
	bne	no_part
	cmp.w	#0x300,0x5a(a5)		// check version
	bmi	no_part

// Start from dev 2 and read boot sectors subsequently searching the
// swappartition. Nb. this initialization is much faster now, at least
// on my Falcon... (Draco)

	moveq.l	#0x02,d7
dev_loop:
	move.w	d7,Device_nbr

// logical mode instead of physical proves it will start with a hddriver
// other than AHDI (Draco).

	clr.l	-(sp)			// start sector (logical)
	move.w	d7,-(sp)		// devno
	move.w	#-1,-(sp)		// use lrecno
	move.w	#1,-(sp)		// one sector
	move.l	a6,-(sp)		// tablestart is fine..
	clr.w	-(sp)
	move.w	#4,-(sp)		// Rwabs()
	trap	#13
	lea	18(sp),sp		

	tst.l	d0
	bmi	skip_dev
	lea	0x20(a6),a0
	cmp.l	#0x54686973,(a0)+
	bne	skip_dev
	cmp.l	#0x69732061,(a0)+
	bne	skip_dev
	cmp.l	#0x204D694E,(a0)+
	bne	skip_dev
	cmp.l	#0x5420564D,(a0)+
	bne	skip_dev
	cmp.l	#0x20737761,(a0)+
	bne	skip_dev
	cmp.l	#0x70706172,(a0)+
	bne	skip_dev
	cmp.l	#0x74697469,(a0)+
	bne	skip_dev
	cmp.l	#0x6F6E2E20,(a0)+
	bne	skip_dev
	cmp.l	#0x446F204E,(a0)+
	bne	skip_dev
	cmp.l	#0x4F542074,(a0)+
	bne	skip_dev
	cmp.l	#0x6F756368,(a0)+
	bne	skip_dev
	cmp.l	#0x21212121,(a0)+
	bne	skip_dev
	cmp.l	#0x5741524E,(a0)+
	bne	skip_dev
	cmp.l	#0x494E473A,(a0)+
	bne	skip_dev
	cmp.l	#0x54686973,(a0)+
	bne	skip_dev
	cmp.l	#0x20706172,(a0)+
	bne	skip_dev
	cmp.l	#0x74697469,(a0)+
	bne	skip_dev
	cmp.l	#0x6F6E2077,(a0)+
	bne	skip_dev
	cmp.l	#0x696C6C20,(a0)+
	bne	skip_dev
	cmp.l	#0x6265206D,(a0)+
	bne	skip_dev
	cmp.l	#0x6F646966,(a0)+
	bne	skip_dev
	cmp.l	#0x69656420,(a0)+
	bne.s	skip_dev
	cmp.l	#0x62792074,(a0)+
	bne.s	skip_dev
	cmp.l	#0x68652073,(a0)+
	bne.s	skip_dev
	cmp.l	#0x77617070,(a0)+
	bne.s	skip_dev
	cmp.l	#0x65722077,(a0)+
	bne.s	skip_dev
	cmp.l	#0x68656E20,(a0)+
	bne.s	skip_dev
	cmp.l	#0x564D2069,(a0)+
	bne.s	skip_dev
	cmp.l	#0x7320656E,(a0)+
	bne.s	skip_dev
	cmp.l	#0x61626C65,(a0)+
	bne.s	skip_dev
	cmp.l	#0x64212121,(a0)+
	bne.s	skip_dev
	cmp.l	#0x20202020,(a0)+
	bne.s	skip_dev
	cmp.l	#0x696C2050,(a0)+
	bne.s	skip_dev
	cmp.l	#0x726F6665,(a0)+
	bne.s	skip_dev
	cmp.l	#0x736F7265,(a0)+
	bne.s	skip_dev
	cmp.l	#0x20313939,(a0)+
	bne.s	skip_dev
	cmp.l	#0x36202020,(a0)+
	beq.s	found
skip_dev:
	addq.w	#0x01,d7
	cmp.w	#0x0f,d7		// only check up to 14 devices
	bmi	dev_loop
no_part:
	st	VM_Installed
	clr.l	VM_size
	bra	return
found:					// d7 holds the device number

// its a vm device
// remove it from bios..

	move.l	(0x4c2).w,d0
	bclr	d7,d0
	move.l	d0,(0x4c2).w		// now bios wont recognise it hopefully..

	bset	#7,2(a5,d7.w)		// set bit in puninfo so that 
					// bios believe it doesn't use it..

// take the whole drive
// assume 512 byte sectorsize and
// use all the disc except the boot

	move.w	0x13(a6),d1		// same as before, but shorter (Draco)
	rol.w	#0x08,d1
	move.w	0x0b(a6),d2
	rol.w	#0x08,d2		// get partition size from the bootsector

	mulu.w	d2,d1
	lsr.l	#8,d1
	lsr.l	#1,d1	
	subq.l	#1,d1
	ble.s	no_part			// partition too small
	
	move.l	d1,d2
	move.l	d1,Part_size
	subq.l	#7,Part_size
	bmi.s	no_part			// partition too small
	
	lsr.l	#3,d2			// /(4096/512)=/8
	beq.s	no_part			// partition too small
	
	cmp.l	#64*1024*1024/4/1024,d2
	bmi.s	ok
	move.l	#64*1024*1024/4/1024,d2
ok:
	move.w	d2,Nbr_of_pages_on_disk

	swap	d2			// the same as before,
	lsr.l	#4,d2			// but shorter... (Draco)
	move.l	d2,VM_size

	// the swap is cleared set up ptrs...

	move.l	table_start,a0
	lea	16*4(a0),a1
	lea	7*16*4(a0),a3

	moveq.l	#13,d1
	moveq.l	#0x01,d0
setup_st_tabs:
	move.l	d0,(a0)+
	move.l	d0,(a3)+
	add.l	#0x00100000,d0
	dbf	d1,setup_st_tabs

	// enable rom image...
	move.l	#0x00e00001,d0
	move.l	d0,(a0)+
	move.l	d0,(a3)+

	// enable hw-reg image..
	move.l	#0x00f00041,d0		// was 0x00f00041
	move.l	d0,(a0)+
	move.l	d0,(a3)+

	// set dummy pages
	move.w	#6*16-1,d1		// fill everything but
					// st-ram and hw-regs with 
					// "bus error" descr..
	move.l	#0x80000005,d0
end_loop:
	move.l	d0,(a0)+
	dbf	d1,end_loop

	// now fill up to TT-ram max with tt-ram..
	move.l	#0x1000000,d0		// start to 0x1000000
	move.l	#0x1000000,d1		// top
	cmp.l	#0x1357bd13,(0x5a8).w	// check ram valid..
	bne.s	tt_not_installed

// This additional check is needed for Falcon TOS, otherwise
// we'll get a bus error from kmalloc() each cold reboot (Draco).

	tst.l	(0x5a4).w
	beq.s	tt_not_installed
	move.l	(0x5a4).w,d1
	and.l	#-256*4*1024,d1		// mask off to megs..		
	// if we got more than 32 meg tt-ram then this VM scheme doesn't work..
	cmp.l	#48*1024*1024+1,d1
	bpl	no_part
tt_not_installed:	
	move.l	a1,a0
	// make	tt-mirror...
fix_tt_loop:
	cmp.l	d0,d1
	beq.s	tt_done
	move.l	d0,(a0)
	addq.l	#1,(a0)+		// set early term
	add.l	#256*4*1024,d0		// add one meg
	bra.s	fix_tt_loop
tt_done:
	move.l	d1,VM_start
	// now make the VM tabs
	
	lea	2(a3),a3
	moveq.l	#4*16-1,d1
vm_loop:
	move.l	a3,(a0)+
	lea	256*4(a3),a3
	dbf	d1,vm_loop		

	move.l	table_2_start,a0
	move.l	page_info_start,a1
	move.l	page_start,a2
	addq.w	#1,a2
	move.w	#64*256-1,d1
	move.w	Nbr_of_pages,d0
	move.w	Nbr_of_pages_on_disk,d2
	sub.w	d2,d1
	sub.w	d0,d2
	subq.w	#1,d2
	subq.w	#1,d0
	bmi.s	skip_tt

setup_tables:
	move.l	a0,(a1)+
	move.l	a2,(a0)+
	addq.w	#4,a1
	lea	4*1024(a2),a2
	move.l	a1,-4(a1)
	dbf	d0,setup_tables

skip_tt:
	// setup st pages 
	move.w	Nbr_of_st_pages,d0
	sub.w	d0,d2
	subq.w	#1,d0
	bmi.s	skip_st_pages
	move.l	st_page_start,a2
	addq.w	#1,a2
setup_tables2:
	move.l	a0,(a1)+
	move.l	a2,(a0)+
	addq.w	#4,a1
	lea	4*1024(a2),a2
	move.l	a1,-4(a1)
	dbf	d0,setup_tables2

skip_st_pages:
	// terminate chain
	clr.l	-4(a1)

	move.l	a1,swap_sector
	
	// set invalid pages
invalid_loop:
	clr.l	(a0)+
	dbf	d2,invalid_loop

	// set error pages
	tst.w	d1
	bmi.s	error_ok
	move.l	#0x80000005,d0
error_loop:
	move.l	d0,(a0)+
	dbf	d1,error_loop
error_ok:

	// setup device sector sizes
	lea	Device_sector_size,a6
	move.l	#0x2000200,(a6)+	// a and b have 512 bytes sectors
	moveq.l	#2,d7
loopa:
	move.w	d7,-(sp)
	move.w	#7,-(sp)
	trap	#13
	addq.w	#4,sp
	tst.l	d0
	beq.s	not_used
	move.l	d0,a0
	move.w	(a0),d0
	move.w	d0,(a6)+
	bra.s	skip2
not_used:
	clr.w	(a6)+
skip2:	addq.w	#1,d7
	cmp.w	#32,d7
	bne.s	loopa

	move.l	page_info_start,page_unused_chain
					// setup page chain
	
	// disable all interupts 
	move.w	sr,-(sp)
	move.w	#0x2700,sr

	pmove	crp,Save_CRP
	pmove	tc,Save_TC

	// pmove tt0,Save_TT0
	dc.l	0xf0390a00,Save_TT0
	// pmove tt1,Save_TT1
	dc.l	0xf0390e00,Save_TT1

	move.l	#0x807e8507,Reg_image	// make sure we have transparent VME
	// pmove Reg_image,tt1
	dc.l	0xf0390c00,Reg_image
	clr.l	Reg_image		// make sure we have disabled transparent TT-ram
	// pmove Reg_image,tt0
	dc.l	0xf0390800,Reg_image

	// the table is set up set root ptrs
	// set CRP
	move.l	#0x80000002,Reg_image
	move.l	table_start,Reg_image+4
	pmove	Reg_image,crp

	// set TC will flush atc
	move.l	#0x80c57800,Reg_image
	pmove	Reg_image,tc
	
	// melt flush and enable write allocate
	movec	cacr,d1
	or.w	#0x2808,d1		// don't change burst status
	and.w	#0x3dfd,d1
	movec	d1,cacr
	move.w	(sp)+,sr

	clr.w	VM_Installed
	move.l	VM_size,d0
	add.l	VM_start,d0
	move.l	d0,VM_stop
	move.l	d0,(0x5a4).w		// set new TT-ram top..
	move.l	#0x1357bd13,(0x5a8).w	// even falcons get some TT-ram!
	// ok return
return:
	movem.l	(sp)+,d2-d7/a2-a6
	move.l	VM_start,Ret_struct
	move.l	VM_size,Ret_struct+4
	move.l	VM_stop,Ret_struct+8
	move.l	#Ret_struct,d0		// send a struct back...
	rts

_Reset_mmu:
	// restore mmu regs..
	move.w	sr,-(sp)
	move.w	#0x2700,sr		// disable interrupts
	pmove	Save_CRP,crp
	pmove	Save_TC,tc
	// pmove Save_TT0,tt0
	dc.l	0xf0390800,Save_TT0
	// pmove Save_TT1,tt1
	dc.l	0xf0390c00,Save_TT1
	move.w	(sp)+,sr

	clr.l	(0x5a4).w
	clr.l	(0x5a8).w
	move.l	VM_start,d0
	cmp.l	#0x1000000,d0
	beq.s	no_TT_ram
	move.l	d0,(0x5a4).w		// reset TT-ram top
	move.l	#0x1357bd13,(0x5a8).w	// set magic..
no_TT_ram:
	rts

Test_if_page_fault:
	tst.w	VM_Installed
	bne	t_return
	tst.w	Swapping
	bne	t_return
	st	Swapping
	movem.l	d0-d7/a0-a6,-(sp)
	// check if invalid desc

	move.l	4+15*4(sp),a0
	move.l	0x10(a0),a0		// a0 is fault adr

	// ptestr #0,(a0),#7,a4		// the same tree is used for user and supervisor code
	dc.l	0xf0109f90
	// pmove psr,Reg_image
	dc.l	0xf0396200,Reg_image
	st	Was_page_fault
	btst	#10-8,Reg_image
	// if not invalid buserror..
	beq	go_on
	// check if its a valid page...
	move.l	a0,d0
	cmp.l	VM_start,d0
	bmi	go_on			// under VM_start
	cmp.l	VM_stop,d0
	bpl	go_on			// over VM_stop

	// check page desc
	move.l	(a4),d0
	bmi	go_on			// not valid page..

	// got a page fault
	clr.b	Was_page_fault
	// run the list

	// move used pages to the end of the list by
	// building up a used list and then appending
	// the used list to the unused...
	// this makes a psudo LRU and this scheme is
	// used by most VM systems..

	lea	page_used_chain,a0
	lea	page_unused_chain,a2	// a2 point to current link
run_loop:
	move.l	a2,a1			// a1 pointes to old link
run_loop2:
	move.l	(a2),d0			// get next
	beq.s	done
	move.l	d0,a2
	move.l	(a2)+,a3		// get descr should be faster than using 020 adr modes..
	bclr	#3,3(a3)		// test u bit in desc
	beq.s	run_loop		// not used..
	move.l	d0,(a0)			// insert into used list..
	move.l	a2,a0			// make the current link the last in used chain
	move.l	(a2),(a1)		// make sure the current link is excluded from the list
	bra.s	run_loop2		// a1 is still the old link...
done:
	// clr.l (a0) 			// make sure the used list is terminated
	move.l	page_used_chain,(a1)	// append list. must be appended to the old link!
	// get the first from list
	move.l	page_unused_chain,a6
	move.l	(a6),a5			// the dscriptor is used to get page number

	move.l	a6,(a0)
	move.l	4(a6),page_unused_chain
	clr.l	4(a6)

	// calculate page adr in mem
	move.l	(a5),d4
	and.w	#-4*1024,d4		// get even page adr

	btst	#4,3(a5)		// test m bit
	beq.s	not_modified
	// save on harddisc if modified
	// calculate sector number
	move.l	a5,d0
	sub.l	table_2_start,d0	// get page number
	and.w	#-4,d0
	add.l	d0,d0			// shift up by 1 get sector number
//	smi	Was_page_fault
//	bmi	go_on
	cmp.l	Part_size,d0
	spl	Was_page_fault
	bpl	go_on
	addq.l	#0x01,d0		// add base (skip boot sector)

	// save page on harddisc
	
	move.l	d0,-(sp)
	move.w	Device_nbr,-(sp)
	move.w	#-1,-(sp)
	move.w	#8,-(sp)		// 512 bytes sectors means 8 sectors pro page
	move.l	d4,-(sp)
	move.w	#0x01,-(sp)		// logical mode (Draco)
	move.w	#4,-(sp)
	trap	#13
	lea	18(sp),sp

	// if something wrong with save then fake bus error..
	tst.l	d0
	smi	Was_page_fault
	bmi.s	mtree

not_modified:
	// load the demanded page
	// calculate sector number
	move.l	a4,d0
	sub.l	table_2_start,d0	// get page number
	and.w	#-4,d0
	add.l	d0,d0			// shift up by 1 get sector number
//	smi	Was_page_fault
//	bmi	go_on
	cmp.l	Part_size,d0
	spl	Was_page_fault
	bpl.s	go_on
	addq.l	#0x01,d0		// add base (skip boot sector)

	move.l	d0,-(sp)
	move.w	Device_nbr,-(sp)
	move.w	#-1,-(sp)
	move.w	#8,-(sp)
	move.l	d4,-(sp)
	clr.w	-(sp)			// logical mode (Draco)			
	move.w	#4,-(sp)
	trap	#13
	lea	18(sp),sp

	// if something wrong with load then fake bus error..
	tst.l	d0
	smi	Was_page_fault

	// modify tree..
mtree	move.l	(a5),(a4)		// copy page desc
	move.b	#1,3(a4)		// set it to page desc
	move.l	a4,(a6)			// setup link
	clr.b	3(a5)			// make the other page invalid...
	move.l	a5,d0
	sub.l	table_2_start,d0	// get page number
	lsl.l	#8,d0			
	lsl.l	#2,d0			// mul by 4096

// Was:	add.l	#VM_start,d0
// but I don't think it was correct... (Draco)

	add.l	VM_start,d0		// add start
	move.l	d0,a0
	// pflush #0,#7,(a0)	// flush old
	dc.l	0xf01038f0

	move.l	4+15*4(sp),a1
	move.l	0x10(a1),a0		// a0 is fault adr
	// test if read or write ssw bit 6 is =1 if read
	btst	#6,0xa+1(a1)
	beq.s	its_a_write
	// ploadr #0,(a0)
	dc.l	0xf0102210
	bra.s	go_on
its_a_write:
	// ploadw	#0,(a0)
	dc.l	0xf0102010
go_on:
	// melt and flush cache
	movec	cacr,d1
	or.w	#0x2808,d1		// don't change burst status
	and.w	#0x3dfd,d1
	movec	d1,cacr
	movem.l	(sp)+,d0-d7/a0-a6
	clr.w	Swapping
	tst.b	Was_page_fault
t_return:
	rts

Patch_bios_rout:
	// just modifiy the stackframe
	// and get one sector at the time
	// and copy it to the VM
	
	// test if physical
	btst	#3,2+2+1(sp)
	bne.s	physical
	// get sectorsize on drive..
	move.w	2+12(sp),d0
	add.w	d0,d0
	move.w	d0,a0
	add.l	#Device_sector_size,a0
	move.w	(a0),Sector_size
	bra.s	patch_skip
physical:
	// physical mode that means 512 bytes sectors
	move.w	#512,Sector_size
	bra.s	sect_ok
sect_error:
	moveq.l	#-1,d0	//unspec error
	rts
patch_skip:
sect_ok:
	cmp.w	#512,Sector_size
	bmi.s	sect_error
	// save parameters
	move.l	2+4(sp),Dest_adr
	move.w	2+8(sp),Nbr_of_sectors
	bne.s	nbr_ok
	moveq.l	#0,d0			// this is actually an
					// error config but
					// AHDI permits it..
	rts
nbr_ok:	
	// calculate how many sectors that can be transfered..

	move.l	#64*1024,d0
	divu	Sector_size,d0
	// d0 holds nbr of sectors to be transfered..
	move.w	d0,Max_trans_sect

	// now modify
	move.l	(sp)+,save_return
	movem.l	(sp),d0-d1/a0-a1
	movem.l	d0-d1/a0-a1,save_return+4
	btst	#0,-2+2+1(sp)
	bne	write
sect_loop:
	movem.l	save_return+4,d0-d1/a0-a1
	movem.l	d0-d1/a0-a1,(sp)	// reset parameters	

	move.w	Max_trans_sect,d0
	cmp.w	Nbr_of_sectors,d0
	bmi.s	ok_sect1
	move.w	Nbr_of_sectors,d0
ok_sect1:
	move.w	d0,Trans_sect

	move.l	swap_sector,-2+4(sp)	// new dest adr..
	move.w	d0,-2+8(sp)		// get the sectors
	move.l	_old_rwabs,a0
	jsr	(a0)			// jump to driver
	tst.l	d0
	bne	patch_done		// found a error

	// copy sector
	move.w	Sector_size,d0		// get sector size
	mulu	Trans_sect,d0
	move.l	d0,-(sp)
	move.l	swap_sector,-(sp)
	move.l	Dest_adr,-(sp)
	add.l	d0,Dest_adr
	jsr	_quickmovb
	lea	12(sp),sp
	
	// fix start sect
	moveq.l	#0,d0
	move.w	Trans_sect,d0
	cmp.w	#-1,-2+10+save_return+4
	beq.s	use_lrecno
	add.w	d0,-2+10+save_return+4
	bra.s	skip_lrecno	
use_lrecno:
	add.l	d0,-2+14+save_return+4
skip_lrecno:
	sub.w	d0,Nbr_of_sectors
	bne	sect_loop
	bra	patch_return
write:
	movem.l	save_return+4,d0-d1/a0-a1
	movem.l	d0-d1/a0-a1,(sp)	// reset parameters	

	move.w	Max_trans_sect,d0
	cmp.w	Nbr_of_sectors,d0
	bmi.s	ok_sect2
	move.w	Nbr_of_sectors,d0
ok_sect2:
	move.w	d0,Trans_sect

	// copy sector
	mulu.w	Sector_size,d0		// get sector size
	move.l	d0,-(sp)
	move.l	Dest_adr,-(sp)
	add.l	d0,Dest_adr
	move.l	swap_sector,-(sp)
	jsr	_quickmovb
	lea	12(sp),sp

	move.l	swap_sector,-2+4(sp)	// new dest adr..
	move.w	Trans_sect,-2+8(sp)	// save the sectors
	move.l	_old_rwabs,a0
	jsr	(a0)			// jump to driver
	tst.l	d0
	bne.s	patch_done		// found a error

	// fix start sect
	moveq.l	#0,d0
	move.w	Trans_sect,d0
	cmp.w	#-1,-2+10+save_return+4
	beq.s	w_use_lrecno
	add.w	d0,-2+10+save_return+4
	bra.s	w_skip_lrecno	
w_use_lrecno:
	add.l	d0,-2+14+save_return+4
w_skip_lrecno:
	sub.w	d0,Nbr_of_sectors
	bne	write

patch_return:
	movem.l	save_return+4,d0-d1/a0-a1
	movem.l	d0-d1/a0-a1,(sp)	// reset parameters	
	moveq.l	#0,d0			// everything is ok...
patch_done:
	move.l	save_return,-(sp)
	moveq.l	#0,d1
	move.l	d1,a0
	move.l	d1,a1
	rts
#endif
