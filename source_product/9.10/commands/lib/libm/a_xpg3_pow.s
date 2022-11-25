# @(#) $Revision: 70.1 $      
#
	version 2
      		set	DOMAIN,1
        	set	OVERFLOW,3
         	set	UNDERFLOW,4

	data

excpt:	long	0,0,0,0,0,0,0,0

pow:	byte	112,111,119,0			#"pow"

powmsg:	byte	112,111,119,58,32,68,79,77	#"pow: DOMAIN error"
	byte	65,73,78,32,101,114,114,111
	byte	114,10,0

	text
ifdef(`_NAMESPACE_CLEAN',`
	global __pow
	sglobal _pow',`
	global _pow ')

                set     EDOM,33
                set     ERANGE,34
                set     addl_f0_f0,0x4000
                set     addl_f2_f0,0x4008
                set     addl_f2_f4,0x400c
                set     addl_f4_f0,0x4010
                set     addl_f4_f2,0x4012
                set     addl_f6_f0,0x4018
                set     addl_f6_f4,0x401c
                set     bogus4,0x18            #offset to do 4 bogus word reads
                set     divl_f4_f0,0x4070
                set     minuszero,0x80000000      #top 32 bits of the real value -0
                set     movf_f0_m,0x456c
                set     movf_f1_m,0x4568
                set     movf_f2_m,0x4564
                set     movf_f3_m,0x4560
                set     movf_f4_m,0x455c
                set     movf_f5_m,0x4558
                set     movf_m_f0,0x44fc
                set     movf_m_f1,0x44f8
                set     movf_m_f2,0x44f4
                set     movf_m_f3,0x44f0
                set     movf_m_f4,0x44ec
                set     movf_m_f5,0x44e8
                set     movf_m_f6,0x44e4
                set     movf_m_f7,0x44e0
                set     movil_m_f2,0x4528
                set     movl_f0_f4,0x4444
                set     movl_f0_f6,0x4446
                set     movl_f2_f4,0x444c
                set     movl_f2_f6,0x444e
                set     mull_f0_f0,0x4040
                set     mull_f0_f2,0x4042
                set     mull_f2_f0,0x4048
                set     mull_f2_f4,0x404c
                set     mull_f4_f2,0x4052
                set     mull_f6_f0,0x4058
                set     mull_f6_f2,0x405a
                set     mull_f6_f4,0x405c
                set     subl_f2_f0,0x4028
                set     subl_f4_f0,0x4030
                set     subl_f4_f2,0x4032
                set     subl_f6_f4,0x403c

ifdef(`_NAMESPACE_CLEAN',`
__pow:')
_pow:	
	ifdef(`PROFILE',`
	mov.l	&p_pow,%a0
	jsr	mcount
	')
	tst.w	flag_68881	#68881 present?
	bne.w	c_pow
	link    %a6,&0		#compute x^y
#	trap	#2		ensure enough stack space
#	dc.w	-12
	movem.l %d2-%d3,-(%sp)     #must save these registers
	movem.l (8,%a6),%d0-%d3	#retrieve x and y
	movem.l	%d0-%d3,excpt+8	#save x and y in case of error
	tst.w   float_soft	#see if hardware is present
	beq.w     flptA1          #branch if use the hardware routine
	   jsr    soft_pow 
	   movem.l (%sp)+,%d2-%d3
	   unlk    %a6
	   rts
flptA1:  jsr   flpt_pow 
       	movem.l (%sp)+,%d2-%d3
	unlk   %a6
	rts

#******************************************************************************
#
#       Procedure  : flpt_pow
#
#       Description: Compute x^y, x in (d0,d1), y in (d2,d3).
#                    This algorithm is taken from the book "Software Manual 
#		     for the Elementary Functions" by William Cody and 
#		     William Waite.
#
#       Author     : Paul Beiser
#
#       Revisions  : 1.0  06/01/81
#                    2.0  09/01/83  For:
#                            o To check for -0 as a valid input
#                            o Hardware floating point
#		     3.0  12/27/84 - added matherr capability (mwm)
#
#       Parameters : (d0,d1)    - x
#	             (d2-d3) 	- y
#
#       Registers  : a0         - address of the floating point card
#                    -(sp)      - sign of the result
#                    d4,d5      - results of the bogus reads
#
#       Result     : Returned in (d0,d1)
#
#       Error(s)   : 0^y, with y<=0
#                    x^y, x<=0 and y not a 32 bit integer
#                    Real underflow
#                    Real overflow
#                    See the "Error Branches" section for more details.
#
#       References : compare, adx, setxp, intxp, flpt_horner, rndzero,
#                    rellnt, lntrel, int, float_loc,
#                    cff_powp, cff_powq, tb_a1, tb_a2, _errno
#
#	Miscel     : Stack space requests are 8 larger than the maximum
#		     calculated by hand to ensure enough space.
#
#******************************************************************************

flpt_pow:
#	trap	#2
#	dc.w	-62
	movem.l	%a2-%a6/%d4-%d7,-(%sp)	#save dedicated registers
	lea     float_loc,%a0            #point to the start of the hardware
	movem.l %d2-%d3,-(%sp)		#save y for use later
	cmp.l   %d0,&minuszero   	#check x for a -0
	bne.w     fpowA1
	   moveq   &0,%d0   	   	#else convert -0 to +0
fpowA1:  cmp.l   %d2,&minuszero           #check y for a -0
	bne.w      fpowA2
	   moveq   &0,%d2 		#else convert -0 to +0
fpowA2:  clr.w	-(%sp)			#sign of the result is positive
	tst.l   %d0       		#test the sign of x
	bmi.w	fnegbase		#negative base - allowed if y is integer
	bne.w	fstep3			#continue if x > 0
	   tst.l   %d2			   #x=0, check the sign of y
	   ble.w	   err1			   #branch if 0 to a non-positive power 
	      moveq   &0,%d1		      #else 0^y, y>0, which is 0
	      bra.w     fdone
#
#  First, check for x=1. Then determine m, and place it on the tos.
#
fstep3:	tst.l   %d1        		
	bne.w	fpow_1			#check for (d0,d1)=1=(3ff00000,00000000)
	   cmp.l   %d0,&0x3ff00000   	   #check top part
	   beq.w	   fdone                   #branch if x was plus or minus 1
fpow_1:	move.l	&0x7fafffff,%d4
	move.l	(2,%sp),%d5
	cmp.l	%d4,%d5
	ble.w	err3
	bchg	%d4,%d4
	cmp.l	%d4,%d5
	bls.w	err4
	jsr	intxp			#d7 <- m; (d0,d1) unchangd
	move.w	%d7,-(%sp)		#save m on the stack for now
#
#  Determine g (r = 0).
#
	moveq	&0,%d7
	jsr	setxp			#(d0,d1) <- g
	move.l	%d0,(movf_m_f1,%a0)	 #(f1,f0) <- g
	move.l	%d1,(movf_m_f0,%a0)
#
#  Now have (f1,f0) = g, and tos = m, sign of result. Determine the 
#  value of p.
#
	lea	tb_a1,%a5		#point to a1 array
	moveq	&1,%d7			#initialize p to 1
	move.l	(72,%a5),%d2		#retrieve a1(9) (8 bytes per entry)
	move.l	(76,%a5),%d3		#9*8 + 4 (to get second part of number)
	jsr	compare			#is g <= a1(9) ?
	bgt.w	fpow1		        #branch if not true
	   moveq   &9,%d7		   #else set p to 9
fpow1:	move.w	%d7,%d6			#scratch register
	addq.w	&4,%d6			#p + 4
	lsl.w	&3,%d6			#8 * (p + 4) to get byte offset
	move.l	(0,%a5,%d6.w),%d2		#retrieve a1(p+4)
	move.l	(4,%a5,%d6.w),%d3
	jsr	compare			#is g <= a1(p+4) ?
	bgt.w	fpow2			#branch if not true
	   addq.w  &4,%d7   		   #else adjust value of p
fpow2:	move.w	%d7,%d6			#scratch register
	addq.w	&2,%d6			#p + 2
	lsl.w	&3,%d6			#8 * (p + 2) to get byte offset
	move.l	(0,%a5,%d6.w),%d2		#retrieve a1(p+2)
	move.l	(4,%a5,%d6.w),%d3
	jsr	compare			#is g <= a1(p+2) ?
	bgt.w	fstep7			#branch if not true
	   addq.w  &2,%d7	   	   #else adjust value of p
#
#  Save the value of p+1 and compute z.
#
fstep7:	addq.w	&1,%d7			#p+1 (p is odd, so p+1 is even)
	movea.w	%d7,%a4			#a4 <- p+1
	lsl.w	&3,%d7			#get byte offset (8 bytes/real entry)
	move.l	(0,%a5,%d7.w),(movf_m_f3,%a0)  #(f3,f2) <- a1(p+1)
	move.l	(4,%a5,%d7.w),(movf_m_f2,%a0)
	tst.w   (movl_f2_f6,%a0)		#(f7,f6) <- a1(p+1)
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (movl_f0_f4,%a0)		#(f5,f4 ) <- g
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (subl_f2_f0,%a0)   	#(f1,f0) <- g - a1(p+1)
	movem.l (bogus4,%a0),%d4-%d5
	move.w	%a4,%d7			#retrieve p+1
	lsl.w	&2,%d7			#((p+1)/2) * 8 to get byte offset
	lea	tb_a2,%a6		#point to a2 array
	move.l	(0,%a6,%d7.w),(movf_m_f3,%a0)  #(f3,f2) <- a2((p+1)/2)
	move.l	(4,%a6,%d7.w),(movf_m_f2,%a0)
	tst.w   (subl_f2_f0,%a0)		#(f1,f0) <- g - a1(p+1) - a2((p+1)/2)
	movem.l (bogus4,%a0),%d4-%d5        #(f1,f0) <- numerator
	tst.w   (addl_f6_f4,%a0)      	#(f5,f4) <- g + a1(p+1) = denominator
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (divl_f4_f0,%a0)		#(f1,f0) <- num/den = z
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f0_f0,%a0)		#(f1,f0) <- z+z, abs(z) <= .044 = 
	movem.l (bogus4,%a0),%d4-%d5        #($3fa6872b,020c49bb)
#
#  Now have (f1,f0) = z, a4.w = p+1, tos = m, sign of the result.
#  Determine u2.
#
	tst.w   (movl_f0_f6,%a0)		#(f7,f6) <- z
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (mull_f0_f0,%a0)		#(f1,f0) <- z*z = v
	movem.l (bogus4,%a0),%d4-%d5
	move.w	%a4,-(%sp)		#save p+1 on stack
	movea.l	(movf_f1_m,%a0),%a4	#compute p(v)
	movea.l	(movf_f0_m,%a0),%a5	  #(a4,a5) <- v
	lea	cff_powp,%a6		#point to the coefficients
	moveq	&3,%d0			#degree of the polynomial
	jsr	flpt_horner		#(f1,f0) <- p(v) (f7,f6) untouched
	movem.l %a4-%a5,(movf_m_f3,%a0)	  #(f3,f2) <- v
	movea.w	(%sp)+,%a4		#restore p+1 in a4
	tst.w   (mull_f2_f0,%a0)		#(f1,f0) <- v * p(v)
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (mull_f6_f0,%a0)		#(f1,f0) <- v * p(v) * z = r
	movem.l (bogus4,%a0),%d4-%d5
	move.l	&0x3fdc551d,(movf_m_f3,%a0)   #(f3,f2) <- k
	move.l	&0x94ae0bf8,(movf_m_f2,%a0)
	tst.w   (movl_f2_f4,%a0)		#(f5,f4) <- k
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (mull_f0_f2,%a0)		#(f3,f2) <- k * r
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f2_f0,%a0)		#(f1,f0) <- r + k*r = r
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (mull_f6_f4,%a0)		#(f5,f4) <- k * z
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f4_f0,%a0)		#(f1,f0) <- r + k*z
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f6_f0,%a0)		#(f1,f0) <- (r + k*z) + z = u2
	movem.l (bogus4,%a0),%d4-%d5
