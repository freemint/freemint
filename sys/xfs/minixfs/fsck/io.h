
# ifndef _io_h
# define _io_h

# include "global.h"


void	read_blocks	(ulong start, ushort count, void *buff);
void	write_blocks	(ulong start, ushort count, void *buff);
int	init_device	(char *name, int rw);
void	close_device	(void);
int	set_size	(long nblocks);


# endif /* _io_h */
