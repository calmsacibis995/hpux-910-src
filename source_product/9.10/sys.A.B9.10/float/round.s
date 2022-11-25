ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/round.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:49:45 $
                                    
#
#	round.sa 3.4 7/29/91
#
#	handle rounding and normalization tasks
#
#
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
#	round --- round result according to precision/mode
#
#	a0 points to the input operand in the internal extended format 
#	d1(high word) contains rounding precision:
#		ext = $0000xxxx
#		sgl = $0001xxxx
#		dbl = $0002xxxx
#	d1(low word) contains rounding mode:
#		RN  = $xxxx0000
#		RZ  = $xxxx0001
#		RM  = $xxxx0010
#		RP  = $xxxx0011
#	d0{31:29} contains the g,r,s bits (extended)
#
#	On return the value pointed to by a0 is correctly rounded,
#	a0 is preserved and the g-r-s bits in d0 are cleared.
#	The result is not typed - the tag field is invalid.  The
#	result is still in the internal extended format.
#
#	The INEX bit of USER_FPSR will be set if the rounded result was
#	inexact (i.e. if any of the g-r-s bits were set).
#
                                    
         global    round            
round:                              
# If g=r=s=0 then result is exact and round is done, else set 
# the inex flag in status reg and continue.  
#
         bsr.b     ext_grs          # this subroutine looks at the 
#					:rounding precision and sets 
#					;the appropriate g-r-s bits.
         tst.l     %d0              # if grs are zero, go force
         bne.w     rnd_cont         # lower bits to zero for size
                                    
         swap      %d1              # set up d1.w for round prec.
         bra.w     truncate         
                                    
rnd_cont:                            
#
# Use rounding mode as an index into a jump table for these modes.
#
         or.l      &inx2a_mask,user_fpsr(%a6) # set inex2/ainex
         lea       mode_tab,%a1     
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
#
# Jump table indexed by rounding mode in d1.w.  All following assumes
# grs != 0.
#
mode_tab:                            
         long      rnd_near         
         long      rnd_zero         
         long      rnd_mnus         
         long      rnd_plus         
#
#	ROUND PLUS INFINITY
#
#	If sign of fp number = 0 (positive), then add 1 to l.
#
rnd_plus:                            
         swap      %d1              # set up d1 for round prec.
         tst.b     local_sgn(%a0)   # check for sign
         bmi.w     truncate         # if positive then truncate
         move.l    &0xffffffff,%d0  # force g,r,s to be all f's
         lea       add_to_l,%a1     
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
#
#	ROUND MINUS INFINITY
#
#	If sign of fp number = 1 (negative), then add 1 to l.
#
rnd_mnus:                            
         swap      %d1              # set up d1 for round prec.
         tst.b     local_sgn(%a0)   # check for sign	
         bpl.w     truncate         # if negative then truncate
         move.l    &0xffffffff,%d0  # force g,r,s to be all f's
         lea       add_to_l,%a1     
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
#
#	ROUND ZERO
#
#	Always truncate.
rnd_zero:                            
         swap      %d1              # set up d1 for round prec.
         bra.w     truncate         
#
#
#	ROUND NEAREST
#
#	If (g=1), then add 1 to l and if (r=s=0), then clear l
#	Note that this will round to even in case of a tie.
#
rnd_near:                            
         swap      %d1              # set up d1 for round prec.
         asl.l     &1,%d0           # shift g-bit to c-bit
         bcc.w     truncate         # if (g=1) then
         lea       add_to_l,%a1     
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
                                    
#
#	ext_grs --- extract guard, round and sticky bits
#
# Input:	d1 =		PREC:ROUND
# Output:  	d0{31:29}=	guard, round, sticky
#
# The ext_grs extract the guard/round/sticky bits according to the
# selected rounding precision. It is called by the round subroutine
# only.  All registers except d0 are kept intact. d0 becomes an 
# updated guard,round,sticky in d0{31:29}
#
# Notes: the ext_grs uses the round PREC, and therefore has to swap d1
#	 prior to usage, and needs to restore d1 to original.
#
ext_grs:                            
         swap      %d1              # have d1.w point to round precision
         cmpi.w    %d1,&0           
         bne.b     sgl_or_dbl       
         bra.b     end_ext_grs      
                                    
