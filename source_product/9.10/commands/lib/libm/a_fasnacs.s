# @(#) $Revision: 70.1 $      
#*************************************************************
#              				 	 	     *
#  ARCSINE/ARCCOSINE of IEEE 32 bit floating point number    *
#              				 	 	     *
#  argument is passed on the stack; answer is returned in d0 *
#              				 	 	     *
#  This routine does not check for or handle infinities,     *
#  denormalized numbers or NaN's but it does handle +0 and   *
#  -0.							     *
#              				 	 	     *
#  The algorithm used here is from Cody & Waite.  	     *
#              				 	 	     *
#  Author: Mark McDowell	September 24, 1984	     *
#							     *
#*************************************************************

		version 2
      		set	DOMAIN,1
    		set	EDOM,33
      		set	bogus4,0x18
          	set	addf_f1_f0,0x40D0
          	set	addf_f1_f3,0x40D6
          	set	addf_f2_f0,0x40E0
          	set	addf_f3_f2,0x40F4
          	set	addf_f4_f3,0x4106
          	set	addl_f4_f2,0x4012
          	set	divf_f2_f3,0x4266
          	set	divf_f3_f2,0x4274
          	set	divl_f2_f4,0x406C
         	set	movf_f0_m,0x456C
          	set	movf_f0_f3,0x4466
         	set	movf_f1_m,0x4568
         	set	movf_m_f0,0x44FC
         	set	movf_m_f1,0x44F8
         	set	movf_m_f2,0x44F4
         	set	movf_m_f3,0x44F0
         	set	movf_m_f4,0x44EC
         	set	movf_m_f5,0x44E8
           	set	movfl_f0_f4,0x43C4
           	set	movfl_f2_f2,0x43D2
           	set	movfl_f3_f4,0x43DC
          	set	movlf_f2_m,0x4578
          	set	mulf_f0_f2,0x41C4
          	set	mulf_f0_f1,0x41C2
          	set	mulf_f1_f1,0x41D2
          	set	mulf_f1_f2,0x41D4
          	set	mulf_f1_f3,0x41D6
          	set	mulf_f3_f2,0x41F4
          	set	mull_f4_f2,0x4052
          	set	subf_f0_f1,0x4142
          	set	subf_f1_f0,0x4150

	data

excpt:	long	0,0,0,0,0,0,0,0

asin:	byte	102,97,115,105,110,0		#"fasin"

acos:	byte	102,97,99,111,115,0		#"facos"

asinmsg: byte	102,97,115,105,110,58,32,68,79	#"fasin: DOMAIN error"
	byte	77,65,73,78,32,101,114,114
	byte	111,114,10,0

acosmsg: byte	102,97,99,111,115,58,32,68,79	#"facos: DOMAIN error"
	byte	77,65,73,78,32,101,114,114
	byte	111,114,10,0

pi:	byte	19		#flags for addition of PI or PI/2
plus:	byte	-128		#flags for addition/subtraction
sign:	byte	60		#flags for result sign

	text
