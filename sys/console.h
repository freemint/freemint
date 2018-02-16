/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _console_h
# define _console_h

# include "mint/mint.h"
# include "mint/file.h"


long file_instat	(FILEPTR *f);
long file_outstat	(FILEPTR *f);
long file_getchar	(FILEPTR *f, int mode);
long file_putchar	(FILEPTR *f, long c, int mode);

long _cdecl sys_c_conin		(void);
long _cdecl sys_c_conout	(int c);
long _cdecl sys_c_auxin		(void);
long _cdecl sys_c_auxout	(int c);
long _cdecl sys_c_prnout	(int c);
long _cdecl sys_c_rawio		(int c);
long _cdecl sys_c_rawcin	(void);
long _cdecl sys_c_necin		(void);
long _cdecl sys_c_conws		(const char *str);
long _cdecl sys_c_conrs		(char *buf);
long _cdecl sys_c_conis		(void);
long _cdecl sys_c_conos		(void);
long _cdecl sys_c_prnos		(void);
long _cdecl sys_c_auxis		(void);
long _cdecl sys_c_auxos		(void);

long _cdecl sys_f_instat	(int fh);
long _cdecl sys_f_outstat	(int fh);
long _cdecl sys_f_getchar	(int fh, int mode);
long _cdecl sys_f_putchar	(int fh, long c, int mode);


# endif /* _console_h */
