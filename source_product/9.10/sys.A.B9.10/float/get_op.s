ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/get_op.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:48:34 $
                                    
#
#	get_op.sa 3.5 4/26/91
#
#  Description: This routine is called by the unsupported format/data
# type exception handler ('unsupp' - vector 55) and the unimplemented
# instruction exception handler ('unimp' - vector 11).  'get_op'
# determines the opclass (0, 2, or 3) and branches to the
# opclass handler routine.  See 68881/2 User's Manual table 4-11
# for a description of the opclasses.
#
# For UNSUPPORTED data/format (exception vector 55) and for
# UNIMPLEMENTED instructions (exception vector 11) the following
# applies:
#
# - For unnormormalized numbers (opclass 0, 2, or 3) the
# number(s) is normalized and the operand type tag is updated.
#		
# - For a packed number (opclass 2) the number is unpacked and the
# operand type tag is updated.
#
# - For denormalized numbers (opclass 0 or 2) the number(s) is not
# changed but passed to the next module.  The next module for
# unimp is do_func, the next module for unsupp is res_func.
#
# For UNSUPPORTED data/format (exception vector 55) only the
# following applies:
#
# - If there is a move out with a packed number (opclass 3) the
# number is packed and written to user memory.  For the other
# opclasses the number(s) are written back to the fsave stack
# and the instruction is then restored back into the '040.  The
# '040 is then able to complete the instruction.
#
# For example:
# fadd.x fpm,fpn where the fpm contains an unnormalized number.
# The '040 takes an unsupported data trap and gets to this
# routine.  The number is normalized, put back on the stack and
# then an frestore is done to restore the instruction back into
# the '040.  The '040 then re-executes the fadd.x fpm,fpn with
# a normalized number in the source and the instruction is
# successful.
#		
# Next consider if in the process of normalizing the un-
# normalized number it becomes a denormalized number.  The
# routine which converts the unnorm to a norm (called mk_norm)
# detects this and tags the number as a denorm.  The routine
# res_func sees the denorm tag and converts the denorm to a
# norm.  The instruction is then restored back into the '040
# which re_executess the instruction.
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
                                    
         global    pirn,pirzrm,pirp 
         global    smalrn,smalrzrm,smalrp 
         global    bigrn,bigrzrm,bigrp 
                                    
pirn:                               
         long      0x40000000,0xc90fdaa2,0x2168c235 # pi
pirzrm:                             
         long      0x40000000,0xc90fdaa2,0x2168c234 # pi
pirp:                               
         long      0x40000000,0xc90fdaa2,0x2168c235 # pi
                                    
#round to nearest
smalrn:                             
         long      0x3ffd0000,0x9a209a84,0xfbcff798 # log10(2)
         long      0x40000000,0xadf85458,0xa2bb4a9a # e
         long      0x3fff0000,0xb8aa3b29,0x5c17f0bc # log2(e)
         long      0x3ffd0000,0xde5bd8a9,0x37287195 # log10(e)
         long      0x00000000,0x00000000,0x00000000 # 0.0
# round to zero;round to negative infinity
smalrzrm:                            
         long      0x3ffd0000,0x9a209a84,0xfbcff798 # log10(2)
         long      0x40000000,0xadf85458,0xa2bb4a9a # e
         long      0x3fff0000,0xb8aa3b29,0x5c17f0bb # log2(e)
         long      0x3ffd0000,0xde5bd8a9,0x37287195 # log10(e)
         long      0x00000000,0x00000000,0x00000000 # 0.0
# round to positive infinity
smalrp:                             
         long      0x3ffd0000,0x9a209a84,0xfbcff799 # log10(2)
         long      0x40000000,0xadf85458,0xa2bb4a9b # e
         long      0x3fff0000,0xb8aa3b29,0x5c17f0bc # log2(e)
         long      0x3ffd0000,0xde5bd8a9,0x37287195 # log10(e)
         long      0x00000000,0x00000000,0x00000000 # 0.0
                                    
#round to nearest
bigrn:                              
         long      0x3ffe0000,0xb17217f7,0xd1cf79ac # ln(2)
         long      0x40000000,0x935d8ddd,0xaaa8ac17 # ln(10)
         long      0x3fff0000,0x80000000,0x00000000 # 10 ^ 0
                                    
         global    ptenrn           
