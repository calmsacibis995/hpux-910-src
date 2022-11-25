ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/do_func.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:47:44 $
                                    
#
#	do_func.sa 3.4 2/18/91
#
# Do_func performs the unimplemented operation.  The operation
# to be performed is determined from the lower 7 bits of the
# extension word (except in the case of fmovecr and fsincos).
# The opcode and tag bits form an index into a jump table in 
# tbldo.sa.  Cases of zero, infinity and NaN are handled in 
# do_func by forcing the default result.  Normalized and
# denormalized (there are no unnormalized numbers at this
# point) are passed onto the emulation code.  
#
# CMDREG1B and STAG are extracted from the fsave frame
# and combined to form the table index.  The function called
# will start with a0 pointing to the ETEMP operand.  Dyadic
# functions can find FPTEMP at -12(a0).
#
# Called functions return their result in fp0.  Sincos returns
# sin(x) in fp0 and cos(x) in fp1.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
pone:    long      0x3fff0000,0x80000000,0x00000000 # +1
mone:    long      0xbfff0000,0x80000000,0x00000000 # -1
pzero:   long      0x00000000,0x00000000,0x00000000 # +0
mzero:   long      0x80000000,0x00000000,0x00000000 # -0
pinf:    long      0x7fff0000,0x00000000,0x00000000 # +inf
minf:    long      0xffff0000,0x00000000,0x00000000 # -inf
qnan:    long      0x7fff0000,0xffffffff,0xffffffff # non-signaling nan
ppiby2:  long      0x3fff0000,0xc90fdaa2,0x2168c235 # +PI/2
mpiby2:  long      0xbfff0000,0xc90fdaa2,0x2168c235 # -PI/2
                                    
         global    do_func          
do_func:                            
         clr.b     cu_only(%a6)     
#
# Check for fmovecr.  It does not follow the format of fp gen
# unimplemented instructions.  The test is on the upper 6 bits;
# if they are $17, the inst is fmovecr.  Call entry smovcr
# directly.
#
         bfextu    cmdreg1b(%a6){&0:&6},%d0 # get opclass and src fields
         cmpi.l    %d0,&0x17        # if op class and size fields are $17, 
#				;it is FMOVECR; if not, continue
         bne.b     not_fmovecr      
         jmp       smovcr           #  jmp directly to emulation
                                    
not_fmovecr:                            
         move.w    cmdreg1b(%a6),%d0 
         and.l     &0x7f,%d0        
         cmpi.l    %d0,&0x38        # if the extension is >= $38, 
         bge.b     serror           # it is illegal
         bfextu    stag(%a6){&0:&3},%d1 
         lsl.l     &3,%d0           # make room for STAG
         add.l     %d1,%d0          # combine for final index into table
         lea.l     tblpre,%a1       # start of monster jump table
         move.l    (%a1,%d0.w*4),%a1 # real target address
         lea.l     etemp(%a6),%a0   # a0 is pointer to src op
         move.l    user_fpcr(%a6),%d1 
         and.l     &0xff,%d1        #  discard all but rounding mode/prec
         fmove.l   &0,%fpcr         
         jmp       (%a1)            
#
#	ERROR
#
         global    serror           
serror:                             
         st.b      store_flg(%a6)   
         rts                        
#
# These routines load forced values into fp0.  They are called
# by index into tbldo.
#
# Load a signed zero to fp0 and set inex2/ainex
#
         global    snzrinx          
snzrinx:                            
         btst.b    &sign_bit,local_ex(%a0) # get sign of source operand
         bne.b     ld_mzinx         # if negative, branch
         bsr       ld_pzero         # bsr so we can return and set inx
         bra.l     t_inx2           # now, set the inx for the next inst
ld_mzinx:                            
         bsr       ld_mzero         # if neg, load neg zero, return here
         bra.l     t_inx2           # now, set the inx for the next inst
#
# Load a signed zero to fp0; do not set inex2/ainex 
#
         global    szero            
szero:                              
         btst.b    &sign_bit,local_ex(%a0) # get sign of source operand
         bne       ld_mzero         # if neg, load neg zero
         bra       ld_pzero         # load positive zero
#
# Load a signed infinity to fp0; do not set inex2/ainex 
#
         global    sinf             