sgl_or_dbl:                            
         movem.l   %d2/%d3,-(%a7)   # make some temp registers
         cmpi.w    %d1,&1           
         bne.b     grs_dbl          
grs_sgl:                            
         bfextu    local_hi(%a0){&24:&2},%d3 # sgl prec. g-r are 2 bits right
         move.l    &30,%d2          # of the sgl prec. limits
         lsl.l     %d2,%d3          # shift g-r bits to MSB of d3
         move.l    local_hi(%a0),%d2 # get word 2 for s-bit test
         andi.l    &0x0000003f,%d2  # s bit is the or of all other 
         bne.b     st_stky          # bits to the right of g-r
         tst.l     local_lo(%a0)    # test lower mantissa
         bne.b     st_stky          # if any are set, set sticky
         tst.l     %d0              # test original g,r,s
         bne.b     st_stky          # if any are set, set sticky
         bra.b     end_sd           # if words 3 and 4 are clr, exit
grs_dbl:                            
         bfextu    local_lo(%a0){&21:&2},%d3 # dbl-prec. g-r are 2 bits right
         move.l    &30,%d2          # of the dbl prec. limits
         lsl.l     %d2,%d3          # shift g-r bits to the MSB of d3
         move.l    local_lo(%a0),%d2 # get lower mantissa  for s-bit test
         andi.l    &0x000001ff,%d2  # s bit is the or-ing of all 
         bne.b     st_stky          # other bits to the right of g-r
         tst.l     %d0              # test word original g,r,s
         bne.b     st_stky          # if any are set, set sticky
         bra.b     end_sd           # if clear, exit
st_stky:                            
         bset      &rnd_stky_bit,%d3 
end_sd:                             
         move.l    %d3,%d0          # return grs to d0
         movem.l   (%a7)+,%d2/%d3   # restore scratch registers
end_ext_grs:                            
         swap      %d1              # restore d1 to original
         rts                        
                                    
#*******************  Local Equates
         set       ad_1_sgl,0x00000100 
         set       ad_1_dbl,0x00000800 
                                    
                                    
#Jump table for adding 1 to the l-bit indexed by rnd prec
                                    
add_to_l:                            
         long      add_ext          
         long      add_sgl          
         long      add_dbl          
         long      add_dbl          
#
#	ADD SINGLE
#
add_sgl:                            
         add.l     &ad_1_sgl,local_hi(%a0) 
         bcc.b     scc_clr          # no mantissa overflow
         roxr.w    local_hi(%a0)    # shift v-bit back in
         roxr.w    local_hi+2(%a0)  # shift v-bit back in
         add.w     &0x1,local_ex(%a0) # and incr exponent
scc_clr:                            
         tst.l     %d0              # test for rs = 0
         bne.b     sgl_done         
         andi.w    &0xfe00,local_hi+2(%a0) # clear the l-bit
sgl_done:                            
         andi.l    &0xffffff00,local_hi(%a0) # truncate bits beyond sgl limit
         clr.l     local_lo(%a0)    # clear d2
         rts                        
                                    
#
#	ADD EXTENDED
#
add_ext:                            
         addq.l    &1,local_lo(%a0) # add 1 to l-bit
         bcc.b     xcc_clr          # test for carry out
         addq.l    &1,local_hi(%a0) # propogate carry
         bcc.b     xcc_clr          
         roxr.w    local_hi(%a0)    # mant is 0 so restore v-bit
         roxr.w    local_hi+2(%a0)  # mant is 0 so restore v-bit
         roxr.w    local_lo(%a0)    
         roxr.w    local_lo+2(%a0)  
         add.w     &0x1,local_ex(%a0) # and inc exp
xcc_clr:                            
         tst.l     %d0              # test rs = 0
         bne.b     add_ext_done     
         andi.b    &0xfe,local_lo+3(%a0) # clear the l bit
add_ext_done:                            
         rts                        
