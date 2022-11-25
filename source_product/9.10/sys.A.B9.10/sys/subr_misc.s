 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/subr_misc.s,v $
 # $Revision: 1.6.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:21:25 $
 # HPUX_ID: @(#)subr_misc.s	52.2		88/04/28

 #(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
 #(c) Copyright 1979 The Regents of the University of Colorado,a body corporate 
 #(c) Copyright 1979, 1980, 1983 The Regents of the University of California
 #(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
 #The contents of this software are proprietary and confidential to the Hewlett-
 #Packard Company, and are limited in distribution to those with a direct need
 #to know.  Individuals having access to this software are responsible for main-
 #taining the confidentiality of the content and for keeping the software secure
 #when not in use.  Transfer to any party is strictly forbidden other than as
 #expressly permitted in writing by Hewlett-Packard Company. Unauthorized trans-
 #fer to or possession by any unauthorized party may be a criminal offense.
 #
 #                    RESTRICTED RIGHTS LEGEND
 #
 #          Use,  duplication,  or disclosure by the Government  is
 #          subject to restrictions as set forth in subdivision (b)
 #          (3)  (ii)  of the Rights in Technical Data and Computer
 #          Software clause at 52.227-7013.
 #
 #                     HEWLETT-PACKARD COMPANY
 #                        3000 Hanover St.
 #                      Palo Alto, CA  94304

#define LOCORE	0
#include "../mach.300/cpu.h"

 # 
 # The one and only high speed version of bzero.  If you
 # can improve this fine, but do it for everyone please!
 #
#ifdef GPROF
 ###############################################################
 #	hack to fix the problem of the C compiler putting an "_"
 #	for the mcount routine on subr_mcnt.c
 ################################################################
	global	mcount
mcount:
	jmp	_mcount
	nop				#nop's here because of gprof
	nop				#  hash bucket collision
	global bzero_precode		# for profiling only
bzero_precode:
#endif /* GPROF */
 ### set	M68040,3		# for testing in userland

 # NOTE:  All timings are for an 9000/380 - 25Mhz 68040 SPU.

 #  1  =  1 clock  ( 40 Nanoseconds)
 #  2  =  2 clocks ( 80 Nanoseconds)
 #  3  =  3 clocks (120 Nanoseconds)
 # 2/3 =  2 clocks if branch taken, 3 clocks if not
 # 2-6 =  2 clocks if no memory reference, 6 clock average with none in cache
 # 1-4 =  1 clocks if no memory reference, 4 clock average with cache misses
 #(21-66) = loop times in clocks for best and worst case.

 # Switch table for clearing 0..36 bytes

Lz35:	clr.l	(%a0)+			#1-4
Lz31:	clr.l	(%a0)+
Lz27:	clr.l	(%a0)+
Lz23:	clr.l	(%a0)+
Lz19:	clr.l	(%a0)+
Lz15:	clr.l	(%a0)+
Lz11:	clr.l	(%a0)+
Lz7:	clr.l	(%a0)+
Lz3:	clr.w	(%a0)+
	clr.b	(%a0)
	rts

Lz33:	clr.l	(%a0)+
Lz29:	clr.l	(%a0)+
Lz25:	clr.l	(%a0)+
Lz21:	clr.l	(%a0)+
Lz17:	clr.l	(%a0)+
Lz13:	clr.l	(%a0)+
Lz9:	clr.l	(%a0)+
Lz5:	clr.l	(%a0)+
Lz1:	clr.b	(%a0)
Lzswitch:
	rts
	bra.b	Lz1
	bra.b	Lz2
	bra.b	Lz3
	bra.b	Lz4
	bra.b	Lz5
	bra.b	Lz6
	bra.b	Lz7
	bra.b	Lz8
	bra.b	Lz9
	bra.b	Lz10
	bra.b	Lz11
	bra.b	Lz12
	bra.b	Lz13
	bra.b	Lz14
	bra.b	Lz15
	bra.b	Lz16
	bra.b	Lz17
	bra.b	Lz18
	bra.b	Lz19
	bra.b	Lz20
	bra.b	Lz21
	bra.b	Lz22
	bra.b	Lz23
	bra.b	Lz24
	bra.b	Lz25
	bra.b	Lz26
	bra.b	Lz27
	bra.b	Lz28
	bra.b	Lz29
	bra.b	Lz30
	bra.b	Lz31
	bra.b	Lz32
	bra.b	Lz33
	bra.b	Lz34
	bra.b	Lz35
Lz36:	clr.l	(%a0)+			#1-4
Lz32:	clr.l	(%a0)+
Lz28:	clr.l	(%a0)+
Lz24:	clr.l	(%a0)+
Lz20:	clr.l	(%a0)+
Lz16:	clr.l	(%a0)+
Lz12:	clr.l	(%a0)+
Lz8:	clr.l	(%a0)+
Lz4:	clr.l	(%a0)
	rts

Lz34:	clr.l	(%a0)+
Lz30:	clr.l	(%a0)+
Lz26:	clr.l	(%a0)+
Lz22:	clr.l	(%a0)+
Lz18:	clr.l	(%a0)+
Lz14:	clr.l	(%a0)+
Lz10:	clr.l	(%a0)+
Lz6:	clr.l	(%a0)+
Lz2:	clr.w	(%a0)+
	rts

	global _blkclr,_bzero	#blkclr(address, num_bytes)
_bzero:
_blkclr:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	mov.l	4(%sp),%a0		# 1  a0 = dest
	mov.l	8(%sp),%d0		# 1  d0 = count
	subi.l	&32+4,%d0		# 1  check if > threshold
	bhi.b	Lbzero_4b_align		#2/3 yes, clear large block

	jmp	Lzswitch+36+36(%pc,%d0.w*2) # 7  clear small block

 # 37 or greater clear, 4 byte align first

Lbzero_4b_align:
	mov.l	%a0,%d1			# 1  get least 2 dest bits
	andi.l	&3,%d1			# 1  mask to misalign bits
	clr.l	(%a0)+			#1-4 copy over four bytes
	add.l	%d1,%d0			# 1  decrement correct amount
	sub.l	%d1,%a0			# 1  make 4 byte aligned

 # 32 byte block clear loop

Lbzero_clear:
	movq.l	&32,%d1			# 1  loop size (for 68030)
Lbzero_clr_loop:
	clr.l	(%a0)+			#(11-35) clear 32 bytes
	clr.l	(%a0)+			# 13 - 43 Nanosec per byte
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	sub.l	%d1,%d0			# 1  check if another loop
	bcc.b   Lbzero_clr_loop		#2/3

	jmp	Lzswitch+32+32(%pc,%d0.w*2) # 7  clear small block

#ifdef GPROF
	global bcopy_precode		# for profiling only
bcopy_precode:
#endif
 #
 # bcopy(src, dest, count);  /* allows overlapping copys */
 #
 # Copy memory from src to dest for count bytes.
 #
        global _iobcopy                 # iobcopy(from, to, size)
	global _bcopy			# bcopy(from, to, size)
	global _overlap_bcopy		# overlap_bcopy(from, to, size)
	global _ovbcopy			# ovbcopy(from, to, size)

 # move the least bytes of the count (backward copy)

