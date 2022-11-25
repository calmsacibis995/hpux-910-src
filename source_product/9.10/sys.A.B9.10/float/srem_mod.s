ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/srem_mod.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:53:05 $
                                    
#
#	srem_mod.sa 3.1 12/10/90
#
#      The entry point sMOD computes the floating point MOD of the
#      input values X and Y. The entry point sREM computes the floating
#      point (IEEE) REM of the input values X and Y.
#
#      INPUT
#      -----
#      Double-extended value Y is pointed to by address in register
#      A0. Double-extended value X is located in -12(A0). The values
#      of X and Y are both nonzero and finite; although either or both
#      of them can be denormalized. The special cases of zeros, NaNs,
#      and infinities are handled elsewhere.
#
#      OUTPUT
#      ------
#      FREM(X,Y) or FMOD(X,Y), depending on entry point.
#
#       ALGORITHM
#       ---------
#
#       Step 1.  Save and strip signs of X and Y: signX := sign(X),
#                signY := sign(Y), X := |X|, Y := |Y|, 
#                signQ := signX EOR signY. Record whether MOD or REM
#                is requested.
#
#       Step 2.  Set L := expo(X)-expo(Y), k := 0, Q := 0.
#                If (L < 0) then
#                   R := X, go to Step 4.
#                else
#                   R := 2^(-L)X, j := L.
#                endif
#
#       Step 3.  Perform MOD(X,Y)
#            3.1 If R = Y, go to Step 9.
#            3.2 If R > Y, then { R := R - Y, Q := Q + 1}
#            3.3 If j = 0, go to Step 4.
#            3.4 k := k + 1, j := j - 1, Q := 2Q, R := 2R. Go to
#                Step 3.1.
#
#       Step 4.  At this point, R = X - QY = MOD(X,Y). Set
#                Last_Subtract := false (used in Step 7 below). If
#                MOD is requested, go to Step 6. 
#
#       Step 5.  R = MOD(X,Y), but REM(X,Y) is requested.
#            5.1 If R < Y/2, then R = MOD(X,Y) = REM(X,Y). Go to
#                Step 6.
#            5.2 If R > Y/2, then { set Last_Subtract := true,
#                Q := Q + 1, Y := signY*Y }. Go to Step 6.
#            5.3 This is the tricky case of R = Y/2. If Q is odd,
#                then { Q := Q + 1, signX := -signX }.
#
#       Step 6.  R := signX*R.
#
#       Step 7.  If Last_Subtract = true, R := R - Y.
#
#       Step 8.  Return signQ, last 7 bits of Q, and R as required.
#
#       Step 9.  At this point, R = 2^(-j)*X - Q Y = Y. Thus,
#                X = 2^(j)*(Q+1)Y. set Q := 2^(j)*(Q+1),
#                R := 0. Return signQ, last 7 bits of Q, and R.
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
                                    
         set       mod_flag,l_scr3  
         set       signy,fp_scr3+4  
         set       signx,fp_scr3+8  
         set       signq,fp_scr3+12 
         set       sc_flag,fp_scr4  
                                    
         set       y,fp_scr1        
         set       y_hi,y+4         
         set       y_lo,y+8         
                                    
         set       r,fp_scr2        
         set       r_hi,r+4         
         set       r_lo,r+8         
                                    
                                    
scale:   long      0x00010000,0x80000000,0x00000000,0x00000000 
                                    
                                    
                                    
         global    smod             
smod:                               
                                    
         move.l    &0,mod_flag(%a6) 
         bra.b     mod_rem          
                                    
         global    srem             
srem:                               
                                    
         move.l    &1,mod_flag(%a6) 
                                    
mod_rem:                            
#..Save sign of X and Y
         movem.l   %d2-%d7,-(%a7)   # save data registers
         move.w    (%a0),%d3        
         move.w    %d3,signy(%a6)   
         andi.l    &0x00007fff,%d3  # Y := |Y|
                                    
#
         move.l    4(%a0),%d4       
         move.l    8(%a0),%d5       # (D3,D4,D5) is |Y|
                                    
         tst.l     %d3              
         bne.b     y_normal         
                                    
         move.l    &0x00003ffe,%d3  # $3FFD + 1
         tst.l     %d4              
         bne.b     hiy_not0         
                                    
hiy_0:                              
         move.l    %d5,%d4          
         clr.l     %d5              
         subi.l    &32,%d3          
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         lsl.l     %d6,%d4          
         sub.l     %d6,%d3          # (D3,D4,D5) is normalized
#                                       ...with bias $7FFD
         bra.b     chk_x            
                                    
hiy_not0:                            
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         sub.l     %d6,%d3          
         lsl.l     %d6,%d4          
         move.l    %d5,%d7          # a copy of D5
         lsl.l     %d6,%d5          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d4          # (D3,D4,D5) normalized
#                                       ...with bias $7FFD
         bra.b     chk_x            
                                    
