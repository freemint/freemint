#ifndef _adidefs_h
#define _adidefs_h

#define ADI_OPEN	1
#define ADI_NAMSIZ	16

#include "adi/whlmoose/whlmoose.h"

struct adif
{
	struct adif	*next;
	long		class;

	char		*lname;
	char		name[ADI_NAMSIZ];
	short		unit;

	unsigned short	flags;

	long		(*open)		(struct adif *);
	long		(*close)	(struct adif *);
	long		resrvd1;	/* (*output) */
	long		(*ioctl)	(struct adif *, short, long);
	long		(*timeout)	(struct adif *);

	long		reserved[16];
};

struct adiinfo
{
	short		(*getfreeunit)	(char *);
	long		(*adi_register)	(struct adif *);

	void		(*move)		(struct adif *, short x, short y);
	void		(*button)	(struct adif *, struct moose_data *);
	void		(*wheel)	(struct adif *, struct moose_data *);

	const char	*fname;
};
#endif /* _adidefs_h */