Lr32:	mov.l	-(%a0),-(%a1)
Lr28:	mov.l	-(%a0),-(%a1)
Lr24:	mov.l	-(%a0),-(%a1)
Lr20:	mov.l	-(%a0),-(%a1)
Lr16:	mov.l	-(%a0),-(%a1)
Lr12:	mov.l	-(%a0),-(%a1)
Lr8:	mov.l	-(%a0),-(%a1)
Lr4:	mov.l	-(%a0),-(%a1)
	rts

Lr33:	mov.l	-(%a0),-(%a1)
Lr29:	mov.l	-(%a0),-(%a1)
Lr25:	mov.l	-(%a0),-(%a1)
Lr21:	mov.l	-(%a0),-(%a1)
Lr17:	mov.l	-(%a0),-(%a1)
Lr13:	mov.l	-(%a0),-(%a1)
Lr9:	mov.l	-(%a0),-(%a1)
Lr5:	mov.l	-(%a0),-(%a1)
Lr1:	mov.b	-(%a0),-(%a1)
	rts

Lr34:	mov.l	-(%a0),-(%a1)
Lr30:	mov.l	-(%a0),-(%a1)
Lr26:	mov.l	-(%a0),-(%a1)
Lr22:	mov.l	-(%a0),-(%a1)
Lr18:	mov.l	-(%a0),-(%a1)
Lr14:	mov.l	-(%a0),-(%a1)
Lr10:	mov.l	-(%a0),-(%a1)
Lr6:	mov.l	-(%a0),-(%a1)
Lr2:	mov.w	-(%a0),-(%a1)

Lswitch_table_rev:
	rts
	bra.b	Lr1
	bra.b	Lr2
	bra.b	Lr3
	bra.b	Lr4
	bra.b	Lr5
	bra.b	Lr6
	bra.b	Lr7
	bra.b	Lr8
	bra.b	Lr9
	bra.b	Lr10
	bra.b	Lr11
	bra.b	Lr12
	bra.b	Lr13
	bra.b	Lr14
	bra.b	Lr15
	bra.b	Lr16
	bra.b	Lr17
	bra.b	Lr18
	bra.b	Lr19
	bra.b	Lr20
	bra.b	Lr21
	bra.b	Lr22
	bra.b	Lr23
	bra.b	Lr24
	bra.b	Lr25
	bra.b	Lr26
	bra.b	Lr27
	bra.b	Lr28
	bra.b	Lr29
	bra.b	Lr30
	bra.b	Lr31
	bra.b	Lr32
	bra.b	Lr33
	bra.b	Lr34
Lr35:	mov.l	-(%a0),-(%a1)
Lr31:	mov.l	-(%a0),-(%a1)
Lr27:	mov.l	-(%a0),-(%a1)
Lr23:	mov.l	-(%a0),-(%a1)
Lr19:	mov.l	-(%a0),-(%a1)
Lr15:	mov.l	-(%a0),-(%a1)
Lr11:	mov.l	-(%a0),-(%a1)
Lr7:	mov.l	-(%a0),-(%a1)
Lr3:	mov.w	-(%a0),-(%a1)
	mov.b	-(%a0),-(%a1)
	rts

 # 4 byte align and backward move with unrolled mov.l loop 32 bytes at a time

Lbcopy_4b_align_rev:
 	cmpi.l	%d0,&4			# 1 check at least < 4 bytes apart
 	bcc.b	Lrbcopy_loop_rev0	#2/3 no, do alignment
	addq.l	&4,%d1			# 1  yes, restore count
 	bra.b	Lrbcopy_loop_rev1	#2/3 and skip alignment

Lrbcopy_loop_rev0:
	mov.l	%a1,%d0			# 1  get least 2 bits of dest address
	neg.l	%d0			# 1  negate because of reverse move
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	-(%a0),-(%a1)		#2-6 copy over four bytes
	add.l	%d0,%d1			# 1  adjust count to alignment
	add.l	%d0,%a1			# 1  decrement dest to alignment
	add.l	%d0,%a0			# 1  decrement src correct amount

Lrbcopy_loop_rev1:
	movq.l	&32,%d0			# 1  loop size counter

 # 64 byte block move loop

Lrbcopy_loop_rev:
	mov.l	-(%a0),-(%a1)		#(23-51) move 32 bytes
	mov.l	-(%a0),-(%a1)		# 24 - 62 Nanosec per byte
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	mov.l	-(%a0),-(%a1)
	sub.l	%d0,%d1			# 1  check if another loop
	bcc.b	Lrbcopy_loop_rev	#2/3 loop if more bytes

	jmp	Lswitch_table_rev+32+32(%pc,%d1.w*2) # 7  move small block

 # Here we know dst > src and buffers overlap, so check if overlap is < 4 bytes

Lbackward:
	adda.l	%d1,%a1			# 1  dst_a1 = dst + count
	subi.l	&32+4,%d1		# 1  check if >= threshold
 	bcc.b	Lbcopy_4b_align_rev	#2/3 yes, move large block

	jmp	Lswitch_table_rev+36+36(%pc,%d1.w*2) # 7  move small block

 # move the least bytes of the count (forward copy)

Lf32:	mov.l	(%a0)+,(%a1)+
Lf28:	mov.l	(%a0)+,(%a1)+
Lf24:	mov.l	(%a0)+,(%a1)+
Lf20:	mov.l	(%a0)+,(%a1)+
Lf16:	mov.l	(%a0)+,(%a1)+
Lf12:	mov.l	(%a0)+,(%a1)+
Lf8:	mov.l	(%a0)+,(%a1)+
Lf4:	mov.l	(%a0),(%a1)
	rts

Lf33:	mov.l	(%a0)+,(%a1)+
Lf29:	mov.l	(%a0)+,(%a1)+
Lf25:	mov.l	(%a0)+,(%a1)+
Lf21:	mov.l	(%a0)+,(%a1)+
Lf17:	mov.l	(%a0)+,(%a1)+
Lf13:	mov.l	(%a0)+,(%a1)+
Lf9:	mov.l	(%a0)+,(%a1)+
Lf5:	mov.l	(%a0)+,(%a1)+
Lf1:	mov.b	(%a0),(%a1)
	rts

Lf34:	mov.l	(%a0)+,(%a1)+
Lf30:	mov.l	(%a0)+,(%a1)+
Lf26:	mov.l	(%a0)+,(%a1)+
Lf22:	mov.l	(%a0)+,(%a1)+
Lf18:	mov.l	(%a0)+,(%a1)+
Lf14:	mov.l	(%a0)+,(%a1)+
Lf10:	mov.l	(%a0)+,(%a1)+
Lf6:	mov.l	(%a0)+,(%a1)+
Lf2:	mov.w	(%a0),(%a1)

Lswitch_table_fwd:
	rts
	bra.b	Lf1
	bra.b	Lf2
	bra.b	Lf3
	bra.b	Lf4
	bra.b	Lf5
	bra.b	Lf6
	bra.b	Lf7
	bra.b	Lf8
	bra.b	Lf9
	bra.b	Lf10
	bra.b	Lf11
	bra.b	Lf12
	bra.b	Lf13
	bra.b	Lf14
	bra.b	Lf15
	bra.b	Lf16
	bra.b	Lf17
	bra.b	Lf18
	bra.b	Lf19
	bra.b	Lf20
	bra.b	Lf21
	bra.b	Lf22
	bra.b	Lf23
	bra.b	Lf24
	bra.b	Lf25
	bra.b	Lf26
	bra.b	Lf27
	bra.b	Lf28
	bra.b	Lf29
	bra.b	Lf30
	bra.b	Lf31
	bra.b	Lf32
	bra.b	Lf33
	bra.b	Lf34
