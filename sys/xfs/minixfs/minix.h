/*
 * This file is part of 'minixfs' Copyright 1991,1992,1993 S.N.Henson
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _minix_h
# define _minix_h

# include <mint/ktypes.h>


# define DOM_TOS	0
# define DOM_MINT	1


/* Useful macro, is non zero only if 'x' is not a power of two
 */
# define NPOW2(x)	(((x) & (x - 1)) != 0)

/* Filename translation modes
 */
# define SRCH_TOS	0x01	/* search with tosify, tos domain  */
# define SRCH_MNT	0x02	/* search with tosify, mint domain */
# define DIR_TOS	0x04	/* dir compat tosify, tos domain  */
# define DIR_MNT	0x08	/* dir compat tosify, mint domain */
# define LWR_TOS	0x10	/* lower case creat, tos domain  */
# define LWR_MNT	0x20	/* lower case creat, mint domain */
# define AEXEC_TOS	0x40	/* auto 'x', tos domain.   */
# define AEXEC_MNT	0x80	/* auto 'x', mint domain. */

/* Macro to determine maximum filename length for a given increment
 */
# define MMAX_FNAME(x)	(((x) << 4) - 2)


# define BLOCK_SIZE	1024L		/* # bytes in a disk block */
# define L_BS		10L		/* log 2 bytes/block */


/* Flag bits for i_mode in the inode
 */

# define I_OLDLINK		0160000	/* old symbolic link mode */

# define I_SYMLINK		0120000	/* symbolic link (Linux compatible) */
# define I_TYPE			0170000	/* this field gives inode type */
# define I_REGULAR		0100000	/* regular file, not dir or special */
# define I_BLOCK_SPECIAL	0060000	/* block special file */
# define I_DIRECTORY		0040000	/* file is a directory */
# define I_CHAR_SPECIAL		0020000	/* character special file */
# define I_NAMED_PIPE		0010000	/* named pipe (FIFO) */
# define I_SET_UID_BIT		0004000	/* set effective uid on exec */
# define I_SET_GID_BIT		0002000	/* set effective gid on exec */
# define I_STICKY		0001000	/* sticky bit */
# define ALL_MODES		0007777	/* all bits for user, group and others */
# define RWX_MODES		0000777	/* mode bits for RWX only */
# define R_BIT			0000004	/* Rwx protection bit */
# define W_BIT			0000002	/* rWx protection bit */
# define X_BIT			0000001	/* rwX protection bit */
# define I_NOT_ALLOC		0000000	/* this inode is free */

/* Useful macros
 */
# define IS_DIR(m)		((m.i_mode & I_TYPE) == I_DIRECTORY)
# define IS_REG(m)		((m.i_mode & I_TYPE) == I_REGULAR)
# define IS_SYM(m)		((m.i_mode & I_TYPE) == I_SYMLINK)

# define IM_SPEC(m)		(((m & I_TYPE) == I_CHAR_SPECIAL) || ((m & I_TYPE) == I_BLOCK_SPECIAL))

/* Flag bits for cookie 'aux' field
 */
# define AUX_DEV	0x1f	/* Mask for bios device */
# define AUX_DRV	0x20	/* First 5 bits are bios dev mask */
# define AUX_DEL	0x40	/* file marked for deletion */
# define AUX_SYNC	0x80	/* l_sync() on next write */


/* Miscellaneous constants
 */
# define SUPER_MAGIC	0x137F	/* magic number contained in super-block */
# define SUPER_V1_30	0x138f	/* magic number for Linux+V1+30chars */
# define SUPER_V2	0x2468	/* v2 magic number */

# define MINIX_VALID_FS	0x0001	/* Clean fs */
# define MINIX_ERROR_FS	0x0002	/* fs has errors */

# define SYMLINK_NAME_MAX	1023	/* Maximum size of symlink name */

# define MINIX_LINK_MAX		250	/* Max links to an inode *MUST* be < 256 */
# define MINIX2_LINK_MAX	65530	/* Max links to an inode *MUST* be < 65536 */


# define FIND		0	/* tells search_dir to search for file */
# define ADD		1	/* tells search_dir to add a dir entry */
# define KILL		2	/* tells search_dir to kill entry	 */
# define POS		3	/* tells search_dir to find position   */