#
#	ADD DOUBLE
#
add_dbl:                            
         add.l     &ad_1_dbl,local_lo(%a0) 
         bcc.b     dcc_clr          
         addq.l    &1,local_hi(%a0) # propogate carry
         bcc.b     dcc_clr          
         roxr.w    local_hi(%a0)    # mant is 0 so restore v-bit
         roxr.w    local_hi+2(%a0)  # mant is 0 so restore v-bit
         roxr.w    local_lo(%a0)    
         roxr.w    local_lo+2(%a0)  
         add.w     &0x1,local_ex(%a0) # incr exponent
dcc_clr:                            
         tst.l     %d0              # test for rs = 0
         bne.b     dbl_done         
         andi.w    &0xf000,local_lo+2(%a0) # clear the l-bit
                                    
dbl_done:                            
         andi.l    &0xfffff800,local_lo(%a0) # truncate bits beyond dbl limit
         rts                        
                                    
error:                              
         rts                        
#
# Truncate all other bits
#
trunct:                             
         long      end_rnd          
         long      sgl_done         
         long      dbl_done         
         long      dbl_done         
                                    
truncate:                            
         lea       trunct,%a1       
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
                                    
end_rnd:                            
         rts                        
                                    
#
#	NORMALIZE
#
# These routines (nrm_zero & nrm_set) normalize the unnorm.  This 
# is done by shifting the mantissa left while decrementing the 
# exponent.
#
# NRM_SET shifts and decrements until there is a 1 set in the integer 
# bit of the mantissa (msb in d1).
#
# NRM_ZERO shifts and decrements until there is a 1 set in the integer 
# bit of the mantissa (msb in d1) unless this would mean the exponent 
# would go less than 0.  In that case the number becomes a denorm - the 
# exponent (d0) is set to 0 and the mantissa (d1 & d2) is not 
# normalized.
#
# Note that both routines have been optimized (for the worst case) and 
# therefore do not have the easy to follow decrement/shift loop.
#
#	NRM_ZERO
#
#	Distance to first 1 bit in mantissa = X
#	Distance to 0 from exponent = Y
#	If X < Y
#	Then
#	  nrm_set
#	Else
#	  shift mantissa by Y
#	  set exponent = 0
#
#input:
#	FP_SCR1 = exponent, ms mantissa part, ls mantissa part
#output:
#	L_SCR1{4} = fpte15 or ete15 bit
#
         global    nrm_zero         
nrm_zero:                            
         move.w    local_ex(%a0),%d0 
         cmp.w     %d0,&64          # see if exp > 64 
         bmi.b     d0_less          
         bsr       nrm_set          # exp > 64 so exp won't exceed 0 
         rts                        
d0_less:                            
         movem.l   %d2/%d3/%d5/%d6,-(%a7) 
         move.l    local_hi(%a0),%d1 
         move.l    local_lo(%a0),%d2 
                                    
         bfffo     %d1{&0:&32},%d3  # get the distance to the first 1 
#				;in ms mant
         beq.b     ms_clr           # branch if no bits were set
         cmp.w     %d0,%d3          # of X>Y
         bmi.b     greater          # then exp will go past 0 (neg) if 
#				;it is just shifted
         bsr       nrm_set          # else exp won't go past 0
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
greater:                            
         move.l    %d2,%d6          # save ls mant in d6
         lsl.l     %d0,%d2          # shift ls mant by count
         lsl.l     %d0,%d1          # shift ms mant by count
         move.l    &32,%d5          
         sub.l     %d0,%d5          # make op a denorm by shifting bits 
         lsr.l     %d5,%d6          # by the number in the exp, then 
#				;set exp = 0.
         or.l      %d6,%d1          # shift the ls mant bits into the ms mant
         move.l    &0,%d0           # same as if decremented exp to 0 
#				;while shifting
         move.w    %d0,local_ex(%a0) 
         move.l    %d1,local_hi(%a0) 
         move.l    %d2,local_lo(%a0) 
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
ms_clr:                             
         bfffo     %d2{&0:&32},%d3  # check if any bits set in ls mant
         beq.b     all_clr          # branch if none set
         add.w     &32,%d3          
         cmp.w     %d0,%d3          # if X>Y
         bmi.b     greater          # then branch
         bsr       nrm_set          # else exp won't go past 0
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
all_clr:                            
         move.w    &0,local_ex(%a0) # no mantissa bits set. Set exp = 0.
         movem.l   (%a7)+,%d2/%d3/%d5/%d6 
         rts                        
