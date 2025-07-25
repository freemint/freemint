/*
 * Filename:     ext2.h
 * Project:      ext2 file system driver for MiNT
 *
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 *
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
 *
 * Portions copyright 1992, 1993, 1994, 1995 Remy Card (card@masi.ibp.fr)
 * and 1991, 1992 Linus Torvalds (torvalds@klaava.helsinki.fi)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# ifndef _ext2_h
# define _ext2_h

# include "global.h"

/*
 * The second extended filesystem constants/structures
 */

/*
 * Define EXT2_PREALLOCATE to preallocate data blocks for expanding files
 */
# define EXT2_PREALLOCATE
# define EXT2_DEFAULT_PREALLOC_BLOCKS	8

/*
 * The second extended file system version
 */
# define EXT2FS_DATE		"95/08/09"
# define EXT2FS_VERSION		"0.5b"

/*
 * Debug code
 */
# ifdef EXT2FS_DEBUG
# define ext2_debug(f, a...)	{ \
					printk ("EXT2-fs DEBUG (%s, %d): %s:", \
						__FILE__, __LINE__, __FUNCTION__); \
				  	printk (f, ## a); \
					}
# else
# define ext2_debug(f, a...)	/**/
# endif

/*
 * Special inodes numbers
 */
# define EXT2_BAD_INO			1	/* Bad blocks inode */
# define EXT2_ROOT_INO			2	/* Root inode */
# define EXT2_ACL_IDX_INO		3	/* ACL inode */
# define EXT2_ACL_DATA_INO		4	/* ACL inode */
# define EXT2_BOOT_LOADER_INO		5	/* Boot loader inode */
# define EXT2_UNDEL_DIR_INO		6	/* Undelete directory inode */

/* First non-reserved inode for old ext2 filesystems */
# define EXT2_GOOD_OLD_FIRST_INO	11

/*
 * The second extended file system magic number
 */
# define EXT2_SUPER_MAGIC		0xEF53

/*
 * Maximal count of links to a file
 */
# define EXT2_LINK_MAX			32000

/*
 * Macro-instructions used to manage several block sizes
 */
# define EXT2_MIN_BLOCK_SIZE		(1024UL)
# define EXT2_MAX_BLOCK_SIZE		(4096UL)
# define EXT2_MIN_BLOCK_LOG_SIZE	(10)
# define EXT2_BLOCK_SIZE(s)		(EXT2_SB(s)->s_blocksize)
// # define EXT2_BLOCK_SIZE(s)		((s)->sbi.s_blocksize)
# define EXT2_BLOCK_SIZE_BITS(s)	(EXT2_SB(s)->s_blocksize_bits)
# define EXT2_BLOCK_SIZE_MASK(s)	(EXT2_SB(s)->s_blocksize_mask)
# define EXT2_ACLE_PER_BLOCK(s)		(EXT2_BLOCK_SIZE(s) / sizeof (struct ext2_acl_entry))
# define EXT2_ADDR_PER_BLOCK(s)		(EXT2_SB(s)->s_addr_per_block)
# define EXT2_ADDR_PER_BLOCK_BITS(s)	(EXT2_SB(s)->s_addr_per_block_bits)
# define EXT2_ADDR_PER_BLOCK_MASK(s)	(EXT2_ADDR_PER_BLOCK(s) - 1)
# define EXT2_INODE_SIZE(s)		(EXT2_SB(s)->s_inode_size)
# define EXT2_FIRST_INO(s)		(EXT2_SB(s)->s_first_ino)

/*
 * Macro-instructions used to manage fragments
 */
# define EXT2_MIN_FRAG_SIZE		(1024UL)
# define EXT2_MAX_FRAG_SIZE		(4096UL)
# define EXT2_MIN_FRAG_LOG_SIZE		(10)
# define EXT2_FRAG_SIZE(s)		(EXT2_SB(s)->s_fragsize)
# define EXT2_FRAGS_PER_BLOCK(s)	(EXT2_SB(s)->s_frags_per_block)

/*
 * ACL structures
 */
struct ext2_acl_header			/* Header of Access Control Lists */
{
	__u32	aclh_size;
	__u32	aclh_file_count;
	__u32	aclh_acle_count;
	__u32	aclh_first_acle;
};

