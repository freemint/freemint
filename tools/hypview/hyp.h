/*
 * HypView - (c) 2001 - 2006 Philipp Donze
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
	char magic[4];               /* Magic constant 'HDOC' */
	long itable_size;            /* Length of index table in bytes */
	unsigned short itable_num;   /* Number of entries in table */
	unsigned char compiler_vers; /* Hypertext compiler version */
	unsigned char compiler_os;   /* Operating system used for translation */
}HYP_HEADER;

/* Valid values for HYP_HEADER->compiler_os */
enum {UNKNOWN_OS = 0, AMIGA_OS, ATARI_OS, MAC_OS};


typedef struct
{
	unsigned char length;       /* Length */
	unsigned char type;         /* Type */
	unsigned long seek_offset;  /* File seek offset*/
	unsigned short comp_diff;   /* Difference packed/unpacked in bytes */
	unsigned short next;        /* Index of successor entry */
	unsigned short previous;    /* Index of predecessor entry */
	unsigned short dir_index;   /* Index of directory/parent entry*/
	char name;                  /* First character of 0-terminated entry name */
}INDEX_ENTRY;

/* Valid values for INDEX_ENTRY->type */
enum {
	INTERNAL = 0, POPUP, EXTERNAL_REF, PICTURE,
	SYSTEM_ARGUMENT, REXX_SCRIPT, REXX_COMMAND, QUIT
};


/* ESC values used in (p)nodes */
#define WINDOWTITLE      35    /* Window title */
#define LINK             36    /* link */
#define ALINK            38    /* alink */
#define EXTERNAL_REFS    48    /* external data block */
#define OBJTABLE         49    /* table with objects */
#define PIC              50    /* picture */
#define LINE             51    /* line */
#define BOX              52    /* box */
#define RBOX             53    /* rounded box */
#define TEXT_ATTRIBUTES	100    /* text/font attribute */

#define LINEY			0x100


/* picture header structure*/
typedef struct
{
	unsigned short width;        /* width in pixels */
	unsigned short height;       /* height in pixel */
	unsigned char planes;        /* number of planes (1..8) */
	unsigned char plane_pic;     /* available planes bit vector */
	unsigned char plane_on_off;  /* filled plane bit vector */
	unsigned char filler;        /* fill byte used to align size of structure */
}HYP_PICTURE;

/* Storage structure for an entry in memory */
typedef struct
{
	unsigned short number;
	void *start;
	unsigned long size;
}LOADED_ENTRY;


/*
 *	Definitions used by loading routines
 */

enum 
{
	REF_FILENAME = 0, REF_NODENAME, REF_ALIASNAME, REF_LABELNAME,
	REF_DATABASE, REF_OS
};

typedef struct
{
	long module_len;
	long num_entries;
	char start;
}REF_FILE;

typedef struct
{
	unsigned short num_index;   /* Number of entries in hypertext file */
	unsigned char comp_vers;    /* Version of compiler used for translation */
	unsigned char comp_os;      /* Operating system ID */
	short st_guide_flags;       /* Special ST-Guide flags (encryption, ...) */
	short line_width;           /* Line width to use for display (@width) */
	char *database;             /* Description for hypertext database (@database) */
	char *hostname;             /* Name of host applikation (@hostname) */
	char *author;               /* Name of authors (@author) */
	char *version;              /* Hypertext version string (@$VER:) */
	INDEX_ENTRY **indextable;   /* Pointer to index table */
	void **cache;               /* Pointer to cache table */
	LOADED_ENTRY *entry;        /* Currently displayed entry */
	char *file;                 /* Full access path to file */
	short index_page;           /* Page number of hypertext index */
	short default_page;         /* Page number of default page (@default) */
	short help_page;            /* Page number of help page */
	REF_FILE *ref;              /* Pointer to REF file structure */
}HYP_DOCUMENT;

/* Valid values for HYP_DOCUMENT->st_guide_flags */
#define	STG_ENCRYPTED	0x200

struct lineptr
{
	short x, y, w, h;
	char *txt;
};
typedef struct lineptr LINEPTR;

typedef struct
{
	unsigned short number;      /* Page number of this entry */
	char *start;                /* Pointer to start of data */
	char *end;                  /* Pointer to end of data */
	long lines, columns;        /* Number of lines and columns */
	long height;
	char *window_title;         /* Pointer to window title */
	LINEPTR *line_ptr;
}LOADED_NODE;

struct _loaded_picture_;
struct _loaded_picture_
{
	unsigned short number;
	MFDB mfdb;
};
typedef struct _loaded_picture_ LOADED_PICTURE;

struct pic_adm
{
	short type;
	unsigned short flags;
	char *src;
	char osrc[8];
	void *data;
	GRECT trans;
	GRECT orig;
};

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
long HypFindNode(DOCUMENT *doc, char *chapter);
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
short HypGetLineY ( LOADED_NODE *node, long line );
long HypGetRealTextLine ( LOADED_NODE *node, short y );
void HypGetTextLine(HYP_DOCUMENT *hyp, long line,char *dst );
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
	short selected;
	char str[256];

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
void search_allref(char *string, short no_message );

/*
 *		Search_D.c
 */
void HypfindFinsih ( short AppID, short ret );

#endif
