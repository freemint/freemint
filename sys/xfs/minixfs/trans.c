/* This file is part of 'minixfs' Copyright 1991,1992,1993 S.N. Henson */

# include "trans.h"


/* Decide whether to translate or not according to 'flag' this assumes 'flag'
 * is the tos domain form and flag<<1 is the mint domain form. This code has
 * been written so Domain() is called at most once .
 */

int
do_trans (long flag, int drive)
{
	long tmode = fs_mode[drive] & flag;
	long mmode = fs_mode[drive] & (flag << 1);
	
	/* If both modes, always translate */
	if (mmode && tmode)
		return 1;
	
	/* If neither, never */
	if (!mmode && !tmode)
		return 0;
	
	if (mmode) 
	{
		if (p_domain (-1) == DOM_MINT)
			return 1;
		else
			return 0;
	}
	
	if (tmode && (p_domain (-1) == DOM_TOS))
		return 1;
	
	return 0;
}


/*
 * I *HATE* this function, it attempts to turn a Minix filename into one
 * which wont cause TOS/DESKTOP etc to blow up, feel free to replace with
 * someting better (I doubt there could be much worse) 
 * if flag == 0 just return a null terminated version of 'name',
 * mnamlength is the maximum filename length
 */

char *
tosify (const char *name, int flag, char *first, int mnamlength)
{
	char *p, *ldt;
	int i;
	
	if (name[0] == '.'
		&& (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
	{
		return (char *) name;
	}
	
	strncpy (first, name, mnamlength);
	first[mnamlength] = 0;
	
	if (!flag)
		return first;
	
	ldt = strrchr (first, '.');
	if (ldt == first)
		ldt = NULL;
	
	p = first;
	for (i = 0; i < 8 && *p && p != ldt; i++, p++)
	{
		if (*p == '.')
			*p = ',';
		else
			*p = toupper (*p);
	}
	
	if (ldt && ldt[1])
	{
		*p++ = '.';
		ldt++;
		for (i = 0; i < 3 && *ldt; i++)
		{
			*p++ = toupper (*ldt); ldt++;
		}
	}
	*p = 0;
	
	return first;
}
