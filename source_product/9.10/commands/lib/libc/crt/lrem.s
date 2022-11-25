# @(#) $Revision: 66.3 $       
#
# lrem(a,b) long a, b; { return (a % b); }
#
# long remainder
# Author: Mark McDowell (8/3/84)
#
	global	___lrem
	sglobal	_lrem
___lrem:
_lrem:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_lrem(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_lrem,%a0
	jsr	mcount
')
	')
	move.l	(4,%sp),%a0	#get dividend (x1,x0)
	move.l	(8,%sp),%a1	#get divisor (y1,y0)
	move.l	%a1,%d1		#move divisor
	cmp.w	%a0,%a0		#is dividend short?
	bne.w	lrem5		#jump if not
	cmp.w	%a1,%a1		#is divisor short?
	bne.w	lrem3		#if not, answer is probably dividend 
	move.l	%a0,%d0		#move dividend
	divs	%d1,%d0		#short division
	bvs.w	lrem4		#check for -32768 % -1
	swap	%d0		#get remainder
	ext.l	%d0		#extend remainder
	rts
lrem1:	beq.w	lrem4		#divisor = dividend?
lrem2:	move.l	%a0,%d0		#answer is dividend (i.e. divisor > dividend)
	rts
lrem3:	neg.l	%d1		#check for -32768 % 32768
	cmp.l	%d1,%a0		#is it so?
	bne.w	lrem2		#if not, dividend is answer
lrem4:	moveq	&0,%d0		#answer is 0
	rts
lrem5:	cmp.w	%a1,%a1		#long dividend; is divisor long?
	beq.w	lrem12		#jump if not
	move.l	%a0,%d0		#d0 <- dividend
	bpl.w	lrem6		#is dividend positive?
	neg.l	%d0		#if not, make it so
lrem6:	tst.l	%d1		#d1 <- divisor
	bpl.w	lrem7		#is divisor positive?
	neg.l	%d1		#if not, make it so
	move.l	%d1,%a1		#save abs(divisor)
lrem7:	cmp.l	%d1,%d0		#is divisor >= dividend?
	bcc.w	lrem1		#if so, answer is dividend or 0
	move.l	%d2,%a0		#save d2
	moveq	&15,%d2		#shift counter
lrem8:	add.l	%d1,%d1		#shift divisor left
	dbmi	%d2,lrem8		#until MSB = 1
	lsr.l	%d2,%d0		#shift dividend right by same amount
	swap	%d1		#shift left really amounted to shift right
	divu	%d1,%d0		#dividend / divisor (cannot overflow)
	move.w	%d0,%d2		#save quotient
	move.l	%a1,%d1		#get divisor (y1,y0)
	mulu	%d1,%d2		#d2 <- r0 * y0
	swap	%d1		#d1 <- (y0,y1)
	mulu	%d0,%d1		#d1 <- r0 * y1
	swap	%d2		#prepare for product
	add.w	%d1,%d2		#add upper 16 bits
	swap	%d2		#shift back into place
	move.l	(4,%sp),%d0	#get original dividend
	bpl.w	lrem9		#is it positive?
	neg.l	%d0		#if not, make it so
lrem9:	sub.l	%d2,%d0		#calculate remainder
	bcc.w	lrem10		#is remainder positive?
	add.l	%a1,%d0		#if not, add back a divisor
lrem10:	tst.b	(4,%sp)		#check sign of dividend
	bpl.w	lrem11		#positive?
	neg.l	%d0		#no: make result negative
lrem11:	move.l	%a0,%d2		#restore d2
	rts
lrem12:	move.l	%a0,%d0		#d0 <- dividend
	bpl.w	lrem13		#is it positive?
	neg.l	%d0		#if not, make it so
lrem13:	tst.w	%d1		#is the divisor positive?
	bpl.w	lrem14
	neg.w	%d1		#if not, make it so
lrem14:	move.w	%d0,%a1		#save lower 16 bits of dividend
	clr.w	%d0		#shift dividend right by 16
	swap	%d0
	divu	%d1,%d0		#first division
	move.w	%a1,%d0		#get lower 16 bits
	divu	%d1,%d0		#divide again
	swap	%d0		#form result
	move.l	%a0,%d1		#get dividend sign
	bpl.w	lrem15		#is it positive?
	neg.w	%d0		#if not, change sign of result
lrem15:	ext.l	%d0		#extend result
	rts

ifdef(`PROFILE',`
		data
p_lrem:	long	0
	')
