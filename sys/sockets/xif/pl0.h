
# ifndef _pl0_h
# define _pl0_h


void	pl0_set_strobe		(short);
void	pl0_set_direction	(short);
long	pl0_send_pkt		(short, char *, long);
long	pl0_recv_pkt		(short, char *, long);
char	pl0_got_ack		(void);
void	pl0_send_ack		(void);
void	pl0_cli			(void);
void	pl0_sti			(void);
short	pl0_get_busy		(void);

short	pl0_init		(void);
void	pl0_open		(void);
void	pl0_close		(void);

void	pl0_busy_int		(void);

extern long pl0_old_busy_int;


# endif /* _pl0_h */