#
#  Now have (f1,f0) = u2, a4.w = p+1, tos = m, sign of result.
#  Determine u1.
#
	move.w	(%sp)+,%d0		#retrieve m; tos now has sign of result
	asl.w	&4,%d0			#m * 16
	move.w	%a4,%d6			#get p + 1
	subq.w	&1,%d6
	sub.w	%d6,%d0			#m*16 - p
	ext.l	%d0			#make it a proper 32 bit number
	move.l  %d0,(movil_m_f2,%a0)	  #(f3,f2) <- convert to a real number
	movem.l (bogus4,%a0),%d4-%d5
	move.l  &0x3fb00000,(movf_m_f5,%a0)  #(f5,f4) <- 1/16
	move.l  &0,(movf_m_f4,%a0)
	tst.w   (mull_f4_f2,%a0)		#(f3,f2) <- float(m*16 - p) * 1/16 = u1
	movem.l (bogus4,%a0),%d4-%d5
#
#  Define reduce(v) as adx(-4,rndzero(adx(4,v))), which is equivalent to the
#  book's definition of float(int(16*v)) * 1/16.
#
#  Compute y1 and y2. (f1,f0) = u2, (f3,f2) = u1.
#
	move.l	(2,%sp),%d0		#retrieve y
	move.l	(6,%sp),%d1
	moveq	&4,%d7			#16 * y
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#y1
	movea.l	%d0,%a2			#(a2,a3) <- y1  (for later)
	movea.l %d1,%a3
	move.l  (2,%sp),(movf_m_f5,%a0) 	  #(f5,f4) <- y
	move.l  (6,%sp),(movf_m_f4,%a0)
	move.l  %d0,(movf_m_f7,%a0)	  #(f7,f6) <- y1
	move.l  %d1,(movf_m_f6,%a0)
	tst.w   (subl_f6_f4,%a0)		#(f5,f4) <- y - y1 = y2
	movem.l (bogus4,%a0),%d4-%d5
