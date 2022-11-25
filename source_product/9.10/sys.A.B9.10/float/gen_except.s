ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/gen_except.s,v $
# $Revision: 1.2.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:48:19 $
                                    
#
#	gen_except.sa 3.6 7/31/91
#
#	gen_except --- FPSP routine to detect reportable exceptions
#	
#	This routine compares the exception enable byte of the
#	user_fpcr on the stack with the exception status byte
#	of the user_fpsr. 
#
#	Any routine which may report an exceptions must load
#	the stack frame in memory with the exceptional operand(s).
#
#	Priority for exceptions is:
#
#	Highest:	bsun
#			snan
#			operr
#			ovfl
#			unfl
#			dz
#			inex2
#	Lowest:		inex1
#
#	Note: The IEEE standard specifies that inex2 is to be
#	reported if ovfl occurs and the ovfl enable bit is not
#	set but the inex2 enable bit is.  
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
                                    
                                    
                                    
                                    
                                    
exc_tbl:                            
         long      bsun_exc         
         long      commone1         
         long      commone1         
         long      ovfl_unfl        
         long      ovfl_unfl        
         long      commone1         
         long      commone3         
         long      commone3         
         long      no_match         
                                    
         global    gen_except       
gen_except:                            
         cmpi.b    1(%a7),&idle_size-4 # test for idle frame
         beq.w     do_check         # go handle idle frame
         cmpi.b    1(%a7),&unimp_40_size-4 # test for orig unimp frame
         beq.b     unimp_x          # go handle unimp frame
         cmpi.b    1(%a7),&unimp_41_size-4 # test for rev unimp frame
         beq.b     unimp_x          # go handle unimp frame
         cmpi.b    1(%a7),&busy_size-4 # if size <> $60, fmt error
         bne.l     fpsp_fmt_error   
         lea.l     busy_size+local_size(%a7),%a1 # init a1 so fpsp.h
#					;equates will work
# Fix up the new busy frame with entries from the unimp frame
#
         move.l    etemp_ex(%a6),etemp_ex(%a1) # copy etemp from unimp
         move.l    etemp_hi(%a6),etemp_hi(%a1) # frame to busy frame
         move.l    etemp_lo(%a6),etemp_lo(%a1) 
         move.l    cmdreg1b(%a6),cmdreg1b(%a1) # set inst in frame to unimp
         move.l    cmdreg1b(%a6),%d0 # fix cmd1b to make it
         and.l     &0x03c30000,%d0  # work for cmd3b
         bfextu    cmdreg1b(%a6){&13:&1},%d1 # extract bit 2
         lsl.l     &5,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put it in the right place
         bfextu    cmdreg1b(%a6){&10:&3},%d1 # extract bit 3,4,5
         lsl.l     &2,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put them in the right place
         move.l    %d0,cmdreg3b(%a1) # in the busy frame
#
# Or in the FPSR from the emulation with the USER_FPSR on the stack.
#
         fmove.l   %fpsr,%d0        
         or.l      %d0,user_fpsr(%a6) 
         move.l    user_fpsr(%a6),fpsr_shadow(%a1) # set exc bits
         or.l      &sx_mask,e_byte(%a1) 
         bra       do_clean         
                                    
#
# Frame is an unimp frame possible resulting from an fmove <ea>,fp0
# that caused an exception
#
# a1 is modified to point into the new frame allowing fpsp equates
# to be valid.
#
unimp_x:                            
         cmpi.b    1(%a7),&unimp_40_size-4 # test for orig unimp frame
         bne.b     test_rev         
         lea.l     unimp_40_size+local_size(%a7),%a1 
         bra.b     unimp_con        
test_rev:                            
         cmpi.b    1(%a7),&unimp_41_size-4 # test for rev unimp frame
         bne.l     fpsp_fmt_error   # if not $28 or $30
         lea.l     unimp_41_size+local_size(%a7),%a1 
                                    
unimp_con:                            
#
# Fix up the new unimp frame with entries from the old unimp frame
#
         move.l    cmdreg1b(%a6),cmdreg1b(%a1) # set inst in frame to unimp
