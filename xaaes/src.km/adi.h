#ifndef _adi_h
#define _adi_h

#include "adi/adidefs.h"

long		adi_ioctl	(short cmd, long arg);
struct adif *	adi_name2adi	(char *aname);
short		adi_getfreeunit	(char *name);
long		adi_close	(struct adif *a);
long		adi_open	(struct adif *a);
long		adi_register	(struct adif *a);



#endif /* _adi_h */