#
#	NRM_SET
#
         global    nrm_set          
nrm_set:                            
         move.l    %d7,-(%a7)       
         bfffo     local_hi(%a0){&0:&32},%d7 # find first 1 in ms mant to d7)
         beq.b     lower            # branch if ms mant is all 0's
                                    
         move.l    %d6,-(%a7)       
                                    
         sub.w     %d7,local_ex(%a0) # sub exponent by count
         move.l    local_hi(%a0),%d0 # d0 has ms mant
         move.l    local_lo(%a0),%d1 # d1 has ls mant
                                    
         lsl.l     %d7,%d0          # shift first 1 to j bit position
         move.l    %d1,%d6          # copy ls mant into d6
         lsl.l     %d7,%d6          # shift ls mant by count
         move.l    %d6,local_lo(%a0) # store ls mant into memory
         moveq.l   &32,%d6          
         sub.l     %d7,%d6          # continue shift
         lsr.l     %d6,%d1          # shift off all bits but those that will
#				;be shifted into ms mant
         or.l      %d1,%d0          # shift the ls mant bits into the ms mant
         move.l    %d0,local_hi(%a0) # store ms mant into memory
         movem.l   (%a7)+,%d7/%d6   # restore registers
         rts                        
                                    
#
# We get here if ms mant was = 0, and we assume ls mant has bits 
# set (otherwise this would have been tagged a zero not a denorm).
#
lower:                              
         move.w    local_ex(%a0),%d0 # d0 has exponent
         move.l    local_lo(%a0),%d1 # d1 has ls mant
         sub.w     &32,%d0          # account for ms mant being all zeros
         bfffo     %d1{&0:&32},%d7  # find first 1 in ls mant to d7)
         sub.w     %d7,%d0          # subtract shift count from exp
         lsl.l     %d7,%d1          # shift first 1 to integer bit in ms mant
         move.w    %d0,local_ex(%a0) # store ms mant
         move.l    %d1,local_hi(%a0) # store exp
         clr.l     local_lo(%a0)    # clear ls mant
         move.l    (%a7)+,%d7       
         rts                        
#
#	denorm --- denormalize an intermediate result
#
#	Used by underflow.
#
# Input: 
#	a0	 points to the operand to be denormalized
#		 (in the internal extended format)
#		 
#	d0: 	 rounding precision
# Output:
#	a0	 points to the denormalized result
#		 (in the internal extended format)
#
#	d0 	is guard,round,sticky
#
# d0 comes into this routine with the rounding precision. It 
# is then loaded with the denormalized exponent threshold for the 
# rounding precision.
#
                                    
         global    denorm           
denorm:                             
         btst.b    &6,local_ex(%a0) # check for exponents between $7fff-$4000
         beq.b     no_sgn_ext       
         bset.b    &7,local_ex(%a0) # sign extend if it is so
no_sgn_ext:                            
                                    
         cmpi.b    %d0,&0           # if 0 then extended precision
         bne.b     not_ext          # else branch
                                    
         clr.l     %d1              # load d1 with ext threshold
         clr.l     %d0              # clear the sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check for inex
         beq.w     no_inex          # if clr, no inex
         bra.b     dnrm_inex        # if set, set inex
                                    
not_ext:                            
         cmpi.l    %d0,&1           # if 1 then single precision
         beq.b     load_sgl         # else must be 2, double prec
                                    
load_dbl:                            
         move.w    &dbl_thresh,%d1  # put copy of threshold in d1
         move.l    %d1,%d0          # copy d1 into d0
         sub.w     local_ex(%a0),%d0 # diff = threshold - exp
         cmp.w     %d0,&67          # if diff > 67 (mant + grs bits)
         bpl.b     chk_stky         # then branch (all bits would be 
#				; shifted off in denorm routine)
         clr.l     %d0              # else clear the sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check flag
         beq.b     no_inex          # if clr, no inex
         bra.b     dnrm_inex        # if set, set inex
                                    
