# KLEENIX_ID @(#)print.s	57.1 89/07/29 
#
# prntf, fprntf, sprntf
#
# These  are   simplified   versions  of  printf,   fprintf,  and  sprintf
# respectively  that have had their feature set trimmed down  considerably
# for performance benefits.  Parameters passed and the result returned are
# the same.  As for formatting, the only  characters that can follow a '%'
# are '%', 's', 'c', 'x' or 'd'.  No flags, field  widths,  padding,  etc.
# are allowed.
#
# Author: Mark McDowell (7/3/86)
#
#
	set	_cnt,0
	set	_ptr,4
	set	_base,8
	set	_flag,12
	set	_file,14
	set	_bufendp,16
	set	sizeof_iob,16

	set	EOF,-1

	set	_IOFBF,0
	set	_IOREAD,1
	set	_IOWRT,2
	set	_IONBF,4
	set	_IOMYBUF,8
	set	_IOEOF,16
	set	_IOERR,32
	set	_IOLBF,128
	set	_IORW,256
	set	_IOEXT,512
	set	_IODUMMY,1024

	set	REGS,-16
	set	COUNT,-20
	set	IOP,-24
	set	FILENO,-28
	set	CNT,-32
	set	START,-36
	set	SPRNT,-38
	set	TMP,-40
	set	BUFFER,-52
	set	LOCALSIZE,BUFFER

	data
L1:
	byte	'0,'0,'0,'1,'0,'2,'0,'3,'0,'4,'0,'5,'0,'6,'0,'7
	byte	'0,'8,'0,'9,'0,'a,'0,'b,'0,'c,'0,'d,'0,'e,'0,'f
	byte	'1,'0,'1,'1,'1,'2,'1,'3,'1,'4,'1,'5,'1,'6,'1,'7
	byte	'1,'8,'1,'9,'1,'a,'1,'b,'1,'c,'1,'d,'1,'e,'1,'f
	byte	'2,'0,'2,'1,'2,'2,'2,'3,'2,'4,'2,'5,'2,'6,'2,'7
	byte	'2,'8,'2,'9,'2,'a,'2,'b,'2,'c,'2,'d,'2,'e,'2,'f
	byte	'3,'0,'3,'1,'3,'2,'3,'3,'3,'4,'3,'5,'3,'6,'3,'7
	byte	'3,'8,'3,'9,'3,'a,'3,'b,'3,'c,'3,'d,'3,'e,'3,'f
	byte	'4,'0,'4,'1,'4,'2,'4,'3,'4,'4,'4,'5,'4,'6,'4,'7
	byte	'4,'8,'4,'9,'4,'a,'4,'b,'4,'c,'4,'d,'4,'e,'4,'f
	byte	'5,'0,'5,'1,'5,'2,'5,'3,'5,'4,'5,'5,'5,'6,'5,'7
	byte	'5,'8,'5,'9,'5,'a,'5,'b,'5,'c,'5,'d,'5,'e,'5,'f
	byte	'6,'0,'6,'1,'6,'2,'6,'3,'6,'4,'6,'5,'6,'6,'6,'7
	byte	'6,'8,'6,'9,'6,'a,'6,'b,'6,'c,'6,'d,'6,'e,'6,'f
	byte	'7,'0,'7,'1,'7,'2,'7,'3,'7,'4,'7,'5,'7,'6,'7,'7
	byte	'7,'8,'7,'9,'7,'a,'7,'b,'7,'c,'7,'d,'7,'e,'7,'f
	byte	'8,'0,'8,'1,'8,'2,'8,'3,'8,'4,'8,'5,'8,'6,'8,'7
	byte	'8,'8,'8,'9,'8,'a,'8,'b,'8,'c,'8,'d,'8,'e,'8,'f
	byte	'9,'0,'9,'1,'9,'2,'9,'3,'9,'4,'9,'5,'9,'6,'9,'7
	byte	'9,'8,'9,'9,'9,'a,'9,'b,'9,'c,'9,'d,'9,'e,'9,'f
	byte	'a,'0,'a,'1,'a,'2,'a,'3,'a,'4,'a,'5,'a,'6,'a,'7
	byte	'a,'8,'a,'9,'a,'a,'a,'b,'a,'c,'a,'d,'a,'e,'a,'f
	byte	'b,'0,'b,'1,'b,'2,'b,'3,'b,'4,'b,'5,'b,'6,'b,'7
	byte	'b,'8,'b,'9,'b,'a,'b,'b,'b,'c,'b,'d,'b,'e,'b,'f
	byte	'c,'0,'c,'1,'c,'2,'c,'3,'c,'4,'c,'5,'c,'6,'c,'7
	byte	'c,'8,'c,'9,'c,'a,'c,'b,'c,'c,'c,'d,'c,'e,'c,'f
	byte	'd,'0,'d,'1,'d,'2,'d,'3,'d,'4,'d,'5,'d,'6,'d,'7
	byte	'd,'8,'d,'9,'d,'a,'d,'b,'d,'c,'d,'d,'d,'e,'d,'f
	byte	'e,'0,'e,'1,'e,'2,'e,'3,'e,'4,'e,'5,'e,'6,'e,'7
	byte	'e,'8,'e,'9,'e,'a,'e,'b,'e,'c,'e,'d,'e,'e,'e,'f
	byte	'f,'0,'f,'1,'f,'2,'f,'3,'f,'4,'f,'5,'f,'6,'f,'7
	byte	'f,'8,'f,'9,'f,'a,'f,'b,'f,'c,'f,'d,'f,'e,'f,'f