y_normal:                            
         addi.l    &0x00003ffe,%d3  # (D3,D4,D5) normalized
#                                       ...with bias $7FFD
                                    
chk_x:                              
         move.w    -12(%a0),%d0     
         move.w    %d0,signx(%a6)   
         move.w    signy(%a6),%d1   
         eor.l     %d0,%d1          
         andi.l    &0x00008000,%d1  
         move.w    %d1,signq(%a6)   # sign(Q) obtained
         andi.l    &0x00007fff,%d0  
         move.l    -8(%a0),%d1      
         move.l    -4(%a0),%d2      # (D0,D1,D2) is |X|
         tst.l     %d0              
         bne.b     x_normal         
         move.l    &0x00003ffe,%d0  
         tst.l     %d1              
         bne.b     hix_not0         
                                    
hix_0:                              
         move.l    %d2,%d1          
         clr.l     %d2              
         subi.l    &32,%d0          
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         lsl.l     %d6,%d1          
         sub.l     %d6,%d0          # (D0,D1,D2) is normalized
#                                       ...with bias $7FFD
         bra.b     init             
                                    
hix_not0:                            
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         sub.l     %d6,%d0          
         lsl.l     %d6,%d1          
         move.l    %d2,%d7          # a copy of D2
         lsl.l     %d6,%d2          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d1          # (D0,D1,D2) normalized
#                                       ...with bias $7FFD
         bra.b     init             
                                    
x_normal:                            
         addi.l    &0x00003ffe,%d0  # (D0,D1,D2) normalized
#                                       ...with bias $7FFD
                                    
init:                               
#
         move.l    %d3,l_scr1(%a6)  # save biased expo(Y)
         move.l    %d0,l_scr2(%a6)  # save d0
         sub.l     %d3,%d0          # L := expo(X)-expo(Y)
#   Move.L               D0,L            ...D0 is j
         clr.l     %d6              # D6 := carry <- 0
         clr.l     %d3              # D3 is Q
         movea.l   &0,%a1           # A1 is k; j+k=L, Q=0
                                    
#..(Carry,D1,D2) is R
         tst.l     %d0              
         bge.b     mod_loop         
                                    
#..expo(X) < expo(Y). Thus X = mod(X,Y)
#
         move.l    l_scr2(%a6),%d0  # restore d0
         bra.w     get_mod          
                                    
#..At this point  R = 2^(-L)X; Q = 0; k = 0; and  k+j = L
                                    
                                    
mod_loop:                            
         tst.l     %d6              # test carry bit
         bgt.b     r_gt_y           
                                    
#..At this point carry = 0, R = (D1,D2), Y = (D4,D5)
         cmp.l     %d1,%d4          # compare hi(R) and hi(Y)
         bne.b     r_ne_y           
         cmp.l     %d2,%d5          # compare lo(R) and lo(Y)
         bne.b     r_ne_y           
                                    
#..At this point, R = Y
         bra.w     rem_is_0         
                                    
r_ne_y:                             
#..use the borrow of the previous compare
         bcs.b     r_lt_y           # borrow is set iff R < Y
                                    
r_gt_y:                             
#..If Carry is set, then Y < (Carry,D1,D2) < 2Y. Otherwise, Carry = 0
#..and Y < (D1,D2) < 2Y. Either way, perform R - Y
         sub.l     %d5,%d2          # lo(R) - lo(Y)
         subx.l    %d4,%d1          # hi(R) - hi(Y)
         clr.l     %d6              # clear carry
         addq.l    &1,%d3           # Q := Q + 1
                                    
r_lt_y:                             
#..At this point, Carry=0, R < Y. R = 2^(k-L)X - QY; k+j = L; j >= 0.
         tst.l     %d0              # see if j = 0.
         beq.b     postloop         
                                    
         add.l     %d3,%d3          # Q := 2Q
         add.l     %d2,%d2          # lo(R) = 2lo(R)
         roxl.l    &1,%d1           # hi(R) = 2hi(R) + carry
         scs       %d6              # set Carry if 2(R) overflows
         addq.l    &1,%a1           # k := k+1
         subq.l    &1,%d0           # j := j - 1
#..At this point, R=(Carry,D1,D2) = 2^(k-L)X - QY, j+k=L, j >= 0, R < 2Y.
                                    
         bra.b     mod_loop         
                                    
postloop:                            
#..k = L, j = 0, Carry = 0, R = (D1,D2) = X - QY, R < Y.
                                    
#..normalize R.
         move.l    l_scr1(%a6),%d0  # new biased expo of R
         tst.l     %d1              
         bne.b     hir_not0         
                                    
hir_0:                              
         move.l    %d2,%d1          
         clr.l     %d2              
         subi.l    &32,%d0          
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         lsl.l     %d6,%d1          
         sub.l     %d6,%d0          # (D0,D1,D2) is normalized
