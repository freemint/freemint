/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _cnf_h
# define _cnf_h

# include "mint/mint.h"


extern const char *drv_list;

struct range
{
	short a, b;
};

// #define Range( a, b )   (((long)(a)<<16)|(int)(b))
#define Range(a,b)	{ range: {(short)a, (short)b} }

struct parsinf
{
	unsigned long opt;	/* options */
	/* */
	const char *file;	/* name of current cnf file   */
	int line;		/* current line in file       */
	/* */
	char *dst;		/* write to, NULL indicates an error */
	char *src;		/* read from                         */
	/* */
	void *data;		/* parser context data */
};

#define SET(opt) (1ul << ((opt) -'A'))

/*============================================================================*/
/* The parser uses callbacks to start actions. Using the same interfaces, both
 * commands  and  variable  assignments  differ  only  in the type entry in the
 * parser's   item  table  (see  below).   The  notation  of  the  interface's
 * declaration  names  starts  with  'PCB_',  followed by one ore more letters
 * for the argument type(s):
 * 'L' is a long integer, either decimal or hex, 'B' is a boolean containing 0
 * or 1,  ' D'  is  a comma  separated  drive list as bit mask, 'T' is a token
 * without  any  spaces  (e.g.  a path),  'A'  the  rest of the line including
 * possible spaces. 'A' can only be the last parameter.
 * Note  that  'T' and 'A' are the same parameter types but has been parsed in
 * different ways.
 * Types with a following 'x' get additional parser infornation.
 *
 * These callback types are available for the parser:
 */
typedef void	(PCB_L)   (long                                        );
typedef void	(PCB_Lx)  (long,                       struct parsinf *);
typedef void	(PCB_B)   (bool                                        );
typedef void	(PCB_Bx)  (bool,                       struct parsinf *);
typedef void	(PCB_D)   (unsigned long                               );
typedef void	(PCB_Dx)  (unsigned long,              struct parsinf *);
typedef void	(PCB_T)   (char *                                      );
typedef void	(PCB_A)   (char *                                      );
typedef void	(PCB_Tx)  (char *,                     struct parsinf *);
typedef void	(PCB_Ax)  (char *,                     struct parsinf *);
typedef void	(PCB_TT)  (const char *, const char *                  );
typedef void	(PCB_TA)  (const char *, const char *                  );
typedef void	(PCB_TTx) (const char *, const char *, struct parsinf *);
typedef void	(PCB_TAx) (const char *, const char *, struct parsinf *);
typedef void	(PCB_0TT) (int,          const char *, const char *    );
typedef void	(PCB_ATK) (const char *, const char *, long            );

/*============================================================================*/
/* The  parser's  item  table contains for every keyword (command/variable) one
 * entry  with  a key string, a type tag and a pointer to a callback. The type
 * tags  starts  all  with 'PI_' but differs then in 'C_' for commands and 'V_'
 * and  ' R_'  for variable equations. While 'R_' means that the ptr reffers a
 * variable  that  will be written directly, the other both means that the ptr
 * refers a callback.
 * The  following part (e.g. 'L', 'TA') are the same as of the interface types
 * (see above) and has to correspond with in each table entry (expect of 'x').
 *
 * These tag types are defined:
 */
enum pitype
{
	PI_C__   = 0x0000, /* --- command callbacks */
	PI_C_L   = 0x0002, /*     command gets long */
	PI_C_B   = 0x0003, /*     command gets bool */
	PI_C_T   = 0x0004, /*     command gets path (or token) e.g. cd */
	PI_C_TT  = 0x0044, /*     command gets two pathes */
	PI_C_0TT = 0x0144, /*     command gets zero and two pathes */
	PI_C_A   = 0x0005, /*     command gets line as string, e.g. echo */
	PI_C_TA  = 0x0054, /*     command gets path and line, e.g. exec */
	PI_C_D   = 0x0006, /*     command gets drive list */
	PI_V__   = 0x1000, /* --- variable callbacks */
	PI_V_L   = 0x1002, /*     variable gets long */
	PI_V_B   = 0x1003, /*     variable gets bool */
	PI_V_T   = 0x1004, /*     variable gets path */
	PI_V_A   = 0x1005, /*     variable gets line */
	PI_V_ATK = 0x1254, /*     variable gets path, line plus const */
	PI_V_D   = 0x1006, /*     variable gets drive list */
	PI_R__   = 0x3000, /* --- references */
	PI_R_S   = 0x3001, /*     reference gets short */
	PI_R_L   = 0x3002, /*     reference gets long */
	PI_R_B   = 0x3003, /*     reference gets bool */
	PI_R_T   = 0x3004, /*     reference gets path */
	PI_R_US  = 0x3007, /*     reference get ushort */
};


struct parser_item
{
	const char *key;
	enum pitype type;
	void *cb;
	union {
		struct range range;
		long dat;
	} dat;
// 	long dat;
};


# if __KERNEL__ == 1

void parser_msg(struct parsinf *, const char *msg);

long parse_include(const char *path, struct parsinf *, struct parser_item *);
long parse_cnf(const char *path, struct parser_item *, void *);

# endif

# endif /* _cnf_h */
