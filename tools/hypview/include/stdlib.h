/*      STDLIB.H

        Standard Library Includes

        Copyright (c) Borland International 1990
        All Rights Reserved.
*/


#if !defined( __STDLIB )
#define __STDLIB


#define EXIT_FAILURE    !0
#define EXIT_SUCCESS    0
#define RAND_MAX        32767


typedef unsigned long   size_t;


typedef struct
{
    short    quot;
    short    rem;
} div_t;

typedef struct
{
    long   quot;
    long   rem;
} ldiv_t;


double  atof( const char *str );
short     atoi( const char *str );
long    atol( const char *str );

char    *ecvt( double d, short ndig, short *dec, short *sign );
char    *fcvt( double d, short ndig, short *dec, short *sign );
char    *gcvt( double value, short dec, char * buf );

char    *itoa( short value, char *string, short radix );
char    *ltoa( long value, char *string, short radix );
char    *ultoa( unsigned long value, char *string, short radix );

void    *malloc( size_t size );
void    *calloc( size_t elt_count, size_t elt_size );
void    free( void *ptr );
void    *realloc( void *ptr, size_t size );

short     abs( short x );
long    labs( long x );
div_t   div( short n, short d );
ldiv_t  ldiv( long n, long d );

short     rand( void );
void    srand( unsigned short seed );
#define random( x ) (rand() % (x))
double  strtod( const char *s, char **endptr );

short     system( const char *command );

void    exit( short status );
void    abort( void );
short     atexit( void (*func)( void ) );

char    *getenv( const char *name );

void    *bsearch( const void *key, const void *base,
          size_t nmemb, size_t size,
          short (*compar)() );
void    qsort( void *base, size_t nmemb, size_t size,
          short (*compar)() );

#endif

/************************************************************************/

