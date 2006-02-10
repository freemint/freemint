#ifndef _TYPES_
#define _TYPES_

/* 16.16 vorzeichenbehaftet */
typedef long	fixed;

/* Logische Werte */
typedef short	bool;

#ifndef NULL
#define	NULL	((void *)0)
#endif

#ifndef TRUE
#define	TRUE	1
#endif

#ifndef FALSE
#define	FALSE	0
#endif

#ifndef NIL
#define	NIL	((void *)-1)
#endif

#endif
