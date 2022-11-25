# @(#) $Revision: 66.3 $      
#
# ulmul(a,b) unsigned long a, b; { return (a * b); }
# unsigned long multiplication
#
# lmul(a,b) long a, b; { return (a * b); }
# long multiplication
#
# Author: Mark McDowell (8/3/84)
#
ifdef(`_NAMESPACE_CLEAN',`
	global	__ulmul,__lmul
	sglobal	_ulmul,_lmul',`
	global	_ulmul,_lmul
 ')
ifdef(`_NAMESPACE_CLEAN',`
__lmul:
__ulmul:	
 ')
_lmul:
_ulmul:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_lmul(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_lmul,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a1	#pointer to parameters
	move.l	(%a1)+,%d1	#multiplicand (x1,x0)
	move.w	%d1,%d0		#save (x0)
	move.w	(%a1)+,%d1	#d1 <- (x1,y1)
	tst.l	%d1		#if x1 and y1 are both zero
	bne.w	ulmul1			#it is a short multiply
	mulu	(%a1),%d0		#short multiply
	rts
ulmul1:	move.l	%d2,%a0		#save d2
	move.w	%d0,%d2		#x0
	mulu	(%a1),%d0		#x0 * y0
	mulu	%d1,%d2		#x0 * y1
	swap	%d1		#d1 <- (y1,x1)
	mulu	(%a1),%d1		#x1 * y0
	add.w	%d2,%d1		#x1 * y0 + x0 * y1
	swap 	%d0		#get upper 16 bits
	add.w	%d1,%d0		#sum for upper 16 bits
	swap	%d0		#into correct order
	move.l	%a0,%d2		#restore d2
	rts

ifdef(`PROFILE',`
		data
p_lmul:	long	0
	')
