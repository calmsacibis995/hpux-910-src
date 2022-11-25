# @(#) $Revision: 70.1 $      
#*************************************************************
#              				 	 	     *
#  COSINE of IEEE 32 bit floating point number   	     *
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
     		set	TLOSS,5
      		set	ERANGE,34
      		set	bogus4,0x18
          	set	addf_f2_f0,0x40E0
          	set	addf_f3_f2,0x40F4
          	set	addl_f2_f0,0x4008
          	set	movf_f0_f1,0x4462
         	set	movf_f0_m,0x456C
         	set	movf_f4_m,0x455C
         	set	movf_m_f0,0x44FC
         	set	movf_m_f2,0x44F4
         	set	movf_m_f3,0x44F0
         	set	movf_m_f4,0x44EC
         	set	movf_m_f5,0x44E8
           	set	movfl_f0_f0,0x43C0
          	set	movfl_m_f0,0x453C
          	set	movil_m_f2,0x4528
          	set	movl_f0_f4,0x4444
           	set	movlf_f0_f0,0x4400
          	set	movlf_f4_m,0x4574
          	set	mulf_f0_f2,0x41C4
          	set	mulf_f1_f1,0x41D2
          	set	mulf_f1_f2,0x41D4
          	set	mull_f2_f4,0x404C
          	set	mull_f4_f2,0x4052
          	set	subl_f2_f0,0x4028

	data

excpt:	long	0,0,0,0,0,0,0,0

sine:	byte	102,115,105,110,0		#"fsin"

cosine:	byte	102,99,111,115,0		#"fcos"

sinmsg:	byte	102,115,105,110,58,32,84,76,79	#"fsin: TLOSS error"
	byte	83,83,32,101,114,114,111,114
	byte	10,0

cosmsg:	byte	102,99,111,115,58,32,84,76,79	#"fcos: TLOSS error"
	byte	83,83,32,101,114,114,111,114
	byte	10,0

	text
