/*      CTYPE.H

        Character definitions

        Copyright (c) Borland International 1990
        All Rights Reserved.
*/


#if !defined( __CTYPE )
#define __CTYPE


short     isalnum( short c );
short     isalpha( short c );
short     isascii( short c );
short     iscntrl( short c );
short     isdigit( short c );
short     isodigit( short c );
short     isxdigit( short c );
short     isgraph( short c );
short     isprint( short c );
short     ispunct( short c );
short     islower( short c );
short     isupper( short c );
short     isspace( short c );
short     tolower( short c );
short     toupper( short c );

#define toascii(c)      ((c) & 0x7f)

#endif

/************************************************************************/