# define BOOT_BLOCK  	0	/* block number of boot block */
# define SUPER_BLOCK 	1	/* block number of super block */
# define ROOT_INODE	1	/* inode number for root directory */


/* Derived sizes
 */
# define NR_ZONE_NUMS	   	9				/* # zone numbers in an inode */
# define NR_DZONE_NUM		(NR_ZONE_NUMS - 2)		/* # zones in inode */
# define ZONE_NUM_SIZE		(sizeof (ushort))		/* # bytes in zone nr */
# define DIR_ENTRY_SIZE		(sizeof (dir_struct))		/* # bytes/dir entry */
# define L_DIR_SIZE		4				/* log2 bytes/dir entry */	
# define INODES_PER_BLOCK	(BLOCK_SIZE / INODE_SIZE)	/* # inodes/disk blk */
# define L_IPB			5				/* log2 inodes/blk */
# define INODE_SIZE		(sizeof (d_inode1))		/* bytes in disk inode */
# define NR_DIR_ENTRIES		(BLOCK_SIZE / DIR_ENTRY_SIZE)	/* # dir entries/block */
# define NR_INDIRECTS		(BLOCK_SIZE / ZONE_NUM_SIZE)	/* # zones/indir block */
# define LNR_IND		9				/* log 2 NR_INDIRECTS */
# define NR_DBL			(NR_DZONE_NUM + NR_INDIRECTS)	/* 1st zone in dbl ind */
# define INTS_PER_BLOCK		(BLOCK_SIZE / sizeof (int))	/* # integers/block */
# define SUPER_SIZE		(sizeof (struct super_block))	/* super_block size */
# define PIPE_SIZE		(NR_DZONE_NUM * BLOCK_SIZE)	/* pipe size in bytes */
# define MAX_ZONES		(NR_DZONE_NUM + (NR_INDIRECTS + 1l) * NR_INDIRECTS)

# define NR_ZONE_NUMS2		10
# define NR_DZONE_NUM2		(NR_ZONE_NUMS2 - 3)
# define ZONE_NUM_SIZE2		(sizeof (long))
# define INODES_PER_BLOCK2	(BLOCK_SIZE / INODE_SIZE2)
# define L_IPB2			4
# define INODE_SIZE2		(sizeof (d_inode))
# define NR_INDIRECTS2		(BLOCK_SIZE / ZONE_NUM_SIZE2)
# define LNR_IND2		8
# define NR_DBL2		(NR_DZONE_NUM2 + NR_INDIRECTS2)
# define MAX_ZONES2		(NR_DZONE_NUMS2 + ((NR_INDIRECTS2 + 1l) * NR_INDIRECTS2 + 1l) * NR_INDIRECTS2)


typedef struct super_block SB;
struct super_block
{
	ushort	s_ninodes;	/* # usable inodes on the minor device */
	ushort	s_nzones;	/* total device size, including bit maps etc */
	ushort	s_imap_blks;	/* # of blocks used by inode bit map */
	ushort	s_zmap_blks;	/* # of blocks used by zone bit map */
	ushort	s_firstdatazn;	/* number of first data zone */
	ushort	s_log_zsize;	/* log2 of blocks/zone */
	ulong	s_max_size;	/* maximum file size on this device */
	ushort	s_magic;	/* magic number to recognize super-blocks */
	ushort	s_state;	/* filesystem state */
	ulong	s_zones;	/* long version of s_nzones for v2 */
};


/* super_info contains information about each Minix filesystem
 */
typedef struct super_info SI;
struct super_info
{
	DI	*di;	/* device identifikator for this device */
	SB	*sblk;	/* ptr to our super block */
	ulong	s_flags;
	
	ushort	dev;	/* device this belongs to */
	ushort	res;	/* alignment */
	
	long	serial;	/* Serial number of disk (ignored for now)*/
	long	ioff;	/* Offset to inode 1 */
	
	ushort	*ibitmap;
	ushort	*zbitmap;
	
	ushort	idirty;	/* ibitmap dirty flag */
	ushort	zdirty;	/* zbitmap dirty flag */
	
	long	ilast;	/* search start for free inodes */
	long	zlast;	/* search start for free zones */
	
