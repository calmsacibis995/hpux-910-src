# @(#) $Revision: 66.3 $     
#
	global   rmul,radd,rsbt,rdvd

        	set  	SIGFPE,8		#floating point exception
         	set	minuszero,0x80000000	#top part of IEEE -0

#******************************************************************************
#
#       Procedure  : rmul
#
#       Description: Do a software 64 bit real multiply.
#
#       Author     : Paul Beiser / Ted Warren
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For:
#                            o To check for -0 as a valid operand
#
#       Parameters : (d0,d1)    - first operand
#                    (d2,d3)    - second operand
#
#       Registers  : d4,d5,d6   - partial products
#                    d7         - sticky bit information
#                    a0         - result exponent
#                    
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : Real overflow. If underflow occurrs, return a 0.
#
#       References : None
#
#       Miscel     : No a registers are destroyed. This is not
#                    quite IEEE because 0 is always returned as 
#                    a result regardless of the sign of the operands. 
#
#******************************************************************************

retzero: moveq   &0,%d0           #return zero
        move.l  %d0,%d1
        rts
#
#  Shortness is defined as < 17 bits of mantissa.
#
short2:  tst.l   %d3              #test opnd2lo for zero
        bne.w     ts2
           move.l  %d0,%d6           #test both operandhi for
           or.l    %d2,%d6           #shortness
           swap    %d6
           and.w   &0x1f,%d6
           beq.w     shxsh           #short times a short
              move.l  %d2,%d6           #test opnd2hi for shortness
              swap    %d6
              and.w   &0x1f,%d6
              bne.w     ts2
                 exg     %d0,%d2
                 exg     %d1,%d3            #short opnd in d0-d1
                 bra.w     longxsh          #long times a short
#
#  If here then opnd2 is definitely not short.
#
ts2:     move.l  %d0,%d6
        swap    %d6              #test opnd1hi for shortness
        and.w   &0x1f,%d6
        bne.w     phase1
           bra.w     longxsh
short1:  move.l  %d2,%d6           #test opnd2hi
        swap    %d6              #for shortness
        and.w   &0x1f,%d6
        bne.w     ph1a
           exg     %d0,%d2
           exg     %d1,%d3
           bra.w     longxsh
           
#******************************************************************************
#
#  64 bit real multiply begins here.
#
rmul:
#	trap	#2
#	dc.w	-4
	  cmp.l   %d0,&minuszero #check first operand for -0
          beq.w     retzero       #return +0 as the answer
          cmp.l   %d2,&minuszero #check second operand for -0
          beq.w     retzero       #return +0 as the answer
          move.l  &0x80007ff0,%d5 #mask for exponent evaluation
          move.l  %d0,%d7         #high order opnd1 -> d7
          beq.w     retzero       #branch if zero operand
          swap    %d0            #duplicate high order word into
          move.w  %d0,%d7         #low order word of d7
          move.l  %d2,%d6         #do the same for opnd2 into d6
          beq.w     retzero       #branch if zero operand
          move.l  %a0,-(%sp)      #a0 must not be altered by this routine
          swap    %d2
          move.w  %d2,%d6
          and.l   %d5,%d6         #use mask to put sign in high order
          and.l   %d5,%d7         #and exponent in low order word
          add.l   %d6,%d7         #form result sign and exponent at once
          moveq   &0xf,%d6        #mask for removing exponent
          and.w   %d6,%d0         #extract mantissas
          and.w   %d6,%d2
          moveq   &0x10,%d6       #mask for inserting hidden one
          or.w    %d6,%d2         #put in hidden one
          or.w    %d6,%d0
          movea.l %d7,%a0         #store result exponent in a0
          moveq   &0,%d7         #use d7 for sticky bit
          tst.l   %d1            #can we do a faster multiply?
          beq.w     short2
#
#                                     B3    B2   B1   B0
#                          X          A3    A2   A1   A0
#                               ---------------------------
#                                               [A0 X B0] (1)
#                                          [A0 X B1]      (2.1)
#                                          [A1 X B0]      (2.2)
#                                     [A1 X B1]           (3.1)
#                                     [A2 X B0]           (3.2)
#                                     [A0 X B2]           (3.3)
#                                [A3 X B0]                (4.1)
#                                [A2 X B1]                (4.2)
#                                [A0 X B3]                (4.3)
#                                [A1 X B2]                (4.4)
#                           [A3 X B1]                     (5.1)
#                           [A1 X B3]                     (5.2)
#                           [A2 x B2]                     (5.3)
#                      [A2 X B3]                          (6.1)
#                      [A3 X B2]                          (6.2)
#                 [A3 X B3]                               (7)
#-------------------------------------------------------------
#                 PP7  PP6  PP5  PP4  PP3  PP2  PP1  PP0
#
# Keep PP4 thru PP7; use PP0 thru PP3 for stickiness.