sinf:                               
         btst.b    &sign_bit,local_ex(%a0) # get sign of source operand
         bne       ld_minf          # if negative branch
         bra       ld_pinf          
#
# Load a signed one to fp0; do not set inex2/ainex 
#
         global    sone             
sone:                               
         btst.b    &sign_bit,local_ex(%a0) # check sign of source
         bne       ld_mone          
         bra       ld_pone          
#
# Load a signed pi/2 to fp0; do not set inex2/ainex 
#
         global    spi_2            
spi_2:                              
         btst.b    &sign_bit,local_ex(%a0) # check sign of source
         bne       ld_mpi2          
         bra       ld_ppi2          
#
# Load either a +0 or +inf for plus/minus operand
#
         global    szr_inf          
szr_inf:                            
         btst.b    &sign_bit,local_ex(%a0) # check sign of source
         bne       ld_pzero         
         bra       ld_pinf          
#
# Result is either an operr or +inf for plus/minus operand
# [Used by slogn, slognp1, slog10, and slog2]
#
         global    sopr_inf         
sopr_inf:                            
         btst.b    &sign_bit,local_ex(%a0) # check sign of source
         bne.l     t_operr          
         bra       ld_pinf          
#
#	FLOGNP1 
#
         global    sslognp1         
sslognp1:                            
         fmovem.x  (%a0),%fp0       
         fcmp.b    %fp0,&-1         
         fbgt.l    slognp1          
         fbeq.l    t_dz2            # if = -1, divide by zero exception
         fmove.l   &0,%fpsr         # clr N flag
         bra.l     t_operr          # take care of operands < -1
#
#	FETOXM1
#
         global    setoxm1i         
setoxm1i:                            
         btst.b    &sign_bit,local_ex(%a0) # check sign of source
         bne       ld_mone          
         bra       ld_pinf          
#
#	FLOGN
#
# Test for 1.0 as an input argument, returning +zero.  Also check
# the sign and return operr if negative.
#
         global    sslogn           
sslogn:                             
         btst.b    &sign_bit,local_ex(%a0) 
         bne.l     t_operr          # take care of operands < 0
         cmpi.w    local_ex(%a0),&0x3fff # test for 1.0 input
         bne.l     slogn            
         cmpi.l    local_hi(%a0),&0x80000000 
         bne.l     slogn            
         tst.l     local_lo(%a0)    
         bne.l     slogn            
         fmove.x   pzero,%fp0       
         rts                        
                                    
         global    sslognd          
sslognd:                            
         btst.b    &sign_bit,local_ex(%a0) 
         beq.l     slognd           
         bra.l     t_operr          # take care of operands < 0
                                    
#
#	FLOG10
#
         global    sslog10          
sslog10:                            
         btst.b    &sign_bit,local_ex(%a0) 
         bne.l     t_operr          # take care of operands < 0
         cmpi.w    local_ex(%a0),&0x3fff # test for 1.0 input
         bne.l     slog10           
         cmpi.l    local_hi(%a0),&0x80000000 
         bne.l     slog10           
         tst.l     local_lo(%a0)    
         bne.l     slog10           
         fmove.x   pzero,%fp0       
         rts                        
                                    
         global    sslog10d         
sslog10d:                            
         btst.b    &sign_bit,local_ex(%a0) 
         beq.l     slog10d          
         bra.l     t_operr          # take care of operands < 0
                                    
#
#	FLOG2
#
         global    sslog2           
sslog2:                             
         btst.b    &sign_bit,local_ex(%a0) 
         bne.l     t_operr          # take care of operands < 0
         cmpi.w    local_ex(%a0),&0x3fff # test for 1.0 input
         bne.l     slog2            
         cmpi.l    local_hi(%a0),&0x80000000 
         bne.l     slog2            
         tst.l     local_lo(%a0)    
         bne.l     slog2            
         fmove.x   pzero,%fp0       
         rts                        
                                    
         global    sslog2d          
sslog2d:                            
         btst.b    &sign_bit,local_ex(%a0) 
         beq.l     slog2d           
         bra.l     t_operr          # take care of operands < 0
                                    