ifdef(`_NAMESPACE_CLEAN',`
        global  __fasin,__facos
        sglobal _fasin,_facos',`
        global  _fasin,_facos
 ')

error:	moveq	&DOMAIN,%d1	#DOMAIN error
	move.l	%d1,excpt
	tst.l	%d7		#arcsine or arccosine?
	bpl.w	error1
	move.l	&acos,excpt+4	#arccosine
	bra.w	error2
error1:	move.l	&asin,excpt+4	#arcsine
error2:	move.l	%d0,-(%sp)	#convert arg to double
ifdef(`_NAMESPACE_CLEAN',`
	jsr	___ftod',`
	jsr	_ftod
 ')
	addq.w	&4,%sp
	move.l	%d0,excpt+8	#save in excpt
	move.l	%d1,excpt+12
	clr.l	excpt+24	#return value is zero
	clr.l	excpt+28
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#notify user of error
	jsr	_matherr	#notify user of error
 ')
	addq.w	&4,%sp
	tst.l	%d0		#report error?
	bne.w	error5
	moveq	&EDOM,%d0	#domain error
	move.l	%d0,_errno	#set errno
	pea	20.w		#length of message
	tst.l	%d7		#arcsine or arccosine?
	bpl.w	error3
	pea	acosmsg		#arccosine
	bra.w	error4
error3:	pea	asinmsg		#arcsine
error4:	pea	2.w		#stderr
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#print the message
	jsr	_write		#print the message
 ')
	lea	(12,%sp),%sp
error5:	move.l	excpt+28,-(%sp)	#get return value
	move.l	excpt+24,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	___dtof',`		#convert to single precision
	jsr	_dtof		#convert to single precision
 ')
	addq.w	&8,%sp
	move.l	(%sp)+,%d7	#restore d7
	unlk	%a6
	rts

ifdef(`_NAMESPACE_CLEAN',`
__facos:
')
_facos:
	ifdef(`PROFILE',`
	mov.l	&p_facos,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_facos
	link	%a6,&0
#	trap	#2
#	dc.w	-24		space to save registers
	move.l	%d7,-(%sp)	#save d7
	moveq	&-1,%d7		#bit 30 of d7 will be 1 if arccosine
	bra.w	asnacs

ifdef(`_NAMESPACE_CLEAN',`
__fasin:
')
_fasin:
	ifdef(`PROFILE',`
	mov.l	&p_fasin,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fasin
	link	%a6,&0
#	trap	#2
#	dc.w	-24		space to save registers
	move.l	%d7,-(%sp)	#save d7
	moveq	&0,%d7		#bit 30 of d7 will be 0 if arcsine

asnacs:	move.l	(8,%a6),%d0	#get argument
	move.l	%d0,%d1		#save
	add.l	%d1,%d1		#remove sign bit
	cmp.l	%d1,&0x7F000000	#compare absolute value with 1
	bhi.w	error		#error if greater than 1
	tst.w	float_soft	#check for 98635A card
	beq.w	hasin
	movem.l	%d2-%d6,-(%sp)	#save registers
	rol.l	&8,%d0		#exponent to lower byte
	smi	%d2		#save LSB of exponent
	moveq	&127,%d6		#exponent offset
	bset	%d6,%d0		#set hidden bit
	add.b	%d2,%d2		#LSB of exponent to extend bit
	addx.b	%d0,%d0		#tack LSB of exponent onto exponent
#
# The upper half of d7 holds four flags: bit 31 is the value of 'i'; bit 30 is 
# set if the operation is arccosine; bit 16 is the sign of the original argument
#
	subx.b	%d7,%d7		#bit 16 of d7 will sign of argument
	swap	%d7
	sub.b	%d0,%d6		#remove exponent bias (this is -exponent)
	clr.b	%d0		#clear out exponent from mantissa
	cmp.l	%d1,&0x7E000000	#compare with 1/2
	bhi.w	asn2		#choose your path
	cmp.l	%d1,&0x74250BFE	#compare with eps
	bcs.w	asn27
#
# The argument is between 5.589425E-4 and 0.5. Now we need to calculate g=Y*Y.
#
	move.b	%d6,%d7		#get ready for g = Y * Y
	subq.b	&1,%d7		#d7 will be the exponent of g
	add.b	%d7,%d7		#double exponent
	move.l	%d0,%a0		#save Y
	move.w	%d0,%d3		#d3 <- y0
	mulu	%d0,%d3		#d3 <- y0 * y0
	move.w	%d0,%d2		#d2 <- y0
	swap	%d0		#d0 <- (y0,y1)
	mulu	%d0,%d2		#d2 <- y1 * y0
	mulu	%d0,%d0		#d0 <- y1 * y1
	add.l	%d2,%d2		#d2 <- y1 * y0 + y1 * y0
	move.w	%d2,%d4
	clr.w	%d2		#shift left 16
	addx.b	%d2,%d2		#include extend bit
	swap	%d2		#move into place
	swap	%d3		#lower 16 bits are 0
	add.w	%d4,%d3		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
	bmi.w	asn1		#normalized?
	add.w	%d3,%d3		#normalize it
	addx.l	%d0,%d0
	addq.b	&1,%d7		#adjust binary point
asn1:	tst.w	%d3		#check round bit
	bpl.w	asn17		#it is set?
	addq.l	&1,%d0		#round up
	neg.w	%d3		#was it halfway?
	bpl.w	asn17
	bclr	%d3,%d0		#round to even
	bra.w	asn17
#
# The argument is between 0.5 and 1.
#
asn2:	moveq	&-1,%d2		#bit number and counter
	bchg	%d2,%d7		#change i
	tst.b	%d6		#if exponent is 0, argument is 1
	bne.w	asn3		#check for easy answer
	moveq	&0,%d0		#result is 0
	moveq	&64,%d6		#make it look really small
	bra.w	asn27
asn3:	clr.w	%d7		#initialize exponent of g
	neg.l	%d0		#1 - Y
	cmp.l	%d0,&0x100000	#need to normalize
	bcc.w	asn4		#find MSB
	moveq	&12,%d1		#not in upper 12 bits
	lsl.l	%d1,%d0		#shift left 12
	add.b	%d1,%d7		#change exponent
asn4:	cmp.l	%d0,&0x4000000	#in upper 6 bits?
	bcc.w	asn5		#jump if it is
	lsl.l	&6,%d0		#shift off 6 zeroes
	addq.b	&6,%d7		#adjust exponent
asn5:	tst.l	%d0		#is bit 31 the MSB?
	bmi.w	asn7		#jump if it is
asn6:	add.l	%d0,%d0		#shift argument left by one
	dbmi	%d2,asn6		#and check MSB
	sub.b	%d2,%d7		#adjust exponent
asn7:	addq.b	&1,%d7		#divide by 2
	move.l	%d0,%a0		#save g

	lsr.l	&8,%d0		#want MSB in bit 23
	move.l	%d0,%d1		#save g for future calculations
#
# The square root is derived by using Newton iteration.
# The initial estimate used will be:   y0 = .425176 + .582338 * g
#
	move.w	&0x8A5D,%d3	#lower 16 bits of .582338
	moveq	&0x4B,%d4	#upper 7 bits of .582338
	move.w	%d3,%d2
	mulu	%d0,%d2		#lower g * lower constant
	move.w	%d4,%d5
	mulu	%d0,%d5		#lower g * upper constant
	swap	%d0		#get upper g into lower word
	mulu	%d0,%d3		#upper g * lower constant
	mulu	%d4,%d0		#upper g * upper constant
#
# Because the calculation of y0 will yield only seven accurate bits toward the
# final result, the lower 16 bits of this product are simply truncated.
#
	move.w	%d0,%d2		#remove 16 lowest and move 16 upper bits
	swap	%d2		#d2 is now aligned with d5 and d3 for adding
	add.l	%d5,%d2		#form the product from the partial products
	add.l	%d3,%d2
	add.l	&0x356A6A01,%d2	#add .425176
#
# The first Newton iteration takes the form: y1 = .5 * (y0 + g/y0).
#
	move.l	%d2,%d4		#save y0 for later
	move.l	%d2,%d0		#the quotient is estimated using the upper
	swap	%d0			#16 bits
#
# d1 contains g and is shifted left 6 so that the quotient will be aligned
# correctly with y0 (in d4) when the division is complete
#
	lsl.l	&6,%d1
	move.l	%d1,%d6		#f is also saved for the next iteration
	divu	%d0,%d1		#first step in g/y0
	move.w	%d1,%d3		#save upper 16 bits of quotient
	clr.w	%d1		#"shift in" zero
	mulu	%d3,%d2		#quotient * lower bits
	sub.l	%d2,%d1		#subtract from remainder
#
# The partial quotient could have been too large and in such a case, the 
# remainder went negative. For this, the quotient must be decremented by 1
# repeatedly while at the same time incrementing the remainder by the divisor
# until the remainder goes positive.
#
	bcc.w	asn9		#negative remainder?
asn8:	subq.w	&1,%d3		#decrement quotient
	add.l	%d4,%d1		#increment remainder by divisor
	bcc.w	asn8		#positive remainder yet?
asn9:	swap	%d3
	divu	%d0,%d1		#second division
#
# Overflow could have occurred with this division. If that happens, the real
# quotient is either $FFFF or $FFFE. We will give the value $FFFF. Its being
# otherwise will not matter since we are not expecting that much accuracy at
# this point.
#
	bvc.w	asn10
	moveq	&-1,%d1
#
# The second part of quotient may be incorrect by a bit or two. To get the
# correct answer would require determining the remainder as before. However,
# that much accuracy is not required.
#
asn10:	move.w	%d1,%d3		#form total quotient
        add.l	%d4,%d3		#add y0 to the quotient
        roxr.l	&1,%d3		#multiply by .5 (d3 now contains y1)
#
# The second Newton iteration is: y2 = .5 * (y1 + g/y1)
#
	move.w	%d3,%d2		#save lower bits of y1
	move.l	%d3,%d1		#isolate upper bits of y1
	swap	%d1
	divu	%d1,%d6		#first division for g/y1
	move.w	%d6,%d0		#save partial quotient
	clr.w	%d6		#"shift in" zero to remainder
	mulu	%d0,%d2		#lower bits of y1 * partial quotient
	sub.l	%d2,%d6		#new remainder
	bcc.w	asn12		#remainder could have gone negative
asn11:	subq.w	&1,%d0		#decrement quotient
	add.l	%d3,%d6		#add divisor to remainder
	bcc.w	asn11		#has remainder gone positive yet?
asn12:	swap	%d0		#shift partial quotient left 16
        divu	%d1,%d6		#second divide
	bvc.w	asn13		#did it overflow?
	moveq	&-1,%d6		#if it did, $FFFF is close enough
	move.w	%d6,%d0
	bra.w	asn15		#division complete for this case
asn13:	move.w	%d6,%d0		#form total quotient
        clr.w	%d6		#"shift in" zero
        move.w	%d3,%d2		#lower bits of y1
	mulu	%d0,%d2		#lower quotient * lower bits of y1
	sub.l	%d2,%d6		#form remainder
	bcc.w	asn15		#did remainder go negative?
asn14:	subq.l	&1,%d0		#decrement quotient
	add.l	%d3,%d6		#add divisor to remainder
	bcc.w	asn14		#has remainder gone positive yet?
asn15:	add.l	%d3,%d0		#y2 = .5 * (y1 + g / y1)
#
# It's time to determine if the exponent of the original argument was
# odd or even. If it was even, we must now multiply the result by sqrt(2).
#
	move.b	%d7,%d6		#d6 will have Y's exponent
	asr.b	&1,%d6		#halve the exponent
	bcc.w	asn16		#jump if no multiply
        move.w	&0xF334,%d2	#lower 16 bits of sqrt(2)
	move.w	&0xB504,%d1	#upper 16 bits of sqrt(2)
	move.w	%d0,%d3
	mulu	%d2,%d3		#lower sqrt(2) * lower y2
	move.w	%d0,%d4
	mulu	%d1,%d4		#upper sqrt(2) * lower y2
	swap	%d0		#move upper y2 to lower word
	mulu	%d0,%d2		#lower sqrt(2) * upper y2
	mulu	%d1,%d0		#upper sqrt(2) * upper y2
	add.l	%d2,%d4		#inner product sum
	move.l	%d4,%d2
	clr.w	%d2
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d4
	clr.w	%d4
	add.l	%d4,%d3		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
	tst.l	%d3		#round bit set?
	bpl.w	asn16
	addq.l	&1,%d0		#round up
asn16:	exg	%d0,%a0		#get g and save Y
#
# At this point, g is in d0 with the binary point at the extreme left. Register
# d7.w contains the number of right shifts necessary to align g properly with
# respect to the binary point. Y is in a0 and its binary point is between bits
# 30 and 31. Register d6.w contains the number of rights shifts necessary to 
# align Y properly with respect to the binary point. For both g and Y, bit 31
# is set.
#
# R(g)=(g*(p2*g+p1))/((g+q1)*g+q0); the denominator is calculated first.
#
asn17:	move.l	&0xB18D0B26,%d1	#q1
	move.b	%d7,%d4		#get binary point
	addq.b	&3,%d4		#change binary point
	move.l	%d0,%d2		#save g
	lsr.l	%d4,%d2		#align binary points
	subx.l	%d2,%d1		#x = g + q1
	move.w	%d0,%d3		#d3 <- g0
	mulu	%d1,%d3		#d1 <- x0 * g0
	move.w	%d0,%d2		#d2 <- g0
	move.w	%d1,%d4		#d4 <- x0
	swap	%d0		#d0 <- (g0,g1)
	mulu	%d0,%d4		#d4 <- x0 * g1
	swap	%d1		#d1 <- (x0,x1)
	mulu	%d1,%d2		#d2 <- x1 * g0
	mulu	%d0,%d1		#d1 <- x1 * g1
	add.l	%d2,%d4		#d4 <- x1 * g0 + x0 * g1
	move.l	%d4,%d2
	clr.w	%d2
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d4
	clr.w	%d4
	add.l	%d4,%d3		#add lower 32 bits
	sne	%d3		#only need to know if non-zero
	addx.l	%d2,%d1		#add upper 32 bits
	moveq	&0,%d4		#for mask
	moveq	&32,%d2		#for bit position
	sub.b	%d7,%d2		#where mask begins
	bset	%d2,%d4		#set LSB of mask
	neg.l	%d4		#complete mask
	ror.l	%d7,%d1		#shift number to align binary point
	and.l	%d1,%d4		#save bits to be removed
	eor.l	%d4,%d1		#remove those bits
	or.b	%d3,%d4		#rest of lower bits
	move.l	&0xB350EFF2,%d2	#q0
	neg.l	%d4		#begin subtraction
	subx.l	%d1,%d2		#subtract upper 32 bits
	tst.l	%d4		#round bit set?
	bpl.w	asn18		#jump if not
	addq.l	&1,%d2		#round up
	neg.l	%d4		#halfway?
	bpl.w	asn18		#jump if not
	bclr	%d4,%d2		#round to even
asn18:	move.l	%d2,%a1		#save Q(g)
	move.w	&0x8120,%d1	#upper bits of p2
	move.w	&0x6518,%d2	#lower bits of p2
	move.w	%d2,%d4		#d4 <- p2_0
	mulu	%d0,%d2		#d2 <- g1 * p2_0
	move.w	%d1,%d3		#d3 <- p2_1
	mulu	%d0,%d1		#d1 <- g1 * p2_1
	swap	%d0		#d0 <- (g1,g0)
	mulu	%d0,%d3		#d3 <- g0 * p2_1
	mulu	%d0,%d4		#d4 <- g0 * p2_0
	add.l	%d2,%d3		#d3 <- g0 * p2_1 + g1 * p2_0
	move.l	%d3,%d2
	clr.w	%d2		#shift
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d3
	clr.w	%d3		#remove upper 16 bits
	add.l	%d4,%d3		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
	move.l	&0xEF166B3C,%d2	#p1
	lsr.l	%d7,%d1		#shift p2 * g to align binary points
	subx.l	%d1,%d2		#P = p2 * g + p1 (rounded)
	move.w	%d0,%d4		#d4 <- g0
	mulu	%d2,%d4		#d4 <- g0 * P0
	move.w	%d2,%d3		#d3 <- P0
	move.w	%d0,%d1		#d1 <- g0
	swap	%d0		#d0 <- (g0,g1)
	swap	%d2		#d2 <- (P0,P1)
	mulu	%d0,%d3		#d3 <- g1 * P0
	mulu	%d2,%d1		#d1 <- g0 * P1
	mulu	%d0,%d2		#d2 <- g1 * P1
	add.l	%d3,%d1		#d1 <- P1 * g0 + P0 * g1
	move.l	%d1,%d3		#save
	clr.w	%d1
	addx.b	%d1,%d1		#save extend bit
	swap	%d1		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	smi	%d4
	addx.l	%d2,%d1		#add upper 32 bits
	cmp.l	%a1,%d1		#is g * P(g) less than Q(g)?
	bhi.w	asn19
	lsr.l	&1,%d1		#shift so no overflow in division
	roxr.b	&1,%d4		#save round bit
	subq.b	&1,%d7		#adjust exponent
asn19:	tst.b	%d4		#is round bit set?
	bpl.w	asn20
	addq.l	&1,%d1		#if so, increment
	cmp.l	%a1,%d1		#could have overflowed to become equal
	bne.w	asn20
	lsr.l	&1,%d1
	subq.b	&1,%d7		#adjust exponent
asn20:	moveq	&-1,%d4		#assume a quotient
	swap	%d6		#save Y's exponent
	st	%d6		#loop counter
	move.l	%a1,%d2		#get Q(g)
	move.l	%d2,%d0		#d2 <- (Q1,Q0)
	clr.w	%d0		#remove Q0
	swap	%d0		#d0 <- Q1
asn21:	swap	%d4		#get ready for next partial quotient
	divu	%d0,%d1		#division
	bvc.w	asn22		#check for overflow
	move.l	%d2,%d3		#d3 <- (Q1,Q0)
	neg.w	%d2		#instead of doing a multiply by FFFF,
	subx.l	%d0,%d3		   #form the product in (d3.l,d2.w)
	neg.w	%d2		#now subtract it from the dividend
	subx.l	%d3,%d1		
	swap	%d1		#shift remainder
	move.w	%d2,%d1
	subx.b	%d3,%d3		#move extend bit to d3
	bpl.w	asn24		#is FFFF correct?
	subq.w	&1,%d4		#no: it must be FFFE
	add.l	%d2,%d1		#add back divisor
	bra.w	asn24
asn22:	move.w	%d1,%d4		#save partial quotient
	clr.w	%d1		#shift dividend
	move.w	%d4,%d3		#save quotient
	mulu	%d2,%d3		#to calculate remainder
	sub.l	%d3,%d1		#remainder
	bcc.w	asn24		#did it go negative?
asn23:	subq.w	&1,%d4		#decrement quotient
	add.l	%d2,%d1		#add back divisor
	bcc.w	asn23		#until it goes positive
asn24:	neg.b	%d6		#check loop counter
	bpl.w	asn21
	addq.b	&4,%d7		#to shift into position
	lsr.l	%d7,%d4		#shift number
	negx.l	%d4		#get round bit
	neg.l	%d4
#
# Now calculate Y+Y*R(g) as Y*(1+R(g)).
#
	bset	%d6,%d4		#add 1 (i.e. x = 1 + R(g))
	swap	%d6		#get Y's exponent
	move.l	%a0,%d0		#get Y
	move.w	%d0,%d3		#d3 <- y0
	mulu	%d4,%d3		#d3 <- x0 * y0
	move.w	%d0,%d2		#d2 <- y0
	move.w	%d4,%d1		#d1 <- x0
	swap	%d0		#d0 <- (y0,y1)
	swap	%d4		#d4 <- (x0,x1)
	mulu	%d4,%d2		#d2 <- x1 * y0
	mulu	%d0,%d1		#d1 <- x0 * y1
	mulu	%d4,%d0		#d0 <- x1 * y1
	add.l	%d2,%d1		#add inner product
	move.l	%d1,%d2
	clr.w	%d1		#remove lower 16 bits
	addx.b	%d1,%d1		#save extend bit
	swap	%d1		#move into place
	swap	%d2
	clr.w	%d2
	add.l	%d2,%d3		#add lower 32 bits
	addx.l	%d1,%d0		#add upper 32 bits
	bmi.w	asn25
	add.l	%d3,%d3		#get a bit
	addx.l	%d0,%d0		#move it
	addq.w	&1,%d6		#new exponent
asn25:	subq.w	&1,%d6		#binary point between bits 30 and 31
	tst.l	%d3		#check round bit
	bpl.w	asn27		#is it set?
	addq.l	&1,%d0		#round up
	bne.w	asn26		#did we overflow?
	roxr.l	&1,%d0		#get bit back
	subq.w	&1,%d6		#adjust exponent
asn26:	neg.l	%d3		#check "sticky" bit
	bpl.w	asn27		#is it set?
	bclr	%d3,%d0		#round to even
#
# The result is in d0 with the binary point between bits 30 and 31. Register
# d6.w contains the number of right shifts necessary to align the number 
# correctly with respect to the binary point. How the value is handled now is
# dependent upon three flags: the original argument sign, the arcsine/arccosine
# flag and i. These three flags are in d7.
#
asn27:	moveq	&0,%d1		#lower 32 bits of result
	swap	%d7		#get flags
	rol.w	&2,%d7		#argsign(2),i(1),flag(0)
	btst	%d7,pi.l		#is PI or PI/2 to be added or none at all?
	bne.w	asn33
	move.l	&0x6487ED51,%d2	#upper 32 bits of PI
	move.l	&0x10B4611A,%d3	#lower 32 bits of PI
	move.b	%d6,%d4		#d4 will be shift amount
	btst	&1,%d7		#is it PI or PI/2?
	seq	%d6		#one less shift if PI/2
	subq.b	&1,%d6		#where binary point is
	sub.b	%d6,%d4		#figure in where binary point of Y is
	moveq	&32,%d5		#to make mask
	cmp.b	%d5,%d4		#large shift?
	bhi.w	asn28
	moveq	&0,%d0		#amount inconsequential
	bra.w	asn29
asn28:	sub.b	%d4,%d5		#LSB of mask
	bset	%d5,%d1		#set LSB of mask
	neg.l	%d1		#extend mask
	ror.l	%d4,%d0		#shift Y
	and.l	%d0,%d1		#save bits to be shifted off
	eor.l	%d1,%d0		#remove them from where they came
asn29:	btst	%d7,plus.l	#is it addition or subtraction?
	bne.w	asn30
	sub.l	%d3,%d1		#subtract lower 32 bits
	subx.l	%d2,%d0		#subtract upper 32 bits
	bcc.w	asn31		#did it go negative?
	addq.b	&4,%d7		#flip sign of result
	neg.l	%d1		#make it positive
	negx.l	%d0
	bra.w	asn31
asn30:	add.l	%d3,%d1		#add to a multiple of PI
	addx.l	%d2,%d0
asn31:	bmi.w	asn33		#is result normalized?
	moveq	&-1,%d5		#counter
asn32:	add.l	%d0,%d0		#shift until normalized
	dbmi	%d5,asn32
	sub.b	%d5,%d6		#calculate result exponent
#
# All the pieces are here; now we just put it together.
#
asn33:	lsr.l	&8,%d0		#move into place for final result
	beq.w	asn34		#is the answer 0?
	negx.l	%d0		#add the round bit
	neg.l	%d0
	moveq	&126,%d2		#exponent offset
	sub.b	%d6,%d2		#result exponent
	lsl.w	&7,%d2		#move into place
	swap	%d2
	add.l	%d2,%d0		#now have exponent and mantissa
	btst	%d7,sign.l	#what will final sign be?
	beq.w	asn34
	bset	&31,%d0		#make it negative
asn34:	movem.l	(%sp)+,%d2-%d7	#cleanup
	unlk	%a6
	rts
#
# Code for using the float card begins here
#
hasin:	lea	float_loc+bogus4,%a0		#address of float card
	lsr.l	&1,%d1				#Y = abs(X)
	tst.l	%d0				#sign of X?
	smi	%d7				#save sign of X
	move.l	%d1,(movf_m_f0-bogus4,%a0)		#f0 <- Y
	cmp.l	%d1,&0x3F000000			#compare with 1/2
	bhi.w	hasin1
	cmp.l	%d1,&0x3A1285FF			#compare with eps
	bcs.w	hasin4
	move.l	%d1,(movf_m_f1-bogus4,%a0)		#f1 <- Y
	tst.w	(mulf_f1_f1-bogus4,%a0)		#f1 <- g = Y * Y
	movem.l	(%a0),%d1/%a1			#wait
	bra.w	hasin3
hasin1:	bchg	&14,%d7				#i = 1 - flag
	move.l	&0x3F800000,(movf_m_f1-bogus4,%a0) #f1 <- 1.0
	tst.w	(subf_f0_f1-bogus4,%a0)		#f1 <- 1.0 - Y
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f1_m-bogus4,%a0),%d0		#get argument
	move.l	%d0,%d1				#save it
	and.l	&0x7FFFFF,%d0			#remove exponent
	or.l	&0x3F000000,%d0			#set to new exponent
	move.l	%d0,(movf_m_f0-bogus4,%a0)		#f0 <- reduced argument
	moveq	&9,%d0				#rotate amount
	rol.l	%d0,%d1				#isolate exponent
	moveq	&127,%d0				#exponent bias
	sub.b	%d0,%d1				#remove bias
	move.b	%d1,%d0				#d0 <- n
	move.l	&0x3F15141A,(movf_m_f2-bogus4,%a0) #f2 <- c2
	tst.w	(mulf_f0_f2-bogus4,%a0)		#f2 <- c2 * f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3ED9B0AB,(movf_m_f3-bogus4,%a0) #f2 <- c1
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- y0
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movf_f0_f3-bogus4,%a0)		#f3 <- f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(divf_f2_f3-bogus4,%a0)		#f3 <- f / y0
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- f / y0 + y0
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movf_f0_f3-bogus4,%a0)		#f3 <- f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(divf_f2_f3-bogus4,%a0)		#f3 <- f / z
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movfl_f3_f4-bogus4,%a0)		#f5,f4 <- f / z
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3E800000,(movf_m_f3-bogus4,%a0) #f3 <- 1/4
	tst.w	(mulf_f3_f2-bogus4,%a0)		#f2 <- z / 4
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movfl_f2_f2-bogus4,%a0)		#f3,f2 <- z / 4
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addl_f4_f2-bogus4,%a0)		#f3,f2 <- y2
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movfl_f0_f4-bogus4,%a0)		#f5,f4 <- f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(divl_f2_f4-bogus4,%a0)		#f5,f4 <- f / y2
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addl_f4_f2-bogus4,%a0)		#f3,f2 <- 2 * y3
	asr.b	&1,%d0				#is n odd or even?
	bcc.w	hasin2
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3FF6A09E,(movf_m_f5-bogus4,%a0) #f5,f4 <- sqrt(2.0)
	move.l	&0x667F3BCD,(movf_m_f4-bogus4,%a0)
	tst.w	(mull_f4_f2-bogus4,%a0)		#f3,f2 <- Y
