# @(#) $Revision: 70.1 $      
#*************************************************************
#              				 	 	     *
#  TANGENT/COTANGENT of IEEE 32 bit floating point number    *
#              				 	 	     *
#  argument is passed on the stack; answer is returned in d0 *
#              				 	 	     *
#  This routine does not check for or handle infinities,     *
#  denormalized numbers or NaN's but it does handle +0 and   *
#  -0.							     *
#              				 	 	     *
#  The algorithm used here is from Cody & Waite.  	     *
#              				 	 	     *
#  Author: Mark McDowell	September 29, 1984	     *
#							     *
#*************************************************************

		version 2
    		set	SING,2
     		set	TLOSS,5
    		set	EDOM,33
      		set	ERANGE,34
      		set	bogus4,0x18
          	set	addf_f0_f2,0x40C4
          	set	addf_f4_f3,0x4106
          	set	divf_f0_f1,0x4242
          	set	divf_f2_f3,0x4266
          	set	divf_f3_f2,0x4274
          	set	movf_f0_f1,0x4462
         	set	movf_f0_m,0x456C
         	set	movf_f1_m,0x4568
         	set	movf_f2_m,0x4564
         	set	movf_f3_m,0x4560
         	set	movf_m_f0,0x44FC
         	set	movf_m_f1,0x44F8
         	set	movf_m_f2,0x44F4
         	set	movf_m_f3,0x44F0
         	set	movf_m_f4,0x44EC
         	set	movf_m_f5,0x44E8
           	set	movfl_f0_f0,0x43C0
          	set	movil_m_f2,0x4528
           	set	movlf_f0_f0,0x4400
          	set	mulf_f0_f1,0x41C2
          	set	mulf_f0_f2,0x41C4
          	set	mulf_f1_f1,0x41D2
          	set	mulf_f1_f2,0x41D4
          	set	mulf_f1_f3,0x41D6
          	set	mull_f4_f2,0x4052
          	set	subl_f2_f0,0x4028

	data

excpt:	long	0,0,0,0,0,0,0,0

tan:	byte	102,116,97,110,0		#"ftan"

cot:	byte	102,99,111,116,0		#"fcot"

tantmsg:	byte	102,116,97,110,58,32,84,76,79	#"ftan: TLOSS error"
	byte	83,83,32,101,114,114,111,114
	byte	10,0

cottmsg:	byte	102,99,111,116,58,32,84,76,79	#"fcot: TLOSS error"
	byte	83,83,32,101,114,114,111,114
	byte	10,0

cotsmsg:	byte	102,99,111,116,58,32,83,73,78	#"fcot: SING error"
	byte	71,32,101,114,114,111,114,32
	byte	10,0

inv:	byte	165		#flags to invert answer

	text