#
#	FMOD
#
pmodt:                              
#				;$21 fmod
#				;dtag,stag
         long      smod             #   00,00  norm,norm = normal
         long      smod_oper        #   00,01  norm,zero = nan with operr
         long      smod_fpn         #   00,10  norm,inf  = fpn
         long      smod_snan        #   00,11  norm,nan  = nan
         long      smod_zro         #   01,00  zero,norm = +-zero
         long      smod_oper        #   01,01  zero,zero = nan with operr
         long      smod_zro         #   01,10  zero,inf  = +-zero
         long      smod_snan        #   01,11  zero,nan  = nan
         long      smod_oper        #   10,00  inf,norm  = nan with operr
         long      smod_oper        #   10,01  inf,zero  = nan with operr
         long      smod_oper        #   10,10  inf,inf   = nan with operr
         long      smod_snan        #   10,11  inf,nan   = nan
         long      smod_dnan        #   11,00  nan,norm  = nan
         long      smod_dnan        #   11,01  nan,zero  = nan
         long      smod_dnan        #   11,10  nan,inf   = nan
         long      smod_dnan        #   11,11  nan,nan   = nan
                                    
         global    pmod             
pmod:                               
         bfextu    stag(%a6){&0:&3},%d0 # stag = d0
         bfextu    dtag(%a6){&0:&3},%d1 # dtag = d1
                                    
#
# Alias extended denorms to norms for the jump table.
#
         bclr.l    &2,%d0           
         bclr.l    &2,%d1           
                                    
         lsl.b     &2,%d1           
         or.b      %d0,%d1          # d1{3:2} = dtag, d1{1:0} = stag
#				;Tag values:
#				;00 = norm or denorm
#				;01 = zero
#				;10 = inf
#				;11 = nan
         lea       pmodt,%a1        
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
                                    
smod_snan:                            
         bra.l     src_nan          
smod_dnan:                            
         bra.l     dst_nan          
smod_oper:                            
         bra.l     t_operr          
smod_zro:                            
         move.b    etemp(%a6),%d1   # get sign of src op
         move.b    fptemp(%a6),%d0  # get sign of dst op
         eor.b     %d0,%d1          # get exor of sign bits
         btst.l    &7,%d1           # test for sign
         beq.b     smod_zsn         # if clr, do not set sign big
         bset.b    &q_sn_bit,fpsr_qbyte(%a6) # set q-byte sign bit
smod_zsn:                            
         btst.l    &7,%d0           # test if + or -
         beq       ld_pzero         # if pos then load +0
         bra       ld_mzero         # else neg load -0
                                    
smod_fpn:                            
         move.b    etemp(%a6),%d1   # get sign of src op
         move.b    fptemp(%a6),%d0  # get sign of dst op
         eor.b     %d0,%d1          # get exor of sign bits
         btst.l    &7,%d1           # test for sign
         beq.b     smod_fsn         # if clr, do not set sign big
         bset.b    &q_sn_bit,fpsr_qbyte(%a6) # set q-byte sign bit
smod_fsn:                            
         tst.b     dtag(%a6)        # filter out denormal destination case
         bpl.b     smod_nrm         
         lea.l     fptemp(%a6),%a0  # a0<- addr(FPTEMP)
         bra.l     t_resdnrm        # force UNFL(but exact) result
smod_nrm:                            
         fmove.l   user_fpcr(%a6),%fpcr # use user's rmode and precision
         fmove.x   fptemp(%a6),%fp0 # return dest to fp0
         rts                        
                                    
#
#	FREM
#
premt:                              
#				;$25 frem
#				;dtag,stag
         long      srem             #   00,00  norm,norm = normal
         long      srem_oper        #   00,01  norm,zero = nan with operr
         long      srem_fpn         #   00,10  norm,inf  = fpn
         long      srem_snan        #   00,11  norm,nan  = nan
         long      srem_zro         #   01,00  zero,norm = +-zero
         long      srem_oper        #   01,01  zero,zero = nan with operr
         long      srem_zro         #   01,10  zero,inf  = +-zero
         long      srem_snan        #   01,11  zero,nan  = nan
         long      srem_oper        #   10,00  inf,norm  = nan with operr
         long      srem_oper        #   10,01  inf,zero  = nan with operr
         long      srem_oper        #   10,10  inf,inf   = nan with operr
         long      srem_snan        #   10,11  inf,nan   = nan
         long      srem_dnan        #   11,00  nan,norm  = nan
         long      srem_dnan        #   11,01  nan,zero  = nan
         long      srem_dnan        #   11,10  nan,inf   = nan
         long      srem_dnan        #   11,11  nan,nan   = nan
                                    
         global    prem             
