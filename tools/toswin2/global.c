/*
 * global.c
 *
 * Globale Typen, Variablen und Hilfsfunktionen
 */

#include <ctype.h>
#include <sys/types.h>
#include <gem.h>

#include "global.h"
#include "ansicol.h"
#include "toswin2.h"

/*
 * Globale Variablen
 */
OBJECT	*winicon,
	*conicon;
int	exit_code;
int	vdi_handle;
int	font_anz;
int	windirty = 0;
short	wco;

/*
 * Lokale Variablen
*/
char	**stringarray;
static int 	lasttextcolor = -1, 
		lastfillcolor = -1, 
		lastwrmode = -1, 
		lastheight = -1, 
		lastfont = -1;
static int 	laststyle = -1,
		lastindex = -1;
static int 	lasteffects = -1;


void	set_fillcolor(int col)
{
	if (col == lastfillcolor) 
		return;
	vsf_color(vdi_handle, col);
	lastfillcolor = col;
}

void	set_textcolor(int col)
{
	if (col == lasttextcolor) 
		return;
	vst_color(vdi_handle, col);
	lasttextcolor = col;
}

void	set_texteffects(int effects)
{
	if (effects == lasteffects) 
		return;
	lasteffects = vst_effects(vdi_handle, effects);
}

void	set_wrmode(int mode)
{
	if (mode == lastwrmode) 
		return;
	vswr_mode(vdi_handle, mode);
	lastwrmode = mode;
}

/*
 * This function sets both the font and the size (in points).
 */
void	set_font(int font, int height)
{
	short cw, ch, bw, bh;

	if (font != lastfont) 
	{
		vst_font(vdi_handle, font);
		lastfont = font;
		lastheight = height-1;
	}
	if (height != lastheight) 
		lastheight = vst_point(vdi_handle, height, &cw, &ch, &bw, &bh);
}

void set_fillstyle(int style, int nr)
{
	if (laststyle != style)
		laststyle = vsf_interior(vdi_handle, style);

	if (nr != lastindex)
		lastindex = vsf_style(vdi_handle, nr);
}

int alert(int def, int undo, int num)
{
	graf_mouse(ARROW, NULL);
	return do_walert(def, undo, rsc_string(num), " TosWin2 ");
}


void global_init(void)
{
	short work_out[57];
	
	rsrc_gaddr(R_TREE, WINICON, &winicon);
	rsrc_gaddr(R_TREE, CONICON, &conicon);
	rsrc_gaddr(R_FRSTR, 0, &stringarray);
	exit_code = 0;
	
	vdi_handle = open_vwork(work_out);
	font_anz = work_out[10];
	if (gl_gdos)
		font_anz += vst_load_fonts(vdi_handle, 0);
	
	init_ansi_colors (work_out);
}

void global_term(void)
{
	if (gl_gdos)
		vst_unload_fonts(vdi_handle, 0);
	v_clsvwk(vdi_handle);
}

/* Like memset bug WHAT is an unsigned long and SIZE
   denotes the number of unsigned longs to write.  */
void
memulset (void* dest, unsigned long what, size_t size)
{
	unsigned long* dst = (unsigned long*) dest;
	size_t chunk = 1;
	size_t max_chunk = size >> 1;
	
	if (size == 0)
		return;
	
	*dst++ = what;
	
	while (chunk < max_chunk) {
		memcpy (dst, dest, chunk * sizeof what);
		dst += chunk;
		chunk <<= 1;
	}
	
	chunk = size - chunk;
	if (chunk != 0)
		memcpy (dst, dest, chunk * sizeof what);
}

#ifdef MEMDEBUG

/* Memory debugging.  We benefit from a nitty-gritty feature of the 
   GNU linker.  The linker will resolve all references to SYMBOL
   with __wrap_SYMBOL and all references to __real_SYMBOL with
   the wrapped symbol.
   
   Note that we can neither use printf nor fprintf because stdio
   itself uses malloc and friends.  */
   
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static int fd_memdebug = -1;
static int tried = 0;

