/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

#ifndef _mint_version_h
#define _mint_version_h

#define THIRD_PARTY		"FreeMiNT"

#ifndef THIRD_PARTY
# ifdef MULTITOS
#  define MINT_NAME		"MultiTOS"
# else
#  define MINT_NAME		"MiNT"
# endif
#else
# define MINT_NAME		THIRD_PARTY
#endif

/*
 * main version macros
 */
#define MINT_MAJ_VERSION	1
#define MINT_MIN_VERSION	18
#define MINT_PATCH_LEVEL	0

/*
 * source status
 * 
 * NOTE: only one of these defines must be nonzero
 */
#define MINT_STATUS_CVS		0
#define MINT_STATUS_BETA	0
#define MINT_STATUS_RELEASE	1

#if (MINT_STATUS_CVS + MINT_STATUS_BETA + MINT_STATUS_RELEASE) > 1
# error only one of the MINT_STATUS_* macros can be 1
#endif

/*
 * pretty printing macros
 */

#if MINT_STATUS_CVS
# define MINT_STATUS		"-ALPHA"
# define MINT_STATUS_IDENT	'a'
#endif

#if MINT_STATUS_BETA
# define MINT_STATUS		"-BETA"
# define MINT_STATUS_IDENT	'b'
#endif

#if MINT_STATUS_RELEASE
# define MINT_STATUS		"-RELEASE"
# define MINT_STATUS_IDENT	'r'
#endif

#define MINT_VERS_STRING	str(MINT_MAJ_VERSION) "." \
				str(MINT_MIN_VERSION) "." \
				str(MINT_PATCH_LEVEL) \
				MINT_STATUS

#if MINT_STATUS_CVS

/* the cvs version always looks for a -cur dir */
# define MINT_VERS_PATH_STRING	str(MINT_MAJ_VERSION) "-" \
				str(MINT_MIN_VERSION) "-" \
				"cur"
#else

# define MINT_VERS_PATH_STRING	str(MINT_MAJ_VERSION) "-" \
				str(MINT_MIN_VERSION) "-" \
				str(MINT_PATCH_LEVEL)

#endif

#endif /* _mint_version_h */
