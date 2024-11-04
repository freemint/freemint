/*
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
# define COOKIE__VDI	0x5f564449L
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

/* MiNT cookies (MiNT is now TOS) */
# define COOKIE_MiNT	0x4d694e54L
# define COOKIE_PMMU	0x504d4d55L
# define COOKIE__ISO	0x5f49534fL
# define COOKIE_EUSB	0x45555342L /* Ethernet USB API exported by inet4 */

/* Some third party cookies */
# define COOKIE_XHDI	0x58484449L
# define COOKIE_SCSI	0x53435349L
# define COOKIE_NVDI	0x4e564449L
# define COOKIE_fVDI	0x66564449L
# define COOKIE_NOVA	0x4e4f5641L
# define COOKIE_FSMC	0x46534d43L
# define COOKIE_RSVF	0x52535646L
# define COOKIE_FSEL	0x4653454CL
# define COOKIE_HBFS	0x48424653L /* BoxKite File Selector */
# define COOKIE_DMAC	0x444D4143L /* FireTOS DMA API */
# define COOKIE__5MS	0x5f354d53L /* 200 Hz system timer alternate vector */
# define COOKIE_SupV	0x53757056L /* SuperVidel XBIOS */

/* Not that we want to support these below ... */
# define COOKIE_STiK	0x5354694bL
# define COOKIE_ICIP	0x49434950L
# define COOKIE_NF	0x5F5F4E46L

/* hardware cookies */
# define COOKIE__CT2	0x5f435432L
# define COOKIE_CT60	0x43543630L
# define COOKIE_HADES	0x68616465L
# define COOKIE__PCI	0x5f504349L
# define COOKIE__CF_	0x5f43465fL /* ColdFire, set by FireTOS and EmuTOS */
# define COOKIE_RAVN	0x5241564eL

/* values of MCH cookie */
# define ST		0
# define STE		0x00010000L
# define STBOOK		0x00010001L
# define STEIDE		0x00010008L
# define MEGASTE	0x00010010L
# define TT		0x00020000L
# define FALCON		0x00030000L
# define MILAN_C	0x00040000L

# endif /* _cookie_h */
