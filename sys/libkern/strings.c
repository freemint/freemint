# include "buildinfo/version.h"

# ifndef str
# define str(x)		_stringify(x)
# define _stringify(x)	#x
# endif

char *kstrings[] = {
	"Wrong FreeMiNT version!\r\n",
	"This module is compiled against " MINT_NAME,
	MINT_VERS_STRING,
	"\r\n",
	"Wrong kentry-version (kernel-version too ",
	"high)",
	"low)"
};