#
#                       Phase 1
#                        (1)
#
phase1:   move.l  %d3,%d5          #check for shortness
         beq.w     short1
ph1a:     mulu    %d1,%d5          #A0*B0
         or.w    %d5,%d7          #keep track of lost bits for stickiness
         clr.w   %d5             #discard bits 0-15
         swap    %d5
#
#                       Phase 2
#
#                       (2.1)
#
         move.l  %d3,%d6
         swap    %d6
         mulu    %d1,%d6          #A0*B1
         add.l   %d6,%d5
#
#                       (2.2)
#
         clr.w   %d4
         move.l  %d1,%d6
         swap    %d6
         mulu    %d3,%d6          #A1*B0
         add.l   %d6,%d5
         addx.w  %d4,%d4
         or.w    %d5,%d7
         move.w  %d4,%d5
         swap    %d5
#
#                       Phase 3
#                       (3.1)
#
#
         move.l  %d3,%d6
         swap    %d6
         swap    %d1
         mulu    %d1,%d6          #A1*B1
         swap    %d1
         add.l   %d6,%d5
#
#                       (3.2)
#
         move.l  %d0,%d6
         swap    %d6
         mulu    %d3,%d6          #A2*B0
         add.l   %d6,%d5
         clr.w   %d4
         addx.w  %d4,%d4
#
#                       (3.3)
#
         move.l  %d2,%d6
         swap    %d6
         mulu    %d1,%d6          #A0*B2
         add.l   %d6,%d5
         or.w    %d5,%d7
         move.w  %d4,%d5
         negx.w  %d5
         neg.w   %d5
         swap    %d5
#
#                       Phase 4
#                       (4.1)
#
         move.w  %d0,%d6
         mulu    %d3,%d6          #A3*B0
         add.l   %d6,%d5
#
#                       (4.2)
#
         swap    %d3
         move.l  %d0,%d6
         swap    %d6
         mulu    %d3,%d6          #A2*B1
         swap    %d3
         add.l   %d6,%d5
         clr.w   %d4
         addx.w  %d4,%d4
#
#                       (4.3)
#
         move.w  %d2,%d6
         mulu    %d1,%d6          #A0*B3
         add.l   %d6,%d5
         negx.w  %d4
         neg     %d4
#
#                       (4.4)
#
         move.l  %d2,%d6
         swap    %d6
         swap    %d1
         mulu    %d1,%d6          #A1*B2
         swap    %d1
         add.l   %d6,%d5
         negx.w  %d4
         neg.w   %d4
         swap    %d4
         swap    %d5
         move.w  %d5,%d4
#
#                       Phase 5
#                       (5.1)
#
#
         clr.w   %d5
         move.l  %d3,%d6
         swap    %d6
         mulu    %d0,%d6          #A3*B1
         add.l   %d6,%d4
#
#                       (5.2)
#
#
         move.l  %d1,%d6
         swap    %d6
         mulu    %d2,%d6          #A1*B3
         add.l   %d6,%d4
#
#                       (5.3)
#
#
         move.l  %d2,%d6
         swap    %d6
         swap    %d0
         mulu    %d0,%d6          #A2*B2
         swap    %d0
         add.l   %d6,%d4
         addx.w  %d5,%d5
         move.w  %d5,%d6
         move.w  %d4,%d5
         move.w  %d6,%d4
         swap    %d5
         swap    %d4
#
#                       Phase 6
#
#                       (6.1)
#
         move.l  %d0,%d6
         swap    %d6
         mulu    %d2,%d6          #A2*B3
         add.l   %d6,%d4
#
#                       (6.2)
#
#
         move.l  %d2,%d6
         swap    %d6
         mulu    %d0,%d6          #A3*B2
         add.l   %d6,%d4
#
#                       Phase 7
#
#                       (7)
#
         move.w  %d0,%d6
         mulu    %d2,%d6          #A3*B3
         swap    %d6
         add.l   %d6,%d4
#
#  Post normalization after multiplication
#
p_norm:   btst    &25,%d4
         bne.w     m_norm_1
