/*
 * Filename:     super.h
 * Project:      ext2 file system driver for MiNT
 * 
 * Note:         Please send suggestions, patches or bug reports to
 *               the MiNT mailing list <freemint-discuss@lists.sourceforge.net>
 * 
 * Copying:      Copyright 1999 Frank Naumann (fnaumann@freemint.de)
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

# ifndef _super_h
# define _super_h

# include "global.h"


long	read_ext2_sb_info	(ushort drv);


/* NOTE: ext2_get_group_desc does not modify the buffer cache
 */

INLINE ext2_gd *
ext2_get_group_desc (SI *s, register ulong group, UNIT **u)
{
	register ulong group_desc;
	register ext2_gd *gd;
	
	if (group >= s->sbi.s_groups_count)
	{
		ALERT (("ext2_get_group_desc: group (%li) >= groups_count (%li)",
			group, s->sbi.s_groups_count));
		
		return NULL;
	}
	
	group_desc = group >> EXT2_DESC_PER_BLOCK_BITS (s);
	
	if (u) *u = s->sbi.s_group_desc_units [group_desc];
	
	gd  = s->sbi.s_group_desc [group_desc];
	gd += group & EXT2_DESC_PER_BLOCK_MASK (s);
	
	return gd;
}


# endif /* _super_h */
