# ext2.xfs

This is a full working Ext2 filesystem driver for FreeMiNT. The complete
project is copyrighted by the GNU Public License.

It can read and write the actual ext2 version as implemented in Linux
for example. The partition size is not limited and the logical sector
size can be 1024, 2048 or 4096 bytes. The only restriction is that
the physical sector size is smaller or equal to the logical sector
size. The blocksize can be configured if you initialize the partition
with mke2fs.

The next good thing is that it's a lot faster as the MinixFS. The Ext2
has a very good structure that is designed for maximal speed and avoids
also fragmentation automatically.

If you have any problems send me an e-mail or write to the MiNT list.
I'm very interested in feedback and suggestions.


## Requirements:
Ext2.xfs requires a XHDI compatible harddisk driver.
It also requires a recent FreeMiNT version (at least 1.15.1 beta 0.6).
The right version is checked on startup.

There are now two versions: ext2.xfs and ext2_st.xfs. The ext2.xfs
version requires at least a 68020 processor. If you have a 68000
processor you must use the ext2_st.xfs version.

It's your task to verify that you use the right version!

NOTE: The 68000 version isn't tested. That's why it's not included here!
      If you are interested to test the 68000 version and the 68000 e2fsprogs
      please contact me. I don't have a 68000 based ATARI and can't test
      it self.


## Tips & Tricks:
Make sure that your ext2-partition is recognised by the harddisk
driver. You can achieve this by either changing its partition ID to
e.g. RAW or by telling your driver to serve the current partition
name (e.g. LNX).

The default cache of the new buffer cache in FreeMiNT is small. To
get a better performance of the Ext2.xfs increase the buffer cache
(I suggest at least a size of 500kb as minimum).

Also use the latest e2fsprogs (version 1.14). These version work
much better as the previous version 1.10 from Axel.


## Installation:
Copy ext2.xfs into the directory of yout boot partition
which contains minix.xfs (e.g. c:\multitos) and restart your computer.

Look in the FreeMiNT 1.15.1 release archive for an example script
to automatically check your filesystems at boottime!


## WARNING:
BE CAREFUL WHEN USING THIS PROGRAM. I, THE AUTHOR, CAN'T TAKE ANY
GARANTY THAT YOU WILL NOT LOOSE ALL OR ANY OF OUR PRECIOUS DATA WHEN
USING THIS PROGRAM. BACKUP OFTEN, BACKUP QUICK.

## Features from ext4 filesystems

These are features that were introduced in later versions of the filesystem,
and are currently not supported by this driver.

### Compatible Features:

These do not affect the kernels ability to mount the filesystem.

- dir_index  
Use hashed b-trees to speed up name lookups in large directories.

- dir_nlink  
Normally, ext4 allows an inode to have no more than 65,000 hard links.
This applies to regular files as well as directories, which means that
there can be no more than 64,998 subdirectories in a directory (because
each of the '.' and '..' entries, as well as the directory entry for the
directory in its parent directory counts as a hard link).  This feature
lifts this limit by causing ext4 to use a link count of 1 to indicate
that the number of hard links to a directory is not known when the link
count might exceed the maximum count limit.

- ext_attr  
This feature enables the use of extended attributes.  This feature is
supported by ext2, ext3, and ext4.

- has_journal  
Create a journal to ensure file system consistency even across unclean
shutdowns.

- resize_inode  
This file system feature indicates that space has been reserved so that
the block group descriptor table can be extended while resizing a mounted
file system.  The online resize operation
is carried out by the kernel, triggered by
**resize2fs (8)**.
By default **mke2fs** will attempt to reserve enough space so that the
file system may grow to 1024 times its initial size.

- sparse_super2  
This feature indicates that there will only be at most two backup
superblocks and block group descriptors.  The block groups used to store
the backup superblock(s) and blockgroup descriptor(s) are stored in the
superblock, but typically, one will be located at the beginning of block
group #1, and one in the last block group in the file system.  This
feature is essentially a more extreme version of sparse_super and is
designed to allow a much larger percentage of the disk to have
contiguous blocks available for data files.

- stable_inodes  
Marks the file system's inode numbers and UUID as stable.
**resize2fs (8)**
will not allow shrinking a file system with this feature, nor
will **tune2fs (8)**
allow changing its UUID.  This feature allows the use of specialized encryption
settings that make use of the inode numbers and UUID

- uninit_bg  
This ext4 file system feature indicates that the block group descriptors
will be protected using checksums, making it safe for
**mke2fs (8)**
to create a file system without initializing all of the block groups.
The kernel will keep a high watermark of unused inodes, and initialize
inode tables and blocks lazily.  This feature speeds up the time to check
the file system using
**e2fsck (8)**,
and it also speeds up the time required for
**mke2fs (8)**
to create the file system.

### Readonly compatible Features:

When present, these would cause the current ext2.xfs to mount the filesystem readonly.


- bigalloc  
This ext4 feature enables clustered block allocation, so that the unit of
allocation is a power of two number of blocks.  That is, each bit in the
what had traditionally been known as the block allocation bitmap now
indicates whether a cluster is in use or not, where a cluster is by
default composed of 16 blocks.  This feature can decrease the time
spent on doing block allocation and brings smaller fragmentation, especially
for large files.