Lf35:	mov.l	(%a0)+,(%a1)+
Lf31:	mov.l	(%a0)+,(%a1)+
Lf27:	mov.l	(%a0)+,(%a1)+
Lf23:	mov.l	(%a0)+,(%a1)+
Lf19:	mov.l	(%a0)+,(%a1)+
Lf15:	mov.l	(%a0)+,(%a1)+
Lf11:	mov.l	(%a0)+,(%a1)+
Lf7:	mov.l	(%a0)+,(%a1)+
Lf3:	mov.w	(%a0)+,(%a1)+
	mov.b	(%a0),(%a1)
	rts

_bcopy:					# bcopy(from, to, size)
_overlap_bcopy:				# overlap bcopy(from, to, size)
_ovbcopy:				# overlap bcopy(from, to, size)
_iobcopy:				# non-overlap, non move16 instr
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	mov.l	4(%sp),%a0		# 1  a0 = src (s1)
	mov.l	8(%sp),%a1		# 1  a1 = dst (s2)
	mov.l	12(%sp),%d1		# 1  d1 = count

 #	if (dst_a1 > src_a0) backwards copy may be necessary

	mov.l	%a1,%d0			# 1  d0 = dst
	sub.l	%a0,%d0			# 1  d0 = dst - src
	bls.b	Lsrc_ge_dst		#2/3 forward move, dst <= src

	adda.l	%d1,%a0			# 1  src_a0 = src + count
	cmp.l	%a0,%a1			# 1  check src+count > dst
	bhi.w	Lbackward		#2/3 yes, overlap, MUST backward copy

	suba.l	%d1,%a0			# 1  restore src_a0 = src
	bra.b	Lforward		# 2  forward move

Lsrc_ge_dst:				#    src >= dst (d0 zero or neg)
	beq.b	Lcexit			#2/3 src == dst, no moving to do

Lforward:
	subi.l	&32+4,%d1		# 1  check if > threshold
	bcc.b	Lfbcopy_ge_36		#2/3 yes, copy medium block

	jmp	Lswitch_table_fwd+36+36(%pc,%d1.w*2) # 7  move small block
Lcexit:
	rts


 # 4 byte align and move with unrolled mov.l loop 32 bytes at a time

Lfbcopy_ge_36:
 	cmpi.l	%d0,&-4			# 1  check if move < 4 bytes apart
 	bls.b	Lbfcopy_loop_fwd0	#2/3 no, do alignment
	addq.l	&4,%d1			# 1  yes, restore count
 	bra.b	Lbfcopy_loop_fwd1	#2/3 and skip alignment

Lbfcopy_loop_fwd0:
	mov.l	%a1,%d0			# 1  d0 = dst
	andi.l	&3,%d0			# 1  mask to misalign bits
	mov.l	(%a0)+,(%a1)+		#2-6 copy over four bytes
	add.l	%d0,%d1			# 1  adjust count for alignment
	sub.l	%d0,%a1			# 1  decrement to alignment
	sub.l	%d0,%a0			# 1  decrement correct amount

Lbfcopy_loop_fwd1:
	movq.l	&32,%d0			# 1  loop size counter

 # 32 byte block move loop

Lbfcopy_loop_fwd:
	mov.l	(%a0)+,(%a1)+		#(23-51) move 32 bytes
	mov.l	(%a0)+,(%a1)+		# 24 - 62 Nanosec per byte
	mov.l	(%a0)+,(%a1)+		#1-6
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	sub.l	%d0,%d1			# 1  check if another loop
	bcc.b	Lbfcopy_loop_fwd	#2/3 loop if more bytes

	jmp	Lswitch_table_fwd+32+32(%pc,%d1.w*2) # 10 move small block

	global _pg_zero			# pg_zero(address); zero 4096 bytes
	global _pg_zero4096		# pg_zero4096(address); zero 4096 bytes
_pg_zero:
_pg_zero4096:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq	&16,%d0			# 1  loop size for 4096 bytes
	bra.b	Lpg_zero		# 2

	global _pg_zero512			# pg_zero(address); zero 4096 bytes
_pg_zero512:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq	&2,%d0			# 1  loop size for 512 bytes
	bra.b	Lpg_zero		# 2

	global _pg_zero256			# pg_zero(address); zero 4096 bytes
_pg_zero256:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq	&1,%d0			# 1  loop size for 256 bytes

 # 64 byte block clear loop and check if 68040 processor

Lpg_zero:
	mov.l	4(%sp),%a0		# 1  a0 = dest
	cmpi.l	_processor,&M68040	# 1  check if move16 opportunity
	blt.b	Lpgzero_clr_entry	#2/3 no, use clr.l 

Lpgzero_040:				#2/3 use mov16 to clear memory
	nop				# ?  resolve pending writebacks
Lpgzero_mov16_loop:
	tst.l	_zeroed_cache_ln	# 1  into the cache in case interrupt
 ###### move16	_zeroed_cache_ln,(%a0)+ #(132-132) move16 _zeroed_cache_line,(%a0)+
 ###### move16  _zeroed_cache_ln,(%a0)+ # 20.7 Nanosec per byte
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 ###### move16  _zeroed_cache_ln,(%a0)+ # 8
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
 	short	0xF608
 	long	_zeroed_cache_ln
	subq.l	&1,%d0			# 1   another loop?
	bne.b	Lpgzero_mov16_loop	#2/3 check if another 256 bytes?
	rts

Lpgzero_clr_entry:
	lsl.l	&2,%d0			# 2  mpy by 4
	subq.l	&1,%d0			# 1  predecrement for dbra
Lpgzero_clr_loop:
	clr.l	(%a0)+			#(11-35) clear 32 bytes
	clr.l	(%a0)+			# 13 - 43 Nanosec per byte
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	clr.l	(%a0)+
	dbra	%d0,Lpgzero_clr_loop	#2/3 check if another 64 bytes?
	rts

	data
	align	gap_pgzero,16
_zeroed_cache_ln:
	space	16		# 16 byte, zero filled, cache line aligned
	text

	global _pg_copy			# pg_copy(from, to); copy 4096 bytes
	global _pg_copy4096		# pg_copy(from, to); copy 4096 bytes
_pg_copy:
_pg_copy4096:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq	&63,%d0			# 1  loop size -1
	bra.b	Lpg_copyit		# 2  continue copy

	global _pg_copy512		# pg_copy512(from, to); copy 512 bytes
_pg_copy512:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq	&7,%d0			# 1  loop size -1
	bra.b	Lpg_copyit		# 2  continue copy

	global _pg_copy256		# pg_copy256(from, to); copy 256 bytes
_pg_copy256:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	movq.l	&3,%d0			# 1  loop size -1
Lpg_copyit:
	mov.l	4(%sp),%a0		# 1  a0 = src (s1)
	mov.l	8(%sp),%a1		# 1  a1 = dst (s2)
	cmpi.l	_processor,&M68040	# 1  check if move16 opportunity
	bge.b	Lpgcopy_040_loop	#2/3 68040, move16 copy

 # 64 byte block move loop