	UNIT	*sunit;	/* actual super block */
	
	/* This lot is filled in as appropriate for each FS type */
	ushort	ipb;	/* Inodes per block */
	ushort	zpind;	/* zones per indirection block */
	ushort	dzpi;	/* direct zones per inode */
	ushort	ndbl;	/* first zone number in double indirect block */
	short	incr;	/* num of dir_structs per dir entry */
	short	mfname;	/* max file name */
};

/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
# define MS_RDONLY		   1	/* Mount read-only */
# define MS_NOSUID		   2	/* Ignore suid and sgid bits */
# define MS_NODEV		   4	/* Disallow access to device special files */
# define MS_NOEXEC		   8	/* Disallow program execution */
# define MS_SYNCHRONOUS		  16	/* Writes are synced at once */
# define MS_REMOUNT		  32	/* Alter flags of a mounted FS */
# define MS_MANDLOCK		  64	/* Allow mandatory locks on an FS */
# define S_QUOTA		 128	/* Quota initialized for file/directory/symlink */
# define S_APPEND		 256	/* Append-only file */
# define S_IMMUTABLE		 512	/* Immutable file */
# define MS_NOATIME		1024	/* Do not update access times. */
# define MS_NODIRATIME		2048    /* Do not update directory access times */
# define S_NOT_CLEAN_MOUNTED	4096	/* not cleanly mounted */

/* This is what a directory entry on the disk looks like. Note: we can use
 * a dirty trick to use this same structure for large filenames > 14 chars
 * the idea is to use only a fraction of the total entries , so that if
 * say the filename size is 30 we just use entries 0,2,4,6,8 etc. d_name
 * then occupies all of the next entry. This forces the max filename size
 * to be 2 less than a power of two (and certainly less than 1022), normally
 * 30 should be more than adequate to cover every filename you'll ever see.
 * 62 is for paranoids, but remember the path name limit of 128 characters.
 */

/* directory entry
 */
typedef struct dir_struct dir_struct;
struct dir_struct
{
	ushort	d_inum;			/* inode number */
	char	d_name[MMAX_FNAME(1)];	/* character string */
};


/* V2.x disk inode
 */
typedef struct d_inode d_inode;
struct d_inode
{
	ushort	i_mode;			/* file type, protection, etc. */
	ushort	i_nlinks;		/* how many links to this file. HACK! */
	ushort	i_uid;			/* user id of the file's owner. */
	ushort	i_gid;			/* group number HACK! */
	ulong	i_size;			/* current file size in bytes */
	ulong	i_atime;		/* when was file data last accessed */
	ulong	i_mtime;		/* when was file data last changed */
	ulong	i_ctime;		/* when was inode data last changed */
	ulong	i_zone[NR_ZONE_NUMS2];	/* block nums for direct, ind, and dbl ind */
};


typedef union bufr bufr;
union bufr
{
	char		bdata[BLOCK_SIZE];		/* ordinary user data */
	dir_struct	bdir[NR_DIR_ENTRIES];		/* directory block */
	long		bind[NR_INDIRECTS2];		/* indirect block */
	d_inode		binode[INODES_PER_BLOCK2];	/* inode block */
};


/* This is a special FILEPTR structure, it is pointed to by the devinfo field,
 * this speeds up read and write. 
 *	For write, 'zones' contains only the current writing block, which
 * is used if lots of small writes take place. For reading it contains a list
 * of the next few zones to read. Care is needed as an O_TRUNC (or a truncate
 * which  nothing uses at  present) can invalidate all  of this.  lckfirst
 * contains a pointer to where a pointer to the first lock is contained. This
 * means that if the first lock is deleted then only *lckfirst need be altered.
 */

typedef struct f_cache f_cache;
struct f_cache
{
	long	zones[PRE_READ];	/* Zonecache for pre-read,write */
	long 	fzone;			/* chunk number in zone[0] */
	long	lzone;			/* Last valid chunk number */
	LOCK 	**lfirst;		/* pointer to pointer with first lock */
};


/* Macros for indirection blocks
 */
# define PIND(tmp, index)	(tmp->bind[index])
# define IND(temp, index)	(temp.bind[index])


# endif /* _minix_h */