#
#  Shift whole mantissa 4 places right. This avoids 1 shift left.
#
         suba.w  &0x10,%a0        #adjust exponent
         move.l  %d4,%d0
         lsr.l   &4,%d0
         and.l   &0xf,%d4
         ror.l   &4,%d4
         move.l  %d5,%d1
         lsr.l   &4,%d1
         or.l    %d4,%d1
         add.l   %d5,%d5          #put round and stcky bits in place
         bra.w     mround
#
#  Now shift whole mantissa right 5 places.
#
m_norm_1: move.l  %d4,%d0
         lsr.l   &5,%d0
         and.l   &0x1f,%d4
         ror.l   &5,%d4
         move.l  %d5,%d1
         lsr.l   &5,%d1
         or.l    %d4,%d1
#
#  Result in (d0,d1). Now round.
#
mround:   btst    &4,%d5          #test round bit
         beq.w     roundun        #if clear then no rounding to do
         and.b   &0xf,%d5         #get bits lost during last alignment
         or.b    %d5,%d7          #factor into sticky bit
mul_rnd2: tst.w   %d7             #test mr. sticky
         bne.w     round_up       #if sticky and round then round up
            btst    &0,%d1          #test lsb of result
            beq.w     roundun        #else round to even
round_up: addq.l  &1,%d1
         bcc.w     rm_4
            addq.l  &1,%d0
rm_4:     btst    &21,%d0
         beq.w     roundun        #test for mantissa overflow
            lsr.l   &1,%d0          #d1 must already be zero
            adda.w  &0x10,%a0
#
#  Extract result sign for later 'or' with the exponent.
#
roundun:  move.l  %a0,%d6          #get sign
         swap    %d6             #place in bottom word
#
#  Complete exponent calculation with tests for overflow and underflow.
#
         move.l  %a0,%d7          #exponent with the sign
         bpl.w     no_clear       #branch if top portion already cleared
            swap    %d7             #else clear the sign bit
            clr.w   %d7
            swap    %d7
no_clear: movea.l  (%sp)+,%a0      #restore original value of a0
         sub.l   &0x4000-0x10,%d7  #remove extra bias minus hidden one
         bmi.w     err_underflow  #exponent underflow?
         cmp.w   %d7,&0x7fd0      #hidden bit add on later
         bhi.w     err_overflow   #or overflow?
#
#  Merge exponent and mantissa.
#
         or.w    %d6,%d7          #place sign with the exponent
         swap    %d7             #place exponent into top portion
         add.l   %d7,%d0          #aha, hidden bit finally adds back!
         rts
         
#*******************************************************************************
#
#  Shorter precision multiply when possible.
#
shxsh:    swap    %d0             #align 16 bits of mantissa into d0
         swap    %d2             #same for d2
         lsr.l   &5,%d0
         lsr.l   &5,%d2
         mulu    %d2,%d0          #A0*B0 only one multiply required here
         swap    %d0             #rotate and mask result into correct bits
         move.l  %d0,%d1
         clr.w   %d1
         lsl.l   &5,%d1
         rol.l   &5,%d0
         and.l   &0x001fffff,%d0
         btst    &20,%d0         #test for post-normalize
         bne.w     roundun        #note: no rounding possible, too few bits
            add.l   %d1,%d1          #shift mantissa left one position
            addx.l  %d0,%d0
            suba.w  &0x10,%a0        #compensate exponent
            bra.w     roundun
#
#  Long times shorter.
#
longxsh:  swap    %d0             #align 16 bits of mantissa into d0
         lsr.l   &5,%d0
         move.w  %d3,%d5
         mulu    %d0,%d5          #A0 * B0
         or.w    %d5,%d7          #keep PP0 in d7 for rounding
         clr.w   %d5
         swap    %d5
         move.l  %d3,%d6
         swap    %d6
         mulu    %d0,%d6          #A0 * B1
         add.l   %d6,%d5
         move.w  %d5,%d4
         clr.w   %d5
         swap    %d5
         move.l  %d2,%d6
         swap    %d6
         mulu    %d0,%d6          #A0 * B2
         add.l   %d6,%d5
         swap    %d4
         move.w  %d5,%d4
         swap    %d4
         clr.w   %d5
         swap    %d5
         move.w  %d2,%d6
         mulu    %d0,%d6          #A0 * B3
         add.l   %d6,%d5
         move.l  %d5,%d0
         move.l  %d4,%d1
         btst    &20,%d0         #test for post-normalize
         bne.w     lxs2
            add.w   %d7,%d7          #shift entire fraction left
            addx.l  %d1,%d1
            addx.l  %d0,%d0
            suba.w  &0x10,%a0        #fix exponent
