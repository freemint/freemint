/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosdir_h
# define _dosdir_h

# include "mint/mint.h"
# include "mint/file.h"

struct proc;

/* table of processes holding locks on drives */
extern struct proc *dlockproc[NUM_DRIVES];
extern long searchtime;

long _cdecl sys_d_setdrv	(int d);
long _cdecl sys_d_getdrv	(void);
long _cdecl sys_d_free		(long *buf, int d);
long _cdecl sys_d_create	(const char *path);
long _cdecl sys_d_delete	(const char *path);
long        sys_d_setpath0	(struct proc *p, const char *path);
long _cdecl sys_d_setpath	(const char *path);
long _cdecl sys_d_getpath	(char *path, int drv);
long _cdecl sys_d_getcwd	(char *path, int drv, int size);
long _cdecl sys_f_setdta	(DTABUF *dta);
DTABUF *_cdecl sys_f_getdta	(void);
long _cdecl sys_f_sfirst	(const char *path, int attrib);
long _cdecl sys_f_snext		(void);
long _cdecl sys_f_attrib	(const char *name, int rwflag, int attr);
long _cdecl sys_f_delete	(const char *name);
long _cdecl sys_f_rename	(short junk, const char *old, const char *new);
long _cdecl sys_d_pathconf	(const char *name, int which);
long _cdecl sys_d_opendir	(const char *path, int flags);
long _cdecl sys_d_readdir	(int len, long handle, char *buf);
long _cdecl sys_d_xreaddir	(int len, long handle, char *buf, XATTR *xattr, long *xret);
long _cdecl sys_d_rewind	(long handle);
long _cdecl sys_d_closedir	(long handle);
long _cdecl sys_f_xattr		(int flag, const char *name, XATTR *xattr);
long _cdecl sys_f_link		(const char *old, const char *new);
long _cdecl sys_f_symlink	(const char *old, const char *new);
long _cdecl sys_f_readlink	(int buflen, char *buf, const char *linkfile);
long _cdecl sys_d_cntl		(int cmd, const char *name, long arg);
long _cdecl sys_f_chown		(const char *name, int uid, int gid);
long _cdecl sys_f_chown16	(const char *name, int uid, int gid, int follow_symlinks);
long _cdecl sys_f_chmod		(const char *name, unsigned short mode);
long _cdecl sys_d_lock		(int mode, int drv);
long _cdecl sys_d_readlabel	(const char *path, char *label, int maxlen);
long _cdecl sys_d_writelabel 	(const char *path, const char *label);
long _cdecl sys_d_chroot	(const char *dir);
long _cdecl sys_f_stat64	(int flag, const char *name, STAT *stat);
long _cdecl sys_f_chdir		(short fd);
long _cdecl sys_f_opendir	(short fd);
long _cdecl sys_f_dirfd		(long handle);


# endif /* _dosdir_h */
