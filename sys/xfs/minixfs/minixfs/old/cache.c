/* This file is part of 'minixfs' Copyright 1991,1992,1993 S.N. Henson */

# include "cache.h"
# include "global.h"

# include "check.h"


/* For this kind of file system a cache is absolutely essential,
 * without it your hard-disk will sound like a buzz-saw .... the 
 * idea is fairly simple, for every block requested if it is not in 
 * the cache load it in at the current position .Then return a 
 * pointer to the block. Additionally all writes go to the cache if
 * an entry exists for the specified block. This means that when a 
 * cache entry is overwritten we must write out the entry if it is
 * 'dirty', the function l_sync() writes out the entire cache along
 * with the zone and inode bitmaps. However for an increase in performance, 
 * three caches are used, one for inodes one for directories and indirection 
 * blocks, one for files, these are kmalloc()'ed at the same time so that 
 * one follows the other in memory, this simplifies cache-flushing for example.
*/

/* Initialise cache */
/* no longer used */

