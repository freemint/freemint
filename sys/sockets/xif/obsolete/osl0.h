#ifndef _SL0_H
#define _SL0_H

/* Utility/Hardware Interface functions */
extern void		sl0_set_dtr	(short);
extern void		sl0_set_rts	(short);
extern void		sl0_set_brk	(short);
extern char		sl0_get_dcd	(void);
extern char		sl0_get_cts	(void);
extern short		sl0_check_errors(void);
extern void		sl0_clear_errors(void);
extern void		sl0_send_byte	(unsigned char);
extern unsigned char	sl0_recv_byte	(void);
extern char		sl0_can_send	(void);
extern char		sl0_can_recv	(void);
extern short		sl0_init	(void);
extern void		sl0_open	(void);
extern void		sl0_close	(void);

/* Interrupt routines */
extern void		sl0_txerror	(void);
extern void		sl0_rxerror	(void);
extern void		sl0_txint	(void);
extern void		sl0_rxint	(void);
extern void		sl0_dcdint	(void);
extern void		sl0_ctsint	(void);

/* XBRA: old vectors */
extern long	sl0_old_txerror;
extern long	sl0_old_rxerror;
extern long	sl0_old_txint;
extern long	sl0_old_rxint;
extern long	sl0_old_dcdint;
extern long	sl0_old_ctsint;

extern char	sl0_on_off;

#endif /* _SL0_H */