ifdef(`_NAMESPACE_CLEAN',`
	global	__ftan,__fcot
	sglobal	_ftan,_fcot',`
	global	_ftan,_fcot
')

zero:	tst.l	%d7		#tangent or cotangent?
	bne.w	error1		#jump if cotangent
	move.l	(%sp)+,%d7	#restore d7
	unlk	%a6
	rts

error1:	moveq	&EDOM,%d1	#domain error
	move.l	%d1,-(%sp)	#save it
	moveq	&SING,%d1	#SING error
	move.l	%d1,excpt
	move.l	%d0,excpt+8	#argument
	clr.l	excpt+12
	move.l	&0x47EFFFFF,%d1	#MAX
	or.l	%d0,%d1		#give correct sign
	move.l	%d1,excpt+24	#return value
	move.l	&0xE0000000,excpt+28
	lea	cotsmsg,%a0	#message
	bra.w	error2

error:	moveq	&ERANGE,%d1	#range error
	move.l	%d1,-(%sp)	#save it
	moveq	&TLOSS,%d1	#TLOSS error
	move.l	%d1,excpt
	move.l	%d0,-(%sp)	#convert arg to double
ifdef(`_NAMESPACE_CLEAN',`
	jsr	___ftod',`
	jsr	_ftod
 ')
	addq.w	&4,%sp
	move.l	%d0,excpt+8	#save in excpt
	move.l	%d1,excpt+12
	clr.l	excpt+24	#return value is zero
	clr.l	excpt+28
	lea	tantmsg,%a0	#assume tangent
	lea	tan,%a1
	tst.l	%d7		#tangent or cotangent?
	beq.w	error3
	lea	cottmsg,%a0	#assume tangent
error2:	lea	cot,%a1
error3:	move.l	%a1,excpt+4	#name
	pea	(%a0)		#save pointer to message
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#notify user of error
	jsr	_matherr	#notify user of error
')
	addq.w	&4,%sp
	move.l	(%sp)+,%a0	#get pointer to error message
	move.l	(%sp)+,%d1	#get error type
	tst.l	%d0		#report error?
	bne.w	error4
	move.l	%d1,_errno	#set errno
	pea	18.w		#length of message
	pea	(%a0)		#message
	pea	2.w		#stderr
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#print the message
	jsr	_write		#print the message
 ')
	lea	(12,%sp),%sp
error4:	move.l	excpt+28,-(%sp)	#get return value
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
__fcot:')
_fcot:	
	ifdef(`PROFILE',`
	mov.l	&p_fcot,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fcot
	link	%a6,&0
#	trap	#2
#	dc.w	-24		space to save registers
	move.l	%d7,-(%sp)	#save d7
	moveq	&-1,%d7		#MSB is flag that this is cotangent
	bra.w	tancot

ifdef(`_NAMESPACE_CLEAN',`
__ftan:')
_ftan:	
	ifdef(`PROFILE',`
	mov.l	&p_ftan,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_ftan
	link	%a6,&0
#	trap	#2
#	dc.w	-24		space to save registers
	move.l	%d7,-(%sp)	#save d7
	moveq	&0,%d7		#MSB is flag that this is tangent

tancot:	move.l  (8,%a6),%d0	#get argument
	move.l  %d0,%d1		#save argument
	add.l   %d1,%d1		#remove sign
	beq.w	zero		#is it zero?
#
# Initial argument check
#
	cmp.l   %d1,&0x8B921000  #if too big, error (i.e. excessive 
	bcc.w     error		      #precision loss in argument reduction)
	tst.w	float_soft	#98635A card present?
	beq.w	htan
	movem.l	%d2-%d6,-(%sp)	#save registers
#
# Isolate mantissa (d0.l), exponent (d6.b) and sign (d7.b)
#
	rol.l   &8,%d0           #normalize argument and save exponent (almost)
        smi     %d2              #remember LSB of exponent
	moveq   &127,%d6         #exponent offset
	bset    %d6,%d0           #set hidden bit (modulo 32, remember?)
	add.b   %d2,%d2           #get LSB of exponent
	addx.b  %d0,%d0           #fix exponent
	subx.b  %d7,%d7           #save sign (quicker than scs d7)
	neg.b	%d7		#set "even" bit to 0
	sub.b   %d0,%d6           #exponent without offset
	clr.b   %d0              #only mantissa in d0.l
	cmp.l   %d1,&0x73713746	#if too small, almost done 
	bcs.w     tan8
#
# The argument must be reduced to the interval (0,PI/4). Actually, Cody & Waite
# specified (-PI/4,PI/4), but here we have taken advantage of the fact that
# tan(-x) = -tan(x) and have folded everything into the positive interval.
#  
        cmp.l	%d1,&0x7E921FB5	#argument reduction necessary?
	bls.w     tan9
#
# The argument will be multiplied by 2/PI and the result will be rounded to
# the nearest integer. This result will be multiplied by PI/2 and subtracted
# from the original argument to obtain a reduced argument in the range
# (-PI/4,PI/4). If it is negative, the absolute value is taken and the sign
# of the result is inverted.
#
        move.w  &0x28BE,%d1	#most significant bits of 2/PI
	move.w  &0x60DC,%d2	#least significant bits of 2/PI
	move.w  %d2,%d3
	mulu    %d0,%d3		#lo*lo
	move.w  %d1,%d4
	mulu    %d0,%d4		#lo*hi
	swap    %d0
	mulu    %d0,%d1		#hi*hi
	mulu    %d0,%d2		#hi*lo
	swap    %d0		#get mantissa back into position
#
# At this point the product is:
#
#        ----- d1.l -----  ----- d3.l -----
#                 ----- d4.l -----
#                 ----- d2.l -----
#
	add.l	%d4,%d2		#add middle products
	move.w	%d2,%d4
	clr.w	%d2		#will shift d2 left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift left 16
	swap	%d4		#shift right 16
	clr.w	%d4
	add.l	%d4,%d3		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
	swap	%d1		#need less than 16 bits
#
# The binary point for this multiplication is now located somewhere within
# d1.w with at least one bit to the right of this point. Where it is is
# dependent upon the original exponent. This product will converted to an
# integer by shifting it right the appropriate number of times and adding
# the round bit that falls off the end.
#
	moveq   &13,%d5		#to calculate shift
	add.b   %d6,%d5		#shift amount
	lsr.w   %d5,%d1		#shift to form integer
#
# If the argument contained an odd multiple of PI/2, the sign must be inverted.
#
	eor.b   %d1,%d7
#
# Round to the nearest integer for modulus.
#
	negx.w	%d1		#round it
	neg.w	%d1
	lsr.b	&1,%d7		#save sign in X bit
	move.b  %d1,%d7           #save "even" or "odd"
	addx.b	%d7,%d7		#restore sign bit
#
# We are now ready to multiply this integer by PI/2 to subtract it from the
# original argument. This argument could have been close to a multiple of
# PI/2 so it is critical that enough accuracy is provided in this multipli-
# cation/subtraction so there will be sufficient significant bits left
# over. For this reason, the integer will be multiplied by PI/2 accurate to
# 48 bits.
#
        lsr.l   &3,%d0		#prepare mantissa for subtraction
	addq.b	&1,%d6		#adjust exponent
	lsl.w   %d5,%d1		#shift integer back into place
	move.w  &0x2168,%d2	#lower 16 bits of PI/2
	move.w  &0xC90F,%d4	#upper 16 bits of PI/2
	mulu    %d1,%d2		#integer*lo PI/2
	mulu    %d1,%d4		#integer*hi PI/2
	mulu    &0xDAA2,%d1      #middle 16 bits of PI/2
#
# At this point the product is:
#
#        ----- d4.l -----  ----- d2.l -----
#                 ----- d1.l -----
#
# The full product will not actually be formed but each of its component parts
# will be subtracted from the original argument in d0.
#
	move.l  %d1,%d3		#d1 and d3 will contain halves of middle product
	neg.l   %d2		#subtract lower bits from zero
	subx.l  %d4,%d0		#subtract upper bits and borrow
	subx.b  %d4,%d4		#set flag for positive or negative result
	clr.w   %d3		#d3 for upper bits of middle product
	swap    %d3		#move into place
	swap    %d1		#d1 for lower bits of middle product
	clr.w   %d1		#isolate
	sub.l   %d1,%d2		#subtract lower bits of middle product
	subx.l  %d3,%d0		#subtract upper bits of middle product w/ borrow
	roxl.b  &1,%d4		#check for negative result from any subtraction
	beq.w     tan1
#
# This is to fold the interval (-PI/4,0) into (0,PI/4).
#
	neg.l   %d2		#absolute value of reduced argument
	negx.l  %d0
#
# The argument is in d0.l and it is necessary that it be normalized so that the
# most significant bit is bit 31.
#
tan1:    subq.b  &4,%d6		#move binary point to between bits 30 and 31
        move.w  %d0,%d5		#save lower 16 bits
	swap    %d0		#must check upper 16 bits for MSB
	move.w  %d0,%d3		#save upper 16 bits
	beq.w     tan2		#is any bit set in upper 16?
	swap    %d0		#yes: restore argument
	bra.w     tan3

tan2:    swap    %d2		#MSB not in upper 16
	move.w  %d2,%d0		#whole argument gets shifted 16
	clr.w	%d2
	add.b   &16,%d6		#sixteen bit shift
        move.w  %d5,%d3		#d3.w now has new upper 16

tan3:    clr.b   %d3		#want to check upper 8
	tst.w   %d3		#anything there?
	bne.w     tan4
	lsl.l   &8,%d0		#no: shift everything left 8
	rol.l   &8,%d2
	move.b  %d2,%d0
	clr.b	%d2
	addq.b  &8,%d6		#eight bit shift
#
# Now I am convinced that the MSB is in the upper 8 bits of d0.l.
#
tan4:    tst.l   %d0		#is it bit 31?
        bmi.w     tan6		#if so, it is normalized

	moveq   &-1,%d5		#counter for finding MSB
tan5:    add.l   %d0,%d0		#shift left 1
	dbmi    %d5,tan5		#is MSB at bit 31 yet?
	neg.b   %d5		#found MSB: make count positive
	add.b   %d5,%d6		#fudge the exponent
	rol.l   %d5,%d2		#need to shift in some bits to the lower end
	moveq	&0,%d3		#for mask
	bset	%d5,%d3		#set bit beyond upper end of mask
	subq.b	&1,%d3		#create mask
	and.b   %d2,%d3		#mask off bits
	eor.b	%d3,%d2		#remove them from where they came
	or.b    %d3,%d0		#and include them with d0.l
#
# Round f
#
tan6:	tst.l	%d2		#round bit set?
	bpl.w	tan7		#jump if not
	addq.l	&1,%d0		#round up
	neg.l	%d2		#sticky bit set?
	bpl.w	tan7		#jump if it is
	bclr	%d2,%d0		#round to even
#
# The argument may be very small, so check.
#
tan7:	cmp.b	%d6,&12		#is this a very small number?
	blt.w	tan9		#jump if large
	bgt.w	tan8		#jump if definitely small
	cmp.l	%d0,&0xB89BA300	#check boundary case
	bcc.w	tan9		#jump if too big
#
# For a small argument, the answer is either f or 1/f
#
tan8:	rol.w	&1,%d7		#shift flags into place
	move.l	%d0,%d4		#if no division, result will be expected in d4
	btst	%d7,inv.l	#must the number be inverted?
	bne.w	tan20		#jump if not
	move.l	&0x80000000,%d1	#numerator is 1.0
	neg.b	%d6		#change exponent
	addq.b	&1,%d6		#adjust exponent
	bra.w	tan14
#
# The argument has been reduced if necessary and is in the interval (eps,PI/4). 
# The binary point is between bits 30 and 31 of d0 with bit 31 set and d6.b 
# containing the number of right shifts needed to properly align this number 
# with respect to the binary point. We will conform to Cody & Waite's 
# terminology and call this argument 'f'.
#
tan9:	rol.w	&1,%d7		#shift flags into place
	move.l  %d0,%d5		#save f
	move.w	%d6,%a1		#and f's exponent
#
# The next thing calculated is g=f*f.
#
	move.w  %d0,%d1
	mulu    %d0,%d1		#lo*lo
	move.w  %d0,%d2
	swap    %d0
	mulu    %d0,%d2		#lo*hi and hi*lo
	mulu    %d0,%d0		#hi*hi
#
# At this point the product is:
#
#        ----- d0.l -----  ----- d1.l -----
#                 ----- d2.l -----
#                 ----- d2.l -----
#
	add.b	%d6,%d6		#exponent of g is twice that of f
	add.l	%d2,%d2		#add inner products
	move.w	%d2,%d3
	clr.w	%d2		#d2 will be shifted left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift d2 left 16
	swap	%d3		#shift d3 right 16
	clr.w	%d3
	add.l	%d3,%d1		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
	bmi.w	tan10		#normalized?
	add.l	%d1,%d1		#shift lower bits
	addx.l	%d0,%d0		#shift upper bits
	addq.b	&1,%d6		#adjust exponent
tan10:	tst.l	%d1		#round bit set?
	bpl.w	tan11		#jump if not
	addq.l	&1,%d0		#round up
	neg.l	%d1		#check sticky bits
	bpl.w	tan11		#jump if set
	bclr	%d1,%d0		#round to even
#
# Calculate p1 * g
#
tan11:	move.w	&0xC433,%d1	#upper bits of p1 (p1_1)
	move.w	&0xB837,%d2	#lower bits of p1 (p1_0)
	move.w	%d1,%d3		#d3 <- p1_1
	move.w	%d2,%d4		#d4 <- p1_0
	mulu	%d0,%d3		#d3 <- g0 * p1_1
	mulu	%d0,%d4		#d4 <- g0 * p1_0
	swap	%d0		#d0 <- (g0,g1)
	mulu	%d0,%d1		#d1 <- g1 * p1_1
	mulu	%d0,%d2		#d2 <- g1 * p1_0
	add.l	%d3,%d2		#add inner products
	move.w	%d2,%d3		#d3 <- lower 16 bits of sum of inner products
	clr.w	%d2		#to shift d2 left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift d2 left 16
	swap	%d3		#shift d3 right 16
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
#
# Calculate P(g) = p1 * g + 1
#
	addq.b	&1,%d6		#new exponent
	lsr.l	%d6,%d1		#shift result
	negx.l	%d1		#subtract rounded result from 1 (gives P(g))
#
# Calculate (p1 * g + 1) * f
#
	move.w	%d1,%d4		#d4 <- P0
	move.w	%d1,%d3		#d3 <- P0
	swap	%d1		#d1 <- (P0,P1)
	move.w	%d1,%d2		#d2 <- P1
	mulu	%d5,%d4		#d4 <- P0 * f0
	mulu	%d5,%d2		#d2 <- P1 * f0
	swap	%d5		#d5 <- (f0,f1)
	mulu	%d5,%d1		#d1 <- P1 * f1
	mulu	%d5,%d3		#d3 <- P0 * f1
	add.l	%d3,%d2		#add inner products
	move.w	%d2,%d3		#d3 <- lower 16 bits of sum of inner products
	clr.w	%d2		#to shift d2 left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift d2 left 16
	swap	%d3		#shift d3 right 16
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d1		#add upper 32 bits
	bmi.w	tan12		#is it normalized?
	add.l	%d4,%d4		#no: get a bit
	addx.l	%d1,%d1		#shift left
	addq.w	&1,%a1		#modify exponent
tan12:	tst.l	%d4		#check round bit
	bpl.w	tan13		#jump if not set
	addq.l	&1,%d1		#round up
	neg.l	%d4		#check sticky bit
	bpl.w	tan13		#jump if set
	bclr	%d4,%d1		#round to even
#
# Calculate q2 * g
#
tan13:	move.w	&0x9F33,%d5	#upper bits of q2 (q2_1)
	move.w	&0x7535,%d2	#lower bits of q2 (q2_0)
	move.w	%d5,%d3		#d3 <- q2_1
	move.w	%d2,%d4		#d4 <- q2_0
	mulu	%d0,%d5		#d5 <- g1 * q2_1
	mulu	%d0,%d2		#d2 <- g1 * q2_0
	swap	%d0		#d0 <- (g1,g0)
	mulu	%d0,%d3		#d3 <- g0 * q2_1
	mulu	%d0,%d4		#d4 <- g0 * q2_0
	add.l	%d3,%d2		#add inner products
	move.w	%d2,%d3		#d3 <- lower 16 bits of sum of inner products
	clr.w	%d2		#to shift d2 left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift d2 left 16
	swap	%d3		#shift d3 right 16
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d5		#add upper 32 bits
	move.l	&0xDBB7AF40,%d2	#q1
#
# Calculate q2 * g + q1
#
	addq.b	&2,%d6		#adjust exponent again
	lsr.l	%d6,%d5		#align binary points
	subx.l	%d5,%d2		#d2 <- x = -(q2*g + q1)
#
# Calculate (q2 * g + q1) * g
#
	move.w	%d2,%d4		#d4 <- x0
	mulu	%d0,%d4		#d4 <- g0 * x0
	move.w	%d2,%d3		#d3 <- x0
	move.w	%d0,%d5		#d5 <- g0
	swap	%d0		#d0 <- (g0,g1)
	swap	%d2		#d2 <- (x0,x1)
	mulu	%d0,%d3		#d3 <- g1 * x0
	mulu	%d2,%d0		#d0 <- g1 * x1
	mulu	%d5,%d2		#d2 <- g0 * x1
	add.l	%d3,%d2		#add inner products
	move.w	%d2,%d3		#d3 <- lower 16 bits of sum of inner products
	clr.w	%d2		#to shift d2 left
	addx.b	%d2,%d2		#save extend bit
	swap	%d2		#shift d2 left 16
	swap	%d3		#shift d3 right 16
	clr.w	%d3
	add.l	%d3,%d4		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
#
# Calculate Q(g) = (q2 * g + q1) * g + 1
#
	subq.b	&4,%d6		#adjust exponent again
	lsr.l	%d6,%d0		#align binary points
	negx.l	%d0		#d0 <- Q(g)
#
# Now determine if the answer will be f * P(g)/Q(g) or Q(g)/(f * P(g)).
#
	move.w	%a1,%d6		#get exponent
	btst	%d7,inv.l	#invert answer?
	bne.w	tan14
	exg	%d0,%d1		#yes
	neg.b	%d6		#adjust exponent
	addq.b	&2,%d6
#
# Perform the division.
#
tan14:	cmp.l	%d0,%d1		#divide overflow?
	bhi.w	tan15		#jump if not
	subq.b	&1,%d6		#adjust exponent
	lsr.l	&1,%d1		#shift so no overflow
	bcc.w	tan15		#round bit set?
	addq.l	&1,%d1		#round numerator
tan15:	moveq	&2,%d5		#counter
	moveq	&-1,%d4		#assume a quotient
	move.l	%d0,%d2		#d2 <- denominator
	clr.w	%d0
	swap	%d0
tan16:	swap	%d4		#get ready for next partial quotient
	divu	%d0,%d1		#division
	bvc.w	tan17		#check for overflow
	move.l	%d2,%d3		#d3 <- (Q1,Q0)
	neg.w	%d2		#instead of doing a multiply by FFFF,
	subx.l	%d0,%d3		   #form the product in (d3.l,d2.w)
	neg.w	%d2		#now subtract it from the dividend
	subx.l	%d3,%d1		
	swap	%d1		#shift remainder
	move.w	%d2,%d1
	subx.b	%d3,%d3		#move extend bit to d3
	bpl.w	tan19		#is FFFF correct?
	subq.w	&1,%d4		#no: it must be FFFE
	add.l	%d2,%d1		#add back divisor
	bra.w	tan19
tan17:	move.w	%d1,%d4		#save partial quotient
	clr.w	%d1		#shift dividend
	move.w	%d4,%d3		#save quotient
	mulu	%d2,%d3		#to calculate remainder
	sub.l	%d3,%d1		#remainder
	bcc.w	tan19		#did it go negative?
tan18:	subq.w	&1,%d4		#decrement quotient
	add.l	%d2,%d1		#add back divisor
	bcc.w	tan18		#until it goes positive
tan19:	subq.b	&1,%d5		#decrement loop counter
	bne.w	tan16
#
# All the pieces are done. Now put them together.
#
tan20:	moveq	&126,%d0		#exponent offset
	sub.b	%d6,%d0		#biased exponent
	lsl.w	&7,%d0		#move into place
	swap	%d0
	lsr.l	&8,%d4		#move mantissa into place
	bcc.w	tan21		#round bit set?
	addq.l	&1,%d4		#yes: round up
tan21:	add.l	%d4,%d0		#mantissa & exponent now in place
	btst	&1,%d7		#answer negative?
	beq.w	tan22
	bset	&31,%d0		#make it negative
tan22:	movem.l	(%sp)+,%d2-%d7	#restore registers
	unlk	%a6
	rts
#
# Code for using the float card begins here
#
htan:	clr.b	%d7		#initialize parity
	lea	float_loc+bogus4,%a0		#address of float card
	move.l	%d0,(movf_m_f0-bogus4,%a0)		#f0 <- X
        cmp.l   %d1,&0x7E921FB4  #is an argument reduction necessary?
        bls.w     htan1		#jump if not
#
# The argument will be multiplied by 2/PI and the result will be rounded to
# the nearest integer. This result will be multiplied by PI/2 and subtracted
# from the original argument to obtain a reduced argument in the range
# (-PI/4,PI/4).
#
	move.l	&0x3F22F983,(movf_m_f1-bogus4,%a0) #f1 <- 2/PI
	tst.w	(mulf_f0_f1-bogus4,%a0)		#f1 <- 2*X/PI
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f1_m-bogus4,%a0),%d1		#d1 <- 2*X/PI
	tst.w	(movfl_f0_f0-bogus4,%a0)		#f0,f1 <- X
	rol.l	&8,%d1				#move exp to lower byte
	smi	%d0				#save LSB of exp
	bset	&31,%d1				#set hidden bit
	add.b	%d0,%d0				#get LSB of exp
	addx.b	%d1,%d1				#d1.b becomes exp
	subx.b	%d7,%d7				#d7.b becomes sign
	moveq	&-98,%d0			#for calculating shift
	sub.b	%d1,%d0				#d0.b = shift
	lsr.l	%d0,%d1				#d1 <- TRUNC(2*Y/PI)
	negx.l	%d1				#round if needed
	tst.b	%d7				#sign?
	bmi.w	htan0
	neg.l	%d1
htan0:	move.b	%d1,%d7				#save parity
	movem.l	(%a0),%d0/%a1			#wait
	move.l	%d1,(movil_m_f2-bogus4,%a0)	#f2,f3 <- XN
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3FF921FB,(movf_m_f5-bogus4,%a0) #f4,f5 <- PI/2
	move.l	&0x54442D18,(movf_m_f4-bogus4,%a0)
	tst.w	(mull_f4_f2-bogus4,%a0)		#f2,f3 <- XN * PI/2
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(subl_f2_f0-bogus4,%a0)		#f0,f1 <- X - XN * PI/2
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movlf_f0_f0-bogus4,%a0)		#f0 <- f
	movem.l	(%a0),%d1/%a1			#wait
#
# Check for small argument
#
htan1:	ror.w	&1,%d7				#move flags to bits 14 & 15
	move.l	(movf_f0_m-bogus4,%a0),%d0		#d0 <- f
	move.l	%d0,%d1
	add.l	%d1,%d1				#abs(f)
	cmp.l	%d1,&0x73000000			#>eps?
	bcc.w	htan5
	add.w	%d7,%d7				#are the flags the same?
	bvs.w	htan2				#jump if f must be inverted
	bcs.w	htan3
	move.l	(%sp)+,%d7
	unlk	%a6
	rts
htan2:	move.l	&0x3f800000,(movf_m_f1-bogus4,%a0) #f1 <- 1.0
	tst.w	(divf_f0_f1-bogus4,%a0)		#f1 <- 1.0 / f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f1_m-bogus4,%a0),%d0		#d0 <- result
	tst.w	%d7				#tan or cot?
	bmi.w	htan4
htan3:	bchg	&31,%d0				#answer is -1/f
htan4:	move.l	(%sp)+,%d7
	unlk	%a6
	rts
#
# The argument has been reduced if necessary and is in the interval
# (1/(2**12),PI/4).
#
htan5:	tst.w	(movf_f0_f1-bogus4,%a0)		#f1 <- f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f1-bogus4,%a0)		#f1 <- g=f*f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0xBDC433B8,(movf_m_f2-bogus4,%a0) #f2 <- P1
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- P1 * g
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f0_f2-bogus4,%a0)		#f2 <- P1 * g * f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f0_f2-bogus4,%a0)		#f2 <- P1 * g * f + f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3C1F3375,(movf_m_f3-bogus4,%a0) #f3 <- Q2
	tst.w	(mulf_f1_f3-bogus4,%a0)		#f3 <- Q2 * g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0xBEDBB7AF,(movf_m_f4-bogus4,%a0) #f4 <- Q1
	tst.w	(addf_f4_f3-bogus4,%a0)		#f3 <- Q2 * g + Q1
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f3-bogus4,%a0)		#f3 <- (Q2 * g + Q1) * g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3F800000,(movf_m_f4-bogus4,%a0) #f4 <- Q0
	tst.w	(addf_f4_f3-bogus4,%a0)		#f3 <- (Q2 * g + Q1) * g + Q0
	add.w	%d7,%d7				#check flags
	scs	%d7				#save parity
	movem.l	(%a0),%d1/%a1			#wait
	bvs.w	htan6				#what divides what?
	tst.w	(divf_f3_f2-bogus4,%a0)		#f2 <- f * P(g) / Q(g)
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f2_m-bogus4,%a0),%d0		#d0 <- result
	bra.w	htan7
