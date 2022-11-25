ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_operr.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:55:48 $
                                    
#
#	x_operr.sa 3.5 7/1/91
#
#	fpsp_operr --- FPSP handler for operand error exception
#
#	See 68040 User's Manual pp. 9-44f
#
# Note 1: For trap disabled 040 does the following:
# If the dest is a fp reg, then an extended precision non_signaling
# NAN is stored in the dest reg.  If the dest format is b, w, or l and
# the source op is a NAN, then garbage is stored as the result (actually
# the upper 32 bits of the mantissa are sent to the integer unit). If
# the dest format is integer (b, w, l) and the operr is caused by
# integer overflow, or the source op is inf, then the result stored is
# garbage.
# There are three cases in which operr is incorrectly signaled on the 
# 040.  This occurs for move_out of format b, w, or l for the largest 
# negative integer (-2^7 for b, -2^15 for w, -2^31 for l).
#
#	  On opclass = 011 fmove.(b,w,l) that causes a conversion
#	  overflow -> OPERR, the exponent in wbte (and fpte) is:
#		byte    56 - (62 - exp)
#		word    48 - (62 - exp)
#		long    32 - (62 - exp)
#
#			where exp = (true exp) - 1
#
#  So, wbtemp and fptemp will contain the following on erroneoulsy
#	  signalled operr:
#			fpts = 1
#			fpte = $4000  (15 bit externally)
#		byte	fptm = $ffffffff ffffff80
#		word	fptm = $ffffffff ffff8000
#		long	fptm = $ffffffff 80000000
#
# Note 2: For trap enabled 040 does the following:
# If the inst is move_out, then same as Note 1.
# If the inst is not move_out, the dest is not modified.
# The exceptional operand is not defined for integer overflow 
# during a move_out.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_operr       
fpsp_operr:                            
#
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
#
# Check if this is an opclass 3 instruction.
#  If so, fall through, else branch to operr_end
#
         btst.b    &tflag,t_byte(%a6) 
         beq.b     operr_end        
                                    
#
# If the destination size is B,W,or L, the operr must be 
# handled here.
#
         move.l    cmdreg1b(%a6),%d0 
         bfextu    %d0{&3:&3},%d0   # 0=long, 4=word, 6=byte
         cmpi.b    %d0,&0           #  check long
         beq.w     operr_long       
         cmpi.b    %d0,&4           # check word
         beq.w     operr_word       
         cmpi.b    %d0,&6           # check byte
         beq.w     operr_byte       
                                    
#
# The size is not B,W,or L, so the operr is handled by the 
# kernel handler.  Set the operr bits and clean up, leaving
# only the integer exception frame on the stack, and the 
# fpu in the original exceptional state.
#
operr_end:                            
         bset.b    &operr_bit,fpsr_except(%a6) 
         bset.b    &aiop_bit,fpsr_aexcept(%a6) 
                                    
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_operr       
                                    
operr_long:                            
         moveq.l   &4,%d1           # write size to d1
         move.b    stag(%a6),%d0    # test stag for nan
         andi.b    &0xe0,%d0        # clr all but tag
         cmpi.b    %d0,&0x60        # check for nan
         beq       operr_nan        
         cmpi.l    fptemp_lo(%a6),&0x80000000 # test if ls lword is special
         bne.b     chklerr          # if not equal, check for incorrect operr
         bsr       check_upper      # check if exp and ms mant are special
         tst.l     %d0              
         bne.b     chklerr          # if d0 is true, check for incorrect operr
         move.l    &0x80000000,%d0  # store special case result
         bsr       operr_store      
         bra.w     not_enabled      # clean and exit
#
#	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
#
chklerr:                            
         move.w    fptemp_ex(%a6),%d0 
         and.w     &0x7fff,%d0      # ignore sign bit
         cmp.w     %d0,&0x3ffe      # this is the only possible exponent value
         bne.b     chklerr2         
fixlong:                            
         move.l    fptemp_lo(%a6),%d0 
         bsr       operr_store      
         bra.w     not_enabled      
