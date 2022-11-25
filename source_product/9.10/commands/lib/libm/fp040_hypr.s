 # HPASM Generated
#
#
# MC68040 Math Library - main routine. 10-April-89 Chris Mills.
#
#    --------------------------------------------------------------------------
#   |   THE FOLLOWING PROGRAMS ARE THE SOLE PROPERTY OF APOLLO COMPUTER INC.   |
#   |         AND CONTAIN ITS PROPRIETARY AND CONFIDENTIAL INFORMATION.        |
#    --------------------------------------------------------------------------
#                            
# 90/02/09  mills   add 68881 to cpu type 
# 90/01/18  mills   er, make that fmathtops, log10, and hyperbolics, minus fmod; also delete e62
# 90/01/18  mills   fmathtops only in syslib_881 version
# 90/01/02  nfo     update register usage file
# 89/12/15  nfo     update register usage file
# 89/12/15  mills   added _fastuii entry points 
# 89/12/04  nfo     update register usage file
# 89/11/30  nfo     change refs to sutlx and dutlx to futlx
# 89/10/12  mills   added tentox and twotox (kernel space only)
# 89/10/09  mills   added __fp040_$(d)log2 calls
# 89/09/11  mills   added all the hyperbolic functions and asin and acos
# 89/09/08  mills   added __fp040_$mod
# 89/08/28  mills   changed reg usage chart
# 89/08/17  mills   added __fp040_$frem, __fp040_$fgetexp
# 89/08/04  mills   deleted __fp040_$sint, __fp040_sintrz (dp versions are identical), 
#                   and changed dintrz to fintrz, dintrnd to fint
# 89/08/03  mills   added syslib_881_version directive, code, added __fp040_$fixl
# 89/07/14  mills   Added __fp040_user_space directive to allow building just dp stuff for interlude handler
# 89/06/23  mills   Added e21,e22,e61,e62,e66 power routines      
# 89/05/16  mills   Added __fp040_$sintrz, __fp040_dintrz, __fp040_sintrnd, __fp040_$dintrnd
# 89/05/11  mills   Added register use chart
# 89/04/10  mills   Initial implementation
#
# >>>> main routine that includes everything else
                                    

                                    



                                    
#%ifdef syslib_881_version %then %else
#%var syslib_881_version
#%endif
#
# If building for the 040 kernel, then must not select 881 versions
#

#
# Notes: 
#
#    [] Power routines need to be performance tuned, particularly in the area of treating the pipeline properly.
#    [] Check for deliberately generated traps being triggered where they occur before exiting
#    [] Preserve registers only in user space versions, since fp reg preservation is needed over
#       a wider area in the emulation routines.                                  
#
#    [] BUG : __fp040_$dsin( 16#3fe4000000000000) drives test1 crazy, bozo results, argument corrupted, etc.
#    [] BUG : __fp040_$dacos(0.5) works in dp mode but not in xp mode 
#     
#    [] Check for monotonicity where approx algorithm shifts from poly to rslt=x, or worse, rslt=K-x.
#
#    [] Review u,v checking branch rats-nest in atan2 and datan2, looks like either code or comments 
#       (or both) are wrong.
#
#    [] Use .x format instead of .d format wherever possible in front ends, like __fp040_$dcos.
# 
#    [] Check in POW for x, y in proper range to do pow = exp(log(x)*y), because
#       it is 5 times faster.
#
#    [] Hide working regs in POW routines in fp op shadows
#
#    [] See if there are shadows in various routines to detect arg=tiny or dnrm, to eliminate
#       excessive execution times at zero performance penalty to benign cases.
#
#    [] SP EXP arg reduction must be done using fdmul and fdsub, in case prec mode = sp.
#
#    [] SP TAN arg reduction must be done using fdmul and fdsub, in case prec mode = sp.
#
#    [] SP SIN/COS arg reduction should be done as |arg| - a*b, where mul and sub are DP
#        versions.
#
#    [] In DP SIN/COS, FMOVE fp0,fp1 is done twice when arg reduction IS required. Branch 
#       around it somehow?
#
#    [] There are two division ops in logn. They should be changed to fsdiv in the sp case, 
#       and fddiv in the dp case.
#
#    [] Tan, dtan divisions should be dsdiv, ddiv, respectively (ditto all divisions!)
#
#    [] __fp040_$datan pushes a fp register to the stack three times in one path, is it better to 
#       create a scratch fp register?
#
#    [] Review item: Do all _v1 alternate entry points (1) set CCs, and (2) preserve/restore 
#       all registers used?
#
#    [] NaNs will arrive at (d)sin,/(d)cos_huge along with infinities. Filter them and handle 
#       appropriately.            
#
#    [] Check if epsilon values for sin/cos/tan/atan can be made smaller for kernel version, 
#       where it is guaranteed that the precision mode will be XP and all traps disabled.
#
                                    
                                    

                                    

