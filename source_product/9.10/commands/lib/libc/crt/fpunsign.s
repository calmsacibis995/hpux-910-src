
# HPUX_ID: @(#) $Revision: 66.3 $
# @(#) $Revision: 66.3 $       
#
	global	___utofl,___fltou,___floatu,___fixu,lntrelu,Atruncu
	sglobal	_utofl,_fltou,_floatu,_fixu

         	set	minuszero,0x80000000	#IEEE -0
       		set	SIGFPE,8		#Floating point exception

#*************************************************************************
#                                                                        *
# _utofl - convert unsigned 32 bit integer to 32 bit floating point      *
#                                                                        *
# call: integer is passed on the stack                                   *
# return: 32 bit floating point result is returned in d0                 *
#                                                                        *
#*************************************************************************
___utofl:
_utofl:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_utofl(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_utofl,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-12
	movem.l	%d2-%d4,-(%sp)	#save registers
	move.l	(8,%a6),%d0	#get operand
	beq.b	sfloat7		#no work if zero
	move.l	%d0,%d1		#d1 is working register
#	bpl.b	sfloat1		#get absolute value
#	neg.l	%d1

sfloat1:	moveq	&31,%d3		#need to find msb
	move.l	%d1,%d2		#search by byte
	swap	%d2		#isolate most significant word
	tst.w	%d2		#in most significant bytes?
	beq.b	sfloat2		#check lower bytes if not
	clr.b	%d2		#check most significant byte
	tst.w	%d2		#is it here?
	bne.b	sfloat3		#branch if it is
	moveq	&23,%d3		#must be in second most significant
	bra.b	sfloat3
sfloat2:	moveq	&15,%d3		#msb is in lower word
	move.w	%d1,%d2		#isolate lower word
	clr.b	%d2		#isolate upper byte
	tst.w	%d2		#is it here?
	bne.b	sfloat3		#branch if it is
	moveq	&7,%d3		#must be in lower byte

sfloat3:	btst	%d3,%d1		#now scan bits
	dbne	%d3,sfloat3	#until found

	moveq	&23,%d2		#calculate shift
	sub.w	%d3,%d2		#number of left shifts

	bpl.b	sfloat5		#left or right?

	neg.w	%d2		#number of right shifts
	moveq	&-1,%d4		#mask for sticky bit
	lsl.w	%d2,%d4		#bits to mask
	not.w	%d4
	lsr.w	&1,%d4		#don't save round bit
	and.w	%d1,%d4		#sticky bit
	lsr.l	%d2,%d1		#shift it
	bcc.b	sfloat6		#round bit set?
	addq.l	&1,%d1		#yes: round up
	btst	&24,%d1		#overflow?
	beq.b	sfloat4		#branch if not
	lsr.l	&1,%d1		#adjust mantissa
	addq.w	&1,%d3		#adjust msb number
	bra.b	sfloat6
sfloat4:	tst.w	%d4		#check sticky bit
	bne.b	sfloat6		#should i have rounded?
	bclr	&0,%d1		#maybe not
	bra.b	sfloat6

sfloat5:	beq.b	sfloat6		#shift at all?
	lsl.l	%d2,%d1		#shift it left

sfloat6:	add.w	&0x7e,%d3	#add exponent bias
	add.l	%d0,%d0		#shift off sign
	lsr.b	&1,%d3		#put in sign -- always positive
	roxl.w	&8,%d3
	swap	%d3		#move into place
	add.l	%d1,%d3		#include the mantissa

	move.l	%d3,%d0		#save the result
sfloat7:	movem.l	(%sp)+,%d2-%d4	#restore registers
	unlk    %a6
	rts

#***************************************************************************
#                                                                          *
#        _fltou                                                            *
#                                                                          *
#        Convert a 32 bit floating point real to a 32 bit integer with     *
#        truncation                                                        *
#                                                                          *
#        input: 32 bit floating point number is passed on stack            *
#                                                                          *
#        output: 32 bit integer is returned in d0                          *
#									   *
#               Paul Beiser 01/11/83 - Modified to handle -0 as input	   *
#									   *
#***************************************************************************

___fltou:
_fltou:	
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fltou(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fltou,%a0
	jsr	mcount
')
	')
	link    %a6,&0
#	trap	#2		test for stack space
#	dc.w	-4
	move.l	%d2,-(%sp)	#save register
	move.l	(8,%a6),%d0	#argument
	beq.b	trunc5		#is it zero?
	cmp.l   %d0,&minuszero   #is it -0?
	bne.b   trunc0
           moveq   &0,%d0           #-0 returns integer value of 0
	   bra.w     trunc5
trunc0:	move.l	%d0,%d1		#save exponent
	move.l	%d0,%d2		#save sign
	and.l	&0x7fffff,%d0	#mantissa
	bset	&23,%d0		#hidden bit
	swap	%d1		#isolate exponent
	add.w	%d1,%d1		#remove sign bit
	lsr.w	&8,%d1		#d1.w <- exponent
	sub.w	&0x7f+23,%d1	#amount to shift left
	beq.b	trunc4		#branch if no shift
	cmp.w	%d1,&-24		#trunc to zero?
	bgt.b	trunc1
	   moveq   &0,%d0	   #result is zero
	   bra.b   trunc5	   #return
trunc1:	tst.l	%d2		#negative?
	bmi.w	trunc1a		#straight-line is positive path
	cmp.w	%d1,&9		#possible overflow?
	blt.b	trunc2
	bra.w	intgrover
trunc1a:			#negative path (up to "trunc2")
	cmp.w	%d1,&8		#possible overflow?
	bgt.w  	intgrover
	blt.b	trunc2
	cmp.l	%d0,&0x800000	#is it -2^31?
	bne.w     intgrover
trunc2:	tst.w	%d1		#left or right shift
	bgt.b	trunc3
	neg.w	%d1
	lsr.l	%d1,%d0		#right shift
	bra.b	trunc4		#done
trunc3:	lsl.l	%d1,%d0		#left shift
trunc4:	tst.l	%d2		#set sign
	bpl.b	trunc5
	neg.l	%d0		#make it negative
trunc5:	move.l	(%sp)+,%d2	#restore register
	unlk    %a6
	rts			#return
#
# Error handlers.
#
intgrover: trap	&SIGFPE         #overflow
	move.l	(%sp)+,%d2	#restore register
	unlk    %a6
	rts

___floatu:
_floatu:  
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_floatu(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_floatu,%a0
	jsr	mcount
')
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-20
	movem.l	%d4-%d7,-(%sp)
	move.l	(8,%a6),%d0
ifdef(`PIC',`
	bsr.l	lntrelu',`
	jsr	lntrelu
')
	movem.l	(%sp)+,%d4-%d7
	unlk	%a6
	rts

___fixu:
_fixu:    
	ifdef(`PROFILE',`
ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_fixu(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_fixu,%a0
	jsr	mcount
')
	')
	link	%a6,&0
#	trap	#2
#	dc.w	-20
	movem.l	%d4-%d7,-(%sp)
	move.l	(8,%a6),%d0
	move.l	(12,%a6),%d1
ifdef(`PIC',`
	bsr.l	Atruncu',`
	jsr	Atruncu
')
	movem.l	(%sp)+,%d4-%d7
	unlk	%a6
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
lntrelu:	moveq	&0,%d7		#will hold sign of result and exponent
	moveq	&0,%d1		#bottom part of mantissa
	tst.l	%d0		#check if non-zero
	bne.w	ifposit		#branch if non-zero -- must be positive
	moveq	&0,%d0		#else returna zero result
	move.l	%d0,%d1
	rts			#and return
#nonzero:	bpl.w	ifposit		#branch if positive 
#	neg.l	%d0		#else convert to positive
#	bvs.w	maxlnt		#branch if had -2^31
#	move.w	&0x8000,%d7		#else set sign bit in result
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
	tst.l	%d0
	blt.w	loop_7		#avoid first test if high-order bit set
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
Atruncu:  move.w	%d0,%d1		#shift everthing to the right by 16
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
	tst.w	%d7		#check sign
	bpl.w	plus		#branch if > 0
	movq	&32,%d5		#allow 31 bits positive
	bra.w	comp
plus:
	movq	&33,%d5		#allow 32 bits positive
comp:
	cmp.w	%d6,%d5
	bgt.w	intover		#too big if don't branch
	beq.w	silkcheck
	tst.w	%d6		#for small numbers
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

ifdef(`PROFILE',`
		data
p_utofl:	long	0
p_fltou:	long	0
p_floatu:	long	0
p_fixu:		long	0
	')
