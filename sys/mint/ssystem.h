/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _mint_ssystem_h
# define _mint_ssystem_h

# define HAS_SSYSTEM
# define S_OSNAME	0
# define S_OSXNAME	1
# define S_OSVERSION	2
# define S_OSHEADER	3
# define S_OSBUILDDATE	4
# define S_OSBUILDTIME	5
# define S_OSCOMPILE	6
# define S_OSFEATURES	7
# define S_GETCOOKIE	8
# define S_SETCOOKIE	9
# define S_GETLVAL	10
# define S_GETWVAL	11
# define S_GETBVAL	12
# define S_SETLVAL	13
# define S_SETWVAL	14
# define S_SETBVAL	15
# define S_SECLEVEL	16
# define S_RUNLEVEL	17	/* currently disabled, reserved */
# define S_TSLICE	18
# define S_FASTLOAD	19
# define S_SYNCTIME	20
# define S_BLOCKCACHE	21
# define S_FLUSHCACHE	22
# define S_CTRLCACHE	23
# define S_INITIALTPA	24
# define S_CTRLALTDEL	25	/* ctraltdel behavoiur */
# define S_DELCOOKIE	26
# define S_LOADKBD	27	/* reload the keyboard table */
# define S_SETEXC	28	/* if 0 only kernel-processes may change sys-vectors via Setexc */
# define S_GETBOOTLOG	29	/* bootlog filepath - arg1 pointer to a buffer of arg2 len */
# define S_CLOCKUTC	100
# define S_TIOCMGET	0x54f8	/* 21752 */

/* Bit defines for S_CTRLCACHE mode */

# define CTRLCACHE_EIC		(1L << 0)	/* Enable Instruction Cache (020-030-040-060-cfv4e) */
# define CTRLCACHE_EDC		(1L << 1)	/* Enable Data Cache (030-040-060-cfv4e) */
# define CTRLCACHE_EBC		(1l << 2)	/* Enable Branch Cache (060-cfv4e) */
# define CTRLCACHE_FI		(1L << 3)	/* Freeze Instruction Cache (020-030) */
# define CTRLCACHE_FD		(1L << 4)	/* Freeze Data Cache (030) */
# define CTRLCACHE_IBE		(1L << 5)	/* Enable Instruction Burst (030) */
# define CTRLCACHE_DBE		(1L << 6)	/* Enable Data Burst (030) */
# define CTRLCACHE_WA		(1L << 7)	/* Enable Data Write Allocate (030) */
# define CTRLCACHE_FIC		(1L << 8)	/* Enable Instruction Cache Full Mode (060) */
# define CTRLCACHE_NAI		(1L << 9)	/* Enable Instruction Cache Allocate Mode (060) */
# define CTRLCACHE_FOC		(1L << 10)	/* Enable Data Cache Full Mode (060) */
# define CTRLCACHE_NAD		(1L << 11)	/* Enable Data Cache Allocate Mode (060) */
# define CTRLCACHE_CABC		(1L << 12)	/* Branch Cache Invalidate All (060-cfv4e) */
# define CTRLCACHE_CUBC 	(1L << 13)	/* Invalidate User Entries Branch Cache (060) */
# define CTRLCACHE_DPI		(1L << 14)	/* Enable CPUSH Invalidate Data Cache (060-cfv4e) */
# define CTRLCACHE_ESB		(1L << 15)	/* Enable Store Buffer (060-cfv4e) */
# define CTRLCACHE_ICINVA	(1L << 16)	/* Invalidate All Instruction Cache (cfv4e) */
# define CTRLCACHE_IDCM		(1L << 17)	/* Instruction Cache Default Mode (cf4e) */
# define CTRLCACHE_IHLCK	(1L << 18)	/* Enable Instruction Cache Half Lock (cfv4e) */
# define CTRLCACHE_IDPI		(1L << 19)	/* Enable CPUSH Invalidate Instruction Cache (cfv4e) */
# define CTRLCACHE_DNFB		(1L << 20)	/* Enable Cache-Inhibited Fill Buffer (cfv4e) */
# define CTRLCACHE_DCINVA	(1L << 21)	/* Invalidate All Data Cache (cfv4e) */
# define CTRLCACHE_DDCM_WT	(0L << 22)	/* Data Cache Default Mode Writethrough (cfv4e) */
# define CTRLCACHE_DDCM_CB	(1L << 22)	/* Data Cache Default Mode Copyback (cfv4e) */
# define CTRLCACHE_DDCM_CIP	(2L << 22)	/* Data Cache Default Mode Inhibited Precise (cfv4e) */
# define CTRLCACHE_DDCM_CII	(3l << 22)	/* Data Cache Default Mode Inhibited Imprecise (cfv4e) */
# define CTRLCACHE_DHLCK	(1L << 24)	/* Enable Data Cache Half Lock (cfv4e) */

/* additional informations about the kernel
 * reserved 900 - 999
 */
# define S_KNAME	900	/* kernel name - arg1 pointer to a buffer of arg2 len */
# define S_CNAME	910	/* compiler name - arg1 pointer to a buffer of arg2 len */
# define S_CVERSION	911	/* compiler version - arg1 pointer to a buffer of arg2 len */
# define S_CDEFINES	912	/* compiler definitions - arg1 pointer to a buffer of arg2 len */
# define S_COPTIM	913	/* compiler flags - arg1 pointer to a buffer of arg2 len */

/* debug section
 * reserved 1000 - 1999
 */
# define S_DEBUGLEVEL	1000	/* debug level */
# define S_DEBUGDEVICE	1001	/* BIOS device number */
# define S_DEBUGKMTRACE	1100	/* KM_TRACE debug feature */

# endif /* _mint_ssystem_h */
