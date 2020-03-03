#include <ctype.h>

#ifndef _IScntrl
#include "ctypeint.h"
#endif

int (tolower)(int c)
{
	return isupper(c) ? ((c) + 'a' - 'A') : (c);
}
