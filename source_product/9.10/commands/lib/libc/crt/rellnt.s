# @(#) $Revision: 66.2 $     
#
	global	rellnt,lntrel,Atrunc
		
      		set	SIGFPE,8

#******************************************************************************
#
#       Procedure rellnt (round)
#
#       Description:
#               Convert a real into a 32 bit integer, with
#               rounding to the nearest.
#
#       Parameters:
#               (d0,d1) - real number to be converted.
#
#       The result is returned in d0.
#
#       Register usage:
#               (d0,d1) - number to be converted
#               d6-d7   - scratch
#
rellnt:	move.w	%d0,%d1		#shift everthing to the right by 16
	swap	%d1		#d1 is correct
	clr.w	%d0
	swap	%d0		#d0 is correct
	move.w	%d0,%d7		#save the sign of the number
	move.w	%d0,%d6
	and.w	&0x7ff0,%d6		#mask out the sign
	lsr.w	&4,%d6
	sub.w	&1022,%d6		#exponent 1 bigger because of leading one
#
#  Check for boundary conditions.
#  
	cmp.w	%d6,&32
	bgt.w	intover
	beq.w	check32		#-2,147,483,648.5 = (c1e00000,00100000)
	tst.w	%d6
	bge.w	in32con		#continue with conversion
	   moveq	&0,%d0	   #else return a zero
	   rts
#
#  Finish the conversion.
#
in32con:	and.w	&0x000f,%d0		#d0 has top 4 bits
	lsr.l	&5,%d1		#place top bits (except hidden one) in d1
	ror.l	&5,%d0
	or.l	%d0,%d1		#correct except for the hidden bit
	neg.w	%d6
	add.w	&32,%d6		#1 <= shifts <= 32
	bset	&31,%d1		#place in hidden bit
	lsr.l	%d6,%d1
	bcc.w	chksign		#branch if rounded correctly
	addq.l	&1,%d1		#round to the nearest
	bpl.w	chksign		#no overflow
	tst.w	%d7		#overflow - check for negative result
	bpl.w	intover		#error if positive 2^31
chksign:	tst.w	%d7		#check the sign
	bpl.w	done3
	neg.l	%d1		#else convert to negative
done3:	move.l	%d1,%d0		#place result in correct register
	rts
#
#  Boundary condition checks.
#
check32:	tst.w	%d0		#check sign first
	bpl.w	intover		#remember, shifted right by 16
	and.w	&0x000f,%d0	#mantissa of 2^31-.5 = ([1]00000 00100000)
	bne.w	intover		#definitely way too large
	lsr.l	&5,%d1		#else shift till get lsb
	bne.w	intover		#if non-zero, less than -2^31 - 0.5
	bcs.w	intover		#branch if equal to -2^31 - 0.5
	move.l	&0x80000000,%d0		#else return -2^31
	rts
#
#  Error handler.
# 
intover:	trap    &SIGFPE	
        rts

#******************************************************************************
#
#       Procedure lntrel
#
#       Description:
#               Convert a 32 bit integer into a real number.
#
#       Parameters:
#               d0      - 32 bit integer to be converted
#
#       The result is returned in (d0,d1).
#
#       Register usage:
#               d0      - number to be converted
#               (d0,d1) - result
#               d4-d7   - scratch
#
#
maxlnt:	move.l	&0xc1e00000,%d0		#return -2^31
	moveq	&0,%d1
	rts
#
#  Main body of lntrel.
#
lntrel:	moveq	&0,%d7		#will hold sign of result and exponent
	moveq	&0,%d1		#bottom part of mantissa
	tst.l	%d0		#check if non-zero
	bne.w	nonzero		#branch if non-zero
	moveq	&0,%d0		#else returna zero result
	move.l	%d0,%d1
	rts			#and return
nonzero:	bpl.w	ifposit		#branch if positive 
	neg.l	%d0		#else convert to positive
	bvs.w	maxlnt		#branch if had -2^31
	move.w	&0x8000,%d7		#else set sign bit in result
#
#  Determine if a 16 bit integer hiding in 32 bits.
#
ifposit:	swap	%d0		#check for a 16 bit integer
	tst.w	%d0
	beq.w	int16		#branch if a 16 bit integer
	move.w	&1023+20,%d4		#place in the bias
	move.w	%d0,%d5		#test if have to left shift
	and.w	&0xfff0,%d5
	bne.w	highpart		#branch if first one in top of word
	move.l	&0x00100000,%d6		#mask for the test for normalization
	swap	%d0		#else restore number
