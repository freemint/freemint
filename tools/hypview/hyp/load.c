/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#ifdef __GNUC__
	#include <osbind.h>
	#include <fcntl.h>
#else
	#include <tos.h>
#endif
#include <string.h>
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

/*	Startet den LH5 Entpacker	*/
static void
Decompress(void *c_data,long c_data_size, void *u_data, long u_data_size)
{
	/*	Variablen fr das Entpacken der LH5 Daten	*/
	lh5_packedMem=c_data;
	lh5_packedLen=c_data_size;

	lh5_unpackedMem=u_data;
	lh5_unpackedLen=u_data_size;

	lh5_decode(TRUE);				/*	Daten dekomprimieren	*/
}

/*	Decodiert die verschluesselten Daten	*/
static void
Decode(unsigned char *ptr, long bytes)
{
	while (bytes--)
		*ptr++ = *ptr ^ 127;							/*	Entschlsseln	*/
}

/*	Gibt die unkomprimierte Groesse des Eintrags <num> zurck	*/
long GetDataSize(HYP_DOCUMENT *doc, long num)
{
	long data_size;
	data_size =  doc->indextable[num + 1]->seek_offset - doc->indextable[num]->seek_offset;
	data_size += doc->indextable[num]->comp_diff;
	
	if(doc->indextable[num]->type == PICTURE)
		data_size += ((long)doc->indextable[num]->next) << 16;

	return(data_size);
}

/*	Ladet die Daten des Eintrags <num> und bestimmt dessen Laenge	*/
void *LoadData(HYP_DOCUMENT *doc,long num, long *data_size)
{
	INDEX_ENTRY *idxent = doc->indextable[num];
	long ret;
	short handle;
	void *data = NULL;

	ret = Fopen(doc->file, O_RDONLY);
	if(ret < 0)
	{
		printf("Error %ld in Fopen(%s)\n",ret,doc->file);
		return(NULL);
	}
	handle = (short)ret;

	ret = Fseek(idxent->seek_offset, handle, 0);
	if(ret < 0)
	{
		Fclose(handle);
		printf("Error %ld in Fseek(%ld,handle,0)\n",ret,idxent->seek_offset);
		return(NULL);
	}

	*data_size = doc->indextable[num + 1]->seek_offset - idxent->seek_offset;

	data = (void *)Malloc(*data_size);
	if(!data)
	{
		Fclose(handle);
		printf("Error in Malloc(%ld)\n",*data_size);
		return(NULL);
	}

	/*	Daten in den Speicher laden	*/
	ret = Fread(handle,*data_size,data);
	if(ret < 0 || ret != *data_size)
	{
		Mfree(data);
		Fclose(handle);
		if(ret < 0)
			printf("Error %ld in Fread(handle,%ld,%lx)\n", ret, *data_size, (unsigned long)data);
		else
			printf("Error in Fread(handle,%ld,%lx). Loaded %ld bytes.\n",*data_size, (unsigned long)data, ret);
		return(NULL);
	}

	Fclose(handle);					/*	Datei schliessen	*/
	return(data);
}

/*
 *		Kopiert von den Eintragsdaten <src> (geladen mit LoadData())
 *		<bytes> Bytes nach <dst>.
 *		Wenn noetig werden die Daten dekomprimiert und entschluesselt.
 */
void GetEntryBytes(HYP_DOCUMENT *doc,long num,void *src,void *dst,long bytes)
{
	INDEX_ENTRY *idxent=doc->indextable[num];

	if(idxent->comp_diff)		/*	Daten komprimiert?	*/
	{
		long data_size = doc->indextable[num + 1]->seek_offset - idxent->seek_offset;
		if(idxent->type == PICTURE)
			data_size += ((long)idxent->next) << 16;

		Decompress(src,data_size,dst,bytes);
	}
	else
		memcpy(dst,src,bytes);

	/*	Daten verschluesselt?	*/
	if(doc->st_guide_flags & STG_ENCRYPTED)
		Decode(dst,bytes);
}