chklerr2:                            
         move.w    fptemp_ex(%a6),%d0 
         and.w     &0x7fff,%d0      # ignore sign bit
         cmp.w     %d0,&0x4000      
         bhs.w     store_max        # exponent out of range
                                    
         move.l    fptemp_lo(%a6),%d0 
         and.l     &0x7fff0000,%d0  # look for all 1's on bits 30-16
         cmp.l     %d0,&0x7fff0000  
         beq.b     fixlong          
                                    
         tst.l     fptemp_lo(%a6)   
         bpl.b     chklepos         
         cmp.l     fptemp_hi(%a6),&0xffffffff 
         beq.b     fixlong          
         bra.w     store_max        
chklepos:                            
         tst.l     fptemp_hi(%a6)   
         beq.b     fixlong          
         bra.w     store_max        
                                    
operr_word:                            
         moveq.l   &2,%d1           # write size to d1
         move.b    stag(%a6),%d0    # test stag for nan
         andi.b    &0xe0,%d0        # clr all but tag
         cmpi.b    %d0,&0x60        # check for nan
         beq.w     operr_nan        
         cmpi.l    fptemp_lo(%a6),&0xffff8000 # test if ls lword is special
         bne.b     chkwerr          # if not equal, check for incorrect operr
         bsr       check_upper      # check if exp and ms mant are special
         tst.l     %d0              
         bne.b     chkwerr          # if d0 is true, check for incorrect operr
         move.l    &0x80000000,%d0  # store special case result
         bsr       operr_store      
         bra.w     not_enabled      # clean and exit
#
#	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
#
chkwerr:                            
         move.w    fptemp_ex(%a6),%d0 
         and.w     &0x7fff,%d0      # ignore sign bit
         cmp.w     %d0,&0x3ffe      # this is the only possible exponent value
         bne.b     store_max        
         move.l    fptemp_lo(%a6),%d0 
         swap      %d0              
         bsr       operr_store      
         bra.w     not_enabled      
                                    
operr_byte:                            
         moveq.l   &1,%d1           # write size to d1
         move.b    stag(%a6),%d0    # test stag for nan
         andi.b    &0xe0,%d0        # clr all but tag
         cmpi.b    %d0,&0x60        # check for nan
         beq.b     operr_nan        
         cmpi.l    fptemp_lo(%a6),&0xffffff80 # test if ls lword is special
         bne.b     chkberr          # if not equal, check for incorrect operr
         bsr       check_upper      # check if exp and ms mant are special
         tst.l     %d0              
         bne.b     chkberr          # if d0 is true, check for incorrect operr
         move.l    &0x80000000,%d0  # store special case result
         bsr       operr_store      
         bra.w     not_enabled      # clean and exit
#
#	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
#
chkberr:                            
         move.w    fptemp_ex(%a6),%d0 
         and.w     &0x7fff,%d0      # ignore sign bit
         cmp.w     %d0,&0x3ffe      # this is the only possible exponent value
         bne.b     store_max        
         move.l    fptemp_lo(%a6),%d0 
         asl.l     &8,%d0           
         swap      %d0              
         bsr       operr_store      
         bra.w     not_enabled      
                                    
#
# This operr condition is not of the special case.  Set operr
# and aiop and write the portion of the nan to memory for the
# given size.
#
operr_nan:                            
         or.l      &opaop_mask,user_fpsr(%a6) # set operr & aiop
                                    
         move.l    etemp_hi(%a6),%d0 # output will be from upper 32 bits
         bsr       operr_store      
         bra       end_operr        
#
# Store_max loads the max pos or negative for the size, sets
# the operr and aiop bits, and clears inex and ainex, incorrectly
# set by the 040.
#
store_max:                            
         or.l      &opaop_mask,user_fpsr(%a6) # set operr & aiop
         bclr.b    &inex2_bit,fpsr_except(%a6) 
         bclr.b    &ainex_bit,fpsr_aexcept(%a6) 
         fmove.l   &0,%fpsr         
                                    
         tst.w     fptemp_ex(%a6)   # check sign
         blt.b     load_neg         
         move.l    &0x7fffffff,%d0  
         bsr       operr_store      
         bra       end_operr        
