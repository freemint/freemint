
#ifndef _my_aes_h
#define _my_aes_h

extern short my_global_aes[];

/* gemlib */
extern short	mt_appl_init    (short *global_aes);
extern short	mt_appl_exit    (short *global_aes);
extern short	mt_graf_handle	(short *Wchar, short *Hchar, short *Wbox, short *Hbox, short *global_aes);

#endif /* _my_aes_h */
