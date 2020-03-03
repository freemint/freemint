#include <ctype.h>

#ifndef _IScntrl
#include "ctypeint.h"
#endif

int (toupper)(int c)
{
	return islower(c) ? ((c) - 'a' - 'A') : (c);
}
