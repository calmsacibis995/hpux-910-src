ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/bugfix.s,v $
# $Revision: 1.3.84.4 $	$Author: rpc $
# $State: Exp $   	$Locker:  $
# $Date: 93/12/14 09:22:53 $
                                    
#
#	bugfix.sa 3.2 1/31/91
#
#
#	This file contains workarounds for bugs in the 040
#	relating to the Floating-Point Software Package (FPSP)
#
#	Fixes for bugs: 1238
#
#	Bug: 1238 
#
#
#    /* The following dirty_bit clear should be left in
#     * the handler permanently to improve throughput.
#     * The dirty_bits are located at bits [23:16] in
#     * longword $08 in the busy frame $4x60.  Bit 16
#     * corresponds to FP0, bit 17 corresponds to FP1,
#     * and so on.
#     */
#    if  (E3_exception_just_serviced)   {
#         dirty_bit[cmdreg3b[9:7]] = 0;
#         }
#
#    if  (fsave_format_version != $40)  {goto NOFIX}
#
#    if !(E3_exception_just_serviced)   {goto NOFIX}
#    if  (cupc == 0000000)              {goto NOFIX}
#    if  ((cmdreg1b[15:13] != 000) &&
#         (cmdreg1b[15:10] != 010001))  {goto NOFIX}
#    if (((cmdreg1b[15:13] != 000) || ((cmdreg1b[12:10] != cmdreg2b[9:7]) &&
#				      (cmdreg1b[12:10] != cmdreg3b[9:7]))  ) &&
#	 ((cmdreg1b[ 9: 7] != cmdreg2b[9:7]) &&
#	  (cmdreg1b[ 9: 7] != cmdreg3b[9:7])) )  {goto NOFIX}
#
#    /* Note: for 6d43b or 8d43b, you may want to add the following code
#     * to get better coverage.  (If you do not insert this code, the part
#     * won't lock up; it will simply get the wrong answer.)
#     * Do NOT insert this code for 10d43b or later parts.
#     *
#     *  if (fpiarcu == integer stack return address) {
#     *       cupc = 0000000;
#     *       goto NOFIX;
#     *       }
#     */
#
#    if (cmdreg1b[15:13] != 000)   {goto FIX_OPCLASS2}
#    FIX_OPCLASS0:
#    if (((cmdreg1b[12:10] == cmdreg2b[9:7]) ||
#	 (cmdreg1b[ 9: 7] == cmdreg2b[9:7])) &&
#	(cmdreg1b[12:10] != cmdreg3b[9:7]) &&
#	(cmdreg1b[ 9: 7] != cmdreg3b[9:7]))  {  /* xu conflict only */
#	/* We execute the following code if there is an
#	   xu conflict and NOT an nu conflict */
#
#	/* first save some values on the fsave frame */
#	stag_temp     = STAG[fsave_frame];
#	cmdreg1b_temp = CMDREG1B[fsave_frame];
#	dtag_temp     = DTAG[fsave_frame];
#	ete15_temp    = ETE15[fsave_frame];
#
#	CUPC[fsave_frame] = 0000000;
#	FRESTORE
#	FSAVE
#
#	/* If the xu instruction is exceptional, we punt.
#	 * Otherwise, we would have to include OVFL/UNFL handler
#	 * code here to get the correct answer.
#	 */
#	if (fsave_frame_format == $4060) {goto KILL_PROCESS}
#
#	fsave_frame = /* build a long frame of all zeros */
#	fsave_frame_format = $4060;  /* label it as long frame */
#
#	/* load it with the temps we saved */
#	STAG[fsave_frame]     =  stag_temp;
#	CMDREG1B[fsave_frame] =  cmdreg1b_temp;
#	DTAG[fsave_frame]     =  dtag_temp;
#	ETE15[fsave_frame]    =  ete15_temp;
#
#	/* Make sure that the cmdreg3b dest reg is not going to
#	 * be destroyed by a FMOVEM at the end of all this code.
#	 * If it is, you should move the current value of the reg
#	 * onto the stack so that the reg will loaded with that value.
#	 */
#
#	/* All done.  Proceed with the code below */
#    }
#
#    etemp  = FP_reg_[cmdreg1b[12:10]];
#    ete15  = ~ete14;
#    cmdreg1b[15:10] = 010010;
#    clear(bug_flag_procIDxxxx);
#    FRESTORE and return;
#
#
#    FIX_OPCLASS2:
#    if ((cmdreg1b[9:7] == cmdreg2b[9:7]) &&
#	(cmdreg1b[9:7] != cmdreg3b[9:7]))  {  /* xu conflict only */
#	/* We execute the following code if there is an
#	   xu conflict and NOT an nu conflict */
#
#	/* first save some values on the fsave frame */
#	stag_temp     = STAG[fsave_frame];
#	cmdreg1b_temp = CMDREG1B[fsave_frame];
#	dtag_temp     = DTAG[fsave_frame];
#	ete15_temp    = ETE15[fsave_frame];
#	etemp_temp    = ETEMP[fsave_frame];
#
#	CUPC[fsave_frame] = 0000000;
#	FRESTORE
#	FSAVE
#
#
#	/* If the xu instruction is exceptional, we punt.
#	 * Otherwise, we would have to include OVFL/UNFL handler
#	 * code here to get the correct answer.
#	 */
#	if (fsave_frame_format == $4060) {goto KILL_PROCESS}
#
#	fsave_frame = /* build a long frame of all zeros */
#	fsave_frame_format = $4060;  /* label it as long frame */
#
#	/* load it with the temps we saved */
#	STAG[fsave_frame]     =  stag_temp;
#	CMDREG1B[fsave_frame] =  cmdreg1b_temp;
#	DTAG[fsave_frame]     =  dtag_temp;
#	ETE15[fsave_frame]    =  ete15_temp;
#	ETEMP[fsave_frame]    =  etemp_temp;
#
#	/* Make sure that the cmdreg3b dest reg is not going to
#	 * be destroyed by a FMOVEM at the end of all this code.
#	 * If it is, you should move the current value of the reg
#	 * onto the stack so that the reg will loaded with that value.
#	 */
#
#	/* All done.  Proceed with the code below */
#    }
#
#    if (etemp_exponent == min_sgl)   etemp_exponent = min_dbl;
#    if (etemp_exponent == max_sgl)   etemp_exponent = max_dbl;
#    cmdreg1b[15:10] = 010101;
#    clear(bug_flag_procIDxxxx);
#    FRESTORE and return;
#
#
#    NOFIX:
#    clear(bug_flag_procIDxxxx);
#    FRESTORE and return;
#
                                    
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
         global    b1238_fix        