#                                       ...with bias $7FFD
         bra.b     get_mod          
                                    
hir_not0:                            
         clr.l     %d6              
         bfffo     %d1{&0:&32},%d6  
         bmi.b     get_mod          # already normalized
         sub.l     %d6,%d0          
         lsl.l     %d6,%d1          
         move.l    %d2,%d7          # a copy of D2
         lsl.l     %d6,%d2          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d1          # (D0,D1,D2) normalized
                                    
#
get_mod:                            
         cmpi.l    %d0,&0x000041fe  
         bge.b     no_scale         
do_scale:                            
         move.w    %d0,r(%a6)       
         clr.w     r+2(%a6)         
         move.l    %d1,r_hi(%a6)    
         move.l    %d2,r_lo(%a6)    
         move.l    l_scr1(%a6),%d6  
         move.w    %d6,y(%a6)       
         clr.w     y+2(%a6)         
         move.l    %d4,y_hi(%a6)    
         move.l    %d5,y_lo(%a6)    
         fmove.x   r(%a6),%fp0      # no exception
         move.l    &1,sc_flag(%a6)  
         bra.b     modorrem         
no_scale:                            
         move.l    %d1,r_hi(%a6)    
         move.l    %d2,r_lo(%a6)    
         subi.l    &0x3ffe,%d0      
         move.w    %d0,r(%a6)       
         clr.w     r+2(%a6)         
         move.l    l_scr1(%a6),%d6  
         subi.l    &0x3ffe,%d6      
         move.l    %d6,l_scr1(%a6)  
         fmove.x   r(%a6),%fp0      
         move.w    %d6,y(%a6)       
         move.l    %d4,y_hi(%a6)    
         move.l    %d5,y_lo(%a6)    
         move.l    &0,sc_flag(%a6)  
                                    
#
                                    
                                    
modorrem:                            
         move.l    mod_flag(%a6),%d6 
         beq.b     fix_sign         
                                    
         move.l    l_scr1(%a6),%d6  # new biased expo(Y)
         subq.l    &1,%d6           # biased expo(Y/2)
         cmp.l     %d0,%d6          
         blt.b     fix_sign         
         bgt.b     last_sub         
                                    
         cmp.l     %d1,%d4          
         bne.b     not_eq           
         cmp.l     %d2,%d5          
         bne.b     not_eq           
         bra.w     tie_case         
                                    
not_eq:                             
         bcs.b     fix_sign         
                                    
last_sub:                            
#
         fsub.x    y(%a6),%fp0      # no exceptions
         addq.l    &1,%d3           # Q := Q + 1
                                    
#
                                    
fix_sign:                            
#..Get sign of X
         move.w    signx(%a6),%d6   
         bge.b     get_q            
         fneg.x    %fp0             
                                    
#..Get Q
#
get_q:                              
         clr.l     %d6              
         move.w    signq(%a6),%d6   # D6 is sign(Q)
         move.l    &8,%d7           
         lsr.l     %d7,%d6          
         andi.l    &0x0000007f,%d3  # 7 bits of Q
         or.l      %d6,%d3          # sign and bits of Q
         swap      %d3              
         fmove.l   %fpsr,%d6        
         andi.l    &0xff00ffff,%d6  
         or.l      %d3,%d6          
         fmove.l   %d6,%fpsr        # put Q in fpsr
                                    
#
restore:                            
         movem.l   (%a7)+,%d2-%d7   
         fmove.l   user_fpcr(%a6),%fpcr 
         move.l    sc_flag(%a6),%d0 
         beq.b     finish           
         fmul.x    scale(%pc),%fp0  # may cause underflow
         bra.l     t_avoid_unsupp   # check for denorm as a
#					;result of the scaling
                                    
finish:                             
         fmove.x   %fp0,%fp0        # capture exceptions & round
         rts                        
                                    
rem_is_0:                            
#..R = 2^(-j)X - Q Y = Y, thus R = 0 and quotient = 2^j (Q+1)
         addq.l    &1,%d3           
         cmpi.l    %d0,&8           # D0 is j 
         bge.b     q_big            
                                    
         lsl.l     %d0,%d3          
         bra.b     set_r_0          
                                    
q_big:                              
         clr.l     %d3              
                                    
set_r_0:                            
         fmove.s   &0f0.0,%fp0      
         move.l    &0,sc_flag(%a6)  
         bra.w     fix_sign         
                                    
tie_case:                            
#..Check parity of Q
         move.l    %d3,%d6          
         andi.l    &0x00000001,%d6  
         tst.l     %d6              
         beq.w     fix_sign         # Q is even
                                    
#..Q is odd, Q := Q + 1, signX := -signX
         addq.l    &1,%d3           
         move.w    signx(%a6),%d6   
         eori.l    &0x00008000,%d6  
         move.w    %d6,signx(%a6)   
         bra.w     fix_sign         
                                    
                                    
	version 3