hasin2:	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3F000000,(movf_m_f0-bogus4,%a0) #f0 <- 0.5
	tst.w	(mulf_f0_f1-bogus4,%a0)		#f1 <- g
	ror.l	&8,%d0				#move exponent change
	asr.l	&1,%d0
	movem.l	(%a0),%d1/%a1			#wait
	add.l	(movlf_f2_m-bogus4,%a0),%d0
	bchg	&31,%d0				#change sign
	move.l	%d0,(movf_m_f0-bogus4,%a0)		#f0 <- Y
hasin3:	move.l	&0xBF012065,(movf_m_f2-bogus4,%a0) #f2 <- P2
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- P2 * g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3F6F166B,(movf_m_f3-bogus4,%a0) #f3 <- P1
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- P2 * g + P1
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- (P2 * g + P1) * g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0xC0B18D0B,(movf_m_f3-bogus4,%a0) #f3 <- Q1
	tst.w	(addf_f1_f3-bogus4,%a0)		#f3 <- g + Q1
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f3-bogus4,%a0)		#f3 <- (g + Q1) * g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x40B350F0,(movf_m_f4-bogus4,%a0) #f4 <- Q0
	tst.w	(addf_f4_f3-bogus4,%a0)		#f3 <- (g + Q1) * g + Q0
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(divf_f3_f2-bogus4,%a0)		#f2 <- R(g)
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f0_f2-bogus4,%a0)		#f3 <- R(g) * y
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f2_f0-bogus4,%a0)		#f0 <- R(g) * y + y
	movem.l	(%a0),%d1/%a1			#wait
