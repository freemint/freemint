
#ifndef tw_vt_h_
#define tw_vt_h_

void	paint		(TEXTWIN *v, int c);
void	gotoxy		(TEXTWIN *v, int x, int y);
void	clrline		(TEXTWIN *v, int r);
void	clear		(TEXTWIN *v);
void	clrchar		(TEXTWIN *v, int x, int y);
void	clrfrom		(TEXTWIN *v, int x1, int y1, int x2, int y2);
void	delete_char	(TEXTWIN *v, int x, int y);
void	set_title	(TEXTWIN *v);
void	set_size	(TEXTWIN *v);

void	set_curs	(TEXTWIN *v, int on);
void	curs_on		(TEXTWIN *v);
void	curs_off	(TEXTWIN *v);
void	clearalltabs	(TEXTWIN *v);

void	vt100_putch	(TEXTWIN *v, int c);
void	vt52_putch	(TEXTWIN *v, int c);

void	original_colors (TEXTWIN *v);

#endif
