# @(#) $Revision: 66.3 $
# C library -- cachectl

# retval = cachectl(opmask,address,length);

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global  __cachectl
	sglobal _cachectl',`
	global  _cachectl
 ')
	global	__cerror

ifdef(`_NAMESPACE_CLEAN',`
__cachectl:
')
_cachectl:
	ifdef(`PROFILE',`
	ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l   p_cachectl(%a0),%a0
	bsr.l	mcount',`
	mov.l   &p_cachectl,%a0
	jsr	mcount
	')
	')
	mov.l    4(%sp),%d0  # opmask
	mov.l    8(%sp),%a1  # start address
	mov.l   12(%sp),%d1  # length
	trap    &12
	mov.l   %d0,%d1
	beq.b   cc_ok
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')
cc_ok:
	rts

ifdef(`PROFILE',`
		data
p_cachectl: long    0
	')
