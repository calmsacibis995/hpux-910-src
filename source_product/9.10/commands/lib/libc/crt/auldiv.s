# @(#) $Revision: 66.3 $     
#
# auldiv(a,b) unsigned long *a, b; { *a /= b; }
#
# unsigned long division
# Author: Mark McDowell (8/3/84)
#
# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
	global	___auldiv
	sglobal	_auldiv
___auldiv:
_auldiv:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_auldiv(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_auldiv,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a0	#point to arguments
	move.l	(%a0)+,%a1	#pointer to dividend (x1,x0)
	move.l	(%a1),%d1		#get dividend
	cmp.l	%d1,(%a0)		#result is 0 if divisor > dividend
	bls.w	auldiv1
	tst.w	(%a0)+		#check y1
	bmi.w	auldiv2		#answer is 1 if MSB set; MSB of dividend is set
	bne.w	auldiv4		#check for short division
	move.l	%d1,%d0		#divisor is <= 16 bits
	clr.w	%d0		#two stage division; shift dividend right by 16
	swap	%d0		
	beq.w	auldiv3		#jump if dividend is <= 16 bits
	divu	(%a0),%d0		#r1 = x1 / y0
	move.w	%d0,(%a1)+	#save r1
	move.w	%d1,%d0		#remainder in upper word; move x0
	divu	(%a0),%d0		#r0 = x0 / y0
	move.w	%d0,(%a1)		#save r0
	rts
auldiv1:	beq.w	auldiv1a	#divisor = dividend?
	moveq	&0,%d0		#answer is 0
	move.l	%d0,(%a1)		#save result
	rts
auldiv1a: tst.l	%d1		#check for 0/0
	bne.b	auldiv2
	trap	&8
auldiv2:	moveq	&1,%d0		#answer is 1
	move.l	%d0,(%a1)		#save result
	rts
auldiv3:	divu	(%a0),%d1		#x1 = y1 = 0; simple division
	move.w	%d1,%d0		#upper word of d0 is 0; move result
	move.l	%d0,(%a1)		#save result
	rts
auldiv4:	move.l	(8,%sp),%d0	#get divisor
	move.l	%d2,%a0		#save d2
	moveq	&15,%d2		#shift count
auldiv5:	add.l	%d0,%d0		#shift divisor left until MSB is set
	dbmi	%d2,auldiv5	#at least one shift - remember previous check?
	lsr.l	%d2,%d1		#shift dividend right by correct amount
	swap	%d0		#shift left really amounted to shift right
	divu	%d0,%d1		#do the division (can't overflow)
	move.w	%d1,%d2		#move result; upper word of d2 is 0
	move.w	%d2,%d0		#r0
	mulu	(8,%sp),%d0	#r0 * y1
	mulu	(10,%sp),%d1	#r0 * y0
	swap	%d1		#form product
	add.w	%d0,%d1
	swap	%d1
	cmp.l	%d1,(%a1)		#is result too big?
	bls.w	auldiv6
	subq.w	&1,%d2		#correct result
auldiv6:	move.l	%d2,(%a1)		#store result
	move.l	%a0,%d2		#restore d2
	rts

ifdef(`PROFILE',`
		data
p_auldiv:	long	0
	')