hasin4:	add.w	%d7,%d7				#carry bit <- flag
	bcs.w	hasin6
	bpl.w	hasin5				#check i
	move.l	&0x3F490FDB,(movf_m_f1-bogus4,%a0) #f1 <- pi/4
	tst.w	(addf_f1_f0-bogus4,%a0)		#f0 <- result + pi/4 
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f1_f0-bogus4,%a0)		#f0 <- result + pi/2 
	movem.l	(%a0),%d1/%a1			#wait
hasin5:	move.l	(movf_f0_m-bogus4,%a0),%d0		#get result
	tst.b	%d7				#change sign?
	bmi.w	hasin8
	move.l	(%sp)+,%d7
	unlk	%a6
	rts
hasin6:	tst.b	%d7				#sign of X?
	bmi.w	hasin9
	tst.w	%d7				#check i
	bpl.w	hasin7
	move.l	&0x3F490FDB,(movf_m_f1-bogus4,%a0) #f1 <- pi/4
	tst.w	(subf_f1_f0-bogus4,%a0)		#f0 <- result - pi/4
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(subf_f1_f0-bogus4,%a0)		#f0 <- result - pi/2
	movem.l	(%a0),%d1/%a1			#wait
hasin7:	move.l	(movf_f0_m-bogus4,%a0),%d0		#get result
hasin8:	bchg	&31,%d0
	move.l	(%sp)+,%d7
	unlk	%a6
	rts
