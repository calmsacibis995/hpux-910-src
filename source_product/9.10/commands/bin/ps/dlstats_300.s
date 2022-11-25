        set     DSKLESS_STATS,184
	global	_dskless_stats
	global	__cerror
		
_dskless_stats:
	mov.l	&DSKLESS_STATS,%d0
	trap	&0
	bcc.w	noerror
	jmp	__cerror
noerror:		
	rts	
