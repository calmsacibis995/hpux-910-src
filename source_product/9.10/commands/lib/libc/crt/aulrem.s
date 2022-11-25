# @(#) $Revision: 66.3 $     
#
# aulrem(a,b) unsigned long *a, b; { *a %= b; }
#
# unsigned long remainder
# Author: Mark McDowell (8/3/84)
#
# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
	global	___aulrem
	sglobal	_aulrem
___aulrem:
_aulrem:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_aulrem(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_aulrem,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a0	#point to arguments
	move.l	(%a0)+,%a1	#pointer to dividend (x1,x0)
	move.l	(%a1),%d0		#get dividend
	move.l	(%a0),%d1		#get divisor
	cmp.l	%d1,%d0		#result is dividend if divisor > dividend
	bhi.w	aulrem1
	tst.w	(%a0)+		#check y1
	bmi.w	aulrem2		#answer is 1 if MSB set; MSB of dividend is set
	bne.w	aulrem4		#check for short division
	move.l	%d0,%d1		#divisor is <= 16 bits
	clr.w	%d0		#two stage division; shift dividend right by 16
	swap	%d0		
	beq.w	aulrem3		#jump if dividend is <= 16 bits
	divu	(%a0),%d0		#r1 = x1 / y0
	move.w	%d1,%d0		#remainder in upper word; move x0
	divu	(%a0),%d0		#r0 = x0 / y0
	clr.w	%d0		#remove quotient
	swap	%d0		#get remainder
	move.l	%d0,(%a1)		#store result
aulrem1:	rts
aulrem2:	sub.l	%d1,%d0		#remainder is dividend - divisor
	move.l	%d0,(%a1)		#store result
	rts
aulrem3:	divu	(%a0),%d1		#x1 = y1 = 0; simple division
	swap	%d1
	move.w	%d1,%d0		#upper word of d0 is 0; move result
	move.l	%d0,(%a1)		#store result
	rts
aulrem4:	move.l	%d2,%a0		#save d2
	moveq	&15,%d2		#shift count
aulrem5:	add.l	%d1,%d1		#shift divisor left until MSB is set
	dbmi	%d2,aulrem5	#at least one shift - remember previous check?
	lsr.l	%d2,%d0		#shift dividend right by correct amount
	swap	%d1		#shift left really amounted to shift right
	divu	%d1,%d0		#do the division (can't overflow)
	move.w	%d0,%d1		#move quotient
	mulu	(8,%sp),%d0	#r0 * y1
	mulu	(10,%sp),%d1	#r0 * y0
	swap	%d1		#form product
	add.w	%d0,%d1
	swap	%d1
	sub.l	(%a1),%d1		#calculate remainder
	bls.w	aulrem6		#is result too big?
	sub.l	(8,%sp),%d1	#correct result
aulrem6:	neg.l	%d1		#make positive
	move.l	%d1,(%a1)		#store result
	move.l	%a0,%d2		#restore d2
	rts

ifdef(`PROFILE',`
		data
p_aulrem:	long	0
	')
