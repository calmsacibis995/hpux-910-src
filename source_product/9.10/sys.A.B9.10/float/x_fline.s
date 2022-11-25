ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_fline.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:55:34 $
                                    
#
#	x_fline.sa 3.3 1/10/91
#
#	fpsp_fline --- FPSP handler for fline exception
#
#	First determine if the exception is one of the unimplemented
#	floating point instructions.  If so, let fpsp_unimp handle it.
#	Next, determine if the instruction is an fmovecr with a non-zero
#	<ea> field.  If so, handle here and return.  Otherwise, it
#	must be a real F-line exception.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_fline       
fpsp_fline:                            
#
#	check for unimplemented vector first.  Use EXC_VEC-4 because
#	the equate is valid only after a 'link a6' has pushed one more
#	long onto the stack.
#
         cmp.w     exc_vec-4(%a7),&unimp_vec 
         beq.l     fpsp_unimp       
                                    
#
#	fmovecr with non-zero <ea> handling here
#
         sub.l     &4,%a7           # 4 accounts for 2-word difference
#				;between six word frame (unimp) and
#				;four word frame
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         movea.l   exc_pc+4(%a6),%a0 # get address of fline instruction
         lea.l     l_scr1(%a6),%a1  # use L_SCR1 as scratch
         move.l    &4,%d0           
         add.l     &4,%a6           # to offset the sub.l #4,a7 above so that
#				;a6 can point correctly to the stack frame 
#				;before branching to mem_read
         bsr.l     mem_read         
         sub.l     &4,%a6           
         move.l    l_scr1(%a6),%d0  # d0 contains the fline and command word
         bfextu    %d0{&4:&3},%d1   # extract coprocessor id
         cmpi.b    %d1,&1           # check if cpid=1
         bne.w     not_mvcr         # exit if not
         bfextu    %d0{&16:&6},%d1  
         cmpi.b    %d1,&0x17        # check if it is an FMOVECR encoding
         bne.w     not_mvcr         
#				;if an FMOVECR instruction, fix stack
#				;and go to FPSP_UNIMP
fix_stack:                            
         cmpi.b    (%a7),&ver_40    # test for orig unimp frame
         bne.b     ck_rev           
         sub.l     &unimp_40_size-4,%a7 # emulate an orig fsave
         move.b    &ver_40,(%a7)    
         move.b    &unimp_40_size-4,1(%a7) 
         clr.w     2(%a7)           
         bra.b     fix_con          
ck_rev:                             
         cmpi.b    (%a7),&ver_41    # test for rev unimp frame
         bne.l     fpsp_fmt_error   # if not $40 or $41, exit with error
         sub.l     &unimp_41_size-4,%a7 # emulate a rev fsave
         move.b    &ver_41,(%a7)    
         move.b    &unimp_41_size-4,1(%a7) 
         clr.w     2(%a7)           
fix_con:                            
         move.w    exc_sr+4(%a6),exc_sr(%a6) # move stacked sr to new position
         move.l    exc_pc+4(%a6),exc_pc(%a6) # move stacked pc to new position
         fmove.l   exc_pc(%a6),%fpiar # point FPIAR to fline inst
         move.l    &4,%d1           
         add.l     %d1,exc_pc(%a6)  # increment stacked pc value to next inst
         move.w    &0x202c,exc_vec(%a6) # reformat vector to unimp
         clr.l     exc_ea(%a6)      # clear the EXC_EA field
         move.w    %d0,cmdreg1b(%a6) # move the lower word into CMDREG1B
         clr.l     e_byte(%a6)      
         bset.b    &uflag,t_byte(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 # restore data registers
         bra.l     uni_2            
                                    
not_mvcr:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 # restore data registers
         frestore  (%a7)+           
         unlk      %a6              
         add.l     &4,%a7           
         bra.l     real_fline       
                                    
                                    
	version 3
