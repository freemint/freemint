/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/*
 * Description: Definition of the basepage structure.
 */

# ifndef _mint_basepage_h
# define _mint_basepage_h


typedef struct basepage BASEPAGE;

struct basepage
{
	long		p_lowtpa;	/* pointer to self (bottom of TPA) */
	long		p_hitpa;	/* pointer to top of TPA + 1 */
	long		p_tbase;	/* base of text segment */
	long		p_tlen;		/* length of text segment */
	long		p_dbase;	/* base of data segment */
	long		p_dlen;		/* length of data segment */
	long		p_bbase;	/* base of BSS segment */
	long		p_blen;		/* length of BSS segment */
	char		*p_dta;		/* pointer to current DTA */
	BASEPAGE	*p_parent;	/* pointer to parent's basepage */
	long		p_flags;	/* memory usage flags */
	char		*p_env;		/* pointer to environment string */
	
	/* Anything after this (except for p_cmdlin) is undocumented,
	 * reserved, subject to change, etc. -- user programs must NOT use!
	 */
	char		p_devx[6];	/* real handles of the standard devices */
	char		p_res2;		/* reserved */
	char		p_defdrv;	/* default drv */
	long		p_undef[17];    /* reserved space */
	long		p_usp;		/* a fake USP to make dLibs programs happy */
	char		p_cmdlin[128];	/* command line image */
};


# endif /* _mint_basepage_h */