- extra_isize  
This ext4 feature reserves a specific amount of space in each inode for
extended metadata such as nanosecond timestamps and file creation time,
even if the current kernel does not currently need to reserve this much
space.  Without this feature, the kernel will reserve the amount of
space for features it currently needs, and the rest may be
consumed by extended attributes.

- huge_file  
This ext4 feature allows files to be larger than 2 terabytes in size.

- metadata_csum  
This ext4 feature enables metadata checksumming.  This feature stores
checksums for all of the file system metadata (superblock, group
descriptor blocks, inode and block bitmaps, directories, and
extent tree blocks).  The checksum algorithm used for the metadata
blocks is different than the one used for group descriptors with the

- orphan_file  
This ext4 feature fixes a potential scalability bottleneck for workloads
that are doing a large number of truncate or file extensions in
parallel.  It is supported by Linux kernels starting version 5.15, and
by e2fsprogs starting with version 1.47.0.

- project  
This ext4 feature provides project quota support. With this feature,
the project ID of inode will be managed when the file system is mounted.

- quota  
Create quota inodes (inode #3 for userquota and inode #4 for group quota) and set them in the superblock.
With this feature, the quotas will be enabled
automatically when the file system is mounted.

- verity  
Enables support for verity protected files.  Verity files are readonly,
and their data is transparently verified against a Merkle tree hidden
past the end of the file.  Using the Merkle tree's root hash, a verity
file can be efficiently authenticated, independent of the file's size.  
This feature is most useful for authenticating important read-only files
on read-write file systems.  If the file system itself is read-only,
then using dm-verity to authenticate the entire block device may provide
much better security.


### Incompatible Features:

When present, these would cause the current ext2.xfs to refuse to mount the filesystem.

- 64bit  
Enables the file system to be larger than 2^32 blocks.  This feature is set
automatically, as needed, but it can be useful to specify this feature
explicitly if the file system might need to be resized larger than 2^32
blocks, even if it was smaller than that threshold when it was
originally created.  Note that some older kernels and older versions
of e2fsprogs will not support file systems with this ext4 feature enabled.

- casefold  
This ext4 feature provides file system level character encoding support
for directories with the casefold (+F) flag enabled.  This feature is
name-preserving on the disk, but it allows applications to lookup for a
file in the file system using an encoding equivalent version of the file
name.

- ea_inode  
Normally, a file's extended attributes and associated metadata must fit within
the inode or the inode's associated extended attribute block. This feature
allows the value of each extended attribute to be placed in the data blocks of a
separate inode if necessary, increasing the limit on the size and number of
extended attributes per file.

- encrypt  
Enables support for file-system level encryption of data blocks and file
names.  The inode metadata (timestamps, file size, user/group ownership,
etc.) is *not* encrypted.

- extent  
This ext4 feature allows the mapping of logical block numbers for a
particular inode to physical blocks on the storage device to be stored
using an extent tree, which is a more efficient data structure than the
traditional indirect block scheme used by the ext2 and ext3 file
systems.  The use of the extent tree decreases metadata block overhead,
improves file system performance, and decreases the needed to run
**e2fsck (8)** on the file system.

- flex_bg  
This ext4 feature allows the per-block group metadata (allocation
bitmaps and inode tables)
to be placed anywhere on the storage media.  In addition,
**mke2fs** will place the per-block group metadata together starting at the first
block group of each "flex_bg group".

- inline_data  
Allow data to be stored in the inode and extended attribute area.

- journal_dev  
This feature is enabled on the superblock found on an external journal
device.  The block size for the external journal must be the same as the
file system which uses it.

- large_dir  
This feature increases the limit on the number of files per directory by
raising the maximum size of directories and, for hashed b-tree directories (see
**dir_index**),
the maximum height of the hashed b-tree used to store the directory entries.

- metadata_csum_seed  
This feature allows the file system to store the metadata checksum seed in the
superblock, which allows the administrator to change the UUID of a file system
using the **metadata_csum** feature while it is mounted.

- meta_bg  
This ext4 feature allows file systems to be resized on-line without explicitly
needing to reserve space for growth in the size of the block group
descriptors.  This scheme is also used to resize file systems which are
larger than 2^32 blocks.  It is not recommended that this feature be set
when a file system is created, since this alternate method of storing
the block group descriptors will slow down the time needed to mount the
file system, and newer kernels can automatically set this feature as
necessary when doing an online resize and no more reserved space is
available in the resize inode.

- mmp  
This ext4 feature provides multiple mount protection (MMP).  MMP helps to
protect the file system from being multiply mounted and is useful in
shared storage environments.



## History:
See in ChangeLog for more details

Thanks to Axel Kaiser who started this project and put it into the GPL.
Now it's almost ready, stable and very fast.

Thanks also to all other people that helped me and told me that
they are very happy to see this development. This was a good
motivation.


Have fun.

Frank Naumann
<fnaumann@freemint.de>

Magdeburg, 16.09.2000