L2:
	byte	'0,'0,'0,'0,'0,'0,'0,'0,'0,'0
	byte	'1,'1,'1,'1,'1,'1,'1,'1,'1,'1
	byte	'2,'2,'2,'2,'2,'2,'2,'2,'2,'2
	byte	'3,'3,'3,'3,'3,'3,'3,'3,'3,'3
	byte	'4,'4,'4,'4,'4,'4,'4,'4,'4,'4
	byte	'5,'5,'5,'5,'5,'5,'5,'5,'5,'5
	byte	'6,'6,'6,'6,'6,'6,'6,'6,'6,'6
	byte	'7,'7,'7,'7,'7,'7,'7,'7,'7,'7
	byte	'8,'8,'8,'8,'8,'8,'8,'8,'8,'8
	byte	'9,'9,'9,'9,'9,'9,'9,'9,'9,'9

L3:
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9

L4:	# this is a continuation of the previous
	# table so keep them in this order
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'a,'b,'c,'d,'e,'f

	text

	global	_prntf,_fprntf,_sprntf

_prntf:
	link	%a6,&LOCALSIZE
	movm.l	%a2-%a5,REGS(%a6)		# save registers
	lea	__iob+1*sizeof_iob,%a2		# stdout
	mov.l	8(%a6),%a3			# format string
	lea	12(%a6),%a4			# argument pointer
	bra.b	L5

_fprntf:
	link	%a6,&LOCALSIZE
	movm.l	%a2-%a5,REGS(%a6)		# save registers
	movm.l	8(%a6),%a2-%a3			# file, format string
	lea	16(%a6),%a4			# argument pointer

L5:
	#
	# check to see if file is writeable:
	#	if ((((iop->_flag & (_IOWRT | _IOEOF)) != _IOWRT) || 
	#	   (iop->_base == NULL) || (iop->_ptr == iop->_base && 
	#	   iop->_cnt == 0 && !(iop->_flag & (_IONBF | _IOLBF)))) 
	#	   ? _wrtchk(iop) : 0 ) return EOF;
	#
	movq	&(_IOWRT|_IOEOF),%d0
	and.w	_flag(%a2),%d0			# iop->_flag & (_IOWRT|_IOEOF)
	subq.w	&_IOWRT,%d0			# compare with _IOWRT
	bne.b	L6
	tst.l	_base(%a2)			# iop->_base == NULL?
	beq.b	L6
	mov.l	_ptr(%a2),%d0			# iop->_ptr == iop->_base?
	cmp.l	%d0,_base(%a2)
	bne.b	L7
	tst.l	_cnt(%a2)			# iop->_cnt == 0?
	bne.b	L7
	mov.w	&(_IONBF|_IOLBF),%d0		# iop->_flag & (_IONBF|_IOLBF)
	and.w	_flag(%a2),%d0
	bne.b	L7