lxs2:     add.w   %d7,%d7          #round bit into carry, leaving stickyness in d7
         bcc.w     roundun
            bra.w     mul_rnd2       #possible rounding to do

#******************************************************************************
#
#       Procedure  : rdvd
#
#       Description: Do a software 64 bit real divide.
#
#       Author     : Sam Sands / Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For:
#                            o To check for -0 as a valid operand
#
#       Parameters : (d0,d1)    - first operand (dividend)
#                    (d2,d3)    - second operand (divisor)
#
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : Real overflow, div/0. If underflow occurrs, return a 0.
#
#       References : None.
#
#       Miscel     : No a registers are destroyed. This is not
#                    quite IEEE because 0 is always returned as 
#                    a result regardless of the sign of the operands. 
#
#******************************************************************************
#
#
#  This routine called 4 times will produce up to 64 quotient bits 
#  d0-d1 is 64 bit dividend 
#  d2-d3 is 64 bit divisor      (should be normalized (bit 31 = 1)) 
#  d4-d5 is 64 bit quotient 
#
dv00:     swap    %d4             #shift quotient left 16 bits 
         swap    %d5 
         move.w  %d5,%d4 
#
         tst.l   %d0             #1st 32 dividend bits  /  1st 16 divisor bits
         beq.w     dv7 
dv0:         swap    %d2 
            divu    %d2,%d0
            bvc.w     normal         #branch if no overflow
#
#  Had an overflow on the divide. Our quotient must be 0xffff or 0xfffe, and 
#  the fixup for the new dividend is derived as follows.
#
#  DVD := Shl16(d0,d1) - Quotient * (d2,d3)
#      := Shl16(d0,d1) - (2^16-c) * (d2,d3);  c = 1 or 2
#      := Shl16(d0,d1) - Shl16(d2,d3) + c(d2,d3)
#      := Shl16( (d0,d1) - (d2,d3) ) + c(d2,d3)
#
            swap    %d2                #restore correct order of divisor
            move.w  &0xffff,%d5         #new quotient
            sub.l   %d3,%d1             #(d0,d1) - (d2,d3)
            subx.l  %d2,%d0
            swap    %d0                #shift left by 16
            swap    %d1
            move.w  %d1,%d0
            clr.w   %d1
            bra.w     dv6               #fixup up dividend (add back at least once)
#
#  Normal divide - no overflow. Go through standard routine.
#
normal:   swap    %d2 
dv7:      move.w  %d0,%d5          #16 bits shifted into quotient register 
         swap    %d1             #shift dividend left 16 bits
         move.w  %d1,%d0          #except for remainder in d0 upper
         clr.w   %d1
         tst.w   %d5             #finish low order part of division: 
         beq.w     dv1       
         moveq   &0,%d7          #d7 is used for borrow bit out of dividend 
         move.w  %d2,%d6          #dividend - (quotient * 2nd 16 divisor bits) 
         beq.w     dv2
            mulu    %d5,%d6 
            sub.l   %d6,%d0
            bcc.w     dv2 
            subq    &1,%d7 
#
dv2:      move.w  %d3,%d6          #dividend - (quotient * 4th 16 divisor bits) 
         beq.w     dv3 
            mulu    %d5,%d6 
            sub.l   %d6,%d1 
            bcc.w     dv3 
               subq.l  &1,%d0 
               bcc.w     dv3 
                  subq    &1,%d7 
#
dv3:      swap    %d3             #dividend - (quotient * 3rd 16 divisor bits) 
         move.w  %d3,%d6 
         beq.w     dv4 
            mulu    %d5,%d6 
            swap    %d1 
            sub.w   %d6,%d1 
            swap    %d1 
            swap    %d6 
            subx.w  %d6,%d0 
            bcc.w     dv4 
               sub.l   &0x10000,%d0 
               bcc.w     dv4 
                  subq    &1,%d7 
dv4:      swap    %d3 
         tst.w   %d7             #restore dividend and quotient if it didn't go 
         bpl.w     dv1 
#
dv5:         subq.l  &1,%d5          #decrement quotient 
            bcc.w     dv6 
               subq.l  &1,%d4          #propagate the borrow in the quotient
dv6:         add.l   %d3,%d1          #add divisor back to dividend 
            addx.l  %d2,%d0 
            bcc.w     dv5            #repeat till dividend >= 0 