b1238_fix:                            
#
# This code is entered only on completion of the handling of an 
# nu-generated ovfl, unfl, or inex exception.  If the version 
# number of the fsave is not $40, this handler is not necessary.
# Simply branch to fix_done and exit normally.
#
         cmpi.b    4(%a7),&ver_40   
         bne.w     fix_done         
#
# Test for cu_savepc equal to zero.  If not, this is not a bug
# #1238 case.
#
         move.b    cu_savepc(%a6),%d0 
         andi.b    &0xfe,%d0        
         beq       fix_done         # if zero, this is not bug #1238
                                    
#
# Test the register conflict aspect.  If opclass0, check for
# cu src equal to xu dest or equal to nu dest.  If so, go to 
# op0.  Else, or if opclass2, check for cu dest equal to
# xu dest or equal to nu dest.  If so, go to tst_opcl.  Else,
# exit, it is not the bug case.
#
# Check for opclass 0.  If not, go and check for opclass 2 and sgl.
#
         move.w    cmdreg1b(%a6),%d0 
         andi.w    &0xe000,%d0      # strip all but opclass
         bne       op2sgl           # not opclass 0, check op2
#
# Check for cu and nu register conflict.  If one exists, this takes
# priority over a cu and xu conflict. 
#
         bfextu    cmdreg1b(%a6){&3:&3},%d0 # get 1st src 
         bfextu    cmdreg3b(%a6){&6:&3},%d1 # get 3rd dest
         cmp.b     %d1,%d0          
         beq.b     op0              # if equal, continue bugfix
#
# Check for cu dest equal to nu dest.  If so, go and fix the 
# bug condition.  Otherwise, exit.
#
         bfextu    cmdreg1b(%a6){&6:&3},%d0 # get 1st dest 
         cmp.b     %d1,%d0          # cmp 1st dest with 3rd dest
         beq.b     op0              # if equal, continue bugfix