load_sgl:                            
         move.w    &sgl_thresh,%d1  # put copy of threshold in d1
         move.l    %d1,%d0          # copy d1 into d0
         sub.w     local_ex(%a0),%d0 # diff = threshold - exp
         cmp.w     %d0,&67          # if diff > 67 (mant + grs bits)
         bpl.b     chk_stky         # then branch (all bits would be 
#				; shifted off in denorm routine)
         clr.l     %d0              # else clear the sticky flag
         bsr       dnrm_lp          # denormalize the number
         tst.b     %d1              # check flag
         beq.b     no_inex          # if clr, no inex
         bra.b     dnrm_inex        # if set, set inex
                                    
chk_stky:                            
         tst.l     local_hi(%a0)    # check for any bits set
         bne.b     set_stky         
         tst.l     local_lo(%a0)    # check for any bits set
         bne.b     set_stky         
         bra.b     clr_mant         
set_stky:                            
         or.l      &inx2a_mask,user_fpsr(%a6) # set inex2/ainex
         move.l    &0x20000000,%d0  # set sticky bit in return value
clr_mant:                            
         move.w    %d1,local_ex(%a0) # load exp with threshold
         move.l    &0,local_hi(%a0) # set d1 = 0 (ms mantissa)
         move.l    &0,local_lo(%a0) # set d2 = 0 (ms mantissa)
         rts                        
dnrm_inex:                            
         or.l      &inx2a_mask,user_fpsr(%a6) # set inex2/ainex
no_inex:                            
         rts                        
                                    
#
#	dnrm_lp --- normalize exponent/mantissa to specified threshhold
#
# Input:
#	a0		points to the operand to be denormalized
#	d0{31:29} 	initial guard,round,sticky
#	d1{15:0}	denormalization threshold
# Output:
#	a0		points to the denormalized operand
#	d0{31:29}	final guard,round,sticky
#	d1.b		inexact flag:  all ones means inexact result
#
# The LOCAL_LO and LOCAL_GRS parts of the value are copied to FP_SCR2
# so that bfext can be used to extract the new low part of the mantissa.
# Dnrm_lp can be called with a0 pointing to ETEMP or WBTEMP and there 
# is no LOCAL_GRS scratch word following it on the fsave frame.
#
         global    dnrm_lp          
dnrm_lp:                            
         move.l    %d2,-(%sp)       # save d2 for temp use
         btst.b    &e3,e_byte(%a6)  # test for type E3 exception
         beq.b     not_e3           # not type E3 exception
         bfextu    wbtemp_grs(%a6){&6:&3},%d2 # extract guard,round, sticky  bit
         move.l    &29,%d0          
         lsl.l     %d0,%d2          # shift g,r,s to their postions
         move.l    %d2,%d0          
not_e3:                             
         move.l    (%sp)+,%d2       # restore d2
         move.l    local_lo(%a0),fp_scr2+local_lo(%a6) 
         move.l    %d0,fp_scr2+local_grs(%a6) 
         move.l    %d1,%d0          # copy the denorm threshold
         sub.w     local_ex(%a0),%d1 # d1 = threshold - uns exponent
         ble.b     no_lp            # d1 <= 0
         cmp.w     %d1,&32          
         blt.b     case_1           # 0 = d1 < 32 
         cmp.w     %d1,&64          
         blt.b     case_2           # 32 <= d1 < 64
         bra.w     case_3           # d1 >= 64
#
# No normalization necessary
#
no_lp:                              
         clr.b     %d1              # set no inex2 reported
         move.l    fp_scr2+local_grs(%a6),%d0 # restore original g,r,s
         rts                        
#
# case (0<d1<32)
#
case_1:                             
         move.l    %d2,-(%sp)       
         move.w    %d0,local_ex(%a0) # exponent = denorm threshold
         move.l    &32,%d0          
         sub.w     %d1,%d0          # d0 = 32 - d1
         bfextu    local_ex(%a0){%d0:&32},%d2 
         bfextu    %d2{%d1:%d0},%d2 # d2 = new LOCAL_HI
         bfextu    local_hi(%a0){%d0:&32},%d1 # d1 = new LOCAL_LO
         bfextu    fp_scr2+local_lo(%a6){%d0:&32},%d0 # d0 = new G,R,S
         move.l    %d2,local_hi(%a0) # store new LOCAL_HI
         move.l    %d1,local_lo(%a0) # store new LOCAL_LO
         clr.b     %d1              
         bftst     %d0{&2:&30}      
         beq.b     c1nstky          
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
c1nstky:                            
         move.l    fp_scr2+local_grs(%a6),%d2 # restore original g,r,s
         andi.l    &0xe0000000,%d2  # clear all but G,R,S
         tst.l     %d2              # test if original G,R,S are clear
         beq.b     grs_clear        
         or.l      &0x20000000,%d0  # set sticky bit in d0
