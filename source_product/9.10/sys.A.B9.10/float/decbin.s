ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/decbin.s,v $
# $Revision: 1.3.84.3 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 20:47:29 $
                                    
#
#	decbin.sa 3.1 12/10/90
#
#	Description: Converts normalized packed bcd value pointed to by
#	register A6 to extended-precision value in FP0.
#
#	Input: Normalized packed bcd value in ETEMP(a6).
#
#	Output:	Exact floating-point representation of the packed bcd value.
#
#	Saves and Modifies: D2-D5
#
#	Speed: The program decbin takes ??? cycles to execute.
#
#	Object Size:
#
#	External Reference(s): None.
#
#	Algorithm:
#	Expected is a normal bcd (i.e. non-exceptional; all inf, zero,
#	and NaN operands are dispatched without entering this routine)
#	value in 68881/882 format at location ETEMP(A6).
#
#	A1.	Convert the bcd exponent to binary by successive adds and muls.
#	Set the sign according to SE. Subtract 16 to compensate
#	for the mantissa which is to be interpreted as 17 integer
#	digits, rather than 1 integer and 16 fraction digits.
#	Note: this operation can never overflow.
#
#	A2. Convert the bcd mantissa to binary by successive
#	adds and muls in FP0. Set the sign according to SM.
#	The mantissa digits will be converted with the decimal point
#	assumed following the least-significant digit.
#	Note: this operation can never overflow.
#
#	A3. Count the number of leading/trailing zeros in the
#	bcd string.  If SE is positive, count the leading zeros;
#	if negative, count the trailing zeros.  Set the adjusted
#	exponent equal to the exponent from A1 and the zero count
#	added if SM = 1 and subtracted if SM = 0.  Scale the
#	mantissa the equivalent of forcing in the bcd value:
#
#	SM = 0	a non-zero digit in the integer position
#	SM = 1	a non-zero digit in Mant0, lsd of the fraction
#
#	this will insure that any value, regardless of its
#	representation (ex. 0.1E2, 1E1, 10E0, 100E-1), is converted
#	consistently.
#
#	A4. Calculate the factor 10^exp in FP1 using a table of
#	10^(2^n) values.  To reduce the error in forming factors
#	greater than 10^27, a directed rounding scheme is used with
#	tables rounded to RN, RM, and RP, according to the table
#	in the comments of the pwrten section.
#
#	A5. Form the final binary number by scaling the mantissa by
#	the exponent factor.  This is done by multiplying the
#	mantissa in FP0 by the factor in FP1 if the adjusted
#	exponent sign is positive, and dividing FP0 by FP1 if
#	it is negative.
#
#	Clean up and return.  Check if the final mul or div resulted
#	in an inex2 exception.  If so, set inex1 in the fpsr and 
#	check if the inex1 exception is enabled.  If so, set d7 upper
#	word to $0100.  This will signal unimp.sa that an enabled inex1
#	exception occured.  Unimp will fix the stack.
#	
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
#
#	PTENRN, PTENRM, and PTENRP are arrays of powers of 10 rounded
#	to nearest, minus, and plus, respectively.  The tables include
#	10**{1,2,4,8,16,32,64,128,256,512,1024,2048,4096}.  No rounding
#	is required until the power is greater than 27, however, all
#	tables include the first 5 for ease of indexing.
#
                                    
                                    
                                    
                                    
rtable:  byte      0,0,0,0          
         byte      2,3,2,3          
         byte      2,3,3,2          
         byte      3,2,2,3          
                                    
         global    decbin           
         global    calc_e           
         global    pwrten           
         global    calc_m           
         global    norm             
         global    ap_st_z          
         global    ap_st_n          
#
         set       fnibs,7          
         set       fstrt,0          
#
         set       estrt,4          
         set       edigits,2        
#
# Constants in single precision
fzero:   long      0x00000000       
fone:    long      0x3f800000       
ften:    long      0x41200000       
                                    
         set       ten,10           
                                    
#
decbin:                             
         fmove.l   &0,%fpcr         # clr real fpcr
         movem.l   %d2-%d5,-(%a7)   
