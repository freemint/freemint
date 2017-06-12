#!/bin/sh

# read compiler name
out=`$CC -v 2>&1 | tail -n 1`

cat <<EOF
/* This is an automatically generated file.
 * Do not edit, edit cdef.sh instead
 */

# ifndef _cdef_h
# define _cdef_h

# define _COMPILER_NAME $out
# define _COMPILER_OPTS $OPTS
# define _COMPILER_DEFS $DEFS

# endif /* _cdef_h */
EOF
