#ifndef _ASSERT_H_
#define _ASSERT_H_

#ifndef NDEBUG

#ifndef __STDLIB
#include <stdlib.h>
#endif

#ifndef __STDIO
#include <stdio.h>
#endif

#define assert(expr)\
	((void)((expr)||(fprintf(stderr, \
	"\nAssertion failed: %s, file %s, line %d\n",\
	 #expr, __FILE__, __LINE__ ),\
	 ((int (*)(void))abort)())))
#else
#define assert(expr)
#endif /* NDEBUG */
#endif /* _ASSERT_H_ */

