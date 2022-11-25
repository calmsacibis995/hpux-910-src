# @(#) $Revision: 70.1 $     
ifdef(`_NAMESPACE_CLEAN',`
        global  __fatan,__fatan2
        sglobal _fatan,_fatan2',`
        global  _fatan,_fatan2
 ')
#******************************************************************
#              				 	 	     	  *
#  ARCTANGENT/ARCTANGENT2 of IEEE 32 bit floating point number(s) *
#              				 	 	     	  *
#  argument is passed on the stack; answer is returned in d0 	  *
#              				 	 	     	  *
#  This routine does not check for or handle infinities,          *
#  denormalized numbers or NaN's but it does handle +0 and -0.	  *
#              				 	 	     	  *
#  The algorithm used here is from Cody & Waite. It has been	  *
#  modified somewhat to reduce the number of divisions required.  *
#              				 	 	     	  *
#  Author: Mark McDowell	October 29, 1984	     	  *
#							     	  *
#******************************************************************

		version 2
      		set	DOMAIN,1
    		set	EDOM,33
      		set	bogus4,0x18
          	set	addf_f0_f1,0x40C2
          	set	addf_f0_f2,0x40C4
          	set	addf_f1_f0,0x40D0
          	set	addf_f1_f3,0x40D6
          	set	addf_f2_f0,0x40E0
          	set	addf_f3_f2,0x40F4
          	set	divf_f0_f1,0x4242
          	set	divf_f2_f1,0x4262
          	set	divf_f3_f2,0x4274
          	set	divl_f4_f2,0x4072
          	set	movf_f0_f1,0x4462
         	set	movf_f0_m,0x456C
         	set	movf_f1_m,0x4568
         	set	movf_f2_m,0x4564
         	set	movf_f3_m,0x4560
         	set	movf_m_f0,0x44FC
         	set	movf_m_f1,0x44F8
         	set	movf_m_f2,0x44F4
         	set	movf_m_f3,0x44F0
           	set	movfl_f0_f2,0x43C2
           	set	movfl_f1_f4,0x43CC
          	set	movlf_f2_m,0x4578
          	set	mulf_f0_f1,0x41C2
          	set	mulf_f0_f2,0x41C4
          	set	mulf_f1_f1,0x41D2
          	set	mulf_f1_f2,0x41D4
          	set	subf_f0_f1,0x4142
          	set	subf_f2_f1,0x4162

	data

excpt:	long	0,0,0,0,0,0,0,0

atan2:	byte	102,97,116,97,110,50,0 		#"fatan2"

atn2msg:	byte	102,97,116,97,110,50,58,32,68	#"fatan2: DOMAIN error"
	byte	79,77,65,73,78,32,101,114
	byte	114,111,114,10,0

array:	long	0x3F060A92	#PI/6
	long	0x3FC90FDB	#PI/2
	long	0x3F860A92	#PI/3

	text

atn00:	add.l	%d2,%d2		#remove sign of v
	bne.w	atn2		#if v=0, error
	movem.l	(%sp)+,%d2-%d7	#restore registers

error:	moveq	&DOMAIN,%d1	#DOMAIN error
	move.l	%d1,excpt
	move.l	&atan2,excpt+4	#name
	move.l	(8,%a6),excpt+8	#set up arguments
	move.l	(12,%a6),excpt+16
	moveq	&0,%d0
	move.l	%d0,excpt+12
	move.l	%d0,excpt+20
	move.l	%d0,excpt+24	#return value is zero
	move.l	%d0,excpt+28
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#notify user of error
	jsr	_matherr	#notify user of error
 ')
	addq.w	&4,%sp
	tst.l	%d0		#report error?
	bne.w	error1
	moveq	&EDOM,%d0	#domain error
	move.l	%d0,_errno	#set errno
	pea	21.w		#length of message
	pea	atn2msg
	pea	2.w		#stderr
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#print the message
	jsr	_write		#print the message
')
	lea	(12,%sp),%sp
error1:	move.l	excpt+28,-(%sp)	#get return value
	move.l	excpt+24,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	___dtof',`		#convert to single precision
	jsr	_dtof		#convert to single precision
')
	unlk	%a6
	rts

atn1:	moveq	&0,%d0		#answer is 0
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk	%a6
	rts

atn2:	move.l	&0x7F921FB6,%d0	#d0 <- PI/2
	add.l	%d7,%d7		#get sign
	roxr.l	&1,%d0		#make it the sign of PI/2
atn3:	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk	%a6
	rts
#
# Get arguments for arctan2
#
ifdef(`_NAMESPACE_CLEAN',`
__fatan2:
')
_fatan2:
	ifdef(`PROFILE',`
	mov.l	&p_fatan2,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fatan2
	tst.w	float_soft	#float card present?
	beq.w	hatn2
	link	%a6,&0		#arctan2 starts here
#	trap	#2
#	dc.w	-24		space for registers
	movem.l	%d2-%d7,-(%sp)	#save registers
	moveq	&-1,%d7		#flag arctan2
	movem.l	(8,%a6),%d0-%d1	#d0 <- v; d1 <- u
	move.l	%d0,%d2		#d2 <- v
	smi	%d7		#save sign
	ext.w	%d7		#bit 15 of d7 <- v sign
	move.l	%d1,%d3		#d3 <- u
	smi	%d7		#bit 0 of d7 <- u sign
	swap	%d7		#bits of d7: 31=v sign, 16=u sign, 15=flag
	add.l	%d3,%d3		#remove sign of u
	beq.w	atn00		#if u=0, answer is PI/2
	add.l	%d2,%d2		#remove sign of v
	beq.w	atn11a		#if v=0, treat like underflow
	addq.b	&3,%d7		#assume N=2
	cmp.l	%d2,%d3		#which is bigger, u or v?
	bcc.w	atn4		#will divide so f <= 1.0
	exg	%d0,%d1		#exchange u and v
	clr.b	%d7		#N=0
atn4:	rol.l	&8,%d0		#exponent to lower byte
	smi	%d2		#save LSB of exponent
	moveq	&127,%d5		#exponent offset
	bset	%d5,%d0		#set hidden bit
	add.b	%d2,%d2		#LSB of exponent to extend bit
	addx.b	%d0,%d0		#tack LSB of exponent onto exponent
	sub.b	%d0,%d5		#d5 <- exponent
	ext.w	%d5		#will work with word in case of ovrflw/undflw
	clr.b	%d0		#d0 <- mantissa
	rol.l	&8,%d1		#exponent to lower byte
	smi	%d2		#save LSB of exponent
	moveq	&127,%d6		#exponent offset
	bset	%d6,%d1		#set hidden bit
	add.b	%d2,%d2		#LSB of exponent to extend bit
	addx.b	%d1,%d1		#tack LSB of exponent onto exponent
	sub.b	%d1,%d6		#d6 <- exponent
	ext.w	%d6		#will work with word in case of ovrflw/undflw
	clr.b	%d1		#d1 <- mantissa
	sub.w	%d5,%d6		#quotient exponent
	addq.w	&1,%d6		#put binary point between bits 30 and 31
	bra.w	atn5		#join up with arctan
#
# Get arguments for arctan
#
ifdef(`_NAMESPACE_CLEAN',`
__fatan:
')
_fatan:
	ifdef(`PROFILE',`
	mov.l	&p_fatan,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fatan
	tst.w	float_soft	#float card present?
	beq.w	hatn
	link	%a6,&0		#arctan starts here
#	trap	#2
#	dc.w	-24		space for registers
	movem.l	%d2-%d7,-(%sp)	#save registers
	moveq	&0,%d7		#flag arctan
	move.l	(8,%a6),%d0	#get argument
	move.l	%d0,%d1		#save it
	add.l	%d1,%d1		#remove sign bit
	cmp.l	%d1,&0x73713746	#compare with eps
	bcs.w	atn3		#if smaller, atan(x)=x
	move.l	%d0,%d7		#save sign
	cmp.l	%d1,&0x98E00A30	#largest number such that atn(x)=PI/2
	bcc.w	atn2
	clr.w	%d7		#set N to 0 and flag atan
	rol.l	&8,%d0		#exponent to lower byte
	smi	%d2		#save LSB of exponent
	moveq	&127,%d6		#exponent offset
	bset	%d6,%d0		#set hidden bit
	add.b	%d2,%d2		#LSB of exponent to extend bit
	addx.b	%d0,%d0		#tack LSB of exponent onto exponent
	sub.b	%d0,%d6		#d6 <- exponent
	clr.b	%d0		#d0 <- mantissa
#
# Tests are made to discover one of four intervals in which f (the argument) is
# found:
#	1) if f <= 2-sqrt(3), f=f and n=0
#	2) if 2-sqrt(3) < f <= 1, f=(sqrt(3)*f-1)/(sqrt(3)+f) and n=1
#	3) if 1 < f < 1/(2-sqrt(3)), f=(sqrt(3)-f)/(sqrt(3)*f+1) and n=3
#	4) if 1/(2-sqrt(3)) <= f, f = 1/f and n=2
#
	cmp.l	%d1,&0x7D126146	#compare with 2.0 - sqrt(3.0)
	bcs.w	atn32		#jump if argument is reduced
	addq.b	&1,%d7		#N=1
	cmp.l	%d1,&0x7F000000	#compare with 1.0
	bls.w	atn16		#jump if inverse not needed
	cmp.l	%d1,&0x80DDB3D8	#compare with 1.0 / (2.0 - sqrt(3.0))
	bcs.w	atn15		#don't jump if only inverse needed
	addq.b	&1,%d7		#N=2
	move.l	&0x80000000,%d1	#d1 <- 1.0
	neg.b	%d6		#adjust exponent
	addq.b	&1,%d6
#
# Case 4: f = 1/f or function is arctangent2 and we are dividing the larger of
# the two arguments into the smaller.
#
atn5:	cmp.l	%d1,%d0		#will there be divide overflow?
	bcs.w	atn6
	lsr.l	&1,%d1		#yes: shift dividend right
	subq.w	&1,%d6		#adjust exponent
atn6:	moveq	&-1,%d2		#assume quotient
	moveq	&2,%d4		#loop counter
	move.l	%d0,%d3		#d3 <- divisor
	clr.w	%d3
	swap	%d3		#d3 <- top half of divisor
atn7:	swap	%d2		#get ready for next partial quotient
	divu	%d3,%d1		#division
	bvc.w	atn8		#check for overflow
	move.l	%d0,%d5
	neg.w	%d0		#instead of doing a multiply by FFFF,
	subx.l	%d3,%d5		   #form the product in (d5.l,d0.w)
	neg.w	%d0		#now subtract it from the dividend
	subx.l	%d5,%d1		
	swap	%d1		#shift remainder
	move.w	%d0,%d1
	subx.b	%d5,%d5		#move extend bit to d5
	bpl.w	atn10		#is FFFF correct?
	subq.w	&1,%d2		#no: it must be FFFE
	add.l	%d0,%d1		#add back divisor
	bra.w	atn10
atn8:	move.w	%d1,%d2		#save partial quotient
	clr.w	%d1		#shift dividend
	move.w	%d2,%d5		#save quotient
	mulu	%d0,%d5		#to calculate remainder
	sub.l	%d5,%d1		#remainder
	bcc.w	atn10		#did it go negative?
atn9:	subq.w	&1,%d2		#decrement quotient
	add.l	%d0,%d1		#add back divisor
	bcc.w	atn9		#until it goes positive
atn10:	subq.b	&1,%d4		#decrement loop counter
	bne.w	atn7
	lsr.l	&1,%d0		#divisor / 2
	cmp.l	%d1,%d0		#halfway?
	bcs.w	atn11
	addq.l	&1,%d2		#round up
#
# f is the reduced argument if the function is not arctangent2; otherwise we
# must make sure f <= 2-sqrt(3)
#
atn11:	move.l	%d2,%d0		#d0 <- f
	tst.w	%d7		#atan or atan2?
	bpl.w	atn32		#reduction complete if atan
	btst	&1,%d7		#what is N?
	bne.w	atn12		#jump if N=2
	cmp.w	%d6,&127		#check for underflow
	blt.w	atn13		#jump if no underflow
atn11a:	btst	&16,%d7		#check sign of u
	beq.w	atn1		#if u is positive, answer is 0
	move.l	&0x80921FB6,%d0	#else, answer is +PI or -PI
	add.l	%d7,%d7		#get sign of v
	roxr.l	&1,%d0		#move to PI
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk	%a6
	rts
atn12:	cmp.w	%d6,&35		#check for overflow
	bge.w	atn2		#big number gives PI/2
#
# Check to see if further reduction of the argument is necessary; if so, use
# the same code used by arctan
#
atn13:	cmp.w	%d6,&2		#now compare with 2.0 - sqrt(3.0)
	bgt.w	atn32		#jump if less
	blt.w	atn14
	cmp.l	%d0,&0x8930A2F5	#check mantissa
	bcs.w	atn32		#jump if less
atn14:	subq.b	&5,%d7		#with next stmt amounts to N=N+1
#
# There are two formulas for reducing f at this point:
#	f=(sqrt(3)*f-1)/(sqrt(3)+f)
#	f=(sqrt(3)-f)/(sqrt(3)*f+1)
# Bit 2 of d7 is set to indicate which of these to use; code for the two is
# shared as much as possible
#
atn15:	addq.b	&6,%d7		#change N and formula flag
#
# calculate sqrt(3) * f
#
atn16:	move.l	&0xD743DDB3,%d5	#d5 <- sqrt(3) swapped
	move.w	%d5,%d1		#d1 <- upper bits of sqrt(3) {x1}
	swap	%d5		#d5 <- sqrt(3)
	move.w	%d5,%d2		#d2 <- lower bits of sqrt(3) {x0}
	move.w	%d1,%d3		#d3 <- x1
	move.w	%d2,%d4		#d4 <- x0
	mulu	%d0,%d3		#d3 <- x1 * f0
	mulu	%d0,%d4		#d4 <- x0 * f0
	swap	%d0		#d0 <- (f0,f1)
	mulu	%d0,%d1		#d1 <- x1 * f1
	mulu	%d0,%d2		#d2 <- x0 * f1
	swap	%d0		#d0 <- (f1,f0)
	add.l	%d3,%d2		#d2 <- x0 * f1 + x1 * f0
	move.w	%d2,%d3		#d3 will be lower part of sum of inner products
	clr.w	%d2		#d2 will be upper part of sum of inner products
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
	moveq	&4,%d3
#
# Now the appropriate additions and subtractions will be done depending upon
# which formula
#
	and.b	%d7,%d3		#which formula?
	bne.w	atn17
	move.l	%d0,%d2		#d2 <- f
	lsr.l	%d6,%d2		#align binary points wrt sqrt(3)
	addx.l	%d3,%d2		#round if necessary
	add.l	%d2,%d5		#d5 <- f + sqrt(3)
	roxr.l	&1,%d5		#shift lost bit back in
	moveq	&2,%d2		#calculate shift for f * sqrt(3)
	sub.b	%d6,%d2
	moveq	&3,%d6		#new exponent will be 3
	rol.l	%d2,%d4		#shift bottom bits
	bset	%d2,%d3		#for mask
	subq.b	&1,%d3		#besides creating mask, initializes X bit to 0
	and.b	%d4,%d3		#get bits to shift out
	eor.b	%d3,%d4		#remove bits from where they came
	lsl.l	%d2,%d1		#shift so binary point is at extreme left
	or.b	%d3,%d1		#move shifted bits in
	subx.b	%d3,%d3		#f * sqrt(3) >= 1? (X bit is still 0 if no shift)
	bmi.w	atn21		#if it is, answer is in (d1,d4)
	neg.l	%d4		#otherwise, subtract from 1
	negx.l	%d1
	addq.b	&4,%d7		#and flag that f is negative
	bra.w	atn21

atn17:	subq.b	&4,%d7		#assume new f will be positive
	move.l	&0x40000000,%d2	#d2 <- 1.0
	move.b	%d6,%d3		#temp copy of exponent
	beq.w	atn18		#if exponent is 0, 1.0 in d2 is aligned correctly
	lsr.l	&1,%d2		#otherwise, align binary points
atn18:	add.l	%d2,%d1		#d1 <- sqrt(3) * f + 1.0
	bcc.w	atn19		#did adding 1.0 make it go greater than 2.0?
	roxr.l	&1,%d1		#if so, shift lost bit back in
	subq.b	&1,%d3		#adjust temporary exponent
atn19:	exg	%d1,%d5		#d5 will be denominator; (d1,d4), numerator
	subq.b	&1,%d1		#d1 <- upper 32 bits of sqrt(3)
	move.l	&0xC265539E,%d4	#d4 <- lower 32 bits of sqrt(3)
	tst.b	%d6		#what is the exponent?
	beq.w	atn20		#if zero, sqrt(3) is aligned correctly
	lsr.l	&1,%d1		#shift sqrt(3)
	lsr.l	&1,%d4		#no bits transferred or lost off end
atn20:	addq.b	&2,%d6		#adjust exponent to put binary point between
	sub.b	%d3,%d6			#bits 30 and 31 with d6.b having offset
	sub.l	%d0,%d1		#d1 <- sqrt(3) - f
	bcc.w	atn21		#did it go negative?
	neg.l	%d4		#yes: negate it
	negx.l	%d1
	addq.b	&4,%d7		#and flag that f is negative
#
# The numerator must be normalized before dividing
#
atn21:	tst.l	%d1		#MSB of numerator in top 32 bits?
	bne.w	atn22		#jump if it is
	exg	%d1,%d4		#shift left 32
	add.b	&32,%d6		#adjust exponent
atn22:	move.l	%d1,%d2		#save upper 32 bits
	swap	%d2		#isolate upper 16
	tst.w	%d2		#MSB in upper 16?
	bne.w	atn23		#jump if it is
	move.w	%d1,%d2		#save new upper 16
	swap	%d1		#shift left 16
	swap	%d4
	move.w	%d4,%d1
	add.b	&16,%d6		#adjust exponent
atn23:	clr.b	%d2		#now check upper 8
	tst.w	%d2		#is MSB here?
	bne.w	atn24		#jump if it is
	lsl.l	&8,%d1		#shift left 8
	rol.l	&8,%d4
	move.b	%d4,%d1
	addq.b	&8,%d6		#adjust exponent
atn24:	tst.l	%d1		#is it normalized yet?
	bmi.w	atn26		#jump if it is
	moveq	&-1,%d3		#counter
atn25:	add.l	%d1,%d1		#shift left
	dbmi	%d3,atn25	#normalized yet?
	neg.b	%d3		#make shift count positive
	add.b	%d3,%d6		#adjust exponent
	moveq	&0,%d2		#for mask
	bset	%d3,%d2
	subq.b	&1,%d2		#create mask
	rol.l	%d3,%d4		#shift lower bits
	and.b	%d4,%d2		#get bits to move
	or.b	%d2,%d1		#move to upper longword
#
# Now divide
#
atn26:	cmp.l	%d5,%d1		#anticipate divide overflow
	bhi.w	atn27
	lsr.l	&1,%d1		#shift dividend right
	subq.b	&1,%d6		#adjust exponent
atn27:	moveq	&-1,%d0		#assume quotient
	moveq	&2,%d4		#loop counter
	move.l	%d5,%d2		#d2 <- divisor
	clr.w	%d2
	swap	%d2		#d2 <- top half of divisor
atn28:	swap	%d0		#get ready for next partial quotient
	divu	%d2,%d1		#division
	bvc.w	atn29		#check for overflow
	move.l	%d5,%d3
	neg.w	%d5		#instead of doing a multiply by FFFF,
	subx.l	%d2,%d3		   #form the product in (d3.l,d5.w)
	neg.w	%d5		#now subtract it from the dividend
	subx.l	%d3,%d1		
	swap	%d1		#shift remainder
	move.w	%d5,%d1
	subx.b	%d3,%d3		#move extend bit to d3
	bpl.w	atn31		#is FFFF correct?
	subq.w	&1,%d0		#no: it must be FFFE
	add.l	%d5,%d1		#add back divisor
	bra.w	atn31
atn29:	move.w	%d1,%d0		#save partial quotient
	clr.w	%d1		#shift dividend
	move.w	%d0,%d3		#save quotient
	mulu	%d5,%d3		#to calculate remainder
	sub.l	%d3,%d1		#remainder
	bcc.w	atn31		#did it go negative?
atn30:	subq.w	&1,%d0		#decrement quotient
	add.l	%d5,%d1		#add back divisor
	bcc.w	atn30		#until it goes positive
atn31:	subq.b	&1,%d4		#decrement loop counter
	bne.w	atn28
	lsr.l	&1,%d5		#denominator / 2
	cmp.l	%d1,%d5		#halfway?
	bcs.w	atn32
	addq.l	&1,%d0		#round up
#
# f is the reduced argument and is found in f; the exponent of f is in d6
# and various flags are found in d7
#
atn32:	cmp.b	%d6,&12		#compare f with epsilon
	bgt.w	atn41		#jump if small
	move.l	%d0,%a0		#save f
	move.w	%d6,%a1		#save exponent of f
#
# g = f * f
#
	add.b	%d6,%d6		#exponent of g is twice that of f
	subq.b	&1,%d6		#move binary point to between bits 30 and 31
	move.w	%d0,%d1		#d1 <- f0
	move.w	%d0,%d2		#d2 <- f0
	swap	%d0		#d0 <- (f0,f1)
	mulu	%d0,%d1		#d1 <- f0 * f1
	mulu	%d0,%d0		#d0 <- f1 * f1
	mulu	%d2,%d2		#d2 <- f0 * f0
	add.l	%d1,%d1		#d1 <- f0 * f1 + f1 * f0
	move.w	%d1,%d3		#d3 will be lower part of sum of inner products
	clr.w	%d1		#d1 will be upper part of sum of inner products
	addx.b	%d1,%d1		#save extend bit
	swap	%d1		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d2		#add lower 32 bits
	addx.l	%d1,%d0		#add upper 32 bits
	bmi.w	atn33		#normalized?
	add.l	%d2,%d2		#shift lower 32 bits
	addx.l	%d0,%d0		#shift upper 32 bits
	addq.b	&1,%d6		#adjust exponent
#
# q0 + g
#
atn33:	move.l	&0xB4CCD302,%d5	#d5 <- q0
	move.l	%d0,%d1		#d1 <- g
	lsr.l	%d6,%d1		#align binary points
	addx.l	%d1,%d5		#d5 <- g + q0
#
# p1 * g
#
	move.w	&0xD086,%d1	#d1 <- upper 16 bits of p1 {p1_1}
	move.w	&0x9100,%d2	#d2 <- lower 16 bits of p1 {p1_0}
	move.w	%d1,%d3		#d3 <- p1_1
	move.w	%d2,%d4		#d4 <- p1_0
	mulu	%d0,%d3		#d3 <- p1_1 * g0
	mulu	%d0,%d4		#d4 <- p1_0 * g0
	swap	%d0		#d0 <- (g0,g1)
	mulu	%d0,%d1		#d1 <- p1_1 * g1
	mulu	%d0,%d2		#d2 <- p1_0 * g1
	add.l	%d3,%d2		#d2 <- p1_0 * g1 + p1_1 * g0
	move.w	%d2,%d3		#d3 will be lower part of sum of inner products
	clr.w	%d2		#d2 will be upper part of sum of inner products
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
#
# P(g) = p1 * g + p0
#
	move.l	&0xF110F594,%d3	#d3 <- p0
	addq.b	&2,%d6		#shift amount (b.p. between bits 32 and 33)
	lsr.l	%d6,%d1		#align binary points
	addx.l	%d3,%d1		#d1 <- -P(g) = -(p1 * g + p0)
#
# g * P(g)
#
	move.w	%d1,%d3		#d3 <- P0
	move.w	%d1,%d4		#d4 <- P0
	swap	%d1		#d1 <- (P0,P1)
	move.w	%d1,%d2		#d2 <- P1
	mulu	%d0,%d1		#d1 <- P1 * g1
	mulu	%d0,%d3		#d3 <- P0 * g1
	swap	%d0		#d0 <- (g1,g0)
	mulu	%d0,%d2		#d2 <- P1 * g0
	mulu	%d0,%d4		#d4 <- P0 * g0
	add.l	%d3,%d2		#d2 <- P1 * g0 + P0 * g1
	move.w	%d2,%d3		#d3 will be lower part of sum of inner products
	clr.w	%d2		#d2 will be upper part of sum of inner products
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
	bmi.w	atn34		#normalized?
	add.l	%d4,%d4		#shift lower 32 bits
	addx.l	%d1,%d1		#shift upper 32 bits
	addq.b	&1,%d6		#adjust exponent
#
# R(g) = g * P(g) / Q(g)
#
atn34:	cmp.l	%d5,%d1		#check for division overflow
	bhi.w	atn35
	lsr.l	&1,%d1		#shift dividend
	subq.b	&1,%d6		#adjust exponent
atn35:	moveq	&-1,%d0		#assume quotient
	moveq	&2,%d4		#loop counter
	move.l	%d5,%d2		#d2 <- divisor
	clr.w	%d2
	swap	%d2		#d2 <- top half of divisor
atn36:	swap	%d0		#get ready for next partial quotient
	divu	%d2,%d1		#division
	bvc.w	atn37		#check for overflow
	move.l	%d5,%d3
	neg.w	%d5		#instead of doing a multiply by FFFF,
	subx.l	%d2,%d3		   #form the product in (d3.l,d5.w)
	neg.w	%d5		#now subtract it from the dividend
	subx.l	%d3,%d1		
	swap	%d1		#shift remainder
	move.w	%d5,%d1
	subx.b	%d3,%d3		#move extend bit to d3
	bpl.w	atn39		#is FFFF correct?
	subq.w	&1,%d0		#no: it must be FFFE
	add.l	%d5,%d1		#add back divisor
	bra.w	atn39
atn37:	move.w	%d1,%d0		#save partial quotient
	clr.w	%d1		#shift dividend
	move.w	%d0,%d3		#save quotient
	mulu	%d5,%d3		#to calculate remainder
	sub.l	%d3,%d1		#remainder
	bcc.w	atn39		#did it go negative?
atn38:	subq.w	&1,%d0		#decrement quotient
	add.l	%d5,%d1		#add back divisor
	bcc.w	atn38		#until it goes positive
atn39:	subq.b	&1,%d4		#decrement loop counter
	bne.w	atn36
#
# 1 + R(g)
#
	subq.b	&1,%d6		#move binary point to between bits 31 and 32
	lsr.l	%d6,%d0		#align correctly
	negx.l	%d0		#d0 <- 1.0 + R(g)  {Y1,Y0}
#
# f * (1 + R(g))
#
	move.l	%a0,%d1		#get f
	move.w	%a1,%d6		#get exponent of f
	move.w	%d1,%d3		#d3 <- f0
	move.w	%d1,%d4		#d4 <- f0
	swap	%d1		#d1 <- (f0,f1)
	move.w	%d1,%d2		#d2 <- f1
	mulu	%d0,%d2		#d2 <- Y0 * f1
	mulu	%d0,%d4		#d4 <- Y0 * f0
	swap	%d0		#d0 <- (Y0,Y1)
	mulu	%d0,%d3		#d3 <- Y1 * f0
	mulu	%d1,%d0		#d0 <- Y1 * f1
	add.l	%d3,%d2		#d2 <- Y0 * f1 + Y1 * f0
	move.w	%d2,%d3		#d3 will be lower part of sum of inner products
	clr.w	%d2		#d5 will be upper part of sum of inner products
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#move into place
	swap	%d3
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
	bmi.w	atn40		#normalized?
	add.l	%d4,%d4		#shift lower 32 bits
	addx.l	%d0,%d0		#shift upper 32 bits
	addq.b	&1,%d6		#adjust exponent
atn40:	tst.l	%d4		#check round bit
	bpl.w	atn41		#jump if clear
	addq.l	&1,%d0		#round up
#
# The result is now <= PI/12. This is added to or subtracted from 0, PI/6,
# PI/3, PI/2, 2*PI/3, 5*PI/6, or PI depending upon various flags found in d7.
# The flags in d7 are: sign of V, sign of U, arctan/arctan2 flag, sign of f,
# and two bits for n.
#
atn41:	asr.l	&1,%d7		#move u sign to bit 15; v sign remains in 31
	roxl.w	&3,%d7		#bits: 4=f sign; (3,2)=N; 1=u sign; 0=flag
	moveq	&7,%d2		#flags for any modification to result
	btst	%d7,%d2		#add to 0?
	bne.w	atn46		#jump if answer is +result or -result
	moveq	&-1,%d5		#assume exponent
	move.l	&0x860A91C1,%d1	#d1 <- PI/6
	move.l	&0xF070F070,%d2	#flags for multiple of PI/6
	btst	%d7,%d2		#do we want PI/6, PI/3 or 2*PI/3? (yes or no)
	bne.w	atn42
	move.l	&0xC90FDAA2,%d1	#d1 <- PI / 2
	move.w	&0xF08,%d2	#flags for multiple of PI/2
	btst	%d7,%d2		#do we want PI/2 or PI? (yes or no)
	bne.w	atn42
	move.l	&0xA78D3632,%d1	#d1 <- 5 * PI / 6
	bra.w	atn43
#
# d1 contains the constant; now determine where its binary point is
#
atn42:	move.l	&0x80808088,%d2	#flags for exponent of -1
	btst	%d7,%d2
	bne.w	atn43
	moveq	&8,%d5		#mask for upper bit of N
	and.b	%d7,%d5		#set or not?
	seq	%d5
	neg.b	%d5		#exponent = 0 or 1
#
# Align binary points for addition or subtraction
#
atn43:	sub.b	%d5,%d6		#shift amount
	cmp.b	%d6,&33		#result insignificant?
	bgt.w	atn45		#jump if so
	lsr.l	%d6,%d0		#shift result
	moveq	&0,%d2		#for carry
	addx.l	%d2,%d0		#round up as needed
#
# Modification by constant
#
	move.l	&0x80707788,%d2	#flags for subtraction
	btst	%d7,%d2
	bne.w	atn44		#add or subtract?
	add.l	%d1,%d0		#result + m * PI / n
	bcc.w	atn45		#overflow?
	roxr.l	&1,%d0		#get lost bit
	subq.b	&1,%d5		#adjust exponent
	bra.w	atn45
atn44:	sub.l	%d0,%d1		#m * PI / n - result
	move.l	%d1,%d0		#d0 <- result
	bmi.w	atn45		#normalized?
	add.l	%d0,%d0		#shift by one bit at most
	addq.b	&1,%d5		#adjust exponent
atn45:	move.b	%d5,%d6		#d6 <- exponent
#
# All the pieces are now available; put the fields together
#
atn46:	moveq	&126,%d1		#exponent offset
	sub.b	%d6,%d1		#exponent field
	add.l	%d7,%d7		#get sign bit
	roxr.b	&1,%d1		#combine exponent and sign bit
	roxl.w	&8,%d1		#move them into place
	swap	%d1
	lsr.l	&8,%d0		#shift mantissa into place
	addx.l	%d1,%d0		#combine all fields
	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk	%a6
	rts
#
# Code for the float card begins here
#
hatn2:	link	%a6,&0
	move.l	(12,%a6),%d1		#get u
	add.l	%d1,%d1			#u = 0 ?
	bne.w	hatan1
	move.l	(8,%a6),%d1		#get v
	add.l	%d1,%d1
	beq.w	error			#atan(0,0)?
	move.l	&0x7F921FB6,%d0		#result = PI/2
	roxr.l	&1,%d0			#set sign of result
	unlk	%a6
	rts
hatan1:	move.l	(8,%a6),%d0		#get v
	add.l	%d0,%d0			#remove sign bit
	bne.w	hatan3			#v = 0?
	tst.b	(12,%a6)			#sign of u?
	bpl.w	hatan2
	move.l	&0x80921FB6,%d0		#result = PI
hatan2:	roxr.l	&1,%d0			#with sign of v
	unlk	%a6
	rts
hatan3:	lsr.l	&1,%d0			#|v|
	lsr.l	&1,%d1			#|u|
	lea	float_loc+bogus4,%a0	#address of float card
	move.l	%d0,(movf_m_f0-bogus4,%a0)	#f0 <- v
	move.l	%d1,(movf_m_f1-bogus4,%a0)	#f1 <- u
	tst.w	(movfl_f0_f2-bogus4,%a0)	#f3,f2 <- v
	movem.l	(%a0),%d1/%a1		#wait
	tst.w	(movfl_f1_f4-bogus4,%a0)	#f5,f4 <- u
	movem.l	(%a0),%d1/%a1		#wait
	tst.w	(divl_f4_f2-bogus4,%a0)	#f3,f2 <- v/u
	movem.l	(%a0),%d1/%a1		#wait
	move.l	(movf_f3_m-bogus4,%a0),%d0	#d0,d1 <- v/u
	move.l	(movf_f2_m-bogus4,%a0),%d1
	add.l	%d0,%d0			#remove sign bit
	cmp.l	%d0,&0x8FDFFFFE		#compare with xmax
	bcs.w	hatan5
	bhi.w	hatan4
	cmp.l	%d1,&0xE0000000		#check lower bits
	bls.w	hatan7
hatan4:	move.l	&0x7F921FB6,%d0		#result = PI/2
	bra.w	hatan6
hatan5:	cmp.l	%d0,&0x70200000		#compare with xmin
	bcc.w	hatan7
	moveq	&0,%d0			#assume answer of 0.0
	tst.b	(12,%a6)			#check sign of u
	bpl.w	hatan6
	move.l	&0x80921FB6,%d0		#result = PI
hatan6:	move.b	(8,%a6),%d1		#get sign of v
	add.b	%d1,%d1			#move sign to X
	roxr.l	&1,%d0			#include sign with result
	unlk	%a6
	rts
hatan7:	move.l	(movlf_f2_m-bogus4,%a0),-(%sp)	#push v/u
	jsr	hatn
	addq.w	&4,%sp
	tst.b	(12,%a6)			#check sign of u
	bpl.w	hatan8
	move.l	%d0,(movf_m_f0-bogus4,%a0) 		#f0 <- result
	move.l	&0x40490FDB,(movf_m_f1-bogus4,%a0)	#f1 <- PI
	tst.w	(subf_f0_f1-bogus4,%a0)	#f1 <- PI - result
	movem.l	(%a0),%d1/%a1
	move.l	(movf_f1_m-bogus4,%a0),%d0	#get result
hatan8:	tst.b	(8,%a6)			#check sign of v
	bpl.w	hatan9
	bchg	&31,%d0			#change sign
hatan9:	unlk	%a6
	rts

hatn:	link	%a6,&0
	move.l	(8,%a6),%d0		#get argument
	add.l	%d0,%d0			#remove sign bit
	cmp.l	%d0,&0x98E00A30		#min value such that tan(x) = PI/2
	bcs.w	hatan10
	move.l	&0x7F921FB6,%d0		#result = PI/2
	roxr.l	&1,%d0			#set sign correctly
	unlk	%a6
	rts
hatan10:	lea	float_loc+bogus4,%a0	#address of float card
	lsr.l	&1,%d0			#f = |X|
	moveq	&0,%d1			#assume n = 0
	cmp.l	%d0,&0x3F800000		#> 1.0 ?
	bls.w	hatan11
	move.l	%d0,(movf_m_f0-bogus4,%a0)	#f0 <- f
	move.l	&0x3F800000,(movf_m_f1-bogus4,%a0)	#f1 <- 1.0
	tst.w	(divf_f0_f1-bogus4,%a0)			#f1 <- 1.0 / f
	movem.l	(%a0),%d1/%a1
	move.l	(movf_f1_m-bogus4,%a0),%d0			#d0 <- 1.0 / f
	moveq	&8,%d1					#n = 2
hatan11: cmp.l	%d0,&0x3E8930A3				#compare with 2 - sqrt(3)
	bls.w	hatan12
	move.l	%d0,(movf_m_f0-bogus4,%a0)			#f0 <- f
	move.l	&0x3F3B67AF,(movf_m_f1-bogus4,%a0)	#f1 <- sqrt(3) - 1
	tst.w	(mulf_f0_f1-bogus4,%a0)			#f1 <- f * (sqrt(3) - 1)
	movem.l	(%a0),%d0/%a1				#wait
	move.l	&0x3F000000,(movf_m_f2-bogus4,%a0)	#f2 <- 0.5
	tst.w	(subf_f2_f1-bogus4,%a0)			#subtract 0.5
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(subf_f2_f1-bogus4,%a0)			#subtract 0.5
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(addf_f0_f1-bogus4,%a0)			#add f
	movem.l	(%a0),%d0/%a1				#wait
	move.l	&0x3FDDB3D7,(movf_m_f2-bogus4,%a0)	#f2 <- sqrt(3)
	tst.w	(addf_f0_f2-bogus4,%a0)			#f2 <- f + sqrt(3)
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(divf_f2_f1-bogus4,%a0)			#f1 <- new f
	movem.l	(%a0),%d0/%a1				#wait
	move.l	(movf_f1_m-bogus4,%a0),%d0			#d0 <- f
	addq.b	&4,%d1					#n = n + 1
hatan12:	movea.l	%d0,%a1					#save f
	add.l	%d0,%d0					#remove sign
	cmp.l	%d0,&0x73000000				#small value?
	bcs.w	hatan13
	move.l	%a1,(movf_m_f0-bogus4,%a0)			#f0 <- f
	tst.w	(movf_f0_f1-bogus4,%a0)			#f1 <- f
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(mulf_f1_f1-bogus4,%a0)			#f1 <- g
	movem.l	(%a0),%d0/%a1				#wait
	move.l	&0xBD508691,(movf_m_f2-bogus4,%a0)	#f2 <- p1
	tst.w	(mulf_f1_f2-bogus4,%a0)			#f2 <- p1 * g
	movem.l	(%a0),%d0/%a1				#wait
	move.l	&0xBEF110F6,(movf_m_f3-bogus4,%a0)	#f3 <- p0
	tst.w	(addf_f3_f2-bogus4,%a0)			#f2 <- p1 * g + p0
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(mulf_f1_f2-bogus4,%a0)			#f2 <- (p1 * g + p0) * g
	movem.l	(%a0),%d0/%a1				#wait
	move.l	&0x3FB4CCD3,(movf_m_f3-bogus4,%a0)	#f3 <- q0
	tst.w	(addf_f1_f3-bogus4,%a0)			#f3 <- g + q0
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(divf_f3_f2-bogus4,%a0)			#f2 <- g * P(g) / Q(g)
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(mulf_f0_f2-bogus4,%a0)			#f2 <- f*(g*P(g)/Q(g))
	movem.l	(%a0),%d0/%a1				#wait
	tst.w	(addf_f2_f0-bogus4,%a0)			#f0 <- result
	movem.l	(%a0),%d0/%a1				#wait
	movea.l	(movf_f0_m-bogus4,%a0),%a1			#a1 <- result
hatan13:	move.l	%a1,%d0					#d0 <- result
	subq.b	&4,%d1					#check n
	bmi.w	hatan15
	beq.w	hatan14
	bchg	&31,%d0					#change sign
hatan14:	move.l	%d0,(movf_m_f0-bogus4,%a0)			#f0 <- f
	lea	array,%a1
	move.l	(0,%a1,%d1.w),(movf_m_f1-bogus4,%a0)		#f1 <- constant
	tst.w	(addf_f1_f0-bogus4,%a0)			#f0 <- result
	movem.l	(%a0),%d0/%a1				#wait
	move.l	(movf_f0_m-bogus4,%a0),%d0			#get result
hatan15:	tst.b	(8,%a6)					#if arg < 0.0  ...
	bpl.w	hatan16
	bchg	&31,%d0					#change sign
hatan16:	unlk	%a6
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCTANGENT				  *
#							  *
# float fatan(arg) float arg;				  *
#							  *
# Errors: Underflow and inexact errors are ignored.	  *
#							  *
# Author: Mark McDowell  6/21/85			  *
#							  *
#**********************************************************

	text
	set	PI,0
c_fatan:
	lea	(4,%sp),%a0
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fatan.s	(%a0),%fp0		#calculate atan
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d1,%fpcr		#restore control register
	rts

#**********************************************************
#							  *
# SINGLE PRECISION ARCTANGENT2				  *
#							  *
# float fatan2(v,u) float v,u;				  *
#							  *
# Errors: If v = 0 and u = 0, matherr() is called with	  *
#		type = DOMAIN and retval = 0.0.		  *
#         Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/24/85			  *
#							  *
#**********************************************************

c_fatan2:	
	lea	(4,%sp),%a0
	lea	(8,%sp),%a1
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.s	(%a1),%fp0		#fp0 <- U
	fbeq	c_atan_3
	fmov.s	(%a0),%fp1		#fp1 <- V
	fdiv	%fp0,%fp1		#fp1 <- V/U
	fabs	%fp1			#fp1 <- |V/U|
	fatan	%fp1
	tst.b	(%a1)			#sign of U?
	bpl.b	c_atan_1
	fmovcr	&PI,%fp0
	fsub	%fp1,%fp0		#fp0 <- PI - result
	fmov	%fp0,%fp1
c_atan_1:	
	tst.b	(%a0)			#sign of V?
	bpl.b	c_atan_2
	fneg	%fp1			#result = -result
c_atan_2:	
	fmov.s	%fp1,%d0
	fmov.l	%d1,%fpcr		#restore control register
	rts
c_atan_3:
	ftest.s	(%a0)			#is V = 0?
	fbeq	c_atan_4
	fsnge	%d0
	fmov.l	%d1,%fpcr		#restore control register
	add.b	%d0,%d0			#get sign of V
	mov.l	&0x7F921FB6,%d0		#result is PI/2
	roxr.l	&1,%d0			#with sign of V
	rts
c_atan_4:	
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	fmov.s	(%a1)+,%fp0		#move U
	fmov.d	%fp0,-(%sp)
	fmov.s	(%a0)+,%fp0		#move V
	fmov.d	%fp0,-(%sp)
	pea	c_atan2name		#"fatan2"
	movq	&DOMAIN,%d0		#type = DOMAIN
	mov.l	%d0,-(%sp)
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_atan_5
	movq	&EDOM,%d0		#errno = EDOM
	mov.l	%d0,_errno
	pea	21.w
	pea	c_atan2msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#write(2,"fatan2: DOMAIN error\n",20)
	jsr	_write			#write(2,"fatan2: DOMAIN error\n",20)
')
	lea	(12,%sp),%sp
c_atan_5:	
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0
	fmov.l	(%sp)+,%fpcr
	rts

	data
c_atan2name:
	byte	102,97,116,97,110,50,0		#"fatan2"
c_atan2msg:
	byte	102,97,116,97,110,50,58,32,68	#"fatan2: DOMAIN error\n"
	byte	79,77,65,73,78,32,101,114
	byte	114,111,114,10,0

ifdef(`PROFILE',`
		data
p_fatan:	long	0
p_fatan2:	long	0
	')
