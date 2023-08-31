/* These are predefined by new versions of GNU cpp.  */

#ifndef __USER_LABEL_PREFIX__
#define __USER_LABEL_PREFIX__ _
#endif

#ifndef __REGISTER_PREFIX__
#define __REGISTER_PREFIX__
#endif

#ifndef __IMMEDIATE_PREFIX__
#define __IMMEDIATE_PREFIX__ #
#endif

/* ANSI concatenation macros.  */

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

/* Use the right prefix for global labels.  */

#ifdef __ASSEMBLER__

#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)

#ifdef __ELF__
#define FUNC(name) .type SYM(name),function; 
#define END(name) .size SYM(name),.-SYM(name)
#else
/* The .proc pseudo-op is accepted, but ignored, by GAS.  We could just
   define this to the empty string for non-ELF systems, but defining it
   to .proc means that the information is available to the assembler if
   the need arises.  */
#define FUNC(name) .proc
#define END(name)
#endif

/* Use the right prefix for registers.  */

#define REG(x) CONCAT1 (__REGISTER_PREFIX__, x)

/* Use the right prefix for immediate values.  */

#define IMM(x) CONCAT1 (__IMMEDIATE_PREFIX__, x)

#define d0 REG(d0)
#define d1 REG(d1)
#define d2 REG(d2)
#define d3 REG(d3)
#define d4 REG(d4)
#define d5 REG(d5)
#define d6 REG(d6)
#define d7 REG(d7)
#define a0 REG(a0)
#define a1 REG(a1)
#define a2 REG(a2)
#define a3 REG(a3)
#define a4 REG(a4)
#define a5 REG(a5)
#define a6 REG(a6)
#define a7 REG(a7)
#define fp REG(fp)
#define sp REG(sp)
#define pc REG(pc)
#define sr REG(sr)

#define fp0 REG(fp0)
#define fp1 REG(fp1)
#define fp2 REG(fp2)
#define fp3 REG(fp3)
#define fp4 REG(fp4)
#define fp5 REG(fp5)
#define fp6 REG(fp6)
#define fp7 REG(fp7)
#define fpcr REG(fpcr)
#define fpsr REG(fpsr)
#define fpiar REG(fpiar)

#ifdef __GCC_HAVE_DWARF2_CFI_ASM
#define CFI_STARTPROC()		.cfi_startproc
#define CFI_OFFSET(reg,off)	.cfi_offset	reg,off
#define CFI_DEF_CFA(reg,off)	.cfi_def_cfa	reg,off
#define CFI_ENDPROC()		.cfi_endproc
#else
#define CFI_STARTPROC()
#define CFI_OFFSET(reg,off)
#define CFI_DEF_CFA(reg,off)
#define CFI_ENDPROC()
#endif

#else /* !__ASSMBLER__ */

#define __SYMBOL_PREFIX __STRINGIFY(__USER_LABEL_PREFIX__)
#define SYM(name) __SYMBOL_PREFIX #name

#endif