ptenrn:                             
         long      0x40020000,0xa0000000,0x00000000 # 10 ^ 1
         long      0x40050000,0xc8000000,0x00000000 # 10 ^ 2
         long      0x400c0000,0x9c400000,0x00000000 # 10 ^ 4
         long      0x40190000,0xbebc2000,0x00000000 # 10 ^ 8
         long      0x40340000,0x8e1bc9bf,0x04000000 # 10 ^ 16
         long      0x40690000,0x9dc5ada8,0x2b70b59e # 10 ^ 32
         long      0x40d30000,0xc2781f49,0xffcfa6d5 # 10 ^ 64
         long      0x41a80000,0x93ba47c9,0x80e98ce0 # 10 ^ 128
         long      0x43510000,0xaa7eebfb,0x9df9de8e # 10 ^ 256
         long      0x46a30000,0xe319a0ae,0xa60e91c7 # 10 ^ 512
         long      0x4d480000,0xc9767586,0x81750c17 # 10 ^ 1024
         long      0x5a920000,0x9e8b3b5d,0xc53d5de5 # 10 ^ 2048
         long      0x75250000,0xc4605202,0x8a20979b # 10 ^ 4096
#round to minus infinity
bigrzrm:                            
         long      0x3ffe0000,0xb17217f7,0xd1cf79ab # ln(2)
         long      0x40000000,0x935d8ddd,0xaaa8ac16 # ln(10)
         long      0x3fff0000,0x80000000,0x00000000 # 10 ^ 0
                                    
         global    ptenrm           
ptenrm:                             
         long      0x40020000,0xa0000000,0x00000000 # 10 ^ 1
         long      0x40050000,0xc8000000,0x00000000 # 10 ^ 2
         long      0x400c0000,0x9c400000,0x00000000 # 10 ^ 4
         long      0x40190000,0xbebc2000,0x00000000 # 10 ^ 8
         long      0x40340000,0x8e1bc9bf,0x04000000 # 10 ^ 16
         long      0x40690000,0x9dc5ada8,0x2b70b59d # 10 ^ 32
         long      0x40d30000,0xc2781f49,0xffcfa6d5 # 10 ^ 64
         long      0x41a80000,0x93ba47c9,0x80e98cdf # 10 ^ 128
         long      0x43510000,0xaa7eebfb,0x9df9de8d # 10 ^ 256
         long      0x46a30000,0xe319a0ae,0xa60e91c6 # 10 ^ 512
         long      0x4d480000,0xc9767586,0x81750c17 # 10 ^ 1024
         long      0x5a920000,0x9e8b3b5d,0xc53d5de5 # 10 ^ 2048
         long      0x75250000,0xc4605202,0x8a20979a # 10 ^ 4096
#round to positive infinity
bigrp:                              
         long      0x3ffe0000,0xb17217f7,0xd1cf79ac # ln(2)
         long      0x40000000,0x935d8ddd,0xaaa8ac17 # ln(10)
         long      0x3fff0000,0x80000000,0x00000000 # 10 ^ 0
                                    
         global    ptenrp           
ptenrp:                             
         long      0x40020000,0xa0000000,0x00000000 # 10 ^ 1
         long      0x40050000,0xc8000000,0x00000000 # 10 ^ 2
         long      0x400c0000,0x9c400000,0x00000000 # 10 ^ 4
         long      0x40190000,0xbebc2000,0x00000000 # 10 ^ 8
         long      0x40340000,0x8e1bc9bf,0x04000000 # 10 ^ 16
         long      0x40690000,0x9dc5ada8,0x2b70b59e # 10 ^ 32
         long      0x40d30000,0xc2781f49,0xffcfa6d6 # 10 ^ 64
         long      0x41a80000,0x93ba47c9,0x80e98ce0 # 10 ^ 128
         long      0x43510000,0xaa7eebfb,0x9df9de8e # 10 ^ 256
         long      0x46a30000,0xe319a0ae,0xa60e91c7 # 10 ^ 512
         long      0x4d480000,0xc9767586,0x81750c18 # 10 ^ 1024
         long      0x5a920000,0x9e8b3b5d,0xc53d5de6 # 10 ^ 2048
         long      0x75250000,0xc4605202,0x8a20979b # 10 ^ 4096
                                    
                                    
                                    
                                    
                                    
         global    get_op           
         global    uns_getop        
         global    uni_getop        