#
# Check for cu and xu register conflict.
#
         bfextu    cmdreg2b(%a6){&6:&3},%d1 # get 2nd dest
         cmp.b     %d1,%d0          # cmp 1st dest with 2nd dest
         beq.b     op0_xu           # if equal, continue bugfix
         bfextu    cmdreg1b(%a6){&3:&3},%d0 # get 1st src 
         cmp.b     %d1,%d0          # cmp 1st src with 2nd dest
         beq       op0_xu           
         bne       fix_done         # if the reg checks fail, exit
#
# We have the opclass 0 situation.
#
op0:                                
         bfextu    cmdreg1b(%a6){&3:&3},%d0 # get source register no
         move.l    &7,%d1           
         sub.l     %d0,%d1          
         clr.l     %d0              
         bset.l    %d1,%d0          
         fmovem.x  %d0,etemp(%a6)   # load source to ETEMP
                                    
         move.b    &0x12,%d0        
         bfins     %d0,cmdreg1b(%a6){&0:&6} # opclass 2, extended
#
#	Set ETEMP exponent bit 15 as the opposite of ete14
#
         btst      &6,etemp_ex(%a6) # check etemp exponent bit 14
         beq       setete15         
         bclr      &etemp15_bit,stag(%a6) 
         bra       finish           
setete15:                            
         bset      &etemp15_bit,stag(%a6) 
         bra       finish           
                                    
#
# We have the case in which a conflict exists between the cu src or
# dest and the dest of the xu.  We must clear the instruction in 
# the cu and restore the state, allowing the instruction in the
# xu to complete.  Remember, the instruction in the nu
# was exceptional, and was completed by the appropriate handler.
# If the result of the xu instruction is not exceptional, we can
# restore the instruction from the cu to the frame and continue
# processing the original exception.  If the result is also
# exceptional, we choose to kill the process.
#
#	Items saved from the stack:
#	
#		$3c stag     - L_SCR1
#		$40 cmdreg1b - L_SCR2
#		$44 dtag     - L_SCR3
#
# The cu savepc is set to zero, and the frame is restored to the
# fpu.
#
op0_xu:                             
         move.l    stag(%a6),l_scr1(%a6) 
         move.l    cmdreg1b(%a6),l_scr2(%a6) 
         move.l    dtag(%a6),l_scr3(%a6) 
         andi.l    &0xe0000000,l_scr3(%a6) 
         move.b    &0,cu_savepc(%a6) 
         move.l    (%a7)+,%d1       # save return address from bsr
         frestore  (%a7)+           
         fsave     -(%a7)           
#
# Check if the instruction which just completed was exceptional.
# 
         cmp.w     (%a7),&0x4060    
         beq       op0_xb           
# 
# It is necessary to isolate the result of the instruction in the
# xu if it is to fp0 - fp3 and write that value to the USER_FPn
# locations on the stack.  The correct destination register is in 
# cmdreg2b.
#
         bfextu    cmdreg2b(%a6){&6:&3},%d0 # get dest register no
         cmpi.l    %d0,&3           
         bgt.b     op0_xi           
         beq.b     op0_fp3          
         cmpi.l    %d0,&1           
         blt.b     op0_fp0          
         beq.b     op0_fp1          
op0_fp2:                            
         fmovem.x  %fp2,user_fp2(%a6) 
         bra.b     op0_xi           
op0_fp1:                            
         fmovem.x  %fp1,user_fp1(%a6) 
         bra.b     op0_xi           
op0_fp0:                            
         fmovem.x  %fp0,user_fp0(%a6) 
         bra.b     op0_xi           
op0_fp3:                            
         fmovem.x  %fp3,user_fp3(%a6) 
#
# The frame returned is idle.  We must build a busy frame to hold
# the cu state information and setup etemp.
#
op0_xi:                             
         move.l    &22,%d0          # clear 23 lwords
         clr.l     (%a7)            