struct ext2_acl_entry			/* Access Control List Entry */
{
	__u32	acle_size;
	__u16	acle_perms;		/* Access permissions */
	__u16	acle_type;		/* Type of entry */
	__u16	acle_tag;		/* User or group identity */
	__u16	acle_pad1;
	__u32	acle_next;		/* Pointer on next entry for the
					 * same inode or on next free entry */
};

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	__u32	bg_block_bitmap;	/* Blocks bitmap block */
	__u32	bg_inode_bitmap;	/* Inodes bitmap block */
	__u32	bg_inode_table;		/* Inodes table block */
	__u16	bg_free_blocks_count;	/* Free blocks count */
	__u16	bg_free_inodes_count;	/* Free inodes count */
	__u16	bg_used_dirs_count;	/* Directories count */
	__u16	bg_pad;
	__u32	bg_reserved[3];
};

/*
 * Macro-instructions used to manage group descriptors
 */
# define EXT2_BLOCKS_PER_GROUP(s)	(EXT2_SB(s)->s_blocks_per_group)
# define EXT2_INODES_PER_GROUP(s)	(EXT2_SB(s)->s_inodes_per_group)
# define EXT2_DESC_PER_BLOCK(s)		(EXT2_SB(s)->s_desc_per_block)
# define EXT2_DESC_PER_BLOCK_BITS(s)	(EXT2_SB(s)->s_desc_per_block_bits)
# define EXT2_DESC_PER_BLOCK_MASK(s)	(EXT2_SB(s)->s_desc_per_block_mask)

/*
 * Constants relative to the data blocks
 */
# define EXT2_NDIR_BLOCKS		(12UL)
# define EXT2_IND_BLOCK			(EXT2_NDIR_BLOCKS)
# define EXT2_DIND_BLOCK		(EXT2_IND_BLOCK + 1)
# define EXT2_TIND_BLOCK		(EXT2_DIND_BLOCK + 1)
# define EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Inode flags
 */
# define EXT2_SECRM_FL			0x00000001 /* Secure deletion */
# define EXT2_UNRM_FL			0x00000002 /* Undelete */
# define EXT2_COMPR_FL			0x00000004 /* Compress file */
# define EXT2_SYNC_FL			0x00000008 /* Synchronous updates */
# define EXT2_IMMUTABLE_FL		0x00000010 /* Immutable file */
# define EXT2_APPEND_FL			0x00000020 /* writes to file may only append */
# define EXT2_NODUMP_FL			0x00000040 /* do not dump file */
# define EXT2_NOATIME_FL		0x00000080 /* do not update atime */
/* Reserved for compression usage... */
# define EXT2_DIRTY_FL			0x00000100
# define EXT2_COMPRBLK_FL		0x00000200 /* One or more compressed clusters */
# define EXT2_NOCOMP_FL			0x00000400 /* Don't compress */
# define EXT2_ECOMPR_FL			0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
# define EXT2_BTREE_FL			0x00001000 /* btree format dir */
# define EXT2_INDEX_FL			0x00001000 /* hash-indexed directory */
# define EXT2_IMAGIC_FL			0x00002000
# define EXT3_JOURNAL_DATA_FL	0x00004000 /* file data should be journaled */
# define EXT2_NOTAIL_FL			0x00008000 /* file tail should not be merged */
# define EXT2_DIRSYNC_FL 		0x00010000 /* Synchronous directory modifications */
# define EXT2_TOPDIR_FL			0x00020000 /* Top of directory hierarchies*/
# define EXT4_HUGE_FILE_FL               0x00040000 /* Set to each huge file */
# define EXT4_EXTENTS_FL 		0x00080000 /* Inode uses extents */
# define EXT4_EA_INODE_FL	        0x00200000 /* Inode used for large EA */
# define EXT4_EOFBLOCKS_FL		0x00400000 /* Blocks allocated beyond EOF */
# define EXT4_SNAPFILE_FL		0x01000000  /* Inode is a snapshot */
# define EXT4_SNAPFILE_DELETED_FL	0x04000000  /* Snapshot is being deleted */
# define EXT4_SNAPFILE_SHRUNK_FL		0x08000000  /* Snapshot shrink has completed */
# define EXT2_RESERVED_FL		0x80000000 /* reserved for ext2 lib */

# define EXT2_FL_USER_VISIBLE		0x004BDFFF /* User visible flags */
# define EXT2_FL_USER_MODIFIABLE	0x004B80FF /* User modifiable flags */


# if 0 /* moved to our global dcntl.h */
/*
 * ioctl commands
 */