#
# Or in the FPSR from the emulation with the USER_FPSR on the stack.
#
         fmove.l   %fpsr,%d0        
         or.l      %d0,user_fpsr(%a6) 
         bra       do_clean         
                                    
#
# Frame is idle, so check for exceptions reported through
# USER_FPSR and set the unimp frame accordingly.  
# A7 must be incremented to the point before the
# idle fsave vector to the unimp vector.
#
                                    
do_check:                            
         add.l     &4,%a7           # point A7 back to unimp frame
#
# Or in the FPSR from the emulation with the USER_FPSR on the stack.
#
         fmove.l   %fpsr,%d0        
         or.l      %d0,user_fpsr(%a6) 
#
# On a busy frame, we must clear the nmnexc bits.
#
         cmpi.b    1(%a7),&busy_size-4 # check frame type
         bne.b     check_fr         # if busy, clr nmnexc
         clr.w     nmnexc(%a6)      # clr nmnexc & nmcexc
         btst.b    &5,cmdreg1b(%a6) # test for fmove out
         bne.b     frame_com        
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) # set exc bits
         or.l      &sx_mask,e_byte(%a6) 
         bra.b     frame_com        
check_fr:                            
         cmp.b     1(%a7),&unimp_40_size-4 
         beq.b     frame_com        
         clr.w     nmnexc(%a6)      
frame_com:                            
         move.b    fpcr_enable(%a6),%d0 # get fpcr enable byte
         and.b     fpsr_except(%a6),%d0 # and in the fpsr exc byte
         bfffo     %d0{&24:&8},%d1  # test for first set bit
         lea.l     exc_tbl,%a0      # load jmp table address
         subi.b    &24,%d1          # normalize bit offset to 0-8
         move.l    (%a0,%d1.w*4),%a0 # load routine address based
#					;based on first enabled exc
         jmp       (%a0)            # jump to routine
#
# Bsun is not possible in unimp or unsupp
#
bsun_exc:                            
         bra       do_clean         
#
# The typical work to be done to the unimp frame to report an 
# exception is to set the E1/E3 byte and clr the U flag.
# commonE1 does this for E1 exceptions, which are snan, 
# operr, and dz.  commonE3 does this for E3 exceptions, which 
# are inex2 and inex1, and also clears the E1 exception bit
# left over from the unimp exception.
#
commone1:                            
         bset.b    &e1,e_byte(%a6)  # set E1 flag
         bra.w     commone          # go clean and exit
                                    
commone3:                            
         tst.b     uflg_tmp(%a6)    # test flag for unsup/unimp state
         bne.b     unse3            
unie3:                              
         bset.b    &e3,e_byte(%a6)  # set E3 flag
         bclr.b    &e1,e_byte(%a6)  # clr E1 from unimp
         bra.w     commone          
                                    
unse3:                              
         tst.b     res_flg(%a6)     
         bne.b     unse3_0          
unse3_1:                            
         bset.b    &e3,e_byte(%a6)  # set E3 flag
unse3_0:                            
         bclr.b    &e1,e_byte(%a6)  # clr E1 flag
         move.l    cmdreg1b(%a6),%d0 
         and.l     &0x03c30000,%d0  # work for cmd3b
         bfextu    cmdreg1b(%a6){&13:&1},%d1 # extract bit 2
         lsl.l     &5,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put it in the right place
         bfextu    cmdreg1b(%a6){&10:&3},%d1 # extract bit 3,4,5
         lsl.l     &2,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put them in the right place
         move.l    %d0,cmdreg3b(%a6) # in the busy frame
                                    
commone:                            
         bclr.b    &uflag,t_byte(%a6) # clr U flag from unimp
         bra.w     do_clean         # go clean and exit
#
# No bits in the enable byte match existing exceptions.  Check for
# the case of the ovfl exc without the ovfl enabled, but with
# inex2 enabled.
#
no_match:                            
         btst.b    &inex2_bit,fpcr_enable(%a6) # check for ovfl/inex2 case
         beq.b     no_exc           # if clear, exit
         btst.b    &ovfl_bit,fpsr_except(%a6) # now check ovfl
         beq.b     no_exc           # if clear, exit
         bra.b     ovfl_unfl        # go to unfl_ovfl to determine if
