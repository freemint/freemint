/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Author: Jrg Westheide <joerg_westheide@su.maus.de>
 * Started: 1999-04-11
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * The only intention of this header file is to provide the possibility to
 * call some functions in the ROM (directly).
 * The trap_1_emu call creates a stack frame and then jumps to the ROM (via
 * the old_vec from the XBRA structure). This means that these calls will not
 * pass through the "beginning" of the trap chain (this is it's intention!)
 *
 */

# ifndef _m68k_syscall_h
# define _m68k_syscall_h


/* values for original system vectors */
extern long old_dos, old_bios, old_xbios, old_trap2;

long	_cdecl	mint_trap2	(void);
long	_cdecl	unused_trap	(void);
long	_cdecl	mint_bios	(void);
long	_cdecl	mint_dos	(void);
long	_cdecl	mint_timer	(void);
long	_cdecl	mint_vbl	(void);
long	_cdecl	mint_5ms	(void);
long	_cdecl	mint_xbios	(void);
long	_cdecl	new_ikbd	(void);
long	_cdecl	new_bus		(void);
long	_cdecl	new_addr	(void);
long	_cdecl	new_ill		(void);
long	_cdecl	new_divzero	(void);
long	_cdecl	new_trace	(void);
long	_cdecl	new_priv	(void);
long	_cdecl	new_linef	(void);
long	_cdecl	new_chk		(void);
long	_cdecl	new_trapv	(void);
long	_cdecl	new_fpcp	(void);
long	_cdecl	new_mmu		(void);
long	_cdecl	new_mmuconf	(void);
long	_cdecl	new_format	(void);
long	_cdecl	new_cpv		(void);
long	_cdecl	new_uninit	(void);
long	_cdecl	new_spurious	(void);
long	_cdecl	new_pmmuacc	(void);

long	_cdecl	new_criticerr	(long error);
void	_cdecl	new_exec_os (register long basepage);

extern long gdos_version;
extern long _cdecl (*aes_handler)(void *);
extern long _cdecl (*vdi_handler)(void *);

char *	_cdecl	lineA0		(void);
void	_cdecl	call_aes	(short **);
long	_cdecl	callout		(long, ...);
# ifndef callout1
long	_cdecl	callout1	(long, int);
# endif
# ifndef callout2
long	_cdecl	callout2	(long, int, int);
# endif
long	_cdecl	callout6	(long, int, int, int, int, int, int);
long	_cdecl	callout6spl7	(long, int, int, int, int, int, int);
void	_cdecl	do_usrcall	(void);

/*
 * callout original TOS trap vectors
 */
long	_cdecl	trap_1_emu(short fnum, ...);
long	_cdecl	trap_13_emu(short fnum, ...);
long	_cdecl	trap_14_emu(short fnum, ...);

# define ROM_Fsetdta(dta)			(void)	trap_1_emu(0x01a,dta)
# define ROM_Dfree(buf,d)				trap_1_emu(0x036,buf,d)
# define ROM_Dcreate(path)			(int)	trap_1_emu(0x039,path)
# define ROM_Ddelete(path)				trap_1_emu(0x03a,path)
# define ROM_Fcreate(fn,mode)				trap_1_emu(0x03c,fn,mode)
# define ROM_Fopen(filename,mode)			trap_1_emu(0x03d,filename,mode)
# define ROM_Fclose(handle)				trap_1_emu(0x03e,handle)
# define ROM_Fread(handle,cnt,buf)			trap_1_emu(0x03f,handle,cnt,buf)
# define ROM_Fwrite(handle,cnt,buf)			trap_1_emu(0x040,handle,cnt,buf)
# define ROM_Fdelete(fn)				trap_1_emu(0x041,fn)
# define ROM_Fseek(where,handle,how)			trap_1_emu(0x042,where,handle,how)
# define ROM_Fattrib(fn,rwflag,attr)		(int)	trap_1_emu(0x043,fn,rwflag,attr)
# define ROM_Fsfirst(filespec,attr)			trap_1_emu(0x04e,filespec,attr)
# define ROM_Fsnext()					trap_1_emu(0x04f)
# define ROM_Frename(zero,oldname,newname)	(int)	trap_1_emu(0x056,zero,oldname,newname)
# define ROM_Fdatime(timeptr,handle,rwflag)		trap_1_emu(0x057,timeptr,handle,rwflag)
# define ROM_Flock(handle,mode,start,length)		trap_1_emu(0x05c,handle,mode,start,length)
# define ROM_Fcntl(f,arg,cmd)				trap_1_emu(0x104,f,arg,cmd)

# define ROM_Bconstat(dev)			(short)	trap_13_emu(0x01,(short)(dev))
# define ROM_Bconin(dev)				trap_13_emu(0x02,(short)(dev))
# define ROM_Bconout(dev,c)				trap_13_emu(0x03,(short)(dev),(short)((c) & 0xff))
# define ROM_Setexc(vnum,vptr)		(void (*)(void))trap_13_emu(0x05,(short)(vnum),(long)(vptr))
# define ROM_Bcostat(dev)			(short)	trap_13_emu(0x08,(short)(dev))
# define ROM_Kbshift(data)				trap_13_emu(0x0B,(short)(data))

# define ROM_Initmous(type,param,vptr)		(void)	trap_14_emu(0x00,(short)(type),(long)(param),(long)(vptr))
# define ROM_Getrez()				(short)	trap_14_emu(0x04)
# define ROM_Iorec(ioDEV)			(void *)trap_14_emu(0x0E,(short)(ioDEV))
# define ROM_Rsconf(baud,flow,uc,rs,ts,sc)		trap_14_emu(0x0F,(short)(baud),(short)(flow),\
								    (short)(uc),(short)(rs),(short)(ts),(short)(sc))
# define ROM_Keytbl(nrml,shft,caps)		(void *)trap_14_emu(0x10,(long)(nrml),(long)(shft),(long)(caps))
# define ROM_Cursconf(rate,attr)		(short)	trap_14_emu(0x15,(short)(rate),(short)(attr))
# define ROM_Settime(time)			(void)	trap_14_emu(0x16,(long)(time))
# define ROM_Gettime()					trap_14_emu(0x17)
# define ROM_Bioskeys()					trap_14_emu(0x18)
# define ROM_Offgibit(ormask)			(void)	trap_14_emu(0x1D,(short)(ormask))
# define ROM_Ongibit(andmask)			(void)	trap_14_emu(0x1E,(short)(andmask))
# define ROM_Dosound(ptr)			(void)	trap_14_emu(0x20,(long)(ptr))
# define ROM_Kbdvbase()					trap_14_emu(0x22)
# define ROM_Vsync()				(void)	trap_14_emu(0x25)
# define ROM_Bconmap(dev)				trap_14_emu(0x2c,(short)(dev))

# define ROM_VsetScreen(lscrn,pscrn,rez,mode)	trap_14_emu(0x05,(long)(lscrn),(long)(pscrn),(short)(rez),(short)(mode))

# define ROM_Kbrate(delay, rate)		(short) trap_14_emu(0x23,(short)(delay),(short)(rate))

# endif /* _m68k_syscall_h */