op0_loop:                            
         clr.l     -(%a7)           
         dbf       %d0,op0_loop     
         move.l    &0x40600000,-(%a7) 
         move.l    l_scr1(%a6),stag(%a6) 
         move.l    l_scr2(%a6),cmdreg1b(%a6) 
         move.l    l_scr3(%a6),dtag(%a6) 
         move.b    &0x6,cu_savepc(%a6) 
         move.l    %d1,-(%a7)       # return bsr return address
         bfextu    cmdreg1b(%a6){&3:&3},%d0 # get source register no
         move.l    &7,%d1           
         sub.l     %d0,%d1          
         clr.l     %d0              
         bset.l    %d1,%d0          
         fmovem.x  %d0,etemp(%a6)   # load source to ETEMP
                                    
         move.b    &0x12,%d0        
         bfins     %d0,cmdreg1b(%a6){&0:&6} # opclass 2, extended
#
#	Set ETEMP exponent bit 15 as the opposite of ete14
#
         btst      &6,etemp_ex(%a6) # check etemp exponent bit 14
         beq       op0_sete15       
         bclr      &etemp15_bit,stag(%a6) 
         bra       finish           
op0_sete15:                            
         bset      &etemp15_bit,stag(%a6) 
         bra       finish           
                                    
#
# The frame returned is busy.  It is not possible to reconstruct
# the code sequence to allow completion.  We will jump to 
# fpsp_fmt_error and allow the kernel to kill the process.
#
#
#	fpsp_fmt_error calls panic. Not what we want to do here...
#	Push the return address back on the stack, and return. SIGFPE
#	will be sent to the process. (RPC, 12/10/93)
#
op0_xb:                             
	move.l    %d1,-(%a7)       # return bsr return address
	bra       finish           
                                    
#
# Check for opclass 2 and single size.  If not both, exit.
#
op2sgl:                             
         move.w    cmdreg1b(%a6),%d0 
         andi.w    &0xfc00,%d0      # strip all but opclass and size
         cmpi.w    %d0,&0x4400      # test for opclass 2 and size=sgl
         bne       fix_done         # if not, it is not bug 1238
#
# Check for cu dest equal to nu dest or equal to xu dest, with 
# a cu and nu conflict taking priority an nu conflict.  If either,
# go and fix the bug condition.  Otherwise, exit.
#
         bfextu    cmdreg1b(%a6){&6:&3},%d0 # get 1st dest 
         bfextu    cmdreg3b(%a6){&6:&3},%d1 # get 3rd dest
         cmp.b     %d1,%d0          # cmp 1st dest with 3rd dest
         beq       op2_com          # if equal, continue bugfix
         bfextu    cmdreg2b(%a6){&6:&3},%d1 # get 2nd dest 
         cmp.b     %d1,%d0          # cmp 1st dest with 2nd dest
         bne       fix_done         # if the reg checks fail, exit
#
# We have the case in which a conflict exists between the cu src or
# dest and the dest of the xu.  We must clear the instruction in 
# the cu and restore the state, allowing the instruction in the
# xu to complete.  Remember, the instruction in the nu
# was exceptional, and was completed by the appropriate handler.
# If the result of the xu instruction is not exceptional, we can
# restore the instruction from the cu to the frame and continue
# processing the original exception.  If the result is also
# exceptional, we choose to kill the process.
#
#	Items saved from the stack:
#	
#		$3c stag     - L_SCR1
#		$40 cmdreg1b - L_SCR2
#		$44 dtag     - L_SCR3
#		etemp        - FP_SCR2
#
# The cu savepc is set to zero, and the frame is restored to the
# fpu.
#
op2_xu:                             
         move.l    stag(%a6),l_scr1(%a6) 
         move.l    cmdreg1b(%a6),l_scr2(%a6) 
         move.l    dtag(%a6),l_scr3(%a6) 
         andi.l    &0xe0000000,l_scr3(%a6) 
         move.b    &0,cu_savepc(%a6) 
         move.l    etemp(%a6),fp_scr2(%a6) 
         move.l    etemp_hi(%a6),fp_scr2+4(%a6) 
         move.l    etemp_lo(%a6),fp_scr2+8(%a6) 
         move.l    (%a7)+,%d1       # save return address from bsr
         frestore  (%a7)+           
         fsave     -(%a7)           
