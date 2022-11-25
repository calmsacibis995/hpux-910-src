ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_bsun.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:55:22 $
                                    
#
#	x_bsun.sa 3.3 7/1/91
#
#	fpsp_bsun --- FPSP handler for branch/set on unordered exception
#
#	Copy the PC to FPIAR to maintain 881/882 compatability
#
#	The real_bsun handler will need to perform further corrective
#	measures as outlined in the 040 User's Manual on pages
#	9-41f, section 9.8.3.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
         global    fpsp_bsun        
fpsp_bsun:                            
#
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
#
         move.l    exc_pc(%a6),user_fpiar(%a6) 
#
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_bsun        
#
                                    
	version 3
