/*  sockets_global.c -- MiNTLib.
    Copyright (C) 2000 Frank Naumann <fnaumann@freemint.de>

    Modified to support Pure-C, Thorsten Otto.

    This file is part of the MiNTLib project, and may only be used
    modified and distributed under the terms of the MiNTLib project
    license, COPYMINT.  By continuing to use, modify, or distribute
    this file you indicate that you have read the license and
    understand and accept it fully.
*/

#include "stsocket.h"

#if !defined(MAGIC_ONLY)
short __libc_newsockets = 1;
#endif

