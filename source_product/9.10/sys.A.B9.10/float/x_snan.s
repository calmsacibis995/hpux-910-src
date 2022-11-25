ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_snan.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:56:16 $
                                    
#
#	x_snan.sa 3.3 7/1/91
#
# fpsp_snan --- FPSP handler for signalling NAN exception
#
# SNAN for float -> integer conversions (integer conversion of
# an SNAN) is a non-maskable run-time exception.
#
# For trap disabled the 040 does the following:
# If the dest data format is s, d, or x, then the SNAN bit in the NAN
# is set to one and the resulting non-signaling NAN (truncated if
# necessary) is transferred to the dest.  If the dest format is b, w,
# or l, then garbage is written to the dest (actually the upper 32 bits
# of the mantissa are sent to the integer unit).
#
# For trap enabled the 040 does the following:
# If the inst is move_out, then the results are the same as for trap 
# disabled with the exception posted.  If the instruction is not move_
# out, the dest. is not modified, and the exception is posted.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_snan        
fpsp_snan:                            
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
#
# Check if trap enabled
#
         btst.b    &snan_bit,fpcr_enable(%a6) 
         bne.b     ena              # If enabled, then branch
                                    
         bsr.l     move_out         # else SNAN disabled
#
# It is possible to have an inex1 exception with the
# snan.  If the inex enable bit is set in the FPCR, and either
# inex2 or inex1 occured, we must clean up and branch to the
# real inex handler.
#
ck_inex:                            
         move.b    fpcr_enable(%a6),%d0 
         and.b     fpsr_except(%a6),%d0 
         andi.b    &0x3,%d0         
         beq.w     end_snan         
