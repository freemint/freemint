/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

# ifndef _m68k_tosbind_h
# define _m68k_tosbind_h

# include <mint/falcon.h>
# include <mint/mintbind.h>


/*
 * GEMDOS
 */
# define TRAP_Pterm0()				Pterm0()
# define TRAP_Cconin()				Cconin()
# define TRAP_Crawcin()				Crawcin()
# define TRAP_Cconws(s)				Cconws(s)
# define TRAP_Dsetdrv(d)			Dsetdrv(d)
# define TRAP_Dgetdrv()				Dgetdrv()
# define TRAP_Fsetdta(dta)			Fsetdta(dta)
# define TRAP_Super(ptr)			Super(ptr)
# define TRAP_Tgettime()			Tgettime()
# define TRAP_Fgetdta()				Fgetdta()
# define TRAP_Sversion()			Sversion()
# define TRAP_Dfree(buf,d)			Dfree(buf,d)
# define TRAP_Dcreate(path)			Dcreate(path)
# define TRAP_Ddelete(path)			Ddelete(path)
# define TRAP_Dsetpath(path)			Dsetpath(path)
# define TRAP_Fcreate(fn,mode)			Fcreate(fn,mode)
# define TRAP_Fopen(filename,mode)		Fopen(filename,mode)
# define TRAP_Fclose(handle)			Fclose(handle)
# define TRAP_Fread(handle,cnt,buf)		Fread(handle,cnt,buf)
# define TRAP_Fwrite(handle,cnt,buf)		Fwrite(handle,cnt,buf)
# define TRAP_Fdelete(fn)			Fdelete(fn)
# define TRAP_Fseek(where,handle,how)		Fseek(where,handle,how)
# define TRAP_Fattrib(fn,rwflag,attr)		Fattrib(fn,rwflag,attr)
# define TRAP_Dgetpath(buf,d)			Dgetpath(buf,d)
# define TRAP_Malloc(size)			Malloc(size)
# define TRAP_Mfree(ptr)			Mfree(ptr)
# define TRAP_Mshrink(ptr,size)			Mshrink(ptr,size)
# define TRAP_Pexec(mode,prog,tail,env)		Pexec(mode,prog,tail,env)
# define TRAP_Pterm(rv)				Pterm(rv)
# define TRAP_Fsfirst(filespec,attr)		Fsfirst(filespec,attr)
# define TRAP_Fsnext()				Fsnext()
# define TRAP_Frename(zero,old,new)		Frename(zero,old,new)
# define TRAP_Fdatime(timeptr,handle,rwflag)	Fdatime(timeptr,handle,rwflag)
# define TRAP_Flock(handle,mode,start,length)	Flock(handle,mode,start,length)
# define TRAP_Fcntl(f,arg,cmd)			Fcntl(f,arg,cmd)

/*
 * GEMDOS TT
 */
# define TRAP_Mxalloc(amt,flag)			Mxalloc(amt,flag)

/*
 * BIOS
 */
# define TRAP_Bconstat(dev)			Bconstat(dev)
# define TRAP_Bconin(dev)			Bconin(dev)
# define TRAP_Bconout(dev,c)			Bconout(dev,c)
# define TRAP_Setexc(vnum,vptr)			Setexc(vnum,vptr)
# define TRAP_Bcostat(dev)			Bcostat(dev)
# define TRAP_Kbshift(data)			Kbshift(data)

/*
 * XBIOS
 */
# define TRAP_Initmous(type,param,vptr)		Initmous(type,param,vptr)
# define TRAP_Physbase()			Physbase()
# define TRAP_Logbase()				Logbase()
# define TRAP_Getrez()				Getrez()
# define TRAP_Setscreen(lscrn,pscrn,rez)	Setscreen(lscrn,pscrn,rez)
# define TRAP_Iorec(ioDEV)			Iorec(ioDEV)
# define TRAP_Rsconf(baud,flow,uc,rs,ts,sc)	Rsconf(baud,flow,uc,rs,ts,sc)
# define TRAP_Keytbl(nrml,shft,caps)		Keytbl(nrml,shft,caps)
# define TRAP_Kbrate(delay,rate)		Kbrate(delay,rate)
# define TRAP_Cursconf(rate,attr)		Cursconf(rate,attr)
# define TRAP_Settime(time)			Settime(time)
# define TRAP_Gettime()				Gettime()
# define TRAP_Offgibit(ormask)			Offgibit(ormask)
# define TRAP_Ongibit(andmask)			Ongibit(andmask)
# define TRAP_Dosound(ptr)			Dosound(ptr)
# define TRAP_Kbdvbase()			Kbdvbase()
# define TRAP_Vsync()				Vsync()
# define TRAP_Supexec(funcptr)			Supexec(funcptr)
# define TRAP_Bconmap(dev)			Bconmap(dev)
# define TRAP_NVMaccess(op,start,count,buf)	NVMaccess(op,start,count,buf)

/*
 * XBIOS Falcon
 */
# define TRAP_VsetScreen(lscrn,pscrn,rez,mode)	VsetScreen(lscrn,pscrn,rez,mode)
# define TRAP_VsetMode(mode)			VsetMode(mode)
# define TRAP_VgetSize(mode)			VgetSize(mode)

# endif /* _m68k_tosbind_h */
