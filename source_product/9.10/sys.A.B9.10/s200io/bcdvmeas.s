 # $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/bcdvmeas.s,v $
 # $Revision: 1.3.84.3 $	$Author: kcs $
 # $State: Exp $   	$Locker:  $
 # $Date: 93/09/17 21:10:14 $
 #  HPUX_ID: @(#)bcdvmeas.s	55.1		88/12/23

 # (c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
 # (c) Copyright 1979 The Regents of the University of Colorado, body corporate 
 # (c) Copyright 1979, 1980, 1983 The Regents of the University of California
 # (c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
 # The contents of this software are proprietary and confidential to the Hewlett
 # Packard Company, and are limited in distribution to those with a direct need
 # to know. Individuals having access to this software are responsible for main-
 # taining confidentiality of the content and for keeping the software secure
 # when not in use.  Transfer to any party is strictly forbidden other than as
 # expressly permitted in writing by Hewlett-Packard Company. Unauthorized trans
 # fer to or possession by any unauthorized party may be a criminal offense.

 #                     RESTRICTED RIGHTS LEGEND

 #           Use,  duplication,  or disclosure by the Government  is
 #           subject to restrictions as set forth in subdivision (b)
 #           (3)  (ii)  of the Rights in Technical Data and Computer
 #           Software clause at 52.227-7013.

 #                      HEWLETT-PACKARD COMPANY
 #                         3000 Hanover St.
 #                       Palo Alto, CA  94304

 #****************************************************************
 #	@(#)vmeas.s	1.3 - 11/28/85
 #	Fast copy routines called by xcopy in VME driver
 #	all are called as
 #		XXX(from, to, count)
 #	They xfer bytes, words, or blocks, incrementing both addresses
 #	or just one address.
 #	"count" is a count of xfers, not bytes. 
 #	All use 'moves' because we are crossing address spaces.
 #	All have the following register conventions:
 #		a0	from address
 #		a1	to address
 #		d0	count
 #		d1	temp for each byte, word, or long
 #****************************************************************
    	set	from,4
  	set	to,8
     	set	count,12

	text
 #	xbytai -- bytes, auto-increment both addresses

	global 	_xbytai
_xbytai:
	mov.l	from(%sp),%a0
	mov.l  to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xbai3
	bra.b	xbai1

 # from userland to vme card 
xbai2:	movs.b (%a0)+,%d1
	mov.b %d1,(%a1)+
xbai1:	dbra	%d0,xbai2
	sub.l	&65536,%d0
	bcc.b	xbai2
	rts

 # from vme card to userland
xbai4:	mov.b (%a0)+,%d1
	movs.b %d1,(%a1)+
xbai3:	dbra	%d0,xbai4
	sub.l	&65536,%d0
	bcc.b	xbai4
	rts

 #	xbytfix1 -- bytes, address 1 fixed

	global 	_xbytfix1
_xbytfix1:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xbf13
	bra.b	xbf11

 # from userland to vme card 
xbf12:	movs.b (%a0),%d1
	mov.b %d1,(%a1)+
xbf11:	dbra	%d0,xbf12
	sub.l	&65536,%d0
	bcc.b	xbf12
	rts

 # from vme card to userland
xbf14:	mov.b (%a0),%d1
	movs.b %d1,(%a1)+
xbf13:	dbra	%d0,xbf14
	sub.l	&65536,%d0
	bcc.b	xbf14
	rts

 #	xbytfix2 -- bytes, address 2 fixed

	global 	_xbytfix2
_xbytfix2:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xbf23
	bra.b	xbf21

 # from userland to vme card 
xbf22:	movs.b (%a0)+,%d1
	mov.b %d1,(%a1)
xbf21:	dbra	%d0,xbf22
	sub.l	&65536,%d0
	bcc.b	xbf22
	rts

 # from vme card to userland
xbf24:	mov.b (%a0)+,%d1
	movs.b %d1,(%a1)
xbf23:	dbra	%d0,xbf24
	sub.l	&65536,%d0
	bcc.b	xbf24
	rts

 #	xwdai -- words, auto-increment both addresses

	global 	_xwdai
_xwdai:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xwai3
	bra.b	xwai1

 # from userland to vme card 
xwai2:	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)+
xwai1:	dbra	%d0,xwai2
	sub.l	&65536,%d0
	bcc.b	xwai2
	rts

 # from vme card to userland
xwai4:	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)+
xwai3:	dbra	%d0,xwai4
	sub.l	&65536,%d0
	bcc.b	xwai4
	rts

 #	xwdfix1 -- words, address 1 fixed

	global 	_xwdfix1
