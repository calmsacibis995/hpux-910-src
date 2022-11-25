# @(#) $Revision: 66.3 $       
#
ifdef(`_NAMESPACE_CLEAN',`
	global	___fneg
	sglobal	_fneg',`
	global	_fneg
')
#*****************************************************************************
#
#  Procedure _fneg
#
#  Author: Paul Beiser   3/2/82
#
#  Description:
#       Return the negation of a real number.
#
#  Parameters:
#       4(sp)   - the real to be negated
#
#  The result is returned in (d0,d1).
#
#  Error conditions: None
#
ifdef(`_NAMESPACE_CLEAN',`
___fneg:
')
_fneg:   
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fneg(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fneg,%a0
	jsr	mcount
')
	')
	link     %a6,&0	
	movem.l	(8,%a6),%d0/%d1
	tst.l	%d0		#see if zero
	beq.w	fdone		#if zero, finished
	   bchg	   &31,%d0	   #else change the sign
fdone:	unlk    %a6
	rts	

ifdef(`PROFILE',`
		data
p_fneg:	long	0
	')