get_op:                             
         clr.b     dy_mo_flg(%a6)   
         tst.b     uflg_tmp(%a6)    # test flag for unsupp/unimp state
         beq.b     uni_getop        
                                    
uns_getop:                            
         btst.b    &direction_bit,cmdreg1b(%a6) 
         bne.w     opclass3         # branch if a fmove out (any kind)
         btst.b    &6,cmdreg1b(%a6) 
         beq.b     uns_notpacked    
                                    
         bfextu    cmdreg1b(%a6){&3:&3},%d0 
         cmp.b     %d0,&3           
         beq.w     pack_source      # check for a packed src op, branch if so
uns_notpacked:                            
         bsr       chk_dy_mo        # set the dyadic/monadic flag
         tst.b     dy_mo_flg(%a6)   
         beq.b     src_op_ck        # if monadic, go check src op
#				;else, check dst op (fall through)
                                    
         btst.b    &7,dtag(%a6)     
         beq.b     src_op_ck        # if dst op is norm, check src op
         bra.b     dst_ex_dnrm      # else, handle destination unnorm/dnrm
                                    
uni_getop:                            
         bfextu    cmdreg1b(%a6){&0:&6},%d0 # get opclass and src fields
         cmpi.l    %d0,&0x17        # if op class and size fields are $17, 
#				;it is FMOVECR; if not, continue
#
# If the instruction is fmovecr, exit get_op.  It is handled
# in do_func and smovecr.sa.
#
         bne.w     not_fmovecr      # handle fmovecr as an unimplemented inst
         rts                        
                                    
not_fmovecr:                            
         btst.b    &e1,e_byte(%a6)  # if set, there is a packed operand
         bne.w     pack_source      # check for packed src op, branch if so
                                    
# The following lines of are coded to optimize on normalized operands
         move.b    stag(%a6),%d0    
         or.b      dtag(%a6),%d0    # check if either of STAG/DTAG msb set
         bmi.b     dest_op_ck       # if so, some op needs to be fixed
         rts                        
                                    
dest_op_ck:                            
         btst.b    &7,dtag(%a6)     # check for unsupported data types in
         beq.b     src_op_ck        # the destination, if not, check src op
         bsr       chk_dy_mo        # set dyadic/monadic flag
         tst.b     dy_mo_flg(%a6)   
         beq.b     src_op_ck        # if monadic, check src op
#
# At this point, destination has an extended denorm or unnorm.
#
dst_ex_dnrm:                            
         move.w    fptemp_ex(%a6),%d0 # get destination exponent
         andi.w    &0x7fff,%d0      # mask sign, check if exp = 0000
         beq.b     src_op_ck        # if denorm then check source op.
#				;denorms are taken care of in res_func 
#				;(unsupp) or do_func (unimp)
#				;else unnorm fall through
         lea.l     fptemp(%a6),%a0  # point a0 to dop - used in mk_norm
         bsr       mk_norm          # go normalize - mk_norm returns:
#				;L_SCR1{7:5} = operand tag 
#				;	(000 = norm, 100 = denorm)
#				;L_SCR1{4} = fpte15 or ete15 
#				;	0 = exp >  $3fff
#				;	1 = exp <= $3fff
#				;and puts the normalized num back 
#				;on the fsave stack
#
         move.b    l_scr1(%a6),dtag(%a6) # write the new tag & fpte15 
#				;to the fsave stack and fall 
#				;through to check source operand
#
src_op_ck:                            
         btst.b    &7,stag(%a6)     
         beq.w     end_getop        # check for unsupported data types on the
#				;source operand
         btst.b    &5,stag(%a6)     
         bne.b     src_sd_dnrm      # if bit 5 set, handle sgl/dbl denorms
#
# At this point only unnorms or extended denorms are possible.
#
src_ex_dnrm:                            
         move.w    etemp_ex(%a6),%d0 # get source exponent
         andi.w    &0x7fff,%d0      # mask sign, check if exp = 0000
         beq.w     end_getop        # if denorm then exit, denorms are 
#				;handled in do_func
         lea.l     etemp(%a6),%a0   # point a0 to sop - used in mk_norm
         bsr       mk_norm          # go normalize - mk_norm returns:
#				;L_SCR1{7:5} = operand tag 
#				;	(000 = norm, 100 = denorm)
#				;L_SCR1{4} = fpte15 or ete15 
#				;	0 = exp >  $3fff
#				;	1 = exp <= $3fff
#				;and puts the normalized num back 
#				;on the fsave stack
#
         move.b    l_scr1(%a6),stag(%a6) # write the new tag & ete15 
         rts                        # end_getop
                                    
#
# At this point, only single or double denorms are possible.
# If the inst is not fmove, normalize the source.  If it is,
# do nothing to the input.
#
src_sd_dnrm:                            
         btst.b    &4,cmdreg1b(%a6) # differentiate between sgl/dbl denorm
         bne.b     is_double        
is_single:                            
         move.w    &0x3f81,%d1      # write bias for sgl denorm
         bra.b     common           # goto the common code
is_double:                            
         move.w    &0x3c01,%d1      # write the bias for a dbl denorm
common:                             
         btst.b    &sign_bit,etemp_ex(%a6) # grab sign bit of mantissa
         beq.b     pos              
         bset      &15,%d1          # set sign bit because it is negative
pos:                                
         move.w    %d1,etemp_ex(%a6) 
#				;put exponent on stack
                                    
         move.w    cmdreg1b(%a6),%d1 
         and.w     &0xe3ff,%d1      # clear out source specifier
         or.w      &0x0800,%d1      # set source specifier to extended prec
         move.w    %d1,cmdreg1b(%a6) # write back to the command word in stack
#				;this is needed to fix unsupp data stack
         lea.l     etemp(%a6),%a0   # point a0 to sop
                                    
         bsr       mk_norm          # convert sgl/dbl denorm to norm
         move.b    l_scr1(%a6),stag(%a6) # put tag into source tag reg - d0
         rts                        # end_getop
#
# At this point, the source is definitely packed, whether
# instruction is dyadic or monadic is still unknown
#
pack_source:                            
         move.l    fptemp_lo(%a6),etemp(%a6) # write ms part of packed 
#				;number to etemp slot
         bsr       chk_dy_mo        # set dyadic/monadic flag
         bsr       unpack           
                                    
         tst.b     dy_mo_flg(%a6)   
         beq.b     end_getop        # if monadic, exit
#				;else, fix FPTEMP
pack_dya:                            
         bfextu    cmdreg1b(%a6){&6:&3},%d0 # extract dest fp reg
         move.l    &7,%d1           
         sub.l     %d0,%d1          
         clr.l     %d0              
         bset.l    %d1,%d0          # set up d0 as a dynamic register mask
         fmovem.x  %d0,fptemp(%a6)  # write to FPTEMP
                                    
         btst.b    &7,dtag(%a6)     # check dest tag for unnorm or denorm
         bne.w     dst_ex_dnrm      # else, handle the unnorm or ext denorm
#
# Dest is not denormalized.  Check for norm, and set fpte15 
# accordingly.
#
         move.b    dtag(%a6),%d0    
         andi.b    &0xf0,%d0        # strip to only dtag:fpte15
         tst.b     %d0              # check for normalized value
         bne.b     end_getop        # if inf/nan/zero leave get_op
         move.w    fptemp_ex(%a6),%d0 
         andi.w    &0x7fff,%d0      
         cmpi.w    %d0,&0x3fff      # check if fpte15 needs setting
         bge.b     end_getop        # if >= $3fff, leave fpte15=0
         or.b      &0x10,dtag(%a6)  
         bra.b     end_getop        
                                    
#
# At this point, it is either an fmoveout packed, unnorm or denorm
#
opclass3:                            
         clr.b     dy_mo_flg(%a6)   # set dyadic/monadic flag to monadic
         bfextu    cmdreg1b(%a6){&4:&2},%d0 
         cmpi.b    %d0,&3           
         bne.w     src_ex_dnrm      # if not equal, must be unnorm or denorm
#				;else it is a packed move out
#				;exit
end_getop:                            
         rts                        
                                    
#
# Sets the DY_MO_FLG correctly. This is used only on if it is an
# unuspported data type exception.  Set if dyadic.
#
chk_dy_mo:                            
         move.w    cmdreg1b(%a6),%d0 
         btst.l    &5,%d0           # testing extension command word
         beq.b     set_mon          # if bit 5 = 0 then monadic
         btst.l    &4,%d0           # know that bit 5 = 1
         beq.b     set_dya          # if bit 4 = 0 then dyadic
         andi.w    &0x007f,%d0      # get rid of all but extension bits {6:0}
         cmpi.w    %d0,&0x0038      # if extension = $38 then fcmp (dyadic)
         bne.b     set_mon          
