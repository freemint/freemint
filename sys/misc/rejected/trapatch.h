/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _trapatch_h
# define _trapatch_h

# ifdef __TURBOC__
# include "include\mint.h"
# else
# include "include/mint.h"
# endif


# define DOS_MAX 0x160


# define TPM_INSTALL	0
# define TPM_DEINSTALL	1
# define TPM_INFO	2

# define TPW_MAXWHAT	0
# define TPW_VERSION	1
# define TPW_ENTRIES	2
# define TPW_ENTRY	3

# define TPL_GEMDOS	0
# define TPL_BIOS	1
# define TPL_XBIOS	2
# define TPL_AES	3
# define TPL_VDI	4

# define ETP_fnum_illegal	-99
# define ETP_no_xbra		-98
# define ETP_xbra_not_found	-97
# define ETP_lib_illegal	-96
# define ETP_info_not_available	-95
# define ETP_dupe		-94
# define ETP_illresvd		-93
# define ETP_invalid_mode	-92


void init_dos_trap (void);

long s_trapatch (ushort mode, ushort info, ushort lib,
                 ushort func, void *vec, void *reserved);

# endif /* _trapatch_h */
