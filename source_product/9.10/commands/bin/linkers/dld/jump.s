# HPUX_ID: @(#) $Revision: 70.1 $

#	jump.s
#
#	Series 300 dynamic loader
#
#	jump table for external entry points


	global	_shl_init, _shl_bor, _shl_term
	global	__shl_load, __shl_findsym, __shl_unload
	global	__shl_get
	global	flag_68010, flag_fpa, fpa_loc

	data
	bra.l	_shl_init
	bra.l	_shl_bor
	bra.l	__shl_load
	bra.l	__shl_findsym
	bra.l	__shl_unload
	bra.l	__shl_get
	bra.l	_shl_term
ifdef(`GETHANDLE',`
	bra.l	__shl_gethandle
')
flag_68010:
	short	0
flag_fpa:
	short	0
	set	fpa_loc,0xfff08000
