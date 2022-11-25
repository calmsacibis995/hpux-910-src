# @(#) $Revision: 66.3 $     
#
# aulmul(a,b) unsigned long *a, b; { *a *= b; }
# unsigned long multiplication
#
# almul(a,b) long *a, b; { *a *= b; }
# long multiplication
#
# Author: Mark McDowell (8/3/84)
#
ifdef(`_NAMESPACE_CLEAN',`
	global	__aulmul,__almul
	sglobal	_aulmul,_almul',`
	global	_aulmul,_almul
 ')
ifdef(`_NAMESPACE_CLEAN',`
__almul:
__aulmul:	
')
_almul:
_aulmul:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_almul(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_almul,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a1	#point to parameters
	move.l	(%a1)+,%a0	#get pointer to multiplicand (x1,x0)
	move.l	(%a0),%d0		#get multiplicand
	move.w	%d0,%d1		#save (x0)
	move.w	(%a1)+,%d0	#d0 <- (x1,y1)
	tst.l	%d0		#if x1 and y1 are both zero
	bne.w	aulmul1			#it is a short multiply
	mulu	(%a1),%d1		#short multiply
	move.l	%d1,(%a0)		#store result
	rts
aulmul1:	move.l	%d2,%a0		#save d2
	move.w	%d1,%d2		#x0
	mulu	(%a1),%d1		#x0 * y0
	mulu	%d0,%d2		#x0 * y1
	swap	%d0		#d0 <- (y1,x1)
	mulu	(%a1),%d0		#x1 * y0
	add.w	%d2,%d0		#x1 * y0 + x0 * y1
	swap	%d1		#upper 16 bits of x0 * y0
	add.w	%d0,%d1		#upper 16 bits result
	swap	%d1		#put into correct order
	move.l	%a0,%d2		#restore d2
	move.l	(4,%sp),%a0	#pointer to result
	move.l	%d1,(%a0)		#store result
	rts

ifdef(`PROFILE',`
		data
p_almul:	long	0
	')
