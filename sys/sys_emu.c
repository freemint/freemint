/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * This file is dedicated to the FreeMiNT project.
 * It's not allowed to use this file on other projects without my explicit
 * permission.
 */

/*
 * begin:	2000-03-26
 * last change: 2000-03-26
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
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
	}
	
	return EBADARG;
}
