
#include "adi.h"
#include "xa_global.h"

static struct moose_data md;

static struct adif *alladifs = 0;

long
adi_register(struct adif *a)
{
	DIAGS(("adi_register: Registered device %s (%s)", a->name, a->lname));
	a->next = alladifs;
	alladifs = a;

	return 0;
}

long
adi_open(struct adif *a)
{
	long error;

	error = (*a->open)(a);
	if (error)
	{
		DIAGS(("adi_open: Cannot open aes device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags |= ADI_OPEN;
	return 0;
}

long
adi_close(struct adif *a)
{
	long error;

	error = (*a->close)(a);
	if (error)
	{
		DIAGS(("adi_close: Cannot close AES device interface %s%d", a->name, a->unit));
		return error;
	}
	a->flags &= ~ADI_OPEN;
	return 0;
}

/*
 * Get an unused unit number for interface name 'name'
 */
short
adi_getfreeunit (char *name)
{
	struct adif *adip;
	short max = -1;
	
	for (adip = alladifs; adip; adip = adip->next)
	{
		if (!strncmp (adip->name, name, ADI_NAMSIZ) && adip->unit > max)
			max = adip->unit;
	}
	
	return max+1;
}

struct adif *
adi_name2adi (char *aname)
{
	char name[ADI_NAMSIZ+1], *cp;
	short i;
	long unit = 0;
	struct adif *a;
	
	for (i = 0, cp = aname; i < ADI_NAMSIZ && *cp; ++cp, ++i)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			unit = atol (cp);
			break;
		}
		name[i] = *cp;
	}
	
	name[i] = '\0';
	for (a = alladifs; a; a = a->next)
	{
		if (!stricmp (a->name, name) && a->unit == unit)
			return a;
	}
	
	return NULL;
}

long
adi_ioctl(short cmd, long arg)
{
	return 0;
}
			