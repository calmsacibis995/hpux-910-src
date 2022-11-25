ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/skeleton.s,v $
# $Revision: 1.3.84.3 $	$Author: kcs $
# $State: Exp $   	$Locker:  $
# $Date: 93/09/17 20:52:09 $
                                    
#
#	skeleton.sa 3.1 12/10/90
#
#	This file contains code that is system dependent and will
#	need to be modified to install the FPSP.
#
#	Each entry point for exception 'xxxx' begins with a 'jmp fpsp_xxxx'.
#	Put any target system specific handling that must be done immediately
#	before the jump instruction.  If there no handling necessary, then
#	the 'fpsp_xxxx' handler entry point should be placed in the exception
#	table so that the 'jmp' can be eliminated. If the FPSP determines that the
#	exception is one that must be reported then there will be a
#	return from the package by a 'jmp real_xxxx'.  At that point
#	the machine state will be identical to the state before
#	the FPSP was entered.  In particular, whatever condition
#	that caused the exception will still be pending when the FPSP
#	package returns.  Thus, there will be system specific code
#	to handle the exception.
#
#	If the exception was completely handled by the package, then
#	the return will be via a 'jmp fpsp_done'.  Unless there is 
#	OS specific work to be done (such as handling a context switch or
#	interrupt) the user program can be resumed via 'rte'.
#
#	In the following skeleton code, some typical 'real_xxxx' handling
#	code is shown.  This code may need to be moved to an appropriate
#	place in the target system, or rewritten.
#	
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         data                       
         global    _fpsr_save       
_fpsr_save: long      0                
                                    
         text                       
                                    
	include(fpsp.h.s)
         set       m68040,3         
         set       resched,0x100    
                                    
                                    
                                    
#
#	Divide by Zero exception
#
#	All dz exceptions are 'real', hence no fpsp_dz entry point.
#
         global    _dz              
         global    real_dz          
_dz:                                
         cmp.l     _processor,&m68040 
         bne       fix_68020        
real_dz:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         bclr.b    &e1,e_byte(%a6)  
         fmove.l   %fpsr,_fpsr_save 
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Inexact exception
#
#	All inexact exceptions are real, but the 'real' handler
#	will probably want to clear the pending exception.
#	The provided code will clear the E3 exception (if pending), 
#	otherwise clear the E1 exception.  The frestore is not really
#	necessary for E1 exceptions.
#
# Code following the 'inex' label is to handle bug #1232.  In this
# bug, if an E1 snan, ovfl, or unfl occured, and the process was
# swapped out before taking the exception, the exception taken on
# return was inex, rather than the correct exception.  The snan, ovfl,
# and unfl exception to be taken must not have been enabled.  The
# fix is to check for E1, and the existence of one of snan, ovfl,
# or unfl bits set in the fpsr.  If any of these are set, branch
# to the appropriate  handler for the exception in the fpsr.  Note
# that this fix is only for d43b parts, and is skipped if the
# version number is not $40.
# 
#
         global    real_inex        
         global    _inex            
_inex:                              
         cmp.l     _processor,&m68040 
         bne       fix_68020        
                                    
         link      %a6,&-local_size 
         fsave     -(%sp)           
         cmpi.b    (%sp),&ver_40    # test version number
         bne.b     not_fmt40        
         fmove.l   %fpsr,-(%sp)     
         btst.b    &e1,e_byte(%a6)  # test for E1 set
         beq.b     not_b1232        
         btst.b    &snan_bit,2(%sp) # test for snan
         beq       inex_ckofl       
         add.l     &4,%sp           
         frestore  (%sp)+           
         unlk      %a6              
         bra       _snan            
inex_ckofl:                            
         btst.b    &ovfl_bit,2(%sp) # test for ovfl
         beq       inex_ckufl       
         add.l     &4,%sp           
         frestore  (%sp)+           
         unlk      %a6              
         bra       _ovfl            
inex_ckufl:                            
         btst.b    &unfl_bit,2(%sp) # test for unfl
         beq       not_b1232        
         add.l     &4,%sp           
         frestore  (%sp)+           
         unlk      %a6              
         bra       _unfl            
                                    
#
# We do not have the bug 1232 case.  Clean up the stack and call
# real_inex.
#
not_b1232:                            
         add.l     &4,%sp           
         frestore  (%sp)+           
         unlk      %a6              
                                    
