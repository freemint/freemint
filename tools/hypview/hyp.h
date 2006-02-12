/*
 * $Id$
 * 
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

#ifndef _hyp_h
#define	_hyp_h	1

#include <compiler.h>		/* for inline macro */

#ifndef _dl_miss_h
	typedef short EVNTDATA[4];
#endif
#ifndef __GEMLIB_DEFS
	typedef long MFDB;
#endif

#ifndef _diallib_h
	typedef long WINDOW_DATA;
#endif

#ifndef DEFS_H
	typedef long DOCUMENT;
	typedef long TEXT_POS;
	typedef long BLOCK;
#endif

/*
 *		Datei-bedingte Strukturen
 */
typedef struct
{
	char magic[4];			/*	'HDOC'	*/
	long itable_size;		/*	Lnge der Indextabelle in Bytes	*/
	unsigned short itable_num;	/*	Anzahl der Eintrge in dieser Tabelle	*/
	unsigned char compiler_vers;	/*	Kennung, mit welcher Version des Compilers bersetzt wurde	*/
	unsigned char compiler_os;	/*	Kennung, unter welchem Betriebsystem bersetzt wurde	*/
}HYP_HEADER;

/*	Werte fr HYP_HEADER->compiler_os	*/
enum {UNKNOWN_OS=0, AMIGA_OS=1, ATARI_OS=2, MAC_OS=3, NO_MORE_OS};


typedef struct
{
	unsigned char length;		/*	Lnge dieses Eintrages	*/
	unsigned char type;			/*	Typ dieses Eintrages	*/
	unsigned long seek_offset;	/*	Seek Offset in die Datei	*/
	unsigned short comp_diff;	/*	Differenz entpackte/gepackte Lnge des Objektes	*/
	unsigned short next;			/*	Index des Nachfolgers	*/
	unsigned short previous;	/*	Index des Vorgngers	*/
	unsigned short dir_index;	/*	Index des Inhaltsverzeichnisses fr dieses Objekt	*/
	char name;						/*	erstes Zeichen des Namens (nullterminiert)	*/
}INDEX_ENTRY;

/*	Werte fr INDEX_ENTRY->type	*/
enum {INTERNAL=0, POPUP, EXTERNAL_REF, PICTURE,
	SYSTEM_ARGUMENT, REXX_SCRIPT, REXX_COMMAND, QUIT};


/*	Werte bei (P)Nodes (ESC-Codes)	*/
#define WINDOWTITLE		35			/*	Fenstertitel	*/
#define LINK			36			/*	link	*/
#define ALINK			38			/*	alink	*/
#define EXTERNAL_REFS		48			/*	Querverweis-Datenblcke	*/
#define OBJTABLE		49			/*	Tabelle mit Objekten & Seiten	*/
#define PIC			50			/*	Bild	*/
#define LINE			51			/*	Linie	*/
#define BOX			52			/*	Box	*/
#define RBOX			53			/*	Box mit runden Ecken	*/
#define TEXT_ATTRIBUTES		100		/*	Textattribute	*/

#define LINEY			0x100


/*	Header eines Bildes	*/
typedef struct
{
	unsigned short width;			/*	Breite des Bildes in Pixel	*/
	unsigned short height;			/*	Hhe des Bildes in Pixel	*/
	unsigned char planes;			/*	Anzahl der Planes (1..8)	*/
	unsigned char plane_pic;		/*	Bit n ==> Daten fr Plane n vorhanden	*/
	unsigned char plane_on_off;		/*	Bit n ==> Plane n ganz ausgefllt	*/
	unsigned char filler;			/*	Fllbyte, damit das Bild auf gerader Adr. liegt 	*/
}HYP_PICTURE;

/*	Frhzeitige Definition	*/
typedef struct
{
	unsigned short number;
	void *start;
	unsigned long size;
}LOADED_ENTRY;


/*
 *		Definitionen fr die Lade-Routinen
 */