#         DATA                             
                                    
# All external references go here...
                                    
# ... none so far ...
                                    

                                    
#                  0
#  FTMP1 must always remain fp7, since Kernel interlude handler code depends in it now.
#
                           
#
#
                           
                           
#
#**************************************************************************                                        
#                                                                         *  
#  Update this chart whenever new routines are added to FP040LIB          *   
#  Last update to this chart was: 12/15/89  by  Orovitz                   *  
#                                                                         *  
#**************************************************************************                                        
#                                                                         *  
#  Register Use Chart - information made use of by code generator...      *  
#  Note : This information is not required by the V1 alternates. The V1   *
#         alternates always save/restore all registers except fp0,fp1,    *
#         d0, d1, a0, a1                                                  *
#  Note : Some routines use more regs than stated here, but in such       *
#         cases they save/restore them inside the routine (in the shadow  *
#         of other ops).                                                  *
#                                                                         *
#  FP0   : All routines                                                   *  
#  FP1   : All routines EXCEPT __fp040_$futlx, __fp040_$getexp                *  
#  FP2   : none                                                           *
#  FP7   : All routines EXCEPT __fp040_$futlx, __fp040_$fintrz, __fp040_$fint,  *
#          __fp040_$fixl, __fp040_$fmod, __fp040_$frem,                         *
#          __fp040_$e21, __fp040_$e61                                         *
#  FP3-FP6 :  None                                                        *  
#                                                                         *  
#                                                                         *
# NOTE: FTMP1 is for all time required to be FP7, t0 match interlude      *
#       handler code.                                                     *  
#                                                                         *                                                                     
#                                                                         *                                                                     
#  D0    : All routines                                                   *
#  D1    : All routines EXCEPT __fp040_$futlx, __fp040_$getexp,               *
#           __fp040_$fintrz, __fp040_$fint, __fp040_$fixl,                      *
#           __fp040_$e21, __fp040_$e61                                        *
#  D2-d7 : None                                                           *  
#                                                                         *
#  A0    : __fp040_$cos, __fp040_$sin, __fp040_$dcos, __fp040_$dsin,              *
#          __fp040_$atan, __fp040_$datan, __fp040_$atan2, __fp040_$datan2,        *
#          p040_$asin, __fp040_$dasin, __fp040_$exp, __fp040_$dexp,             *
#          __fp040_$logn, __fp040_$log10, __fp040_$log2,                        *
#          __fp040_$dlogn, __fp040_$dlog10, __fp040_$dlog2,                     *
#          __fp040_$e22, __fp040_$e66                                         *
#                                                                         *
#                                                                         *
#  A1-A7 : None (A7 used frequently, but always restored)                 *
#                                                                         *  
#**************************************************************************                                        
                                    
                                    
                                    
