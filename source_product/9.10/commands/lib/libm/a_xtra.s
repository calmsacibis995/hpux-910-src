# @(#) $Revision: 70.1 $     
#
	global	adx,intxp,setxp,compare
	global   flpt_horner,flpt_hornera,soft_horner,soft_hornera
        
	set     bogus4,0x18            #offset to do 4 bogus word reads
	set     addl_f2_f0,0x4008
	set     movf_m_f0,0x44fc
	set     movf_m_f1,0x44f8
	set     movf_m_f2,0x44f4
	set     movf_m_f3,0x44f0
	set     movf_m_f5,0x44e8
	set     movl_f4_f0,0x4450
	set     mull_f4_f0,0x4050
	set     minuszero,0x80000000      #top 32 bits of the real value -0

#******************************************************************************
#
#       Procedure adx
#
#       Description:
#               Augment a real number's exponent. This procedure is
#               used only in the elementary function evaluations.
#
#       Revision: Paul Beiser 01/13/83 Check for -0
#
#       Parameters:
#               (d0,d1) - the real to be augmented.
#                d7     - amount to be augmented.
#
#       The result is returned in (d0,d1).
#
#       Error conditions:
#               All arguments are defined to be in range, so error 
#               conditions cannot arise.
#
#       External references:
#               There are none.
#
#       NOTE: A test for a real 0 is present because this function
#             is used to implement reduce(V) in the power function,
#             where 0 is a valid argument.
#             
adx:     cmp.l   %d0,&minuszero   #check for a -0
	beq.w     adxret          #return with a -0
	swap	%d0		#else get exp in the low word
	beq.w	adxret		#stpower could furnish 0 argument!
	move.w	%d0,%d6		#extract old exponent
	and.w	&0x800f,%d0	#first, remove old exponent in the result
	and.w	&0x7ff0,%d6
	asl.w	&4,%d7		#faster if don't have to shift back
	add.w	%d7,%d6		#new exponent computed
	and.w	&0x7ff0,%d6	#large exp and negative augment; negative sign
	or.w	%d6,%d0		#place in new exponent
	swap	%d0		#restore correct order
adxret:	rts

#******************************************************************************
#
#       Procedure intxp
#
#       Description:
#               Extract the exponent from a real number. The exponent
#               extracted assumes a mantissa value in the range [.5,1).
#               This procedure is used only in the elementary function 
#               evaluations.
#
#       Parameters:
#               (d0,d1) - real number
#
#       The result is returned in d7 and (d0,d1) is unchanged.
#
#       Error conditions:
#               All arguments are defined to be in range, so error 
#               conditions cannot arise.
#
#       External references:
#               There are none.
#
intxp:	move.l	%d0,%d7		#don't destroy the original number
	swap	%d7		#place exponent into low word
	and.w	&0x7ff0,%d7
	lsr.w	&4,%d7
	sub.w	&1022,%d7	#mantissa in range [0.5,1) (ignore hidden bit)
	rts

#******************************************************************************
#
#       Procedure setxp
#
#       Description:
#               Set the exponent of a real number. The mantissa is
#               assumed to be in the range [.5,1). This procedure is 
#               used only in the elementary function evaluations.
#
#       Parameters:
#               (d0,d1) - real number
#                d7     - unbiased value of the new exponent.
#
#       The result is returned in (d0,d1).
#
#       Error conditions:
#               All arguments are defined to be in range, so error 
#               conditions cannot arise.
#
#       External references:
#               There are none.
#
setxp:	swap	%d0
	and.w	&0x800f,%d0	#remove the exponent
	add.w	&1022,%d7	#hidden bit becomes part of exponent
	lsl.w	&4,%d7		#always positive after bias add, so do lsl
	or.w	%d7,%d0		#place in new exponent
	swap	%d0		#re-align
	rts

#******************************************************************************
#
#       Procedures : flpt_horner / flpt_hornera
#
#       Description: Evaluate a polynomial. "flpt_hornera" assumes that the
#                    leading coefficient is 1, and thus avoids an extra 
#                    multiply. These procedures are used only in the 
#                    elementary function evaluation.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For: Hardware floating point
#
#       Parameters : (a4,a5)    - real number to be evaluated (w)
#                    a0         - address of the floating point hardware
#                    a6         - address of the coefficients
#                    d0         - the degree of the polynomial
#
#       Registers  : f0-f5      - scratch floating point registers
#                    d4-d5      - results of the bogus reads
#                    
#       Result     : Returned in (f1,f0).
#
#       Error(s)   : All arguments are defined to be in a restricted range,
#                    so error conditions cannot arise.
#
#       Miscel     : The caller must save and restore the contents of f0-f5.
#                    (a4,a5) is left unchanged.
#
#******************************************************************************

flpt_horner: move.l (%a6)+,(movf_m_f1,%a0)  #first coefficient result in (f1,f0)
         move.l  (%a6)+,(movf_m_f0,%a0)
         movem.l %a4-%a5,(movf_m_f5,%a0)    #(f5,f4) <- w