L6:
	pea	(%a2)
	jsr	__wrtchk			# is file writeable?
	addq.w	&4,%sp
	tst.l	%d0
	beq.b	L7
	movq	&EOF,%d0			# no: return EOF
	movm.l	REGS(%a6),%a2-%a5		# restore register
	unlk	%a6
	rts
L7:
	movq	&0,%d0
	mov.l	%d0,COUNT(%a6)			# initialize count
	mov.w	%d0,SPRNT(%a6)			# sprnt flag
	mov.l	%a2,IOP(%a6)			# save file
	mov.b	_file(%a2),%d0			# file number
	mov.l	%d0,FILENO(%a6)
	mov.w	_flag(%a2),%d0			# if (iop->_flag & _IOEXT)
	and.w	&_IOEXT,%d0
	beq.b	L7A				# then
	mov.l	_bufendp(%a2),%a5		# a5 = iop->_bufendp 
	bra.b	L7B
L7A:	mov.l	%a2,%a0				# else
	sub.l	&__iob,%a0
	mov.l	%a0,%d0
	lsr.l	&2,%d0
	add.l	&___bufendtab,%d0		# a5 = ___bufendtab[iop - __iob]
	mov.l	%d0,%a0
	mov.l	(%a0),%a5			# end of buffer
L7B:	movq	&0,%d1				# assume unbuffered or line
	mov.w	&(_IONBF|_IOLBF),%d0		#    buffered
	and.w	_flag(%a2),%d0
	bne.b	L8
	mov.l	%a5,%d1				# end of buffer
	sub.l	_base(%a2),%d1			# buffer size
L8:
	mov.l	%d1,CNT(%a6)			# for buffer flushing
	mov.l	_ptr(%a2),%a2			# start of buffer
	mov.l	%a2,START(%a6)			# remember start
	bra.b	L9

_sprntf:
	link	%a6,&LOCALSIZE
	movm.l	%a2-%a5,REGS(%a6)		# save registers
	movm.l	8(%a6),%a2-%a3			# dest, format string
	lea	16(%a6),%a4			# argument pointer
	movq	&1,%d0
	mov.w	%d0,SPRNT(%a6)			# signal that this is sprnt
	movq	&-1,%d0
	mov.l	%d0,%a5
L9:
	movq	&'%,%d1				# for finding formats
L10:
	mov.b	(%a3)+,%d0			# get character
	beq.w	L63
	cmp.b	%d0,%d1				# is this a '%'?
	bne.b	L11
	mov.b	(%a3)+,%d0
	cmp.b	%d0,%d1				# a second '%'?
	beq.b	L11
	cmp.b	%d0,&'c				# c format?
	bne.b	L13
	mov.l	(%a4)+,%d0
L11:
	cmp.l	%a2,%a5				# more room in buffer?
	bhs.b	L12
	mov.b	%d0,(%a2)+			# move character to result
	bra.b	L10
L12:
	mov.b	%d0,-(%sp)			# save character
	bsr.w	L73
	mov.b	(%sp)+,(%a2)+
	bra.b	L9
#
# "d" format
#
L13:
	cmp.b	%d0,&'d				# d format?
	bne.w	L37
	mov.w	SPRNT(%a6),TMP(%a6)		# are we buffering?
	bne.b	L14
	mov.l	%a5,%d0				# end of buffer
	sub.l	%a2,%d0				# room left in buffer
	cmp.l	%d0,&11				# we need at most 11 characters
	shs	TMP(%a6)
	bhs.b	L14
	pea	(%a2)				# save register
	lea	BUFFER(%a6),%a2			# now we've got room
L14:
	mov.l	(%a4)+,%d0			# get number
	bne.b	L15
	mov.b	&'0,(%a2)+			# special case for 0
	bra.w	L49
L15:
	bpl.b	L16				# negative number?
	mov.b	&'-,(%a2)+			# minus sign
	neg.l	%d0				# absolute value