#**************************************************************************                                        
#                                                                         *  
#  The following routines reside in __fp040lib, but are not intended as     *  
#  fmathtops for the compiler - these routines follow normal domain/OS    *  
#  calling conventions in terms of the registers they preserve. However,  *  
#  these functions still expect their arguments in fp0 and deposit their  *                         
#  results in fp0. Some of them expect a value-parameter on the stack     *  
#  to make them behave one way or another.                                *  
#                                                                         *
# These routines also depend on the floating point condition codes being  *
# set according to the value in fp0 upon entry.                           *
#**************************************************************************                                        
#                                                                         *  
#                                                                         *  
#  __fp040_$asin   __fp040_$acos                                              *  
#  __fp040_$dasin  __fp040_$dacos                                             *  
#                                                                         *  
#  __fp040_$sinh   __fp040_$cosh    __fp040_$tanh                               *  
#  __fp040_$dsinh  __fp040_$dcosh   __fp040_$dtanh  __fp040_$datanh               *                
#                                                                         *
# Note : There is no sp version of atanh because this routine is used     *
#        only by C and the interlude hanlder, not ftn.                    *
#                                                                         *  
#                                                                         *  
#**************************************************************************                                        
                                    
                                    
#
# Alternate entry point descriptions:
#************************************
#
# Main entry points: 
#                    The main entry points of each routine rely on the compiler
#                    making use of the info in the above chart, therefore they do
#                    not explicitly preserve registers beyond the scratch set.
#                    The V1 entry points depend on the floating point condition codes 
#                    being set to reflect the operand. In the case of diadics, the 
#                    floating CCs are set to reflect the argument in FP0.
#
# Alternate entry points V1:
#                    The V1 entry points do not expect the fp CC flags to be set in 
#                    any particular way. Furthermore, they will preserve any registers
#                    outside the scratch set. These entry points are intended for use 
#                    by the FP instruction emulator inside the kernel, or by calls 
#                    out of misc_math_asm.asm to support ftn_$xxx calls, or 
#                    fpp_libm_asm.asm
#                    
#                    NOTE: Some routines have _V1 alternates ONLY, and no main routine
#                          names.
#**************************************************************************************************           
#
#
# NOTE: The syslib_881_version code is all at the bottom of __fp040_main.asm, and other 
#       include files are commented out if syslib_881_version is true .
#
#



                                    
#%include '/us/ins/fpp.ins.asm'
#
#
# MC68040 Math Library - Constant definitions include file. 
# 10-April-89 Chris Mills.
#
#    --------------------------------------------------------------------------
#   |   THE FOLLOWING PROGRAMS ARE THE SOLE PROPERTY OF APOLLO COMPUTER INC.   |
#   |         AND CONTAIN ITS PROPRIETARY AND CONFIDENTIAL INFORMATION.        |
#    --------------------------------------------------------------------------
#
# 90/04/09  mills   fsincos_reduction is half off what it should be (2*pi)
# 90/01/25  mills   inaccurate log_c3, c4
# 90/01/12  mills   new stuff needed for fixes to new dexp
# 90/01/02  nfo     insert constants for optimized updated SEXP and DEXP.
# 89/12/28  mills   etemp_offset_plus4/plus8 incorrect
# 89/12/08  nfo     fix SP 2/pi and 1/pi (last bit)
# 89/11/28  mills   all new stuff for satan
# 89/11/21  mills   all new stuff for slogn
# 89/11/09  mills   all new stuff for dlogn
# 89/10/26  nfo     filled out some dp constants whose ls had been left zero
# 89/10/25  nfo     added FPSTATUS bit definitions
# 89/10/12  mills   added logn(2) and logn(10)
# 89/10/09  mills   added log2 conts (c4)
# 89/06/27  mills   Minor changes 
# 89/04/10  mills   Initial implementation
#
#  FP Condition Code bit defs
                           # NaN bit
                           # zero bit
                           # neg bit
#
# Misc constants for general use
                           
                           
                           
                           
                           
                           
                           # ( pi/4 ) 
                           # ( pi/4 - lsb )
                           
                           # ( pi/4 ms - lsb ) 
                           
                           
                           
                           
                           
                           # ( ln(2) = .6931471805599453 )
                           # ( 1/ln(2) = 1.4426950408889630 )
                           # ( 16/ln(2) = 23.0831... )
                           # ( 1/ln(2) = 1.4426950408889630 )
                           # ( 64/ln(2) = 92.3325...)
                           # ( ln(2) / 2.0 )
                           # ( ln(2) / 2.0 )
                           # ( ln(2)/16 = .04332... )
                           
                           
                           
                                    

                                    

                                    

                                    


                                    

                                    

                                    
 
   
         global    __fp040_sinh       
         global    __fp040_tanh       
                               
 
         global    __fp040_cosh       
         global    __fp040_dcosh      

         global    __fp040_dsinh      
         global    __fp040_dtanh      
# There is no sp atanh routine, can be called only from C applications (= dp)

#
#
# MC68040 Math Library - hyperbolic routines. 12-Sept-89 Chris Mills.
#
#    --------------------------------------------------------------------------
#   |   THE FOLLOWING PROGRAMS ARE THE SOLE PROPERTY OF APOLLO COMPUTER INC.   |
#   |         AND CONTAIN ITS PROPRIETARY AND CONFIDENTIAL INFORMATION.        |
#    --------------------------------------------------------------------------
#
# 90/07/12 mills   delete commented bug fixes
# 90/06/07 mills   back off hwbugfixes for d37a mask rev
# 90/05/25 mills   misc hwbugfix adjustments
# 90/05/02 mills   every fmovem must be preceeded by a fnop
# 90/04/30 mills   nop after every Bcc and FBcc op
# 90/02/15 mills   Bugs in datanh
# 90/01/18 mills   Back out last change
# 90/01/18 mills   this entire file is a NOP in syslib_881 version
# 89/11/30 nfo     eliminate saving fpcontrol in reg. d2, set Fccs before exit
# 89/09/12 mills   Initial implementation
#
#************************************************************************
                                    
                                    # return location
                           # 0=asin, non-0 = acos    
                                    
#************************************************************************
#
  