Lpgcopy_loop:
	mov.l	(%a0)+,(%a1)+		#(23-51) move 32 bytes
	mov.l	(%a0)+,(%a1)+		# 24 - 62 Nanosec per byte
	mov.l	(%a0)+,(%a1)+		#1-6
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	mov.l	(%a0)+,(%a1)+
	dbra	%d0,Lpgcopy_loop	#2/3 check if another 64 bytes?
	rts

 # 68040, forward move,  src and dest can be 16 byte aligned
 # 16 byte align before move16 in a move 16 loop

Lpgcopy_040_loop:			# a1 = dest, a0 = src

 # 64 byte block move loop
	nop				# ?  resolve pending writebacks
Lpgcopy_move16_loop:  			#(22-36) move16 (%a0)+,(%a1)+
 ######	move16 (%a0)+,(%a1)+		# 27 - 45 Nanosec per byte
 ######	move16 (%a0)+,(%a1)+		# 18  
 ######	move16 (%a0)+,(%a1)+		# 18  
 ######	move16 (%a0)+,(%a1)+		# 18  
	long	0xF6209000
	long	0xF6209000
	long	0xF6209000
	long	0xF6209000
	dbra	%d0,Lpgcopy_move16_loop	# 0 check if another 64 bytes?
	rts

#ifndef	SEMAPHORE_DEBUG
 # Get the beta class lock and wait if already locked.

	global	_b_psema,_b_sema_sleep
_b_psema:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	mov.l	4(%sp),%a0		#  2 get b_sema_t struct pointer
	bset	&0,(%a0)		#  3 check if locked and acquire
	bne.b	_b_psema0		#2/3 already locked.
	rts				#    got lock, return

_b_psema0:
	jmp	_b_sema_sleep		#    go wait for lock release 
#ifdef GPROF
	nop				#nop's here because of gprof
	nop				#  hash bucket collision
#endif

 # Release the beta class lock and schedule anybody waiting for lock

	global	_b_vsema,_b_sema_wanted
_b_vsema:
#ifdef GPROF_MCOUNT
 	jsr	mcount
#endif
	mov.l	4(%sp),%a0		#  2 get b_sema_t struct pointer
	btst	&1,(%a0)		#  1 check if semaphone has
	bne.b	_b_vsema0		#2/3 anybody waiting.
	clr.b	(%a0)			#  2 no, release semiphore
	rts				#    nobody, locked released, return

_b_vsema0:
	jmp	_b_sema_wanted		#    go schedule any waiters
#ifdef GPROF
	nop				#nop's here because of gprof
	nop				#  hash bucket collision
#endif

 # Get the beta class lock(ret 1) and dont wait(ret 0) if already locked.

	global	_b_cpsema
_b_cpsema:
#ifdef GPROF_MCOUNT
	jsr	mcount
#endif
	mov.l	4(%sp),%a0		#  2 get b_sema_t struct pointer
	bset	&0,(%a0)		#  3 check if locked and acquire
	bne.b	_b_cpsema0		#2/3 already locked.
	movq	&1,%d0			#  1 got lock, return 1
	rts				#    got lock, return

_b_cpsema0:
	movq	&0,%d0			#  1 did not get lock, return 0
	rts				#    return
#ifdef GPROF
	nop				#nop's here because of gprof
	nop				#  hash bucket collision
#endif
#endif	SEMA_DEBUG

#ifdef	BCOPY_VALIDATE_AND_TEST
/* Command to time and validate bcopy() (dlb) */

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <sys/utsname.h>


/* number of default passed to get min value */
#define ITERATIONS	8

/* max iterations to time */
#define MAX_ITER	8192

#define Min(a, b)	(((a) < (b))?(a):(b))

#define	MAXSIZE		(2*1024*1024+37)
/* verious sizes of arrays to validate and time */
int sizes[] = {
	 0,  1,  2,  3,  4,  5,  6,  7 , 8,  9, 
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
	70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
	90, 91, 92, 93, 94, 95, 96, 97, 98, 128,
	256, 512, 1023, 1024, 2048, 4096, 8192, 16384, 32768, 
	1024 * 1024 -1, 1024 *1024, 1024 *1024 +1,
	2 * 1024 * 1024 -1, 2 * 1024 *1024, MAXSIZE
};

/* Test for the 7 cases */
#define NCASES		7
char *cases[NCASES] = {
	"Case 0: dest < source, non-overlap",
	"Case 1: dest > source, non-overlap",
	"Case 2: dest < source, non-overlap > 2Gb",
	"Case 3: dest > source, non-overlap > 2Gb",
	"Case 4: dest < source, with overlap",
	"Case 5: dest > source, with overlap",
	"Case 6: dest <> source, with overlap < 4 bytes",
};

/* number of numbers in sizes[] */
u_int nsizes =  sizeof(sizes) / sizeof(int); 

#define	NALIGN_src	sizeof(int)
#define	NALIGN_dst	sizeof(int)

/* slop bytes on each end of test arrays */
#define	slopbytes	16

/* Macro for getting 16 byte aligned memory for tests */
#define MALLOC16(X) ((malloc((X) + 15) + 15) & ~0x0f)

/* max number of times to remember */
double tim[NALIGN_src][NALIGN_dst][sizeof(sizes)/sizeof(int)];

struct utsname name;

int	dflag	= 0;		/* Turn on debug flag */
int	lflag	= 0;		/* Turn on list flag */
int	fflag	= 0;		/* Print all errors */
int	tflag	= 0;		/* Set timeing flag */
int	vflag	= 1;		/* validation flag */
int	bflag	= 0;		/* use Binary sizes */
int	iter	= ITERATIONS;	/* default interations */
int	processor = 0;		/* processor type     */
u_int	maxsz	= -1;		/* max size to test   */
int	Vflag	= 0;		/* One value only */
int	Case    = -1;		/* default cases  */

#define	PRESET		8192 	/* size of random array of chars on stack */
char ran_numb[PRESET];	/* array of random chars */

double time_bcopy();

usage(pname)
{
	fprintf(stderr, 
"Usage: %s [-vtfdb][-V 1value][-i iterations][-C ase #][-m maxsize] [messages]\n", pname);

	fprintf(stderr, 
"Options...     Description\n\n");
	fprintf(stderr, 
"-v             Turn OFF validation - default on\n");
	fprintf(stderr, 
"-t             Turn ON Timing - default off\n");
	fprintf(stderr, 
"-f             Dont quit on first validation error\n");
	fprintf(stderr, 
"-d             Turn on debug\n");
	fprintf(stderr, 
"-b             Turn on binary sizes flag\n");
	fprintf(stderr, 
"-V <value>     Validate and/or Time only <value> number of bytes\n");
	fprintf(stderr, 
"-i <iterations> Do <iterations> for more/less accurate times - default = 8\n");
	fprintf(stderr, 
"-C <case numb> Do just <case numb> for validation and/or timing - default = all Cases\n");
	fprintf(stderr, 
"-m <maxsize>   Validate and/or Time to <maxsize> bytes - default = 2097189\n");
	fprintf(stderr, 
"[mesages]      Optional message to stdout\n");
	fprintf(stderr, "\n");
	exit(2);
}

