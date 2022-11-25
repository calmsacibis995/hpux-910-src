ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_store.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:56:30 $
                                    
#
#	x_store.sa 3.2 1/24/91
#
#	store --- store operand to memory or register
#
#	Used by underflow and overflow handlers.
#
#	a6 = points to fp value to be stored.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
fpreg_mask:                            
         byte      0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    dest_ext         
         global    dest_dbl         
         global    dest_sgl         
                                    
         global    store            
store:                              
         btst.b    &e3,e_byte(%a6)  
         beq.b     e1_sto           
e3_sto:                             
         move.l    cmdreg3b(%a6),%d0 
         bfextu    %d0{&6:&3},%d0   # isolate dest. reg from cmdreg3b
sto_fp:                             
         lea       fpreg_mask,%a1   
         move.b    (%a1,%d0.w),%d0  # convert reg# to dynamic register mask
         tst.b     local_sgn(%a0)   
         beq.b     is_pos           
         bset.b    &sign_bit,local_ex(%a0) 
is_pos:                             
         fmovem.x  (%a0),%d0        # move to correct register
#
#	if fp0-fp3 is being modified, we must put a copy
#	in the USER_FPn variable on the stack because all exception
#	handlers restore fp0-fp3 from there.
#
         cmp.b     %d0,&0x80        
         bne.b     not_fp0          
         fmovem.x  %fp0,user_fp0(%a6) 
         rts                        
not_fp0:                            
         cmp.b     %d0,&0x40        
         bne.b     not_fp1          
         fmovem.x  %fp1,user_fp1(%a6) 
         rts                        
not_fp1:                            
         cmp.b     %d0,&0x20        
         bne.b     not_fp2          
         fmovem.x  %fp2,user_fp2(%a6) 
         rts                        
not_fp2:                            
         cmp.b     %d0,&0x10        
         bne.b     not_fp3          
         fmovem.x  %fp3,user_fp3(%a6) 
         rts                        
not_fp3:                            
         rts                        
                                    
e1_sto:                             
         bsr.l     g_opcls          # returns opclass in d0
         cmpi.b    %d0,&3           
         beq       opc011           # branch if opclass 3
         move.l    cmdreg1b(%a6),%d0 
         bfextu    %d0{&6:&3},%d0   # extract destination register
         bra.b     sto_fp           
                                    
opc011:                             
         bsr.l     g_dfmtou         # returns dest format in d0
#				;ext=00, sgl=01, dbl=10
         move.l    %a0,%a1          # save source addr in a1
         move.l    exc_ea(%a6),%a0  # get the address
         cmpi.l    %d0,&0           # if dest format is extended
         beq.w     dest_ext         # then branch
         cmpi.l    %d0,&1           # if dest format is single
         beq.b     dest_sgl         # then branch
#
#	fall through to dest_dbl
#
                                    
#
#	dest_dbl --- write double precision value to user space
#
#Input
#	a0 -> destination address
#	a1 -> source in extended precision
#Output
#	a0 -> destroyed
#	a1 -> destroyed
#	d0 -> 0
#
#Changes extended precision to double precision.
# Note: no attempt is made to round the extended value to double.
#	dbl_sign = ext_sign
#	dbl_exp = ext_exp - $3fff(ext bias) + $7ff(dbl bias)
#	get rid of ext integer bit
#	dbl_mant = ext_mant{62:12}
#
#	    	---------------   ---------------    ---------------
#  extended ->  |s|    exp    |   |1| ms mant   |    | ls mant     |
#	    	---------------   ---------------    ---------------
#	   	 95	    64    63 62	      32      31     11	  0
#				     |			     |
#				     |			     |
#				     |			     |
#		 	             v   		     v
#	    		      ---------------   ---------------
#  double   ->  	      |s|exp| mant  |   |  mant       |
#	    		      ---------------   ---------------
#	   	 	      63     51   32   31	       0
#
dest_dbl:                            
         clr.l     %d0              # clear d0
         move.w    local_ex(%a1),%d0 # get exponent
         sub.w     &0x3fff,%d0      # subtract extended precision bias
         cmp.w     %d0,&0x4000      # check if inf
         beq.b     inf              # if so, special case
         add.w     &0x3ff,%d0       # add double precision bias
         swap      %d0              # d0 now in upper word
         lsl.l     &4,%d0           # d0 now in proper place for dbl prec exp
         tst.b     local_sgn(%a1)   
         beq.b     get_mant         # if postive, go process mantissa
         bset.l    &31,%d0          # if negative, put in sign information
#				; before continuing
         bra.b     get_mant         # go process mantissa
inf:                                
         move.l    &0x7ff00000,%d0  # load dbl inf exponent
         clr.l     local_hi(%a1)    # clear msb
         tst.b     local_sgn(%a1)   
         beq.b     dbl_inf          # if positive, go ahead and write it
         bset.l    &31,%d0          # if negative put in sign information
