/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosmem_h
# define _dosmem_h

# include "mint/mint.h"
# include "mint/mem.h"


long _cdecl m_addalt (long start, long size);
long _cdecl m_xalloc (long size, int mode);
long _cdecl m_alloc (long size);
long _cdecl m_free (virtaddr block);
long _cdecl m_shrink (int dummy, virtaddr block, long size);
long _cdecl m_validate (int pid, void *addr, long size, long *flags);
long _cdecl m_access (int mode, void *addr, long size);
long _cdecl p_exec (int mode, const void *ptr1, const void *ptr2, const void *ptr3);
long terminate (PROC *curproc, int code, int que);
long _cdecl p_term (int code);
long kernel_pterm (PROC *p, int code);
long _cdecl p_term0 (void);
long _cdecl p_termres (long save, int code);
long _cdecl p_waitpid (int pid, int nohang, long *rusage);
long _cdecl p_wait3 (int nohang, long *rusage);
long _cdecl p_wait (void);
long _cdecl p_vfork (void);
long _cdecl p_fork (void);


# endif /* _dosmem_h */