#
# Inexact enabled and reported, and we must take an inexact exception.
#
take_inex:                            
         move.b    &inex_vec,exc_vec+1(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_inex        
#
# SNAN is enabled.  Check if inst is move_out.
# Make any corrections to the 040 output as necessary.
#
ena:                                
         btst.b    &5,cmdreg1b(%a6) # if set, inst is move out
         beq.w     not_out          
                                    
         bsr.l     move_out         
                                    
report_snan:                            
         move.b    (%a7),ver_tmp(%a6) 
         cmpi.b    (%a7),&ver_40    # test for orig unimp frame
         bne.b     ck_rev           
         moveq.l   &13,%d0          # need to zero 14 lwords
         bra.b     rep_con          
ck_rev:                             
         moveq.l   &11,%d0          # need to zero 12 lwords
rep_con:                            
         clr.l     (%a7)            
loop1:                              
         clr.l     -(%a7)           # clear and dec a7
         dbra.w    %d0,loop1        
         move.b    ver_tmp(%a6),(%a7) # format a busy frame
         move.b    &busy_size-4,1(%a7) 
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_snan        
#
# Exit snan handler by expanding the unimp frame into a busy frame
#
end_snan:                            
         bclr.b    &e1,e_byte(%a6)  
                                    
         move.b    (%a7),ver_tmp(%a6) 
         cmpi.b    (%a7),&ver_40    # test for orig unimp frame
         bne.b     ck_rev2          
         moveq.l   &13,%d0          # need to zero 14 lwords
         bra.b     rep_con2         
ck_rev2:                            
         moveq.l   &11,%d0          # need to zero 12 lwords
rep_con2:                            
         clr.l     (%a7)            
loop2:                              
         clr.l     -(%a7)           # clear and dec a7
         dbra.w    %d0,loop2        
         move.b    ver_tmp(%a6),(%a7) # format a busy frame
         move.b    &busy_size-4,1(%a7) # write busy size
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     fpsp_done        
                                    
#
# Move_out 
#
move_out:                            
         move.l    exc_ea(%a6),%a0  # get <ea> from exc frame
                                    
         bfextu    cmdreg1b(%a6){&3:&3},%d0 # move rx field to d0{2:0}
         cmpi.l    %d0,&0           # check for long
         beq.b     sto_long         # branch if move_out long
                                    
         cmpi.l    %d0,&4           # check for word
         beq.b     sto_word         # branch if move_out word
                                    
         cmpi.l    %d0,&6           # check for byte
         beq.b     sto_byte         # branch if move_out byte
                                    
#
# Not byte, word or long
#
         rts                        
#	
# Get the 32 most significant bits of etemp mantissa
#
sto_long:                            
         move.l    etemp_hi(%a6),%d1 
         move.l    &4,%d0           # load byte count
#
# Set signalling nan bit
#
         bset.l    &30,%d1          
#
# Store to the users destination address
#
         tst.l     %a0              # check if <ea> is 0
         beq.b     wrt_dn           # destination is a data register
                                    
         move.l    %d1,-(%a7)       # move the snan onto the stack
         move.l    %a0,%a1          # load dest addr into a1
         move.l    %a7,%a0          # load src addr of snan into a0
         bsr.l     mem_write        # write snan to user memory
         move.l    (%a7)+,%d1       # clear off stack
         rts                        
#
# Get the 16 most significant bits of etemp mantissa
#
sto_word:                            
         move.l    etemp_hi(%a6),%d1 
         move.l    &2,%d0           # load byte count
#
# Set signalling nan bit
#
         bset.l    &30,%d1          
#
# Store to the users destination address
#
         tst.l     %a0              # check if <ea> is 0
         beq.b     wrt_dn           # destination is a data register
                                    
         move.l    %d1,-(%a7)       # move the snan onto the stack
         move.l    %a0,%a1          # load dest addr into a1
         move.l    %a7,%a0          # point to low word
         bsr.l     mem_write        # write snan to user memory
         move.l    (%a7)+,%d1       # clear off stack
         rts                        
#
# Get the 8 most significant bits of etemp mantissa
#
sto_byte:                            
         move.l    etemp_hi(%a6),%d1 
         move.l    &1,%d0           # load byte count
#
# Set signalling nan bit
#
         bset.l    &30,%d1          
#
# Store to the users destination address
#
         tst.l     %a0              # check if <ea> is 0
         beq.b     wrt_dn           # destination is a data register
         move.l    %d1,-(%a7)       # move the snan onto the stack
         move.l    %a0,%a1          # load dest addr into a1
         move.l    %a7,%a0          # point to source byte
         bsr.l     mem_write        # write snan to user memory
         move.l    (%a7)+,%d1       # clear off stack
         rts                        
                                    
#
#	wrt_dn --- write to a data register
#
#	We get here with D1 containing the data to write and D0 the
#	number of bytes to write: 1=byte,2=word,4=long.
#
wrt_dn:                             
         move.l    %d1,l_scr1(%a6)  # data
         move.l    %d0,-(%a7)       # size
         bsr.l     get_fline        # returns fline word in d0
         move.l    %d0,%d1          
         andi.l    &0x7,%d1         # d1 now holds register number
         move.l    (%sp)+,%d0       # get original size
         cmpi.l    %d0,&4           
         beq.b     wrt_long         
         cmpi.l    %d0,&2           
         bne.b     wrt_byte         
wrt_word:                            
         or.l      &0x8,%d1         
         bra.l     reg_dest         
wrt_long:                            
         or.l      &0x10,%d1        
         bra.l     reg_dest         
wrt_byte:                            
         bra.l     reg_dest         
#
# Check if it is a src nan or dst nan
#
not_out:                            
         move.l    dtag(%a6),%d0    
         bfextu    %d0{&0:&3},%d0   # isolate dtag in lsbs
                                    
         cmpi.b    %d0,&3           # check for nan in destination
         bne.b     issrc            # destination nan has priority
dst_nan:                            
         btst.b    &6,fptemp_hi(%a6) # check if dest nan is an snan
         bne.b     issrc            # no, so check source for snan
         move.w    fptemp_ex(%a6),%d0 
         bra.b     cont             
issrc:                              
         move.w    etemp_ex(%a6),%d0 
cont:                               
         btst.l    &15,%d0          # test for sign of snan
         beq.b     clr_neg          
         bset.b    &neg_bit,fpsr_cc(%a6) 
         bra.w     report_snan      
clr_neg:                            
         bclr.b    &neg_bit,fpsr_cc(%a6) 
         bra.w     report_snan      
                                    
                                    
	version 3