real_inex:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
not_fmt40:                            
         fmove.l   %fpsr,_fpsr_save 
         bclr.b    &e3,e_byte(%a6)  # clear and test E3 flag
         beq.b     inex_cke1        
#
# Clear dirty bit on dest resister in the frame before branching
# to b1238_fix.
#
         movem.l   %d0/%d1,user_da(%a6) 
         bfextu    cmdreg1b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        # test for bug1238 case
         movem.l   user_da(%a6),%d0/%d1 
         bra.b     inex_done        
inex_cke1:                            
         bclr.b    &e1,e_byte(%a6)  
inex_done:                            
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Overflow exception
#
                                    
         global    real_ovfl        
         global    _ovfl            
_ovfl:                              
         cmp.l     _processor,&m68040 
         bne       fix_68020        
         jmp       fpsp_ovfl        
real_ovfl:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         fmove.l   %fpsr,_fpsr_save 
         bclr.b    &e3,e_byte(%a6)  # clear and test E3 flag
         bne.b     ovfl_done        
         bclr.b    &e1,e_byte(%a6)  
ovfl_done:                            
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Underflow exception
#
                                    
         global    real_unfl        
         global    _unfl            
_unfl:                              
         cmp.l     _processor,&m68040 
         bne       fix_68020        
         jmp       fpsp_unfl        
real_unfl:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         fmove.l   %fpsr,_fpsr_save 
         bclr.b    &e3,e_byte(%a6)  # clear and test E3 flag
         bne.b     unfl_done        
         bclr.b    &e1,e_byte(%a6)  
unfl_done:                            
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Signalling NAN exception
#
                                    
         global    real_snan        
         global    _snan            
_snan:                              
         cmp.l     _processor,&m68040 
         bne       fix_68020        
         jmp       fpsp_snan        
real_snan:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         fmove.l   %fpsr,_fpsr_save 
         bclr.b    &e1,e_byte(%a6)  # snan is always an E1 exception
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Operand Error exception
#
                                    
         global    real_operr       
         global    _operr           
_operr:                             
         cmp.l     _processor,&m68040 
         bne       fix_68020        
         jmp       fpsp_operr       
real_operr:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         bclr.b    &e1,e_byte(%a6)  # operr is always an E1 exception
         fmove.l   %fpsr,_fpsr_save 
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	BSUN exception
#
#	This sample handler simply clears the nan bit in the FPSR.
#
                                    
         global    real_bsun        
         global    _bsun            
_bsun:                              
         cmp.l     _processor,&m68040 
         bne       fix_68020        
         jmp       fpsp_bsun        
real_bsun:                            
         link      %a6,&-local_size 
         fsave     -(%sp)           
         bclr.b    &e1,e_byte(%a6)  # bsun is always an E1 exception
         fmove.l   %fpsr,-(%sp)     
         bclr.b    &nan_bit,(%sp)   
         move.l    (%sp),_fpsr_save 
         fmove.l   (%sp)+,%fpsr     
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	F-line exception
#
#	A 'real' F-line exception is one that the FPSP isn't supposed to 
#	handle. E.g. an instruction with a co-processor ID that is not 1.
#
#
                                    
         global    real_fline       
         global    _fline           
_fline:                             
         jmp       fpsp_fline       
real_fline:                            
         jmp       _fault           
                                    
#
#	Unsupported data type exception
#
                                    
         global    real_unsupp      
         global    _unsupp          
_unsupp:                            
         jmp       fpsp_unsupp      
real_unsupp:                            
# Should never get here.....
         link      %a6,&-local_size 
         fsave     -(%sp)           
         bclr.b    &e1,e_byte(%a6)  # unsupp is always an E1 exception
         frestore  (%sp)+           
         unlk      %a6              
         jmp       _fault           
                                    
#
#	Trace exception
#
         global    real_trace       
real_trace:                            
         rte                        
                                    
#
#	fpsp_fmt_error --- exit point for frame format error
#
#	The fpu stack frame does not match the frames existing
#	or planned at the time of this writing.  The fpsp is
#	unable to handle frame sizes not in the following
#	version:size pairs:
#
#	{4060, 4160} - busy frame
#	{4028, 4130} - unimp frame
#	{4000, 4100} - idle frame
#
#	This entry point simply holds an f-line illegal value.  
#	Replace this with a call to your kernel panic code or
#	code to handle future revisions of the fpu.
#
         global    fpsp_fmt_error   
