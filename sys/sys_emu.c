/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-03-26
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * changes since last version:
 * 
 * 2000-03-26:
 * 
 * - initial revision
 * 
 * known bugs:
 * 
 * todo:
 * 
 */

# include "sys_emu.h"

# include "scsidrv.h"
# include "xhdi.h"
# include "pcibios.h"


long _cdecl
sys_emu (ushort which, ushort op,
		long a1, long a2, long a3, long a4,
		long a5, long a6, long a7)
{
	switch (which)
	{
		/* call exist */
		case 0:
		{
			return E_OK;
		}
		
		/* XHDI */
		case 1:
		{
			return sys_xhdi (op, a1, a2, a3, a4, a5, a6, a7);
		}
		
		/* SCSIDRV */
		case 2:
		{
			return sys_scsidrv (op, a1, a2, a3, a4, a5, a6, a7);
		}
#ifdef PCI_BIOS
		/* PCI-BIOS */
		case 3:
		{
			return sys_pcibios (op, a1, a2, a3, a4, a5, a6, a7);
		}
#endif /* PCI-BIOS */
	}
	
	return EBADARG;
}