loop4:	add.l	%d0,%d0		#at least 1 and most 4 shifts
	subq.w	&1,%d4
	cmp.l	%d0,%d6
	blt.w	loop4		#until normalized
	bra.w	shdone
highpart:	move.w	%d0,%d5		#see if at least 8 right shifts
	and.w	&0x0ff0,%d5
	bne.w	finrit		#if non-zero, then at most 7 more shifts
	swap	%d0		#restore mantissa
	addq.l	&8,%d4		#adjust exponent
	move.b	%d0,%d1
	ror.l	&8,%d1		#d1 is correct
	lsr.l	&8,%d0		#d0 is correct
	bra.w	insmask
finrit:	swap	%d0		#restore mantissa
insmask:	move.l	&0x00200000,%d6		#mask for the test for normalization
	cmp.l	%d0,%d6
	blt.w	shdone		#if <, d0 correctly lined up
loop_7:	lsr.l	&1,%d0
	roxr.l	&1,%d1
	addq.l	&1,%d4
	cmp.l	%d0,%d6		#continue until normalized
	bge.w	loop_7
	bra.w	shdone
#
#  Have a 16 bit integer to convert, so do it fast.
#
int16:	swap	%d0		#restore the integer 
	move.w	&1023+15,%d4		#place in the bias
	move.l	&0x00100000,%d6		#mask for the test for normalization
	lsl.l	&5,%d0		#shift by at least 5
	cmp.l	%d0,%d6		#see if done
	bge.w	shdone
#
#   At most 15 shifts left.
#
	move.l	%d0,%d5		#check for shift by 8
	and.l	&0x001fe000,%d5
	bne.w	chk7		#branch if 7 or less shifts left
	lsl.l	&8,%d0		#else shift by 8
	subq.w	&8,%d4		#adjust exponent, and finish the shift
chk7:	cmp.l	%d0,%d6		#check implied one
	bge.w	shdone
lp_7:	add.l	%d0,%d0		#else shift left
	subq.w	&1,%d4
	cmp.l	%d0,%d6
	blt.w	lp_7		#continue until normalized
#
#  Splice result together.
#
shdone:	subq.w	&1,%d4		#hidden bit will add back
	lsl.w	&4,%d4		#place in correct locations
	or.w	%d4,%d7		#place exponent in with sign
	swap	%d7		#in correct order
	add.l	%d7,%d0		#add in exponent and sign
	rts

#******************************************************************************
#
#       Procedure @trunc
#
#       Description:
#               Convert a floating point real into a 32 bit integer,
#               with truncation.
#
#       Registers:
#               (d0,d1) - floating point number to be converted
#               a0      - return address
#
#       Error conditions:
#               If the numeric item is too large in magnitude, a
#               trap is executed.
#
Atrunc:  move.w	%d0,%d1		#shift everthing to the right by 16
	swap	%d1		#d1 is correct
	clr.w	%d0
	swap	%d0		#d0 is correct
	move.w	%d0,%d7		#save the sign of the number
	move.w	%d0,%d6
	and.w	&0x7ff0,%d6		#mask out the sign
	lsr.w	&4,%d6
	sub.w	&1022,%d6		#exponent 1 bigger because of leading one
#
#  Check for boundary conditions.
#
	cmp.w	%d6,&32
	bgt.w	intover		#too big if don't branch
	beq.w	silkcheck
skip:	tst.w	%d6		#for small numbers
	bgt.w	in32cont	#branch if will convert
	   clr.l    %d0		   #else return zero
	   rts
#
#  Place top bits (except for hidden bit) all in d1.
#
in32cont: and.w	&0x000f,%d0	#d0 has top 4 bits
	lsr.l	&5,%d1
	ror.l	&5,%d0
	or.l	%d0,%d1		#correct except for the hidden bit
#
#  Finish the conversion.
#
	neg.w	%d6
	add.w	&32,%d6		#1 <= shifts <= 31
	bset	&31,%d1		#place in hidden bit
	lsr.l	%d6,%d1
	tst.w	%d7		#check the sign
	bpl.w	done32
	   neg.l	%d1	   #else convert to negative
done32:	move.l	%d1,%d0		#place result in d0
	rts
#
#  Boundary condition checks.
#
silkcheck: tst.w	%d0		#check the sign first
	bpl.w	intover
	and.w	&0x000f,%d0
	bne.w	intover		#if ms bite non-zero, way too large
	lsr.l	&5,%d1		#shift fractional portion out
	bne.w	intover
	move.l	&0x80000000,%d0
	rts