#
#  Compute w, w1, w2, and iw1.
#
	tst.w   (mull_f2_f4,%a0)		#(f5,f4) <- y2 * u1
	movem.l (bogus4,%a0),%d4-%d5
	move.l  (2,%sp),(movf_m_f7,%a0) 	  #(f7,f6) <- y
	move.l  (6,%sp),(movf_m_f6,%a0)
	tst.w   (mull_f6_f0,%a0)		#(f1,f0) <- u2 * y
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f4_f0,%a0)		#(f1,f0) <- u2*y + u1*y2 = w
	movem.l (bogus4,%a0),%d4-%d5
#
#  Now have (f1,f0) = w, (f3,f2) = u1, (a2,a3) = y1.
#
	move.l  (movf_f1_m,%a0),%d0	  #(d0,d1) <- w
	move.l  (movf_f0_m,%a0),%d1
	moveq	&4,%d7			#reduce w
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w1
	move.l	%d0,(movf_m_f5,%a0)	  #(f5,f4) <- w1
	move.l  %d1,(movf_m_f4,%a0)
	tst.w   (subl_f4_f0,%a0)		#(f1,f0) <- w - w1 = w2
	movem.l (bogus4,%a0),%d4-%d5
	move.l  %a2,(movf_m_f7,%a0)	  #(f7,f6) <- y1
	move.l  %a3,(movf_m_f6,%a0)
	tst.w   (mull_f6_f2,%a0)		#(f3,f2) <- u1 * y1
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f4_f2,%a0)		#(f3,f2) <- w1 + u1*y1 = w
	movem.l (bogus4,%a0),%d4-%d5
#
#  Now have (f3,f2) = w, (f1,f0) = w2.
#  Continue iterations to produce final w, w1, w2, and iw1.
#
 	move.l  (movf_f3_m,%a0),%d0	  #(d0,d1) <- w
	move.l  (movf_f2_m,%a0),%d1
	moveq	&4,%d7			#reduce w
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w1
	move.l  %d0,(movf_m_f5,%a0)	  #(f5,f4) <- w1
	move.l  %d1,(movf_m_f4,%a0)
	tst.w   (subl_f4_f2,%a0)		#(f3,f2) <- w - w1
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f2_f0,%a0)		#(f1,f0) <- w2 + (w - w1) = w2
	movem.l (bogus4,%a0),%d4-%d5
	move.l  (movf_f1_m,%a0),%d0	  #(d0,d1) <- w2
	move.l  (movf_f0_m,%a0),%d1
	moveq	&4,%d7			#reduce w2
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w
	move.l  %d0,(movf_m_f3,%a0)	  #(f3,f2) <- w
	move.l  %d1,(movf_m_f2,%a0)
	tst.w   (addl_f2_f4,%a0)		#(f5,f4) <- w + w1
	movem.l (bogus4,%a0),%d4-%d5
	move.l  (movf_f5_m,%a0),%d0        #(d0,d1) <- w + w1
	move.l  (movf_f4_m,%a0),%d1
	moveq	&4,%d7			#multiply by 16
	jsr	adx
	jsr	int			#int(16*(w + w1))
	movea.l	%d0,%a2			#no longer need w1 - save iw1 in a2
	tst.w   (subl_f2_f0,%a0)		#(f1,f0) <- w2 - w = w2
	movem.l (bogus4,%a0),%d4-%d5
#
#  Now have (f1,f0) = w2, a2 = iw1. Test for out-of-range.
#
	cmpa.l	%a2,&16383		#check against int(16*log2(xmax)-1)
	bgt.w	err3			#overflow
	cmpa.l	%a2,&-16351		#check against int(16*log2(xmin)+1)
	blt.w	err4			#underflow
#
#  Compute p' and m'. Iw1 (a2.w) must be a 16 bit quantity at this stage!
#
	move.l	(movf_f1_m,%a0),%d6	#retrieve sign of w2
	ble.w	fstep13_1		#branch if no compensation necessary
	   addq.w  &1,%a2		   #iw1 <- iw1 + 1
	   move.l  &0xbfb00000,(movf_m_f3,%a0)	#-1/16
	   move.l  &0,(movf_m_f2,%a0)
	   tst.w   (addl_f2_f0,%a0)	   #(f1,f0) <- w2 - 1/16 = w2
	   movem.l (bogus4,%a0),%d4-%d5
fstep13_1: moveq	&0,%d7			#initial guess for i
	move.w	%a2,%d6			#get iw1
	blt.w	fs13			#if iw1 < 0, then i is zero
	   addq.w  &1,%d7		   #else i = 1
fs13:	asr.w	&4,%d6			#iw1/16
	bpl.w	fs14			#div must be to zero, not to the left;
	   addq.w  &1,%d6		   #therefore, -13/16 = 0, not -1
fs14:	add.w	%d6,%d7			#m' <- iw1/16 + i
	movea.w	%d7,%a3			#a3.w <- m'
	asl.w	&4,%d7			#m'*16
	sub.w	%a2,%d7			#p' <- m'*16 - iw1
	movea.w	%d7,%a1			#a1.w <- p'
#
#  Compute z. Have a3.w = m', a1.w = p', and (f1,f0) = w2.
#  -1/16 <= w2 <= 0; i.e. ($bfb00000,00000000) <= w2 <= 0.
#
	tst.w   (movl_f0_f6,%a0)		#(f7,f6) <- w2  (untouched by horner)
	movem.l (bogus4,%a0),%d4-%d5
	move.l  (movf_f1_m,%a0),%a4	  #(a4,a5) <- w2
	move.l  (movf_f0_m,%a0),%a5
	lea	cff_powq,%a6		#point to coefficients
	moveq	&6,%d0			#degree of the polynomial
	jsr	flpt_horner		#(f1,f0) <- q(w2)
	tst.w   (mull_f6_f0,%a0)		#(f1,f0) <- q(w2) * w2 = z
	movem.l (bogus4,%a0),%d4-%d5