# define EXT2_IOC_GETFLAGS		_IOR('f', 1UL, ulong)
# define EXT2_IOC_SETFLAGS		_IOW('f', 2UL, ulong)
# define EXT2_IOC_GETVERSION		_IOR('v', 1UL, ulong)
# define EXT2_IOC_SETVERSION		_IOW('v', 2UL, ulong)
# endif


/*
 * Structure of an inode on the disk
 */
struct ext2_inode
{
	__u16	i_mode;			/* File mode */
	__u16	i_uid;			/* Owner Uid */
	__u32	i_size;			/* Size in bytes */
	__u32	i_atime;		/* Access time */
	__u32	i_ctime;		/* Creation time */
	__u32	i_mtime;		/* Modification time */
	__u32	i_dtime;		/* Deletion Time */
	__u16	i_gid;			/* Group Id */
	__u16	i_links_count;		/* Links count */
	__u32	i_blocks;		/* Blocks count */
	__u32	i_flags;		/* File flags */
	union {
		struct {
			__u32  l_i_reserved1;
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
		struct {
			__u32  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	__u32	i_block[EXT2_N_BLOCKS];	/* Pointers to blocks */
	__u32	i_version;		/* File version (for NFS) */
	__u32	i_file_acl;		/* File ACL */
	__u32	i_dir_acl;		/* Directory ACL */
	__u32	i_faddr;		/* Fragment address */
	union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__u16	i_pad1;
			__u32	l_i_reserved2[2];
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

# define i_size_high	i_dir_acl

# if defined(__linux__) || defined(__MINT__)
# define i_reserved1	osd1.linux1.l_i_reserved1
# define i_frag		osd2.linux2.l_i_frag
# define i_fsize	osd2.linux2.l_i_fsize
# define i_reserved2	osd2.linux2.l_i_reserved2
# endif

# ifdef	__hurd__
# define i_translator	osd1.hurd1.h_i_translator
# define i_frag		osd2.hurd2.h_i_frag;
# define i_fsize	osd2.hurd2.h_i_fsize;
# define i_uid_high	osd2.hurd2.h_i_uid_high
# define i_gid_high	osd2.hurd2.h_i_gid_high
# define i_author	osd2.hurd2.h_i_author
# endif

# ifdef	__masix__
# define i_reserved1	osd1.masix1.m_i_reserved1
# define i_frag		osd2.masix2.m_i_frag
# define i_fsize	osd2.masix2.m_i_fsize
# define i_reserved2	osd2.masix2.m_i_reserved2
# endif

/*
 * File system states
 */
# define EXT2_VALID_FS			0x0001	/* Unmounted cleanly */
# define EXT2_ERROR_FS			0x0002	/* Errors detected */

/*
 * Mount flags
 */
# define EXT2_MOUNT_CHECK_NORMAL	0x0001	/* Do some more checks */
# define EXT2_MOUNT_CHECK_STRICT	0x0002	/* Do again more checks */
# define EXT2_MOUNT_CHECK		(EXT2_MOUNT_CHECK_NORMAL | EXT2_MOUNT_CHECK_STRICT)
# define EXT2_MOUNT_GRPID		0x0004	/* Create files with directory's group */
# define EXT2_MOUNT_DEBUG		0x0008	/* Some debugging messages */
# define EXT2_MOUNT_ERRORS_CONT		0x0010	/* Continue on errors */
# define EXT2_MOUNT_ERRORS_RO		0x0020	/* Remount fs ro on errors */
# define EXT2_MOUNT_ERRORS_PANIC	0x0040	/* Panic on errors */
# define EXT2_MOUNT_MINIX_DF		0x0080	/* Mimics the Minix statfs */

# define clear_opt(o, opt)		(o &= ~EXT2_MOUNT_##opt)
# define set_opt(o, opt)		(o |= EXT2_MOUNT_##opt)
# define test_opt(sb, opt)		(EXT2_SB (s)->s_mount_opt & EXT2_MOUNT_##opt)

/*
 * Maximal mount counts between two filesystem checks
 */
# define EXT2_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
# define EXT2_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/*
 * Behaviour when detecting errors
 */
# define EXT2_ERRORS_CONTINUE		1	/* Continue execution */
# define EXT2_ERRORS_RO			2	/* Remount fs read-only */
# define EXT2_ERRORS_PANIC		3	/* Panic */
# define EXT2_ERRORS_DEFAULT		EXT2_ERRORS_CONTINUE

/*
 * Structure of the super block
 */
struct ext2_super_block
{
	__u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
	__u32	s_free_blocks_count;	/* Free blocks count */
	__u32	s_free_inodes_count;	/* Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
	__u32	s_log_block_size;	/* Block size */
	__s32	s_log_frag_size;	/* Fragment size */
	__u32	s_blocks_per_group;	/* # Blocks per group */
	__u32	s_frags_per_group;	/* # Fragments per group */
	__u32	s_inodes_per_group;	/* # Inodes per group */
	__u32	s_mtime;		/* Mount time */
	__u32	s_wtime;		/* Write time */
	__u16	s_mnt_count;		/* Mount count */
	__s16	s_max_mnt_count;	/* Maximal mount count */
	__u16	s_magic;		/* Magic signature */
	__u16	s_state;		/* File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level; 	/* minor revision level */
	__u32	s_lastcheck;		/* time of last check */
	__u32	s_checkinterval;	/* max. time between checks */
	__u32	s_creator_os;		/* OS */
	__u32	s_rev_level;		/* Revision level */
	__u16	s_def_resuid;		/* Default uid for reserved blocks */
	__u16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 *
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__u32	s_first_ino; 		/* First non-reserved inode */
	__u16   s_inode_size; 		/* size of inode structure */
	__u16	s_block_group_nr; 	/* block group # of this superblock */
	__u32	s_feature_compat; 	/* compatible feature set */
	__u32	s_feature_incompat; 	/* incompatible feature set */
	__u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	__u8	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	__u32	s_algorithm_use_bitmap;	/* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_padding1;
	__u32	s_reserved[204];	/* Padding to the end of the block */
};

/*
 * Codes for operating systems
 */
# define EXT2_OS_LINUX			0
# define EXT2_OS_HURD			1
# define EXT2_OS_MASIX			2
# define EXT2_OS_FREEBSD		3
# define EXT2_OS_LITES			4
# define EXT2_OS_MINT			5

/*
 * Revision levels
 */
# define EXT2_GOOD_OLD_REV		0	/* The good old (original) format */
# define EXT2_DYNAMIC_REV		1 	/* V2 format w/ dynamic inode sizes */

# define EXT2_CURRENT_REV		EXT2_GOOD_OLD_REV
# define EXT2_MAX_SUPP_REV		EXT2_DYNAMIC_REV

# define EXT2_GOOD_OLD_INODE_SIZE	128

/*
 * Feature set definitions
 */

# define EXT2_HAS_COMPAT_FEATURE(sb, mask)	(EXT2_SB(sb)->s_feature_compat & (mask))
# define EXT2_HAS_RO_COMPAT_FEATURE(sb, mask)	(EXT2_SB(sb)->s_feature_ro_compat & (mask))
# define EXT2_HAS_INCOMPAT_FEATURE(sb, mask)	(EXT2_SB(sb)->s_feature_incompat & (mask))

# define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001

# define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
# define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
# define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004

# define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
# define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002

# define EXT2_FEATURE_COMPAT_SUPP	0
# define EXT2_FEATURE_INCOMPAT_SUPP	EXT2_FEATURE_INCOMPAT_FILETYPE
# define EXT2_FEATURE_RO_COMPAT_SUPP	( EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	\
					| EXT2_FEATURE_RO_COMPAT_LARGE_FILE	\
					| EXT2_FEATURE_RO_COMPAT_BTREE_DIR	)

/*
 * Default values for user and/or group using reserved blocks
 */
# define EXT2_DEF_RESUID		0
# define EXT2_DEF_RESGID		0

/*
 * Structure of a directory entry
 */
# define EXT2_NAME_LEN 255

struct ext2_dir_entry
{
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u16	name_len;		/* Name length */
	char	name[EXT2_NAME_LEN];	/* File name */
};

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext2_dir_entry_2
{
	__u32	inode;			/* Inode number */
	__u16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	name[EXT2_NAME_LEN];	/* File name */
};

/*
 * Ext2 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
# define EXT2_FT_UNKNOWN	0
# define EXT2_FT_REG_FILE	1
# define EXT2_FT_DIR		2
# define EXT2_FT_CHRDEV		3
# define EXT2_FT_BLKDEV 	4
# define EXT2_FT_FIFO		5
# define EXT2_FT_SOCK		6
# define EXT2_FT_SYMLINK	7

# define EXT2_FT_MAX		8

/*
 * EXT2_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
# define EXT2_DIR_PAD		4
# define EXT2_DIR_ROUND 	(EXT2_DIR_PAD - 1)
# define EXT2_DIR_REC_LEN(len)	(((len) + 8 + EXT2_DIR_ROUND) & ~EXT2_DIR_ROUND)




typedef struct ext2_super_block	ext2_sb;
typedef struct ext2_group_desc	ext2_gd;
typedef struct ext2_inode	ext2_in;
typedef struct ext2_dir_entry	ext2_de;
typedef struct ext2_dir_entry_2	ext2_d2;

typedef struct ext2_sb_info	ext2_si;


# define EXT2_SB(s)		(&(s->sbi))


struct ext2_sb_info
{
	__u32	s_blocksize;		/* Size of a block in bytes */
	__u32	s_blocksize_bits;	/* Size of a block in bits */
	__u32	s_blocksize_mask;	/* bitmask */

