/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _bios_h
# define _bios_h

# include "mint/mint.h"
# include "biosfs.h"


extern char *kbshft;

long _cdecl sys_b_tickcal (void);
long _cdecl sys_b_drvmap (void);
long _cdecl sys_b_kbshift (int mode);
long _cdecl sys_b_mediach (int dev);
struct bpb *_cdecl sys_b_getbpb (int dev);
long _cdecl sys_b_rwabs (int rwflag, void *buffer, int number, int recno, int dev, long lrecno);
long _cdecl sys_b_setexc (int number, long vector);

extern IOREC_T *keyrec; /* keyboard i/o record pointer */
extern BCONMAP2_T *bconmap2;
#define MAPTAB (bconmap2->maptab)
extern short kintr;

ushort ionwrite (IOREC_T *wrec);
ushort ionread (IOREC_T *irec);
ushort btty_ionread (struct bios_tty *b);

void checkbtty_nsig (struct bios_tty *b);
void checkbttys (void);
void checkbttys_vbl (void);

void	init_bios (void);
long	overlay_bdevmap (int dev, BDEVMAP *newmap);

long _cdecl sys_b_ubconstat (int dev);
long _cdecl sys_b_ubconin (int dev);
long _cdecl sys_b_ubconout (int dev, int c);
long _cdecl sys_b_ubcostat (int dev);
long _cdecl sys_b_ursconf (int baud, int flow, int uc, int rs, int ts, int sc);

long bconstat (int dev);
long bconin (int dev);
long bconout (int dev, int c);
long bcostat (int dev);

extern short bconbsiz;
extern char bconbuf[256];
extern short bconbdev;

long _cdecl bflush (void);
long _cdecl do_bconin (int dev);
int checkkeys (void);

# endif /* _bios_h */