#
# Calculate exponent:
#  1. Copy bcd value in memory for use as a working copy.
#  2. Calculate absolute value of exponent in d1 by mul and add.
#  3. Correct for exponent sign.
#  4. Subtract 16 to compensate for interpreting the mant as all integer digits.
#     (i.e., all digits assumed left of the decimal point.)
#
# Register usage:
#
#  calc_e:
#	(*)  d0: temp digit storage
#	(*)  d1: accumulator for binary exponent
#	(*)  d2: digit count
#	(*)  d3: offset pointer
#	( )  d4: first word of bcd
#	( )  a0: pointer to working bcd value
#	( )  a6: pointer to original bcd value
#	(*)  FP_SCR1: working copy of original bcd value
#	(*)  L_SCR1: copy of original exponent word
#
calc_e:                             
         move.l    &edigits,%d2     # # of nibbles (digits) in fraction part
         moveq.l   &estrt,%d3       # counter to pick up digits
         lea.l     fp_scr1(%a6),%a0 # load tmp bcd storage address
         move.l    etemp(%a6),(%a0) # save input bcd value
         move.l    etemp_hi(%a6),4(%a0) # save words 2 and 3
         move.l    etemp_lo(%a6),8(%a0) # and work with these
         move.l    (%a0),%d4        # get first word of bcd
         clr.l     %d1              # zero d1 for accumulator
e_gd:                               
         mulu.l    &ten,%d1         # mul partial product by one digit place
         bfextu    %d4{%d3:&4},%d0  # get the digit and zero extend into d0
         add.l     %d0,%d1          # d1 = d1 + d0
         addq.b    &4,%d3           # advance d3 to the next digit
         dbf.w     %d2,e_gd         # if we have used all 3 digits, exit loop
         btst      &30,%d4          # get SE
         beq.b     e_pos            # don't negate if pos
         neg.l     %d1              # negate before subtracting
e_pos:                              
         sub.l     &16,%d1          # sub to compensate for shift of mant
         bge.b     e_save           # if still pos, do not neg
         neg.l     %d1              # now negative, make pos and set SE
         or.l      &0x40000000,%d4  # set SE in d4,
         or.l      &0x40000000,(%a0) # and in working bcd
e_save:                             
         move.l    %d1,l_scr1(%a6)  # save exp in memory
#
#
# Calculate mantissa:
#  1. Calculate absolute value of mantissa in fp0 by mul and add.
#  2. Correct for mantissa sign.
#     (i.e., all digits assumed left of the decimal point.)
#
# Register usage:
#
#  calc_m:
#	(*)  d0: temp digit storage
#	(*)  d1: lword counter
#	(*)  d2: digit count
#	(*)  d3: offset pointer
#	( )  d4: words 2 and 3 of bcd
#	( )  a0: pointer to working bcd value
#	( )  a6: pointer to original bcd value
#	(*) fp0: mantissa accumulator
#	( )  FP_SCR1: working copy of original bcd value
#	( )  L_SCR1: copy of original exponent word
#
calc_m:                             
         moveq.l   &1,%d1           # word counter, init to 1
         fmove.s   fzero,%fp0       # accumulator
#
#
#  Since the packed number has a long word between the first & second parts,
#  get the integer digit then skip down & get the rest of the
#  mantissa.  We will unroll the loop once.
#
         bfextu    (%a0){&28:&4},%d0 # integer part is ls digit in long word
         fadd.b    %d0,%fp0         # add digit to sum in fp0
#
#
#  Get the rest of the mantissa.
#
loadlw:                             
         move.l    (%a0,%d1.l*4),%d4 # load mantissa lonqword into d4
         moveq.l   &fstrt,%d3       # counter to pick up digits
         moveq.l   &fnibs,%d2       # reset number of digits per a0 ptr
md2b:                               
         fmul.s    ften,%fp0        # fp0 = fp0 * 10
         bfextu    %d4{%d3:&4},%d0  # get the digit and zero extend
         fadd.b    %d0,%fp0         # fp0 = fp0 + digit
#
#
#  If all the digits (8) in that long word have been converted (d2=0),
#  then inc d1 (=2) to point to the next long word and reset d3 to 0
#  to initialize the digit offset, and set d2 to 7 for the digit count;
#  else continue with this long word.
#
         addq.b    &4,%d3           # advance d3 to the next digit
         dbf.w     %d2,md2b         # check for last digit in this lw
