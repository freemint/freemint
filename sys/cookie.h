/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * header file for dealing with the cookie jar
 *
 */

# ifndef _cookie_h
# define _cookie_h

# include "mint/mint.h"


/* cookie jar definition
 */
struct cookie
{
	long tag;
	long value;
};

/* the cookie base address
 */
# define CJAR		((struct cookie **) 0x5a0)

/* exported functions
 */
# if __KERNEL__ == 1
void	init_cookies	(void);
long	get_toscookie	(ulong tag, ulong *val);
long	set_toscookie	(ulong tag, ulong val);
long	get_cookie	(struct cookie *cj, ulong tag, ulong *ret);
long	set_cookie	(struct cookie *cj, ulong tag, ulong val);
long	del_cookie	(struct cookie *cj, ulong tag);
long	add_rsvfentry	(char *name, char portflags, char bdev);
long	del_rsvfentry	(char *name);
# endif

/* List of currently available tag values
 */

/* TOS cookies */
# define COOKIE__CPU	0x5f435055L
# define COOKIE__VDO	0x5f56444fL
# define COOKIE__FPU	0x5f465055L
# define COOKIE__FDC	0x5f464443L
# define COOKIE__SND	0x5f534e44L
# define COOKIE__MCH	0x5f4d4348L
# define COOKIE__SWI	0x5f535749L
# define COOKIE__FRB	0x5f465242L
# define COOKIE__FLK	0x5f464c4bL
# define COOKIE__NET	0x5f4e4554L
# define COOKIE__IDT	0x5f494454L
# define COOKIE__AKP	0x5f414b50L

/* MiNT cookies */
# define COOKIE_MiNT	0x4d694e54L
# define COOKIE_PMMU	0x504d4d55L

/* Some third party cookies */
# define COOKIE_XHDI	0x58484449L
# define COOKIE_SCSI	0x53435349L
# define COOKIE_NVDI	0x4e564449L
# define COOKIE_FSMC	0x46534d43L
# define COOKIE_RSVF	0x52535646L
# define COOKIE_FSEL	0x4653454CL
# define COOKIE_HBFS	0x48424653L /* BoxKite File Selector */

/* Not that we want to support these below ... */
# define COOKIE_STiK	0x5354694bL
# define COOKIE_ICIP	0x49434950L
# define COOKIE_NF	0x5F5F4E46L

/* hardware cookies */
# define COOKIE_CT60	0x43543630L
# define COOKIE_HADES	0x68616465L

/* values of MCH cookie
 */
# define ST		0
# define STE		0x00010000L
# define MEGASTE	0x00010010L
# define TT		0x00020000L
# define FALCON		0x00030000L
# define MILAN_C	0x00040000L

# endif /* _cookie_h */
