/* Global Variables
 */

# include "global.h"


char lfname [] = "lost+found";


char *drvnam;

char version;		/* Non-zero for V2 */

char modified;
char preen;
char no_size;

ilist *inolist;

llist *inums;

llist *zinums;

long maxzone,minzone;	/* Max/Min Allowed Zone Numbers */
long maxino;		/* Max Inode Number */
long ioff;		/* Zone offset to inode block */

int incr;		/* Directory increment */

long berr;		/* Bitmap errors */

char badroot;		/* Bad root inode or forced reallocation */

long zonecount;		/* Maximum Zone Count */
long indcount;		/* Number of indirection blocks */
char dindcount;		/* Number of double indirection blocks */
char tindcount;		/* '1' If triple indirection block */

char truncated,done_trunc;	/* Flags for truncation of files */

char quit_trav;

long cino;		/* current inode */

ushort *zbitmap;	/* Zone bitmap */
ushort *szbitmap;	/* Shadow zone bitmap */
ushort *ibitmap;	/* Inode Bitmap */

int cdirty;	

super_block *Super;

long ndir;
long nreg;
long nfifo;
long nchr;
long nblk;
long nsym;
long zfree;
long ifree;

char ally;
char alln;
char info;

ushort lfinode;