L16:
	mov.l	&10000,%d1			# check for 4 or less digits
	cmp.l	%d0,%d1
	blo.w	L32
	divu	%d1,%d0
	bvc.w	L24
	mov.l	&2000000000,%d1
	cmp.l	%d0,%d1				# need that first digit
	blo.b	L17
	mov.b	&'2,(%a2)+			# first digit is "2"
	sub.l	%d1,%d0
	bra.b	L18
L17:
	lsr.l	&1,%d1				# 1000000000
	cmp.l	%d0,%d1				# check for 1 for first digit
	blo.b	L19
	mov.b	&'1,(%a2)+			# first digit is "1"
	sub.l	%d1,%d0
L18:
	cmp.l	%d0,&655360000			# still too large to split?
	bhs.b	L19
	divu	&10000,%d0			# split it
	lea	L2,%a0				# decimal ASCII character table
	swap	%d0				# save lower digits
	mov.w	%d0,-(%sp)
	clr.w	%d0
	swap	%d0
	bra.b	L25
L19:
	mov.l	&800000000,%d1			# binary search to find digit
	cmp.l	%d0,%d1				# must be 6,7,8 or 9
	blo.b	L21				# branch if 6 or 7
	sub.l	%d1,%d0				# remove 800000000
	mov.l	&100000000,%d1			# now check for 8 or 9?
	cmp.l	%d0,%d1
	blo.b	L20
	sub.l	%d1,%d0				# must be 9
	mov.b	&'9,(%a2)+			# put ASCII character
	bra.b	L23
L20:	mov.b	&'8,(%a2)+			# must be 8
	bra.b	L23
L21:
	mov.l	&700000000,%d1			# check for 6 or 7
	cmp.l	%d0,%d1				# which is it?
	blo.b	L22
	sub.l	%d1,%d0				# must be 7
	mov.b	&'7,(%a2)+			# put ASCII character
	bra.b	L23
L22:
	sub.l	&600000000,%d0			# must be 6
	mov.b	&'6,(%a2)+
L23:
	divu	&10000,%d0			# now split number into halves
	lea	L2,%a0				# decimal ASCII character table
	swap	%d0				# save lower digits
	mov.w	%d0,-(%sp)
	clr.w	%d0
	swap	%d0
	divu	&100,%d0			# two pairs of upper digits
	bra.b	L27
L24:
	lea	L2,%a0				# decimal ASCII character table
	swap	%d0				# save lower digits
	mov.w	%d0,-(%sp)
	clr.w	%d0
	swap	%d0
	cmp.w	%d0,&10000			# do we have five upper digits?
	blo.b	L26
L25:
	divu	&10000,%d0			# get upper digit
	add.w	&'0,%d0				# convert to ASCII
	mov.b	%d0,(%a2)+			# and stash it away
	clr.w	%d0
	swap	%d0				# four upper digits left
	divu	&100,%d0			# split into two pairs
	bra.b	L27
L26:
	cmp.w	%d0,&100			# less than five upper digits
	blo.b	L29				# do we have three or four?
	divu	&100,%d0			# yes: lop off lower two digits
	cmp.w	%d0,&10				# were there three or four?
	bhs.b	L27
	add.w	&'0,%d0				# must have been three
	mov.b	%d0,(%a2)+			# convert to ASCII
	bra.b	L28				# and stash it
L27:
	mov.b	0(%a0,%d0),(%a2)+		# four upper digits
	mov.b	L3-L2(%a0,%d0),(%a2)+		# move two ASCII digits
L28:
	swap	%d0				# get lower two of upper digits
	mov.b	0(%a0,%d0),(%a2)+		# two ASCII characters
	mov.b	L3-L2(%a0,%d0),(%a2)+
	bra.b	L31
L29:
	cmp.w	%d0,&10				# one or two upper digits
	bhs.b	L30				# which is it?
	add.w	&'0,%d0				# one: convert to ASCII
	mov.b	%d0,(%a2)+			# stuff it away
	bra.b	L31
L30:
	mov.b	0(%a0,%d0),(%a2)+		# two upper digits
	mov.b	L3-L2(%a0,%d0),(%a2)+ 		# store ASCII characters
