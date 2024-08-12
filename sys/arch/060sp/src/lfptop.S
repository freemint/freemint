;
; lfptop.s:
;	This file is appended to the top of the 060ILSP package
; and contains the entry points into the package. The user, in
; effect, branches to one of the branch table entries located here.
;

	bra.l	_facoss_
	.dc.w	0x0000
	bra.l	_facosd_
	.dc.w	0x0000
	bra.l	_facosx_
	.dc.w	0x0000

	bra.l	_fasins_
	.dc.w	0x0000
	bra.l	_fasind_
	.dc.w	0x0000
	bra.l	_fasinx_
	.dc.w	0x0000

	bra.l	_fatans_
	.dc.w	0x0000
	bra.l	_fatand_
	.dc.w	0x0000
	bra.l	_fatanx_
	.dc.w	0x0000

	bra.l	_fatanhs_
	.dc.w	0x0000
	bra.l	_fatanhd_
	.dc.w	0x0000
	bra.l	_fatanhx_
	.dc.w	0x0000

	bra.l	_fcoss_
	.dc.w	0x0000
	bra.l	_fcosd_
	.dc.w	0x0000
	bra.l	_fcosx_
	.dc.w	0x0000

	bra.l	_fcoshs_
	.dc.w	0x0000
	bra.l	_fcoshd_
	.dc.w	0x0000
	bra.l	_fcoshx_
	.dc.w	0x0000

	bra.l	_fetoxs_
	.dc.w	0x0000
	bra.l	_fetoxd_
	.dc.w	0x0000
	bra.l	_fetoxx_
	.dc.w	0x0000

	bra.l	_fetoxm1s_
	.dc.w	0x0000
	bra.l	_fetoxm1d_
	.dc.w	0x0000
	bra.l	_fetoxm1x_
	.dc.w	0x0000

	bra.l	_fgetexps_
	.dc.w	0x0000
	bra.l	_fgetexpd_
	.dc.w	0x0000
	bra.l	_fgetexpx_
	.dc.w	0x0000

	bra.l	_fgetmans_
	.dc.w	0x0000
	bra.l	_fgetmand_
	.dc.w	0x0000
	bra.l	_fgetmanx_
	.dc.w	0x0000

	bra.l	_flog10s_
	.dc.w	0x0000
	bra.l	_flog10d_
	.dc.w	0x0000
	bra.l	_flog10x_
	.dc.w	0x0000

	bra.l	_flog2s_
	.dc.w	0x0000
	bra.l	_flog2d_
	.dc.w	0x0000
	bra.l	_flog2x_
	.dc.w	0x0000

	bra.l	_flogns_
	.dc.w	0x0000
	bra.l	_flognd_
	.dc.w	0x0000
	bra.l	_flognx_
	.dc.w	0x0000

	bra.l	_flognp1s_
	.dc.w	0x0000
	bra.l	_flognp1d_
	.dc.w	0x0000
	bra.l	_flognp1x_
	.dc.w	0x0000

	bra.l	_fmods_
	.dc.w	0x0000
	bra.l	_fmodd_
	.dc.w	0x0000
	bra.l	_fmodx_
	.dc.w	0x0000

	bra.l	_frems_
	.dc.w	0x0000
	bra.l	_fremd_
	.dc.w	0x0000
	bra.l	_fremx_
	.dc.w	0x0000

	bra.l	_fscales_
	.dc.w	0x0000
	bra.l	_fscaled_
	.dc.w	0x0000
	bra.l	_fscalex_
	.dc.w	0x0000

	bra.l	_fsins_
	.dc.w	0x0000
	bra.l	_fsind_
	.dc.w	0x0000
	bra.l	_fsinx_
	.dc.w	0x0000

	bra.l	_fsincoss_
	.dc.w	0x0000
	bra.l	_fsincosd_
	.dc.w	0x0000
	bra.l	_fsincosx_
	.dc.w	0x0000

	bra.l	_fsinhs_
	.dc.w	0x0000
	bra.l	_fsinhd_
	.dc.w	0x0000
	bra.l	_fsinhx_
	.dc.w	0x0000

	bra.l	_ftans_
	.dc.w	0x0000
	bra.l	_ftand_
	.dc.w	0x0000
	bra.l	_ftanx_
	.dc.w	0x0000

	bra.l	_ftanhs_
	.dc.w	0x0000
	bra.l	_ftanhd_
	.dc.w	0x0000
	bra.l	_ftanhx_
	.dc.w	0x0000

	bra.l	_ftentoxs_
	.dc.w	0x0000
	bra.l	_ftentoxd_
	.dc.w	0x0000
	bra.l	_ftentoxx_
	.dc.w	0x0000

	bra.l	_ftwotoxs_
	.dc.w	0x0000
	bra.l	_ftwotoxd_
	.dc.w	0x0000
	bra.l	_ftwotoxx_
	.dc.w	0x0000

	bra.l	_fabss_
	.dc.w	0x0000
	bra.l	_fabsd_
	.dc.w	0x0000
	bra.l	_fabsx_
	.dc.w	0x0000

	bra.l	_fadds_
	.dc.w	0x0000
	bra.l	_faddd_
	.dc.w	0x0000
	bra.l	_faddx_
	.dc.w	0x0000

	bra.l	_fdivs_
	.dc.w	0x0000
	bra.l	_fdivd_
	.dc.w	0x0000
	bra.l	_fdivx_
	.dc.w	0x0000

	bra.l	_fints_
	.dc.w	0x0000
	bra.l	_fintd_
	.dc.w	0x0000
	bra.l	_fintx_
	.dc.w	0x0000

	bra.l	_fintrzs_
	.dc.w	0x0000
	bra.l	_fintrzd_
	.dc.w	0x0000
	bra.l	_fintrzx_
	.dc.w	0x0000

	bra.l	_fmuls_
	.dc.w	0x0000
	bra.l	_fmuld_
	.dc.w	0x0000
	bra.l	_fmulx_
	.dc.w	0x0000

	bra.l	_fnegs_
	.dc.w	0x0000
	bra.l	_fnegd_
	.dc.w	0x0000
	bra.l	_fnegx_
	.dc.w	0x0000

	bra.l	_fsqrts_
	.dc.w	0x0000
	bra.l	_fsqrtd_
	.dc.w	0x0000
	bra.l	_fsqrtx_
	.dc.w	0x0000

	bra.l	_fsubs_
	.dc.w	0x0000
	bra.l	_fsubd_
	.dc.w	0x0000
	bra.l	_fsubx_
	.dc.w	0x0000

; leave room for future possible additions
	.balign	0x400

