# @(#) $Revision: 66.3 $      
#
# alrem(a,b) long a, b; { return (a % b); }
#
# long remainder
# Author: Mark McDowell (8/6/84)
#
# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
	global	___alrem
	sglobal	_alrem
___alrem:
_alrem:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l   &DLT,%a0
	lea.l   -6(%pc,%a0.l),%a0
	mov.l	p_alrem(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_alrem,%a0
	jsr	mcount
')
	')
	move.l	(4,%sp),%a0	#get pointer to dividend
	move.l	(%a0),%d0		#get dividend (a1,a0)
	move.l	(8,%sp),%d1	#get divisor (b1,b0)
	move.l	%d0,%a1		#a1 <- dividend
	cmp.w	%a1,%a1		#is dividend short?
	bne.w	alrem5		#jump if not
	move.l	%d1,%a1		#a1 <- divisor
	cmp.w	%a1,%a1		#is divisor short?
	bne.w	alrem3		#if not, answer is probably dividend
	divs	%d1,%d0		#short division
	bvs.w	alrem2		#check for -32768 % -1
	swap	%d0		#get remainder
	ext.l	%d0		#extend remainder
	move.l	%d0,(%a0)		#store remainder
	rts
alrem1:	bhi.w	alrem4
alrem2:	moveq	&0,%d0		#divisor = dividend; answer is 0
	move.l	%d0,(%a0)
	rts
alrem3:	neg.l	%d1		#check for -32768 % 32768
	cmp.l	%d1,%d0		#is such the case?
	beq.w	alrem2
alrem4:	rts
alrem5:	move.l	%d1,%a1		#a1 <- divisor
	cmp.w	%a1,%a1		#long dividend; is divisor long?
	beq.w	alrem12		#jump if not
	tst.l	%d0		#check sign of dividend
	bpl.w	alrem6		#is dividend positive?
	neg.l	%d0		#if not, make it so
alrem6:	tst.l	%d1		#check sign of divisor
	bpl.w	alrem7		#is divisor positive?
	neg.l	%d1		#if not, make it so
	move.l	%d1,%a1		#save abs(divisor)
alrem7:	cmp.l	%d1,%d0		#is divisor >= dividend?
	bcc.w	alrem1		#if so, answer is dividend or 0
	move.l	%d2,%a0		#save d2
	moveq	&15,%d2		#shift counter
alrem8:	add.l	%d1,%d1		#shift divisor left
	dbmi	%d2,alrem8		#until MSB = 1
	lsr.l	%d2,%d0		#shift dividend right by same amount
	swap	%d1		#shift left really amounted to shift right
	divu	%d1,%d0		#dividend / divisor (cannot overflow)
	move.w	%d0,%d1		#save quotient
	move.l	%a1,%d2		#get divisor (b1,b0)
	mulu	%d2,%d1		#d2 <- r0 * b0
	swap	%d2		#d1 <- (b0,b1)
	mulu	%d0,%d2		#d1 <- r0 * b1
	swap	%d1		#prepare for product
	add.w	%d2,%d1		#add upper 16 bits
	swap	%d1		#shift back into place
	move.l	%a0,%d2		#restore d2
	move.l	(4,%sp),%a0	#get pointer to dividend
	move.l	(%a0),%d0		#get dividend
	bpl.w	alrem9		#is it positive?
	neg.l	%d0		#if not, make it so
alrem9:	sub.l	%d1,%d0		#calculate remainder
	bcc.w	alrem10		#is remainder positive?
	add.l	%a1,%d0		#if not, add back a divisor
alrem10:	tst.b	(%a0)		#check sign of dividend
	bpl.w	alrem11		#positive?
	neg.l	%d0		#no: make result negative
alrem11:	move.l	%d0,(%a0)		#store result
	rts
alrem12:	tst.l	%d0		#long dividend; short divisor
	bpl.w	alrem13		#is dividend positive?
	neg.l	%d0		#if not, make it so
alrem13:	tst.w	%d1		#check sign of divisor
	bpl.w	alrem14		#is it positive?
	neg.w	%d1		#if not, make it so
alrem14:	move.w	%d0,%a1		#save lower 16 bits of dividend
	clr.w	%d0		#shift dividend right by 16
	swap	%d0
	divu	%d1,%d0		#first division
	move.w	%a1,%d0		#get lower 16 bits
	divu	%d1,%d0		#divide again
	swap	%d0		#get remainder
	tst.b	(%a0)		#check sign of dividend
	bpl.w	alrem15		#is it positive?
	neg.w	%d0		#if not, change sign of remainder
alrem15:	ext.l	%d0		#extend remainder
	move.l	%d0,(%a0)		#store remainder
	rts

ifdef(`PROFILE',`
		data
p_alrem:	long	0
	')
