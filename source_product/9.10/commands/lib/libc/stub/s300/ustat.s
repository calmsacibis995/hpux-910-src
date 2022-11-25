# @(#) $Revision: 66.1 $
# C library -- ustat

include(defines.mh)
	set     USTAT,2
ifdef(`_NAMESPACE_CLEAN',`
	global	__ustat
	sglobal	_ustat',`
	global	_ustat
 ')
	global	__cerror
ifdef(`_NAMESPACE_CLEAN',`
__ustat:
 ')
_ustat:
	ifdef(`PROFILE',`
        ifdef(`PIC',`
        mov.l   &DLT,%a0
        lea.l   -6(%pc,%a0.l),%a0
        mov.l   p_ustat(%a0),%a0
        bsr.l   mcount',`
        mov.l   &p_ustat,%a0
        jsr     mcount
        ')
	')
	move.l	&USTAT,-(%sp)
	move.l	(8,%sp),-(%sp)		#|device
	move.l	(16,%sp),-(%sp)		#|buffer
	ifdef(`PIC',`
	bsr	sys',`
	jsr	sys
	')
	add.l	&12,%sp
	bcc.b	noerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
	rts

sys:
	movq	&SYS_utssys,%d0
	trap	&0
	rts

noerror:
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_ustat:	long	0
	')