#					;it is an unsupp or unimp exc
                                    
# No exceptions are to be reported.  If the instruction was 
# unimplemented, no FPU restore is necessary.  If it was
# unsupported, we must perform the restore.
no_exc:                             
         tst.b     uflg_tmp(%a6)    # test flag for unsupp/unimp state
         beq.b     uni_no_exc       
uns_no_exc:                            
         tst.b     res_flg(%a6)     # check if frestore is needed
         bne.w     do_clean         # if clear, no frestore needed
uni_no_exc:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         unlk      %a6              
         bra       finish_up        
#
# Unsupported Data Type Handler:
# Ovfl:
#   An fmoveout that results in an overflow is reported this way.
# Unfl:
#   An fmoveout that results in an underflow is reported this way.
#
# Unimplemented Instruction Handler:
# Ovfl:
#   Only scosh, setox, ssinh, stwotox, and scale can set overflow in 
#   this manner.
# Unfl:
#   Stwotox, setox, and scale can set underflow in this manner.
#   Any of the other Library Routines such that f(x)=x in which
#   x is an extended denorm can report an underflow exception. 
#   It is the responsibility of the exception-causing exception 
#   to make sure that WBTEMP is correct.
#
#   The exceptional operand is in FP_SCR1.
#
ovfl_unfl:                            
         tst.b     uflg_tmp(%a6)    # test flag for unsupp/unimp state
         beq.b     ofuf_con         
#
# The caller was from an unsupported data type trap.  Test if the
# caller set CU_ONLY.  If so, the exceptional operand is expected in
# FPTEMP, rather than WBTEMP.
#
         tst.b     cu_only(%a6)     # test if inst is cu-only
         beq.w     unse3            
#	move.w	#$fe,CU_SAVEPC(a6)
         clr.b     cu_savepc(%a6)   
         bset.b    &e1,e_byte(%a6)  # set E1 exception flag
         move.w    etemp_ex(%a6),fptemp_ex(%a6) 
         move.l    etemp_hi(%a6),fptemp_hi(%a6) 
         move.l    etemp_lo(%a6),fptemp_lo(%a6) 
         bset.b    &fptemp15_bit,dtag(%a6) # set fpte15
         bclr.b    &uflag,t_byte(%a6) # clr U flag from unimp
         bra.w     do_clean         # go clean and exit
                                    
ofuf_con:                            
         move.b    (%a7),ver_tmp(%a6) # save version number
         cmpi.b    1(%a7),&busy_size-4 # check for busy frame
         beq.b     busy_fr          # if unimp, grow to busy
         cmpi.b    (%a7),&ver_40    # test for orig unimp frame
         bne.b     try_41           # if not, test for rev frame
         moveq.l   &13,%d0          # need to zero 14 lwords
         bra.b     ofuf_fin         
try_41:                             
         cmpi.b    (%a7),&ver_41    # test for rev unimp frame
         bne.l     fpsp_fmt_error   # if neither, exit with error
         moveq.l   &11,%d0          # need to zero 12 lwords
                                    
ofuf_fin:                            
         clr.l     (%a7)            
loop1:                              
         clr.l     -(%a7)           # clear and dec a7
         dbra.w    %d0,loop1        
         move.b    ver_tmp(%a6),(%a7) 
         move.b    &busy_size-4,1(%a7) # write busy fmt word.
busy_fr:                            
         move.l    fp_scr1(%a6),wbtemp_ex(%a6) # write
         move.l    fp_scr1+4(%a6),wbtemp_hi(%a6) # execptional op to
         move.l    fp_scr1+8(%a6),wbtemp_lo(%a6) # wbtemp
         bset.b    &e3,e_byte(%a6)  # set E3 flag
         bclr.b    &e1,e_byte(%a6)  # make sure E1 is clear
         bclr.b    &uflag,t_byte(%a6) # clr U flag
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
         move.l    cmdreg1b(%a6),%d0 # fix cmd1b to make it
         and.l     &0x03c30000,%d0  # work for cmd3b
         bfextu    cmdreg1b(%a6){&13:&1},%d1 # extract bit 2
         lsl.l     &5,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put it in the right place
         bfextu    cmdreg1b(%a6){&10:&3},%d1 # extract bit 3,4,5
         lsl.l     &2,%d1           
         swap      %d1              
         or.l      %d1,%d0          # put them in the right place
         move.l    %d0,cmdreg3b(%a6) # in the busy frame
                                    
