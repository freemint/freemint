#ifndef _mint_biosvecs_h
#define _mint_biosvecs_h

struct mpb;
struct bpb;

/*
 * XBIOS function dispatcher.
 * Calls that are currently not used by MiNT start with an extra underscore
 */
typedef struct {
	void _cdecl (*p_b_getmpb)(struct mpb *ptr);
	long _cdecl (*p_b_bconstat)(int dev);
	long _cdecl (*p_b_bconin)(int dev);
	long _cdecl (*p_b_bconout)(int dev, int c);
	long _cdecl (*p_b_rwabs)(int rwflag, void *buffer, int number, int recno, int dev, long lrecno);
	long _cdecl (*p_b_setexc)(int number, long vector);
	long _cdecl (*p_b_tickcal)(void);
	struct bpb *_cdecl(*p_b_getbpb)(int dev);
	long _cdecl (*p_b_bcostat)(int dev);
	long _cdecl (*p_b_mediach)(int dev);
	long _cdecl (*p_b_drvmap)(void);
	long _cdecl (*p_b_kbshift)(int mode);
	long _cdecl (*_res_00c)(void);
	long _cdecl (*_res_00d)(void);
	long _cdecl (*_res_00e)(void);
	long _cdecl (*_res_00f)(void);

	long _cdecl (*_res_010)(void);
	long _cdecl (*_res_011)(void);
	long _cdecl (*_res_012)(void);
	long _cdecl (*_res_013)(void);
	long _cdecl (*_res_014)(void);
	long _cdecl (*_res_015)(void);
	long _cdecl (*_res_016)(void);
	long _cdecl (*_res_017)(void);
	long _cdecl (*_res_018)(void);
	long _cdecl (*_res_019)(void);
	long _cdecl (*_res_01a)(void);
	long _cdecl (*_res_01b)(void);
	long _cdecl (*_res_01c)(void);
	long _cdecl (*_res_01d)(void);
	long _cdecl (*_res_01e)(void);
	long _cdecl (*_res_01f)(void);
} bios_vecs;

#endif
