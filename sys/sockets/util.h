/*
 *	Prototypes and the like for functions that may be used anywhere.
 *
 *	10/22/93, kay roemer.
 */

# ifndef _util_h
# define _util_h


/* Unlimited amount of data for FIONREAD/FIONWRITE */
# define NO_LIMIT	0x7fffffffL

# define ALIGN2(x)	(((x) + 1) & ~1)
# define ALIGN4(x)	(((x) + 3) & ~3)


INLINE void *
setstack (register void *sp)
{
	register void *osp __asm__("d0") = 0;
	
	__asm__ volatile
	(
		"movel sp,%0;"
		"movel %2,sp;"
		: "=a" (osp)
		: "0" (osp), "a" (sp)
	);
	
	return osp;
}

extern char stack[8192];


# endif /* _util_h */