#
# Check if the frame to be restored is busy or unimp.
#** NOTE *** Bug fix for errata (0d43b #3)
# If the frame is unimp, we must create a busy frame to 
# fix the bug with the nmnexc bits in cases in which they
# are set by a previous instruction and not cleared by
# the save. The frame will be unimp only if the final 
# instruction in an emulation routine caused the exception
# by doing an fmove <ea>,fp0.  The exception operand, in
# internal format, is in fptemp.
#
do_clean:                            
         cmpi.b    1(%a7),&unimp_40_size-4 
         bne.b     do_con           
         moveq.l   &13,%d0          # in orig, need to zero 14 lwords
         bra.b     do_build         
do_con:                             
         cmpi.b    1(%a7),&unimp_41_size-4 
         bne.b     do_restore       # frame must be busy
         moveq.l   &11,%d0          # in rev, need to zero 12 lwords
                                    
do_build:                            
         move.b    (%a7),ver_tmp(%a6) 
         clr.l     (%a7)            
loop2:                              
         clr.l     -(%a7)           # clear and dec a7
         dbra.w    %d0,loop2        
#
# Use a1 as pointer into new frame.  a6 is not correct if an unimp or
# busy frame was created as the result of an exception on the final
# instruction of an emulation routine.
#
# We need to set the nmcexc bits if the exception is E1. Otherwise,
# the exc taken will be inex2.
#
         lea.l     busy_size+local_size(%a7),%a1 # init a1 for new frame
         move.b    ver_tmp(%a6),(%a7) # write busy fmt word
         move.b    &busy_size-4,1(%a7) 
         move.l    fp_scr1(%a6),wbtemp_ex(%a1) # write
         move.l    fp_scr1+4(%a6),wbtemp_hi(%a1) # exceptional op to
         move.l    fp_scr1+8(%a6),wbtemp_lo(%a1) # wbtemp
#	btst.b	#E1,E_BYTE(a1)
#	beq.b	do_restore
         bfextu    user_fpsr(%a6){&17:&4},%d0 # get snan/operr/ovfl/unfl bits
         bfins     %d0,nmcexc(%a1){&4:&4} # and insert them in nmcexc
         move.l    user_fpsr(%a6),fpsr_shadow(%a1) # set exc bits
         or.l      &sx_mask,e_byte(%a1) 
                                    
do_restore:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
#
# If trace mode enabled, then go to trace handler.  This handler 
# cannot have any fp instructions.  If there are fp inst's and an 
# exception has been restored into the machine then the exception 
# will occur upon execution of the fp inst.  This is not desirable 
# in the kernel (supervisor mode).  See MC68040 manual Section 9.3.8.
#
finish_up:                            
         btst.b    &7,(%a7)         # test T1 in SR
         bne.b     g_trace          
         btst.b    &6,(%a7)         # test T0 in SR
         bne.b     g_trace          
         bra.l     fpsp_done        
#
# Change integer stack to look like trace stack
# The address of the instruction that caused the
# exception is already in the integer stack (is
# the same as the saved friar)
#
# If the current frame is already a 6-word stack then all
# that needs to be done is to change the vector# to TRACE.
# If the frame is only a 4-word stack (meaning we got here
# on an Unsupported data type exception), then we need to grow
# the stack an extra 2 words and get the FPIAR from the FPU.
#
g_trace:                            
         bftst     exc_vec-4(%sp){&0:&4} 
         bne       g_easy           
                                    
         sub.w     &4,%sp           
         move.l    4(%sp),(%sp)     
         move.l    8(%sp),4(%sp)    
         sub.w     &busy_size,%sp   
         fsave     (%sp)            
         fmove     %fpiar,busy_size+exc_ea-4(%sp) 
         frestore  (%sp)            
         add.w     &busy_size,%sp   
                                    
g_easy:                             
         move.w    &trace_vec,exc_vec-4(%a7) 
         bra.l     real_trace       
#
                                    
	version 3