main(argc, argv)
int	 argc;
char	*argv[];
{
    extern char	*optarg;
    extern int 	optind;

    char	*s1_test,	*s2_test;
    char	*s1_cntl,	*s2_cntl;
    char 	*test_ray,	*cntl_ray;
    register 	jj;
    int		size;
    int		ii, kk;
    int		s_off, d_off;
    double	dft;
    int		icase, ncases = NCASES;
    u_int	asize;
    u_int	maxsize = MAXSIZE;
    char	stk_cntl[(PRESET + slopbytes)*2];
    char	stk_test[(PRESET + slopbytes)*2 + 16];

    /* do not do output buffering in case of core dump */
    setbuf(stdout, 0);

    /* get optional parameters */
    while ((ii = getopt(argc, argv, "dlftvbi:m:p:V:C:")) != EOF) 
    {
	switch (ii) 
	{
	case 'd':	dflag++;		break;	/* Turn on debug      */
	case 'l':	lflag++;		break;	/* Turn on list       */
	case 'f':	fflag++;		break;	/* Print all errors   */
	case 't':	tflag++;		break;  /* Set timeing        */
	case 'v':	vflag = 0;		break;  /* Clear validate     */
	case 'b':	bflag++;		break;  /* Binary sizes flag  */
	case 'i':	iter = atoi(optarg);	break;	/* number iterations  */
	case 'm':	maxsz = atoi(optarg);	break;	/* max size to test   */
	case 'p':	processor = atoi(optarg);break;	/* processor type     */
	case 'V':	maxsz  = atoi(optarg); Vflag++; break;/* one value only */
	case 'C':	Case = atoi(optarg); 	break;	/* start case number  */
	default:	usage(argv[0]); exit(2);
	}
    }
    /* if default validation only, then cap max test size */
    if (vflag && !tflag) maxsz = 10000;

    /* check of only one size to check */
    if (Vflag) 
    {
    	bflag = 0;		/* clear binary size flag */
	nsizes = 1;		/* set number of sizes to one */
   	sizes[0] = maxsize = maxsz;	/* set 1st size to the one size */
    }
    /* find index into sizes array that is <= max size */
    for (ii = 0; ii < nsizes; ii++) 
    {
	if (sizes[ii] >= maxsz)
	{
		/* cap the max size */
	    	sizes[ii] = maxsz;
		nsizes = ii+1;
		break;
	}
    }
    /* cap the max size to test */
    if (maxsz < maxsize) maxsize = maxsz;

    /* round up to next mod 16 bytes */
    asize = (maxsize+15) & ~0x0f;

    /* print header message */
    printf("\n%s ", argv[0]);
    if (vflag) 		printf("validation"); 
    if (vflag && tflag) printf(" and ");
    if (tflag) 
    {
	printf("timing"); 
    	uname(&name);
   	printf(" of bcopy(s1, s2, n) on %s machine -- release %s\n",
	    name.machine, name.release);
    }
    /* now write out optional comments */
    for ( ; optind < argc; optind++) 
    {
	printf("%s ", argv[optind]);
    }
    printf("\n");

    /* now check if something to do */
    if (tflag == 0 && vflag == 0) usage(argv[0]);

    /* get memory for tests */
    if (!(test_ray = (char *)MALLOC16((asize + slopbytes) * 2)))
	fprintf(stderr,"malloc failed, no memory\n"), exit(2);

    if (!(cntl_ray = (char *)MALLOC16((asize + slopbytes) * 2)))
	fprintf(stderr,"malloc failed, no memory\n"), exit(2);

    /* get an array of non -1 random numbers */
    for (ii = 0; ii < PRESET; ii++) 
    {
	/* get a random non -1 number */
	while ((jj = rand() & 0xff) == 0xff)
		;
	ran_numb[ii] = jj;
    }
    /* check if default number of cases */
    if (Case < 0 || Case > ncases)
    {
	Case = 0;
	ncases = NCASES;
    /* if just one case was specified */
    } else 
    {
    	ncases = Case + 1;
    }
    /* Now Validate for all sizes */
    for (ii = 0; vflag && (ii < nsizes); ii++) 
    {
	if (bflag)
	{
	    size = 1;
	    for (jj = 0; jj < ii; jj++) { size *= 2; }
	    sizes[ii] = size;
	    if (size > maxsize) break;
	} else 
	{
	    size = sizes[ii];
	}
	printf("Validation of %7d bytes\n", size);

	/* do will all possible destination/source alignments */
	for (d_off = 0; d_off < NALIGN_dst; d_off++) 
	for (s_off = 0; s_off < NALIGN_src; s_off++) 
	{
	    /* Try all NCASES */
    	    for (icase = Case; icase < ncases; icase++)
    	    {
		switch(icase)
		{
		/* dest < source, non-overlap */
		case 0:
    		    s1_test = test_ray	+ sizeof(int);
		    s2_test = s1_test	+ asize + slopbytes;
		    s1_cntl = cntl_ray	+ sizeof(int);
    		    s2_cntl = s1_cntl	+ asize + slopbytes;
		    break;

		/* dest > source, non-overlap */
		case 1:
    		    s2_test = test_ray	+ sizeof(int);
		    s1_test = s2_test	+ asize + slopbytes;
		    s2_cntl = cntl_ray	+ sizeof(int);
    		    s1_cntl = s2_cntl	+ asize + slopbytes;
		    break;

		/* dest < source, non-overlap > 2Gb */
		case 2:
		    if (size > PRESET) continue;
    		    s1_test = test_ray	+ sizeof(int);
		    s2_test = (char *)(((int)(stk_test + 15) & - 16) + sizeof(int));
		    s1_cntl = cntl_ray	+ sizeof(int);
    		    s2_cntl = stk_cntl	+ sizeof(int);
		    break;

		/* dest > source, non-overlap > 2Gb */
		case 3:
		    if (size > PRESET) continue;
    		    s2_test = test_ray	+ sizeof(int);
		    s1_test = (char *)(((int)(stk_test + 15) & - 16) + sizeof(int));
		    s2_cntl = cntl_ray	+ sizeof(int);
    		    s1_cntl = stk_cntl	+ sizeof(int);
		    break;

		/* dest < source, overlap */
		case 4:
    		    s1_test = test_ray	+ sizeof(int);
		    s2_test = s1_test	+ NALIGN_dst + NALIGN_src;
    		    s1_cntl = cntl_ray	+ sizeof(int);
		    s2_cntl = s1_cntl	+ NALIGN_dst + NALIGN_src;
		    break;

		/* dest > source, overlap */
		case 5:
    		    s2_test = test_ray	+ sizeof(int);
		    s1_test = s2_test	+ slopbytes;
    		    s2_cntl = cntl_ray	+ sizeof(int);
		    s1_cntl = s2_cntl	+ slopbytes;
		    break;

		/* dest <> source, overlap < 4 bytes */
		case 6:
    		    s2_test = s1_test = test_ray + sizeof(int);
		    s2_cntl = s1_cntl = cntl_ray + sizeof(int);
		    break;
		}
		/* add in the alignments */
		s2_test += s_off;
		s2_cntl += s_off;
		s1_test += d_off;
		s1_cntl += d_off;

		if (dflag) 
			printf("s1/s2=%d/%d - %s (dest = 0x%x, source = 0x%x)\n", 
			d_off, 
			s_off, 
			cases[icase],
			s1_test,
			s2_test);
	
		/* preset whole dest arrays to known values */
		memset(s1_test - sizeof(int), -1, size + slopbytes + sizeof(int));
		memset(s1_cntl - sizeof(int), -1, size + slopbytes + sizeof(int));
    	
		/* preset source arrays with known data */
		if (size <= PRESET)
		{
		    /* whole source arrays */
		    memcpy(s2_test, ran_numb, size);
		    memcpy(s2_cntl, ran_numb, size);
		} else
		{
		    /* preset beginning of source arrays */
		    memcpy(s2_test, ran_numb, PRESET);
		    memcpy(s2_cntl, ran_numb, PRESET);
    	
		    /* preset end of source arrays */
		    memcpy(s2_test + size - PRESET, ran_numb, PRESET);
		    memcpy(s2_cntl + size - PRESET, ran_numb, PRESET);
		}
		/* now validate the routine */
		val_bcopy(s1_cntl, s2_cntl, s1_test, s2_test, size, icase);
	    }
	}
    }
    /* Now Time for all ncases, sizes and offsets */
    for (icase = Case; tflag && (icase < ncases); icase++)
    {
	int	minnumb = -1;
	int	maxnumb = 0;

	/* All sizes */
	for (ii = 0; ii < nsizes; ii++) 
	{
	    if (bflag)
	    {
	        size = 1;
	        for (jj = 0; jj < ii; jj++) { size *= 2; }
	        sizes[ii] = size;
	        if (size > maxsize) break;
	    } else 
	    {
	        size = sizes[ii];
	    }
	    switch(icase)
	    {
	    /* dest < source, non-overlap */
	    case 0:
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s1_test = test_ray	+ sizeof(int);
		s2_test = s1_test	+ asize + slopbytes;
		break;

	    /* dest > source, non-overlap */
	    case 1:
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
		s2_test = test_ray	+ sizeof(int);
    		s1_test = s2_test	+ asize + slopbytes;
		break;

	    /* dest < source, non-overlap > 2Gb */
	    case 2:
		if (size > PRESET) continue;
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s1_test = test_ray	+ sizeof(int);
		s2_test = (char *)(((int)(stk_test + 15) & - 16) + sizeof(int));
		break;

	    /* dest > source, non-overlap > 2Gb */
	    case 3:
		if (size > PRESET) continue;
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s2_test = test_ray	+ sizeof(int);
		s1_test = (char *)(((int)(stk_test + 15) & - 16) + sizeof(int));
		break;

	    /* dest < source, overlap */
	    case 4:
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s1_test = test_ray	+ sizeof(int);
		s2_test = s1_test	+ NALIGN_dst + NALIGN_src;
		break;

	    /* dest > source, overlap */
	    case 5:
		if (size < (NALIGN_dst + NALIGN_src + sizeof(int))) continue;
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s2_test = test_ray	+ sizeof(int);
		s1_test = s2_test	+ NALIGN_dst + NALIGN_src;
		break;

	    /* dest <> source, overlap < 4 bytes */
	    case 6:
		if (minnumb == -1) minnumb = ii;
		maxnumb = ii;
    		s2_test = s1_test = test_ray + sizeof(int);
		break;
	    }

	    /* do will all possible destination/source alignments */
	    for (d_off = 0; d_off < NALIGN_dst; d_off++) 
	    for (s_off = 0; s_off < NALIGN_src; s_off++) 
	    {
		char *s1 = s1_test + d_off;
		char *s2 = s2_test + s_off;

		if (dflag) 
		    printf("source = 0x%x, dest = 0x%x, source - dest = %d\n",
			s2, s1, s2-s1);

    		/* do the requred iterations for good timeings */
    		for (kk = 0; kk < iter; kk++) 
    		{
	 	    /* get the Micorseconds per call time */
		    dft = time_bcopy(s1, s2, size);

		    /* save minimum value from ITERATIONS */
		    if ((kk == 0) || (dft < tim[s_off][d_off][ii]))
			 tim[s_off][d_off][ii] = dft;
		}
	    }
	}

	if (minnumb != -1) 
	{
	    print_times(minnumb, maxnumb, icase);
	}
    }
}

