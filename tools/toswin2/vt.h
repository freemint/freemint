#ifndef _tw_vt_h_
#define _tw_vt_h_

extern void	paint			(TEXTWIN *v, int c);
extern void	gotoxy		(TEXTWIN *v, int x, int y);
extern void	clrline		(TEXTWIN *v, int r);
extern void	clear			(TEXTWIN *v);
extern void	clrchar		(TEXTWIN *v, int x, int y);
extern void	clrfrom		(TEXTWIN *v, int x1, int y1, int x2, int y2);
extern void	delete_char	(TEXTWIN *v, int x, int y);
extern void	set_title	(TEXTWIN *v);
extern void	set_size		(TEXTWIN *v);

extern void	set_curs			(TEXTWIN *v, int on);
extern void	curs_on			(TEXTWIN *v);
extern void	curs_off			(TEXTWIN *v);
extern void	clearalltabs	(TEXTWIN *v);

extern void	vt100_putch		(TEXTWIN *v, int c);
extern void	vt52_putch		(TEXTWIN *v, int c);


#endif
