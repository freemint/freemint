# ifndef _mint_xbiosvecs_h
# define _mint_xbiosvecs_h

#include "struct_iorec.h"
#include "struct_kbdvbase.h"

struct pbdef;

/*
 * XBIOS function dispatcher.
 * Calls that are currently not used by MiNT start with an extra underscore
 */
typedef struct {
	void _cdecl (*_p_b_initmouse)(short type, void *par, void (*mousevec)(void *) );
	long _cdecl (*_p_b_ssbrk)(short count);
	void *_cdecl (*_p_b_physbase)(void);
	void *_cdecl (*_p_b_logbase)(void);
	short _cdecl (*p_b_getrez)(void);
	long _cdecl (*p_b_vsetscreen)(long log, long phys, int rez, int mode);
	void _cdecl (*_p_b_setpalette)(void *palptr);
	short _cdecl (*_p_b_setcolor)(short colornum, short color);
	short _cdecl (*_p_b_floprd)(void *buf, long filler, short devno, short sectno, short trackno, short sideno, short count);
	short _cdecl (*_p_b_flopwr)(void *buf, long filler, short devno, short sectno, short trackno, short sideno, short count);
	short _cdecl (*_p_b_flopfmt)(void *buf, long filler, short devno, short spt, short trackno, short sideno, short interlv, long magic, short virgin);
	void  _cdecl (*_p_b_dbmsg)(short rsrvd, short msg_num, long msg_arg);
	long _cdecl (*p_b_midiws)(int, const char *);
	void _cdecl (*_p_b_mfpint)(short number, short (*vector)(void));
	IOREC_T *_cdecl (*p_b_iorec)(short dev);
	long _cdecl (*p_b_rsconf)(int baud, int flow, int uc, int rs, int ts, int sc);

	struct keytab *_cdecl (*p_b_keytbl)(char *unshift, char *shift, char *caps);
	long _cdecl (*p_b_random)(void);
	void _cdecl (*_p_b_protobt)(void *buf, long serialno, short disktype, short execflag);
	short _cdecl (*_p_b_flopver)(void *buf, long filler, short devno, short sectno, short trackno, short sideno, short count);
	void _cdecl (*_p_b_scrdmp)(void);
	long _cdecl (*p_b_cursconf)(int, int);
	void _cdecl (*p_b_settime)(ulong datetime);
	long _cdecl (*p_b_gettime)(void);
	void _cdecl (*p_b_bioskey)(void);
	void _cdecl (*_p_b_ikbdws)(short count, const void *ptr);
	void _cdecl (*_p_b_jdisint)(short number);
	void _cdecl (*_p_b_jenabint)(short number);
	signed char _cdecl (*_p_b_giaccess)(short data, short regno);
	void _cdecl (*_p_b_offgibit)(short bitno);
	void _cdecl (*_p_b_ongibit)(short bitno);
	void _cdecl (*_p_b_xbtimer)(short timer, short control, short data, void (*vector)(void));

	long _cdecl (*p_b_dosound)(const char *p);
	short _cdecl (*_p_b_setprt)(short config);
	KBDVEC *_cdecl (*p_b_kbdvbase)(void);
	ushort _cdecl (*p_b_kbrate)(short delay, short rate);
	short _cdecl (*_p_b_prtblk)(struct pbdef *par);
	void _cdecl (*_p_b_vsync)(void);
	long _cdecl (*p_b_supexec)(Func funcptr, long a1, long a2, long a3, long a4, long a5);
	void _cdecl (*_p_b_puntaes)(void);
	short _cdecl (*_p_b_floprate)(short devno, short newrate);
	int16_t _cdecl (*_p_b_dmaread)(long sector, short count, void *buffer, short devno);
	int16_t _cdecl (*_p_b_dmawrite)(long sector, short count, void *buffer, short devno);
	long _cdecl (*_res_02b)(void);
	long _cdecl (*p_b_bconmap)(short dev);
	long _cdecl (*_res_02d)(void);
	int16_t _cdecl (*_p_b_nvmaccess)(short op, short start, short count, char *buffer);
	void _cdecl (*_p_waketime)(unsigned short date, unsigned short time);

	long _cdecl (*_res_030)(void);
	long _cdecl (*_res_031)(void);
	long _cdecl (*_res_032)(void);
	long _cdecl (*_res_033)(void);
	long _cdecl (*_res_034)(void);
	long _cdecl (*_res_035)(void);
	long _cdecl (*_res_036)(void);
	long _cdecl (*_res_037)(void);
	long _cdecl (*_res_038)(void);
	long _cdecl (*_res_039)(void);
	long _cdecl (*_res_03a)(void);
	long _cdecl (*_res_03b)(void);
	long _cdecl (*_res_03c)(void);
	long _cdecl (*_res_03d)(void);
	long _cdecl (*_res_03e)(void);
	long _cdecl (*_res_03f)(void);

	long _cdecl (*_res_040)(void);
	long _cdecl (*_res_041)(void);
	long _cdecl (*_res_042)(void);
	long _cdecl (*_res_043)(void);
	long _cdecl (*_res_044)(void);
	long _cdecl (*_res_045)(void);
	long _cdecl (*_res_046)(void);
	long _cdecl (*_res_047)(void);
	long _cdecl (*_res_048)(void);
	long _cdecl (*_res_049)(void);
	long _cdecl (*_res_04a)(void);
	long _cdecl (*_res_04b)(void);
	long _cdecl (*_res_04c)(void);
	long _cdecl (*_res_04d)(void);
	long _cdecl (*_res_04e)(void);
	long _cdecl (*_res_04f)(void);

	long _cdecl (*_res_050)(void);
	long _cdecl (*_res_051)(void);
	long _cdecl (*_res_052)(void);
	long _cdecl (*_res_053)(void);
	long _cdecl (*_res_054)(void);
	long _cdecl (*_res_055)(void);
	long _cdecl (*_res_056)(void);
	long _cdecl (*_res_057)(void);
	long _cdecl (*_res_058)(void);
	long _cdecl (*_res_059)(void);
	long _cdecl (*_res_05a)(void);
	long _cdecl (*_res_05b)(void);
	long _cdecl (*_res_05c)(void);
	long _cdecl (*_res_05d)(void);
	long _cdecl (*_res_05e)(void);
	long _cdecl (*_res_05f)(void);

	long _cdecl (*_res_060)(void);
	long _cdecl (*_res_061)(void);
	long _cdecl (*_res_062)(void);
	long _cdecl (*_res_063)(void);
	long _cdecl (*_res_064)(void);
	long _cdecl (*_res_065)(void);
	long _cdecl (*_res_066)(void);
	long _cdecl (*_res_067)(void);
	long _cdecl (*_res_068)(void);
	long _cdecl (*_res_069)(void);
	long _cdecl (*_res_06a)(void);
	long _cdecl (*_res_06b)(void);
	long _cdecl (*_res_06c)(void);
	long _cdecl (*_res_06d)(void);
	long _cdecl (*_res_06e)(void);
	long _cdecl (*_res_06f)(void);

	long _cdecl (*_res_070)(void);
	long _cdecl (*_res_071)(void);
	long _cdecl (*_res_072)(void);
	long _cdecl (*_res_073)(void);
	long _cdecl (*_res_074)(void);
	long _cdecl (*_res_075)(void);
	long _cdecl (*_res_076)(void);
	long _cdecl (*_res_077)(void);
	long _cdecl (*_res_078)(void);
	long _cdecl (*_res_079)(void);
	long _cdecl (*_res_07a)(void);
	long _cdecl (*_res_07b)(void);
	long _cdecl (*_res_07c)(void);
	long _cdecl (*_res_07d)(void);
	long _cdecl (*_res_07e)(void);
	long _cdecl (*_res_07f)(void);
} xbios_vecs;

#endif