prem:                               
                                    
         bfextu    stag(%a6){&0:&3},%d0 # stag = d0
         bfextu    dtag(%a6){&0:&3},%d1 # dtag = d1
#
# Alias extended denorms to norms for the jump table.
#
         bclr      &2,%d0           
         bclr      &2,%d1           
                                    
         lsl.b     &2,%d1           
         or.b      %d0,%d1          # d1{3:2} = dtag, d1{1:0} = stag
#				;Tag values:
#				;00 = norm or denorm
#				;01 = zero
#				;10 = inf
#				;11 = nan
         lea       premt,%a1        
         move.l    (%a1,%d1.w*4),%a1 
         jmp       (%a1)            
                                    
srem_snan:                            
         bra.l     src_nan          
srem_dnan:                            
         bra.l     dst_nan          
srem_oper:                            
         bra.l     t_operr          
srem_zro:                            
         move.b    etemp(%a6),%d1   # get sign of src op
         move.b    fptemp(%a6),%d0  # get sign of dst op
         eor.b     %d0,%d1          # get exor of sign bits
         btst.l    &7,%d1           # test for sign
         beq.b     srem_zsn         # if clr, do not set sign big
         bset.b    &q_sn_bit,fpsr_qbyte(%a6) # set q-byte sign bit
srem_zsn:                            
         btst.l    &7,%d0           # test if + or -
         beq       ld_pzero         # if pos then load +0
         bra       ld_mzero         # else neg load -0
                                    
srem_fpn:                            
         move.b    etemp(%a6),%d1   # get sign of src op
         move.b    fptemp(%a6),%d0  # get sign of dst op
         eor.b     %d0,%d1          # get exor of sign bits
         btst.l    &7,%d1           # test for sign
         beq.b     srem_fsn         # if clr, do not set sign big
         bset.b    &q_sn_bit,fpsr_qbyte(%a6) # set q-byte sign bit
srem_fsn:                            
         tst.b     dtag(%a6)        # filter out denormal destination case
         bpl.b     srem_nrm         
         lea.l     fptemp(%a6),%a0  # a0<- addr(FPTEMP)
         bra.l     t_resdnrm        # force UNFL(but exact) result
srem_nrm:                            
         fmove.l   user_fpcr(%a6),%fpcr # use user's rmode and precision
         fmove.x   fptemp(%a6),%fp0 # return dest to fp0
         rts                        
#
#	FSCALE
#
pscalet:                            
#				;$26 fscale
#				;dtag,stag
         long      sscale           #   00,00  norm,norm = result
         long      sscale           #   00,01  norm,zero = fpn
         long      scl_opr          #   00,10  norm,inf  = nan with operr
         long      scl_snan         #   00,11  norm,nan  = nan
         long      scl_zro          #   01,00  zero,norm = +-zero
         long      scl_zro          #   01,01  zero,zero = +-zero
         long      scl_opr          #   01,10  zero,inf  = nan with operr
         long      scl_snan         #   01,11  zero,nan  = nan
         long      scl_inf          #   10,00  inf,norm  = +-inf
         long      scl_inf          #   10,01  inf,zero  = +-inf
         long      scl_opr          #   10,10  inf,inf   = nan with operr
         long      scl_snan         #   10,11  inf,nan   = nan
         long      scl_dnan         #   11,00  nan,norm  = nan
         long      scl_dnan         #   11,01  nan,zero  = nan
         long      scl_dnan         #   11,10  nan,inf   = nan
         long      scl_dnan         #   11,11  nan,nan   = nan
                                    
         global    pscale           
pscale:                             
         bfextu    stag(%a6){&0:&3},%d0 # stag in d0
         bfextu    dtag(%a6){&0:&3},%d1 # dtag in d1
         bclr.l    &2,%d0           # alias  denorm into norm
         bclr.l    &2,%d1           # alias  denorm into norm
         lsl.b     &2,%d1           
         or.b      %d0,%d1          # d1{4:2} = dtag, d1{1:0} = stag
