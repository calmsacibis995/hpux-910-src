# @(#) $Revision: 66.3 $      
#
# uldiv(a,b) unsigned long a, b; { return (a / b); }
#
# unsigned long division
# Author: Mark McDowell (8/3/84)
#
	global	___uldiv
	sglobal	_uldiv
___uldiv:
_uldiv:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l  &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_uldiv(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_uldiv,%a0
	jsr	mcount
')
	')
	lea	(4,%sp),%a0	#point to arguments
	move.l	(%a0)+,%d1	#get arguments: (x1,x0)
	move.l	(%a0),%a1			       #(y1,y0)
	cmp.l	%a1,%d1		#result is 0 or 1 if divisor >= dividend
	bcc.w	uldiv1
	tst.w	(%a0)+		#check y1
	bmi.w	uldiv2		#answer is 1 if MSB set; MSB of dividend is set
	bne.w	uldiv4		#check for short division
	move.l	%d1,%d0		#divisor is <= 16 bits
	clr.w	%d0		#two stage division; shift dividend right by 16
	swap	%d0		
	beq.w	uldiv3		#jump if dividend is <= 16 bits
	divu	(%a0),%d0		#r1 = x1 / y0
	move.w	%d0,%a1		#save r1
	move.w	%d1,%d0		#remainder in upper word; move x0
	divu	(%a0),%d0		#r0 = x0 / y0
	swap	%d0		#get ready to combine result
	move.w	%a1,%d0		#d0 <- (r0,r1)
	swap	%d0		#d0 <- (r1,r0) the result
	rts
uldiv1:  beq.w	uldiv1a		#divisor = dividend?
	moveq	&0,%d0		#answer is 0
	rts
uldiv1a: tst.l	%d1		#check for 0/0
	bne.b	uldiv2
	trap	&8
uldiv2:	moveq	&1,%d0		#answer is 1
	rts
uldiv3:	divu	(%a0),%d1		#x1 = y1 = 0; simple division
	move.w	%d1,%d0		#upper word of d0 is 0; move result
	rts
uldiv4:	move.l	%d2,%a0		#save d2
	move.l	%a1,%d2		#x1 <> 0; y1 <> 0; long division; save divisor
	moveq	&15,%d0		#shift count
uldiv5:	add.l	%d2,%d2		#shift divisor left until MSB is set
	dbmi	%d0,uldiv5	#at least one shift - remember previous check?
	lsr.l	%d0,%d1		#shift dividend right by correct amount
	swap	%d2		#shift left really amounted to shift right
	divu	%d2,%d1		#do the division (can't overflow)
	move.w	%d1,%d0		#move result; upper word of d0 is 0
	move.l	%a1,%d2		#answer could be off by 1, so check; get divisor
	mulu	%d2,%d1		#r0 * y0
	swap	%d2		#d2 <- y1
	mulu	%d0,%d2		#r0 * y1
	swap	%d1		#form product
	add.w	%d2,%d1
	swap	%d1
	cmp.l	%d1,(4,%sp)	#is result too big?
	bls.w	uldiv6
	subq.w	&1,%d0		#correct result
uldiv6:	move.l	%a0,%d2		#restore d2
	rts

ifdef(`PROFILE',`
		data
p_uldiv:	long	0
	')