nextlw:                             
         addq.l    &1,%d1           # inc lw pointer in mantissa
         cmp.l     %d1,&2           # test for last lw
         ble       loadlw           # if not, get last one
                                    
#
#  Check the sign of the mant and make the value in fp0 the same sign.
#
m_sign:                             
         btst      &31,(%a0)        # test sign of the mantissa
         beq.b     ap_st_z          # if clear, go to append/strip zeros
         fneg.x    %fp0             # if set, negate fp0
                                    
#
# Append/strip zeros:
#
#  For adjusted exponents which have an absolute value greater than 27*,
#  this routine calculates the amount needed to normalize the mantissa
#  for the adjusted exponent.  That number is subtracted from the exp
#  if the exp was positive, and added if it was negative.  The purpose
#  of this is to reduce the value of the exponent and the possibility
#  of error in calculation of pwrten.
#
#  1. Branch on the sign of the adjusted exponent.
#  2p.(positive exp)
#   2. Check M16 and the digits in lwords 2 and 3 in decending order.
#   3. Add one for each zero encountered until a non-zero digit.
#   4. Subtract the count from the exp.
#   5. Check if the exp has crossed zero in #3 above; make the exp abs
#	   and set SE.
#	6. Multiply the mantissa by 10**count.
#  2n.(negative exp)
#   2. Check the digits in lwords 3 and 2 in decending order.
#   3. Add one for each zero encountered until a non-zero digit.
#   4. Add the count to the exp.
#   5. Check if the exp has crossed zero in #3 above; clear SE.
#   6. Divide the mantissa by 10**count.
#
#  *Why 27?  If the adjusted exponent is within -28 < expA < 28, than
#   any adjustment due to append/strip zeros will drive the resultane
#   exponent towards zero.  Since all pwrten constants with a power
#   of 27 or less are exact, there is no need to use this routine to
#   attempt to lessen the resultant exponent.
#
# Register usage:
#
#  ap_st_z:
#	(*)  d0: temp digit storage
#	(*)  d1: zero count
#	(*)  d2: digit count
#	(*)  d3: offset pointer
#	( )  d4: first word of bcd
#	(*)  d5: lword counter
#	( )  a0: pointer to working bcd value
#	( )  FP_SCR1: working copy of original bcd value
#	( )  L_SCR1: copy of original exponent word
#
#
# First check the absolute value of the exponent to see if this
# routine is necessary.  If so, then check the sign of the exponent
# and do append (+) or strip (-) zeros accordingly.
# This section handles a positive adjusted exponent.
#
ap_st_z:                            
         move.l    l_scr1(%a6),%d1  # load expA for range test
         cmp.l     %d1,&27          # test is with 27
         ble.w     pwrten           # if abs(expA) <28, skip ap/st zeros
         btst      &30,(%a0)        # check sign of exp
         bne.b     ap_st_n          # if neg, go to neg side
         clr.l     %d1              # zero count reg
         move.l    (%a0),%d4        # load lword 1 to d4
         bfextu    %d4{&28:&4},%d0  # get M16 in d0
         bne.b     ap_p_fx          # if M16 is non-zero, go fix exp
         addq.l    &1,%d1           # inc zero count
         moveq.l   &1,%d5           # init lword counter
         move.l    (%a0,%d5.l*4),%d4 # get lword 2 to d4
         bne.b     ap_p_cl          # if lw 2 is zero, skip it
         addq.l    &8,%d1           # and inc count by 8
         addq.l    &1,%d5           # inc lword counter
         move.l    (%a0,%d5.l*4),%d4 # get lword 3 to d4
ap_p_cl:                            
         clr.l     %d3              # init offset reg
         moveq.l   &7,%d2           # init digit counter
ap_p_gd:                            
         bfextu    %d4{%d3:&4},%d0  # get digit
         bne.b     ap_p_fx          # if non-zero, go to fix exp
         addq.l    &4,%d3           # point to next digit
         addq.l    &1,%d1           # inc digit counter
         dbf.w     %d2,ap_p_gd      # get next digit
