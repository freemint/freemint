/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _memory_h
# define _memory_h

# include "mint/mint.h"
# include "mint/mem.h"
# include "mint/proc.h"


#define ALT_1 0x780000L		/* moved from fasttext.h */
#define ALT_2 0x790000L
#define ALT_0 0x810000L

typedef struct screen SCREEN;
struct screen
{
	short	hidecnt;	/* cursor hide count */
	short	mcurx, mcury;	/* current mouse x, y position */
	char	mdraw;
	char	mouseflag;
	long	junk1;
	short	savex, savey;	/* saved x, y position */
	short	msavelen;	/* mouse save stuff */
	long	msaveaddr;
	short	msavestat;
	long	msavearea[64];
	long	user_tim;
	long	next_tim;	/* time vector stuff */
	long	user_but;
	long	user_cur;
	long	user_mot;	/* more user vectors */
	short	cheight;	/* character height */
	short	maxx;		/* number of characters across - 1 */
	short	maxy;		/* number of characters high - 1 */
	short	linelen;	/* length (in bytes) of a line of characters */
	short	bgcol;		/* background color */
	short	fgcol;		/* foreground color */
	char	*cursaddr;	/* cursor address */
	short	v_cur_of;	/* offset from physical screen address */
	short	cx, cy;		/* current (x,y) position of cursor */
	char	period;		/* cursor flash period (in frames) */
	char	curstimer;	/* cursor flash timer */
	char	*fontdata;	/* pointer to font data */
	short	firstcode;	/* first ASCII code in font */
	short	lastcode;	/* last ASCII code in font */
	short	form_width;	/* # bytes/scanline in font data */
	short	xpixel;
	char	*fontoff;	/* pointer to font offset table */
	char	flags;		/* e.g. cursor on/off */
	char	v_delay;	/* delay before cursor flash; undocumented! */
	short	ypixel;
	short	width;		/* length of a screen scan line */
	short	planes;		/* number of planes on screen */
	short	planesiz;	/* length of a screen scan line */
};

extern MMAP core, alt, swap;

extern short forcefastload;
extern ulong initialmem;


void	init_mem (void);
void	restr_screen (void);
int	add_region (MMAP map, ulong place, ulong size, ushort mflags);
void	init_core (void);
void	init_swap (void);

long	change_prot_status (PROC *proc, long start, short newmode);
long	attach_region (PROC *proc, MEMREGION *reg);
void	detach_region (PROC *proc, MEMREGION *reg);
long	detach_region_by_addr (PROC *p, long block);

MEMREGION *get_region (MMAP map, ulong size, short mode);
MEMREGION *_get_region (MMAP map, ulong size, short mode, short cmode, MEMREGION *descr, short kernel_flag);
void	free_region (MEMREGION *reg);
long	shrink_region (MEMREGION *reg, ulong newsize);

long	max_rsize (MMAP map, long needed);
long	tot_rsize (MMAP map, short flag);
long	freephysmem (void);
long 	alloc_region (MMAP map, ulong size, short mode);
MEMREGION *fork_region (MEMREGION *reg, long txtsize);
MEMREGION *create_env (const char *env, ulong flags);
MEMREGION *create_base (const char *cmd, MEMREGION *env,
			unsigned long flags, unsigned long prgsize, long *err);
MEMREGION *load_region (const char *name, MEMREGION *env, const char *cmdlin, XATTR *x,
			long *fp, long *err);
long	load_and_reloc (FILEPTR *f, FILEHEAD *fh, char *where, long start,
			long nbytes, BASEPAGE *base);
long	memused (PROC *p);
void	recalc_maxmem (PROC *p);
int	valid_address (long addr);
MEMREGION *addr2mem (PROC *p, long addr);
MEMREGION *addr2region (long addr);
MEMREGION *proc_addr2region (PROC *p, long addr);

long	realloc_region (MEMREGION *, long);
long	_cdecl sys_s_realloc (long);

# ifdef DEBUG_INFO
void DUMP_ALL_MEM (void);
void DUMPMEM (MMAP map);

# if WITH_KERNFS
long  kern_get_memdebug (SIZEBUF **buffer);
# endif

# endif


# endif /* _memory_h */
