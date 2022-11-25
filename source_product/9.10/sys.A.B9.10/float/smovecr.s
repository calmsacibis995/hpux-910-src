ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/smovecr.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:52:51 $
                                    
#
#	smovecr.sa 3.1 12/10/90
#
#	The entry point sMOVECR returns the constant at the
#	offset given in the instruction field.
#
#	Input: An offset in the instruction word.
#
#	Output:	The constant rounded to the user's rounding
#		mode unchecked for overflow.
#
#	Modified: fp0.
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
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
fzero:   long      00000000         
#
#	FMOVECR 
#
         global    smovcr           
smovcr:                             
         bfextu    cmdreg1b(%a6){&9:&7},%d0 # get offset
         bfextu    user_fpcr(%a6){&26:&2},%d1 # get rmode
#
# check range of offset
#
         tst.b     %d0              # if zero, offset is to pi
         beq.b     pi_tbl           # it is pi
         cmpi.b    %d0,&0x0a        # check range $01 - $0a
         ble.b     z_val            # if in this range, return zero
         cmpi.b    %d0,&0x0e        # check range $0b - $0e
         ble.b     sm_tbl           # valid constants in this range
         cmpi.b    %d0,&0x2f        # check range $10 - $2f
         ble.b     z_val            # if in this range, return zero 
         cmpi.b    %d0,&0x3f        # check range $30 - $3f
         ble       bg_tbl           # valid constants in this range
z_val:                              
         fmove.s   fzero,%fp0       
         rts                        
pi_tbl:                             
         tst.b     %d1              # offset is zero, check for rmode
         beq.b     pi_rn            # if zero, rn mode
         cmpi.b    %d1,&0x3         # check for rp
         beq.b     pi_rp            # if 3, rp mode
pi_rzrm:                            
         lea.l     pirzrm,%a0       # rmode is rz or rm, load PIRZRM in a0
         bra       set_finx         
pi_rn:                              
         lea.l     pirn,%a0         # rmode is rn, load PIRN in a0
         bra       set_finx         
pi_rp:                              
         lea.l     pirp,%a0         # rmode is rp, load PIRP in a0
         bra       set_finx         
sm_tbl:                             
         subi.l    &0xb,%d0         # make offset in 0 - 4 range
         tst.b     %d1              # check for rmode
         beq.b     sm_rn            # if zero, rn mode
         cmpi.b    %d1,&0x3         # check for rp
         beq.b     sm_rp            # if 3, rp mode
sm_rzrm:                            
         lea.l     smalrzrm,%a0     # rmode is rz or rm, load SMRZRM in a0
         cmpi.b    %d0,&0x2         # check if result is inex
         ble       set_finx         # if 0 - 2, it is inexact
         bra       no_finx          # if 3, it is exact
sm_rn:                              
         lea.l     smalrn,%a0       # rmode is rn, load SMRN in a0
         cmpi.b    %d0,&0x2         # check if result is inex
         ble       set_finx         # if 0 - 2, it is inexact
         bra       no_finx          # if 3, it is exact
sm_rp:                              
         lea.l     smalrp,%a0       # rmode is rp, load SMRP in a0
         cmpi.b    %d0,&0x2         # check if result is inex
         ble       set_finx         # if 0 - 2, it is inexact
         bra       no_finx          # if 3, it is exact
bg_tbl:                             
         subi.l    &0x30,%d0        # make offset in 0 - f range
         tst.b     %d1              # check for rmode
         beq.b     bg_rn            # if zero, rn mode
         cmpi.b    %d1,&0x3         # check for rp
         beq.b     bg_rp            # if 3, rp mode
bg_rzrm:                            
         lea.l     bigrzrm,%a0      # rmode is rz or rm, load BGRZRM in a0
         cmpi.b    %d0,&0x1         # check if result is inex
         ble       set_finx         # if 0 - 1, it is inexact
         cmpi.b    %d0,&0x7         # second check
         ble       no_finx          # if 0 - 7, it is exact
         bra       set_finx         # if 8 - f, it is inexact
bg_rn:                              
         lea.l     bigrn,%a0        # rmode is rn, load BGRN in a0
         cmpi.b    %d0,&0x1         # check if result is inex
         ble       set_finx         # if 0 - 1, it is inexact
         cmpi.b    %d0,&0x7         # second check
         ble       no_finx          # if 0 - 7, it is exact
         bra       set_finx         # if 8 - f, it is inexact
bg_rp:                              
         lea.l     bigrp,%a0        # rmode is rp, load SMRP in a0
         cmpi.b    %d0,&0x1         # check if result is inex
         ble       set_finx         # if 0 - 1, it is inexact
         cmpi.b    %d0,&0x7         # second check
         ble       no_finx          # if 0 - 7, it is exact
#	bra	set_finx	;if 8 - f, it is inexact
set_finx:                            
         or.l      &inx2a_mask,user_fpsr(%a6) # set inex2/ainex
no_finx:                            
         mulu.l    &12,%d0          # use offset to point into tables
         move.l    %d1,l_scr1(%a6)  # load mode for round call
         bfextu    user_fpcr(%a6){&24:&2},%d1 # get precision
         tst.l     %d1              # check if extended precision
#
# Precision is extended
#
         bne.b     not_ext          # if extended, do not call round
         fmovem.x  (%a0,%d0),%fp0   # return result in fp0
         rts                        
#
# Precision is single or double
#
not_ext:                            
         swap      %d1              # rnd prec in upper word of d1
         add.l     l_scr1(%a6),%d1  # merge rmode in low word of d1
         move.l    (%a0,%d0),fp_scr1(%a6) # load first word to temp storage
         move.l    4(%a0,%d0),fp_scr1+4(%a6) # load second word
         move.l    8(%a0,%d0),fp_scr1+8(%a6) # load third word
         clr.l     %d0              # clear g,r,s
         lea       fp_scr1(%a6),%a0 
         btst.b    &sign_bit,local_ex(%a0) 
         sne       local_sgn(%a0)   # convert to internal ext. format
                                    
         bsr.l     round            # go round the mantissa
                                    
         bfclr     local_sgn(%a0){&0:&8} # convert back to IEEE ext format
         beq.b     fin_fcr          
         bset.b    &sign_bit,local_ex(%a0) 
fin_fcr:                            
         fmovem.x  (%a0),%fp0       
         rts                        
                                    
                                    
	version 3