ap_p_fx:                            
         move.l    %d1,%d0          # copy counter to d2
         move.l    l_scr1(%a6),%d1  # get adjusted exp from memory
         sub.l     %d0,%d1          # subtract count from exp
         bge.b     ap_p_fm          # if still pos, go to pwrten
         neg.l     %d1              #  get abs
         move.l    (%a0),%d4        # load lword 1 to d4
         or.l      &0x40000000,%d4  #  and set SE in d4
         or.l      &0x40000000,(%a0) #  and in memory
#
# Calculate the mantissa multiplier to compensate for the striping of
# zeros from the mantissa.
#
ap_p_fm:                            
         move.l    &ptenrn,%a1      # get address of power-of-ten table
         clr.l     %d3              # init table index
         fmove.s   fone,%fp1        # init fp1 to 1
         moveq.l   &3,%d2           # init d2 to count bits in counter
ap_p_el:                            
         asr.l     &1,%d0           # shift lsb into carry
         bcc.b     ap_p_en          # if 1, mul fp1 by pwrten factor
         fmul.x    (%a1,%d3),%fp1   # mul by 10**(d3_bit_no)
ap_p_en:                            
         add.l     &12,%d3          # inc d3 to next rtable entry
         tst.l     %d0              # check if d0 is zero
         bne.b     ap_p_el          # if not, get next bit
         fmul.x    %fp1,%fp0        # mul mantissa by 10**(no_bits_shifted)
         bra.b     pwrten           # go calc pwrten
#
# This section handles a negative adjusted exponent.
#
ap_st_n:                            
         clr.l     %d1              # clr counter
         moveq.l   &2,%d5           # set up d5 to point to lword 3
         move.l    (%a0,%d5.l*4),%d4 # get lword 3
         bne.b     ap_n_cl          # if not zero, check digits
         sub.l     &1,%d5           # dec d5 to point to lword 2
         addq.l    &8,%d1           # inc counter by 8
         move.l    (%a0,%d5.l*4),%d4 # get lword 2
ap_n_cl:                            
         move.l    &28,%d3          # point to last digit
         moveq.l   &7,%d2           # init digit counter
ap_n_gd:                            
         bfextu    %d4{%d3:&4},%d0  # get digit
         bne.b     ap_n_fx          # if non-zero, go to exp fix
         subq.l    &4,%d3           # point to previous digit
         addq.l    &1,%d1           # inc digit counter
         dbf.w     %d2,ap_n_gd      # get next digit
ap_n_fx:                            
         move.l    %d1,%d0          # copy counter to d0
         move.l    l_scr1(%a6),%d1  # get adjusted exp from memory
         sub.l     %d0,%d1          # subtract count from exp
         bgt.b     ap_n_fm          # if still pos, go fix mantissa
         neg.l     %d1              # take abs of exp and clr SE
         move.l    (%a0),%d4        # load lword 1 to d4
         and.l     &0xbfffffff,%d4  #  and clr SE in d4
         and.l     &0xbfffffff,(%a0) #  and in memory
#
# Calculate the mantissa multiplier to compensate for the appending of
# zeros to the mantissa.
#
ap_n_fm:                            
         move.l    &ptenrn,%a1      # get address of power-of-ten table
         clr.l     %d3              # init table index
         fmove.s   fone,%fp1        # init fp1 to 1
         moveq.l   &3,%d2           # init d2 to count bits in counter
ap_n_el:                            
         asr.l     &1,%d0           # shift lsb into carry
         bcc.b     ap_n_en          # if 1, mul fp1 by pwrten factor
         fmul.x    (%a1,%d3),%fp1   # mul by 10**(d3_bit_no)
ap_n_en:                            
         add.l     &12,%d3          # inc d3 to next rtable entry
         tst.l     %d0              # check if d0 is zero
         bne.b     ap_n_el          # if not, get next bit
         fdiv.x    %fp1,%fp0        # div mantissa by 10**(no_bits_shifted)