enum 
{
	REF_FILENAME=0, REF_NODENAME=1, REF_ALIASNAME=2, REF_LABELNAME=3,
	REF_DATABASE=4, REF_OS=5
};

typedef struct
{
	long module_len;
	long num_entries;
	char start;
}REF_FILE;

typedef struct
{
	unsigned short num_index;	/*	Anzahl der Eintrge im Hypertext	*/
	unsigned char comp_vers;	/*	Compiler Version	*/
	unsigned char comp_os;		/*	Betriebsystem-Kennung	*/
	short st_guide_flags;		/*	Spez. ST-Guide Flags (Verschlsselung,...)	*/
	short line_width;		/*	Verwendete Zeilenbreite (@width)	*/
	char *database;			/*	Info zum Hypertext (@database)	*/
	char *hostname;			/*	Name der Host-Applikation (@hostname)	*/
	char *author;			/*	Name des Autors (@author)	*/
	char *version;			/*	Versionsstring (@$VER:)	*/
	INDEX_ENTRY **indextable;	/*	Zeiger auf die Indextabelle	*/
	void **cache;			/*	Zeiger auf Cache-Tabelle	*/
	LOADED_ENTRY *entry;		/*	Zur Zeit dargestellte Seite	*/
	char *file;			/*	Zeiger auf den vollstndigen Zugriffspfad	*/
	short index_page;		/* Seitennummer der Index-Seite	*/
	short default_page;		/*	Seitennummer der Standard-Seite (@default)	*/
	short help_page;		/*	Seitennummer der Hilfeseite	*/
	REF_FILE *ref;			/*	Zeiger auf die REF-Datei	*/
}HYP_DOCUMENT;

/* Definitionen fr <st_guide_flags>	*/
#define	STG_ENCRYPTED	0x200

struct lineptr
{
	short x, y, w, h;
	char *txt;
};
typedef struct lineptr LINEPTR;

typedef struct
{
	unsigned short number;		/*	Seitennummer der geladenen Seite	*/
	char *start;					/*	Zeiger auf den Anfang der Daten	*/
	char *end;						/*	Zeiger auf das Ende der Daten	*/
	long lines,columns;			/*	Anzahl Zeilen und Kolonnen	*/
	long height;
	char *window_title;			/*	Zeiger auf den Fenstertitel	*/
	LINEPTR *line_ptr;
}LOADED_NODE;

struct _loaded_picture_;
struct _loaded_picture_
{
	unsigned short number;
	MFDB mfdb;
};
typedef struct _loaded_picture_ LOADED_PICTURE;

struct prepnode
{
	HYP_DOCUMENT *hyp;
	LOADED_NODE *node;
	struct pic_adm *pic_adm;
	short pic_count;
	short pic_idx;
	LINEPTR *line_ptr;
	long y, h;
	short line;
	short min_lines;
	short empty;
	short empty_lines;
	short empty_start;
	short limage_add;

	short start_idx;
	short start_y;
	short start_line;
	
	short between;
	long y_start;

	short last_line;
	short last_width;
	long last_y;

	short width;
	
};



/*	Decode MACRO	*/
#define DEC_255(ptr)	(unsigned short) ((((unsigned char *)ptr)[1] - 1 ) * 255 + ((unsigned char *)ptr)[0] - 1)
static inline short
dec_from_chars(unsigned char *ptr)
{
	unsigned short val;

	val = (ptr[1] - 1) * 255;
	val += ptr[0];
	val--;
	return val;
}

static inline void
dec_to_chars(unsigned short _val, unsigned char *ptr)
{
	ptr[0] = (unsigned char)(_val);
	ptr[1] = (unsigned char)((unsigned long)_val / 255) + 1;
	ptr[0] += ptr[1];
}
static inline void
long_to_chars(long _val, char *p)
{
	union { char c[4]; long l[1]; } val;

	val.l[0] = _val;
	*p++ = val.c[0];
	*p++ = val.c[1];
	*p++ = val.c[2];
	*p   = val.c[3];
}
static inline long
long_from_chars(char *p)
{
	union { char c[4]; long l[1]; } val;

	val.c[0] = *p++;
	val.c[1] = *p++;
	val.c[2] = *p++;
	val.c[3] = *p;

	return val.l[0];
}

