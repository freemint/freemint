/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1993,1994 Atari Corporation.
 * All rights reserved.
 */

/* MiNT routines for doing console I/O */

# include "console.h"

# include "mint/filedesc.h"
# include "mint/proc.h"

# include "dosfile.h"
# include "filesys.h"
# include "tty.h"


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
	if (!f)
		return EBADF;
	
	if (is_terminal (f))
		return tty_getchar (f, mode);
	
	{
		char c;
		long r;
		
		r = xdd_read (f, &c, 1L);
		if (r != 1)
			return MiNTEOF;
		else
			return ((long) c) & 0xff;
	}
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
c_conin (void)
{
	return file_getchar (curproc->p_fd->ofiles[0], COOKED | ECHO);
}

long _cdecl
c_conout (int c)
{
	return file_putchar (curproc->p_fd->ofiles[1], (long) c, COOKED);
}

long _cdecl
c_auxin (void)
{
	return file_getchar (curproc->p_fd->ofiles[2], RAW);
}

long _cdecl
c_auxout (int c)
{
	return file_putchar (curproc->p_fd->ofiles[2], (long) c, RAW);
}

long _cdecl
c_prnout (int c)
{
	return file_putchar (curproc->p_fd->ofiles[3], (long) c, RAW);
}

long _cdecl
c_rawio (int c)
{
	PROC *p = curproc;
	
	if (c == 0x00ff)
	{
		long r;
		
		if (!file_instat (p->p_fd->ofiles[0]))
			return 0;
		
		r = file_getchar (p->p_fd->ofiles[0], RAW);
		if (r <= E_OK)
			return 0;
		
		return r;
	}
	else
		return file_putchar (p->p_fd->ofiles[1], (long) c, RAW);
}

long _cdecl
c_rawcin (void)
{
	return file_getchar (curproc->p_fd->ofiles[0], RAW);
}

long _cdecl
c_necin (void)
{
	return file_getchar (curproc->p_fd->ofiles[0], COOKED | NOECHO);
}

long _cdecl
c_conws (const char *str)
{
	const char *p = str;
	long cnt = 0;
	
	while (*p++)
		cnt++;
	
	return f_write (1, cnt, str);
}

long _cdecl
c_conrs (char *buf)
{
	long size, count, r;
	
	size = ((long) *buf) & 0xff;
	if (is_terminal (curproc->p_fd->ofiles[0]))
	{
		/* TB: When reading from a terminal, f_read is used which
		 * automatically breaks at the first newline
		 */
		r = f_read (0, size, buf + 2);
		if (r < E_OK)
			buf [1] = 0;
		else
			buf [1] = (char) r;
		
		return (r < E_OK) ? r : E_OK;
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
		r = c_conin ();
		
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
			 	return f_read (0, 1L, &dummy);
			 else
				return E_OK;
		}
		
		buf [count + 2] = dummy;
	}
}

long _cdecl
c_conis (void)
{
	return -(!!file_instat (curproc->p_fd->ofiles[0]));
}

long _cdecl
c_conos (void)
{
	return -(!!file_outstat (curproc->p_fd->ofiles[1]));
}

long _cdecl
c_prnos (void)
{
	return -(!!file_outstat (curproc->p_fd->ofiles[3]));
}

long _cdecl
c_auxis (void)
{
	return -(!!file_instat (curproc->p_fd->ofiles[2]));
}

long _cdecl
c_auxos (void)
{
	return -(!!file_outstat (curproc->p_fd->ofiles[2]));
}


/* Extended GEMDOS routines
 */

long _cdecl
f_instat (int h)
{
	PROC *proc;
	int fh = h;
	
# if O_GLOBAL
	if (fh >= 100)
	{
		proc = rootproc;
		fh -= 100;
	}
	else
# endif
		proc = curproc;
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Finstat: bad handle %d", h));
		return EBADF;
	}
	
	return file_instat (proc->p_fd->ofiles[fh]);
}

long _cdecl
f_outstat (int h)
{
	PROC *proc;
	int fh = h;
	
# if O_GLOBAL
	if (fh >= 100)
	{
		proc = rootproc;
		fh -= 100;
	}
	else
# endif
		proc = curproc;
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Foutstat: bad handle %d", h));
		return EBADF;
	}
	
	return file_outstat (proc->p_fd->ofiles[fh]);
}

long _cdecl
f_getchar (int h, int mode)
{
	PROC *proc;
	int fh = h;
	
# if O_GLOBAL
	if (fh >= 100)
	{
		proc = rootproc;
		fh -= 100;
	}
	else
# endif
		proc = curproc;
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Fgetchar: bad handle %d", h));
		return EBADF;
	}
	
	return file_getchar (proc->p_fd->ofiles[fh], mode);
}

long _cdecl
f_putchar (int h, long c, int mode)
{
	PROC *proc;
	int fh = h;
	
# if O_GLOBAL
	if (fh >= 100)
	{
		proc = rootproc;
		fh -= 100;
	}
	else
# endif
		proc = curproc;
	
	if (fh < MIN_HANDLE || fh >= proc->p_fd->nfiles)
	{
		DEBUG (("Fputchar: bad handle %d", h));
		return EBADF;
	}
	
	return file_putchar (proc->p_fd->ofiles[fh], c, mode);
}