#************************************************************************
#      F P 0 4 0 _ $ S I N H  / F P 0 4 0 _ $ C O S H   ( H P - U X )
#************************************************************************
 
__fp040_sinh:                            # PROCEDURE __hpasm_string1,nocode
ifdef(`PROFILE',` 
         mov.l     &p___fp040_sinh,%a0 
         bsr       mcount           
         data                       
p___fp040_sinh: long      0                
         text                       
')
         mov.l     &0,-(%sp)        
         bsr       __fp040_sinh_hack  
         addq.l    &4,%sp           
                                    # RETURN __fp040_$sinh
         rts                        
                                    
__fp040_cosh:                            # PROCEDURE __hpasm_string1,nocode
ifdef(`PROFILE',` 
         mov.l     &p___fp040_cosh,%a0 
         bsr       mcount           
         data                       
p___fp040_cosh: long      0                
         text                       
')
         mov.l     &1,-(%sp)        
         bsr       __fp040_sinh_hack  
         addq.l    &4,%sp           
                                    # RETURN __fp040_$cosh
         rts                        
                                    
__fp040_dsinh:                            # PROCEDURE __hpasm_string1,nocode
ifdef(`PROFILE',` 
         mov.l     &p___fp040_dsinh,%a0 
         bsr       mcount           
         data                       
p___fp040_dsinh: long      0                
         text                       
')
         mov.l     &0,-(%sp)        
         bsr       __fp040_dsinh_hack 
         addq.l    &4,%sp           
                                    # RETURN __fp040_$dsinh
         rts                        
                                    
__fp040_dcosh:                            # PROCEDURE __hpasm_string1,nocode
ifdef(`PROFILE',` 
         mov.l     &p___fp040_dcosh,%a0 
         bsr       mcount           
         data                       
p___fp040_dcosh: long      0                
         text                       
')
         mov.l     &1,-(%sp)        
         bsr       __fp040_dsinh_hack 
         addq.l    &4,%sp           
                                    # RETURN __fp040_$dcosh
         rts                        

#************************************************************************
#      F P 0 4 0 _ $ S I N H 
#************************************************************************
__fp040_sinh_hack:                            # PROCEDURE __hpasm_string1,nocode
#
# This routine takes a parameter on the stack: 0 = sinh, non-0 = cosh.
#
#      IF (ABS(X).GT.2.44141E-04) THEN
#           F=EXP(X)             
#           FTN_$SINH_FPX=(F-1./F)/2.
#      ELSE 
#           FTN_$SINH_FPX=X
#                                    
         fbun      sinh_exit        # quit if arg is a nan
         fabs      %fp0,%fp1        
         fcmp.s    %fp1,&0x39800000 # if X < epsilon then rslt = x
         fblt      sinh_tiny        
sinh_tiny_merge: bsr       __fp040_dexp       # exp(x)
         fmove.l   &1,%fp1          
         fdiv      %fp0,%fp1        # 1./exp(f)
         tst.l     4(%sp)          
         bne       sinh_cosh        
         fsub      %fp1,%fp0        # exp(f) - 1./exp(f)
         bra       sinh_merge       # cosh = (exp(f) + 1./exp(f))/2.
sinh_cosh: fadd      %fp1,%fp0        # exp(f) + 1./exp(f)                
sinh_merge: fmul.s    &0x3f000000,%fp0 # sinh = (exp(f) - 1./exp(f))/2.
         bra       sinh_exit        
                                    
sinh_tiny: tst.l     4(%sp)          
         bne       sinh_tiny_merge  
         fmove     %fp0,%fp0        # set fccs
                                    
sinh_exit:                            # RETURN __fp040_$sinh
         rts                        

#************************************************************************
#      F P 0 4 0 _ $ D S I N H 
#************************************************************************
__fp040_dsinh_hack:                            # PROCEDURE __hpasm_string1,nocode
#
#      IF (DABS(X).GT.7.4505D-09)THEN 
#           F=DEXP(X)    
#           FTN_$DSINH=(F-1D0/F)/2D0 
#      ELSE 
#           FTN_$DSINH=X
#
         fbun      dsinh_exit       # quit if arg is a nan
         fabs      %fp0,%fp1        
         fcmp.d    %fp1,&0x3e40000000000000 # if X < epsilon then rslt = x
         fblt      dsinh_tiny       
dsinh_tiny_merge: bsr       __fp040_dexp       # exp(x)
         fmove.l   &1,%fp1          
         fdiv      %fp0,%fp1        # 1./exp(f)
         tst.l     4(%sp)          
         bne       dsinh_dcosh      
         fsub      %fp1,%fp0        # exp(f) - 1./exp(f)
         bra       dsinh_merge      # cosh = (exp(f) + 1./exp(f))/2.
dsinh_dcosh: fadd      %fp1,%fp0        # exp(f) + 1./exp(f)                
dsinh_merge: fmul.s    &0x3f000000,%fp0 # dsinh = (exp(f) - 1./exp(f))/2.
         bra       dsinh_exit       
                                    
dsinh_tiny: tst.l     4(%sp)          
         bne       dsinh_tiny_merge 
         fmove     %fp0,%fp0        # set fccs
                                    
dsinh_exit:                            # RETURN __fp040_$dsinh
         rts                        
                                    
  
#************************************************************************
#      F P 0 4 0 _ $ T A N H 
#************************************************************************
__fp040_tanh:                            # PROCEDURE __hpasm_string1,nocode
#                   
#     IF (X.GT.445) THEN
#        FTN_$TANH= 1.0 
#     ELSE IF (X.LT.-455.0) THEN
#        FTN_$TANH=-1.0 
#     ELSE IF(ABS(X).GT.2.4414E-04) THEN 
#        F=EXP(2*X)            { Must be > 0! }
#        FTN_$TANH=(F-1)/(F+1)    
#     ELSE
#        FTN_$TANH=X
#
ifdef(`PROFILE',` 
         mov.l     &p___fp040_tanh,%a0 
         bsr       mcount           
         data                       
p___fp040_tanh: long      0                
         text                       
')
         fbun      tanh_exit        
         fabs      %fp0,%fp1        
         fcmp.s    %fp1,&0x42320000 
         fbgt      tanh_x_big       
tanh_merge: fcmp.s    %fp1,&0x39800000 
         fble      tanh_tiny        # if x < epsilon then rslt = x
         fmul.s    &0x40000000,%fp0 # 2*x
         bsr       __fp040_exp        # exp(2*x) -> fp0
         fmove     %fp0,%fp1        
         fadd.s    &0x3f800000,%fp1 
         fsub.s    &0x3f800000,%fp0 
         fdiv      %fp1,%fp0        
         bra       tanh_exit        
                                    
tanh_x_big: ftest     %fp0             
         fblt      tanh_big_neg     
         fmove.l   &1,%fp0          
         bra       tanh_exit        
                                    
tanh_big_neg: fmove.l   &-1,%fp0         
         bra       tanh_exit        
                                    
tanh_tiny: fmove     %fp0,%fp0        # set fccs
         bra       dtanh_exit       
                                    
tanh_exit:                            # RETURN __fp040_$tanh
         rts                        

#************************************************************************
#      F P 0 4 0 _ $ D T A N H 
#************************************************************************
__fp040_dtanh:                            # PROCEDURE __hpasm_string1,nocode
#                   
#     IF (X.GT.355.0) THEN
#        FTN_$TANH= 1.0 
#     ELSE IF (X.LT.-355.0) THEN
#        FTN_$TANH=-1.0 
#     ELSE IF(ABS(X).GT.4505.0E-09) THEN 
#        F=EXP(2*X)            { Must be > 0! }
#        FTN_$TANH=(F-1)/(F+1)    
#     ELSE
#        FTN_$TANH=X
#
ifdef(`PROFILE',` 
         mov.l     &p___fp040_dtanh,%a0 
         bsr       mcount           
         data                       
p___fp040_dtanh: long      0                
         text                       
')
         fbun      dtanh_exit       
         fabs      %fp0,%fp1        
         fcmp.d    %fp1,&0x4076300000000000 
         fbgt      dtanh_x_big      
dtanh_merge: fcmp.d    %fp1,&0x3ed2e53400000000 
         fble      dtanh_tiny       # if x < epsilon then rslt = x
         fmul.l    &2,%fp0          # 2*x
         bsr       __fp040_dexp       # exp(2*x) -> F
         fmove     %fp0,%fp1        
         fadd.l    &1,%fp1          # F+1
         fsub.l    &1,%fp0          # F-1
         fdiv      %fp1,%fp0        # (F-1)/(F+1)
         bra       dtanh_exit       
                                    
dtanh_x_big: ftest     %fp0             
         fblt      dtanh_big_neg    
         fmove.l   &1,%fp0          
         bra       dtanh_exit       
                                    
dtanh_big_neg: fmove.l   &-1,%fp0         
         bra       dtanh_exit       
                                    
dtanh_tiny: fmove     %fp0,%fp0        # set fccs
         bra       dtanh_exit       
                                    
dtanh_exit:                            # RETURN __fp040_$dtanh
         rts                        
#************************************************************************
#      F P 0 4 0 _ $ D A T A N H 
#************************************************************************


                                    
	version 3