ifdef(`_NAMESPACE_CLEAN',`
        global  __fsin,__fcos
        sglobal _fsin,_fcos',`
        global  _fsin,_fcos
 ')

serror:	clr.b	-(%sp)		#flag sine
	move.l	&sine,excpt+4	#name
	bra.w	error

cerror:	st	-(%sp)		#flag cosine
	move.l	&cosine,excpt+4	#name

error:	moveq	&TLOSS,%d1	#TLOSS error
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
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#notify user of error
	jsr	_matherr	#notify user of error
')
	addq.w	&4,%sp
	tst.l	%d0		#report error?
	bne.w	error2
	moveq	&ERANGE,%d0	#range error
	move.l	%d0,_errno	#set errno
	lea	sinmsg,%a0	#assume sine
	tst.b	(%sp)		#sine or cosine?
	beq.w	error1
	lea	cosmsg,%a0	#cosine
error1:	pea	18.w		#length of message
	pea	(%a0)		#message
	pea	2.w		#stderr
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#print the message
	jsr	_write		#print the message
 ')
	lea	(12,%sp),%sp
error2:	move.l	excpt+28,-(%sp)	#get return value
	move.l	excpt+24,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	___dtof',`		#convert to single precision
	jsr	_dtof		#convert to single precision
')
	unlk	%a6
	rts

ansone:  move.l  &0x3F800000,%d0  #answer is one
	unlk	%a6
        rts

ifdef(`_NAMESPACE_CLEAN',`
__fcos:
')
_fcos:
	ifdef(`PROFILE',`
	mov.l	&p_fcos,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fcos
	link	%a6,&0
#	trap	#2
#	dc.w	-22		space to save registers
	move.l  (8,%a6),%d0	#get argument
	move.l  %d0,%d1		#save argument
#
# Initial argument checks
#
	add.l   %d1,%d1		#remove sign
	cmp.l	%d1,&0x73000000	#will result be 1?
        bls.w     ansone
	cmp.l   %d1,&0x8E000000  #if too big, error (i.e. excessive 
	bcc.w     cerror		      #precision loss in argument reduction)
#
# Check for float card
#
	tst.w	float_soft
	beq.w	hcos
#
# Save environment
#
        movem.l %d2-%d6,-(%sp)     #save registers
        move.b  %d7,-(%sp)        #save d7.b (only byte will be used)
#
# Isolate mantissa (d0.l), exponent (d6.b) and sign (d7.b)
#
	rol.l   &8,%d0           #normalize argument and save exponent (almost)
        smi     %d2              #remember LSB of exponent
	moveq   &127,%d6         #exponent offset
	bset    %d6,%d0           #set hidden bit (modulo 32!)
	add.b   %d2,%d2           #get LSB of exponent
	addx.b  %d0,%d0           #fix exponent
	clr.b   %d7              #cos(x)=cos(-x) so sign is unimportant
	sub.b   %d0,%d6           #exponent without offset
	clr.b   %d0              #only mantissa in d0.l
#
# Using the formula, cos(x)=sin(x+PI/2), we add PI/2 to x and jump to the
# SINE routine.
#
	moveq	&0,%d2		#lower 32 bits
	move.l	&0xC90FDAA2,%d1	#upper 32 bits of PI/2
	move.l	&0x2168C235,%d3	#lower 32 bits of PI/2
	move.b  %d6,%d5		#save exponent
        beq.w     cos3		#exponent the same as PI/2?
        bpl.w     cos1		#is exponent of PI/2 less?
	exg     %d0,%d1		#swap addend and augend for alignment
	exg	%d2,%d3
        neg.b   %d5		#make shift amount positive
        bra.w	cos2
cos1:	clr.b	%d6		#new exponent
cos2:	ror.l	%d5,%d0		#shift upper 32 bits
	lsr.l	%d5,%d2		#shift lower 32 bits
	moveq	&32,%d4
	sub.b	%d5,%d4		#LSB of mask
	moveq	&0,%d5
	bset	%d4,%d5		#bit set is same as LSB
	neg.l	%d5		#create mask
	and.l	%d0,%d5		#save bits
	eor.l	%d5,%d0		#remove them from where they came
	or.l	%d5,%d2		#put them into their proper place
cos3:    add.l   %d3,%d2		#add PI/2
	addx.l	%d1,%d0
	bcc.w     sincos		#overflow?
	roxr.l  &1,%d0		#yes: move MSB into register
	roxr.l	&1,%d2
	subq.b  &1,%d6		#increment exponent (sign is flipped)
        bra.w 	sincos		#catch the sine routine (always needs reduction)

#*************************************************************
#              				 	 	     *
#  SINE of IEEE 32 bit floating point number   	     	     *
#              				 	 	     *
#  argument is passed on the stack; answer is returned in d0 *
#              				 	 	     *
#  If the argument is too large, 0 is returned and errno is  *
#  set to EDOM. This routine does not check for or handle    *
#  infinities, denormalized numbers or NaN's but it does     *
#  handle +0 and -0.					     *
#              				 	 	     *
#  The algorithm used here is from Cody & Waite.  	     *
#              				 	 	     *
#*************************************************************

done:    unlk	%a6
	rts

#
# Several constants are required. Listed below are pairs of numbers required for
# the polynomial evaluation. The first number is an offset for the amount to 
# shift the result, and the second is the coefficient. The final number is a
# flag that polynomial evaluation is complete.
#
const:   short    0x0000
	long    0xCFB22280      #r3 = 0.1980741872E-3
        short    0x00FE
        long    0x88873D9D      #r2 = 0.8333025139E-2
        short    0x00FE
        long    0xAAAAA3F7      #r1 = 0.1666665668E+0
        short    0x8000

ifdef(`_NAMESPACE_CLEAN',`
__fsin:
')
_fsin:	
	ifdef(`PROFILE',`
	mov.l	&p_fsin,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_fsin
	link	%a6,&0
#	trap	#2
#	dc.w	-22		to save registers
	move.l  (8,%a6),%d0	#get argument
	move.l  %d0,%d1		#save argument
#
# Initial argument checks
#
	add.l   %d1,%d1		#remove sign
	cmp.l   %d1,&0x73D12ED2	#if too small, done 
	bcs.w     done                  #(i.e. sin(x) = x for small x)
	cmp.l   %d1,&0x8E000000  #if too big, error (i.e. excessive 
	bcc.w     serror		      #precision loss in argument reduction)
#
# Check for float card
#
	tst.w	float_soft
	beq.w	hsin
#
# Save environment
#
        movem.l %d2-%d6,-(%sp)     #save registers
        move.b  %d7,-(%sp)        #save d7.b (only byte will be used)
#
# Isolate mantissa (d0.l), exponent (d6.b) and sign (d7.b)
#
	rol.l   &8,%d0           #normalize argument and save exponent (almost)
        smi     %d2              #remember LSB of exponent
	moveq   &127,%d6         #exponent offset
	bset    %d6,%d0           #set hidden bit (modulo 32!)
	add.b   %d2,%d2           #get LSB of exponent
	addx.b  %d0,%d0           #fix exponent
	subx.b  %d7,%d7           #save sign (quicker than scs d7)
	sub.b   %d0,%d6           #exponent without offset
	clr.b   %d0              #only mantissa in d0.l
#
# The argument must be reduced to the interval (0,PI/2). Actually, Cody & Waite
# specified (-PI/2,PI/2), but here we have taken advantage of the fact that
# sin(-x) = -sin(x) and have folded everything into the positive interval.
#  
        cmp.l   %d1,&0x7F921FB4  #is an argument reduction necessary?
        bls.w     sin11		#jump if not
	moveq	&0,%d2		#lower 32 bits are 0
#
# The argument will be multiplied by 1/PI and the result will be rounded to
# the nearest integer. This result will be multiplied by PI and subtracted
# from the original argument to obtain a reduced argument in the range
# (-PI/2,PI/2). If it is negative, the absolute value is taken and the sign
# of the result is inverted.
#
sincos:  move.w  &0xA2F9,%d1	#most significant bits of 1/PI
	move.w  &0x836E,%d5	#least significant bits of 1/PI
	move.w  %d5,%d3
	mulu    %d0,%d3		#lo*lo
	move.w  %d1,%d4
	mulu    %d0,%d4		#lo*hi
	swap    %d0
	mulu    %d0,%d1		#hi*hi
	mulu    %d0,%d5		#hi*lo
	swap    %d0		#get mantissa back into position
#
# At this point the product is:
#
#        ----- d1.l -----  ----- d3.l -----
#                 ----- d4.l -----
#                 ----- d5.l -----
#
	add.l	%d5,%d4		#add inner product
	move.l	%d4,%d5
	clr.w	%d4		#remove lower 16 bits
	addx.b	%d4,%d4		#save extend bit
	swap	%d4		#shift left 16
	swap	%d5		#shift right 16
	clr.w	%d5
	add.l	%d5,%d3		#add lower 32 bits
	addx.l	%d4,%d1		#add upper 32 bits
	swap	%d1
#
# The binary point for this multiplication is now located somewhere within
# d1.w with at least one bit to the right of this point. Where it is is
# dependent upon the original exponent. This product will converted to an
# integer by shifting it right the appropriate number of times and adding
# the round bit that falls off the end.
#
	moveq   &16,%d5		#to calculate shift
	add.b   %d6,%d5		#shift amount
	lsr.w   %d5,%d1		#shift to form integer
#
# If the argument contained an odd multiple of PI, the sign must be inverted.
#
	eor.b   %d1,%d7
#
# Round to the nearest integer for modulus.
#
	negx.w	%d1		#round it
	neg.w	%d1
#
# We are now ready to multiply this integer by PI to subtract it from the
# original argument. This argument could have been close to a multiple of
# PI so it is critical that enough accuracy is provided in this multipli-
# cation/subtraction so there will be sufficient significant bits left
# over. For this reason, the integer will be multiplied by PI accurate to
# 48 bits.
#
	subq.b	&2,%d5
	lsl.w   %d5,%d1		#shift integer back into place
	moveq	&7,%d3		#for bits to shift off
	and.b	%d0,%d3		#save the bits
	lsr.l	&3,%d0		#shift upper 32 bits
	lsr.l	&3,%d2		#shift lower 32 bits
	ror.l	&3,%d3		#shift bits that move from d0 to d2
	or.l	%d3,%d2		#put them in place
	move.w	%d1,%d3		#d3 <- integer
	mulu	&0xDAA2,%d3	#middle product
	move.w	%d3,%d4		#save lower 16 bits
	clr.w	%d3		#remove lower 16 bits
	swap	%d3		#move into place for subtraction
	swap	%d4		#more moving into place
	clr.w	%d4
	sub.l	%d4,%d2		#subtract middle product
	subx.l	%d3,%d0
	subx.b	%d4,%d4		#set flag for positive or negative result
	move.w	%d1,%d3		#d3 <- integer
	mulu	&0xC90F,%d1	#upper product
	mulu	&0x2168,%d3	#lower product
	sub.l	%d3,%d2		#more subtraction
	subx.l	%d1,%d0
	roxl.b  &1,%d4		#check for negative result from any subtraction
	beq.w     sin1
#
# This is to fold the interval (-PI/2,0) into (0,PI/2).
#
	neg.l   %d2		#absolute value of reduced argument
	negx.l  %d0
#
# The reduced argument may be very small in which case sin(x)=x. Compare the
# argument against 1/(2**12).
#
sin1:    addq.b  &2,%d5		#use exponent info to form 1/(2**12)
	moveq   &0,%d4		#will create 1/(2**12)
	bset    %d5,%d4		#put bit in the right place
	cmp.l   %d0,%d4		#is it small?
	bge.w     sin5		#jump if it's big enough to continue
#
# The result is now in (d0.l,d2.l). We will need to find the MSB and normalize.
#
	moveq   &16,%d3		#constant
        subq.b  &1,%d5		#MSB search from 1/(2**13)
	tst.l	%d0		#is MSB in upper 32 bits?
	bne.w	sin2		#jump if it is
	swap	%d2		#will shift left 16
	move.w	%d2,%d0
	clr.w	%d2
	moveq	&15,%d5		#start searching from bit 15
	add.b	%d3,%d6		#adjust exponent
sin2:    btst    %d5,%d0		#is this MSB?
	dbne    %d5,sin2		#if not, keep searching
	moveq   &23,%d4		#where MSB must go
	sub.b   %d5,%d4		#d4 is amount to shift left
	cmp.b   %d4,%d3		#shift at least 16?
	blt.w     sin3		#jump if not
	sub.b   %d3,%d4		#if so, decrease shift by 16
	swap    %d0		#and do the shift
	swap    %d2
	move.w  %d2,%d0
	clr.w	%d2		#shift in 0's
sin3:    moveq   &0,%d3		#d3 will be the mask for shifting
	bset	%d4,%d3		#create starting bit for mask
	subq.w	&1,%d3		#set correct number of 1's
	lsl.l   %d4,%d0		#shift upper result bits
	rol.l   %d4,%d2		#shift lower result bits
	and.w   %d2,%d3		#mask off what I need
	eor.w	%d3,%d2		#clear field moved out
	or.w    %d3,%d0		#put into upper part
	tst.l	%d2		#check round bit
	bpl.w	sin4		#if not set, don't round
	addq.l	&1,%d0		#otherwise round up
	neg.l	%d2		#if exactly halfway, this 'neg' will
	bpl.w	sin4			#give a negative result
	bclr	%d2,%d0		#round to even (d2 must be 0x80000000)
sin4:	move.l  %d0,%d2		#result must be in d2 for cleanup
	moveq   &98,%d0		#doctor up exponent:
	add.b   %d5,%d0			#dependent upon where MSB was
	sub.b   %d6,%d0			#dependent upon original exponent
	bra.w     sin17		#clean up
#
# The argument is in d0.l and it is necessary that it be normalized so that the
# most significant bit is bit 31.
#
sin5:    subq.b  &3,%d6		#move binary point to between bits 30 and 31
        move.w  %d0,%d5		#save lower 16 bits
	swap    %d0		#must check upper 16 bits for MSB
	move.w  %d0,%d3		#save upper 16 bits
	beq.w     sin6		#is any bit set in upper 16?
	swap    %d0		#yes: restore argument
	bra.w     sin7

sin6:    swap    %d2		#MSB not in upper 16
	move.w  %d2,%d0		#whole argument gets shifted 16
	clr.w	%d2		#shift in 0's
	add.b   &16,%d6		#shift amount
        move.w  %d5,%d3		#d3.w now has new upper 16

sin7:    clr.b   %d3		#want to check upper 8
	tst.w   %d3		#anything there?
	bne.w     sin8		#jump if there is
	lsl.l   &8,%d0		#no: shift everything left 8
	rol.l   &8,%d2		#move lower bits into position
	move.b  %d2,%d0
	clr.b	%d2		#shift in 0's
	addq.b  &8,%d6		#more shift amount
#
# Now I am convinced that the MSB is in the upper 8 bits of d0.l.
#
sin8:    tst.l   %d0		#is it bit 31?
        bmi.w     sin10		#if so, it is normalized

	moveq   &-1,%d5		#counter for finding MSB
sin9:    add.l   %d0,%d0		#shift left 1
	dbmi    %d5,sin9		#is MSB at bit 31 yet?
	neg.b   %d5		#found MSB: make count positive
	add.b   %d5,%d6		#fudge the exponent
	rol.l   %d5,%d2		#need to shift in some bits to the lower end
	moveq   &0,%d3		#mask
	bset	%d5,%d3		#lowest bit to be set
	subq.b	&1,%d3		#create mask
	and.b   %d2,%d3		#mask off bits
	eor.b	%d3,%d2		#remove them
	or.b    %d3,%d0		#and include them with d0.l
sin10:	tst.l	%d2		#round bit set
	bpl.w	sin11		#jump if not
	addq.l	&1,%d0		#round up
	neg.l	%d2		#if halfway (ie. 0x80000000), 'neg'
	bpl.w	sin11			#will give 0x80000000
	bclr	%d2,%d0		#round to even
#
# The argument has been reduced if necessary and is in the interval
# (1/(2**12),PI/2). The binary point is between bits 30 and 31 of d0 with
# bit 31 set and d6.b containing the number of right shifts needed to properly 
# align this number with respect to the binary point. The least significant bit 
# of d7 is the sign of the result. We will conform to Cody & Waite's 
# terminology and call this argument 'f'.
#
sin11:	move.l  %d0,%a0		#save f
#
# The next thing calculated is g=f*f.
#
	move.w  %d0,%d1		#d1 <- lo
	mulu    %d0,%d1		#lo*lo
	move.w  %d0,%d2		#d2 <- lo
	swap    %d0		#d0 <- lo,hi
	mulu    %d0,%d2		#lo*hi and hi*lo
	mulu    %d0,%d0		#hi*hi
#
# At this point the product is:
#
#        ----- d0.l -----  ----- d1.l -----
#                 ----- d2.l -----
#                 ----- d2.l -----
#
# To make later computations faster, the final product will be formed in
# (d0.w,d1.w) instead of in a single 32 bit register.
#
	add.l	%d2,%d2		#lo*hi + hi*lo
	move.l	%d2,%d4		#save sum
	clr.w	%d2		#only upper 16 for d2
	addx.b	%d2,%d2		#save extend bit from addition
	swap	%d2		#move into place
	swap	%d4		#d4 will be lower 16 bits
	clr.w	%d4		#remove upper 16
	add.l	%d4,%d1		#add lower 32 bits
	addx.l	%d2,%d0		#add upper 32 bits
	tst.l	%d1		#check round bit
	bpl.w	sin12		#jump if not set
	addq.l	&1,%d0		#round up
	neg.l	%d1		#if exactly halfway (ie. 0x80000000)
	bpl.w	sin12			#neg will make it negative again
	bclr	%d1,%d0		#round to even
sin12:	move.w	%d0,%d1		#g will be in (d0.w,d1.w)
	move.w	%d6,%d0		#save exponent of f
	swap	%d0		#move into place
	add.b	%d6,%d6		#exponent of g is 2 times exponent of f
	addq.b	&4,%d6		#move binary point again
#
# R(g)=(((r4*g+r3)*g+r2)*g+r1)*g
# g is in (d0.w,d1.w)
#
# This polynomial is evaluated in the following loop. First is the 
# multiplication by g. Then a value is fetched from a table to indicate the 
# amount of shifting to do after the next multiplication. The end of the 
# polynomial evaluation is signalled by this value being negative.
#
# The success of this algorithm depends a great deal on the polynomial being
# evaluated. That is, it depends on the range of the original argument and the
# possible ranges of intermediate results. It also capitalizes on the fact that 
# the signs of successive coefficients alternate. In short, this routine is by
# no means general purpose.
#
	move.l  &0xAE9C5A92,%d2	#r4
	lea     const,%a1	#pointer to table
#
# First multiply by g.
#
sin13:	move.w  %d1,%d3
	mulu    %d2,%d3		#lo*lo
	move.w  %d0,%d4
	mulu    %d2,%d4		#hi*lo
	swap    %d2
	move.w  %d1,%d5
	mulu    %d2,%d5		#lo*hi
	mulu    %d0,%d2		#hi*hi
#
# At this point the product is:
#
#        ----- d2.l -----  ----- d3.l -----
#                 ----- d4.l -----
#                 ----- d5.l -----
#
	add.l	%d5,%d4		#lo*hi + hi*lo
	move.l	%d4,%d5		#save
	clr.w	%d4		#d4 will be upper 16 bits
	addx.b	%d4,%d4		#save extend bit from add
	swap	%d4		#move into place
	swap	%d5		#d5 will be lower 16 bits
	clr.w	%d5		#remove upper 16 bits
	add.l	%d5,%d3		#add lower 32 bits
	addx.l	%d4,%d2		#add upper 32 bits
#
# Shift the result in preparation for the later addition. The shift amount
# is in d6.w. The result at this point is found in (d2.l,d3.l).
#
	moveq	&0,%d4		#in case there is no shift
        tst.b  	%d6		#check shift
	beq.w     sin14		#shift any?
	moveq	&32,%d4		#constant
	sub.b	%d6,%d4		#d4 is used to create mask
	moveq	&0,%d5		#d5 will contain mask
	bset	%d4,%d5		#set LSB of mask
	neg.l	%d5		#extend this bit
	ror.l	%d6,%d3		#shift lower 32
	move.l	%d3,%d4		#d4 will catch extra bits
	and.l	%d5,%d4		#mask off what is not needed
	eor.l	%d4,%d3		#remove from d3 what went into d4
	ror.l	%d6,%d2		#shift upper 32
	and.l	%d2,%d5		#mask off what will go into d3
	or.l	%d5,%d3		#put it into d3
	eor.l	%d5,%d2		#remove these bits from d2
#
# (d2.l,d3.l,d4.l) has the shifted result. We will now get the shift amount for 
# the next time around. We do it here because a negative shift implies the end
# of polynomial evaluation. Notice that this shift amount is also a function of
# the position of the binary point (as would be expected).
#
sin14:	add.w   (%a1)+,%d6	#get shift offset
	bmi.w     sin15		#are we done?
	move.l  %d2,%d5		#save result so far
	move.l  (%a1)+,%d2	#get constant from table
	add.l   %d3,%d3		#need round bit
	subx.l  %d5,%d2		#subtract constant from result with carry
	or.l	%d4,%d3		#how about the sticky bit?
	bne.w	sin13		#no further action if set
	bclr	%d3,%d2		#round to even
	bra.w     sin13		#continue "polynomiating"
#
# -R(g) is found in (d2.l,d3.l). The sine result is found by the equation 
# f+f*R(g). This is equivalent to f*(1+R(g)) and although this could lose
# precision, the operation is done carefully. "1+R(g)" is easy because it only 
# involves subtracting (d2.l,d3.l) from 1. It just so happens that the binary 
# point is at the extreme left of this number and 0 < R(g) < 1. The "1" in 
# "1.00000..." is therefore off in limbo, and whether I subtract R(g) from 1 or 
# 0 makes no difference.
#
sin15:	add.l   %d3,%d3		#get round bit
        negx.l  %d2		#subtract from "1"
	or.l	%d4,%d3		#sticky bit?
	bne.w	sin16		#okay if set
	bclr	%d3,%d2		#otherwise, round to even
#
# Next...multiply by f.
#
sin16:	move.l  %a0,%d1		#get f
	move.w  %d1,%d3
	mulu    %d2,%d3		#lo*lo
	move.w  %d1,%d4
	move.w  %d2,%d5
	swap    %d1
	swap    %d2
	mulu    %d1,%d5		#hi*lo
	mulu    %d2,%d4		#lo*hi
	mulu    %d1,%d2		#hi*hi
#
# At this point the product is:
#
#        ----- d2.l -----  ----- d3.l -----
#                 ----- d4.l -----
#                 ----- d5.l -----
#
	add.l	%d5,%d4		#hi*lo + lo*hi
	move.l	%d4,%d5		#save
	clr.w	%d4		#d4 will have upper 16 of sum
	addx.b	%d4,%d4		#save extend bit
	swap	%d4		#move into position
	swap	%d5		#d5 is lower 16 of sum
	clr.w	%d5		#remove upper 16
	add.l	%d5,%d3		#add lower 32 bits
	sne	%d3		#consider it a sticky bit
	addx.l	%d4,%d2		#add upper 32 bits
#
# Either bit 30 or 31 is the MSB and which one is set will affect the result's
# exponent. Therefore, flag where that bit is.
#
	spl     %d1		#MSB flag
	swap    %d0		#move exponent to d0.w (we stashed it, remember?)
        moveq   &0x7E,%d4	#sort of an exponent offset
        sub.b   %d0,%d4		#affected by exponent of f
        move.l  %d4,%d0		#(this is important because we want the
#				upper 16 bits to be 0.)
        add.b   %d1,%d0		#also affected by position of MSB
	addq.b  &8,%d1		#how much to shift result mantissa
	move.b	%d2,%d6		#save lower 8 bits (round and sticky)
	add.b	%d6,%d6		#if shift is 8, removes round bit
	bclr	%d1,%d6		#this removes round bit for sure
	lsr.l   %d1,%d2		#shift mantissa
	bcc.w	sin17		#what was round bit?
	addq.l	&1,%d2		#if set, round up
	or.b	%d6,%d3		#check sticky bit
	bne.w	sin17		#if set, answer is okay
	bclr	%d3,%d2		#otherwise, round to even
#
# This is the common final cleanup routine.
#
# The mantissa is in d2.l's lower 24 bits. The exponent - 1 is in d0.b. (-1
# so we don't have to clear the hidden bit before combining fields.) The sign
# of the result is in the LSB of d7.
#
sin17:	lsl.w   &8,%d0		#move exponent into place
	lsr.b   &1,%d7		#get sign bit
	roxr.w  &1,%d0		#combine sign bit and exponent
	swap    %d0		#get ready for mantissa
	add.l   %d2,%d0		#insert mantissa
	move.b  (%sp)+,%d7	#restore registers
        movem.l (%sp)+,%d2-%d6
	unlk	%a6
	rts			#THE END.
#
# Code for using the float card begins here
#
hcos:	move.b  %d7,-(%sp)        #save d7.b (only byte will be used)
	clr.b	%d7		#0 means positive sign
#
# Using the formula, cos(x)=sin(x+PI/2), we add PI/2 to x and jump to the
# SINE routine.
#
	lea	float_loc+bogus4,%a0		#address of float card
	lsr.l	&1,%d1				#d1 <- abs(X)
	move.l	%d1,(movfl_m_f0-bogus4,%a0)	#f0,f1 <- abs(X)
	move.l	&0x3FF921FB,(movf_m_f3-bogus4,%a0) #f2,f3 <- PI/2
	move.l	&0x54442D18,(movf_m_f2-bogus4,%a0) 
	tst.w	(addl_f2_f0-bogus4,%a0)		#f0,f1 <- abs(X) + PI/2
	movem.l	(%a0),%d1/%a1			#wait
        bra.w 	hsincos		#catch the sine routine (always needs reduction)

hsin:	move.b  %d7,-(%sp)        #save d7.b (only byte will be used)
	tst.l	%d0		#sign?
	smi	%d7		#SGN <- sign
	lea	float_loc+bogus4,%a0		#address of float card
	move.l	%d1,%d0				#d0 <- X << 1
	lsr.l	&1,%d0				#d0 <- abs(X)
	move.l	%d0,(movf_m_f0-bogus4,%a0)		#f0 <- abs(X) (=Y)
        cmp.l   %d1,&0x7F921FB4  #is an argument reduction necessary?
        bls.w     hsin2		#jump if not
	tst.w	(movfl_f0_f0-bogus4,%a0)		#f0,f1 <- Y
	movem.l	(%a0),%d1/%a1			#wait
#
# The argument will be multiplied by 1/PI and the result will be rounded to
# the nearest integer. This result will be multiplied by PI and subtracted
# from the original argument to obtain a reduced argument in the range
# (-PI/2,PI/2).
#
hsincos:	tst.w	(movl_f0_f4-bogus4,%a0)		#f4,f5 <- Y
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3FD45F30,(movf_m_f3-bogus4,%a0) #f2,f3 <- 1/PI
	move.l	&0x6DC9C883,(movf_m_f2-bogus4,%a0)
	tst.w	(mull_f2_f4-bogus4,%a0)		#f4,f5 <- Y/PI
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movlf_f4_m-bogus4,%a0),%d1	#d1 <- Y/PI
	rol.l	&8,%d1				#move exp to lower byte
	smi	%d0				#save LSB of exp
	bset	&31,%d1				#set hidden bit
	add.b	%d0,%d0				#get LSB of exp
	addx.b	%d1,%d1				#d1.b becomes exp
	moveq	&-98,%d0			#for calculating shift
	sub.b	%d1,%d0				#d0.b = shift
	lsr.l	%d0,%d1				#d1 <- TRUNC(Y/PI)
	negx.l	%d1				#round if needed
	neg.l	%d1
	moveq	&1,%d0				#mask
	and.b	%d1,%d0				#check LSB
	beq.w	hsin1				#jump if even
	not.b	%d7				#sgn <- -sgn
hsin1:	move.l	%d1,(movil_m_f2-bogus4,%a0)	#f2,f3 <- XN
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x400921FB,(movf_m_f5-bogus4,%a0) #f4,f5 <- PI
	move.l	&0x54442D18,(movf_m_f4-bogus4,%a0)
	tst.w	(mull_f4_f2-bogus4,%a0)		#f2,f3 <- XN * PI
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(subl_f2_f0-bogus4,%a0)		#f0,f1 <- Y - XN * PI
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(movlf_f0_f0-bogus4,%a0)		#f0 <- f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	(movf_f0_m-bogus4,%a0),%d0		#d0 <- f
	add.l	%d0,%d0				#abs(f)
	cmp.l	%d0,&0x73000000			#> eps ?
	bcs.w	hsin3
#
# The argument has been reduced if necessary and is in the interval
# (1/(2**12),PI/2).
#
hsin2:	tst.w	(movf_f0_f1-bogus4,%a0)		#f1 <- f
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f1-bogus4,%a0)		#f1 <- g=f*f
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x362E9C5B,(movf_m_f2-bogus4,%a0) #f2 <- r4
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- r4*g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0xB94FB222,(movf_m_f3-bogus4,%a0) #f3 <- r3
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- r4*g+r3
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- (r4*g+r3)*g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0x3C08873E,(movf_m_f3-bogus4,%a0) #f3 <- r2
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- (r4*g+r3)*g+r2
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- ((r4*g+r3)*g+r2)*g
	movem.l	(%a0),%d1/%a1			#wait
	move.l	&0xBE2AAAA4,(movf_m_f3-bogus4,%a0) #f3 <- r1
	tst.w	(addf_f3_f2-bogus4,%a0)		#f2 <- ((r4*g+r3)*g+r2)*g+r1
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f1_f2-bogus4,%a0)		#f2 <- R(g)
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(mulf_f0_f2-bogus4,%a0)		#f2 <- f * R(g)
	movem.l	(%a0),%d1/%a1			#wait
	tst.w	(addf_f2_f0-bogus4,%a0)		#f0 <- f + f * R(g)
	movem.l	(%a0),%d1/%a1			#wait
hsin3:	move.l	(movf_f0_m-bogus4,%a0),%d0		#get f = result
	tst.b	%d7				#change sign?
	bpl.w	hsin4
	bchg	&31,%d0				#yes
hsin4:	move.b	(%sp)+,%d7			#restore d7
	unlk	%a6
	rts

#**********************************************************
#							  *
# SINGLE PRECISION SINE					  *
#							  *
# float fsin(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and underflow are ignored.	  *
#							  *
# Author: Mark McDowell  6/21/85			  *
#							  *
#**********************************************************

	text
c_fsin:
	lea	(4,%sp),%a0
	fmov	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov	%d0,%fpcr
	fmov.s	(%a0),%fp0		#fp0 <- arg
	fabs	%fp0,%fp1
	fcmp.s	%fp1,&0f1.349304E10	#total loss of precision?
	fbgt	c_sin2
	fsin	%fp0			#calculate sin
	fmov.s	%fp0,%d0		#save result
	fmov	%d1,%fpcr		#restore control register
	rts
c_sin2:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	subq.w	&8,%sp
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_sin_name		#name = "fsin"
	movq	&TLOSS,%d0
	mov.l	%d0,-(%sp)		#type = TLOSS
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_sin3
	movq	&ERANGE,%d0
	mov.l	%d0,_errno		#errno = ERANGE
	pea	18.w
	pea	c_sin_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#write(2,"fsin: TLOSS error\n",18)
	jsr	_write			#write(2,"fsin: TLOSS error\n",18)
 ')
	lea	(12,%sp),%sp
c_sin3:	
	movq	&0,%d0
	fmov	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov.l	(%sp)+,%fpcr
	rts

#**********************************************************
#							  *
# SINGLE PRECISION COSINE				  *
#							  *
# float fcos(arg) float arg;				  *
#							  *
# Errors: If |arg| > PI*(2^32), matherr() is called	  *
#		with type = TLOSS and retval = 0.0.	  *
#         Inexact errors and quiet NaN's as inputs are    *
#		ignored.				  *
#							  *
# Author: Mark McDowell  6/21/85			  *
#							  *
#**********************************************************

c_fcos:
	lea	(4,%sp),%a0
	fmov	%fpcr,%d1		#save control register
	movq	&0,%d0
	fmov	%d0,%fpcr
	fmov.s	(%a0),%fp0		#fp0 <- arg
	fabs	%fp0,%fp1
	fcmp.s	%fp1,&0f1.349304E10	#total loss of precision?
	fbgt	c_cos2
	fcos	%fp0			#calculate cos
	fmov.s	%fp0,%d0		#save result
	fmov	%d1,%fpcr		#restore control register
	rts
c_cos2:
	mov.l	%d1,-(%sp)		#save control register
	movq	&0,%d0
	mov.l	%d0,-(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	subq.w	&8,%sp
	fmov.d	%fp0,-(%sp)		#arg1 = arg
	pea	c_cos_name
	movq	&TLOSS,%d0
	mov.l	%d0,-(%sp)		#type = TLOSS
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_cos3
	movq	&ERANGE,%d0
	mov.l	%d0,_errno		#errno = ERANGE
	pea	18.w
	pea	c_cos_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`		#write(2,"fcos: TLOSS error\n",18)
	jsr	_write			#write(2,"fcos: TLOSS error\n",18)
')
	lea	(12,%sp),%sp
c_cos3:	
	movq	&0,%d0
	fmov	%d0,%fpcr
	fmov.d	(%sp)+,%fp0		#get retval
	fmov.s	%fp0,%d0		#save result
	fmov	(%sp)+,%fpcr		#restore control register
	rts

	data
c_sin_name:
	byte	102,115,105,110,0		#"fsin"
c_sin_msg:
	byte	102,115,105,110,58,32,84,76,79	#"fsin: TLOSS error\n"
	byte	83,83,32,101,114,114,111,114
	byte	10,0
c_cos_name:
	byte	102,99,111,115,0		#"fcos"
c_cos_msg:
	byte	102,99,111,115,58,32,84,76,79	#"fcos: TLOSS error\n"
	byte	83,83,32,101,114,114,111,114
	byte	10,0

ifdef(`PROFILE',`
	data
p_fsin:	long	0
p_fcos:	long	0
	')
