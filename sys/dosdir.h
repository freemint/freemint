/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosdir_h
# define _dosdir_h

# include "mint/mint.h"
# include "mint/file.h"


/* table of processes holding locks on drives */
extern PROC *dlockproc [NUM_DRIVES];
extern long searchtime;

long _cdecl d_setdrv	(int d);
long _cdecl d_getdrv	(void);
long _cdecl d_free	(long *buf, int d);
long _cdecl d_create	(const char *path);
long _cdecl d_delete	(const char *path);
long _cdecl d_setpath	(const char *path);
long _cdecl d_getpath	(char *path, int drv);
long _cdecl d_getcwd	(char *path, int drv, int size);
long _cdecl f_setdta	(DTABUF *dta);
long _cdecl f_getdta	(void);
long _cdecl f_sfirst	(const char *path, int attrib);
long _cdecl f_snext	(void);
long _cdecl f_attrib	(const char *name, int rwflag, int attr);
long _cdecl f_delete	(const char *name);
long _cdecl f_rename	(int junk, const char *old, const char *new);
long _cdecl d_pathconf	(const char *name, int which);
long _cdecl d_opendir	(const char *path, int flags);
long _cdecl d_readdir	(int len, long handle, char *buf);
long _cdecl d_xreaddir	(int len, long handle, char *buf, XATTR *xattr, long *xret);
long _cdecl d_rewind	(long handle);
long _cdecl d_closedir	(long handle);
long _cdecl f_xattr	(int flag, const char *name, XATTR *xattr);
long _cdecl f_link	(const char *old, const char *new);
long _cdecl f_symlink	(const char *old, const char *new);
long _cdecl f_readlink	(int buflen, char *buf, const char *linkfile);
long _cdecl d_cntl	(int cmd, const char *name, long arg);
long _cdecl f_chown	(const char *name, int uid, int gid);
long _cdecl f_chmod	(const char *name, unsigned mode);
long _cdecl d_lock	(int mode, int drv);
long _cdecl d_readlabel	(const char *path, char *label, int maxlen);
long _cdecl d_writelabel (const char *path, const char *label);
long _cdecl d_chroot	(const char *dir);
long _cdecl f_stat64	(int flag, const char *name, STAT *stat);


# endif /* _dosdir_h */