extern void* __real_malloc (size_t);
extern void* __real_calloc (size_t, size_t);
extern void* __real_realloc (void*, size_t);
extern void __real_free (void*);
extern void* __wrap_malloc (size_t);
extern void* __wrap_calloc (size_t, size_t);
extern void* __wrap_realloc (void*, size_t);
extern void __wrap_free (void*);
extern unsigned long _base;

#define FPUTS(s) (write (fd_memdebug, (s), strlen (s)))
#define PRINT_HEX(n) (print_hex ((unsigned long) n))
#define PRINT_CALLER \
	(PRINT_HEX ((unsigned long) __builtin_return_address (0) - \
	            (_base + 0x100UL)))

static
void print_hex (unsigned long hex)
{
	int shift;
	static char* hex_digits = "0123456789ABCDEF";
	static char outbuf[10];
	char* cursor = outbuf + 2;
	
	outbuf[0] = '0';
	outbuf[1] = 'x';
	
	for (shift = 28; shift >= 0; shift -= 4) {
		*cursor++ = hex_digits[0xf & (hex >> shift)];
	}
	write (fd_memdebug, outbuf, 10);
}

void*
__wrap_malloc (size_t bytes)
{
	void* retval;
	
	retval = __real_malloc (bytes);
	
	if (fd_memdebug < 0 && tried == 0) {
		fd_memdebug = open (MEMDEBUG, O_CREAT|O_WRONLY|O_TRUNC);
		++tried;
	}
	
	if (fd_memdebug >= 0) {
		FPUTS ("malloc: ");
		PRINT_CALLER;
		FPUTS (": ");
		PRINT_HEX (bytes);
		FPUTS (" bytes at ");
		PRINT_HEX (retval);
		FPUTS ("\n");
	}
	
	return retval;
}

void*
__wrap_calloc (size_t nmemb, size_t bytes)
{
	void* retval;
	
	retval = __real_calloc (nmemb, bytes);
	
	if (fd_memdebug < 0 && tried == 0) {
		fd_memdebug = open (MEMDEBUG, O_CREAT|O_WRONLY|O_TRUNC);
		++tried;
	}
	
	if (fd_memdebug >= 0) {
		FPUTS ("calloc: ");
		PRINT_CALLER;
		FPUTS (": ");
		PRINT_HEX (nmemb);
		FPUTS (" x ");
		PRINT_HEX (bytes);
		FPUTS (" bytes at ");
		PRINT_HEX (retval);
		FPUTS ("\n");
	}
	
	return retval;
}

void*
__wrap_realloc (void* where, size_t bytes)
{
	void* retval;
	void* old_buf = where;
	
	if (fd_memdebug < 0 && tried == 0) {
		fd_memdebug = open (MEMDEBUG, O_CREAT|O_WRONLY|O_TRUNC);
		++tried;
	}
	
	if (fd_memdebug >= 0) {
		/* Mainly to detect a segmentation fault due to
		   faulty realloc pointer.  */
		FPUTS ("pre_realloc: ");
		PRINT_CALLER;
		FPUTS (": to ");
		PRINT_HEX (bytes);
		FPUTS (" bytes at ");
		PRINT_HEX (where);
		FPUTS ("\n");
	}
	
	retval = __real_realloc (where, bytes);
	
	if (fd_memdebug >= 0) {
		FPUTS ("realloc: ");
		PRINT_CALLER;
		FPUTS (": ");
		PRINT_HEX (bytes);
		FPUTS (" from ");
		PRINT_HEX (old_buf);
		FPUTS (" to ");
		PRINT_HEX (retval);
		FPUTS ("\n");
	}
	
	return retval;
}

void
__wrap_free (void* where) 
{
	if (fd_memdebug < 0 && tried == 0) {
		fd_memdebug = open (MEMDEBUG, O_CREAT|O_WRONLY|O_TRUNC);
		++tried;
	}
	
	if (fd_memdebug) {
		FPUTS ("free: ");
		PRINT_CALLER;
		FPUTS (": ");
		FPUTS ("at ");
		PRINT_HEX (where);
		FPUTS ("\n");
	}
	
	__real_free (where);
}

#endif