#
#  Finish (finally!) the computation.
#
	addq.w	&1,%a1			#p' + 1
	move.w	%a1,%d7
	lsl.w	&3,%d7			#get offset (8 bytes per entry)
	lea	tb_a1,%a6		#point to a1 table
	move.l	(0,%a6,%d7.w),(movf_m_f3,%a0)   #(f3,f2) <-  a1(p'+1)
	move.l	(4,%a6,%d7.w),(movf_m_f2,%a0)
	tst.w   (mull_f2_f0,%a0)		#(f1,f0) <- z * a1(p'+1)
	movem.l (bogus4,%a0),%d4-%d5
	tst.w   (addl_f2_f0,%a0)		#(f1,f0) <- z * a1(p'+1) + a1(p'+1) = z
	movem.l (bogus4,%a0),%d4-%d5
	move.l  (movf_f1_m,%a0),%d0	#get z
	move.l  (movf_f0_m,%a0),%d1
	move.w	%a3,%d7			#get m'
	jsr	adx			#augment z by m'
#
#  Check sign of result and return.
#
fdone:	move.w  (%sp)+,%d7		#check the sign of the result
	addq.l  &8,%sp			#remove y from the stack
	tst.w   %d7
	beq.w	fdone1			#branch if positive
	   bset	   &31,%d0		   #else set sign to -
fdone1:  movem.l	(%sp)+,%a2-%a6/%d4-%d7	#restore dedicated registers
	rts
#
#  Special code for y being integral and x < 0.
#
fnegbase: movea.l %d0,%a2			#(a2,a3) <- x
	movea.l %d1,%a3
	move.l	%d2,%d0    		#get y into correct registers
	move.l	%d3,%d1    
	move.l	&0xc1e00000,%d2		#bounds checking. first check for -2^31
	moveq	&0,%d3
	jsr	compare
	blt.w	err2			#number too small
	move.l	&0x41dfffff,%d2		#check against 2^31 - 1
	move.l	&0xffc00000,%d3
	jsr	compare
	bgt.w	err2			#number too large
#
#  Y must now be in the range of 32 bit integers, so it can be converted
#  to an integer without fear of triggering an error.
#
	jsr	rellnt			#only allow 32 bit integers
	movea.w	%d0,%a5			#save bottom of integer so look at lsb
	jsr	lntrel			#if y is integral, same as before
	cmp.l	%d0,(2,%sp)		#check against value "lntrel(rellnt(y))"
	bne.w	err2			#branch if neg base to non-integral power
	cmp.l	%d1,(6,%sp)		#else, check next portion
	bne.w	err2			#branch if neg base to non-integral power
	move.w	%a5,%d0			#have integer power, so set sign
	and.b	&0x01,%d0		#check lsb
	beq.w	fsignok			#branch if even integer
	   addq.w   &1,(%sp)		   #else set sign to negative
fsignok:	move.l	%a2,%d0			#get top portion of x into d0
	bclr	&31,%d0			#make positive
	move.l  %a3,%d1          		#restore all of x
	bra.w	fstep3			#continue with the calculation

#******************************************************************************
#
#       Procedure  : soft_pow
#
#       Description: Compute x^y, x in (d0,d1), y in (d2,d3).
#                    This algorithm is taken from the book "Software Manual 
#		     for the Elementary Functions" by William Cody and 
#		     William Waite.
#
#       Author     : Paul Beiser 
# 
#       Revisions  : 1.0  06/01/81 
#                    2.0  09/01/83  For: 
#                            o To check for -0 as a valid input 
#		     3.0  12/27/84 - added matherr capability (mwm)
# 
#       Parameters : (d0,d1)    - x
#	             (d2,d3)	- y
#
#       Registers  : -(sp)      - sign of the result
#
#       Result     : Returned in (d0,d1)
#
#       Error(s)   : 0^y, with y<=0
#                    x^y, x<=0 and y not a 32 bit integer
#                    Real underflow
#                    Real overflow
#                    See the "Error Branches" section for more details.
#
#       References : compare, adx, setxp, intxp, soft_horner, rndzero,
#                    radd, rsbt, rmul, rdvd, rellnt, lntrel, int,
#                    cff_powp, cff_powq, tb_a1, tb_a2, _errno
#
#	Miscel     : Stack space requests are 8 larger than the maximum
#		     calculated by hand to ensure enough space.
#
#******************************************************************************

soft_pow:
#	trap #2
#	dc.w	-74
	movem.l	%a2-%a6/%d4-%d7,-(%sp)	#save dedicated registers
	movem.l %d2-%d3,-(%sp)		#save y
	cmp.l   %d0,&minuszero   	#check x for a -0
	bne.w     powA1
	   moveq   &0,%d0   		   #else convert -0 to +0
powA1:   cmp.l   %d2,&minuszero       	#check y for a -0
	bne.w      powA2
	   moveq   &0,%d2 		   #else convert -0 to +0
powA2:   clr.w	-(%sp)			#sign of the result is positive
	tst.l   %d0       		#test the sign of x
	bmi.w	negbase			#negative base - allowed if y is integer
	bne.w	step3			#continue if x > 0
	   tst.l   %d2     		   #x = 0 at this point; check is y <= 0?
	   ble.w	   err1			   #branch if 0 to a non-positive power 
	      moveq   &0,%d1		      #else 0^y, y>0, which is 0
	      bra.w     done
#
#  First, check for x=1. Then determine m, and place it on the tos.
#
step3:	tst.l   %d1       		
	bne.w	pow_1			#check for (d0,d1)=1=(3ff00000,00000000)
           cmp.l   %d0,&0x3ff00000          #check top part
   	   beq.w     done 	   	   #jump if x was plus or minus 1
pow_1:	move.l	&0x7fafffff,%d4
	move.l	(2,%sp),%d5
	cmp.l	%d4,%d5
	ble.w	err3
	bchg	%d4,%d4
	cmp.l	%d4,%d5
	bls.w	err4
	jsr	intxp			#d7 <- m; (d0,d1) unchangd
	move.w	%d7,-(%sp)		#save m on the stack for now
#
#  Determine g (r = 0).
#
	moveq	&0,%d7
	jsr	setxp			#(d0,d1) <- g
	movea.l	%d0,%a0			#(a0,a1) <- g
	movea.l	%d1,%a1
#
#  Now have (a0,a1) = g, and tos = m, sign of result. Determine the 
#  value of p.
#
	lea	tb_a1,%a5		#point to a1 array
	moveq	&1,%d7			#initialize p to 1
	move.l	(72,%a5),%d2		#retrieve a1(9) (8 bytes per entry)
	move.l	(76,%a5),%d3		#9*8 + 4 (to get second part of number)
	jsr	compare			#is g <= a1(9) ?
	bgt.w	pow1		        #branch if not true
	moveq	&9,%d7			#else set p to 9
pow1:	move.w	%d7,%d6			#scratch register
	addq.w	&4,%d6			#p + 4
	lsl.w	&3,%d6			#8 * (p + 4) to get byte offset
	move.l	(0,%a5,%d6.w),%d2		#retrieve a1(p+4)
	move.l	(4,%a5,%d6.w),%d3
	jsr	compare			#is g <= a1(p+4) ?
	bgt.w	pow2			#branch if not true
	   addq.w  &4,%d7   		   #else adjust value of p
