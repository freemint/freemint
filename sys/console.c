/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/* MiNT routines for doing console I/O */

# include "console.h"

# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/proc.h"

# include "init.h"
# include "dosfile.h"
# include "filesys.h"
# include "tty.h"
# include "proc.h"
# ifdef WITH_NATIVE_FEATURES
# include "arch/native_features.h"
# endif

/*
 * These routines are what Cconout, Cauxout, etc. ultimately call.
 * They take an integer argument which is the user's file handle,
 * and do the translation to a file ptr (and return an appropriate error
 * message if necessary.
 * "mode" may be RAW, COOKED, ECHO, or COOKED|ECHO.
 * note that the user may not call them directly!
 */

long
file_instat (FILEPTR *f)
{
	long r;

	if (!f)
		return EBADF;

	/* default is to assume input waiting (e.g. TOS files) */
	r = 1;

	if (is_terminal (f))
		tty_ioctl (f, FIONREAD, &r);
	else
		xdd_ioctl (f, FIONREAD, &r);

	return r;
}

long
file_outstat (FILEPTR *f)
{
	long r;

	if (!f)
		return EBADF;

	/* default is to assume output OK (e.g. TOS files) */
	r = 1;

	if (is_terminal (f))
		tty_ioctl (f, FIONWRITE, &r);
	else
		xdd_ioctl (f, FIONWRITE, &r);

	return r;
}

long
file_getchar (FILEPTR *f, int mode)
{
	char c;
	long r;

	if (!f)
		return EBADF;

	if (is_terminal (f))
		return tty_getchar (f, mode);

	r = xdd_read (f, &c, 1L);
	if (r < 0) {
		return r;
	}

	return (r == 1) ? (((long) c) & 0xff) : MiNTEOF;
}

long
file_putchar (FILEPTR *f, long c, int mode)
{
	if (!f)
		return EBADF;

	if (is_terminal (f))
		return tty_putchar (f, c & 0x7fffffffL, mode);

	{
		char ch;

		ch = c & 0x00ff;
		return xdd_write (f, &ch, 1L);
	}
}


/*
 * OK, here are the GEMDOS console I/O routines
 */

long _cdecl
sys_c_conin (void)
{
	return file_getchar (get_curproc()->p_fd->ofiles[0], COOKED | ECHO);
}

long _cdecl
sys_c_conout (int c)
{
	return file_putchar (get_curproc()->p_fd->ofiles[1], (long) c, COOKED);
}

long _cdecl
sys_c_auxin (void)
{
	return file_getchar (get_curproc()->p_fd->ofiles[2], RAW);
}

long _cdecl
sys_c_auxout (int c)
{
	return file_putchar (get_curproc()->p_fd->ofiles[2], (long) c, RAW);
}

long _cdecl
sys_c_prnout (int c)
{
	return file_putchar (get_curproc()->p_fd->ofiles[3], (long) c, RAW);
}

long _cdecl
sys_c_rawio (int c)
{
	PROC *p = get_curproc();

	if (c == 0x00ff)
	{
		long r;

		if (!file_instat (p->p_fd->ofiles[0]))
			return 0;

		r = file_getchar (p->p_fd->ofiles[0], RAW);
		if (r < E_OK)
			return 0;

		return r;
	}
	else
		return file_putchar (p->p_fd->ofiles[1], (long) c, RAW);
}

long _cdecl
sys_c_rawcin (void)
{
	return file_getchar (get_curproc()->p_fd->ofiles[0], RAW);
}

long _cdecl
sys_c_necin (void)
{
	return file_getchar (get_curproc()->p_fd->ofiles[0], COOKED | NOECHO);
}

#if __GNUC_PREREQ(12,0)
/* avoid a library call to strlen which we don't have */
#pragma GCC push_options
#pragma GCC optimize "-O1"
#endif
long _cdecl
sys_c_conws (const char *str)
{
	const char *p = str;
	long cnt = 0;

	while (*p++)
		cnt++;

# ifdef WITH_NATIVE_FEATURES
	if( write_boot_file == 2 )
		nf_debug(str);
# endif
	return sys_f_write (1, cnt, str);
}
#if __GNUC_PREREQ(12,0)
#pragma GCC pop_options
#endif

