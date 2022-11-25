ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_unsupp.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:57:16 $
                                    
#
#	x_unsupp.sa 3.3 7/1/91
#
#	fpsp_unsupp --- FPSP handler for unsupported data type exception
#
# Trap vector #55	(See table 8-1 Mc68030 User's manual).	
# Invoked when the user program encounters a data format (packed) that
# hardware does not support or a data type (denormalized numbers or un-
# normalized numbers).
# Normalizes denorms and unnorms, unpacks packed numbers then stores 
# them back into the machine to let the 040 finish the operation.  
#
# Unsupp calls two routines:
# 	1. get_op -  gets the operand(s)
# 	2. res_func - restore the function back into the 040 or
# 			if fmove.p fpm,<ea> then pack source (fpm)
# 			and store in users memory <ea>.
#
#  Input: Long fsave stack frame
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
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_unsupp      
fpsp_unsupp:                            
#
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
                                    
         move.b    (%a7),ver_tmp(%a6) # save version number
         move.b    (%a7),%d0        # test for valid version num
         andi.b    &0xf0,%d0        # test for $4x
         cmpi.b    %d0,&ver_4       # must be $4x or exit
         bne.l     fpsp_fmt_error   
                                    
         fmove.l   &0,%fpsr         # clear all user status bits
         fmove.l   &0,%fpcr         # clear all user control bits
#
#	The following lines are used to ensure that the FPSR
#	exception byte and condition codes are clear before proceeding,
#	except in the case of fmove, which leaves the cc's intact.
#
unsupp_con:                            
         move.l    user_fpsr(%a6),%d1 
         btst      &5,cmdreg1b(%a6) # looking for fmove out
         bne       fmove_con        
         and.l     &0xff00ff,%d1    # clear all but aexcs and qbyte
         bra.b     end_fix          
fmove_con:                            
         and.l     &0x0fff40ff,%d1  # clear all but cc's, snan bit, aexcs, and qbyte
end_fix:                            
         move.l    %d1,user_fpsr(%a6) 
                                    
         st        uflg_tmp(%a6)    # set flag for unsupp data
                                    
         bsr.l     get_op           # everything okay, go get operand(s)
         bsr.l     res_func         # fix up stack frame so can restore it
         clr.l     -(%a7)           
         move.b    ver_tmp(%a6),(%a7) # move idle fmt word to top of stack
         bra.l     gen_except       
#
                                    
	version 3