dbl_inf:                            
         move.l    %d0,local_ex(%a1) # put the new exp back on the stack
         bra.b     dbl_wrt          
get_mant:                            
         move.l    local_hi(%a1),%d1 # get ms mantissa
         bfextu    %d1{&1:&20},%d1  # get upper 20 bits of ms
         or.l      %d1,%d0          # put these bits in ms word of double
         move.l    %d0,local_ex(%a1) # put the new exp back on the stack
         move.l    local_hi(%a1),%d1 # get ms mantissa
         move.l    &21,%d0          # load shift count
         lsl.l     %d0,%d1          # put lower 11 bits in upper bits
         move.l    %d1,local_hi(%a1) # build lower lword in memory
         move.l    local_lo(%a1),%d1 # get ls mantissa
         bfextu    %d1{&0:&21},%d0  # get ls 21 bits of double
         or.l      %d0,local_hi(%a1) # put them in double result
dbl_wrt:                            
         move.l    &0x8,%d0         # byte count for double precision number
         exg       %a0,%a1          # a0=supervisor source, a1=user dest
         bsr.l     mem_write        # move the number to the user's memory
         rts                        
#
#	dest_sgl --- write single precision value to user space
#
#Input
#	a0 -> destination address
#	a1 -> source in extended precision
#
#Output
#	a0 -> destroyed
#	a1 -> destroyed
#	d0 -> 0
#
#Changes extended precision to single precision.
#	sgl_sign = ext_sign
#	sgl_exp = ext_exp - $3fff(ext bias) + $7f(sgl bias)
#	get rid of ext integer bit
#	sgl_mant = ext_mant{62:12}
#
#	    	---------------   ---------------    ---------------
#  extended ->  |s|    exp    |   |1| ms mant   |    | ls mant     |
#	    	---------------   ---------------    ---------------
#	   	 95	    64    63 62	   40 32      31     12	  0
#				     |	   |
#				     |	   |
#				     |	   |
#		 	             v     v
#	    		      ---------------
#  single   ->  	      |s|exp| mant  |
#	    		      ---------------
#	   	 	      31     22     0
#
dest_sgl:                            
         clr.l     %d0              
         move.w    local_ex(%a1),%d0 # get exponent
         sub.w     &0x3fff,%d0      # subtract extended precision bias
         cmp.w     %d0,&0x4000      # check if inf
         beq.b     sinf             # if so, special case
         add.w     &0x7f,%d0        # add single precision bias
         swap      %d0              # put exp in upper word of d0
         lsl.l     &7,%d0           # shift it into single exp bits
         tst.b     local_sgn(%a1)   
         beq.b     get_sman         # if positive, continue
         bset.l    &31,%d0          # if negative, put in sign first
         bra.b     get_sman         # get mantissa
sinf:                               
         move.l    &0x7f800000,%d0  # load single inf exp to d0
         tst.b     local_sgn(%a1)   
         beq.b     sgl_wrt          # if positive, continue
         bset.l    &31,%d0          # if negative, put in sign info
         bra.b     sgl_wrt          
                                    
get_sman:                            
         move.l    local_hi(%a1),%d1 # get ms mantissa
         bfextu    %d1{&1:&23},%d1  # get upper 23 bits of ms
         or.l      %d1,%d0          # put these bits in ms word of single
                                    
sgl_wrt:                            
         move.l    %d0,l_scr1(%a6)  # put the new exp back on the stack
         move.l    &0x4,%d0         # byte count for single precision number
         tst.l     %a0              # users destination address
         beq.b     sgl_dn           # destination is a data register
         exg       %a0,%a1          # a0=supervisor source, a1=user dest
         lea.l     l_scr1(%a6),%a0  # point a0 to data
         bsr.l     mem_write        # move the number to the user's memory
         rts                        
sgl_dn:                             
         bsr.l     get_fline        # returns fline word in d0
         and.w     &0x7,%d0         # isolate register number
         move.l    %d0,%d1          # d1 has size:reg formatted for reg_dest
         or.l      &0x10,%d1        # reg_dest wants size added to reg#
         bra.l     reg_dest         # size is X, rts in reg_dest will
#				;return to caller of dest_sgl
                                    
dest_ext:                            
         tst.b     local_sgn(%a1)   # put back sign into exponent word
         beq.b     dstx_cont        
         bset.b    &sign_bit,local_ex(%a1) 
dstx_cont:                            
         clr.b     local_sgn(%a1)   # clear out the sign byte
                                    
         move.l    &0x0c,%d0        # byte count for extended number
         exg       %a0,%a1          # a0=supervisor source, a1=user dest
         bsr.l     mem_write        # move the number to the user's memory
         rts                        
                                    
                                    
	version 3