#
#
# Calculate power-of-ten factor from adjusted and shifted exponent.
#
# Register usage:
#
#  pwrten:
#	(*)  d0: temp
#	( )  d1: exponent
#	(*)  d2: {FPCR[6:5],SM,SE} as index in RTABLE; temp
#	(*)  d3: FPCR work copy
#	( )  d4: first word of bcd
#	(*)  a1: RTABLE pointer
#  calc_p:
#	(*)  d0: temp
#	( )  d1: exponent
#	(*)  d3: PWRTxx table index
#	( )  a0: pointer to working copy of bcd
#	(*)  a1: PWRTxx pointer
#	(*) fp1: power-of-ten accumulator
#
# Pwrten calculates the exponent factor in the selected rounding mode
# according to the following table:
#	
#	Sign of Mant  Sign of Exp  Rounding Mode  PWRTEN Rounding Mode
#
#	ANY	  ANY	RN	RN
#
#	 +	   +	RP	RP
#	 -	   +	RP	RM
#	 +	   -	RP	RM
#	 -	   -	RP	RP
#
#	 +	   +	RM	RM
#	 -	   +	RM	RP
#	 +	   -	RM	RP
#	 -	   -	RM	RM
#
#	 +	   +	RZ	RM
#	 -	   +	RZ	RM
#	 +	   -	RZ	RP
#	 -	   -	RZ	RP
#
#
pwrten:                             
         move.l    user_fpcr(%a6),%d3 # get user's FPCR
         bfextu    %d3{&26:&2},%d2  # isolate rounding mode bits
         move.l    (%a0),%d4        # reload 1st bcd word to d4
         asl.l     &2,%d2           # format d2 to be
         bfextu    %d4{&0:&2},%d0   #  {FPCR[6],FPCR[5],SM,SE}
         add.l     %d0,%d2          # in d2 as index into RTABLE
         lea.l     rtable,%a1       # load rtable base
         move.b    (%a1,%d2),%d0    # load new rounding bits from table
         clr.l     %d3              # clear d3 to force no exc and extended
         bfins     %d0,%d3{&26:&2}  # stuff new rounding bits in FPCR
         fmove.l   %d3,%fpcr        # write new FPCR
         asr.l     &1,%d0           # write correct PTENxx table
         bcc.b     not_rp           # to a1
         lea.l     ptenrp,%a1       # it is RP
         bra.b     calc_p           # go to init section
not_rp:                             
         asr.l     &1,%d0           # keep checking
         bcc.b     not_rm           
         lea.l     ptenrm,%a1       # it is RM
         bra.b     calc_p           # go to init section
not_rm:                             
         lea.l     ptenrn,%a1       # it is RN
calc_p:                             
         move.l    %d1,%d0          # use d0
         bpl.b     no_neg           # if exp is negative,
         neg.l     %d0              # invert it
         or.l      &0x40000000,(%a0) # and set SE bit
no_neg:                             
         clr.l     %d3              # table index
         fmove.s   fone,%fp1        # init fp1 to 1
e_loop:                             
         asr.l     &1,%d0           # shift next bit into carry
         bcc.b     e_next           # if zero, skip the mul
         fmul.x    (%a1,%d3),%fp1   # mul by 10**(d3_bit_no)
e_next:                             
         add.l     &12,%d3          # inc d3 to next rtable entry
         tst.l     %d0              # check if d0 is zero
         bne.b     e_loop           # not zero, continue shifting
#
#
#  Check the sign of the adjusted exp and make the value in fp0 the
#  same sign. If the exp was pos then multiply fp1*fp0;
#  else divide fp0/fp1.
#
# Register Usage:
#  norm:
#	( )  a0: pointer to working bcd value
#	(*) fp0: mantissa accumulator
#	( ) fp1: scaling factor - 10**(abs(exp))
#
norm:                               
         btst      &30,(%a0)        # test the sign of the exponent
         beq.b     mul              # if clear, go to multiply
div:                                
         fdiv.x    %fp1,%fp0        # exp is negative, so divide mant by exp
         bra.b     end_dec          
mul:                                
         fmul.x    %fp1,%fp0        # exp is positive, so multiply by exp
#
#
# Clean up and return with result in fp0.
#
# If the final mul/div in decbin incurred an inex exception,
# it will be inex2, but will be reported as inex1 by get_op.
#
end_dec:                            
         fmove.l   %fpsr,%d0        # get status register	
         bclr.l    &inex2_bit+8,%d0 # test for inex2 and clear it
         fmove.l   %d0,%fpsr        # return status reg w/o inex2
         beq.b     no_exc           # skip this if no exc
         or.l      &inx1a_mask,user_fpsr(%a6) # set inex1/ainex
no_exc:                             
         movem.l   (%a7)+,%d2-%d5   
         rts                        
                                    
	version 3