print_times(minnum, maxnum, casenum)
int	minnum;
int	maxnum;
int	casenum;
{
    int		size, ii, jj;
    int		s_off, d_off;
    int		anumb;
    double	pft, aft;

    printf("\n%s\n", cases[casenum]);

    /* check if any data to print */
    if (minnum > maxnum) return;

    printf(
"bcopy      Usec   Usec  Nanoseconds deviation from the average of alignments\n");
    printf(
"(s1,s2,n)   per    per\n");
    printf(
" length    call   byte  0/0 1/0 2/0 3/0 0/1 1/1 2/1 3/1 0/2 1/2 2/2 3/2 0/3 1/3 2/3 3/3\n");
    for (ii = minnum; ii <= maxnum; ii++) 
    {
	printf("%7d", sizes[ii]);

	/* set size to divisor of sizes */
	if (sizes[ii] == 0)	size = 1;
	else			size = sizes[ii];

	/* calculate average time for all alignments */
	for (aft = 0.0, s_off = 0, anumb = 0; s_off < NALIGN_src; s_off++) 
	for (d_off = 0; d_off < NALIGN_dst; d_off++)
	{
		if (casenum == 6 && s_off == d_off) continue;
		aft += tim[s_off][d_off][ii];
		anumb++;
	}
	/* round ft per call */
	aft = aft / (double)anumb;
	
	/* print Elapsed time and average microseconds per byte */
	printf(" %7.1f %6.3f ", aft, aft/(double)size);

	/* print delta nanoseconds for each alignment */
	for (s_off = 0; s_off < NALIGN_src; s_off++) 
	for (d_off = 0; d_off < NALIGN_dst; d_off++) 
	{
		if (casenum == 6 && s_off == d_off)
		{
			printf("  na"); 
		} else
		{
			pft = (tim[s_off][d_off][ii] - aft) / (double)size;
			printf("%4.0f", pft*1000.0); 
		}
	}
    	printf("\n");
    }
}

/* Nsec of run time in u_int */
double
time_bcopy(s1, s2, size)
register	char *s1;
register	char *s2;
register	unsigned int size;
{
	struct timeval  tp_start, tp_end;
	register 	jj;
	int		calls;
	double		ftd;
	unsigned int	fsize;

	/* Now time the routine for a 5 Ms run */
	for (calls = 1; calls <= MAX_ITER; calls++)
	{
		jj = calls;
		gettimeofday(&tp_start, NULL);

		/* execute bcopy a number of times */
		do {
			bcopy(s2, s1, size);
		} while (--jj);

		gettimeofday(&tp_end, NULL);

		/* calculate difference in Microseconds */
		ftd = (tp_end.tv_sec - tp_start.tv_sec) * 1000000.0 
			+ tp_end.tv_usec - tp_start.tv_usec;

		/* do long enough to get a good timeing */
		if (ftd > 5000.0) break;

		/* not long enough, so try another prediction */
		if (ftd < 3000.0) 
		{
			/* double at least */
			calls = (calls * 6000.0) / ftd;
		} 
		else 
		{
			/* add 50% when close */
			calls = calls + (calls+1)/2;
		}
	}
	/* calculate number of microseconds per call */
	ftd /=  (double)calls;
    
	if (size == 0)	fsize = 1;
	else		fsize = size;

	if (dflag) fprintf(stderr, "size = %d, Usec/byte = %6.3f\n", size, ftd/fsize); 

	return(ftd);
}