set_dya:                            
         st.b      dy_mo_flg(%a6)   # set the inst flag type to dyadic
         rts                        
set_mon:                            
         clr.b     dy_mo_flg(%a6)   # set the inst flag type to monadic
         rts                        
#
#	MK_NORM
#
# Normalizes unnormalized numbers, sets tag to norm or denorm, sets unfl
# exception if denorm.
#
# CASE opclass 0x0 unsupp
#	mk_norm till msb set
#	set tag = norm
#
# CASE opclass 0x0 unimp
#	mk_norm till msb set or exp = 0
#	if integer bit = 0
#	   tag = denorm
#	else
#	   tag = norm
#
# CASE opclass 011 unsupp
#	mk_norm till msb set or exp = 0
#	if integer bit = 0
#	   tag = denorm
#	   set unfl_nmcexe = 1
#	else
#	   tag = norm
#
# if exp <= $3fff
#   set ete15 or fpte15 = 1
# else set ete15 or fpte15 = 0
                                    
# input:
#	a0 = points to operand to be normalized
# output:
#	L_SCR1{7:5} = operand tag (000 = norm, 100 = denorm)
#	L_SCR1{4}   = fpte15 or ete15 (0 = exp > $3fff, 1 = exp <=$3fff)
#	the normalized operand is placed back on the fsave stack
mk_norm:                            
         clr.l     l_scr1(%a6)      
         bclr.b    &sign_bit,local_ex(%a0) 
         sne       local_sgn(%a0)   # transform into internal extended format
                                    
         cmpi.b    1+exc_vec(%a6),&0x2c # check if unimp
         bne.b     uns_data         # branch if unsupp
         bsr       uni_inst         # call if unimp (opclass 0x0)
         bra.b     reload           
uns_data:                            
         btst.b    &direction_bit,cmdreg1b(%a6) # check transfer direction
         bne.b     bit_set          # branch if set (opclass 011)
         bsr       uns_opx          # call if opclass 0x0
         bra.b     reload           
bit_set:                            
         bsr       uns_op3          # opclass 011
reload:                             
         cmp.w     local_ex(%a0),&0x3fff # if exp > $3fff
         bgt.b     end_mk           #    fpte15/ete15 already set to 0
         bset.b    &4,l_scr1(%a6)   # else set fpte15/ete15 to 1
#				;calling routine actually sets the 
#				;value on the stack (along with the 
#				;tag), since this routine doesn't 
#				;know if it should set ete15 or fpte15
#				;ie, it doesn't know if this is the 
#				;src op or dest op.
end_mk:                             
         bfclr     local_sgn(%a0){&0:&8} 
         beq.b     end_mk_pos       
         bset.b    &sign_bit,local_ex(%a0) # convert back to IEEE format
end_mk_pos:                            
         rts                        
#
#     CASE opclass 011 unsupp
#
uns_op3:                            
         bsr.l     nrm_zero         # normalize till msb = 1 or exp = zero
         btst.b    &7,local_hi(%a0) # if msb = 1
         bne.b     no_unfl          # then branch
set_unfl:                            
         or.w      &dnrm_tag,l_scr1(%a6) # set denorm tag
         bset.b    &unfl_bit,fpsr_except(%a6) # set unfl exception bit
no_unfl:                            
         rts                        
#
#     CASE opclass 0x0 unsupp
#
uns_opx:                            
         bsr.l     nrm_zero         # normalize the number
         btst.b    &7,local_hi(%a0) # check if integer bit (j-bit) is set 
         beq.b     uns_den          # if clear then now have a denorm
uns_nrm:                            
         or.b      &norm_tag,l_scr1(%a6) # set tag to norm
         rts                        
uns_den:                            
         or.b      &dnrm_tag,l_scr1(%a6) # set tag to denorm
         rts                        
#
#     CASE opclass 0x0 unimp
#
uni_inst:                            
         bsr.l     nrm_zero         
         btst.b    &7,local_hi(%a0) # check if integer bit (j-bit) is set 
         beq.b     uni_den          # if clear then now have a denorm
uni_nrm:                            
         or.b      &norm_tag,l_scr1(%a6) # set tag to norm
         rts                        
