/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _mint_version_h
# define _mint_version_h


# define MINT_MAJ_VERSION	1
# define MINT_MIN_VERSION	16
# define MINT_PATCH_LEVEL	0

# define THIRD_PARTY		"FreeMiNT"

# if 1
# define MINT_BETA		"-ALPHA"
# define MINT_BETA_IDENT	'a'
# else
# define MINT_BETA		""
# define MINT_BETA_IDENT	0
# endif

# define MINT_VERS_STRING	str(MINT_MAJ_VERSION) "."	\
				str(MINT_MIN_VERSION) "."	\
				str(MINT_PATCH_LEVEL)	\
				MINT_BETA

# ifndef THIRD_PARTY
#  ifdef MULTITOS
#   define MINT_NAME		"MultiTOS"
#  else
#   define MINT_NAME		"MiNT"
#  endif
# else
#  define MINT_NAME		THIRD_PARTY
# endif


# endif /* _mint_version_h */
