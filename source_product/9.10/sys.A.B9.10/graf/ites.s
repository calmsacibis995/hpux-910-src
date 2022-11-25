 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ites.s,v $
 # $Revision: 1.4.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:00:45 $

 # HPUX_ID: @(#)ites.s	55.1		88/12/23


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


 #
 # Block transfer subroutine:
 #	This variation is used for copying backwards.
 #	Must be 16 bit word alligned.
 #	bltb(destination, source, count)
 #	returns count

	global	_bltb
	text

_bltb:	mov.l	4(%sp),%a0	#destination
	mov.l	8(%sp),%a1	#source
	mov.l	12(%sp),%d1	#count
	mov.l	%d1,%d0		#copy remaining count
	and.l	&~3,%d0		#count mod 4 is number of long words
	beq	bltb5		#hmm, must not be any
	sub.l	%d0,%d1		#long words moved * 4 = bytes moved
	asr.l	&2,%d0		#number of long words
	cmp.l	%d0,&12		#do we have a bunch to do?
	blt	bltb38		#no, just do normal moves
        nop
	movm.l	%d1-%d7/%a2-%a6,-(%sp) #save some registers
bltb34:	
        nop
	movm.l	(%a1)+,%d1-%d7/%a2-%a6 #block move via various registers
        nop
	movm.l	%d1-%d7/%a2-%a6,(%a0)
	adda.l	&48,%a0		#movem.l won't let me auto inc a destination
	sub.l	&12,%d0		#we moved twelve longs worth
	cmp.l	%d0,&12		#do we have another 12 to go
	bge	bltb34		#yes, keep at it
        nop
	movm.l	(%sp)+,%d1-%d7/%a2-%a6 #restore registers
	tst.l	%d0		#any long's left
	beq	bltb5		#no, nothing but a few random bytes
bltb38:	subq.l	&1,%d0		#dbf is a crock
bltb4:	mov.l	(%a1)+,(%a0)+	#copy as many long words as possible
	dbf	%d0,bltb4		#while long word count
bltb5:	tst.l	%d1		#anything left to do?
	beq	bltb7		#nothing left
bltb6:	mov.w	(%a1)+,(%a0)+	#copy any residual words
	subq.l	&2,%d1
	bgt.b	bltb6
bltb7:	rts

 #
 # Block transfer subroutine:
 #	This variation is used for copying forwards.
 #	Must be 16 bit word alligned.
 #	bltf(destination, source, count)
 #	returns count

	global	_bltf
	text

_bltf:	mov.l	4(%sp),%a0	#destination
	mov.l	8(%sp),%a1	#source
	mov.l	12(%sp),%d1	#count
	mov.l	%d1,%d0		#copy remaining count
	and.l	&~3,%d0		#count mod 4 is number of long words
	beq	bltf5		#hmm, must not be any
	sub.l	%d0,%d1		#long words moved * 4 = bytes moved
	asr.l	&2,%d0		#number of long words
	cmp.l	%d0,&12		#do we have a bunch to do?
	blt	bltf38		#no, just do normal moves
        nop
	movm.l	%d1-%d7/%a2-%a6,-(%sp) #save some registers
bltf34:	suba.l	&48,%a1		#movem.l won't let me auto dec a source
        nop
	movm.l	(%a1),%d1-%d7/%a2-%a6 #block move via various registers
        nop
	movm.l	%d1-%d7/%a2-%a6,-(%a0)
	sub.l	&12,%d0		#we moved twelve longs worth
	cmp.l	%d0,&12		#do we have another 12 to go
	bge	bltf34		#yes, keep at it
        nop
	movm.l	(%sp)+,%d1-%d7/%a2-%a6 #restore registers
	tst.l	%d0		#any long's left
	beq	bltf5		#no, nothing but a few random bytes
bltf38:	subq.l	&1,%d0		#dbf is a crock
bltf4:	mov.l	-(%a1),-(%a0)	#copy as many long words as possible
	dbf	%d0,bltf4		#while long word count
bltf5:	tst.l	%d1		#anything left to do?
	beq	bltf7		#nothing left
bltf6:	mov.w	-(%a1),-(%a0)	#copy any residual words
	subq.l	&2,%d1
	bgt.b	bltf6
bltf7:	rts