uni_den:                            
         or.b      &dnrm_tag,l_scr1(%a6) # set tag to denorm
         rts                        
                                    
#
#	Decimal to binary conversion
#
# Special cases of inf and NaNs are completed outside of decbin.  
# If the input is an snan, the snan bit is not set.
# 
# input:
#	ETEMP(a6)	- points to packed decimal string in memory
# output:
#	fp0	- contains packed string converted to extended precision
#	ETEMP	- same as fp0
unpack:                             
         move.w    cmdreg1b(%a6),%d0 # examine command word, looking for fmove's
         and.w     &0x3b,%d0        
         beq       move_unpack      # special handling for fmove: must set FPSR_CC
                                    
         move.w    etemp(%a6),%d0   # get word with inf information
         bfextu    %d0{&20:&12},%d1 # get exponent into d1
         cmpi.w    %d1,&0x0fff      # test for inf or NaN
         bne.b     try_zero         # if not equal, it is not special
         bfextu    %d0{&17:&3},%d1  # get SE and y bits into d1
         cmpi.w    %d1,&7           # SE and y bits must be on for special
         bne.b     try_zero         # if not on, it is not special
#input is of the special cases of inf and NaN
         tst.l     etemp_hi(%a6)    # check ms mantissa
         bne.b     fix_nan          # if non-zero, it is a NaN
         tst.l     etemp_lo(%a6)    # check ls mantissa
         bne.b     fix_nan          # if non-zero, it is a NaN
         bra.w     finish           # special already on stack
fix_nan:                            
         btst.b    &signan_bit,etemp_hi(%a6) # test for snan
         bne.w     finish           
         or.l      &snaniop_mask,user_fpsr(%a6) # always set snan if it is so
         bra.w     finish           
try_zero:                            
         move.w    etemp_ex+2(%a6),%d0 # get word 4
         andi.w    &0x000f,%d0      # clear all but last ni(y)bble
         tst.w     %d0              # check for zero.
         bne.w     not_spec         
         tst.l     etemp_hi(%a6)    # check words 3 and 2
         bne.w     not_spec         
         tst.l     etemp_lo(%a6)    # check words 1 and 0
         bne.w     not_spec         
         tst.l     etemp(%a6)       # test sign of the zero
         bge.b     pos_zero         
         move.l    &0x80000000,etemp(%a6) # write neg zero to etemp
         clr.l     etemp_hi(%a6)    
         clr.l     etemp_lo(%a6)    
         bra.w     finish           
pos_zero:                            
         clr.l     etemp(%a6)       
         clr.l     etemp_hi(%a6)    
         clr.l     etemp_lo(%a6)    
         bra.w     finish           
                                    
not_spec:                            
         fmovem.x  %fp0,-(%a7)      # save fp0 - decbin returns in it
         bsr.l     decbin           
         fmove.x   %fp0,etemp(%a6)  # put the unpacked sop in the fsave stack
         fmovem.x  (%a7)+,%fp0      
         fmove.l   &0,%fpsr         # clr fpsr from decbin
         bra       finish           
                                    
#
# Special handling for packed move in:  Same results as all other
# packed cases, but we must set the FPSR condition codes properly.
#
move_unpack:                            
         move.w    etemp(%a6),%d0   # get word with inf information
         bfextu    %d0{&20:&12},%d1 # get exponent into d1
         cmpi.w    %d1,&0x0fff      # test for inf or NaN
         bne.b     mtry_zero        # if not equal, it is not special
         bfextu    %d0{&17:&3},%d1  # get SE and y bits into d1
         cmpi.w    %d1,&7           # SE and y bits must be on for special
         bne.b     mtry_zero        # if not on, it is not special
#input is of the special cases of inf and NaN
         tst.l     etemp_hi(%a6)    # check ms mantissa
         bne.b     mfix_nan         # if non-zero, it is a NaN
         tst.l     etemp_lo(%a6)    # check ls mantissa
         bne.b     mfix_nan         # if non-zero, it is a NaN
#input is inf
         or.l      &inf_mask,user_fpsr(%a6) # set I bit
         tst.l     etemp(%a6)       # check sign
         bge.w     finish           
         or.l      &neg_mask,user_fpsr(%a6) # set N bit
         bra.w     finish           # special already on stack