fhorloop:    tst.w   (mull_f4_f0,%a0)         #w * previous result
            movem.l (bogus4,%a0),%d4-%d5       #bogus reads and get error flag
            move.l  (%a6)+,(movf_m_f3,%a0)    #get the next coefficient
            move.l  (%a6)+,(movf_m_f2,%a0)
            tst.w   (addl_f2_f0,%a0)         #add coefficient to previous result
            movem.l (bogus4,%a0),%d4-%d5       #bogus reads with no error flag
            subq.w  &1,%d0                  #see if done
            bne.w     fhorloop
fhordone: rts

flpt_hornera:  movem.l %a4-%a5,(movf_m_f5,%a0)    #(f5,f4) <- w
         tst.w   (movl_f4_f0,%a0)         #w is also first partial result
         movem.l (bogus4,%a0),%d4-%d5       #bogus reads with no error flag
fhorlopa: move.l  (%a6)+,(movf_m_f3,%a0)    #get the next coefficient
         move.l  (%a6)+,(movf_m_f2,%a0)
         tst.w   (addl_f2_f0,%a0)         #previous result + coefficient
         movem.l (bogus4,%a0),%d4-%d5       #bogus reads with no error flag
         subq.w  &1,%d0                  #see if done
         beq.w     fhordone
            tst.w  (mull_f4_f0,%a0)          #else result*w
            movem.l (bogus4,%a0),%d4-%d5       #bogus reads with no error flag
            bra.w     fhorlopa

#******************************************************************************
#
#       Procedures : soft_horner / soft_hornera
#
#       Description: Evaluate a polynomial. "soft_hornera" assumes that the
#                    leading coefficient is 1, and thus avoids an extra 
#                    multiply. These procedures are used only in the software 
#                    versions of the elementary function evaluations.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83 
#
#       Parameters : (a4,a5)    - real number to be evaluated
#                    a6         - address of the coefficients
#                    d0         - the degree of the polynomial
#
#       Registers  : d2,d3      - scratch
#                    
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : None; all arguments defined to be in a restricted range.
#
#       References : radd, rmul
#
#******************************************************************************

soft_horner:
#	trap	#2
#	dc.w	-6
	 move.w  %d0,-(%sp)    #save the degree of the polynomial
         move.l  (%a6)+,%d0       #initialize result to first coeff.
         move.l  (%a6)+,%d1
horloop:     move.l  %a4,%d2          #get w
            move.l  %a5,%d3   
            jsr    rmul           #previous result * w
            move.l  (%a6)+,%d2       #get next coefficient
            move.l  (%a6)+,%d3
            jsr    radd           #add to previous result
            subq.w  &1,(%sp)
            bne.w     horloop
hordone:  addq.l  &2,%sp          #remove the degree count
         rts
soft_hornera:
#	trap	#2
#	dc.w	-2
	 move.w  %d0,-(%sp)  #save the degree of the polynomial
         move.l  %a4,%d0          #initialize result to w
         move.l  %a5,%d1   
horloopa: move.l  (%a6)+,%d2       #get next coefficient; (d0,d1) ok
         move.l  (%a6)+,%d3
         jsr    radd           #do the addition; (d0,d1) has result
         subq.w  &1,(%sp)
         beq.w     hordone
            move.l  %a4,%d2          #get w; (d0,d1) correct
            move.l  %a5,%d3   
            jsr    rmul           #(d0,d1) has result
            bra.w     horloopa

#******************************************************************************
#
#       Procedure  : compare
#
#       Description: Compare operand 1 with operand 2. Both operands are
#                    64 bit floating point reals.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For -0 as valid input
#
#       Parameters : (d0,d1)    - operand 1
#                    (d2,d3)    - operand 2
#
#       Result     : Returned in the ccr (EQ,NE,GT,LT,GE,LE).
#
#       Misc       : The operands are not destroyed, and no other registers 
#                    are used.
#
#******************************************************************************

compare:  tst.l   %d0             #test first for sign of the first operand
         bpl.w     rcomp2 
         tst.l   %d2             #test sign of second operand
         bpl.w     rcomp2
#
         cmp.l   %d2,%d0          #both negative so do test backward
         bne.w     cmpend         #ccr set here 
         cmp.l   %d3,%d1          #first part equal, check second part
         beq.w     cmpend         #EQ flag set
         bhi.w     grt            #unsigned compare
lst:         move    &8,%cc         #XNZVC = 01000
            rts
#
rcomp2:   cmp.l   %d0,%d2          #at least one positive, ordinary test
         bne.w     checkm0        #must check for 0 compared with -0
         cmp.l   %d1,%d3          #both must be positive
         beq.w     cmpend
         bls.w     lst            #branch if LT
grt:         move    &0,%cc         #XNZVC = 00000
cmpend:   rts
#
# Check for the operands being 0 and -0.
#
checkm0:  tst.l   %d0
         bpl.w     d2minus        #branch if second operand is negative
            cmp.l   %d0,&minuszero  #else (d0,d1) is negative
            bne.w     finm0       #reset condition code
            tst.l   %d2
            bne.w     finm0       #must check all of it
               rts                 #had (d0,d1) = -0 and (d2,d3) = 0
d2minus:  cmp.l   %d2,&minuszero  #(d2,d3) is negative
         bne.w     finm0       #reset condition code
         tst.l   %d0
         bne.w     finm0       #must check all of it
            rts              #had (d2,d3) = -0 and (d0,d1) = 0
finm0:   cmp.l   %d0,%d2        #else reset condition code 
        rts