/*
 *		Hyp.c
 */
short HypLoad(DOCUMENT *doc, short handle);
void HypClose(DOCUMENT *doc);
long HypGetNode(DOCUMENT *doc);
/* [GS] 0.35.2a Start */
long HypFindNode(DOCUMENT *doc, char *chapter);
/* Ende */
void HypGotoNode(DOCUMENT *doc, char *chapter,long node_num);

/*
 *		Load.c
 */
long GetDataSize(HYP_DOCUMENT *doc,long nr);
void *LoadData(HYP_DOCUMENT *doc,long nr, long *data_size);
void GetEntryBytes(HYP_DOCUMENT *doc,long nr,void *src,void *dst,long bytes);

/*
 *		Tool.c
 */
short find_nr_by_title(HYP_DOCUMENT *hyp_doc,char *title);
char *skip_esc(char *pos);

/*
 *		Cache.c
 */
long GetCacheSize(long num_elements);
void InitCache(HYP_DOCUMENT *hyp);
void ClearCache(HYP_DOCUMENT *hyp);
void TellCache(HYP_DOCUMENT *hyp, long node_num,void *node);
void *AskCache(HYP_DOCUMENT *hyp, long node_num);
void RemoveNodes(HYP_DOCUMENT *hyp);

/*
 *		Prepare.c
 */
short PrepareNode(HYP_DOCUMENT *hyp, LOADED_NODE *node);

/*
 *		Display.c
 */
extern HYP_DOCUMENT *Hyp;
void HypDisplayPage(DOCUMENT *doc);

/*
 *		Lh5d.c
 */
extern char *lh5_packedMem;
extern long lh5_packedLen;
extern char *lh5_unpackedMem;
extern long lh5_unpackedLen;

void lh5_decode(short reset);

/*
 *		Click.c
 */
void HypClick(DOCUMENT *doc, EVNTDATA *event);

/*
 *		Popup.c
 */
typedef struct
{
	DOCUMENT *doc;
	long lines,cols;
	short x,y;
	void *entry;
}POPUP_INFO;

void OpenPopup(DOCUMENT *doc, long num, short x, short y);

/*
 *		Autoloc.c
 */
LINEPTR *HypGetYLine(LOADED_NODE *node, long y);
void HypGetTextLine(HYP_DOCUMENT *hyp, long line,char *dst);
long HypAutolocator(DOCUMENT *doc,long line);

/*
 *		Cursor.c
 */
void HypGetCursorPosition(DOCUMENT *doc,short x, short y, TEXT_POS *pos);

/*
 *		Block.c
 */
short HypBlockOperations(DOCUMENT *doc, short op, BLOCK *block, void *param);

/*
 *		Ref.c
 */
typedef struct _result_entry_ 
{
	struct _result_entry_ *next;
/* [GS] 0.35.2a Start: */
	short selected;
	char str[256];
/* Ende */

	char path[256];
	char dbase_description[256];
	char node_name[256];
	unsigned short line;
	short is_label;
} RESULT_ENTRY;

REF_FILE *ref_load(short handle);
char *ref_find(REF_FILE *ref, short type);
short ref_findnode(REF_FILE *ref, char *string, long *node, short *line);
RESULT_ENTRY *ref_findall(REF_FILE *ref, char *string, short *num_results);

/*
 *		Ext_Refs.c
 */
short HypCountExtRefs(LOADED_NODE *entry);
void HypExtRefPopup(DOCUMENT *doc, short x, short y);
void HypOpenExtRef(char *name, short new_window);

/*
 *		Search.c
 */
/* [GS] 0.35.2a Start: */
void search_allref(char *string, short no_message );
/* Ende; alt:
void search_allref(char *string);
*/

#endif