_xwdfix1:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xwf13
	bra.b	xwf11

 # from userland to vme card 
xwf12:	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
xwf11:	dbra	%d0,xwf12
	sub.l	&65536,%d0
	bcc.b	xwf12
	rts

 # from vme card to userland
xwf14:	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
xwf13:	dbra	%d0,xwf14
	sub.l	&65536,%d0
	bcc.b	xwf14
	rts

 #	xwdfix2 -- words, address 2 fixed

	global 	_xwdfix2
_xwdfix2:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne.b	xwf23
	bra.b	xwf21

 # from userland to vme card 
xwf22:	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
xwf21:	dbra	%d0,xwf22
	sub.l	&65536,%d0
	bcc.b	xwf22
	rts

 # from vme card to userland
xwf24:	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
xwf23:	dbra	%d0,xwf24
	sub.l	&65536,%d0
	bcc.b	xwf24
	rts


 #	xblkai -- 64-byte blocks, auto-increment both addresses

	global 	_xblkai
_xblkai:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne	xkai3
	bra	xkai1

 # from userland to vme card 
 xkai2:	movs.l (%a0)+,%d1	#16 pairs of long moves == 64 bytes
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
	movs.l (%a0)+,%d1
	mov.l %d1,(%a1)+
xkai1:	dbra	%d0,xkai2
	sub.l	&65536,%d0
	bcc	xkai2
	rts

 # from vme card to userland
 xkai4:	mov.l (%a0)+,%d1	#16 pairs of long moves == 64 bytes
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
	mov.l (%a0)+,%d1
	movs.l %d1,(%a1)+
xkai3:	dbra	%d0,xkai4
	sub.l	&65536,%d0
	bcc	xkai4
	rts

 #	xblkfix1 -- 64-byte blocks, address 1 fixed
 #	must use word moves to prevent 68xxx auto-increment

	global 	_xblkfix1
_xblkfix1:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne	xkf13
	bra	xkf11

 # from userland to vme card 
xkf12:	movs.w (%a0),%d1		#32 pairs of word moves == 64 bytes
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
	movs.w (%a0),%d1
	mov.w %d1,(%a1)+
xkf11:	dbra	%d0,xkf12
	sub.l	&65536,%d0
	bcc	xkf12
	rts

 # from vme card to userland
xkf14:	mov.w (%a0),%d1		#32 pairs of word moves == 64 bytes
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
	mov.w (%a0),%d1
	movs.w %d1,(%a1)+
xkf13:	dbra	%d0,xkf14
	sub.l	&65536,%d0
	bcc	xkf14
	rts

 #	xblkfix2 -- 64-byte blocks, address 2 fixed
 #	must use word moves to prevent 68xxx auto-increment

	global 	_xblkfix2
_xblkfix2:
	mova.l	from(%sp),%a0
	mova.l to(%sp),%a1
	mov.l  count(%sp),%d0
	tst.l	readvme
	bne	xkf23
	bra	xkf21

 # from userland to vme card 
xkf22:	movs.w (%a0)+,%d1	#32 pairs of word moves == 64 bytes
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
	movs.w (%a0)+,%d1
	mov.w %d1,(%a1)
xkf21:	dbra	%d0,xkf22
	sub.l	&65536,%d0
	bcc	xkf22
	rts

 # from vme card to userland
xkf24:	mov.w (%a0)+,%d1	#32 pairs of word moves == 64 bytes
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
	mov.w (%a0)+,%d1
	movs.w %d1,(%a1)
xkf23:	dbra	%d0,xkf24
	sub.l	&65536,%d0
	bcc	xkf24
	rts


 #*********************************************************
 # loadfc(readvme) -- load function code registers for moves
 #	readvme <> 0 => reading from the vme
 #	readvme == 0 => writing to the vme
 #	code #1 => user space
 #	code #5 => supervisor space
 #*********************************************************

 # if == 0 then reading from user space and writing to vme card
 # else != 0 then reading from vme card and writing to user space
	data
readvme:
	long	0
	text

	global	_loadfc
_loadfc:
	mov.l	4(%sp),readvme
 #	movq	&1,%d0		#d0 user, d1 super
 #	movq	&5,%d1
 #	tst.l	4(%sp)		#swap this if readvme
 #	beq.b	noswap
 #	exg	%d0,%d1
 #noswap:	movc	%d0,%sfc		#d0 source, d1 destination
 #	movc	%d1,%dfc
	rts
