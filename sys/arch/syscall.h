/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * begin:	1999-04
 * last change: 1999-04-11
 * 
 * Author: J”rg Westheide <joerg_westheide@su.maus.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * The only intention of this header file is to provide the possibility to 
 * call some functions in the ROM (directly).
 * The trap_1_emu call creates a stack frame and then jumps to the ROM (via
 * the old_vec from the XBRA structure). This means that these calls will not
 * pass through the "beginning" of the trap chain (this is it's intention!)
 */

# ifndef _m68k_syscall_h
# define _m68k_syscall_h


/* values for original system vectors */
extern long old_dos, old_bios, old_xbios;

extern void	*pc_valid_return;

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
long	_cdecl	new_format	(void);
long	_cdecl	new_cpv		(void);
long	_cdecl	new_uninit	(void);
long	_cdecl	new_spurious	(void);
long	_cdecl	new_pmmuacc	(void);

void		sig_return	(void);

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


# ifdef SYSCALL_REENTRANT

extern long trap_1_emu(int fnum, ...);

/* Currently these bindings are only needed in biosfs.c in the rsvf_* functions
 * and in tosfs.c, the old TOS-FS.
 */

# define ROM_Fsetdta(dta)			(void)	trap_1_emu( 26, dta)
# define ROM_Dfree(buf, d)			(long)	trap_1_emu( 54, buf, d)
# define ROM_Dcreate(path)			(int)	trap_1_emu( 57, path)
# define ROM_Ddelete(path)			(long)	trap_1_emu( 58, path)
# define ROM_Fcreate(fn, mode)			(long)	trap_1_emu( 60, fn, mode)
# define ROM_Fopen(filename, mode)		(long)	trap_1_emu( 61, filename, mode)
# define ROM_Fclose(handle)			(long)	trap_1_emu( 62, handle)
# define ROM_Fread(handle, cnt, buf)		(long)	trap_1_emu( 63, handle, cnt, buf)
# define ROM_Fwrite(handle, cnt, buf)		(long)	trap_1_emu( 64, handle, cnt, buf)
# define ROM_Fdelete(fn)			(long)	trap_1_emu( 65, fn)
# define ROM_Fseek(where, handle, how)		(long)	trap_1_emu( 66, where, handle, how)
# define ROM_Fattrib(fn, rwflag, attr)		(int)	trap_1_emu( 67, fn, rwflag, attr)
# define ROM_Fsfirst(filespec, attr)		(long)	trap_1_emu( 78, filespec, attr)
# define ROM_Fsnext()				(long)	trap_1_emu( 79)
# define ROM_Frename(zero, old, new)		(int)	trap_1_emu( 86, zero, old, new)
# define ROM_Fdatime(timeptr, handle, rwflag)	(long)	trap_1_emu( 87, timeptr, handle, rwflag)
# define ROM_Flock(handle, mode, start, length)	(long)	trap_1_emu( 92, handle, mode, start, length)
# define ROM_Fcntl(f, arg, cmd)			(long)	trap_1_emu(260, f, arg, cmd)

# else

# include <mintbind.h>

# define ROM_Fsetdta(dta)			Fsetdta(dta)
# define ROM_Dfree(buf, d)			Dfree(buf, d)
# define ROM_Dcreate(path)			Dcreate(path)
# define ROM_Ddelete(path)			Ddelete(path)
# define ROM_Fcreate(fn, mode)			Fcreate(fn, mode)
# define ROM_Fopen(filename, mode)		Fopen(filename, mode)
# define ROM_Fclose(handle)			Fclose(handle)
# define ROM_Fread(handle, cnt, buf)		Fread(handle, cnt, buf)
# define ROM_Fwrite(handle, cnt, buf)		Fwrite(handle, cnt, buf)
# define ROM_Fdelete(fn)			Fdelete(fn)
# define ROM_Fseek(where, handle, how)		Fseek(where, handle, how)
# define ROM_Fattrib(fn, rwflag, attr)		Fattrib(fn, rwflag, attr)
# define ROM_Fsfirst(filespec, attr)		Fsfirst(filespec, attr)
# define ROM_Fsnext()				Fsnext()
# define ROM_Frename(zero, old, new)		Frename(zero, old, new)
# define ROM_Fdatime(timeptr, handle, rwflag)	Fdatime(timeptr, handle, rwflag)
# define ROM_Flock(handle, mode, start, length)	Flock(handle, mode, start, length)
# define ROM_Fcntl(f, arg, cmd)			Fcntl(f, arg, cmd)

# endif


# endif /* _m68k_syscall_h */