hasin9:	move.l	&0x3FC90FDB,%d0			#d0 <- pi/2
	tst.w	%d7				#check i
	bpl.w	hasin10
	move.l	&0x3F490FDB,%d0			#d0 <- pi/4
hasin10:	move.l	%d0,(movf_m_f1-bogus4,%a0)		#store constant
	tst.w	(addf_f1_f0-bogus4,%a0)		#f0 <- result + constant
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f1_f0-bogus4,%a0)		#f0 <- result + 2 * constant
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f0_m-bogus4,%a0),%d0		#get result
	move.l	(%sp)+,%d7
	unlk	%a6
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCSINE				  *
#							  *
# float fasin(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact and underflow errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/24/85			  *
#							  *
#**********************************************************

	text
c_fasin:
	lea	(4,%sp),%a0
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fasin.s	(%a0),%fp0		#calculate asin
	fmov.l	%fpsr,%d0		#get status register
	and.w	&0x2000,%d0		#domain error?
	bne.b	c_asin2
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d1,%fpcr		#restore control register
	rts
c_asin2:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	subq.w	&8,%sp
	fmov.s	(%a0),%fp0
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_asin_name
	movq	&DOMAIN,%d0
	mov.l	%d0,-(%sp)		#type = DOMAIN
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_asin3
	movq	&EDOM,%d0
	mov.l	%d0,_errno		#errno = EDOM
	pea	20.w
	pea	c_asin_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`			#write(2,"fasin: DOMAIN error\n",20)
	jsr	_write			#write(2,"fasin: DOMAIN error\n",20)
 ')
	lea	(12,%sp),%sp
c_asin3:	
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov.l	(%sp)+,%fpcr		#restore control register
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCCOSINE				  *
#							  *
# float facos(arg) float arg;				  *
#							  *
# Errors: If arg < -1 or arg > 1, matherr() is called 	  *
#		with type = DOMAIN and retval = 0.0.	  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/24/85			  *
#							  *
#**********************************************************

c_facos:
	lea	(4,%sp),%a0
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	facos.s	(%a0),%fp0		#calculate acos
	fmov.l	%fpsr,%d0		#get status register
	andi.w	&0x2000,%d0		#domain error?
	bne.b	c_acos2
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d1,%fpcr		#restore control register
	rts
c_acos2:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	subq.w	&8,%sp
	fmov.s	(%a0),%fp0
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_acos_name
	movq	&DOMAIN,%d0
	mov.l	%d0,-(%sp)		#type = DOMAIN
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_acos3
	movq	&EDOM,%d0
	mov.l	%d0,_errno		#errno = EDOM
	pea	20.w
	pea	c_acos_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`			#write(2,"facos: DOMAIN error\n",20)
	jsr	_write			#write(2,"facos: DOMAIN error\n",20)
 ')
	lea	(12,%sp),%sp
c_acos3:	
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov.l	(%sp)+,%fpcr		#restore control register
	rts

	data
c_asin_name:
	byte	102,97,115,105,110,0		#"fasin"
c_asin_msg:
	byte	102,97,115,105,110,58,32,68,79	#"fasin: DOMAIN error\n"
	byte	77,65,73,78,32,101,114,114
	byte	111,114,10,0
c_acos_name:
	byte	102,97,99,111,115,0		#"facos"
c_acos_msg:
	byte	102,97,99,111,115,58,32,68,79	#"facos: DOMAIN error\n"
	byte	77,65,73,78,32,101,114,114
	byte	111,114,10,0

ifdef(`PROFILE',`
	data
p_facos:long	0
p_fasin:long	0
	')
