|
| Initialization code; this is common to both 16 and 32 bit libraries,
| so be careful!
|
| This is is *very* minimalistic version that does not do any argument
| parsing. Suitable for ACCs, and auto-folder programs.

| No __FASTCALL__ modifications required.

|
| The following external symbols are declared with .globl rather than
| .extern in order to ensure they will be linked into the executable
| even if they are not used in this file.

	.globl	__app		| short, defined in crtinit.c
	.globl	__base		| BASEPAGE *, defined in globals.c
	.globl	__stksize	| long, defined by user or in stksiz.c

|
| externs to pull ident strings of all used libraries into the
| executable; if a library is not used, then the extern is
| satisfied by a dummy in the library

| nobody needs this
|	.globl	___Ident_gnulib
|	.globl	___Ident_socketlib
|	.globl	___Ident_gem

|
| Functions defined elsewhere.

	.extern	__crtinit_noargs	| void _crtinit(void);

|
| Assumption: basepage is passed in a0 for accessories; for programs
| a0 is always 0.

	.text
	.even
#ifdef __ELF__
	.section	.text.entry.mint,"ax",@progbits
#endif /* __ELF__ */
	.globl	__start
__start:
	subl	a6, a6		| clear a6 for debuggers
	cmpl	#0, a0		| test if acc or program
	beqs	__startprg	| if a program, go elsewhere
	movel	a0, __base	| acc basepage is in A0
	lea	a0@(252), sp	| use the command line as a temporary stack
	jmp	__acc_main	| function is in accmain.c

|
| program startup code: doesn''t actually do much, other than push
| the basepage onto the stack and call _start1 in crtinit.c
|
__startprg:
	movel	sp@(4), a0	| get basepage
	movel	a0, __base	| save it
	movel	a0@(4), d0	| get _base->p_hitpa
	bclr	#0, d0		| round off
	movel	d0, sp		| set stack (temporarily)
	subl	#64, sp		| see note in mintlib/setstack.S
	jmp	__crtinit_noargs	| in minimain.c
