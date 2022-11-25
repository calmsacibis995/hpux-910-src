ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/sto_res.s,v $
# $Revision: 1.3.84.3 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 20:54:16 $
                                    
#
#	sto_res.sa 3.1 12/10/90
#
#	Takes the result and puts it in where the user expects it.
#	Library functions return result in fp0.	If fp0 is not the
#	users destination register then fp0 is moved to the the
#	correct floating-point destination register.  fp0 and fp1
#	are then restored to the original contents. 
#
#	Input:	result in fp0,fp1 
#
#		d2 & a0 should be kept unmodified
#
#	Output:	moves the result to the true destination reg or mem
#
#	Modifies: destination floating point register
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
         global    sto_cos          
sto_cos:                            
         bfextu    cmdreg1b(%a6){&13:&3},%d0 # extract cos destination
         cmpi.b    %d0,&3           # check for fp0/fp1 cases
         ble.b     c_fp0123         
         fmovem.x  %fp1,-(%a7)      
         moveq.l   &7,%d1           
         sub.l     %d0,%d1          # d1 = 7- (dest. reg. no.)
         clr.l     %d0              
         bset.l    %d1,%d0          # d0 is dynamic register mask
         fmovem.x  (%a7)+,%d0       
         rts                        
c_fp0123:                            
         cmpi.b    %d0,&0           
         beq.b     c_is_fp0         
         cmpi.b    %d0,&1           
         beq.b     c_is_fp1         
         cmpi.b    %d0,&2           
         beq.b     c_is_fp2         
c_is_fp3:                            
         fmovem.x  %fp1,user_fp3(%a6) 
         rts                        
c_is_fp2:                            
         fmovem.x  %fp1,user_fp2(%a6) 
         rts                        
c_is_fp1:                            
         fmovem.x  %fp1,user_fp1(%a6) 
         rts                        
c_is_fp0:                            
         fmovem.x  %fp1,user_fp0(%a6) 
         rts                        
                                    
                                    
         global    sto_res          
sto_res:                            
         bfextu    cmdreg1b(%a6){&6:&3},%d0 # extract destination register
         cmpi.b    %d0,&3           # check for fp0/fp1 cases
         ble.b     fp0123           
         fmovem.x  %fp0,-(%a7)      
         moveq.l   &7,%d1           
         sub.l     %d0,%d1          # d1 = 7- (dest. reg. no.)
         clr.l     %d0              
         bset.l    %d1,%d0          # d0 is dynamic register mask
         fmovem.x  (%a7)+,%d0       
         rts                        
fp0123:                             
         cmpi.b    %d0,&0           
         beq.b     is_fp0           
         cmpi.b    %d0,&1           
         beq.b     is_fp1           
         cmpi.b    %d0,&2           
         beq.b     is_fp2           
is_fp3:                             
         fmovem.x  %fp0,user_fp3(%a6) 
         rts                        
is_fp2:                             
         fmovem.x  %fp0,user_fp2(%a6) 
         rts                        
is_fp1:                             
         fmovem.x  %fp0,user_fp1(%a6) 
         rts                        
is_fp0:                             
         fmovem.x  %fp0,user_fp0(%a6) 
         rts                        
                                    
                                    
	version 3