	__u32	s_fragsize;		/* Size of a fragment in bytes */
	__u32	s_fragsize_bits;	/* Size of a fragment in bits */
	__u32	s_fragsize_mask;	/* bitmask */

	__u32	s_groups_count;		/* Number of groups in the fs */
	__u32	s_db_per_group;		/* Number of descriptor blocks per group */
	__u32	s_itb_per_group;	/* Number of inode table blocks per group */
	__u32	s_frags_per_block;	/* Number of fragments in a group */
	__u32	s_inodes_per_block;	/* Number of inodes per block */
	__u32	s_desc_per_block;	/* Number of group descriptors per block */
	__u32	s_desc_per_block_bits;	/* */
	__u32	s_desc_per_block_mask;	/* */
	__u32	s_addr_per_block;	/* */
	__u32	s_addr_per_block_bits;	/* */

	long	s_group_desc_size;	/* size of the ptr array */
	ext2_gd **s_group_desc;		/* ptr array to our group blocks (resident) */
	ext2_sb	*s_sb;			/* ptr to our superblock (resident) */

	UNIT	**s_group_desc_units;	/* the units for the group descriptors blocks */
	UNIT	*s_sb_unit;		/* the unit for the superblock */

	__u32	s_dirty;		/* dirty flag for super block */
	__u32	s_mount_opt;		/* mount options for this super block */

# if 0
	__u16	s_rename_lock;
	struct wait_queue *s_rename_wait;
# endif

