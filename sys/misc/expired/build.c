/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/* 
 * build.c - FreeMiNT. 
 * Set compilation time variables.
 * Author: Guido Flohr <gufl0000@stud.uni-sb.de> 
 * Started: Mon, 30 Mar 1998.
 */  

# ifdef __TURBOC__
# include "include\mint.h"
# else
# include "include/mint.h"
# endif
# include "build.h"

/* The sole purpose of this file is to speed up kernel compilation
 * because it is short...
 */

/* definitions in automatically created buildtime.h */
long MiNT_date = ((ulong) BUILD_DAY << 24) \
	| ((ulong) BUILD_MONTH << 16) \
	| (ulong) BUILD_YEAR;
long MiNT_time = ((ulong) BUILD_DAY_OF_WEEK << 24) \
	| ((ulong) BUILD_HOUR << 16) \
	| ((ulong) BUILD_MIN << 8) \
	| (ulong) BUILD_SEC;

/* That's all.  Won't win the Pulitzer Prize for this one.  */
