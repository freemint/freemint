/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file for other projects without my
 * explicit permission.
 */

/*
 * slb.h dated 99-08-15
 * 
 * Author:
 * Thomas Binder
 * (gryf@hrzpub.tu-darmstadt.de)
 * 
 * Purpose:
 * Structures and constants necessary for MagiC-style "share libraries".
 * 
 * History:
 * 
 * 99-08-20: - changed definition from *SHARED_LIB to SHARED_LIB
 *             much more logical
 * 
 * 99-08-15: - Creation (Gryf)
 */

# ifndef _mint_slb_h
# define _mint_slb_h

# include "basepage.h"
# include "mem.h"


/* How much stack space for a library's init() call? */
# define SLB_INIT_STACK	1024L

typedef struct shared_lib SHARED_LIB;
typedef struct slb_head SLB_HEAD;

typedef long _cdecl (*SLB_EXEC)(SHARED_LIB *sl, long fn, short nargs, ...);
typedef long _cdecl (*SLB_FUNC)(long pid, long fn, short nargs, ...);

/* The file header of a shared library */
struct slb_head
{
	long		slh_magic;	/* Magic value (0x70004afc) */
	char		*slh_name;	/* Name of the library */
	long		slh_version;	/* Version number */
	long		slh_flags;	/* Flags, currently 0L and unused */
	long	_cdecl	(*slh_slb_init)(void);
					/* Pointer to init()-function */
	void	_cdecl	(*slh_slb_exit)(void);
					/* Pointer to exit()-function */
	long	_cdecl	(*slh_slb_open)(BASEPAGE *b);
					/* Pointer to open()-function */
	long	_cdecl	(*slh_slb_close)(BASEPAGE *b);
					/* Pointer to close()-function */
	char		**slh_names;	/* Pointer to functions names, or 0L */
	long		slh_reserved[8];/* Currently 0L and unused */
	long		slh_no_funcs;	/* Number of functions */
	SLB_FUNC	slh_functions[0];
					/* The function pointers */
};

/*
 * Shared library structure, contents is completely _private_, i.e. MUST NOT
 * be used in applications
 */
struct shared_lib
{
	SHARED_LIB	*slb_next;	/* Pointer to next element in list */
	SLB_HEAD	*slb_head;	/* Pointer to library header */
	long		slb_version;	/* Version number */
	short		slb_used;	/* Usage counter */
	short		slb_users[(MAXPID + 7) / 8];
					/* List of using processes */
	short		slb_opened[(MAXPID + 7) / 8];
					/* Did process call slb_open()? */
	SLB_EXEC	*slb_exec;	/* Pointer to execution function */
	PROC		*slb_proc;	/* Corresponding process */
	MEMREGION	*slb_region;	/* Region of this structure */
	char		slb_name[1];	/* Name of the library. MUST BE THE
					 * LAST ELEMENT! */
};


# endif /* _mint_slb_h */
