# @(#) $Revision: 66.1 $
#  C - library stime

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__stime
	sglobal	_stime',`
	global	_stime
 ')
	global	__cerror
ifdef(`_NAMESPACE_CLEAN',`
__stime:
 ')
_stime:
	ifdef(`PROFILE',`
        ifdef(`PIC',`
        mov.l   &DLT,%a0
        lea.l   -6(%pc,%a0.l),%a0
        mov.l   p_stime(%a0),%a0
        bsr.l   mcount',`
        mov.l   &p_stime,%a0
        jsr     mcount
        ')
	')
	movq	&SYS_stime,%d0
	move.l	(4,%sp),%a0
	move.l	(%a0),(4,%sp)		#| copy time to set
	trap	&0
	bcc.b	noerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
noerror:
	movq	&0,%d0
	rts

ifdef(`PROFILE',`
		data
p_stime:	long	0
	')