mfix_nan:                            
         or.l      &nan_mask,user_fpsr(%a6) # set NaN bit
         move.b    &nan_tag,stag(%a6) # set stag to NaN
         btst.b    &signan_bit,etemp_hi(%a6) # test for snan
         bne.b     mn_snan          
         or.l      &snaniop_mask,user_fpsr(%a6) # set snan bit
         btst.b    &snan_bit,fpcr_enable(%a6) # test for snan enabled
         bne.b     mn_snan          
         bset.b    &signan_bit,etemp_hi(%a6) # force snans to qnans
mn_snan:                            
         tst.l     etemp(%a6)       # check for sign
         bge.w     finish           # if clr, go on
         or.l      &neg_mask,user_fpsr(%a6) # set N bit
         bra.w     finish           
                                    
mtry_zero:                            
         move.w    etemp_ex+2(%a6),%d0 # get word 4
         andi.w    &0x000f,%d0      # clear all but last ni(y)bble
         tst.w     %d0              # check for zero.
         bne.b     mnot_spec        
         tst.l     etemp_hi(%a6)    # check words 3 and 2
         bne.b     mnot_spec        
         tst.l     etemp_lo(%a6)    # check words 1 and 0
         bne.b     mnot_spec        
         tst.l     etemp(%a6)       # test sign of the zero
         bge.b     mpos_zero        
         or.l      &neg_mask+z_mask,user_fpsr(%a6) # set N and Z
         move.l    &0x80000000,etemp(%a6) # write neg zero to etemp
         clr.l     etemp_hi(%a6)    
         clr.l     etemp_lo(%a6)    
         bra.b     finish           
mpos_zero:                            
         or.l      &z_mask,user_fpsr(%a6) # set Z
         clr.l     etemp(%a6)       
         clr.l     etemp_hi(%a6)    
         clr.l     etemp_lo(%a6)    
         bra.b     finish           
                                    
mnot_spec:                            
         fmovem.x  %fp0,-(%a7)      # save fp0 - decbin returns in it
         bsr.l     decbin           
         fmove.x   %fp0,etemp(%a6)  
#				;put the unpacked sop in the fsave stack
         fmovem.x  (%a7)+,%fp0      
                                    
finish:                             
         move.w    cmdreg1b(%a6),%d0 # get the command word
         and.w     &0xfbff,%d0      # change the source specifier field to 
#				;extended (was packed).
         move.w    %d0,cmdreg1b(%a6) # write command word back to fsave stack
#				;we need to do this so the 040 will 
#				;re-execute the inst. without taking 
#				;another packed trap.
                                    
fix_stag:                            
#Converted result is now in etemp on fsave stack, now set the source 
#tag (stag) 
#	if (ete =$7fff) then INF or NAN
#		if (etemp = $x.0----0) then
#			stag = INF
#		else
#			stag = NAN
#	else
#		if (ete = $0000) then
#			stag = ZERO
#		else
#			stag = NORM
#
# Note also that the etemp_15 bit (just right of the stag) must
# be set accordingly.  
#
         move.w    etemp_ex(%a6),%d1 
         andi.w    &0x7fff,%d1      # strip sign
         cmp.w     %d1,&0x7fff      
         bne.b     z_or_nrm         
         move.l    etemp_hi(%a6),%d1 
         bne.b     is_nan           
         move.l    etemp_lo(%a6),%d1 
         bne.b     is_nan           
is_inf:                             
         move.b    &0x40,stag(%a6)  
         move.l    &0x40,%d0        
         rts                        
is_nan:                             
         move.b    &0x60,stag(%a6)  
         move.l    &0x60,%d0        
         rts                        
z_or_nrm:                            
         tst.w     %d1              
         bne.b     is_nrm           
is_zro:                             
# For a zero, set etemp_15
         move.b    &0x30,stag(%a6)  
         move.l    &0x20,%d0        
         rts                        
is_nrm:                             
# For a norm, check if the exp <= $3fff; if so, set etemp_15
         cmpi.w    %d1,&0x3fff      
         ble.b     set_bit15        
         move.b    &0,stag(%a6)     
         bra.b     end_is_nrm       
set_bit15:                            
         move.b    &0x10,stag(%a6)  
end_is_nrm:                            
         move.l    &0,%d0           
end_fix:                            
         rts                        
                                    
end_get:                            
         rts                        
                                    
	version 3
