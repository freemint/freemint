/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosfile_h
# define _dosfile_h

# include "mint/mint.h"
# include "mint/file.h"

extern short select_coll;

long _cdecl sys_f_open (const char *name, short mode);
long _cdecl sys_f_create (const char *name, short attrib);
long _cdecl sys_f_close (short fd);
long _cdecl sys_f_read (short fd, long count, void *buf);
long _cdecl sys_f_write (short fd, long count, const void *buf);
long _cdecl sys_f_seek (long place, short fd, short how);
long _cdecl sys_f_dup (short fd);
long _cdecl sys_f_force (short newh, short oldh);
long _cdecl sys_f_datime (ushort *timeptr, short fd, short wflag);
long _cdecl sys_f_lock (short fd, short mode, long start, long length);
long _cdecl sys_f_cntl (short fd, long arg, short cmd);
long _cdecl sys_f_select (unsigned short timeout, long *rfdp, long *wfdp, long *xfdp);
long _cdecl sys_f_midipipe (short pid, short in, short out);
long _cdecl sys_f_fchown (short fd, short uid, short gid);
long _cdecl sys_f_fchmod (short fd, ushort mode);
long _cdecl sys_f_seek64 (llong place, short fd, short how, llong *newpos);
long _cdecl sys_f_poll (POLLFD *fds, ulong nfds, ulong timeout);

long _cdecl sys_ffstat (short fd, struct stat *st);
long _cdecl sys_fwritev (short fd, const struct iovec *iov, long niov);
long _cdecl sys_freadv (short fd, const struct iovec *iov, long niov);


# endif /* _dosfile_h */
