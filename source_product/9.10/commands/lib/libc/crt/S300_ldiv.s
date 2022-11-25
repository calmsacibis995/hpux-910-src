# @(#) $Revision: 66.3 $       
#
# ldiv(a,b) long a, b; { return (a / b); }
#
# long division
# Author: Mark McDowell (8/3/84)
#
	global	___ldiv
___ldiv:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_ldiv(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_ldiv,%a0
	jsr	mcount
')
	')
	move.l	(4,%sp),%a0	#get dividend (x1,x0)
	move.l	(8,%sp),%a1	#get divisor (y1,y0)
	move.l	%a1,%d1		#move divisor
	cmp.w	%a0,%a0		#is dividend short?
	bne.w	ldiv7		#jump if not
	cmp.w	%a1,%a1		#is divisor short?
	bne.w	ldiv4		#if not, answer is probably 0
	move.l	%a0,%d0		#move dividend
	divs	%d1,%d0		#short division
	bvs.w	ldiv1		#check for -32768 / -1
	ext.l	%d0		#clear out remainder and make into long result
	rts
ldiv1:	neg.l	%d0		#-32768 / -1 so answer is -(-32768) = 32768
	rts
ldiv2:	move.l	%a0,%d2		#restore d2
ldiv3:	moveq	&0,%d0		#abs(divisor) > abs(dividend) so answer is 0
	rts
ldiv4:	neg.l	%d1		#check for -32768 / 32768
	cmp.l	%d1,%a0		#is such the case?
	bne.w	ldiv3
ldiv5:	moveq	&-1,%d0		#divisor = -dividend so answer is -1
	rts
ldiv6:	bhi.w	ldiv2		#divisor = dividend?
	move.l	%a0,%d2		#restore d2
	tst.l	%d0		#sign of result?
	bmi.w	ldiv5
	moveq	&1,%d0		#divisor = dividend so answer is 1
	rts
ldiv7:	cmp.w	%a1,%a1		#long dividend; is divisor long
	beq.w	ldiv14		#jump if not
	exg	%d2,%a0		#save d2; d2 <- dividend
	move.l	%d2,%d0		#result sign will be MSB of d0
	bpl.w	ldiv8		#is dividend positive?
	neg.l	%d2		#if not, make it so
ldiv8:	tst.l	%d1		#is divisor positive?
	bpl.w	ldiv9
	neg.l	%d1		#if not, make it so
	not.l	%d0		#change the sign of the result
	move.l	%d1,%a1		#a1 <- abs(divisor)
ldiv9:	cmp.l	%d1,%d2		#is divisor >= dividend?
	bcc.w	ldiv6		#if so, answer is 0, -1 or +1
	move.w	&15,%d0		#shift counter
ldiv10:	add.l	%d1,%d1		#shift divisor left
	dbmi	%d0,ldiv10		#until MSB = 1
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
	move.l	(4,%sp),%d1	#get original dividend
	bpl.w	ldiv11		#is it positive?
	neg.l	%d1		#if not, make it so
ldiv11:	cmp.l	%d1,%d2		#is quotient off?
	bcc.w	ldiv12		
	subq.w	&1,%d0		#if so, subtract 1
ldiv12:	move.l	%d0,%d1		#save sign and result
	moveq	&0,%d0		#initialize upper bits
	move.w	%d1,%d0		#set lower bits
	tst.l	%d1		#check sign of result
	bpl.w	ldiv13		#positive?
	neg.l	%d0		#no: make negative
ldiv13:	move.l	%a0,%d2		#restore d2
	rts
ldiv14:	move.l	%a0,%d0		#d0 <- dividend
	bpl.w	ldiv15		#is it positive?
	neg.l	%d0		#if not, make it so
	move.w	%d0,%a0		#save lower 16 bits
	not.l	%d1		#change the sign of the result
ldiv15:	move.w	%a1,%d1		#get divisor
	bpl.w	ldiv16		#is it positive?
	neg.w	%d1		#if not, make it so
ldiv16:	clr.w	%d0		#shift dividend right by 16
	swap	%d0
	divu	%d1,%d0		#first division
	move.w	%d0,%a1		#save it
	move.w	%a0,%d0		#get lower 16 bits
	divu	%d1,%d0		#divide again
	swap	%d0		#form result
	move.w	%a1,%d0		#get upper 16 bits of quotient
	swap	%d0		#put in proper order
	tst.l	%d1		#sign of result?
	bpl.w	ldiv17		#is it positive?
	neg.l	%d0		#no: change sign
ldiv17:	rts

ifdef(`PROFILE',`
		data
p_ldiv:	long	0
	')
