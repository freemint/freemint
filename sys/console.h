/*
 * $Id$
 * 
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

long _cdecl c_conin	(void);
long _cdecl c_conout	(int c);
long _cdecl c_auxin	(void);
long _cdecl c_auxout	(int c);
long _cdecl c_prnout	(int c);
long _cdecl c_rawio	(int c);
long _cdecl c_rawcin	(void);
long _cdecl c_necin	(void);
long _cdecl c_conws	(const char *str);
long _cdecl c_conrs	(char *buf);
long _cdecl c_conis	(void);
long _cdecl c_conos	(void);
long _cdecl c_prnos	(void);
long _cdecl c_auxis	(void);
long _cdecl c_auxos	(void);

long _cdecl f_instat	(int fh);
long _cdecl f_outstat	(int fh);
long _cdecl f_getchar	(int fh, int mode);
long _cdecl f_putchar	(int fh, long c, int mode);


# endif /* _console_h */