#                               (at most twice more if bit 31 of divisor is 1) 
dv1:     rts 

#******************************************************************************
#
#  Main body of the real divide.
#
rdvd:
#	trap	#2
#	dc.w	-4
	 tst.l   %d2             #check for zero
         beq.w     err_divzero     #branch if divisor is a zero
         cmp.l   %d2,&minuszero   #check for -0
         beq.w     err_divzero     #branch if divisor is a zero
#
#  Check for a zero dividend.
#
dvndzer:  tst.l   %d0
         bne.w     checkn  
divret0:     moveq   &0,%d0          #else return a zero result
            move.l  %d0,%d1
            rts         
checkn:   cmp.l   %d0,&minuszero  #check for -0
         beq.w     divret0        
#
#  Prepare mantissas for divide, and save exponents for later.
#
procdvd:  moveq   &0x000f,%d6      #masks for the mantissa preparation
         moveq   &0x0010,%d7
         swap    %d2             #get the mantissas
         move.w  %d2,-(%sp)       #push the divisor exponent
         and.w   %d6,%d2
         or.w    %d7,%d2
         swap    %d2
         swap    %d0             #same for next operand
         move.w  %d0,-(%sp)       #push the dividend exponent
         and.w   %d6,%d0
         or.w    %d7,%d0
         swap    %d0             #mantissas ready for divide; compute exp
#
#  Divide of the mantissas with the remainder in (d0,d1) 
#  and a 55 bit result to enajle proper rounding. The result
#  is generated in (d4,d5).
#
        add.l   %d1,%d1           #preshift dividend so quotient lines up right 
        addx.l  %d0,%d0 

        moveq   &11,%d7          #normalize divisor so that bit 31 = 1 
        lsl.l   %d7,%d2 
        rol.l   %d7,%d3 
        move.l  %d3,%d6 
        and.w   &0xf800,%d3 
        and.w   &0x07ff,%d6 
        or.w    %d6,%d2 
        
