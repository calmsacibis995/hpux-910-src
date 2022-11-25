# @(#) $Revision: 66.3 $      
#
# ulrem(a,b) unsigned long a, b; { return (a % b); }
#
# unsigned long remainder
# Author: Mark McDowell (8/3/84)
#
	global	___ulrem
	sglobal	_ulrem
___ulrem:
_ulrem:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_ulrem(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_ulrem,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a0	#point to arguments
	move.l	(%a0)+,%d0	#get arguments: (x1,x0)
	move.l	(%a0),%d1			       #(y1,y0)
	cmp.l	%d1,%d0		#result is dividend if divisor > dividend
	bhi.w	ulrem1
	tst.w	(%a0)+		#check y1
	bmi.w	ulrem2		#answer is (dividend - divisor) if MSB set
	bne.w	ulrem4		#check for short division
	move.l	%d0,%d1		#divisor is <= 16 bits
	clr.w	%d0		#two stage division; shift dividend right by 16
	swap	%d0		
	beq.w	ulrem3		#jump if dividend is <= 16 bits
	divu	(%a0),%d0		#r1 = x1 / y0
	move.w	%d1,%d0		#remainder in upper word; move x0
	divu	(%a0),%d0		#r0 = x0 / y0
	clr.w	%d0		#clear quotient
	swap	%d0		#remainder in d0
ulrem1:	rts
ulrem2:	sub.l	%d1,%d0		#dividend - divisor
	rts
ulrem3:	divu	(%a0),%d1		#x1 = y1 = 0; simple division
	swap	%d1		#remainder to lower word
	move.w	%d1,%d0		#upper word of d0 is 0; move result
	rts
ulrem4:	move.l	%d2,%a0		#save d2
	move.l	%d1,%a1		#x1 <> 0; y1 <> 0; long division; save divisor
	moveq	&15,%d2		#shift count
ulrem5:	add.l	%d1,%d1		#shift divisor left until MSB is set
	dbmi	%d2,ulrem5	#at least one shift - remember previous check?
	lsr.l	%d2,%d0		#shift dividend right by correct amount
	swap	%d1		#shift left really amounted to shift right
	divu	%d1,%d0		#do the division (can't overflow)
	move.w	%d0,%d2		#save quotient
	move.l	%a1,%d1		#quotient may be off by 1, so check; get divisor
	mulu	%d1,%d0		#r0 * y0
	swap	%d1		#d1 <- y1
	mulu	%d2,%d1		#r0 * y1
	swap	%d0		#form product
	add.w	%d1,%d0
	swap	%d0
	sub.l	(4,%sp),%d0	#calculate remainder
	bls.w	ulrem6		#negative remainder?
	sub.l	%a1,%d0		#correct result
ulrem6:	neg.l	%d0		#give correct sign
	move.l	%a0,%d2		#restore d2
	rts

ifdef(`PROFILE',`
		data
p_ulrem:	long	0
	')