L31:
	movq	&0,%d0				# now handle lower four digits
	mov.w	(%sp)+,%d0			# get them
	divu	&100,%d0			# split into two pairs
	mov.b	0(%a0,%d0),(%a2)+		# convert first pair to ASCII
	mov.b	L3-L2(%a0,%d0),(%a2)+
	swap	%d0				# get second pair
	mov.b	0(%a0,%d0),(%a2)+		# convert to ASCII
	mov.b	L3-L2(%a0,%d0),(%a2)+
	bra.w	L49				# done
L32:
	cmp.w	%d0,&100			# check for number of digits
	blo.b	L35				# do we have three or four?
	lea	L2,%a0				# decimal ASCII character table
	divu	&100,%d0			# lop off lower two digits
	cmp.w	%d0,&10				# were there three or four?
	bhs.b	L33
	add.w	&'0,%d0				# must have been three
	mov.b	%d0,(%a2)+			# convert to ASCII
	bra.b	L34				# and stash it
L33:
	mov.b	0(%a0,%d0),(%a2)+		# four lower digits
	mov.b	L3-L2(%a0,%d0),(%a2)+		# move two ASCII digits
L34:
	swap	%d0				# get lower two of upper digits
	mov.b	0(%a0,%d0),(%a2)+		# two ASCII characters
	mov.b	L3-L2(%a0,%d0),(%a2)+
	bra.w	L49
L35:
	cmp.w	%d0,&10				# one or two lower digits
	bhs.b	L36				# which is it?
	add.w	&'0,%d0				# one: convert to ASCII
	mov.b	%d0,(%a2)+			# stuff it away
	bra.w	L49
L36:
	lea	L2,%a0				# decimal ASCII character table
	mov.b	0(%a0,%d0),(%a2)+		# two lower digits
	mov.b	L3-L2(%a0,%d0),(%a2)+ 		# store ASCII characters
	bra.w	L49
#
# "x" format
#
L37:
	cmp.b	%d0,&'x				# x format?
	bne.w	L55
	mov.w	SPRNT(%a6),TMP(%a6)		# are we buffering?
	bne.b	L38
	mov.l	%a5,%d0				# end of buffer
	sub.l	%a2,%d0				# room left in buffer
	subq.l	&8,%d0				# we need exactly 8 characters
	shs	TMP(%a6)
	bhs.b	L38
	pea	(%a2)				# save register
	lea	BUFFER(%a6),%a2			# now we've got room
L38:
	lea	L1,%a0
	mov.b	(%a4)+,%d1			# get first byte of arg
	beq.b	L39
	cmp.b	%d1,&0x10			# 7 or 8 hex digits?
	bhs.b	L42
	lea	L4,%a1				# 7 hex digits
	mov.b	0(%a1,%d1),(%a2)+		# convert to ASCII
	bra.b	L43
L39:
	mov.b	(%a4)+,%d1			# get second byte of arg
	beq.b	L40
	cmp.b	%d1,&0x10			# 5 or 6 hex digits?
	bhs.b	L44
	lea	L4,%a1				# 5 hex digits
	mov.b	0(%a1,%d1),(%a2)+		# convert to ASCII
	bra.b	L45
L40:
	mov.b	(%a4)+,%d1			# get third byte of arg
	beq.b	L41
	cmp.b	%d1,&0x10			# 3 or 4 hex digits?
	bhs.b	L46
	lea	L4,%a1				# 3 hex digits
	mov.b	0(%a1,%d1),(%a2)+		# convert to ASCII
	bra.b	L47
L41:
	mov.b	(%a4)+,%d1			# get fourth byte of arg
	cmp.b	%d1,&0x10			# 1 or 2 digits?
	bhs.b	L48
	lea	L4,%a1				# 1 digit
	mov.b	0(%a1,%d1),(%a2)+		# convert to ASCII
	bra.b	L49
L42:
	add.w	%d1,%d1
	mov.b	0(%a0,%d1),(%a2)+		# nibble 7
	mov.b	1(%a0,%d1),(%a2)+		# nibble 6
	movq	&0,%d1
L43:
	mov.b	(%a4)+,%d1			# get second byte of arg
