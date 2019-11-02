#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __GNUC__
#include <gem.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>
#ifndef _WORD
#define _WORD short
#endif
#define cdecl
#endif
#ifdef __PUREC__
#include <tos.h>
#include <aes.h>
#include <vdi.h>
#ifndef _WORD
#define _WORD int
#endif
#endif
#include <ctype.h>

#ifndef FALSE
#  define FALSE 0
#  define TRUE 1
#endif