load_neg:                            
         move.l    &0x80000000,%d0  
         bsr       operr_store      
         bra       end_operr        
                                    
#
# This routine stores the data in d0, for the given size in d1,
# to memory or data register as required.  A read of the fline
# is required to determine the destination.
#
operr_store:                            
         move.l    %d0,l_scr1(%a6)  # move write data to L_SCR1
         move.l    %d1,-(%a7)       # save register size
         bsr.l     get_fline        # fline returned in d0
         move.l    (%a7)+,%d1       
         bftst     %d0{&26:&3}      # if mode is zero, dest is Dn
         bne.b     dest_mem         
#
# Destination is Dn.  Get register number from d0. Data is on
# the stack at (a7). D1 has size: 1=byte,2=word,4=long/single
#
         andi.l    &7,%d0           # isolate register number
         cmpi.l    %d1,&4           
         beq.b     op_long          # the most frequent case
         cmpi.l    %d1,&2           
         bne.b     op_con           
         or.l      &8,%d0           
         bra.b     op_con           
op_long:                            
         or.l      &0x10,%d0        
op_con:                             
         move.l    %d0,%d1          # format size:reg for reg_dest
         bra.l     reg_dest         # call to reg_dest returns to caller
#				;of operr_store
#
# Destination is memory.  Get <ea> from integer exception frame
# and call mem_write.
#
dest_mem:                            
         lea.l     l_scr1(%a6),%a0  # put ptr to write data in a0
         move.l    exc_ea(%a6),%a1  # put user destination address in a1
         move.l    %d1,%d0          # put size in d0
         bsr.l     mem_write        
         rts                        
#
# Check the exponent for $c000 and the upper 32 bits of the 
# mantissa for $ffffffff.  If both are true, return d0 clr
# and store the lower n bits of the least lword of FPTEMP
# to d0 for write out.  If not, it is a real operr, and set d0.
#
check_upper:                            
         cmpi.l    fptemp_hi(%a6),&0xffffffff # check if first byte is all 1's
         bne.b     true_operr       # if not all 1's then was true operr
         cmpi.w    fptemp_ex(%a6),&0xc000 # check if incorrectly signalled
         beq.b     not_true_operr   # branch if not true operr
         cmpi.w    fptemp_ex(%a6),&0xbfff # check if incorrectly signalled
         beq.b     not_true_operr   # branch if not true operr
true_operr:                            
         move.l    &1,%d0           # signal real operr
         rts                        
not_true_operr:                            
         clr.l     %d0              # signal no real operr
         rts                        
                                    
#
# End_operr tests for operr enabled.  If not, it cleans up the stack
# and does an rte.  If enabled, it cleans up the stack and branches
# to the kernel operr handler with only the integer exception
# frame on the stack and the fpu in the original exceptional state
# with correct data written to the destination.
#
end_operr:                            
         btst.b    &operr_bit,fpcr_enable(%a6) 
         beq.b     not_enabled      
enabled:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_operr       
                                    
not_enabled:                            
#
# It is possible to have either inex2 or inex1 exceptions with the
# operr.  If the inex enable bit is set in the FPCR, and either
# inex2 or inex1 occured, we must clean up and branch to the
# real inex handler.
#
ck_inex:                            
         move.b    fpcr_enable(%a6),%d0 
         and.b     fpsr_except(%a6),%d0 
         andi.b    &0x3,%d0         
         beq.w     operr_exit       
#
# Inexact enabled and reported, and we must take an inexact exception.
#
take_inex:                            
         move.b    &inex_vec,exc_vec+1(%a6) 
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_inex        
#
# Since operr is only an E1 exception, there is no need to frestore
# any state back to the fpu.
#
operr_exit:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         unlk      %a6              
         bra.l     fpsp_done        
                                    
                                    
	version 3
