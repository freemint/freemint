/* Global Variables
 */

# ifndef _global_h
# define _global_h

# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <mintbind.h>
# include <sys/types.h>

# include "fs.h"


# define INLINE		inline
# define FASTFN		static inline


extern char lfname [];

/* General linked list structure
 */
typedef struct linked_list
{
	long member;
	struct linked_list *next;
} llist;

/* Structure for determining inode names
 */
typedef struct inlist
{
	ushort inum;		/* Inode number */
	ushort iparent;		/* Parent directory */
	struct inlist *next;	/* pointer to next entry */
	char name[1];		/* Actually longer than this ... */
} ilist;

extern char *drvnam;

extern char version;		/* Non-zero for V2 */

extern char modified;
extern char preen;
extern char no_size;

extern ilist *inolist;

extern llist *inums;

extern llist *zinums;

extern long maxzone,minzone;	/* Max/Min Allowed Zone Numbers */
extern long maxino;		/* Max Inode Number */
extern long ioff;		/* Zone offset to inode block */

extern int incr;		/* Directory increment */

extern long berr;		/* Bitmap errors */

extern char badroot;		/* Bad root inode or forced reallocation */

extern long zonecount;		/* Maximum Zone Count */
extern long indcount;		/* Number of indirection blocks */
extern char dindcount;		/* Number of double indirection blocks */
extern char tindcount;		/* '1' If triple indirection block */

extern char flag_trunc,done_trunc;	/* Flags for truncation of files */

extern char quit_trav;

extern long cino;		/* current inode */

extern ushort *zbitmap;		/* Zone bitmap */
extern ushort *szbitmap;	/* Shadow zone bitmap */
extern ushort *ibitmap;		/* Inode Bitmap */

extern int cdirty;	

extern super_block *Super;

extern long ndir;
extern long nreg;
extern long nfifo;
extern long nchr;
extern long nblk;
extern long nsym;
extern long zfree;
extern long ifree;

extern char ally;
extern char alln;
extern char info;

extern ushort lfinode;


# endif /* _global_h */