	/* mirrored superblock in native endian
	 * nn - not neccessary
	 * u  - updateable (not mirrored)
	 */
	__u32	s_inodes_count;		/* Inodes count */
	__u32	s_blocks_count;		/* Blocks count */
	__u32	s_r_blocks_count;	/* Reserved blocks count */
/* u 	__u32	s_free_blocks_count;	 * Free blocks count */
/* u 	__u32	s_free_inodes_count;	 * Free inodes count */
	__u32	s_first_data_block;	/* First Data Block */
/* nn	__u32	s_log_block_size;	 * Block size */
/* nn	__s32	s_log_frag_size;	 * Fragment size */
	__u32	s_blocks_per_group;	/* # Blocks per group */
	__u32	s_frags_per_group;	/* # Fragments per group */
	__u32	s_inodes_per_group;	/* # Inodes per group */
/* u 	__u32	s_mtime;		 * Mount time */
/* u 	__u32	s_wtime;		 * Write time */
/* u 	__u16	s_mnt_count;		 * Mount count */
/* nn	__s16	s_max_mnt_count;	 * Maximal mount count */
/* nn	__u16	s_magic;		 * Magic signature */
/* u 	__u16	s_state;		 * File system state */
	__u16	s_errors;		/* Behaviour when detecting errors */
	__u16	s_minor_rev_level; 	/* minor revision level */
/* nn	__u32	s_lastcheck;		 * time of last check */
/* nn	__u32	s_checkinterval;	 * max. time between checks */
/* nn	__u32	s_creator_os;		 * OS */
	__u32	s_rev_level;		/* Revision level */
	__u16	s_resuid;		/* Default uid for reserved blocks */
	__u16	s_resgid;		/* Default gid for reserved blocks */

	/* These fields are for EXT2_DYNAMIC_REV superblocks only.
	 */
	__u32	s_first_ino; 		/* First non-reserved inode */
	__u16   s_inode_size; 		/* size of inode structure */
/* nn	__u16	s_block_group_nr; 	 * block group # of this superblock */
	__u32	s_feature_compat; 	/* compatible feature set */
	__u32	s_feature_incompat; 	/* incompatible feature set */
	__u32	s_feature_ro_compat; 	/* readonly-compatible feature set */
/* nn	__u32	s_algorithm_use_bitmap;	 * For compression */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
};


# endif /* _ext2_h */
