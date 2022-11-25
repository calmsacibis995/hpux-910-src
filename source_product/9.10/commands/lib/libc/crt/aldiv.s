# @(#) $Revision: 66.4 $      
#
# aldiv(a,b) long a, b; { *a /= b; }
#
# long division
# Author: Mark McDowell (8/6/84)
#
# Note: Although this function is designed to store its result indirectly 
# through a pointer, calling code may also require the result to be found 
# in register d0. Therefore, if any modifications are made to this code, 
# be certain d0 is correct on exit.
#
	global	___aldiv
	sglobal	_aldiv
___aldiv:
_aldiv:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_aldiv(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_aldiv,%a0
	jsr	mcount
')
	')
	move.l	(4,%sp),%a0	#get pointer to dividend
	move.l	(%a0),%d0		#get dividend (x1,x0)
	move.l	(8,%sp),%d1	#get divisor (y1,y0)
	move.l	%d0,%a1		#a1 <- dividend
	cmp.w	%a1,%a1		#is dividend short?
	bne.w	aldiv7		#jump if not
	move.l	%d1,%a1		#a1 <- divisor
	cmp.w	%a1,%a1		#is divisor short?
	bne.w	aldiv4		#if not, answer is probably 0
	divs	%d1,%d0		#short division
	bvs.w	aldiv1		#check for -32768 / -1
	ext.l	%d0		#clear out remainder and make into long result
	move.l	%d0,(%a0)		#store quotient
	rts
aldiv1:	neg.l	%d0		#-32768 / -1 so answer is -(-32768) = 32768
	move.l	%d0,(%a0)		#store result
	rts
aldiv2:	move.l	%a0,%d2		#restore d2
	move.l	(4,%sp),%a0	#pointer to result
aldiv3:	moveq	&0,%d0		#answer is 0 (i.e. divisor > dividend)
	move.l	%d0,(%a0)		#store quotient
	rts
aldiv4:	neg.l	%d1		#check for -32768 / 32768
	cmp.l	%d1,%d0		#is such the case?
	bne.w	aldiv3		#if not, answer is 0
aldiv5:	moveq	&-1,%d0		#answer is -1 (i.e. divisor = -dividend)
	move.l	%d0,(%a0)		#store quotient
	rts
aldiv6:	bhi.w	aldiv2		#jump if divisor > dividend
	move.l	%a0,%d2		#restore d2
	move.l	(4,%sp),%a0	#pointer to result
	tst.l	%d0		#is result positive or negative?
	bmi.w	aldiv5
	moveq	&1,%d0		#answer is 1 (i.e. divisor = dividend)
	move.l	%d0,(%a0)		#store quotient
	rts
aldiv7:	move.l	%d1,%a1		#a1 <- divisor
	cmp.w	%a1,%a1		#long dividend; is divisor long
	beq.w	aldiv14		#jump if not
	move.l	%d2,%a0		#save d2
	move.l	%d0,%d2		#result sign will be MSB of d0
	bpl.w	aldiv8		#is dividend positive?
	neg.l	%d2		#if not, make it so
aldiv8:	tst.l	%d1
	bpl.w	aldiv9		#is divisor positive?
	neg.l	%d1		#if not, make it so
	not.l	%d0		#change the sign of the result
	move.l	%d1,%a1		#a1 <- abs(divisor)
aldiv9:	cmp.l	%d1,%d2		#is divisor >= dividend?
	bcc.w	aldiv6		#if so, answer is 0, -1 or +1
	move.w	&15,%d0		#shift counter
aldiv10:	add.l	%d1,%d1		#shift divisor left
	dbmi	%d0,aldiv10		#until MSB = 1
	lsr.l	%d0,%d2		#shift dividend right by same amount
	swap	%d1		#shift left really amounted to shift right
	divu	%d1,%d2		#dividend / divisor (cannot overflow)
	move.w	%d2,%d0		#save quotient
	move.l	%a1,%d1		#get original divisor (y1,y0)
	mulu	%d1,%d2		#d2 <- r0 * y0
	swap	%d1		#d1 <- (y0,y1)
	mulu	%d0,%d1		#d1 <- r0 * y1
	swap	%d2		#prepare for product
	add.w	%d1,%d2		#add upper 16 bits
	swap	%d2		#shift back into place
	move.l	(4,%sp),%a1	#pointer to dividend
	move.l	(%a1),%d1		#get dividend
	bpl.w	aldiv11		#is it positive?
	neg.l	%d1		#if not, make it so
aldiv11:	cmp.l	%d1,%d2		#is quotient off?
	bcc.w	aldiv12		
	subq.w	&1,%d0		#if so, subtract 1
aldiv12:	move.l	%d0,%d1		#save sign and result
	moveq	&0,%d0		#initialize upper bits
	move.w	%d1,%d0		#set lower bits
	tst.l	%d1		#check sign of result
	bpl.w	aldiv13		#positive?
	neg.l	%d0		#no: make negative
aldiv13:	move.l	%d0,(%a1)		#store quotient
	move.l	%a0,%d2		#restore d2
	rts
aldiv14:	tst.l	%d0		#d0 <- dividend
	bpl.w	aldiv15		#is it positive?
	neg.l	%d0		#if not, make it so
	not.l	%d1		#change the sign of the result
aldiv15:	move.w	%a1,%d1		#get divisor
	bpl.w	aldiv16		#is it positive?
	neg.w	%d1		#if not, make it so
aldiv16:	move.w	%d0,%a1		#save lower 16 bits of dividend
	clr.w	%d0		#shift dividend right by 16
	swap	%d0
	divu	%d1,%d0		#first division
	move.w	%d0,(%a0)+	#save it
	move.w	%a1,%d0		#get lower 16 bits
	divu	%d1,%d0		#divide again
	move.w	%d0,(%a0)+	#save it
	tst.l	%d1		#sign of result?
	bpl.w	aldiv17		#is it positive?
	neg.l	-(%a0)		#no: change sign
	move.l	(%a0),%d0	#get result
	rts
aldiv17: move.l	-(%a0),%d0	#get result
	rts

ifdef(`PROFILE',`
		data
p_aldiv:	long	0
	')
