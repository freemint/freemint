
#ifndef tw_vt_h
# define tw_vt_h 1

#ifndef tw_textwin_h
# include "textwin.h"
#endif

void	paint		(TEXTWIN* tw, unsigned int c);
void	gotoxy		(TEXTWIN* tw, short x, short y);
void	clrline		(TEXTWIN* tw, short r);
void	clear		(TEXTWIN* tw);
void	clrchar		(TEXTWIN* tw, short x, short y);
void	clrfrom		(TEXTWIN* tw, short x1, short y1, short x2, short y2);
void	delete_char	(TEXTWIN* tw, short x, short y);
void	set_title	(TEXTWIN* tw);
void	set_size	(TEXTWIN* tw);

void	reset_tabs	(TEXTWIN* tw);

void	vt_quote_putch	(TEXTWIN* tw, unsigned int c);
void	vt100_putch	(TEXTWIN* tw, unsigned int c);
void	vt52_putch	(TEXTWIN* tw, unsigned int c);
void	delete_line	(TEXTWIN* tw);
void	insert_line	(TEXTWIN* tw);
void	reverse_cr	(TEXTWIN* tw);
void	new_line	(TEXTWIN* tw);

void	cuu		(TEXTWIN* tw, short n);
void	cuu1		(TEXTWIN* tw);
void	cud		(TEXTWIN* tw, short n);
void	cud1		(TEXTWIN* tw);
void	cub		(TEXTWIN* tw, short n);
void	cub1		(TEXTWIN* tw);
void	cuf		(TEXTWIN* tw, short n);
void	cuf1		(TEXTWIN* tw);

void	original_colors (TEXTWIN* tw);
void	inverse_video	(TEXTWIN* tw, int flag);
void	save_cursor	(TEXTWIN* tw);
void	restore_cursor	(TEXTWIN* tw);

void	vt_reset	(TEXTWIN* tw, bool full, bool saved);

#endif