pow2:	move.w	%d7,%d6			#scratch register
	addq.w	&2,%d6			#p + 2
	lsl.w	&3,%d6			#8 * (p + 2) to get byte offset
	move.l	(0,%a5,%d6.w),%d2		#retrieve a1(p+2)
	move.l	(4,%a5,%d6.w),%d3
	jsr	compare			#is g <= a1(p+2) ?
	bgt.w	step7			#branch if not true
	addq.w	&2,%d7			#else adjust value of p
#
#  Save the value of p+1 and compute z.
#
step7:	addq.w	&1,%d7			#p+1 (p is odd, so p+1 is even)
	movea.w	%d7,%a4			#a4 <- p+1
	lsl.w	&3,%d7			#get byte offset (8 bytes/real entry)
	move.l	(0,%a5,%d7.w),%d2		#retrieve a1(p+1)
	move.l	(4,%a5,%d7.w),%d3		#remember, (d0,d1) still has g !!
	movea.l	%d2,%a2			#(a2,a3) <- a1(p+1)
	movea.l	%d3,%a3
	jsr	rsbt			#g - a1(p+1)
	move.w	%a4,%d7			#retrieve p+1
	lsl.w	&2,%d7			#((p+1)/2) * 8 to get byte offset
	lea	tb_a2,%a6		#point to a2 array
	move.l	(0,%a6,%d7.w),%d2		#retrieve a2((p+1)/2)
	move.l	(4,%a6,%d7.w),%d3
	jsr	rsbt			#[ g - a1(p+1) ] - a2((p+1)/2)
	movea.l	%d0,%a5			#(a5,a6) <- numerator
	movea.l	%d1,%a6
	move.l	%a2,%d2			#retrieve a1(p+1)
	move.l	%a3,%d3
	move.l	%a0,%d0			#retrieve g
	move.l	%a1,%d1
	jsr	radd			#g + a1(p+1)
	move.l	%d0,%d2			#place denominator in correct registers
	move.l	%d1,%d3
	move.l	%a5,%d0			#restore numerator
	move.l	%a6,%d1
	jsr	rdvd			#z
	moveq	&1,%d7
	jsr	adx			#z <- z + z
	movea.l	%d0,%a0			#(a0,a1) <- z
	movea.l	%d1,%a1			#abs(z) <= .044 = ($3fa6872b,020c49bb)
#
#  Now have (a0,a1) = z, a4.w = p+1, tos = m, sign of the result.
#  Determine u2.
#
	move.l	%d0,%d2			#compute v
	move.l	%d1,%d3
	jsr	rmul			#v <- z * z
	movea.l	%d0,%a2			#(a2,a3) <- v
	movea.l	%d1,%a3
	move.w	%a4,-(%sp)		#save p+1 on stack
	movea.l	%d0,%a4			#compute p(v)
	movea.l	%d1,%a5			#(a4,a5) <- v
	lea	cff_powp,%a6		#point to the coefficients
	moveq	&3,%d0			#degree of the polynomial
	jsr	soft_horner		#(d0,d1) <- p(v)
	movea.w	(%sp)+,%a4		#restore p+1 in a4
	move.l	%a2,%d2			#restore v in (d2,d3)
	move.l	%a3,%d3
	jsr	rmul			#v * p(v)
	move.l	%a0,%d2			#retrieve z
	move.l	%a1,%d3
	jsr	rmul			#r = v * z * p(v)
	movea.l	%d0,%a5			#(a5,a6) <- r
	movea.l	%d1,%a6
	movea.l	&0x3fdc551d,%a2		#(a2,a3) <- k
	movea.l	&0x94ae0bf8,%a3
	move.l	%a2,%d2			#(d2,d3) <- k
	move.l	%a3,%d3
	jsr	rmul			#k * r
	move.l	%a5,%d2			#(d2,d3) <- r
	move.l	%a6,%d3
	jsr	radd			#r <- r + k*r
	movea.l	%d0,%a5			#(a5,a6) <- r
	movea.l	%d1,%a6
	move.l	%a2,%d0			#(d0,d1) <- k
	move.l	%a3,%d1
	move.l	%a0,%d2			#(d2,d3) <- z
	move.l	%a1,%d3
	jsr	rmul			#k * z
	move.l	%a5,%d2			#retrieve r
	move.l	%a6,%d3
	jsr	radd			#r + k*z
	move.l	%a0,%d2			#retrieve z
	move.l	%a1,%d3
	jsr	radd			#u2 = (r + k*z) + z
	movea.l	%d0,%a0			#(a0,a1) <- u2
	movea.l	%d1,%a1
#
#  Now have (a0,a1) = u2, a4.w = p+1, tos = m, sign of result.
#  Determine u1.
#
	move.w	(%sp)+,%d0		#retrieve m; tos now has sign of result
	asl.w	&4,%d0			#m * 16
	move.w	%a4,%d6			#get p + 1
	subq.w	&1,%d6
	sub.w	%d6,%d0			#m*16 - p
	ext.l	%d0			#make it a proper 32 bit number
	jsr	lntrel			#convert to a real number
	moveq	&-4,%d7			#multiply by 1/16
	jsr	adx			#u1 <- float(m*16 - p) * 1/16
	movea.l	%d0,%a4			#(a4,a5) <- u1
	movea.l	%d1,%a5
#
#  Define reduce(v) as adx(-4,rndzero(adx(4,v))), which is equivalent to the
#  book's definition of float(int(16*v)) * 1/16.
#
#  Compute y1 and y2. (a0,a1) = u2, (a2,a3) = y (next 2 instrs.), (a4,a5) = u1.
#
	movea.l	(2,%sp),%a2
	movea.l	(6,%sp),%a3
	move.l	%a2,%d0			#get y
	move.l	%a3,%d1			#reduce y to y1
	moveq	&4,%d7			#16 * y
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#y1
	movem.l	%d0-%d1,-(%sp)		#y1 needed later
	move.l	%d0,%d2
	move.l	%d1,%d3
	move.l	%a2,%d0			#get y
	move.l	%a3,%d1
	jsr	rsbt			#y2 <- y - y1
#
#  Compute w, w1, w2, and iw1.
#
	move.l	%a4,%d2			#get u1
	move.l	%a5,%d3
	jsr	rmul			#u1 * y2; y2 no longer needed
	movem.l	%d0-%d1,-(%sp)		#save u1*y2 on tos
	move.l	%a0,%d0			#get u2
	move.l	%a1,%d1
	move.l	%a2,%d2			#get y
	move.l	%a3,%d3
	jsr	rmul			#y * u2
	movem.l	(%sp)+,%d2-%d3		#get u1 * y2
	jsr	radd			#w <- u2*y + u1*y2
	movea.l	%d0,%a0			#(a0,a1) <- w
	movea.l	%d1,%a1
