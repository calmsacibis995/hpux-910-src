ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_unimp.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:57:04 $
                                    
#
#	x_unimp.sa 3.3 7/1/91
#
#	fpsp_unimp --- FPSP handler for unimplemented instruction	
#	exception.
#
# Invoked when the user program encounters a floating-point
# op-code that hardware does not support.  Trap vector# 11
# (See table 8-1 MC68030 User's Manual).
#
# 
# Note: An fsave for an unimplemented inst. will create a short
# fsave stack.
#
#  Input: 1. Six word stack frame for unimplemented inst, four word
#            for illegal
#            (See table 8-7 MC68030 User's Manual).
#         2. Unimp (short) fsave state frame created here by fsave
#            instruction.
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
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_unimp       
         global    uni_2            
fpsp_unimp:                            
         link      %a6,&-local_size 
         fsave     -(%a7)           
uni_2:                              
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
         move.b    (%a7),%d0        # test for valid version num
         andi.b    &0xf0,%d0        # test for $4x
         cmpi.b    %d0,&ver_4       # must be $4x or exit
         bne.l     fpsp_fmt_error   
#
#	Temporary D25B Fix
#	The following lines are used to ensure that the FPSR
#	exception byte and condition codes are clear before proceeding
#
         move.l    user_fpsr(%a6),%d0 
         and.l     &0xff,%d0        # clear all but accrued exceptions
         move.l    %d0,user_fpsr(%a6) 
         fmove.l   &0,%fpsr         # clear all user bits
         fmove.l   &0,%fpcr         # clear all user exceptions for FPSP
                                    
         clr.b     uflg_tmp(%a6)    # clr flag for unsupp data
                                    
         bsr.l     get_op           # go get operand(s)
         clr.b     store_flg(%a6)   
         bsr.l     do_func          # do the function
         fsave     -(%a7)           # capture possible exc state
         tst.b     store_flg(%a6)   
         bne.b     no_store         # if STORE_FLG is set, no store
         bsr.l     sto_res          # store the result in user space
no_store:                            
         bra.l     gen_except       # post any exceptions and return
                                    
                                    
	version 3