L44:
	add.w	%d1,%d1
	mov.b	0(%a0,%d1),(%a2)+		# nibble 5
	mov.b	1(%a0,%d1),(%a2)+		# nibble 4
	movq	&0,%d1
L45:
	mov.b	(%a4)+,%d1			# get third byte of arg
L46:
	add.w	%d1,%d1
	mov.b	0(%a0,%d1),(%a2)+		# nibble 3
	mov.b	1(%a0,%d1),(%a2)+		# nibble 2
	movq	&0,%d1
L47:
	mov.b	(%a4)+,%d1			# get fourth byte of arg
L48:
	add.w	%d1,%d1
	mov.b	0(%a0,%d1),(%a2)+		# nibble 1
	mov.b	1(%a0,%d1),(%a2)+		# nibble 0
#
# "d" or "x" formatting may have been done in temporary buffer
#
L49:
	tst.w	TMP(%a6)			# result in temp buffer?
	bne.w	L9
	mov.l	%a2,%d0				# end of string
	lea	BUFFER(%a6),%a0			# start of temp buffer
	sub.l	%a0,%d0				# number of characters
	mov.l	%a5,%d1				# end of real buffer
	mov.l	(%sp)+,%a2			# real buffer pointer
	sub.l	%a2,%d1				# room in buffer
	beq.b	L52
L50:
	cmp.l	%d1,%d0				# is there room now?
	bge.b	L53
	sub.w	%d1,%d0				# remainder after moving
	subq.w	&1,%d1				# for counting
L51:
	mov.b	(%a0)+,(%a2)+			# move result
	dbra	%d1,L51
L52:
	movm.l	%d0/%a0,-(%sp)			# save count, char ptr
	bsr.w	L73
	movm.l	(%sp)+,%d0/%a0			# restore count, char ptr
	mov.l	%a5,%d1				# end of real buffer
	sub.l	%a2,%d1				# room in buffer
	bra.b	L50
L53:
	subq.w	&1,%d0				# for counting
L54:
	mov.b	(%a0)+,(%a2)+			# move result
	dbra	%d0,L54
	bra.w	L9
#
# "s" format
# We  automatically  assume  this it is "s" format  since it  couldn't  be
# anything else.  Strings for sprnt are handled  differently than for prnt
# and fprnt  because we don't have to worry  about  overflowing  a buffer.
# For prnt and  fprnt,  there are two  paths  depending  on the  amount of
# remaining space in the buffer.  This is done primarily because there are
# faster  instructions  if the size is less than 2^16.  (I can't imagine a
# buffer being bigger than this, but the code is here to handle it just in
# case.)
#
L55:
	cmp.b	%d0,&'s
	bne.w	L100
	mov.l	(%a4)+,%a0			# string pointer
	tst.w	SPRNT(%a6)			# is this "sprnt"?
	beq.b	L57
L56:
	mov.b	(%a0)+,(%a2)+			# move string
	bne.b	L56
	subq.w	&1,%a2				# delete null terminator
	bra.w	L10
L57:
	mov.l	%a5,%d0				# end of buffer
	sub.l	%a2,%d0				# room left in buffer
	beq.b	L59
	subq.l	&1,%d0				# for counting
	mov.l	%d0,%d1
	clr.w	%d1
	tst.l	%d1				# less than 2^16? (should be)
	bne.b	L62
L58:
	mov.b	(%a0)+,(%a2)+			# move string
	dbeq	%d0,L58
	beq.b	L60
L59:
	tst.b	(%a0)				# just at end of string?
	beq.w	L9
	pea	(%a0)				# save character ptr
	bsr.w	L73				# flush the buffer
	mov.l	(%sp)+,%a0			# restore character ptr
	bra.b	L57				# and keep going
L60:
	subq.w	&1,%a2				# unget null
	bra.w	L9
L61:
	mov.l	%a5,%d0				# end of buffer
	sub.l	%a2,%d0				# room left in buffer
	subq.l	&1,%d0				# for counting
L62:
	mov.b	(%a0)+,(%a2)+			# move string
	beq.b	L60
	subq.l	&1,%d0				# one more character
	bcc.b	L62
	tst.b	(%a0)				# just at end of string?
	beq.w	L9
	pea	(%a0)				# save character ptr
	bsr.w	L73				# flush the buffer
	mov.l	(%sp)+,%a0			# restore character ptr
	bra.b	L61				# and keep going
