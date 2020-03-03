| setstack.S -- MiNTLib.
| Copyright (C) 1999, 2000 Guido Flohr <guido@freemint.de>
|
| This file is part of the MiNTLib project, and may only be used
| modified and distributed under the terms of the MiNTLib project
| license, COPYMINT.  By continuing to use, modify, or distribute
| this file you indicate that you have read the license and
| understand and accept it fully.
|

|
| _setstack: changes the stack pointer; called as
|     void setstack( void *newsp )
| called from crtinit.c once the new stack size has been decided upon
|
| Note: It is strictly forbidden to read or write data at *newsp or beyond.
| Unfortunately, there are known bugs in the trap #1 handlers of most
| operating systems. For example, older versions of the MiNT kernel
| reads unconditionally 36 bytes from the user stack on any trap #1, even
| if there are no parameters at all. So if *newsp is actually unreadable
| (end of RAM, MMU protected...), the program will crash on the first
| trap #1 call.
| As a workaround, we can waste some space at the top of the new stack
| to ensure it will be readable by the buggy OS. 64 bytes should be enough.
|
| WARNING WARNING WARNING: after you do this, local variables may no longer
| be accessible!
| destroys a0 and a7

	.globl	__setstack
__setstack:
	movel	sp@+, a1	| save return address
#ifndef __FASTCALL__
	movel	sp@, a0		| new stack pointer
#endif
	movel	a0, sp		| new stack pointer
	subl	#64+4, sp	| push some unused space for buggy OS and a
				| fake parameter to be popped by the caller
	jmp	a1@		| back to caller