#
#  Now have (a0,a1) = w, (a4,a5) = u1, tos = y1.
#
	moveq	&4,%d7			#reduce w
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w1
	movea.l	%d0,%a2			#(a2,a3) <- w1
	movea.l	%d1,%a3
	move.l	%d0,%d2
	move.l	%d1,%d3
	move.l	%a0,%d0			#get w
	move.l	%a1,%d1
	jsr	rsbt			#w2 <- w - w1
	movem.l	(%sp)+,%d2-%d3		#get y1
	movea.l	%d0,%a0			#(a0,a1) <- w2
	movea.l	%d1,%a1
	move.l	%a4,%d0			#get u1
	move.l	%a5,%d1
	jsr	rmul			#u1 * y1
	move.l	%a2,%d2			#get w1
	move.l	%a3,%d3
	jsr	radd			#w <- w1 + u1*y1
	movea.l	%d0,%a4			#(a4,a5) <- w
	movea.l	%d1,%a5
#
#  Now have (a4,a5) = w (w also in (d0,d1)), (a2,a3) = w1, (a0,a1) = w2.
#  Continue iterations to produce final w, w1, w2, and iw1.
#
	moveq	&4,%d7			#reduce w
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w1
	movea.l	%d0,%a2			#(a2,a3) <- w1
	movea.l	%d1,%a3
	move.l	%d0,%d2
	move.l	%d1,%d3
	move.l	%a4,%d0			#get w
	move.l	%a5,%d1
	jsr	rsbt			#w - w1
	move.l	%a0,%d2			#get w2
	move.l	%a1,%d3
	jsr	radd			#w2 <- w2 + (w - w1)
	movea.l	%d0,%a0			#(a0,a1) <- w2
	movea.l	%d1,%a1
	moveq	&4,%d7			#reduce w2
	jsr	adx
	jsr	rndzero			#float(int())
	moveq	&-4,%d7			#multiply by 1/16 = 0.0625
	jsr	adx			#w
	movea.l	%d0,%a4			#(a4,a5) <- w
	movea.l	%d1,%a5
	move.l	%a2,%d2			#get w1
	move.l	%a3,%d3
	jsr	radd			#w + w1
	moveq	&4,%d7			#multiply by 16
	jsr	adx
	jsr	int			#int(16*(w + w1))
	movea.l	%d0,%a2			#no longer need w1 - save iw1 in a2
	move.l	%a0,%d0			#get w2
	move.l	%a1,%d1
	move.l	%a4,%d2			#get w
	move.l	%a5,%d3
	jsr	rsbt			#w2 <- w2 - w
	movea.l	%d0,%a4			#(a4,a5) <- w2
	movea.l	%d1,%a5
#
#  Now have (a4,a5) = w2, a2 = iw1. Test for out-of-range.
#
	cmpa.l	%a2,&16383		#check against int(16*log2(xmax)-1)
	bgt.w	err3			#overflow
	cmpa.l	%a2,&-16351		#check against int(16*log2(xmin)+1)
	blt.w	err4			#underflow
#
#  Compute p' and m'. Iw1 (a2.w) must be a 16 bit quantity at this stage!
#
	move.l	%a4,%d6			#retrieve sign of w2
	ble.w	step13_1		#branch if no compensation necessary
	addq.w	&1,%a2			#iw1 <- iw1 + 1
	move.l	%a4,%d0			#get w2
	move.l	%a5,%d1
	move.l	&0xbfb00000,%d2		#-1/16
	moveq	&0,%d3
	jsr	radd			#w2 - 1/16
	movea.l	%d0,%a4			#(a4,a5) <- w2
	movea.l	%d1,%a5
step13_1: moveq	&0,%d7			#initial guess for i
	move.w	%a2,%d6			#get iw1
	blt.w	s13			#if iw1 < 0, then i is zero
	addq.w	&1,%d7			#else i = 1
s13:	asr.w	&4,%d6			#iw1/16
	bpl.w	s14			#div must be to zero, not to the left;
	addq.w	&1,%d6			#therefore, -13/16 = 0, not -1
s14:	add.w	%d6,%d7			#m' <- iw1/16 + i
	movea.w	%d7,%a0			#a0.w <- m'
	asl.w	&4,%d7			#m'*16
	sub.w	%a2,%d7			#p' <- m'*16 - iw1
	movea.w	%d7,%a1			#a1.w <- p'
#
#  Compute z. Have a0.w = m', a1.w = p', and (a4,a5) = w2.
#  -1/16 <= w2 <= 0; i.e. ($bfb00000,00000000) <= w2 <= 0.
#
	lea	cff_powq,%a6		#point to coefficients; (a4,a5) correct
	moveq	&6,%d0			#degree of the polynomial
	jsr	soft_horner
	move.l	%a4,%d2			#get w2
	move.l	%a5,%d3
	jsr	rmul			#z <- w2 * q(w2)
#
#  Finish (finally!) the computation.
#
	addq.w	&1,%a1			#p' + 1
	move.w	%a1,%d7
	lsl.w	&3,%d7			#get offset (8 bytes per entry)
	lea	tb_a1,%a6		#point to a1 table
	move.l	(0,%a6,%d7.w),%d2		#get a1(p'+1)
	move.l	(4,%a6,%d7.w),%d3
	movea.l	%d2,%a1			#(a1,a2) <- a1(p'+1)
	movea.l	%d3,%a2
	jsr	rmul			#a1(p'+1) * z
	move.l	%a1,%d2			#get a1(p'+1)
	move.l	%a2,%d3
	jsr	radd			#z <- a1(p'+1) + a1(p'+1)*z
	move.w	%a0,%d7			#get m'
	jsr	adx			#augment z by m'
#
#  Check sign of result and return.
#
done:	move.w  (%sp)+,%d7		#check the sign of the result
	addq.l  &8,%sp			#remove y
	tst.w   %d7
	beq.w     done1
	   bset	   &31,%d0		   #else set sign to -
done1:	movem.l	(%sp)+,%a2-%a6/%d4-%d7	#restore dedicated registers
	rts
#
#  Special code for y being integral and x < 0.
#
negbase: movea.l %d0,%a2			#(a2,a3) <- x	
	movea.l %d1,%a3
	move.l	%d2,%d0     		#get y into correct registers
	move.l	%d3,%d1    
	move.l	&0xc1e00000,%d2		#bounds checking. first check for -2^31
	moveq	&0,%d3
	jsr	compare
	blt.w	err2			#number too small
	move.l	&0x41dfffff,%d2		#check against 2^31 - 1
	move.l	&0xffc00000,%d3
	jsr	compare
	bgt.w	err2			#number too large
