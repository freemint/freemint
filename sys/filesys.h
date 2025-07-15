/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _filesys_h
# define _filesys_h

# include "mint/mint.h"
# include "mint/file.h"

# include "xfs_xdd.h"


/*
 * forward declarations
 */
struct ucred;


/*
 * exported definitions
 */
# define DIRSEP(p)	(((p) == '\\') || ((p) == '/'))
# define MAX_LINKS	4
# define follow_links	((char *) -1L)


/*
 * external variables
 */
extern FILESYS *active_fs;
extern FILESYS *drives[NUM_DRIVES];
extern int aliasdrv[NUM_DRIVES];
extern long dosdrvs;


/*
 * exported functions
 */
void kill_cache(fcookie *dir, char *name);
long cache_lookup(fcookie *dir, char *name, fcookie *res);
void cache_init(void);
void clobber_cookie(fcookie *fc);
void init_filesys(void);
const char *xfs_fsname(FILESYS *fs);
void xfs_add(FILESYS *fs);
void close_filesys(void);
long _s_ync(void);
long _cdecl sys_s_ync(void);
long _cdecl sys_fsync(short fh);
void _changedrv(ushort drv, const char *function);
#define changedrv(drv) _changedrv(drv, FUNCTION);
long disk_changed(ushort drv);
long _cdecl relpath2cookie(struct proc *p, fcookie *dir, const char *path, char *lastnm,
		    fcookie *res, int depth);
long _cdecl path2cookie(struct proc *p, const char *path, char *lastname, fcookie *res);
void _cdecl release_cookie(fcookie *fc);
void dup_cookie(fcookie *new, fcookie *old);
int _cdecl denyshare(FILEPTR *list, FILEPTR *newfileptr);
int denyaccess(struct ucred *cred, XATTR *, ushort);
LOCK *denylock(ushort pid, LOCK *list, LOCK *newlock);
long dir_access(struct ucred *cred, fcookie *, ushort, ushort *);
int has_wild(const char *name);
void copy8_3(char *dest, const char *src);
int pat_match(const char *name, const char *template);
int samefile(fcookie *, fcookie *);

# endif /* _filesys_h */