#
# Check if the instruction which just completed was exceptional.
# 
         cmp.w     (%a7),&0x4060    
         beq       op2_xb           
# 
# It is necessary to isolate the result of the instruction in the
# xu if it is to fp0 - fp3 and write that value to the USER_FPn
# locations on the stack.  The correct destination register is in 
# cmdreg2b.
#
         bfextu    cmdreg2b(%a6){&6:&3},%d0 # get dest register no
         cmpi.l    %d0,&3           
         bgt.b     op2_xi           
         beq.b     op2_fp3          
         cmpi.l    %d0,&1           
         blt.b     op2_fp0          
         beq.b     op2_fp1          
op2_fp2:                            
         fmovem.x  %fp2,user_fp2(%a6) 
         bra.b     op2_xi           
op2_fp1:                            
         fmovem.x  %fp1,user_fp1(%a6) 
         bra.b     op2_xi           
op2_fp0:                            
         fmovem.x  %fp0,user_fp0(%a6) 
         bra.b     op2_xi           
op2_fp3:                            
         fmovem.x  %fp3,user_fp3(%a6) 
#
# The frame returned is idle.  We must build a busy frame to hold
# the cu state information and fix up etemp.
#
op2_xi:                             
         move.l    &22,%d0          # clear 23 lwords
         clr.l     (%a7)            
op2_loop:                            
         clr.l     -(%a7)           
         dbf       %d0,op2_loop     
         move.l    &0x40600000,-(%a7) 
         move.l    l_scr1(%a6),stag(%a6) 
         move.l    l_scr2(%a6),cmdreg1b(%a6) 
         move.l    l_scr3(%a6),dtag(%a6) 
         move.b    &0x6,cu_savepc(%a6) 
         move.l    fp_scr2(%a6),etemp(%a6) 
         move.l    fp_scr2+4(%a6),etemp_hi(%a6) 
         move.l    fp_scr2+8(%a6),etemp_lo(%a6) 
         move.l    %d1,-(%a7)       
         bra       op2_com          
                                    
#
# We have the opclass 2 single source situation.
#
op2_com:                            
         move.b    &0x15,%d0        
         bfins     %d0,cmdreg1b(%a6){&0:&6} # opclass 2, double
                                    
         cmp.w     etemp_ex(%a6),&0x407f # single +max
         bne.b     case2            
         move.w    &0x43ff,etemp_ex(%a6) # to double +max
         bra       finish           
case2:                              
         cmp.w     etemp_ex(%a6),&0xc07f # single -max
         bne.b     case3            
         move.w    &0xc3ff,etemp_ex(%a6) # to double -max
         bra       finish           
case3:                              
         cmp.w     etemp_ex(%a6),&0x3f80 # single +min
         bne.b     case4            
         move.w    &0x3c00,etemp_ex(%a6) # to double +min
         bra       finish           
case4:                              
         cmp.w     etemp_ex(%a6),&0xbf80 # single -min
         bne       fix_done         
         move.w    &0xbc00,etemp_ex(%a6) # to double -min
         bra       finish           
#
# The frame returned is busy.  It is not possible to reconstruct
# the code sequence to allow completion.  fpsp_fmt_error causes
# an fline illegal instruction to be executed.
#
# You should replace the jump to fpsp_fmt_error with a jump
# to the entry point used to kill a process. 
#
#
#	fpsp_fmt_error calls panic. Not what we want to do here...
#	Push the return address back on the stack, and return. SIGFPE
#	will be sent to the process. (RPC, 12/10/93)
#
op2_xb:                             
	move.l    %d1,-(%a7)       # return bsr return address
	bra       finish           
                                    
#
# Enter here if the case is not of the situations affected by
# bug #1238, or if the fix is completed, and exit.
#
finish:                             
fix_done:                            
# Begin GSL Additions, clear process flag
         set       s268040_fp,0x00040000 
         set       not_s268040_fp,0xfffbffff 
# Should be "_u+U_PROCP", but U_PROCP is 0x0, so forget about it.
         mov.l     %a0,-(%sp)       
         mov.l     _u,%a0           
         and.l     &not_s268040_fp,(%a0) 
         mov.l     (%sp)+,%a0       
# End GSL Additions
         rts                        
                                    
                                    
	version 3