grs_clear:                            
         andi.l    &0xe0000000,%d0  # clear all but G,R,S
         move.l    (%sp)+,%d2       
         rts                        
#
# case (32<=d1<64)
#
case_2:                             
         move.l    %d2,-(%sp)       
         move.w    %d0,local_ex(%a0) # unsigned exponent = threshold
         sub.w     &32,%d1          # d1 now between 0 and 32
         move.l    &32,%d0          
         sub.w     %d1,%d0          # d0 = 32 - d1
         bfextu    local_ex(%a0){%d0:&32},%d2 
         bfextu    %d2{%d1:%d0},%d2 # d2 = new LOCAL_LO
         bfextu    local_hi(%a0){%d0:&32},%d1 # d1 = new G,R,S
         bftst     %d1{&2:&30}      
         bne.b     c2_sstky         # bra if sticky bit to be set
         bftst     fp_scr2+local_lo(%a6){%d0:&32} 
         bne.b     c2_sstky         # bra if sticky bit to be set
         move.l    %d1,%d0          
         clr.b     %d1              
         bra.b     end_c2           
c2_sstky:                            
         move.l    %d1,%d0          
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
end_c2:                             
         clr.l     local_hi(%a0)    # store LOCAL_HI = 0
         move.l    %d2,local_lo(%a0) # store LOCAL_LO
         move.l    fp_scr2+local_grs(%a6),%d2 # restore original g,r,s
         andi.l    &0xe0000000,%d2  # clear all but G,R,S
         tst.l     %d2              # test if original G,R,S are clear
         beq.b     clear_grs        
         or.l      &0x20000000,%d0  # set sticky bit in d0
clear_grs:                            
         andi.l    &0xe0000000,%d0  # get rid of all but G,R,S
         move.l    (%sp)+,%d2       
         rts                        
#
# d1 >= 64 Force the exponent to be the denorm threshold with the
# correct sign.
#
case_3:                             
         move.w    %d0,local_ex(%a0) 
         tst.w     local_sgn(%a0)   
         bge.b     c3con            
c3neg:                              
         or.l      &0x80000000,local_ex(%a0) 
c3con:                              
         cmp.w     %d1,&64          
         beq.b     sixty_four       
         cmp.w     %d1,&65          
         beq.b     sixty_five       
#
# Shift value is out of range.  Set d1 for inex2 flag and
# return a zero with the given threshold.
#
         clr.l     local_hi(%a0)    
         clr.l     local_lo(%a0)    
         move.l    &0x20000000,%d0  
         st.b      %d1              
         rts                        
                                    
sixty_four:                            
         move.l    local_hi(%a0),%d0 
         bfextu    %d0{&2:&30},%d1  
         andi.l    &0xc0000000,%d0  
         bra.b     c3com            
                                    
sixty_five:                            
         move.l    local_hi(%a0),%d0 
         bfextu    %d0{&1:&31},%d1  
         andi.l    &0x80000000,%d0  
         lsr.l     &1,%d0           # shift high bit into R bit
                                    
c3com:                              
         tst.l     %d1              
         bne.b     c3ssticky        
         tst.l     local_lo(%a0)    
         bne.b     c3ssticky        
         tst.b     fp_scr2+local_grs(%a6) 
         bne.b     c3ssticky        
         clr.b     %d1              
         bra.b     c3end            
                                    
c3ssticky:                            
         bset.l    &rnd_stky_bit,%d0 
         st.b      %d1              
c3end:                              
         clr.l     local_hi(%a0)    
         clr.l     local_lo(%a0)    
         rts                        
                                    
                                    
	version 3
