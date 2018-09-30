/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <mint/ssystem.h>

#include "cops_rsc.h"
#include "phstuff.h"


/*----------------------------------------------------------------------------------------*/ 
/* CPX-Datei laden und relozieren */
/* Funktionsresultat:	Zeiger auf den Programmstart oder 0 */
/*----------------------------------------------------------------------------------------*/ 

static void *
load_and_reloc(CPX_DESC *cpx_desc, long handle, long fsize, struct program_header *phead, long *cmplt_size)
{
	void *addr = NULL;
	long TD_len, TDB_len;

	/* Laenge von Text- und Data-Segment */
	TD_len = phead->ph_tlen + phead->ph_dlen;
	/* Laenge von Text-, Data- und BSS-Segment */
	TDB_len = TD_len + phead->ph_blen;
	*cmplt_size = TDB_len;

	/* Speicher fuer Text-, Data- und BSS-Segment anfordern */
	addr = malloc(TDB_len);	
	if (addr)
	{
		long relo_len;

		DEBUG(("%s: malloc(%lu) -> %p\n", __FUNCTION__, TDB_len, addr));

		cpx_desc->segm.text_seg = addr;
		cpx_desc->segm.len_text = phead->ph_tlen;
		cpx_desc->segm.data_seg = (unsigned char *)addr + phead->ph_dlen;
		cpx_desc->segm.len_data = phead->ph_dlen;
		cpx_desc->segm.bss_seg = (unsigned char *)addr + phead->ph_dlen + phead->ph_blen;
		cpx_desc->segm.len_bss = phead->ph_blen;

		/* Laenge der Relokationsdaten */
		relo_len = fsize - sizeof(struct program_header) - TD_len - phead->ph_slen;

		/* Text- und Data-Segment laden */
		if (Fread((short)handle, TD_len, addr) == TD_len)
		{
			/* Symboltabelle ueberspringen */
			Fseek(phead->ph_slen, (short)handle, 1);

			/* BSS-Segment loeschen */
			memset((char *)addr + TD_len, 0, phead->ph_blen);

			/* Datei relozieren */
			if ((phead->ph_absflag == 0) && relo_len)
			{
				void *relo_mem;

				/* Speicher fuer Relokationsdaten anfordern */
				relo_mem = malloc(relo_len);
				if (relo_mem)
				{
					if (Fread((short)handle, relo_len, relo_mem) == relo_len)
					{
						unsigned char *relo;
						long relo_offset;

						/* Zeiger auf die Relokationsdaten */
						relo = (unsigned char *)relo_mem;
						/* Startoffset fuer Relokationsdaten */
						relo_offset = *(long *)relo;
						relo += 4;

						/* Relokationsdaten vorhanden? */
						if (relo_offset)
						{
							unsigned char *code_ptr;
							unsigned char relo_val;

							/* erstes zu relozierendes Langwort */
							code_ptr = ((unsigned char *)addr) + relo_offset;
							*(unsigned long *)code_ptr += (unsigned long)addr;
							
							while ((relo_val = *relo++) != 0)
							{
								if (relo_val == 1)	
									code_ptr += 254;
								else
								{
									code_ptr += relo_val;
									*(unsigned long *)code_ptr += (unsigned long)addr;
								}
							}
						}
					}
					else
					{
						free(addr);
						addr = NULL;
					}

					/* Speicher fuer Relokationsdaten freigeben */
					free(relo_mem);
				}
				else
				{
					free(addr);
					addr = NULL;
				}
			}
		}
		else
		{
			free(addr);
			addr = NULL;
		}	
	}

	/* Programm geladen? */
	if (addr)
	{
		if (!Ssystem(-1, 0L, 0L))
		{
			/* flush cpu caches */
			Ssystem(S_FLUSHCACHE, (long)addr, TDB_len);
		}
		else
		{
#if 0 /* correct clear_cpu_caches.s */
			extern long clear_cpu_caches(void);
			Supexec(clear_cpu_caches);
#endif
		}
	}

	return addr;
}

void *
load_cpx(CPX_DESC *cpx_desc, char *cpx_path, long *cmplt_size, short load_header)
{
	char name[256];
	struct xattr xattr;
	void *addr = NULL;

	strcpy(name, cpx_path);
	strcat(name, cpx_desc->file_name);

	if (Fxattr(0, name, &xattr) == 0)
	{
		long handle;

		handle = Fopen(name, O_RDONLY);
		if (handle > 0)
		{
			struct program_header phead;

			if (load_header)
				/* load CPX header */
				Fread((short)handle, sizeof(struct cpxhead), &(cpx_desc->old.header));
			else
				Fseek(sizeof(struct cpxhead), (short)handle, 0);

			xattr.st_size -= sizeof(struct cpxhead);

			/* load program header */
			if (Fread((short)handle, sizeof(phead), &phead) == sizeof(phead))
			{
				/* bra.s am Anfang? */
				if (phead.ph_branch == PH_MAGIC)
					/* load and relocate */
					addr = load_and_reloc(cpx_desc, handle, xattr.st_size, &phead, cmplt_size);
			}
			Fclose((short)handle);
		}
	}

	return addr;
}

short
unload_cpx(void *addr)
{
	free(addr);
	DEBUG(("%s: free(%p)\n", __FUNCTION__, addr));

	return 1;
}
