/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosfile_h
# define _dosfile_h

# include "mint/mint.h"
# include "mint/file.h"


extern short select_coll;

long _cdecl f_open (const char *name, short mode);
long _cdecl f_create (const char *name, short attrib);
long _cdecl f_close (short fd);
long _cdecl f_read (short fd, long count, char *buf);
long _cdecl f_write (short fd, long count, const char *buf);
long _cdecl f_seek (long place, short fd, short how);
long _cdecl f_dup (short fd);
long _cdecl f_force (short newh, short oldh);
long _cdecl f_datime (ushort *timeptr, short fd, short wflag);
long _cdecl f_lock (short fd, short mode, long start, long length);
long _cdecl f_cntl (short fd, long arg, short cmd);
long _cdecl f_select (ushort timeout, long *rfdp, long *wfdp, long *xfdp);
long _cdecl f_midipipe (short pid, short in, short out);
long _cdecl f_fchown (short fd, short uid, short gid);
long _cdecl f_fchmod (short fd, ushort mode);
long _cdecl f_seek64 (llong place, short fd, short how, llong *newpos);
long _cdecl f_poll (POLLFD *fds, ulong nfds, ulong timeout);

long _cdecl sys_fstat (short fd, struct stat *st);


# endif /* _dosfile_h */