htan6:	tst.w	(divf_f2_f3-bogus4,%a0)		#f2 <- Q(g) / (f * P(g))
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f3_m-bogus4,%a0),%d0		#d0 <- result
htan7:	tst.b	%d7				#even or odd?
	beq.w	htan8
	bchg	%d7,%d0				#change sign
htan8:	move.l	(%sp)+,%d7
	unlk	%a6
	rts

#**********************************************************
#							  *
# SINGLE PRECISION TANGENT				  *
#							  *
# float ftan(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI/2*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and underflows are ignored.	  *
#	  Overflow errors return MAXFLOAT.		  *
#							  *
# Author: Mark McDowell  6/21/85			  *
#							  *
#**********************************************************

	text
c_ftan:
	lea	(4,%sp),%a0
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.s	(%a0),%fp0		#fp0 <- arg
	fabs	%fp0,%fp1
	fcmp.s	%fp1,&0f6.746519E9	#total loss of precision?
	fbgt	c_tan2
	ftan	%fp0			#calculate tan
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d1,%fpcr		#restore control register
	fmov.l	%fpsr,%d1		#get status register
	btst	&25,%d1			#overflow?
	bne.w	c_tan4
	rts
c_tan2:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	subq.w	&8,%sp
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_tan_name		#name = "ftan"
	movq	&TLOSS,%d0
	mov.l	%d0,-(%sp)		#type = TLOSS
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_tan3
	movq	&ERANGE,%d0
	mov.l	%d0,_errno		#errno = ERANGE
	pea	18.w
	pea	c_tan_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`			#write(2,"ftan: TLOSS error\n",18)
	jsr	_write			#write(2,"ftan: TLOSS error\n",18)
')
	lea	(12,%sp),%sp
c_tan3:	
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov.l	(%sp)+,%fpcr
	rts
c_tan4:
	add.l	%d0,%d0
	mov.l	0xFEFFFFFE,%d0		#result = MAXFLOAT
	roxr.l	&1,%d0
	rts


#**********************************************************
#							  *
# SINGLE PRECISION COTANGENT				  *
#							  *
# float fcot(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI/2*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#	  If arg = 0.0, matherr() is called with type =   *
#		SING and retval = MAXFLOAT.		  *
#         Inexact errors and underflows are ignored.	  *
#							  *
# Author: Mark McDowell  6/26/85			  *
#							  *
#**********************************************************

c_fcot:
	lea	(4,%sp),%a0
	fmov.l	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.s	(%a0),%fp0		#fp0 <- arg
	fbeq	c_cot2
	fabs	%fp0,%fp1
	fcmp.s	%fp1,&0f6.746519E9	#total loss of precision?
	fbgt	c_cot3
	ftan	%fp0,%fp1		#calculate cot
	fmov.w	&1,%fp0
	fdiv	%fp1,%fp0
	fmov.s	%fp0,%d0		#save result
	fmov.l	%d1,%fpcr		#restore control register
	fmov.l	%fpsr,%d1
	btst	&25,%d1			#overflow?
	bne.w	c_cot8
	rts
c_cot2:
	mov.l	%d1,-(%sp)		#save control register
	mov.l	&0xE0000000,-(%sp)
	mov.l	&0x47EFFFFF,-(%sp)
	movq	&SING,%d0
	bra.b	c_cot4
c_cot3:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)
	mov.l	%d0,-(%sp)
	movq	&TLOSS,%d0
c_cot4:
	subq.w	&8,%sp
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_cot_name
	mov.l	%d0,-(%sp)		#type
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	addq.w	&6,%sp
	tst.l	%d0
	bne.b	c_cot7
	cmp.w	(%sp),&SING
	bne.b	c_cot5
	movq	&EDOM,%d0		#singularity error
	mov.l	%d0,_errno		#set errno
	pea	17.w
	pea	c_sing_msg
	bra.b	c_cot6
c_cot5:
	movq	&ERANGE,%d0		#loss of precision error
	mov.l	%d0,_errno		#set errno
	pea	18.w
	pea	c_tloss_msg
c_cot6:
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`
	jsr	_write
 ')
	lea	(12,%sp),%sp
c_cot7:	
	lea	(22,%sp),%sp
	movq	&0,%d0
	fmov.l	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov.l	(%sp)+,%fpcr		#restore control register
	rts
c_cot8:	
	add.l	%d0,%d0
	mov.l	&0xFEFFFFFE,%d0
	roxr.l	&1,%d0
	rts

	data
c_tan_name:
	byte	102,116,97,110,0		#"ftan"
c_tan_msg:
	byte	102,116,97,110,58,32,84,76,79	#"ftan: TLOSS error\n"
	byte	83,83,32,101,114,114,111,114
	byte	10,0
c_cot_name:
	byte	102,99,111,116,0		#"fcot"
c_tloss_msg:
	byte	102,99,111,116,58,32,84,76,79	#"fcot: TLOSS error\n"
	byte	83,83,32,101,114,114,111,114
	byte	10,0
c_sing_msg:
	byte	102,99,111,116,58,32,83,73	#"fcot: SING error\n"
	byte	78,71,32,101,114,114,111,114
	byte	10,0

ifdef(`PROFILE',`
	data
p_ftan:	long	0
p_fcot:	long	0
	')
