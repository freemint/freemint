; here are the macros...

;
; The copy_parameters macro is called from GEMDOS_handler.
; It copies the parameters from where the application placed them to the
; current (supervisor) stack so that the called routine can use them
; The parameter is the number of _words_ to copy (currently up to 10)
;
define(`copy_parameters',
`ifelse($1, `0', `',
`	move.l	C_A0(a0),a1		; get pointer to parameters from C_A0(a0)'
)

ifelse(
$1, `10', `	adda.w	#20,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)'
,
$1, `9', `	adda.w	#18,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.w	-(a1),-(sp)'
,
$1, `8', `	adda.w	#16,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)'
,
$1, `7', `	adda.w	#14,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.w	-(a1),-(sp)'
,
$1, `6', `	adda.w	#12,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)'
,
$1, `5', `	adda.w	#10,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.w	-(a1),-(sp)'
,
$1, `4', `	addq.w	#8,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)'
,
$1, `3', `	addq.w	#6,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.w	-(a1),-(sp)'
,
$1, `2', `	move.l (a1),-(sp)'
,
$1, `1', `	move.w (a1),-(sp)'
,
$1, `0', `'
,
`	adda.w	#20,a1 		; set a1 behind last parameter
	
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)
	move.l	-(a1),-(sp)'
)')

;
; The remove_parameters macro removes the parameters placed on the 
; stack by the above copy parameters macro.
; The parameters is the number of words to remove
;
define(`remove_parameters', 
`ifelse(
$1, `10', `	adda.w	#20,sp',
$1, `9', `	adda.w	#18,sp',
$1, `8', `	adda.w	#16,sp',
$1, `7', `	adda.w	#14,sp',
$1, `6', `	adda.w	#12,sp',
$1, `5', `	adda.w	#10,sp',
$1, `4', `	addq.w	#8,sp',
$1, `3', `	addq.w	#6,sp',
$1, `2', `	addq.w	#4,sp',
$1, `1', `	addq.w	#2,sp',
$1, `0', `',
         `	adda.w	#20,sp')'
)

;
; The macro create_GEMDOS_stub creates a stub (for a GEMDOS function)
; to be called from the TraPatch chain. It only places the address of the
; function to call in a1 and calls a GEMDOS handler (depending on the number
; of parameters)
; Parameters: 1st: name of the routine called
;             2nd: number of words(!) the called routine needs as parameters
;
define(
`create_GEMDOS_stub',
`;**********************************************************
;** $1 stub for TraPatch chain
;*

	XDEF _stub_$1
	
	dc.l	`$'58425241		; XBRA
	dc.l	`$'54726150		; TraP
	dc.l	0
	
_stub_$1:
	
	lea	_$1,a1			; address of function to call
	bra	GEMDOS_$2parm_handler
	
')dnl
dnl

;
; The macro GEMDOS_handler creates the part of MiNT's gemdos handler in 
; the TraPatch chain, which is shared by all functions with the same number
; of parameters. Therefore it takes the number of parameter words as a parameter
;

define(
`GEMDOS_handler',
`
	XDEF GEMDOS_$1parm_handler
GEMDOS_$1parm_handler:
	
	clr.w	-(sp)			; no frame format needed
	move.l	_curproc,d0
	addq.l	#4,d0			; make it a pointer to context area
	move.l	d0,-(sp)		; push pointer to syscall context save
	jsr	_build_context

; copy parameters onto process stack. a0 was set by _build_context

	move.l	-4(a0),sp		; this puts us in our private stack
	
copy_parameters($1)
	
	move.l	C_A1(a0),-(sp)		; address of function to call
	
after_workaround_$1:
	
	jsr	_enter_gemdos		; set up vectors appropriately

; check here to see if we need to flush the Bconout() buffer

	tst.w	_bconbsiz		; characters in buffer?
	beq.s	noflush_G$1		; no: OK to proceed

; watch out, this could cause a context switch

	jsr	_bflush 		; flush the buffer

noflush_G$1:
	
	move.l	(sp)+,a0
	jsr	(a0)			; GO! Do the call!
	
remove_parameters($1)
	
	jmp	out_GEMDOS

')dnl
dnl

;
; The macro create_workaround_stub creates a stub which is very similar
; to the beginning of the gemdos_handler created by the GEMDOS handler 
; macro but it also contains a specialised workaround for bugs in 
; MiNT lib < 0.51.
; This macro shall only be called for Pvfork, Psigblock and Psigsetmask!
; The parameters are the same as for create GEMDOS stub:
;     1st: name of the routine called
;     2nd: number of words(!) the called routine needs as parameters
;

define(
`create_workaround_stub',
`;********************************************************************************************
;** $1 stub for TraPatch chain (with workaround for bug in MiNT lib < 0.51
;*
	XDEF _stub_$1
_stub_$1:
	
	clr.w	-(sp)				; no frame format needed
	move.l	_curproc,d0
	addq.l	#4,d0				; for compatibility
	move.l	d0,-(sp)			; push pointer to syscall context save
	
	jsr	_build_context
	
; copy parameters onto process stack. a0 and a1 were set by _build_context
	
	move.l	-4(a0),sp			; this puts us in our private stack
	
copy_parameters($2)
	
; Part 2 of the workaround fpr MiNT lib bug

ifelse($1, `p_vfork',
`	move.l	C_WA_VFORK(a0),C_A1(a0)		; save the register in the place from where it will be restored by _restore_context',
$1, `p_sigblock',
`	move.l	C_WA_SIGBLOCK(a0),C_A0(a0)	; save the register in the place from where it will be restored by _restore_context',
$1, `p_sigsetmask',
`	move.l	C_WA_SIGSETMASK(a0),C_A0(a0)	; save the register in the place from where it will be restored by _restore_context'
)
	
; end of workarounds

	move.l	#_$1,-(sp)			; address of function to call
	
	bra	after_workaround_$2

')dnl
dnl

	
; now the code...
	
	XDEF	out_GEMDOS
	
`%include "magic.i"'

; here are the handlers for n words of parameters

GEMDOS_handler(0)
GEMDOS_handler(1)
GEMDOS_handler(2)
GEMDOS_handler(3)
GEMDOS_handler(4)
GEMDOS_handler(5)
GEMDOS_handler(6)
GEMDOS_handler(7)
GEMDOS_handler(8)
GEMDOS_handler(9)
GEMDOS_handler(10)

; here are the stubs with the name of the function to call and
; it's number of parameter _words_ (i.e. 16 Bit)

create_GEMDOS_stub(p_term0,		 0)
create_GEMDOS_stub(c_conin,		 0)
create_GEMDOS_stub(c_conout,		 1)
create_GEMDOS_stub(c_auxin,		 0)
create_GEMDOS_stub(c_auxout,		 1)
create_GEMDOS_stub(c_prnout,		 1)
create_GEMDOS_stub(c_rawio,		 1)
create_GEMDOS_stub(c_rawcin,		 0)
create_GEMDOS_stub(c_necin,		 0)
create_GEMDOS_stub(c_conws,		 2)
create_GEMDOS_stub(c_conrs,		 2)
create_GEMDOS_stub(c_conis,		 0)
create_GEMDOS_stub(d_setdrv,		 1)
create_GEMDOS_stub(c_conos,		 0)
create_GEMDOS_stub(c_prnos,		 0)
create_GEMDOS_stub(c_auxis,		 0)
create_GEMDOS_stub(c_auxos,		 0)
create_GEMDOS_stub(m_addalt,		 4)
create_GEMDOS_stub(s_realloc,		 2)
create_GEMDOS_stub(s_lbopen,		 10)
create_GEMDOS_stub(s_lbclose,		 4)
create_GEMDOS_stub(d_getdrv,		 0)
create_GEMDOS_stub(f_setdta,		 2)
create_GEMDOS_stub(s_uper,		 2)
create_GEMDOS_stub(t_getdate,		 0)
create_GEMDOS_stub(t_setdate,		 1)
create_GEMDOS_stub(t_gettime,		 0)
create_GEMDOS_stub(t_settime,		 1)
create_GEMDOS_stub(f_getdta,		 0)
create_GEMDOS_stub(s_version,		 0)
create_GEMDOS_stub(p_termres,		 3)
create_GEMDOS_stub(d_free,		 3)
create_GEMDOS_stub(d_create,		 2)
create_GEMDOS_stub(d_delete,		 2)
create_GEMDOS_stub(d_setpath,		 2)
create_GEMDOS_stub(f_create,		 3)
create_GEMDOS_stub(f_open,		 3)
create_GEMDOS_stub(f_close,		 1)
create_GEMDOS_stub(f_read,		 5)
create_GEMDOS_stub(f_write,		 5)
create_GEMDOS_stub(f_delete,		 2)
create_GEMDOS_stub(f_seek,		 4)
create_GEMDOS_stub(f_attrib,		 4)
create_GEMDOS_stub(m_xalloc,		 3)
create_GEMDOS_stub(f_dup,		 1)
create_GEMDOS_stub(f_force,		 2)
create_GEMDOS_stub(d_getpath,		 3)
create_GEMDOS_stub(m_alloc,		 2)
create_GEMDOS_stub(m_free,		 2)
create_GEMDOS_stub(m_shrink,		 5)
create_GEMDOS_stub(p_exec,		 7)
create_GEMDOS_stub(p_term,		 1)
create_GEMDOS_stub(f_sfirst,		 3)
create_GEMDOS_stub(f_snext,		 0)
create_GEMDOS_stub(f_rename,		 5)
create_GEMDOS_stub(f_datime,		 4)
create_GEMDOS_stub(f_lock,		 6)

create_GEMDOS_stub(s_yield,		 0)
create_GEMDOS_stub(f_pipe,		 2)
create_GEMDOS_stub(f_fchown,		 3)
create_GEMDOS_stub(f_fchmod,		 2)
create_GEMDOS_stub(f_cntl,		 4)
create_GEMDOS_stub(f_instat,		 1)
create_GEMDOS_stub(f_outstat,		 1)
create_GEMDOS_stub(f_getchar,		 2)
create_GEMDOS_stub(f_putchar,		 4)
create_GEMDOS_stub(p_wait,		 0)
create_GEMDOS_stub(p_nice,		 1)
create_GEMDOS_stub(p_getpid,		 0)
create_GEMDOS_stub(p_getppid,		 0)
create_GEMDOS_stub(p_getpgrp,		 0)
create_GEMDOS_stub(p_setpgrp,		 2)
create_GEMDOS_stub(p_getuid,		 0)
create_GEMDOS_stub(p_setuid,		 1)
create_GEMDOS_stub(p_kill,		 2)
create_GEMDOS_stub(p_signal,		 3)
create_workaround_stub(p_vfork,		 0)
create_GEMDOS_stub(p_getgid,		 0)
create_GEMDOS_stub(p_setgid,		 1)
create_workaround_stub(p_sigblock,	 2)
create_workaround_stub(p_sigsetmask,	 2)
create_GEMDOS_stub(p_usrval,		 2)
create_GEMDOS_stub(p_domain,		 1)
create_GEMDOS_stub(p_sigreturn,		 0)
create_GEMDOS_stub(p_fork,		 0)
create_GEMDOS_stub(p_wait3,		 3)
create_GEMDOS_stub(f_select,		 7)
create_GEMDOS_stub(p_rusage,		 2)
create_GEMDOS_stub(p_setlimit,		 3)
create_GEMDOS_stub(t_alarm,		 2)
create_GEMDOS_stub(p_pause,		 0)
create_GEMDOS_stub(s_ysconf,		 1)
create_GEMDOS_stub(p_sigpending,	 0)
create_GEMDOS_stub(d_pathconf,		 3)
create_GEMDOS_stub(p_msg,		 5)
create_GEMDOS_stub(f_midipipe,		 3)
create_GEMDOS_stub(p_renice,		 2)
create_GEMDOS_stub(d_opendir,		 3)
create_GEMDOS_stub(d_readdir,		 5)
create_GEMDOS_stub(d_rewind,		 2)
create_GEMDOS_stub(d_closedir,		 2)
create_GEMDOS_stub(f_xattr,		 5)
create_GEMDOS_stub(f_link,		 4)
create_GEMDOS_stub(f_symlink,		 4)
create_GEMDOS_stub(f_readlink,		 5)
create_GEMDOS_stub(d_cntl,		 5)
create_GEMDOS_stub(f_chown,		 4)
create_GEMDOS_stub(f_chmod,		 3)
create_GEMDOS_stub(p_umask,		 1)
create_GEMDOS_stub(p_semaphore,		 5)
create_GEMDOS_stub(d_lock,		 2)
create_GEMDOS_stub(p_sigpause,		 2)
create_GEMDOS_stub(p_sigaction,		 5)
create_GEMDOS_stub(p_geteuid,		 0)
create_GEMDOS_stub(p_getegid,		 0)
create_GEMDOS_stub(p_waitpid,		 4)
create_GEMDOS_stub(d_getcwd,		 4)
create_GEMDOS_stub(s_alert,		 2)
create_GEMDOS_stub(t_malarm,		 2)
create_GEMDOS_stub(p_sigintr,		 2)
create_GEMDOS_stub(s_uptime,		 4)
create_GEMDOS_stub(s_trapatch,		 8)
create_GEMDOS_stub(d_xreaddir,		 9)
create_GEMDOS_stub(p_seteuid,		 1)
create_GEMDOS_stub(p_setegid,		 1)
create_GEMDOS_stub(p_getauid,		 0)
create_GEMDOS_stub(p_setauid,		 1)
create_GEMDOS_stub(p_getgroups,		 3)
create_GEMDOS_stub(p_setgroups,		 3)
create_GEMDOS_stub(t_setitimer,		 9)
create_GEMDOS_stub(d_chroot,		 2)
create_GEMDOS_stub(f_stat,		 5)
create_GEMDOS_stub(p_setreuid,		 2)
create_GEMDOS_stub(p_setregid,		 2)
create_GEMDOS_stub(s_ync,		 0)
create_GEMDOS_stub(s_hutdown,		 2)
create_GEMDOS_stub(d_readlabel,		 5)
create_GEMDOS_stub(d_writelabel,	 4)
create_GEMDOS_stub(s_system,		 5)
create_GEMDOS_stub(t_gettimeofday,	 4)
create_GEMDOS_stub(t_settimeofday,	 4)
create_GEMDOS_stub(p_getpriority,	 2)
create_GEMDOS_stub(p_setpriority,	 3)
