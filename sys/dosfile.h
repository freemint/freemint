/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosfile_h
# define _dosfile_h

# include "mint/mint.h"
# include "mint/file.h"


extern short select_coll;

FILEPTR * do_open (const char *name, int mode, int attr, XATTR *x, long *err);
long do_pclose (PROC *p, FILEPTR *f);
long do_close (FILEPTR *f);
long get_opens (fcookie *object, struct listopens *l);
long _cdecl f_open (const char *name, int mode);
long _cdecl f_create (const char *name, int attrib);
long _cdecl f_close (int fh);
long _cdecl f_read (int fh, long count, char *buf);
long _cdecl f_write (int fh, long count, const char *buf);
long _cdecl f_seek (long place, int fh, int how);
long _cdecl f_dup (int fh);
long _cdecl f_force (int newh, int oldh);
long _cdecl f_datime (ushort *timeptr, int fh, int wflag);
long _cdecl f_lock (int fh, int mode, long start, long length);
long _cdecl f_pipe (short *usrh);
long _cdecl f_cntl (int fh, long arg, int cmd);
long _cdecl f_select (unsigned timeout, long *rfdp, long *wfdp, long *xfdp);
long _cdecl f_midipipe (int pid, int in, int out);

long _cdecl f_fchown (int fh, int uid, int gid);
long _cdecl f_fchmod (int fh, unsigned mode);


# endif /* _dosfile_h */
