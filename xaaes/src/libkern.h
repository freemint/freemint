
# ifndef _libkern_h
# define _libkern_h

# include <stdarg.h>
# include <sys/types.h>

typedef unsigned char uchar;

/* XXX clean me up */
# ifndef _cdecl
# define _cdecl
# endif


# define str(x)		_stringify (x)
# define _stringify(x)	#x


/*
 * kernel character classification and conversion
 */

extern uchar _mint_ctype[];

# define _CTc		0x01	/* control character */
# define _CTd		0x02	/* numeric digit */
# define _CTu		0x04	/* upper case */
# define _CTl		0x08	/* lower case */
# define _CTs		0x10	/* whitespace */
# define _CTp		0x20	/* punctuation */
# define _CTx		0x40	/* hexadecimal */

# define isalnum(c)	(  _mint_ctype[(uchar)(c)] & (_CTu|_CTl|_CTd))
# define isalpha(c)	(  _mint_ctype[(uchar)(c)] & (_CTu|_CTl))
# define isascii(c)	(!((c) & ~0x7f))
# define iscntrl(c)	(  _mint_ctype[(uchar)(c)] &  _CTc)
# define isdigit(c)	(  _mint_ctype[(uchar)(c)] &  _CTd)
# define isgraph(c)	(!(_mint_ctype[(uchar)(c)] & (_CTc|_CTs)) && (_mint_ctype[(uchar)(c)]))
# define islower(c)	(  _mint_ctype[(uchar)(c)] &  _CTl)
# define isprint(c)	(!(_mint_ctype[(uchar)(c)] &  _CTc)       && (_mint_ctype[(uchar)(c)]))
# define ispunct(c)	(  _mint_ctype[(uchar)(c)] &  _CTp)
# define isspace(c)	(  _mint_ctype[(uchar)(c)] &  _CTs)
# define isupper(c)	(  _mint_ctype[(uchar)(c)] &  _CTu)
# define isxdigit(c)	(  _mint_ctype[(uchar)(c)] &  _CTx)
# define iswhite(c)	(isspace (c))

# define isodigit(c)	((c) >= '0' && (c) <= '7')
# define iscymf(c)	(isalpha(c) || ((c) == '_'))
# define iscym(c)	(isalnum(c) || ((c) == '_'))

# define _toupper(c)	((c) ^ 0x20)
# define _tolower(c)	((c) ^ 0x20)
# define _toascii(c)	((c) & 0x7f)
# define _toint(c)	((c) <= '9' ? (c) - '0' : toupper (c) - 'A')

int	_cdecl _mint_tolower	(int c);
int	_cdecl _mint_toupper	(int c);

static inline int
_mint_toupper_inline (register int c)
{
	return (islower (c) ? _toupper (c) : c);
}

static inline int
_mint_tolower_inline (register int c)
{
	return (isupper (c) ? _tolower (c) : c);
}

# define _ctype			_mint_ctype
# define toupper		_mint_toupper
# define tolower		_mint_tolower
# define TOUPPER		_mint_toupper_inline
# define TOLOWER		_mint_tolower_inline


/*
 * kernel string functions
 */

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
char *	_cdecl _mint_strchr	(const char *str, long which);
char *	_cdecl _mint_strrchr	(const char *str, long which);
char *	_cdecl _mint_strrev	(char *s);

char *	_cdecl _mint_strstr	(const char *s, const char *w);

long	_cdecl _mint_strtol	(const char *nptr, char **endptr, long base);
ulong	_cdecl _mint_strtoul	(const char *nptr, char **endptr, long base);

void *	_cdecl _mint_memchr	(void *s, long search, ulong size);
long	_cdecl _mint_memcmp	(const void *s1, const void *s2, ulong size);

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

/*
 * kernel block functions
 */

void *	_cdecl memcpy		(void *dst, const void *src, ulong nbytes);
void *	_cdecl memset		(void *dst, int ucharfill, ulong size);

void	_cdecl bcopy		(const void *src, void *dst, ulong nbytes);
void	_cdecl bzero		(void *dst, ulong size);

# endif /* _libkern_h */