ifdef(`PIC',`
	bsr.l	dv0	#inner	loop	of	divide
	bsr.l	dv00
	bsr.l	dv00
	bsr.l	dv00',`
	jsr	dv0
	jsr	dv00
	jsr	dv00
	jsr	dv00
')
        move.l  %d4,%d2           #place here so sticky bit can be set 
        move.l  %d5,%d3
#
#  Compute the new exponent and sign.
#
         moveq   &0,%d7          #contain the exponent and sign of result
         move.l  %d7,%d5          #exponent calculation registers
         move.l  %d7,%d6
         move.w  (%sp)+,%d5       #get dividend exponent
         move.w  (%sp)+,%d6       #get divisor exponent
         eor.w   %d5,%d6          #compute sign of result
         bpl.w     possign
             move.w  &0x8000,%d7     #negative sign
possign:  eor.w   %d5,%d6          #restore exponents - nice trick
         move.w  &0x7ff0,%d4      #masks for the exponents
         and.w   %d4,%d5          #mask out exponents
         and.w   %d4,%d6
         sub.l   %d6,%d5          #dividend exponent - divisor exponent
         add.l   &0x3ff0-0x10,%d5  #bias - hidden bit (hidden bit adds later)
#
#  Normalize mantissa if necessary and compute sticky bit.
#
possitv:  btst    &22,%d2         #check leading bit for normalize
         bne.w     shftd          #branch if already a one
            add.l   %d3,%d3          #else make it a leading one
            addx.l  %d2,%d2
            sub.l   &0x10,%d5        #adjust exponent
shftd:    or.l    %d0,%d1          #set sticky bit with remainder
         beq.w     rnd            #if zero, sticky bit set correctly
            or.b    &1,%d3          #else set sticky bit
#
#  Do the round and check for overflow and underflow.
#
rnd:      btst    &1,%d3          #check round bit
         beq.w     rend           #branch if nothing to round
         addq.l  &0x2,%d3         #add 1 in the round bit
         bcc.w     rndcon         #branch if nothing to propagate
            addq.l  &1,%d2          #else propagate the carry
rndcon:   move.b  %d3,%d0          #get the sticky bit
         lsr.b   &1,%d0          #place into carry
         bcs.w     norml          #branch if number not halfway between
            and.b   &0xf8,%d3        #all zero so clear lsb (round to even)
norml:    btst    &23,%d2         #check for overflow
         beq.w     rend           #if a zero then no overflow
            lsr.l   &1,%d2          #only bit set is #24 because of overflow
            add.l   &0x10,%d5        #adjust exponent accordingly
rend:     tst.l   %d5             #check for underflow 
         bmi.w     err_underflow  #underflow error handler
         cmp.w   %d5,&0x7fd0      #check for overflow (remember, hidden bit! )
         bhi.w     err_overflow   #overflow error handler
#
#  Splice result together.
#
         lsr.l   &1,%d2          #throw away round and sticky bits
         roxr.l  &1,%d3
         lsr.l   &1,%d2
         roxr.l  &1,%d3
         or.w    %d5,%d7          #place exponent with sign
         swap    %d7
         add.l   %d7,%d2          #ah!, hidden bit finally adds back!!
         move.l  %d2,%d0          #place in the correct registers
         move.l  %d3,%d1
         rts

#******************************************************************************
#
#       Procedure  : radd / rsbt
#
#       Description: Do a software 64 bit real addition/subtraction.
#
#       Author     : Sam Sands / Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For:
#                            o To check for -0 as a valid operand
#
#       Parameters : (d0,d1)    - first operand
#                    (d2,d3)    - second operand
#
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : Real overflow. If underflow occurrs, return a 0.
#
#       References : None.
#
#       Miscel     : No a registers are destroyed. This is not
#                    quite IEEE because 0 is always returned as 
#                    a result regardless of the sign of the operands. 
#
#******************************************************************************

first_z:  move.l  %d7,%d0          #if subtracting from zero then the
         move.l  %d3,%d1          #result is operand2 with the sign
         rts                    #complemented previously
#
#  This is the subtract front end. The second operand is subtracted 
#  by complementing its sign.
#
rsbt: 	 cmp.l   %d2,&minuszero  #check second operand for -0
         bne.w     rsbt1
            rts                    #(d0,d1) is the result 
rsbt1:    move.l  %d2,%d7          #copy operand2 high order to d7 
         bne.w     subnonz        #zero value?
            rts                    #else (d0,d1) is the result
subnonz:  bchg    &31,%d7         #complement sign bit for subtract
         bne.w     second_p       #test if plus or minus

second_m: cmp.l   %d0,&minuszero  #check first operand for -0
         bne.w     sec11          #branch if not a -0
            moveq   &0,%d0          #else make it a plus 0
sec11:    move.l  %d0,%d6          #copy operand1 high order to d6
         beq.w     first_z        #-(d2,d3) is the result
         bmi.w     same_sig       #if signs are different then set
         
difsigns: move.w  &-1,%d6         #subtract flag 
         bra.w     add1 
         
prenorm:  moveq   &0,%d4          #no prenormalization to do 
         bra.w     do_it          #so clear overflow (g,r,s)
#
#  This is the add front end.
#
radd:     cmp.l   %d2,&minuszero  #check second operand for -0
         bne.w      radd1
            rts                    #(d0,d1) is the result 
radd1:    move.l  %d2,%d7          #copy operand2 high order to d7
         bne.w     add_11         #test for zero 
            rts                    #else (d0,d1) is the result
add_11:   bmi.w     second_m       #test sign
second_p: cmp.l   %d0,&minuszero  #check first operand for -0
         bne.w     sss11          #branch if not a -0
            moveq   &0,%d0          #else make it a plus 0
sss11:    move.l  %d0,%d6          #copy operand1 high order to d6
         beq.w     first_z        #also test it for zero 
         bmi.w     difsigns       #and check its sign
same_sig: clr.w   %d6             #clear subtract flag

#******************************************************************************
#
#  Common to both the add and subtract.
#
add1:     moveq   &0x000f,%d4      #masks for mantissa extraction
         moveq   &0x0010,%d5
         swap    %d0             #clear out exponent of operand1
         and.w   %d4,%d0          #and put in hidden one bit
         or.w    %d5,%d0 
         swap    %d0 
         swap    %d2             #do the same for operand2
         and.w   %d4,%d2
         or.w    %d5,%d2
         swap    %d2 
         swap    %d6             #note: sign flag goes into high part 
         swap    %d7 
         move.w  &0x7ff0,%d4      #take difference of exponents
         move.w  %d4,%d5
         and.w   %d6,%d4
         and.w   %d7,%d5
         sub.w   %d5,%d4 
         beq.w     prenorm        #skip prenormalization
         asr.w   &4,%d4          #faster to shift difference
         bpl.w     add2           #larger operand in d0-d1?
         neg.w   %d4             #otherwise swap
         move.w  %d7,%d6          #use larger exponent
         exg     %d0,%d2
         exg     %d1,%d3
add2:     moveq   &-1,%d7         #all ones mask in d7
         cmp.w   %d4,&32         #use move.l for >= 32 
         bge.w     long_sh    
            lsr.l   %d4,%d7          #rotate mask and merge to shift
            ror.l   %d4,%d2          #a 64 bit value by N positions
            ror.l   %d4,%d3          #without looping
            move.l  %d3,%d4          #dump spillover into d3
            move.l  %d2,%d5 
            and.l   %d7,%d2
            and.l   %d7,%d3 
            not.l   %d7 
            and.l   %d7,%d5 
            or.l    %d5,%d3 
            and.l   %d7,%d4 

do_it:    move.w  %d6,%d5          #get result exponent
         tst.l   %d6
         bmi.w     sub_it         #remember subtract flag?
#
#  Add 2 numbers with the same signs.
#
add_it:   and.w   &0x7ff0,%d5      #mask out exponent
         move.l  &0x00200000,%d7  #mask for mantissa overflow test
         add.l   %d3,%d1          #this is it, sports fans
         addx.l  %d2,%d0 
         cmp.l   %d0,%d7          #test for mantissa overflow
         blt.w     add3 
            add.w   &16,%d5         #exponent in bits 15/5    
            lsr.l   &1,%d0          #everything right and increment 
            roxr.l  &1,%d1          #the exponent
            roxr.l  &1,%d4
            bcc.w     add3           #don't forget to catch the
               or.w    &1,%d4          #sticky bit
add3:     cmp.l   %d4,&0x80000000  #test for rounding
         bcs.w     add5           #if lower then no rounding to do  
         bhi.w     add4           #if higher then round up
            btst    &0,%d1          #otherwise test mr. sticky
            beq.w     add5
add4:     addq.l  &1,%d1          #here we are at the roundup
         bcc.w     add5 
            addq.l  &1,%d0
            cmp.l   %d0,%d7          #a word to the wise: test for
            blt.w     add5           #mantissa overflow when you 
               lsr.l   &1,%d0          #round up during an add
               add.w   &16,%d5         #exponent in bits 15/5  
add5:     cmp.w   %d5,&0x7fe0      #check for exponent overflow
         bhi.w     err_overflow
         tst.w    %d6            #get sign of the result     
         bpl.w     add6           #positive result
            add.w   &0x8000,%d5      #copy sign bit
add6:     swap    %d5
         clr.w   %d5             #for the or
         bclr    &20,%d0         #hide hidden one
         or.l    %d5,%d0          #exponent into mantissa
         rts          
#
#  Add two numbers with differing signs.
#
sub_it:   lsr.w   &4,%d5          #align in correct location
         and.w   &0x07ff,%d5      #get rid of the sign bit
         neg.l   %d4             #zero minus overlow 
         subx.l  %d3,%d1          #subtract low order
         subx.l  %d2,%d0          #subtract high order
         tst.l   %d0             #test for top 21 bits all zero
         beq.w     zerores        #at least 21 left shifts necessary
         bpl.w     sign_un        #did we do it the right way?
            add.w   &0x8000,%d6      #flip sign of result
            neg.l   %d1             #Note: this path only taken if path 
            negx.l  %d0                   #thru prenormalized was taken
            tst.l   %d0             #check for top 21 bits being zero
            beq.w     zerores        #at least 21 left shifts necessary
sign_un:  move.l  &0x00100000,%d7  #post normalization mask 
         cmp.l   %d0,%d7          #test for post normalization
         bge.w     sub1 
         add.l   %d4,%d4          #shift everything left one
         addx.l  %d1,%d1          #shift along guard bit first
         addx.l  %d0,%d0          #time only
         subq.w  &1,%d5          #decrement exponent
         cmp.l   %d0,%d7          #normalized yet?
         bge.w     sub1 
         move.l  %d0,%d4          #test for shift by 16
         and.l   &0x001fffe0,%d4  #test high 16 bits
         bne.w     norm8lop       #if not 16 , check by 8
            sub.w   &16,%d5         #adjust exponent
            swap    %d0
            swap    %d1
            move.w  %d1,%d0
            clr.w   %d1
            bra.w     normlopp       #less than 5 shifts left (maybe 0)
norm8lop: move.l  %d0,%d4          #test for shift by 8
         and.l   &0x001fe000,%d4  #check 8 high bits
         bne.w     normloop       #at least one shift still necesarry!
            sub.w   &8,%d5          #adjust exponent
            lsl.l   &8,%d0
            rol.l   &8,%d1
            move.b  %d1,%d0          #d0 correct
            clr.b   %d1             #d1 correct
normlopp: cmp.l   %d0,%d7          #must test here - could be done
         bge.w     sub2           #no rounding necessary
normloop: add.l   %d1,%d1          #this is for post normalizing < 8 times
         addx.l  %d0,%d0          #for any additional shifting
         subq.w  &1,%d5          #note: this code can be improved
         cmp.l   %d0,%d7
         blt.w     normloop
         bra.w     sub2           #no rounding necessary
sub1:     cmp.l   %d4,&0x80000000  #rounding for subtract
         bcs.w     sub2           #same sequence as add
         bhi.w     sub3
            btst    &0,%d1
            beq.w     sub2
sub3:     addq.l  &1,%d1          #round up
         bcc.w     sub2
            addq.l  &1,%d0
            btst    &21,%d0         #mantissa overflow?
            beq.w     sub2 
               asr.l   &1,%d0
               addq    &1,%d5          #increment exponent (can't overflow)
sub2:     tst.w   %d5             #test for exponent underflow
         ble.w     err_underflow   
            lsl.w   &5,%d5          #exponent in top so can place in sign
            add.w   %d6,%d6          #get sign
            roxr.w  &1,%d5          #into exponent
            swap    %d5
            clr.w   %d5             #for the or
            bclr    &20,%d0         #hide hidden one
            or.l    %d5,%d0          #exponent into mantissa
            rts          
            
shifted_: bclr    &20,%d0         #more than 55 shifts to prenormalize
         swap    %d6             #so reconstruct larger operand and
         clr.w   %d6             #return in d0-d1
         or.l    %d6,%d0
         rts         
         
long_sh:  beq.w     ls1            #branch if exactly 32 shifts
         cmp.w   %d4,&55         #if shift count is too large then
         bgt.w     shifted_       #don't bother
            sub.w   &32,%d4 
            lsr.l   %d4,%d7 
            ror.l   %d4,%d2 
            ror.l   %d4,%d3 
            move.l  %d3,%d4
            move.l  %d2,%d5 
            and.l   %d7,%d2
            and.l   %d7,%d3 
            not.l   %d7 
            and.l   %d7,%d5 
            or.l    %d5,%d3 
            and.l   %d7,%d4 
            beq.w     ls1 
               or.w    &1,%d3 
ls1:      move.l  %d3,%d4 
         move.l  %d2,%d3 
         moveq   &0,%d2 
         bra.w     do_it 
         
zerores:  tst.l   %d1
         bne.w     longnorm       #if result was zero after subtract, done
         tst.l   %d4             #check guard bit
         bmi.w     longnorm     
            rts                  
            
longnorm: add.l   %d4,%d4          #result nearly zero, shift 21 or more
         addx.l  %d1,%d1
         bcs.w     norm21         #exact shift by 21
         swap    %d1             #test for shift of 16
         tst.w   %d1
         bne.w     test8          #test for shift of 8
            sub.w   &16,%d5         #adjust exponent (d1 correct)
            move.l  %d1,%d7          #check which byte first one in
            swap    %d7            
            and.w   &0xff00,%d7
            bne.w     lnloop         #less than 8 shifts left
               lsl.l   &8,%d1          #else adjust 
               subq.w  &8,%d5
               bra.w     lnloop
test8:    move.w  %d1,%d7          #check lower bytes
         swap    %d1             #d1 in correct order
         and.w   &0xff00,%d7
         bne.w     lnloop         #less than 8 shifts left
            lsl.l   &8,%d1          #else adjust 
            subq.w  &8,%d5
lnloop:   subq.w  &1,%d5          #less than 8 shifts left
         add.l   %d1,%d1
         bcc.w     lnloop 
norm21:   sub.w   &21,%d5         #adjust exponent
         swap    %d1             #rotate left 20 or more places
         rol.l   &4,%d1          #copy over the boundary
         move.l  %d1,%d0 
         and.l   &0x000fffff,%d0  #save high 20 bits
         and.l   &0xfff00000,%d1  #save low 12 bits
         bra.w     sub2           #hidden 1 is already gone

#
# Code for exceptions.
#
err_underflow:
	 moveq   &0,%d0
	 moveq   &0,%d1
	 rts
err_divzero:
err_overflow:
	 trap    &SIGFPE
	 rts
