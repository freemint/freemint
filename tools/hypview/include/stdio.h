/*      STDIO.H

        Standard I/O Definition Includes

        Copyright (c) Borland International 1990
        All Rights Reserved.
*/


#if !defined( __STDIO )
#define __STDIO


#include <stdarg.h>


typedef unsigned long   size_t;
typedef unsigned long   fpos_t;

typedef struct
{
        void *BufPtr;           /* next byte write              */
        void *BufLvl;           /* next byte read               */
        void *BufStart;         /* first byte of buffer         */
        void *BufEnd;           /* first byte after buffer      */
        short  Handle;            /* gemdos handle                */
        char Flags;             /* some Flags                   */
        char resv;
        char ChrBuf;            /* little buffer                */
        char ungetFlag;
} FILE;




/****** FileIo constants ************************************************/

#define NULL        ((void *)0)
#define OPEN_MAX        32
#define FOPEN_MAX       32
#define FILENAME_MAX    128
#define PATH_MAX        128
#define BUFSIZ          1024
#define EOF             (-1)

#define O_RDONLY    0x00
#define O_WRONLY    0x01
#define O_RDWR      0x02
#define O_APPEND    0x08
#define O_CREAT     0x20
#define O_TRUNC     0x40
#define O_EXCL      0x80

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define TMP_MAX     65535
#define L_tmpnam    13

#define _IOFBF      0
#define _IOLBF      1
#define _IONBF      2

#define stdout      (&_StdOutF)
#define stdin       (&_StdInF)
#define stderr      (&_StdErrF)
#define stdaux      (&_StdAuxF)
#define stdprn      (&_StdPrnF)

/****** External data **************************************************/

extern FILE         _StdOutF;
extern FILE         _StdInF;
extern FILE         _StdErrF;
extern FILE         _StdAuxF;
extern FILE         _StdPrnF;

extern short errno;
extern char         *sys_errlist[];


/****** FileIo routines *************************************************/

void    clearerr( FILE *stream );
short     fclose( FILE *stream );
short     feof( FILE *stream );
short     ferror( FILE *stream );
short     fflush( FILE *stream );
short     fgetc( FILE *stream );
short     getc( FILE *stream );
short             getchar( void );
short     fgetpos( FILE *stream, fpos_t *pos );
char    *fgets( char *str, short n, FILE *stream );
FILE    *fopen( const char *filename, const char *mode );
short     fprintf( FILE *stream, const char *format, ... );
short     fputc( short ch, FILE *stream );
short     putc( short ch, FILE *stream );
short     putchar( short c );
short     fputs( const char *str, FILE *stream );
size_t  fread( void *buf, size_t elem_Siz, size_t count, FILE *stream );
FILE    *freopen( const char *filename, const char *mode, FILE *stream );
short     fscanf( FILE *stream, const char *format, ... );
short     fseek( FILE *stream, long offset, short mode );
void    rewind( FILE *stream);
short     fsetpos( FILE *stream, const fpos_t *pos );
long    ftell( FILE *stream );
size_t  fwrite( const void *buf, size_t elem_Siz, size_t count,
                FILE *stream );
char    *gets( char *str );
void    perror( char *s );
short     printf( const char *format, ... );
short     puts( const char *str );
short     scanf( const char *format, ... );
void    setbuf( FILE *stream, char *buf );
short     setvbuf( FILE *stream, char *buf, short type, size_t size );
short     sprintf( char *string, const char *format, ... );
short     sscanf( const char *string, const char *format, ... );
char    *tmpnam( char *s );
FILE    *tmpfile( void );
short     ungetc( short ch, FILE *stream );
short     vfprintf( FILE *stream, const char *format, va_list param );
short     vprintf( const char *format, va_list param );
short     vsprintf( char *string, const char *format, va_list param );
short     vfscanf( FILE *stream, const char *format, va_list param );
short     vscanf( const char *format, va_list param );
short     vsscanf( char *string, const char *format, va_list param );
short     fileno( FILE *stream );

/****** FileIo macros ***************************************************/

#define getc( c )     fgetc( c )
#define getchar()     fgetc( &_StdInF )
#define putc( c, s )  fputc( c, s )
#define putchar( c )  fputc( c, &_StdOutF )
#define fileno( s )   ( s )->Handle

/****** Handle level FileIo routines ***********************************/

short     open( const char *filename, short access, ... );
short     close( short handle );
short     creat( const char *filename, ... );
size_t  read( short handle, void *buf, size_t nbyte );
size_t  write( short handle, void *buf, size_t nbyte );
size_t  lseek( short handle, size_t offset, short mode );
short     remove( const char *filename );
short     unlink( const char *filename );
short     rename( const char *oldname, const char *newname );


#endif

/***********************************************************************/