fpsp_fmt_error:                            
         pea       fmt_string       
         jsr       _panic           
         long      0xf27f0000       # f-line illegal 
fmt_string:                            
         byte      "Bad Floating Point Stack Frame, Possibly Need New Kernel",10,0 
                                    
#
#	fpsp_done --- FPSP exit point
#
#	The exception has been handled by the package and we are ready
#	to return to user mode, but there may be OS specific code
#	to execute before we do.  If there is, do it now.
#
#
         global    fpsp_done        
fpsp_done:                            
         tst.l     _runrun          
         bne.w     fp_resched       
         rte                        
fp_resched:                            
         and       &0xf8ff,%sr      
         andi.w    &0xf000,6(%sp)   
         ori.w     &resched,6(%sp)  
         jmp       _fault           
                                    
#
# Fix 68020 -- used to fix up the 68020/68030 state after an exception
# See section 5.2.2 in the 68881/68882 user's manual
#
fix_68020:                            
         move.l    %d7,-(%sp)       
         fsave     -(%sp)           
         mov.b     (%sp),%d7        
         beq       null             
         clr.l     %d7              
         mov.b     1(%sp),%d7       
         bset      &3,(%sp,%d7)     
null:                               
         fmove.l   %fpsr,_fpsr_save 
         frestore  (%sp)+           
         move.l    (%sp)+,%d7       
         jmp       _fault           
                                    
#
#	mem_write --- write to user or supervisor address space
#
# Writes to memory while in supervisor mode.  copyout accomplishes
# this via a 'moves' instruction.  copyout is a UNIX SVR3 (and later) function.
# If you don't have copyout, use the local copy of the function below.
#
#	a0 - supervisor source address
#	a1 - user destination address
#	d0 - number of bytes to write (maximum count is 12)
#
# The supervisor source address is guaranteed to point into the supervisor
# stack.  The result is that a UNIX
# process is allowed to sleep as a consequence of a page fault during
# copyout.  The probability of a page fault is exceedingly small because
# the 68040 always reads the destination address and thus the page
# faults should have already been handled.
#
# If the EXC_SR shows that the exception was from supervisor space,
# then just do a dumb (and slow) memory move.  In a UNIX environment
# there shouldn't be any supervisor mode floating point exceptions.
#
         global    mem_write        
mem_write:                            
         btst.b    &5,exc_sr(%a6)   # check for supervisor state
         beq.b     user_write       
super_write:                            
         move.b    (%a0)+,(%a1)+    
         subq.l    &1,%d0           
         bne.b     super_write      
         rts                        
user_write:                            
         move.l    %d1,-(%sp)       # preserve d1 just in case
         move.l    %d0,-(%sp)       
         move.l    %a1,-(%sp)       
         move.l    %a0,-(%sp)       
         jsr       _copyout         
         add.w     &12,%sp          
         move.l    (%sp)+,%d1       
         rts                        
#
#	mem_read --- read from user or supervisor address space
#
# Reads from memory while in supervisor mode.  copyin accomplishes
# this via a 'moves' instruction.  copyin is a UNIX SVR3 (and later) function.
# If you don't have copyin, use the local copy of the function below.
#
# The FPSP calls mem_read to read the original F-line instruction in order
# to extract the data register number when the 'Dn' addressing mode is
# used.
#
#Input:
#	a0 - user source address
#	a1 - supervisor destination address
#	d0 - number of bytes to read (maximum count is 12)
#
# Like mem_write, mem_read always reads with a supervisor 
# destination address on the supervisor stack.  Also like mem_write,
# the EXC_SR is checked and a simple memory copy is done if reading
# from supervisor space is indicated.
#
         global    mem_read         
mem_read:                            
         btst.b    &5,exc_sr(%a6)   # check for supervisor state
         beq.b     user_read        
super_read:                            
         move.b    (%a0)+,(%a1)+    
         subq.l    &1,%d0           
         bne.b     super_read       
         rts                        
user_read:                            
         move.l    %d1,-(%sp)       # preserve d1 just in case
         move.l    %d0,-(%sp)       
         move.l    %a1,-(%sp)       
         move.l    %a0,-(%sp)       
         jsr       _copyin          
         add.w     &12,%sp          
         move.l    (%sp)+,%d1       
         rts                        
                                    
                                    
	version 3
