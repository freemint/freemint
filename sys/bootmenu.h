/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 */

# ifndef _bootmenu_h
# define _bootmenu_h

extern short load_xfs_f;
extern short load_xdd_f;
extern short load_auto;

int boot_kernel_p(void);
void read_ini(void);
void pause_and_ask(void);

# endif
