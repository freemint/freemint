/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

# ifndef _libkern_h
# define _libkern_h

# include <stdarg.h>

# include "mint/kernel.h"
# include "mint/kcompiler.h"
# include "mint/ktypes.h"
# include "mint/basepage.h"


# ifndef str
# define str(x)		_stringify(x)
# define _stringify(x)	#x
# endif

/*
 * character classification and conversion
 * generic macros
 */

# define _CTc		0x01	/* control character */
# define _CTd		0x02	/* numeric digit */
# define _CTu		0x04	/* upper case */
# define _CTl		0x08	/* lower case */
# define _CTs		0x10	/* whitespace */
# define _CTp		0x20	/* punctuation */
# define _CTx		0x40	/* hexadecimal */

# define isalnum(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl|_CTd))
# define isalpha(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl))
# define isascii(c)	(!((c) & ~0x7f))
# define iscntrl(c)	(  _ctype[(unsigned char)(c)] &  _CTc)
# define isdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTd)
# define isgraph(c)	(!(_ctype[(unsigned char)(c)] & (_CTc|_CTs)) && (_ctype[(unsigned char)(c)]))
# define islower(c)	(  _ctype[(unsigned char)(c)] &  _CTl)
# define isprint(c)	(!(_ctype[(unsigned char)(c)] &  _CTc)       && (_ctype[(unsigned char)(c)]))
# define ispunct(c)	(  _ctype[(unsigned char)(c)] &  _CTp)
# define isspace(c)	(  _ctype[(unsigned char)(c)] &  _CTs)
# define isupper(c)	(  _ctype[(unsigned char)(c)] &  _CTu)
# define isxdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTx)
# define iswhite(c)	(isspace (c))

# define isodigit(c)	((c) >= '0' && (c) <= '7')
# define iscymf(c)	(isalpha(c) || ((c) == '_'))
# define iscym(c)	(isalnum(c) || ((c) == '_'))

# define _toupper(c)	((c) ^ 0x20)
# define _tolower(c)	((c) ^ 0x20)
# define _toascii(c)	((c) & 0x7f)
# define _toint(c)	((c) <= '9' ? (c) - '0' : TOUPPER(c) - 'A')

# define TOUPPER(c)	(islower(c) ? _toupper(c) : c)
# define TOLOWER(c)	(isupper(c) ? _tolower(c) : c)


# if __KERNEL__ == 1 || __KERNEL__ == 2

/*
 * character classification and conversion
 * kernel specific
 */

extern unsigned char _mint_ctype[];

int	_cdecl _mint_tolower	(int c);
int	_cdecl _mint_toupper	(int c);

# define _ctype			_mint_ctype
# define toupper		_mint_toupper
# define tolower		_mint_tolower


/*
 * kernel string functions
 */

long	_cdecl kvsprintf	(char *buf, long buflen, const char *fmt, va_list args) __attribute__((format(printf, 3, 0)));
long	_cdecl ksprintf		(char *buf, long buflen, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int	_cdecl ksprintf_old	(char *buf, const char *fmt, ...);

char *	_cdecl _mint_getenv	(BASEPAGE *bp, const char *var);

long	_cdecl _mint_atol	(const char *s);
long	_cdecl strtonumber	(const char *name, long *result, int neg, int zerolead);

long	_cdecl _mint_strlen	(const char *s);

long	_cdecl _mint_strcmp	(const char *str1, const char *str2);
long	_cdecl _mint_strncmp	(const char *str1, const char *str2, long len);

long	_cdecl _mint_stricmp	(const char *str1, const char *str2);
long	_cdecl _mint_strnicmp	(const char *str1, const char *str2, long len);

char *	_cdecl _mint_strcpy	(char *dst, const char *src);
char *	_cdecl _mint_strncpy	(char *dst, const char *src, long len);
void	_cdecl _mint_strncpy_f	(char *dst, const char *src, long len);

char *	_cdecl _mint_strlwr	(char *s);
char *	_cdecl _mint_strupr	(char *s);

char *	_cdecl _mint_strcat	(char *dst, const char *src);
char *	_cdecl _mint_strchr	(const char *s, long charwanted);
char *	_cdecl _mint_strrchr	(const char *str, long which);
char *	_cdecl _mint_strrev	(char *s);

char *	_cdecl _mint_strstr	(const char *s, const char *w);

long	_cdecl _mint_strtol	(const char *nptr, char **endptr, long base);
llong	_cdecl _mint_strtoll	(const char *nptr, char **endptr, long base);
ulong	_cdecl _mint_strtoul	(const char *nptr, char **endptr, long base);
ullong	_cdecl _mint_strtoull	(const char *nptr, char **endptr, long base);

void *	_cdecl _mint_memchr	(void *s, long search, unsigned long size);
long	_cdecl _mint_memcmp	(const void *s1, const void *s2, unsigned long size);

# define atol			_mint_atol
# define strlen			_mint_strlen
# define strcmp			_mint_strcmp
# define strncmp		_mint_strncmp
# define stricmp		_mint_stricmp
# define strnicmp		_mint_strnicmp
# define strcpy			_mint_strcpy
# define strncpy		_mint_strncpy
# define strncpy_f		_mint_strncpy_f
# define strlwr			_mint_strlwr
# define strupr			_mint_strupr
# define strcat			_mint_strcat
# define strchr			_mint_strchr
# define strrchr		_mint_strrchr
# define strrev			_mint_strrev
# define strstr			_mint_strstr
# define strtol			_mint_strtol
# define strtoll		_mint_strtoll
# define strtoul		_mint_strtoul
# define strtoull		_mint_strtoull
# define memchr			_mint_memchr
# define memcmp			_mint_memcmp

/* for backward compatibility */
int	_cdecl _mint_o_stricmp	(const char *str1, const char *str2);
int	_cdecl _mint_o_strnicmp	(const char *str1, const char *str2, int len);


/*
 * kernel time help functions
 */

void	_cdecl unix2calendar	(long tv_sec,
				 unsigned short *year, unsigned short *month,
				 unsigned short *day, unsigned short *hour,
				 unsigned short *minute, unsigned short *second);
long	_cdecl unix2xbios	(long tv_sec);
long	_cdecl dostime		(long tv_sec);
long	_cdecl unixtime		(unsigned short time, unsigned short date);


/*
 * kernel block functions
 */

void	_cdecl _mint_blockcpy	(char *dst, const char *src, unsigned long nblocks);
void	_cdecl _mint_quickcpy	(void *dst, const void *src, unsigned long nbytes);
void	_cdecl _mint_quickswap	(void *dst, void *src, unsigned long nbytes);
void	_cdecl _mint_quickzero	(char *place, unsigned long size);

void	_cdecl _mint_bcopy	(const void *src, void *dst, unsigned long nbytes);
void	_cdecl _mint_bzero	(void *dst, unsigned long size);

/* old definitions */
# define quickmovb		memcpy
# define quickmove		_mint_quickcpy
# define quickswap		_mint_quickswap
# define quickzero		_mint_quickzero
# define bcopy			_mint_bcopy
# define bzero			_mint_bzero

/* introduced from ozk I think */
# define mint_bzero		_mint_bzero

void *	_cdecl memcpy		(void *dst, const void *src, unsigned long nbytes);
void *	_cdecl memset		(void *dst, int ucharfill, unsigned long size);

# endif

# if __KERNEL__ == 2
# include "kernel_xfs_xdd.h"
# endif

# if __KERNEL__ == 3
# include "kernel_module.h"
# endif

void unaligned_putl(char *addr, long value);

# endif /* _libkern_h */