#
#  Y must now be in the range of 32 bit integers, so it can be converted
#  to an integer without fear of triggering an error.
#
	jsr	rellnt			#only allow 32 bit integers
	movea.w	%d0,%a5			#save bottom of integer so look at lsb
	jsr	lntrel			#if y is integral, same as before
	cmp.l	%d0,(2,%sp)		#check against value "lntrel(rellnt(y))"
	bne.w	err2			#branch if neg base to non-integral power
	cmp.l	%d1,(6,%sp)		#else, check next portion
	bne.w	err2			#branch if neg base to non-integral power
	move.w	%a5,%d0			#have integer power, so set sign
	and.b	&0x01,%d0		#check lsb
	beq.w	signok			#branch if even integer
	   addq.w   &1,(%sp)		   #else set sign to negative
signok:	move.l	%a2,%d0    		#get top portion of x into d0
	bclr	&31,%d0			#make positive
	move.l  %a3,%d1                   #get rest of x
	bra.w	step3			#continue with the calculation

#******************************************************************************
#
#                              Error Branches
#
err1:
err2:	moveq	&DOMAIN,%d0	#domain error
	move.l	%d0,excpt
	move.l	&pow,excpt+4	#name
	clr.l	excpt+24	#return value is zero
	clr.l	excpt+28
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#matherr(&excpt)
	jsr	_matherr	#matherr(&excpt)
 ')
	addq.w	&4,%sp
	tst.l	%d0		#user want error reported?
	bne.w	err2a
	moveq	&EDOM,%d0	#domain error
	move.l	%d0,_errno
	pea	18.w		#length of message
	pea	powmsg		#message
	pea	2.w		#stderr
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`
	jsr	_write
 ')
	lea	(12,%sp),%sp
err2a:	move.l	excpt+24,%d0	#return value
	move.l	excpt+28,%d1
	adda.w	&10,%sp
	bra.w	done1

err3:	moveq	&OVERFLOW,%d0	#overflow error
	move.l	%d0,excpt
	move.l	&0x7FEFFFFF,%d0	#return value is HUGE
	tst.w	(%sp)
	beq.b	err3a
	bchg	%d0,%d0
err3a:	move.l	%d0,excpt+24	#return value is HUGE
	moveq	&-1,%d0
	move.l	%d0,excpt+28
	bra.w	err4a

err4:	moveq	&UNDERFLOW,%d0	#overflow error
	move.l	%d0,excpt
	clr.l	excpt+24	#return value is zero
	clr.l	excpt+28

err4a:	move.l	&pow,excpt+4	#name
	pea	excpt
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`	#matherr(&excpt)
	jsr	_matherr	#matherr(&excpt)
 ')
	addq.w	&4,%sp
	tst.l	%d0		#user want error reported?
	bne.w	err4b
	moveq	&ERANGE,%d0	#range error
	move.l	%d0,_errno
err4b:	move.l	excpt+24,%d0	#return value
	move.l	excpt+28,%d1
	adda.w	&10,%sp
	bra.w	done1

#******************************************************************************
#
#       Procedure int
#
#       Description:
#               Truncate a real number to a 32 bit integer. This
#               procedure is used only in the x^y evaluation. Because
#               INT is called only from x^y, no ESCAPE is generated with
#               numbers out of range. No A registers are changed.
#
#       Parameters:
#               (d0,d1) - real to be truncated
#
#       The result is returned in d0.
#
#       Error conditions:
#               If the numeric item is too large in magnitude,
#               d0 is returned with either 2^31 - 1 or -2^31.
#
#       External references:
#               There are none.
#
int:	move.w	%d0,%d1		#shift everthing to the right by 16
	swap	%d1		#d1 is correct
	clr.w	%d0
	swap	%d0		#d0 is correct
	move.w	%d0,%d7		#save the sign of the number
	move.w	%d0,%d6
	and.w	&0x7ff0,%d6	#mask out the sign
	lsr.w	&4,%d6
	sub.w	&1022,%d6	#exponent 1 bigger because of leading one
	and.w	&0x000f,%d0	#d0 has top 4 bits
#
#  Check for boundary conditions.
#
	cmp.w	%d6,&32
	bge.w	intov1		#definitely too large
	tst.w	%d6		#for small numbers
	bgt.w	in32cont	#branch if will convert
	moveq	&0,%d0		#else return 0
	rts
#
#  Place top bits (except for hidden bit) all in d1.
#
in32cont: lsr.l	&5,%d1
	ror.l	&5,%d0
	or.l	%d0,%d1		#correct except for the hidden bit
#
#  Finish the conversion.
#
	neg.w	%d6
	add.w	&32,%d6		#1 <= shifts <= 31
	bset	&31,%d1		#place in hidden bit
	lsr.l	%d6,%d1
	tst.w	%d7		#check the sign
	bpl.w	done32
	neg.l	%d1		#else convert to negative
done32:	move.l	%d1,%d0		#place result in correct register
	rts
#
#  Had overflow, return correct sign.
#
intov1:	tst.w	%d7		#check the sign
	bpl.w	retpos		#return positive result
	move.l	&0x80000000,%d0	#return correct result
	rts
retpos:	move.l	&0x7fffffff,%d0
	rts
#
#  Coefficients for the polynomial evaluation.
#
cff_powp: 
	long	0x3f3c78fd,0xdb4afc28		#0.4344577567216311964 e -03
	long	0x3f624924,0x2e278dac		#0.2232142128592425897 e -02
	long	0x3f899999,0x999e080e		#0.1250000000050379917 e -01
	long	0x3fb55555,0x5555554d		#0.8333333333333321141 e -01
#
cff_powq: 
	long	0x3eef4edd,0xe392cc80		#0.1492885268059560819 e -04
	long	0x3f242f7a,0xe0384c74		#0.1540029044098976460 e -03
	long	0x3f55d87e,0x18d7cd9f		#0.1333354131358578470 e -02
	long	0x3f83b2ab,0x6e131d98		#0.9618129059517241696 e -02
	long	0x3fac6b08,0xd703026d		#0.5550410866408559533 e -01
	long	0x3fcebfbd,0xff82c4ce		#0.2402265069590953706 e +00
	long	0x3fe62e42,0xfefa39ef		#0.6931471805599452963 e +00
#
tb_a1:	long	0x00000000,0x00000000		#dummy entry for indexing
	long	0x3ff00000,0x00000000,0x3feea4af,0xa2a490da
	long	0x3fed5818,0xdcfba487,0x3fec199b,0xdd85529c
	long	0x3feae89f,0x995ad3ad,0x3fe9c491,0x82a3f090
	long	0x3fe8ace5,0x422aa0db,0x3fe7a114,0x73eb0187
	long	0x3fe6a09e,0x667f3bcd,0x3fe5ab07,0xdd485429
	long	0x3fe4bfda,0xd5362a27,0x3fe3dea6,0x4c123422
	long	0x3fe306fe,0x0a31b715,0x3fe2387a,0x6e756238
	long	0x3fe172b8,0x3c7d517b,0x3fe0b558,0x6cf9890f
	long	0x3fe00000,0x00000000