#
# final cleanup
#
L63:
	tst.w	SPRNT(%a6)			# was this an sprnt?
	beq.b	L64
	clr.b	(%a2)				# null terminate the string
	mov.l	%a2,%d0				# end of string
	sub.l	8(%a6),%d0			# length of string
	movm.l	REGS(%a6),%a2-%a5		# restore registers
	unlk	%a6
	rts
L64:
	mov.l	IOP(%a6),%a0			# file
	mov.l	%a2,%d0				# buffer pointer
	sub.l	_ptr(%a0),%d0			# characters added
	sub.l	%d0,_cnt(%a0)			# update iop->_cnt
	mov.l	%a2,_ptr(%a0)			# update iop->_ptr
	mov.l	%a2,%d0				# buffer pointer
	add.l	_cnt(%a0),%d0
	cmp.l	%d0,%a5				# in case of interrupt
	bls.b	L65
	pea	(%a0)
	jsr	__bufsync
	addq.w	&4,%sp
L65:
	mov.l	IOP(%a6),%a0			# file
	mov.l	_ptr(%a0),%d0			# number of buffer characters
	mov.l	_base(%a0),%a1
	sub.l	%a1,%d0
	movq	&_IONBF,%d1			# unbuffered?
	and.w	_flag(%a0),%d1
	bne.b	L69
	mov.w	&_IOLBF,%d1			# line buffered?
	and.w	_flag(%a0),%d1
	beq.b	L70
	subq.l	&1,%d0				# counter
	movq	&0,%d1
	mov.w	%d0,%d1
	cmp.l	%d0,%d1				# less than 2^16?
	beq.b	L67
	movq	&'\n,%d1			# linefeed
L66:
	cmp.b	%d1,(%a1)+			# search for linefeeds
	beq.b	L69
	subq.l	&1,%d0
	bcc.b	L66
	bra.b	L70
L67:
	movq	&'\n,%d1			# linefeed
L68:
	cmp.b	%d1,(%a1)+			# search for linefeeds
	dbeq	%d0,L68
	bne.b	L70
L69:
	bsr.w	L73				# flush it
	bra.b	L71
L70:
	mov.l	%a2,%d0				# buffer pointer
	sub.l	START(%a6),%d0			# new chars
	add.l	%d0,COUNT(%a6)			# update count
L71:
	mov.l	IOP(%a6),%a0			# file
	mov.l	COUNT(%a6),%d0			# number of characters
	movq	&_IOERR,%d1
	and.w	_flag(%a0),%d1			# error?
	beq.b	L72
	movq	&EOF,%d0			# return EOF
L72:
	movm.l	REGS(%a6),%a2-%a5		# restore registers
	unlk	%a6
	rts
#
# flush the buffer
# values in iop are updated as necessary
#
L73:
	mov.l	%a2,%d0				# buffer pointer
	sub.l	START(%a6),%d0			# new chars being flushed
	add.l	%d0,COUNT(%a6)			# update count
	mov.l	IOP(%a6),%a0			# file
	mov.l	%a2,%d0				# last of characters
	mov.l	_base(%a0),%d1			# iop->_base
	mov.l	%d1,_ptr(%a0)			# reset iop->_ptr
	mov.l	%d1,%a2
	mov.l	%a2,START(%a6)
	sub.l	%d1,%d0				# number of characters
	mov.l	CNT(%a6),_cnt(%a0)		# update iop->_cnt
	mov.l	%d0,-(%sp)			# number of characters
	mov.l	%d0,-(%sp)
	mov.l	%d1,-(%sp)			# character pointer
	mov.l	FILENO(%a6),-(%sp)		# file number
	jsr	_write
	lea	12(%sp),%sp
	cmp.l	%d0,(%sp)+			# everything written?
	beq.b	L74
	mov.l	IOP(%a6),%a0			# file
	movq	&_IOERR,%d0
	or.w	%d0,_flag(%a0)			# indicate error
L74:
	rts
L100:
	trap	&8				# Because Mark said so!