/* Validate parallel copy routines */
val_bcopy(s1_cntl, s2_cntl, s1_test, s2_test, size, mcase)
char		*s1_cntl; 	/* dest control array	*/
char		*s2_cntl; 	/* source control array	*/
char		*s1_test; 	/* dest test array	*/
char		*s2_test;	/* source test array	*/
unsigned int	size;		/* size of arrays	*/
int		mcase;		/* ascii case type	*/
{
	char	*s3;

	/* The caller must meet this criteria!! */
/****	if ((s1_cntl - s2_cntl) != (s1_test - s2_test))
	{
printf( "Invalid test -- bug in validation program on %s\n", cases[mcase]);
		exit(6);
	}
****/
	if (lflag) printit(s1_test, s2_test, size);

	/* copy the control array using "control" copy routine */
	movemem(s1_cntl, s2_cntl, size);

	if (lflag) printit(s1_test, s2_test, size);

	/* copy the test array into the test array */
	bcopy(s2_test, s1_test, size);

	/* compare the dest arrays */
	if (memcmp(	s1_cntl - sizeof(int), 
			s1_test - sizeof(int), 
			size + slopbytes + sizeof(int)))
	{
printf( "Failed %s at %7d bytes with source - dest = %d bytes\n", 
		cases[mcase],
		size, 
		s2_test - s1_test);

		/* print out failure place */
		compar(s1_cntl, s1_test, size);
	}
}

/* routine to compare the data */
compar(good, bad, size)
register char *good, *bad;
{
    register ii;

    for (ii = 0; ii < size; good++, bad++, ii++) 
    {
	if (*good != *bad) 
	{
	    printf("mismatch at byte %d, good = %d, bad = %d\n",
		 ii, *good, *bad);

	    /* print only 1st error */
	    if (!fflag) exit(3);
	}
    } 
    if (!fflag) 
    {
    	printf("failure occured outside the copied memory bounds\n");
	exit(3);
    }
}

/* control copy routine */
movemem(s1, s2, n)
register unsigned char		*s1;
register unsigned char		*s2;
register size_t			n;
{
	if (n == 0) return;

	/* check if dest > source and overlaping buffer */
	if (s1 > s2 && s1 < s2+n) {
		s1 += n;
		s2 += n;
		/* copy backwards */
		do {
			*--s1 = *--s2;
		} while (--n);
	} else	
	{
		/* copy forwards */
		do {
			*s1++ = *s2++;
		} while (--n);
	}
}

printit(s1, s2, size)
unsigned char	*s1;
unsigned char	*s2;
int	size;
{
	int mm;
	
	printf("\nCompare s1=0x%x, s2=0x%x\n", s1, s2);
	for (mm = 0; mm < size; mm++)
	{
		if (mm % 16 == 0) printf("\n");
		if (*s1 == *s2)
			printf("%2x=%2x ", *s1, *s2);
		else
			printf("%2x#%2x ", *s1, *s2);
		s1++;
		s2++;
	}
	if ((mm-1) % 16 != 0) printf("\n");
}
#endif	BCOPY_VALIDATE_AND_TEST
#ifdef	BZERO_VALIDATE_AND_TEST
/* Command to time and validate bzero() (dlb) */

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <sys/utsname.h>

int	processor = 0;

/* number of default passed to get min value */
#define ITERATIONS	8

/* max iterations to time */
#define MAX_ITER	8192

/* verious sizes of arrays to validate and time */
int sizes[] = {
	 0,  1,  2,  3,  4,  5,  6,  7 , 8,  9, 
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
	70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
	90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
	100, 101, 102, 103, 104, 105, 106, 107,
	108, 109, 110, 111, 112, 113, 114, 115,
	116, 117, 118, 119, 120, 121, 122, 123,
	124, 125, 126, 127,
	128, 144, 160, 176, 192, 208, 224, 240,
	256, 512,
	/*
	 * (1024-4-1)..(1024+4+1)
	 */
        1019, 1020, 1021, 1022, 1023, 1024, 1025,
	1026, 1027, 1028, 1029, 1030, 1031, 1032,
	1033, 1034, 1035, 1036, 1037, 1038, 1039,
	1040, 1041, 1042, 1043, 1044, 1045,
	2048, 4096, 8196, 16384, 32768, 
	1024 * 1024 -1, 1024 *1024, 1024 *1024 +1,
	2 * 1024 * 1024 -1, 2 * 1024 *1024, 2 * 1024 *1024 +37 
};

/* number of numbers in sizes[] */
int nsizes =  sizeof(sizes) / sizeof(int); 

#define	NALIGN_dst	16

/* slop bytes on each end of test arrays */
#define	slopbytes	(NALIGN_dst + sizeof(int))

/* max number of times to remember */
u_long tim[NALIGN_dst][sizeof(sizes)/sizeof(int)];

struct utsname name;

int	dflag	= 0;		/* Turn on debug flag */
int	aflag	= 0;		/* Just one alignment flag */
int	tflag	= 0;		/* Set timeing flag */
int	vflag	= 1;		/* validation flag */
int	fflag	= 0;		/* Print all errors */
int	Vflag	= 0;		/* One value only */
int	iter	= ITERATIONS;	/* default interations */

usage(pname)
{
	fprintf(stderr, 
"Usage: %s [-pvtfd][-V 1value][-i iterations][-m maxsize] [messages]\n", pname);

	fprintf(stderr, 
"Options...     Description\n\n");
	fprintf(stderr, 
"-v             Turn OFF validation - default on\n");
	fprintf(stderr, 
"-t             Turn ON Timing - default off\n");
	fprintf(stderr, 
"-f             Dont quit on first validation error\n");
	fprintf(stderr, 
"-d             Turn on debug\n");
	fprintf(stderr, 
"-V <value>     Validate and/or Time only <value> number of bytes\n");
	fprintf(stderr, 
"-i <iterations> Do <iterations> for more/less accurate times - default = 8\n");
	fprintf(stderr, 
"-m <maxsize>   Validate and/or Time to <maxsize> bytes - default = 2097189\n");
	fprintf(stderr, 
"[mesages]      Optional message to stdout\n");
	fprintf(stderr, "\n");
	exit(2);
}

/*
 * malloc() and ensure that its on a 16 byte boundry
 */
/*** #define MALLOC16(x) malloc(((x) + 15) & ~0x0f) ***/
#define MALLOC16(X) ((malloc((X) + 15) + 15) & ~0x0f)