#
tb_a2:	long	0x00000000,0x00000000		#dummy entry for indexing
	long	0xbc7e9c23,0x179c0000,0x3c611065,0x89500000
	long	0x3c5c7c46,0xb0700000,0xbc641577,0xee040000
	long	0x3c76324c,0x05460000,0x3c6ada09,0x11f00000
	long	0x3c79b07e,0xb6c80000,0x3c78a62e,0x4adc0000

#**********************************************************
#							  *
# DOUBLE PRECISION X**Y					  *
#							  *
# double pow(x,y) double x,y;				  *
#							  *
# Errors: matherr() is called with type = DOMAIN and	  *
#		retval = 0.0 if: 1) x = 0.0 and y <= 0.0, *
#		or 2) x < 0.0 and y is not an integer	  *
#	  Overflow will invoke matherr with type = 	  *
#		OVERFLOW and retval = HUGE		  *
#	  Underflow will invoke matherr with type = 	  *
#		UNDERFLOW and retval = 0.0		  *
#	  Inexact errors are ignored.			  *
#							  *
# Author: Mark McDowell  6/10/85			  *
#							  *
#**********************************************************

	text
c_pow:
	link	%a6,&-10
	lea	(8,%a6),%a0
	lea	(16,%a6),%a1
	clr.w	-2(%a6)			#assume positive result
	fmov.l	%fpcr,%d0		#save control register
	movq	&0,%d1
	fmov.l	%d1,%fpcr		#ext precision
	fmov.d	(%a1),%fp1		#%fp1 <- y
	fmov.d	(%a0),%fp0		#%fp0 <- x
	fbneq	c_pow2
	ftest	%fp1
	fbeq	c_pow1			#0.0**0.0
	fbngt	c_pow5			#0.0**y with y < 0.0
	fmov.l	%d0,%fpcr		#restore control register
	movq	&0,%d0			#0.0**y = 0.0
	movq	&0,%d1
	unlk	%a6
	rts	
c_pow1:
	mov.l	&0x3FF00000,%d0		#0.0**0.0 = 1.0
	movq	&0,%d1
	unlk	%a6
	rts
c_pow2:
	fbnlt	c_pow3			#is x < 0.0?
	fmovm.x %fp3,-(%sp)
	fint	%fp1,%fp3
	fcmp	%fp3,%fp1
	fbne	c_pow4a			#x**y with x < 0.0 and y not an integer
	fmovm.x %fp2,-(%sp)
	fscale.w &-1,%fp3
	fint	%fp3,%fp2
	fcmp	%fp3,%fp2
	fsne	-2(%a6)			#set if y is odd
	fabs	%fp0
	fmovm.x (%sp)+,%fp2-%fp3
c_pow3:	
	flog10	%fp0			#log10(x)
	fmul	%fp1,%fp0		#y * log10(x)
	ftentox	%fp0			#10 ** (y * log10(x))
	fmov.l	%fpsr,%d1
	andi.w	&0x1800,%d1		#overflow or underflow?
	bne.w	c_pow7
	tst.w	-2(%a6)			#change sign?
	beq.b	c_pow4
	fneg	%fp0
c_pow4:
	fmov.d	%fp0,(%sp)		#get result
	fmov.l	%fpsr,%d1
	andi.w	&0x1800,%d1		#overflow or underflow?
	bne.w	c_pow7
	fmov.l	%d0,%fpcr		#restore control register
	mov.l	(%sp)+,%d0		#move result
	mov.l	(%sp)+,%d1
	unlk 	%a6
	rts
c_pow4a:
	fmovm.x	(%sp)+,%fp3
	fmov.l	%d0,%fpcr		#restore control register
	movq	&0,%d0
	mov.l	%d0,(%sp)		#retval = 0.0
	mov.l	%d0,-(%sp)
	bra	c_pow5a
c_pow5:
	fmov.l	%d0,%fpcr		#restore control register
	movq	&-1,%d0
	mov.l	%d0,(%sp)		#retval = -HUGE_VAL
	mov.l	&0xFFEFFFFF,-(%sp)
c_pow5a:
	movm.l	(%a0),%d0-%d1
	movm.l	(%a1),%a0-%a1
	movm.l	%d0-%d1/%a0-%a1,-(%sp)
	pea	c_pow_name
	movq	&DOMAIN,%d0
	mov.l	%d0,-(%sp)		#type = DOMAIN
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_pow6
	movq	&EDOM,%d0
	mov.l	%d0,_errno		#errno = EDOM
	pea	18.w
	pea	c_pow_msg
	pea	2.w
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__write',`			#write(2,"pow: DOMAIN error\n",18)
	jsr	_write			#write(2,"pow: DOMAIN error\n",18)
 ')
	lea	(12,%sp),%sp
c_pow6:	
	mov.l	(%sp)+,%d0		#get retval
	mov.l	(%sp)+,%d1
	unlk	%a6
	rts
c_pow7:
	fmov.l	%d0,%fpcr		#restore control register
	and.w	&0x1000,%d1		#overflow or underflow?
	bne.b	c_pow8			#jump if overflow
	movq	&UNDERFLOW,%d0
	mov.l	%d0,(%sp)		#type = UNDERFLOW
	movq	&0,%d0			#retval = 0.0
	mov.l	%d0,-(%sp)
	mov.l	%d0,-(%sp)
	bra.b	c_pow9
c_pow8:
	movq	&OVERFLOW,%d0
	mov.l	%d0,(%sp)		#type = OVERFLOW
	movq	&-1,%d0			#retval = HUGE
	mov.l	%d0,-(%sp)
	mov.l	&0x7FEFFFFF,-(%sp)
	tst.w	-2(%a6)
	beq.b	c_pow9
	bchg	&7,(%sp)
c_pow9:
	movm.l	(%a0),%d0-%d1
	movm.l	(%a1),%a0-%a1
	movm.l	%d0-%d1/%a0-%a1,-(%sp)
	pea	c_pow_name		#name = "pow"
	mov.l	-10(%a6),-(%sp)	#get type
	pea	(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	jsr	__matherr',`		#matherr(&excpt)
	jsr	_matherr		#matherr(&excpt)
 ')
	lea	(28,%sp),%sp
	tst.l	%d0
	bne.b	c_pow10
	movq	&ERANGE,%d0
	mov.l	%d0,_errno		#errno = ERANGE
c_pow10:	
	mov.l	(%sp)+,%d0		#get retval
	mov.l	(%sp)+,%d1
	unlk	%a6
	rts

	data
c_pow_name:
	byte	112,111,119,0			#"pow"
c_pow_msg:
	byte	112,111,119,58,32,68,79,77	#"pow: DOMAIN error\n"
	byte	65,73,78,32,101,114,114,111
	byte	114,10,0

ifdef(`PROFILE',`
	data
p_pow:	long	0
	')