#				;dtag values     stag values:
#				;000 = norm      00 = norm
#				;001 = zero	 01 = zero
#				;010 = inf	 10 = inf
#				;011 = nan	 11 = nan
#				;100 = dnrm
#
#
         lea.l     pscalet,%a1      # load start of jump table
         move.l    (%a1,%d1.w*4),%a1 # load a1 with label depending on tag
         jmp       (%a1)            # go to the routine
                                    
scl_opr:                            
         bra.l     t_operr          
                                    
scl_dnan:                            
         bra.l     dst_nan          
                                    
scl_zro:                            
         btst.b    &sign_bit,fptemp_ex(%a6) # test if + or -
         beq       ld_pzero         # if pos then load +0
         bra       ld_mzero         # if neg then load -0
scl_inf:                            
         btst.b    &sign_bit,fptemp_ex(%a6) # test if + or -
         beq       ld_pinf          # if pos then load +inf
         bra       ld_minf          # else neg load -inf
scl_snan:                            
         bra.l     src_nan          
#
#	FSINCOS
#
         global    ssincosz         
ssincosz:                            
         btst.b    &sign_bit,etemp(%a6) # get sign
         beq.b     sincosp          
         fmove.x   mzero,%fp0       
         bra.b     sincoscom        
sincosp:                            
         fmove.x   pzero,%fp0       
sincoscom:                            
         fmovem.x  pone,%fp1        # do not allow FPSR to be affected
         bra.l     sto_cos          # store cosine result
                                    
         global    ssincosi         
ssincosi:                            
         fmove.x   qnan,%fp1        # load NAN
         bsr.l     sto_cos          # store cosine result
         fmove.x   qnan,%fp0        # load NAN
         bra.l     t_operr          
                                    
         global    ssincosnan       
ssincosnan:                            
         move.l    etemp_ex(%a6),fp_scr1(%a6) 
         move.l    etemp_hi(%a6),fp_scr1+4(%a6) 
         move.l    etemp_lo(%a6),fp_scr1+8(%a6) 
         bset.b    &signan_bit,fp_scr1+4(%a6) 
         fmovem.x  fp_scr1(%a6),%fp1 
         bsr.l     sto_cos          
         bra.l     src_nan          
#
# This code forces default values for the zero, inf, and nan cases 
# in the transcendentals code.  The CC bits must be set in the
# stacked FPSR to be correctly reported.
#
#**Returns +PI/2
         global    ld_ppi2          
ld_ppi2:                            
         fmove.x   ppiby2,%fp0      # load +pi/2
         bra.l     t_inx2           # set inex2 exc
                                    
#**Returns -PI/2
         global    ld_mpi2          
ld_mpi2:                            
         fmove.x   mpiby2,%fp0      # load -pi/2
         or.l      &neg_mask,user_fpsr(%a6) # set N bit
         bra.l     t_inx2           # set inex2 exc
                                    
#**Returns +inf
         global    ld_pinf          
ld_pinf:                            
         fmove.x   pinf,%fp0        # load +inf
         or.l      &inf_mask,user_fpsr(%a6) # set I bit
         rts                        
                                    
#**Returns -inf
         global    ld_minf          
ld_minf:                            
         fmove.x   minf,%fp0        # load -inf
         or.l      &neg_mask+inf_mask,user_fpsr(%a6) # set N and I bits
         rts                        
                                    
#**Returns +1
         global    ld_pone          
ld_pone:                            
         fmove.x   pone,%fp0        # load +1
         rts                        
                                    
#**Returns -1
         global    ld_mone          
ld_mone:                            
         fmove.x   mone,%fp0        # load -1
         or.l      &neg_mask,user_fpsr(%a6) # set N bit
         rts                        
                                    
#**Returns +0
         global    ld_pzero         
ld_pzero:                            
         fmove.x   pzero,%fp0       # load +0
         or.l      &z_mask,user_fpsr(%a6) # set Z bit
         rts                        
                                    
#**Returns -0
         global    ld_mzero         
ld_mzero:                            
         fmove.x   mzero,%fp0       # load -0
         or.l      &neg_mask+z_mask,user_fpsr(%a6) # set N and Z bits
         rts                        
                                    
                                    
	version 3