main(argc, argv)
int	 argc;
char	*argv[];
{
    extern char	*optarg;
    extern int 	optind;

    register char	*s1_test, *s2_source, *s3;
    register int	chr;
    char		*s1_cntl;
    char 		*source, *dest_test, *dest_cntl;
    char		*cntl_s1;
    register 	jj;
    int		size;
    int		calls;
    int		ii, kk, vv;
    int		d_off;
    struct timeval  tp_start, tp_end;
    u_long		ft;
    u_int		max	= -1;
    u_int		maxsize = sizes[nsizes-1];


    /* do not do output buffering in case of core dump */
    setbuf(stdout, 0);

    /* get optional parameters */
    while ((ii = getopt(argc, argv, "dftvi:m:V:p:")) != EOF) 
    {
	switch (ii) 
	{
	case 'p':	processor  = atoi(optarg); break;/* set processor     */
	case 'd':	dflag++; break;			/* Turn on debug      */
	case 'f':	fflag++; break;			/* Print all errors   */
	case 't':	tflag++; break;      		/* Set timeing        */
	case 'v':	vflag = 0; break;      		/* Clear validate     */
	case 'i':	iter = atoi(optarg); break;	/* number iterations  */
	case 'm':	max  = atoi(optarg); break;	/* max size to test   */
	case 'V':	max  = atoi(optarg); Vflag++;break;/* one value only  */
	default:	usage(argv[0]); exit(2);
	}
    }

    /* check of only one size to check */
    if (Vflag) 
    {
	nsizes = 1;
   	sizes[0] = maxsize = max;
    }
    /* cap the max size to test */
    if (max < maxsize) maxsize = max;

    /* print header message */
    printf("\n%s ", argv[0]);
    if (vflag) 		printf("validation"); 
    if (vflag && tflag) printf(" and ");
    if (tflag) 
    {
	printf("timing"); 
    	uname(&name);
    	printf(" of bzero(s1, n) on %s machine -- release %s\n",
	    name.machine, name.release);
    }
    /* now write out optional comments */
    for ( ; optind < argc; optind++) 
    {
	printf("%s ", argv[optind]);
    }
    printf("\n");

    /* now check if something to do */
    if (tflag == 0 && vflag == 0) usage(argv[0]);

    /* get memory for tests */
    if (!(source = (char *)MALLOC16(maxsize + slopbytes)))
	fprintf(stderr,"no memory\n"), exit(2);

    if (!(dest_cntl = (char *)MALLOC16(maxsize + slopbytes + 32)))
	fprintf(stderr,"no memory\n"), exit(2);

    if (!(dest_test = (char *)MALLOC16(maxsize + slopbytes + 32)))
	fprintf(stderr,"no memory\n"), exit(2);

    /* fill the source array with non-zero random numbers */
    for (ii = 0; ii < (maxsize + slopbytes); ii++) 
    {
	/* get a random number */
	do
	{
		jj = rand() & 0xff;
	} while (jj == 0);
	source[ii] = jj;
    }

    /* do the requred iterations for good timeings */
    for (kk = 0; kk < iter; kk++) 
    {
	/* all sizes */
	for (ii = 0; ii < nsizes; ii++) 
	{
	    /* do will all possible destination alignments */
	    for (d_off = 0; d_off < NALIGN_dst; d_off++) 
	    {
	    	/* set up the destination array addreses */
	    	s1_test = &dest_test[d_off] + sizeof(int);
	    	s1_cntl = &dest_cntl[d_off] + sizeof(int);

		/* get len of bzero compare test */
		if ((size = sizes[ii]) >= max) 
		{
		    size = sizes[ii] = max;
		    nsizes = ii+1;
		}

		/* only validate on 1st iteration */
		if (kk == 0 && vflag == 1) 
		{
		    if (d_off == 0)
		    {
			printf("Validation of %7d bytes\n", size);
		    }

		    /* make dest arrays of data same as source */
		    memcpy(dest_cntl, source, size + slopbytes);
		    memcpy(dest_test, source, size + slopbytes);

		    /* make up the control array */
		    memset(s1_cntl, 0, size);

		    /* make up the test array */
		    bzero(s1_test, size);

		    /* compare the dest areas */
		    if (memcmp(dest_cntl, dest_test, size + slopbytes))
		    {
			printf(
			    "failed validation at %7d bytes s1 = %d\n",
			    size, d_off);

			/* print out failure place */
			compar(s1_cntl, s1_test, size);
		    }
		}

		/* Now time the routine */
		for (calls = 1; tflag && calls <= MAX_ITER; calls++)
		{
		    jj = calls;
		    gettimeofday(&tp_start, NULL);
    
		    /* execute bzero a number of times */
		    do
		    {
			    bzero(s1_test, size);
		    } while (--jj);
    
		    gettimeofday(&tp_end, NULL);

		    /* calculate difference in microseconds */
		    ft = (tp_end.tv_sec - tp_start.tv_sec) * 1000000 
			    + tp_end.tv_usec - tp_start.tv_usec;

		    /* do long enough to get a good timeing */
		    if (ft > 5000) break;

		    /* not long enough, so try another prediction */
		    if (ft < 3000) 
		    {
			    calls = calls * (6000 / ft);
		    } 
		    else 
		    {
			    calls = calls + (calls+1)/2;
		    }
		} 
		/* calculate number of nanoseconds per call */
		ft = (ft * 1000) / calls;

		if (!size) size = 1;
		if (dflag) 
		{
		    fprintf(stderr,
			"offset = %d, size = %d, Nsec/byte = %6d\n", 
			d_off, size, ft/size); 
		}
					    
		/* save minimum value from ITERATIONS */
		if (kk == 0)
		    tim[d_off][ii]= ft;
		else if (ft < tim[d_off][ii]) 
		    tim[d_off][ii] = ft;
	    }
	}
    }
    if (tflag == 0) exit(0);

    printf(
"bzero      Usec   Usec  Nanoseconds deviation from average s1/value alignments\n");
    printf(
"(s1,n)     per    per\n");
    printf(
" length    call   byte    0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15\n");
    for (ii = 0; ii < nsizes; ii++) 
    {
	double dft;
	printf("%7d", sizes[ii]);

	/* set kk to divisor of sizes */
	if (sizes[ii] == 0)	kk = 1;
	else			kk = sizes[ii];

	/* calculate average time for all alignments */
	for (ft = 0, d_off = 0; d_off < NALIGN_dst; d_off++)
	{
	    ft += tim[d_off][ii]; 
	    /*** dft += tim[d_off][ii]; ***/
	}
	/* round ft per call */
	ft = (ft + NALIGN_dst/2) / NALIGN_dst;
	/*** dft = dft / NALIGN_dst; ***/
	

	/* print Elapsed time and average time per byte */
/***	printf(" %7.1f %6.3f ", dft/1000.0, dft/1000.0/kk); ***/

	printf(" %5u.%01d %2u.%03d ", 
		(ft+50)/1000,
		((ft+50)%1000)/100,
		(2*ft/kk+1)/2000,
		((2*ft/kk+1)%2000)/2);

	/* print delta nanoseconds for each alignment */
	for (d_off = 0; d_off < NALIGN_dst; d_off++) 
	{
	    jj = (int)(tim[d_off][ii] - ft) / kk;
	    printf("%4d", jj); 
	}
    	printf("\n");
    }
    exit(0);
}

/* routine to compare the data */
compar(good, bad, size)
register char *good, *bad;
{
    register ii;

    for (ii = 0; ii < size; good++, bad++, ii++) 
    {
	if (*good != *bad) 
	{
	    printf("mismatch at byte %d, good = %d, bad = %d\n",
		 ii, *good, *bad);

	    /* print only 1st error */
	    if (!fflag) exit(3);
	}
    } 
    if (!fflag) 
    {
	printf("failure occured outside the copied memory bounds\n");
	exit(3);
    }
}
#endif	BZERO_VALIDATE_AND_TEST
