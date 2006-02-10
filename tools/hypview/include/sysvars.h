/*
	Die Systemvariablen, so wie sie im Profibuch eingetragen sind.
	(c) 20.02.98 by Ph. Donze
*/

#define	ETV_TIMER	(long *)0x0400
#define	ETV_CRITIC	(long *)0x0404
#define	ETV_TERM		(long *)0x0408
#define	ETV_XTRA		(long *)0x040C
#define	MEMVALID		(long *)0x0420			/*	Magic-Value: $752019F3	*/
#define	MEMCNTRL		(char *)0x0424
#define	RESVALID		(long *)0x0426
#define	RESVECTOR	(long *)0x042A
#define	PHYSTOP		(long *)0x042E
#define	_MEMBOT		(long *)0x0432
#define	_MEMTOP		(long *)0x0436
#define	MEMVAL2		(long *)0x043A			/*	Magic-Value: $237698AA	*/
#define	FLOCK			(short *)0x043E
#define	SEEKRATE		(short *)0x0440
#define	_TIMR_MS		(short *)0x0442
#define	_FVERIFY		(short *)0x0444
#define	_BOOTDEV		(short *)0x0446
#define	PALMODE		(short *)0x0448
#define	DEFSHIFTMD	(char *)0x044A
#define	SSHIFTMD		(char *)0x044C
#define	_V_BAS_AD	(long *)0x044E
#define	VBLSEM		(short *)0x0452
#define	NVBLS			(short *)0x0454
#define	_VBLQUEUE	(long *)0x0456
#define	COLORPTR		(long *)0x045A
#define	SCREENPT		(long *)0x045E
#define	_VBCLOCK		(long *)0x0462
#define	_FRCLOCK		(long *)0x0466
#define	HDV_INIT		(long *)0x046A
#define	SWV_VEC		(long *)0x046E
#define	HDV_BPB		(long *)0x0472
#define	HDV_RW		(long *)0x0476
#define	HDV_BOOT		(long *)0x047A
#define	HDV_MEDIACH	(long *)0x047E
#define	_CMDLOAD		(short *)0x0482
#define	CONTERM		(char *)0x0484
#define	TRP14RET		(long *)0x0486			/*	undef	*/
#define	CRITICRET	(long *)0x048A			/*	undef	*/
#define	THEMD			(long *)0x048E			/*	zeigt auf struct MD	*/
#define	_____MD		(long *)0x049E			/*	undef	*/
#define	SAVPTR		(long *)0x04A2
#define	_NFLOPS		(short *)0x04A6
#define	CON_STATE	(long *)0x04A8
#define	SAV_ROW		(short *)0x04AC
#define	SAV_CONTEXT	(long *)0x04AE
#define	_BUFL			(long *)0x04B2
#define	_HZ_200		(long *)0x04BA
#define	THE_ENV		(long *)0x04BE
#define	_DRVBITS		(long *)0x04C2
#define	_DSKBUFP		(long *)0x04C6
#define	_AUTOPATH	(long *)0x04CA
#define	_VBL_LIST	(long *)0x04CE
#define	PRT_CNT		(short *)0x04EE
#define	_PRTABT		(short *)0x04F0
#define	_SYSBASE		(long *)0x04F2
#define	_SHELL_P		(long *)0x04F6
#define	END_OS		(long *)0x04FA
#define	EXEC_OS		(long *)0x04FE
#define	SCR_DUMP		(long *)0x0502
#define	PRV_LSTO		(long *)0x0506
#define	PRV_LST		(long *)0x050A
#define	PRV_AUXO		(long *)0x050E
#define	PRV_AUX		(long *)0x0512
#define	PUN_PTR		(long *)0x0516
#define	MEMVAL3		(long *)0x051A			/*	Magic-value: $5555AAAA	*/
#define	XCONSTAT		(long *)0x051E
#define	XCONIN		(long *)0x053E
#define	XCOSTAT		(long *)0x055E
#define	XCONOUT		(long *)0x057E
#define	_LONGFRAME	(short *)0x059E
#define	_P_COOKIES	(long *)0x05A0
#define	RAMTOP		(long *)0x05A4
#define	RAMVALID		(long *)0x05A8			/* Magic-value: $1357BD13	*/
#define	BELL_HOOK	(long *)0x05AC
#define	KCL_HOOK		(long *)0x05B0
