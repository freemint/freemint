
#ifndef tw_vt_h_
#define tw_vt_h_

void	paint		(TEXTWIN* tw, unsigned int c);
void	gotoxy		(TEXTWIN* tw, short x, short y);
void	clrline		(TEXTWIN* tw, short r);
void	clear		(TEXTWIN* tw);
void	clrchar		(TEXTWIN* tw, short x, short y);
void	clrfrom		(TEXTWIN* tw, short x1, short y1, short x2, short y2);
void	delete_char	(TEXTWIN* tw, short x, short y);
void	set_title	(TEXTWIN* tw);
void	set_size	(TEXTWIN* tw);

#if 0
void	set_curs	(TEXTWIN* tw, int on);
void	curs_on		(TEXTWIN* tw);
void	curs_off	(TEXTWIN* tw);
#endif
void	reset_tabs	(TEXTWIN* tw);

void	vt_quote_putch	(TEXTWIN* tw, unsigned int c);
void	vt100_putch	(TEXTWIN* tw, unsigned int c);
void	vt52_putch	(TEXTWIN* tw, unsigned int c);
void	delete_line	(TEXTWIN* tw, int row, int end);
void	insert_line	(TEXTWIN* tw, int row, int end);

void	original_colors (TEXTWIN* tw);

#endif