long _cdecl
sys_c_conrs (char *buf)
{
	long size, count, r;

	size = ((long) *buf) & 0xff;
	if (is_terminal (get_curproc()->p_fd->ofiles[0]))
	{
		/* TB: When reading from a terminal, f_read is used which
		 * automatically breaks at the first newline
		 */
		r = sys_f_read (0, size, buf + 2);
		if (r < E_OK)
		{
			buf [1] = 0;
			return r;
		}

		buf [1] = (char) r;
		return E_OK;
	}

	/* TB: In all other cases, we must read character by character, breaking
	 * at a newline or carriage return, or when the maximum number of
	 * characters has been read
	 */
	for (count = 0L; ; count++)
	{
		char dummy;

		if (count == size)
		{
			buf [1] = (char) count;
			buf [count + 2] = 0;
			return E_OK;
		}
		r = sys_c_conin ();

		/* Also break on error... */
		if (r < E_OK)
		{
			buf [count + 2] = 0;
			buf [1] = (char) count;
			return r;
		}

		dummy = (char) r;

		/* ... or at EOF */
		if ((short) r == MiNTEOF)
		{
			buf [count + 2] = 0;
			buf [1] = (char) count;
			return E_OK;
		}

		if ((dummy == '\r') || (dummy == '\n'))
		{
			buf [count + 2] = 0;
			buf [1] = (char) count;

			/* When we've read a carriage return, the next char
			 * has to be skipped, because it usually is a
			 * linefeed (GEMDOS-style lines end with CR/LF).
			 * Problems may occur with files that have lines
			 * ending with CR only, but I have not seen many yet.
			 */
			 if (dummy == '\r')
			 	return sys_f_read (0, 1L, &dummy);
			 else
				return E_OK;
		}

		buf [count + 2] = dummy;
	}
}

long _cdecl
sys_c_conis (void)
{
	return -(!!file_instat (get_curproc()->p_fd->ofiles[0]));
}

long _cdecl
sys_c_conos (void)
{
	return -(!!file_outstat (get_curproc()->p_fd->ofiles[1]));
}

long _cdecl
sys_c_prnos (void)
{
	return -(!!file_outstat (get_curproc()->p_fd->ofiles[3]));
}

long _cdecl
sys_c_auxis (void)
{
	return -(!!file_instat (get_curproc()->p_fd->ofiles[2]));
}

long _cdecl
sys_c_auxos (void)
{
	return -(!!file_outstat (get_curproc()->p_fd->ofiles[2]));
}

/* Extended GEMDOS routines
 */

long _cdecl
sys_f_instat (int fh)
{
	PROC *proc;

# if O_GLOBAL
	if (fh & 0x8000)
	{
		proc = rootproc;
		fh &= ~0x8000;
	}
	else
# endif
		proc = get_curproc();

	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Finstat: bad handle %d", fh));
		return EBADF;
	}

	return file_instat (proc->p_fd->ofiles[fh]);
}

long _cdecl
sys_f_outstat (int fh)
{
	PROC *proc;

# if O_GLOBAL
	if (fh & 0x8000)
	{
		proc = rootproc;
		fh &= ~0x8000;
	}
	else
# endif
		proc = get_curproc();

	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Foutstat: bad handle %d", fh));
		return EBADF;
	}

	return file_outstat (proc->p_fd->ofiles[fh]);
}

long _cdecl
sys_f_getchar (int fh, int mode)
{
	PROC *proc;

# if O_GLOBAL
	if (fh & 0x8000)
	{
		proc = rootproc;
		fh &= ~0x8000;
	}
	else
# endif
		proc = get_curproc();

	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Fgetchar: bad handle %d", fh));
		return EBADF;
	}

	return file_getchar (proc->p_fd->ofiles[fh], mode);
}

long _cdecl
sys_f_putchar (int fh, long c, int mode)
{
	PROC *proc;

# if O_GLOBAL
	if (fh & 0x8000)
	{
		proc = rootproc;
		fh &= ~0x8000;
	}
	else
# endif
		proc = get_curproc();

	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Fputchar: bad handle %d", fh));
		return EBADF;
	}

	return file_putchar (proc->p_fd->ofiles[fh], c, mode);
}
