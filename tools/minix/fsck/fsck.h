
# ifndef _fsck_h
# define _fsck_h

# include "global.h"
# include <time.h>


# define NSIZE	16	/* Size of big inode cache in blocks */


/* Inode status structure
 */
typedef struct
{
	ushort	flag;		/* Flags or'ed together */
# define I_FREE		0x01	/* Inode free */
# define I_DIR		0x02	/* Is a directory */
# define I_FDD		0x04	/* Fix '..' inode silently */
# define I_D		0x08	/* '.' present */
# define I_DD		0x10	/* '..' present */
# define I_FOUND	0x20	/* Has a parent */
# define I_LINK		0x40	/* Dir has a dir hard link in it */
# define I_FIXDD	0x80	/* Prompt for fixing '..' */
# define I_EMP		0x100	/* Dir contains free slots */
# define I_ZAP		0x200	/* Inode about to be cleared */
	long	links;		/* Number of links to this inode */
	ushort	parent; 	/* Inode of parent (from '..')*/
	
} inode_stat;


/* Structure for multiply allocated blocks
 */
typedef struct zlst
{
	zone_nr zone;		/* zones */
	ushort inum;		/* inode zone found on */
        ushort flag;		/* Status flag */
# define FOUND		0x1
# define REMOVE		0x2
# define IGNORE		0x4
	time_t	mod;		/* Modified time of zone */
	struct zlst *next;	/* Pointer to next entry */
	
} zlist;


void do_fsck1 (void);
void do_fsck2 (void);
	

# endif /* _fsck_h */
