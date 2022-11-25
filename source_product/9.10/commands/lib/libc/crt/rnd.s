# @(#) $Revision: 66.2 $    
#
	global	rndnear,rndzero
		
#******************************************************************************
#  
#  Clear the bottom (d5) bits of (d0,d1). Used by rndzero and rndnear.
#
clearbit: moveq	&-1,%d7		#mask for the clear
	cmp.w	%d5,&32		
	blt.w	cleard1		#branch if only have to clear bits in d1
	moveq	&0,%d1		#else clear all of d1
	sub.w	&32,%d5		#adjust count
	bne.w	clearcon		#branch if more to clear
	rts			#else return
clearcon: lsl.l	%d5,%d7		#get mask
	and.l	%d7,%d0
	rts	
cleard1:	lsl.l	%d5,%d7
	and.l	%d7,%d1
	rts	

#******************************************************************************
#
#       Procedure rndzero
#
#       Description:
#               Round a real number to a whole real number, with a
#               rounding mode of round-to-zero. If the real is too large in 
#               magnitude to be rounded, the same number is returned.
#               No A registers are changed.
#
#       Parameters:
#               (d0,d1) - real number to be rounded
#
#       The result is returned in (d0,d1).
#
#       Error conditions:
#               There are none.
#
#       External references:
#               There are none.
#
#       Register usage:
#               All D registers except d2,d3, and d4.
#
rndzero:	move.l	%d0,%d5		#extract the exponent
	swap	%d5		#place in low word
	and.w	&0x7ff0,%d5		#get rid of sign bit
	lsr.w	&4,%d5		#in low 11 bits
	sub.w	&1022,%d5		#unbiased exponent plus one 
#
#  Check if number is too small or too large.
#
	bgt.w	rndcn0		#branch if not too small
	moveq	&0,%d0		#else return a zero
	move.l	%d0,%d1
	rts	
rndcn0:	cmp.w	%d5,&53
	blt.w	rndcn1		#branch if number not too large
	   rts			   #else	return with same number
rndcn1:	neg.w	%d5		#map into correct range
	add.w	&53,%d5		#1 <= d5 <= 52  (so can clear correct bits)
        bra     clearbit	#clear correct number of bits

#******************************************************************************
#
#       Procedure  : rndnear
#
#       Description: Round a real number to the nearest whole real number.
#                    If the real is too large to be rounded, the same 
#                    number is returned.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#
#       Parameters : (d0,d1)    - real argument
#
#       Registers  : d5-d7      - scratch
#                    
#       Result     : The result is returned in (d0,d1).
#
#       Error(s)   : None
#
#       References : None
#
#******************************************************************************

rndnear:  move.l  %d0,%d6          #extract the exponent
         swap    %d6             #place in low word
         and.w   &0x7ff0,%d6      #get rid of sign bit
         lsr.w   &4,%d6          #in low 11 bits
         sub.w   &1022,%d6       #unbiased exponent plus one 
#
#  Check if number is too small or large.
#
         bgt.w     checknxt       #branch if check for exponent too large
         blt.w     rnd_zero       #branch if so small that return a zero
            moveq   &0,%d1          #else return + or - 1.0
            tst.l   %d0             #determine sign
            bmi.w     retmin
               move.l  &0x3ff00000,%d0
               rts
retmin:      move.l  &0xbff00000,%d0
            rts
rnd_zero: moveq   &0,%d0
         move.l  %d0,%d1 
         rts
checknxt: cmp.w   %d6,&53
         blt.w     nearcon        #continue the round; 1 <= exp <= 52
            rts                    #else return with same number
#
#  Compute index for the addition of 0.5.
#  
nearcon:  neg.w   %d6             #map into correct range
         add.w   &53,%d6         #1 <= d6 <= 52  (so can add in a 1)
         move.w  %d6,%d5          #save for later clear of mantissa bits
         subq.w  &1,%d6          #number of left shifts for the mask
         moveq   &1,%d7          #mask for the add
#
#  Add 0.5 (in magnitude) to the number to be rounded.
#
         cmp.w   %d6,&32         #see if add to d0 or d1
         bge.w     add0           #branch if add to d0
            lsl.l   %d6,%d7          #shift over correct number of places
            add.l   %d7,%d1
            bcc.w     fin_r2         #no need to check for overflow
               addq.l  &1,%d0          #propagate carry
fin_r2:	       bra.w     clearbit       #if overflow, exponent adjusted!
#
add0:    sub.w   &32,%d6          #get the correct mask
        lsl.l   %d6,%d7
        add.l   %d7,%d0           #do add - oveflow goes into mantissa
        bra.w     clearbit  	#clear the correct bits
