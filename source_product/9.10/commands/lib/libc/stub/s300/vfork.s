# @(#) $Revision: 66.1 $
# C library -- _vfork
#
#
# pid = _vfork();
#
# d1 == 0 in parent process, d1 == 1 in child process.
# d0 == pid of child in parent, d0 == pid of parent in child.
#
# trickery here, based on VAX solution by keith sklower,
# pops return address off of the stack,
# and then returns with a jump indirect, since only one person can return
# with a ret off this stack...
#

include(defines.mh)
ifdef(`_NAMESPACE_CLEAN',`
	global	__vfork
	sglobal	_vfork',`
	global	_vfork
 ')
	global	__cerror
ifdef(`_NAMESPACE_CLEAN',`
__vfork:
')
_vfork:
	ifdef(`PROFILE',`
        ifdef(`PIC',`
        mov.l   &DLT,%a0
        lea.l   -6(%pc,%a0.l),%a0
        mov.l   p_vfork(%a0),%a0
        bsr.l   mcount',`
        mov.l   &p_vfork,%a0
        jsr     mcount
        ')
	')
	move.l	(%sp)+,%a0
	movq	&SYS_vfork,%d0
	trap	&0
	bcs.b	verror		## if c-bit not set, fork ok
	tst.l	%d1		## parent or child process ?
	beq.b 	parent
child:
	movq	&0,%d0		## return 0 in child, child pid in parent
parent:
	jmp	(%a0)

verror:
	move.l	%a0,-(%sp)	## restore return address for rts from __cerror
	ifdef(`PIC',`
	bra.l	__cerror',`
	jmp	__cerror
	')

ifdef(`PROFILE',`
		data
p_vfork:	long	0
	')
