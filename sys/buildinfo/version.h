/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _version_h
# define _version_h


# define MAJ_VERSION	1
# define MIN_VERSION	15
# define PATCH_LEVEL	13

# define THIRD_PARTY	"FreeMiNT"

# if 1
# define BETA		"-BETA"
# define BETA_IDENT	'b'
# else
# define BETA		""
# define BETA_IDENT	0
# endif

# define VERS_STRING	str (MAJ_VERSION) "."	\
			str (MIN_VERSION) "."	\
			str (PATCH_LEVEL)	\
			BETA

# ifndef THIRD_PARTY
#  ifdef MULTITOS
#   define MINT_NAME	"MultiTOS"
#  else
#   define MINT_NAME	"MiNT"
#  endif
# else
#  define MINT_NAME	THIRD_PARTY
# endif


# endif /* _version_h */
