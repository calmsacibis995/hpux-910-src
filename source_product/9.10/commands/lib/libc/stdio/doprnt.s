# @(#) $Revision: 70.11 $     
#
# __doprnt
# 
# 680XX assembly language version
#
# Much of the general structure of this routine is the same as
# that of the C version. Individual formatting algorithms may
# differ greatly, though, in order to exploit the capabilities
# of the processor(s). The primary goal in writing this version
# was performance.
#
# Author: Mark McDowell
#	  August 1986
#
#         Modified June, 1991 by Rick Ferreri.  Made compatible with
#	  8.0 C version of _doprnt() (except NLS support.  If using
#	  multi-byte characters, then just call C version (___doprnt_68k))
#

	version 2

	#*****************************************
	# Beware of the following values changing!
	# They all come from <stdio.h>.		

	set	_IONBF,00004
	set	_IOERR,00040
	set	_IOLBF,00200			# declared as signed byte
	set	_IOEXT,01000
	set	_IODUMMY,02000
	set	_NFILE,60
	set	_cnt,0
	set	_ptr,4
	set	_base,8
	set	_flag,12
	set	_fileL,14
	set	_fileH,15
	set	_bufendp,16
	#****************************************


	set	EOF,-1				# end of file
	set	EOL,'\n				# end of line

	#****************************************
	# Default float and integer precisions
	#

	set	DEF_INT_PREC,1
	set	DEF_FLOAT_PREC,6

	#****************************************
	# Useful constants for floating point formats.
	# These values should reflect the contents of 
	# "print.h".
	#

	# Maximum total number of digits in E format
	#

	set	MAXECVT,17
	set	MAXQECVT,34

	# Maximum total number of digits in f format
	#
	set	MAXFCVT,60
	set	MAXFSIG,17
	set	MAXQFSIG,34
	
	#
	# FLAGS - These will be stored in register %d7.
	# The numbering and ordering of these are important so do not
	# change them without understanding the effect it will have!
	#

	set	FBLANK,(1<0)			# blank flag seen
	set	FPLUS,(1<1)			# plus flag seen
	set	FMINUS,(1<2)			# minus flag seen
	set	FDOT,(1<4)			# dot seen
	set	FHLENGTH,(1<5)			# half-length flag seen
	set	FPADZERO,(1<6)			# pad with zeroes
	set	FSHARP,(1<7)			# sharp flag seen
	set	FEXTEXP,(1<8)			# 4 char exponent
	set	FSUFFIX,(1<9)			# formatted string has exponent
	set	FDZERO,(1<10)			# 0's after number
	set	FRDP,(1<11)			# dec pt after number
	set	FCZERO,(1<12)			# 0's between number & dec pt
	set	FBZERO,(1<13)			# 0's between dec pt & number
	set	FLDP,(1<14)			# dec pt before number
	set	FAZERO,(1<15)			# 0's before number
# begin long double changes
	set	FLDOUBLE,(1<16)			# long double flag seen (L)
# end long double changes

	set	PREFIX_NULL,0			# zero means no prefix
	set	PREFIX_PLUS,'+			# single character
	set	PREFIX_MINUS,'-			#    prefix is positive
	set	PREFIX_BLANK,'\040
	set	PREFIX_0x,-'x			# two character
	set	PREFIX_0X,-'X			# prefix is negative

	set	FORMAT,8			# pointer to FORMAT parameter
	set	IOP,16				# pointer to IOP parameter

						# local variables:
	set	REGS,-40			# save area for registers
						# zero counts:
	set	AZERO,-44			#    before number
	set	BZERO,-48			#    between number and dec pt
	set	CZERO,-52			#    between dec pt and number
	set	DZERO,-56			#    after number
	set	BUFEND,-60			# ptr to end of output buffer
	set	SPRINTF,-61			# flag indicating sprintf
	set	SUFFIXBUF,-68			# buffer for exponent
	set	BUFFER,-140			# general purpose buffer
	set	COUNT,-144			# total output char count
	set	START,-148			# where buffering starts
						# (since invoking or last flush)
	set	SIGN,-152			# for sign on _ecvt call
	set	DECPT,-156			# for dec pt on _ecvt call
	set	RADIX,-158			# radix character
	set	EXP_3_4,-160			# 0 if 3 digit exp
						# 1 if 4 digit exp
						# ONLY used if FEXTEXP is set
	set	LOCALSIZE,-160			# total amt of local storage

	data
	#
	# hex conversion tables
	#								
__L20:
	byte	'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0,'0
	byte	'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1,'1
	byte	'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2,'2
	byte	'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3,'3
	byte	'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4,'4
	byte	'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5,'5
	byte	'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6,'6
	byte	'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7,'7
	byte	'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8,'8
	byte	'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9,'9
	byte	'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A,'A
	byte	'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B,'B
	byte	'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C,'C
	byte	'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D,'D
	byte	'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E,'E
	byte	'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F,'F

__L30:
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9,'A,'B,'C,'D,'E,'F

	#
	# decimal conversion tables
	#								
__L40:
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

__L50:
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9
	byte	'0,'1,'2,'3,'4,'5,'6,'7,'8,'9


#
# These are the primary register definitions:
#
#	 %d2 = length of main part of formatted string
#	 %d3 = prefix (e.g. '+','-','0x',etc.)
#	 %d4 = total length of formatted string (minus padding)
#	 %d5 = field width
#	 %d6 = precision
#	 %d7 = flags register
#	 %a2 = destination buffer pointer
#	 %a3 = source format pointer
#	 %a4 = source arguments pointer
#	 %a5 = destination buffer end pointer 
#	       OR pointer to main part of formatted string
#
# The routine can be divided into three main parts:
#    (1) scanning for a format character
#	 registers %a2-%a5 are defined as above (%a5 is buffer end pointer);
#	 values for registers %d5-%d7 are being determined
#    (2) processing the format character
#	 values for registers %d2-%d4 are being determined as well as setting 
#	 %a5 to point to the formatted string; %d6 will be trashed
#    (3) outputting the result
#	 
#

	text
	global	__doprnt
__doprnt:
ifdef(`PROFILE',`
  ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a1
	mov.l	p__doprnt(%a1),%a0
	bsr.l	mcount',`
	mov.l	&p__doprnt,%a0
	jsr	mcount
  ')
')
	link.l	%a6,&LOCALSIZE			# get local space
	movm.l	%d2-%d7/%a2-%a5,REGS(%a6)	# save registers
	movm.l	8(%a6),%a3-%a5			# get parameters
# Check to see if using multi-byte chars (__nl_char_size == 1?).  If so, 
# then call the C language version of doprnt() for the 68k (__doprnt_68k()).
ifdef(`PIC',`
	bsr.l	LPROLOG1
	mov.l   ___nl_char_size(%a1),%a0
	mov.l	(%a0),%d0',`
	mov.l	___nl_char_size,%d0
')
	cmpi	%d0,&1
	beq	OK
	mov.l	%a5,-(%sp)
        mov.l   %a4,-(%sp)                      # push current addr in arg list
        mov.l   %a3,-(%sp)                      # push address of format
	bsr.l	___doprnt_68k
	add	&12,%sp				# clean off stack
	bra.w	L2170
# Otherwise, continue setting things up
OK:
#  Store _nl_radix character in RADIX
#  %a1 should still contain logical address of DLT
ifdef(`NLS',`
  ifdef(`PIC',`
	mov.l	__nl_radix(%a1),%a0
	mov.b	3(%a0),RADIX(%a6)
  ',`
	mov.b	__nl_radix+3,RADIX(%a6)
  ')
',`
	mov.b	&0x2E,RADIX(%a6)
')
	mov.l	_ptr(%a5),%a2			# buffer pointer
	movq	&0,%d0
	mov.l	%d0,COUNT(%a6)			# initialize char count
	ori.w	&_IODUMMY,%d0			# is "dummy" iop?
	and.w	_flag(%a5),%d0			# printing to string?
	cmp.w	%d0,&_IODUMMY
	seq	%d1				# 'sprintf' flag
	beq.w	L60
	movq	&0,%d0
	ori.w	&_IOEXT,%d0			# is this a _FILEX struct
	and.w	_flag(%a5),%d0
	cmp.w	%d0,&_IOEXT
	beq.w   L55
	mov.b	_fileL(%a5),%d0			# unbuffered or line buffered?
	lsl.w	&2,%d0				# for array indexing
# %a1 should still contain logical address of DLT
# __bufendtab[iop->_fileL]
ifdef(`PIC',`
	mov.l	___bufendtab(%a1),%a0
	add.l	%a0,%d0',`
	add.l	&___bufendtab,%d0
')
	mov.l	%d0,%a0
	mov.l	(%a0),%a5			# end of buffer
	bra	L65

L55:
	mov.l	_bufendp(%a5),%a5		# end of buffer
	bra	L65

L60:
	movq	&-1,%d1				# assume this is 'sprintf'
	mov.l	%d1,%a5

L65:
	mov.b	%d1,SPRINTF(%a6)		# flag if this is 'sprintf'
	mov.l	%a2,START(%a6)			# remember where we started
	mov.l	%a5,BUFEND(%a6)			#   and the end of the buffer

#
# THIS IS THE TOP OF THE MAIN LOOP
#

L70:
	movq	&0,%d2				# initialize
	movq	&'%,%d3				# for searching for formats
L80:
	mov.b	(%a3)+,%d2			# get format character
	beq.w	L2100				# end of format?
	cmp.b	%d2,%d3				# is this a '%'?
	bne.b	L90				# if not, copy it to buffer
	mov.b	(%a3)+,%d2			# get next character
	beq.w	L2100				# end of format?
	cmp.b	%d2,&'\040 			# check valid character range
	bcs.b	L90
	cmp.b	%d2,&'x				# must be >= ' ' and <= 'x'
	bhi.b	L90
	movq	&0,%d5				# width = 0
	movq	&1,%d6				# prec = 1 (default int prec)
	movq	&0,%d7				# flagword = 0
	bra.l	L2270
L90:
	cmp.l	%a2,%a5				# more room in buffer?
	bcs.b	L100
	bsr.w	L2230				# flush buffer (resets %a2)
L100:
	mov.b	%d2,(%a2)+			# move character to result
	bra.b	L80

#
# Scanning the format string repeatedly comes back to this point
# until the format letter is encountered.
#

L110:
	mov.b	(%a3)+,%d2			# get format character
	beq.w	L2100				# end of format?
	cmp.b	%d2,&'\040 			# check valid character range
	bcs.b	L90
	cmp.b	%d2,&'x				# must be >= ' ' and <= 'x'
	bhi.b	L90
	bra.l	L2270
	#
	# '+' flag found (sign)
	#								
L120:
	or.w	&FPLUS,%d7			# flagword |= FPLUS
	bra.b	L110
	#
	# '-' flag found (left justify)
	#								
L130:
	or.w	&FMINUS,%d7			# flagword |= FMINUS
	and.w	&~FPADZERO,%d7			# flagword &= ~FPADZERO
	bra.b	L110
	#
	# ' ' flag found (sign)
	#								
L140:
	or.w	&FBLANK,%d7			# flagword |= FBLANK
	bra.b	L110
	#
	# '#' flag found (special format handling)
	#								
L150:
	or.w	&FSHARP,%d7			# flagword |= FSHARP
	bra.b	L110
	#
	# '.' found (precision may follow)
	#								
L160:
	or.w	&FDOT,%d7			# flagword |= FDOT
	movq	&0,%d6				# prec = 0
	bra.b	L110
	#
	# 'h' found (long precision)
	#								
L170:
	or.w	&FHLENGTH,%d7			# flagword |= FHLENGTH
	bra.b	L110
	#
	# 'L' found (long double)
	#
L175:
	or.l	&FLDOUBLE,%d7			# flagword |= FLDOUBLE
	bra.b L110
	#
	# '*' found (dynamic width or precision)
	#								
L180:
	movq	&FDOT,%d0
	and.b	%d7,%d0				# if (!(flagword & FDOT))
	bne.b	L190
	mov.l	(%a4)+,%d5			# width
	bpl.b	L110
	neg.l	%d5				# width = -width
	eor.w	&FMINUS,%d7			# flagword ^= FMINUS
	bra.b	L110
L190:
	mov.l	(%a4)+,%d6			# prec
	bpl.b	L110				# if (prec < 0)
	movq	&-1,%d6				# prec = -1 ==> use default
	bra.b	L110
	#
	# '0' found (pad with zeroes)
	#
L200:
	movq	&FDOT|FMINUS,%d0		# if (!(flagword & (FDOT |
	and.b	%d7,%d0				# FMINUS)))
	bne.b	L110
	or.w	&FPADZERO,%d7			# flagword |= FPADZERO
	bra.b	L110
	#
	# digit found (static width or precision)
	#								
L210:
	mov.l	%a3,%a0				# copy current address into a0
	movq	&'0,%d0				# constant '0'
	movq	&9,%d1				# constant 9
	sub.b	%d0,%d2				# convert to decimal
	mov.l	%d2,%d4				# to accumulate decimal value
L220:
	mov.b	(%a3)+,%d2			# get character
	beq.w	L2100				# end of format?
	sub.b	%d0,%d2				# digit?
	bcs.b	L221
	cmp.b	%d2,%d1
	bhi.b	L221
	add.l	%d4,%d4				# result * 2
	mov.l	%d4,%a1				# save it
	lsl.l	&2,%d4				# result * 8
	add.l	%a1,%d4				# result * 10
	add.l	%d2,%d4				# accumulate result
	bra.b	L220
L221:
# Check if we have a positional parameter
	mov.b	-(%a3),%d2			# back up ptr and get char
	movq	&'$,%d1				# is the char a '$' ?
	cmp.b	%d1,%d2
	bne	L230
# To get here and to successfully call _printmsg(), we must ensure that no
# other formatting characters have been encountered at this point.  If this
# is true, then we should be able to start out with a pointer to the
# original format string, read past characters until we hit a '%', read
# past all ASCII digits and see a '$'.  The address at this point should be
# the same as the current format (%a3) pointer.
        movq    &2,%d1				# 1 for % and 1 for 1st digit
        suba.l  %d1,%a0
# %a0 now contains the address of the '%' character.
# start a1 at beginning of format string passed in
	mov.l	FORMAT(%a6),%a1
# blow past characters until we hit a '%'
	movq	&'%,%d0
L222:
	cmp.b	%d0,(%a1)+
	bne.b	L222
	movq	&1,%d1
	suba.l	%d1,%a1
	cmp.l	%a0,%a1				# if same address, then ok
						# to flush buffer and call
						# C version of doprnt.
	beq.b	L227
	bra.b	L230
L227:
	mov.l	FORMAT(%a6),%a5			# assume sprintf
	tst.b	SPRINTF(%a6)			# is this 'sprintf'?
	bne.b	L228				# don't flush if sprintf
	mov.l	%a0,%a5				# bsr hoses a0, copy to a5
	bsr.w	L2230				# flush buffer (resets %a2)
L228:
	mov.l	IOP(%a6),-(%sp)
        mov.l   %a4,-(%sp)                      # push current addr in arg list
        mov.l   %a5,-(%sp)                      # push address of format
	bsr.l	___doprnt_68k
	add	&12,%sp				# clean off stack
	add.l	COUNT(%a6),%d0			# add chars already flushed
	bra.w	L2170
L230:
	movq	&FDOT,%d0			# if (flagword & FDOT)
	and.b	%d7,%d0
	beq.b	L240
	mov.l	%d4,%d6				# prec = num
	bra.w	L110
L240:
	mov.l	%d4,%d5				# width = num
	bra.w	L110

#
# 'n' format
#

LN10:
	mov.l	(%a4)+,%a1			# pointer to int
	mov.l	%a2,%d0				# current position in buffer
	sub.l	START(%a6),%d0			# number of characters to flush
	add.l	COUNT(%a6),%d0			# add chars already flushed
	movq	&FHLENGTH,%d1			# if (flagword & FHLENGTH)
	and.w	%d7,%d1
	beq.b	LN30
	mov.w	%d0,(%a1)			# copy as long int
	bra.w	L80
LN30:
	mov.l	%d0,(%a1)			# copy as long int
	bra.w	L80

#
# 'd' and 'i' format
#

L250:
	movq	&FDOT,%d1			# if (flagword & FDOT)
	and.b	%d7,%d1
	beq.b	L255
	and.w	&~FPADZERO,%d7			# flagword &= ~FPADZERO
L255:
	movq	&PREFIX_NULL,%d3		# assume null prefix
	movq	&0,%d4				# total width = 0
	lea	BUFFER(%a6),%a5			# start of buffer
	mov.l	%a5,%a1
	mov.l	(%a4)+,%d0			# get number
	bpl.b	L260				# negative number?
	neg.l	%d0				# absolute value
	movq	&PREFIX_MINUS,%d3		# prefix result with '-'
	movq	&1,%d4				# width now has minus sign
	bra.b	L270
L260:
	movq	&FPLUS|FBLANK,%d1		# if (flagword & (FPLUS|FBLANK))
	and.b	%d7,%d1
	beq.b	L270
	movq	&1,%d4				# increment width
	movq	&PREFIX_PLUS,%d3		# assume '+'
	subq.b	&FPLUS,%d1			# is it to be plus?
	bpl.b	L270
	movq	&PREFIX_BLANK,%d3		# prefix result with ' '
L270:
	tst.l	%d0
	bne.b	L280				# special case for 0
	tst.l	%d6				# if (prec == 0)
	beq.w	L490
	mov.b	&'0,(%a1)+
	bra.w	L490
L280:
ifdef(`PIC',`
	bsr.l	LPROLOG0
	mov.l	__L40(%a0),%a0',`		# decimal ASCII character table
	lea	__L40,%a0			# decimal ASCII character table
')
	mov.l	&10000,%d1			# check for 4 or less digits
	cmp.l	%d0,%d1
	bcs.w	L440
	divu	%d1,%d0
	bvc.w	L360
	mov.l	&2000000000,%d1
	cmp.l	%d0,%d1				# need that first digit
	bcs.b	L290
	mov.b	&'2,(%a1)+			# first digit is '2'
	sub.l	%d1,%d0
	bra.b	L300
L290:
	lsr.l	&1,%d1				# 1000000000
	cmp.l	%d0,%d1				# check for 1 for first digit
	bcs.b	L310
	mov.b	&'1,(%a1)+			# first digit is '1'
	sub.l	%d1,%d0
L300:
ifdef(`PIC',`
	bsr.l	LPROLOG0
	mov.l	__L40(%a0),%a0',`		# decimal ASCII character table
	lea	__L40,%a0			# decimal ASCII character table
')
	cmp.l	%d0,&655360000			# still too large to split?
	bhs.b	L310
	divu	&10000,%d0			# split it
	swap	%d0				# save lower digits
	mov.w	%d0,%d2
	clr.w	%d0
	swap	%d0
	bra.b	L370
L310:
	mov.l	&800000000,%d1			# binary search to find digit
	cmp.l	%d0,%d1				# must be 6,7,8 or 9
	bcs.b	L330				# branch if 6 or 7
	sub.l	%d1,%d0				# remove 800000000
	mov.l	&100000000,%d1			# now check for 8 or 9?
	cmp.l	%d0,%d1
	bcs.b	L320
	sub.l	%d1,%d0				# must be 9
	mov.b	&'9,(%a1)+			# put ASCII character
	bra.b	L350
L320:	
	mov.b	&'8,(%a1)+			# must be 8
	bra.b	L350
L330:
	mov.l	&700000000,%d1			# check for 6 or 7
	cmp.l	%d0,%d1				# which is it?
	bcs.b	L340
	sub.l	%d1,%d0				# must be 7
	mov.b	&'7,(%a1)+			# put ASCII character
	bra.b	L350
L340:
	sub.l	&600000000,%d0			# must be 6
	mov.b	&'6,(%a1)+
L350:
	divu	&10000,%d0			# now split number into halves
	swap	%d0				# save lower digits
	mov.w	%d0,%d2
	clr.w	%d0
	swap	%d0
	divu	&100,%d0			# two pairs of upper digits
	bra.b	L390
L360:
	swap	%d0				# save lower digits
	mov.w	%d0,%d2
	clr.w	%d0
	swap	%d0
	cmp.w	%d0,&10000			# do we have five upper digits?
	bcs.b	L380
L370:
	divu	&10000,%d0			# get upper digit
	add.w	&'0,%d0				# convert to ASCII
	mov.b	%d0,(%a1)+			# and stash it away
	clr.w	%d0
	swap	%d0				# four upper digits left
	divu	&100,%d0			# split into two pairs
	bra.b	L390
L380:
	cmp.w	%d0,&100			# less than five upper digits
	bcs.b	L410				# do we have three or four?
	divu	&100,%d0			# yes: lop off lower two digits
	cmp.w	%d0,&10				# were there three or four?
	bhs.b	L390
	add.w	&'0,%d0				# must have been three
	mov.b	%d0,(%a1)+			# convert to ASCII
	bra.b	L400				# and stash it
L390:
	mov.b	0(%a0,%d0),(%a1)+		# four upper digits
	mov.b	__L50-__L40(%a0,%d0),(%a1)+	# move two ASCII digits
L400:
	swap	%d0				# get lower two of upper digits
	mov.b	0(%a0,%d0),(%a1)+		# two ASCII characters
	mov.b	__L50-__L40(%a0,%d0),(%a1)+
	bra.b	L430
L410:
	cmp.w	%d0,&10				# one or two upper digits
	bhs.b	L420				# which is it?
	add.w	&'0,%d0				# one: convert to ASCII
	mov.b	%d0,(%a1)+			# stuff it away
	bra.b	L430
L420:
	mov.b	0(%a0,%d0),(%a1)+		# two upper digits
	mov.b	__L50-__L40(%a0,%d0),(%a1)+ 	# store ASCII characters
L430:
	divu	&100,%d2			# split lower four digits
	mov.b	0(%a0,%d2),(%a1)+		# convert first pair to ASCII
	mov.b	__L50-__L40(%a0,%d2),(%a1)+
	swap	%d2				# get second pair
	mov.b	0(%a0,%d2),(%a1)+		# convert to ASCII
	mov.b	__L50-__L40(%a0,%d2),(%a1)+
	bra.w	L490				# done
L440:
	cmp.w	%d0,&100			# check for number of digits
	bcs.b	L470				# do we have three or four?
	divu	&100,%d0			# lop off lower two digits
	cmp.w	%d0,&10				# were there three or four?
	bhs.b	L450
	add.w	&'0,%d0				# must have been three
	mov.b	%d0,(%a1)+			# convert to ASCII
	bra.b	L460				# and stash it
L450:
	mov.b	0(%a0,%d0),(%a1)+		# four lower digits
	mov.b	__L50-__L40(%a0,%d0),(%a1)+	# move two ASCII digits
L460:
	swap	%d0				# get lower two of upper digits
	mov.b	0(%a0,%d0),(%a1)+		# two ASCII characters
	mov.b	__L50-__L40(%a0,%d0),(%a1)+
	bra.b	L490
L470:
	cmp.w	%d0,&10				# one or two lower digits
	bhs.b	L480				# which is it?
	add.w	&'0,%d0				# one: convert to ASCII
	mov.b	%d0,(%a1)+			# stuff it away
	bra.b	L490
L480:
	mov.b	0(%a0,%d0),(%a1)+		# two lower digits
	mov.b	__L50-__L40(%a0,%d0),(%a1)+ 	# store ASCII characters
L490:
	mov.l	%a1,%d2				# end of number
	sub.l	%a5,%d2				# number length
	add.w	%d2,%d4				# actual length += number length
	tst.l	%d6				# prec < 0?
	bpl.b	L491
	movq	&DEF_INT_PREC,%d6		# prec = DEF_INT_PREC
L491:
	sub.l	%d2,%d6				# number of zeroes needed
	bls.w	L1600				
	add.l	%d6,%d4				# actual length += zeroes
	mov.l	%d6,AZERO(%a6)
	or.w	&FAZERO,%d7
	bra.w	L1600

#
# 'u' format
#

L500:
	movq	&FDOT,%d1			# if (flagword & FDOT)
	and.b	%d7,%d1
	beq.b	L505
	and.w	&~FPADZERO,%d7			# flagword &= ~FPADZERO
L505:
	movq	&PREFIX_NULL,%d3		# no prefix
	movq	&0,%d4				# total width = 0
	lea	BUFFER(%a6),%a5			# start of buffer
	mov.l	%a5,%a1
	mov.l	(%a4)+,%d0			# get number
	movq	&FHLENGTH,%d1			# if (flagword & FHLENGTH)
	and.w	%d7,%d1
	beq.b	L510
	and.l	&0xFFFF,%d0			# use only lower bits
	bra.w	L270
L510:
	mov.l	&3000000000,%d1
	cmp.l	%d0,%d1				# can we handle like signed?
	bcs.w	L270
	sub.l	%d1,%d0				# subtract it off
	mov.l	&1000000000,%d1
	cmp.l	%d0,%d1				# is upper digit a '3' or '4'?
	bhs.b	L520
	mov.b	&'3,(%a1)+			# it's a '3'
	bra.w	L300
L520:
	sub.l	%d1,%d0				# subtract top digit
	mov.b	&'4,(%a1)+			# it's a '4'
	bra.w	L300

#
# 'x', 'X' formats
#

L530:
	movq	&FDOT,%d1			# if (flagword & FDOT)
	and.b	%d7,%d1
	beq.b	L535
	and.w	&~FPADZERO,%d7			# flagword &= ~FPADZERO
L535:
	lea	BUFFER(%a6),%a5			# start of buffer
ifdef(`PIC',`
	bsr.l	LPROLOG1
	mov.l	__L20(%a1),%a0			# hex conversion tables
	mov.l	__L30(%a1),%a1',`
	lea	__L20,%a0			# hex conversion tables
	lea	__L30,%a1
')
	movq	&FHLENGTH,%d1			# if (flagword & FHLENGTH)
	and.b	%d7,%d1
	beq.b	L540
	addq.w	&2,%a4				# skip first two bytes
	mov.l	&'0<24|'0<16|'0<8|'0,(%a5)+	# start with four zeroes
	bra.b	L550
L540:
	mov.b	(%a4)+,%d1			# get first byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 7
	mov.b	0(%a1,%d1),(%a5)+		# nibble 6
	mov.b	(%a4)+,%d1			# get second byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 5
	mov.b	0(%a1,%d1),(%a5)+		# nibble 4
L550:
	mov.b	(%a4)+,%d1			# get third byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 3
	mov.b	0(%a1,%d1),(%a5)+		# nibble 2
	mov.b	(%a4)+,%d1			# get fourth byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 1
	mov.b	0(%a1,%d1),(%a5)+		# nibble 0
	mov.l	%a5,%a1				# end of buffer
	subq.l	&8,%a5				# start of buffer
	cmp.b	%d2,&'X				# 'X' or 'x' format?
	beq.b	L560
	mov.l	%a5,%a0				# start of buffer
	mov.l	&'x-'X<24|'x-'X<16|'x-'X<8|'x-'X,%d1
	or.l	%d1,(%a0)+			# convert to lower case
	or.l	%d1,(%a0)			# note: '0'-'9' not affected
L560:
	tst.l	%d6				# prec < 0?
	bpl.b	L561
	movq	&DEF_INT_PREC,%d6		# prec = DEF_INT_PREC
L561:
	mov.l	%d6,%d4				# total length = precision
	movq	&PREFIX_NULL,%d3		# no prefix
	tst.b	%d7				# FSHARP set?
	bpl.b	L570
	movq	&FHLENGTH,%d1			# if (flagword & FHLENGTH)
	and.b	%d7,%d1
	beq.b	L563				# no prefix if zero
	tst.w	-2(%a4)				# is it zero?
	beq.b	L570
	bra.b	L566
L563:
	tst.l	-4(%a4)				# check for a long zero
	beq.b	L570
L566:
	sub.w	%d2,%d3				# set up prefix
	addq.l	&2,%d4				# two characters for prefix
L570:
	movq	&8,%d2				# assume number length
	sub.l	%d2,%d6				# need at least this many?
	beq.w	L1600
	bcs.b	L580
	mov.l	%d6,AZERO(%a6)			# number of extra zeroes
	or.w	&FAZERO,%d7
	bra.w	L1600
L580:
	not.w	%d6				# counter
	movq	&'0,%d1				# search for leading zeroes
L590:
	cmp.b	%d1,(%a5)+			# leading zeroes?
	dbne	%d6,L590
	beq.b	L600
	subq.w	&1,%a5				# wider than precision
	add.w	%d6,%d4				# increment total length
	addq.w	&1,%d4				#   by extra precision
L600:
	sub.l	%a5,%a1				# length of number
	mov.l	%a1,%d2
	bra.w	L1600

#
# 'p', 'P' formats
#
# Similar to 'x' format except:
#    1. "h" flag (long) ignored
#    2. "#" flag recognized for NULL pointer
#    3. All values printed out as a sequence of 2 hex digits for each
#       byte in the pointer.
#

LP530:
	lea	BUFFER(%a6),%a5			# start of buffer
ifdef(`PIC',`
	bsr.l	LPROLOG1
	mov.l	__L20(%a1),%a0			# hex conversion tables
	mov.l	__L30(%a1),%a1',`
	lea	__L20,%a0			# hex conversion tables
	lea	__L30,%a1
')
	mov.b	(%a4)+,%d1			# get first byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 7
	mov.b	0(%a1,%d1),(%a5)+		# nibble 6
	mov.b	(%a4)+,%d1			# get second byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 5
	mov.b	0(%a1,%d1),(%a5)+		# nibble 4
	mov.b	(%a4)+,%d1			# get third byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 3
	mov.b	0(%a1,%d1),(%a5)+		# nibble 2
	mov.b	(%a4)+,%d1			# get fourth byte of arg
	mov.b	0(%a0,%d1),(%a5)+		# nibble 1
	mov.b	0(%a1,%d1),(%a5)+		# nibble 0
	mov.l	%a5,%a1				# end of buffer
	subq.l	&8,%a5				# start of buffer
	cmp.b	%d2,&'P				# 'P' or 'p' format?
	beq.b	LP560
	mov.l	%a5,%a0				# start of buffer
	mov.l	&'x-'X<24|'x-'X<16|'x-'X<8|'x-'X,%d1
	or.l	%d1,(%a0)+			# convert to lower case
	or.l	%d1,(%a0)			# note: '0'-'9' not affected
LP560:
	tst.l	%d6				# prec < 0?
	bpl.b	LP561
	movq	&DEF_INT_PREC,%d6		# prec = DEF_INT_PREC
LP561:
	mov.l	%d6,%d4				# total length = precision
	movq	&PREFIX_NULL,%d3		# no prefix
	tst.b	%d7				# FSHARP set?
	bpl.b	LP570
	addq.l	&2,%d4				# two characters for prefix
	cmp.b	%d2,&'P				# 'P' or 'p' format?
	beq.b	LP562
	movq	&PREFIX_0x,%d3			# set up prefix
	bra.b	LP570
LP562:
	movq	&PREFIX_0X,%d3			# set up prefix
LP570:
	movq	&8,%d2				# assume number length
	sub.l	%d2,%d6				# need at least this many?
	beq.w	L1600
	bcs.b	LP580
	mov.l	%d6,AZERO(%a6)			# number of extra zeroes
	or.w	&FAZERO,%d7
	bra.w	L1600
LP580:
	not.w	%d6				# counter
	movq	&'0,%d1				# search for leading zeroes
LP590:
	cmp.b	%d1,(%a5)+			# leading zeroes?
	dbne	%d6,LP590
	beq.b	LP600
	subq.w	&1,%a5				# wider than precision
	add.w	%d6,%d4				# increment total length
	addq.w	&1,%d4				#   by extra precision
LP600:
	sub.l	%a5,%a1				# length of number
	mov.l	%a1,%d2
	bra.w	L1600

#
# 'o' format
#

L610:
	movq	&FDOT,%d1			# if (flagword & FDOT)
	and.b	%d7,%d1
	beq.b	L615
	and.w	&~FPADZERO,%d7			# flagword &= ~FPADZERO
L615:
	movq	&PREFIX_NULL,%d3		# no prefix
	movq	&'0,%d4				# for ASCII conversion
	lea	BUFFER+2(%a6),%a5		# start of buffer
	mov.l	%a5,%a1
	mov.b	%d4,(%a1)+			# always a leading zero
	movq	&10,%d1				# counter
	mov.l	(%a4)+,%d0			# get argument
	movq	&FHLENGTH,%d2			# if (flagword & FHLENGTH)
	and.b	%d7,%d2
	beq.b	L620
	mov.b	%d4,(%a1)+			# put in leading zeroes
	mov.l	&'0<24|'0<16|'0<8|'0,(%a1)+	# four more
	movq	&5,%d1				# counter
	swap	%d0				# don't need upper bits
	movq	&1,%d2				# one bit at top
	rol.l	&1,%d0				# move it to lower bit
	bra.b	L640
L620:
	movq	&3,%d2				# two bits at top
	rol.l	&2,%d0				# move them to lower bits
	bra.b	L640
L630:
	movq	&07,%d2				# three bits
	rol.l	&3,%d0				# move three bits to bottom
L640:
	and.b	%d0,%d2				# isolate bits
	add.b	%d4,%d2				# convert to ASCII
	mov.b	%d2,(%a1)+			# save in buffer
	dbra	%d1,L630
	tst.l	%d6				# prec < 0?
	bpl.b	L641
	movq	&DEF_INT_PREC,%d6		# prec = DEF_INT_PREC
L641:
	mov.l	%d6,%d4				# total length = precision
	movq	&12,%d2				# assume number length
	sub.l	%d2,%d6				# need at least this many?
	beq.w	L1600
	bcs.b	L650
	mov.l	%d6,AZERO(%a6)			# number of extra zeroes
	or.w	&FAZERO,%d7
	bra.w	L1600
L650:
	not.w	%d6				# counter
	movq	&'0,%d1				# search for leading zeroes
L660:
	cmp.b	%d1,(%a5)+			# leading zeroes?
	dbne	%d6,L660
	beq.b	L670
	subq.w	&1,%a5				# wider than precision
L670:
	tst.b	%d7				# need to precede with '0'?
	bpl.b	L680
	cmp.b	%d1,(%a5)			# already got a '0'?
	beq.b	L680
	subq.w	&1,%a5				# no: simply back up pointer
L680:
	mov.l	%a1,%d2				# calculate value length
	sub.l	%a5,%d2
	mov.l	%d2,%d4				# is also the total width
	bra.w	L1600

#
# 'c' and '%' formats
#

L690:
	mov.l	(%a4)+,%d2			# get character

L700:
	lea	BUFFER(%a6),%a5			# where to put character
	mov.b	%d2,(%a5)			# store it away
	movq	&1,%d2				# character length
	movq	&PREFIX_NULL,%d3		# null prefix
	movq	&1,%d4				# total width
	bra.w	L1600

#
# 's' format
#

L710:
	movq	&PREFIX_NULL,%d3		# no prefix
	mov.l	(%a4)+,%d0			# pointer to string
	bne.b	L730				# nil pointer?
L720:
	movq	&0,%d2				# length = 0
	movq	&0,%d4				# total length = 0
	bra.w	L1600
L730:
	mov.l	%d0,%a5				# start of string
	mov.l	%a5,%a1
	movq	&FDOT,%d1			# check if precision specified
	and.b	%d7,%d1				# if ((!(flagword & FDOT))
	beq.b	L740				#
	tst.l	%d6				# || (prec < 0))
	bpl.b	L750
L740:
	tst.b	(%a1)+				# find end of string
	bne.b	L740
	subq.w	&1,%a1				# back up to null terminator
	mov.l	%a1,%d2				# calculate length
	sub.l	%a5,%d2
	mov.l	%d2,%d4				# is also total length
	bra.w	L1600
L750:
	movq	&0,%d1				# counter
	mov.w	%d6,%d1				# prec
	cmp.l	%d6,%d1				# < 65536 bytes?
	bne.b	L780
	subq.w	&1,%d1				# adjust counter
	bcs.b	L720				# prec == 0?
L760:
	tst.b	(%a1)+				# search for end within prec
	dbeq	%d1,L760
	bne.b	L800
L770:
	subq.w	&1,%a1				# string is longer than prec
	mov.l	%a1,%d2				# calculate length
	sub.l	%a5,%d2
	mov.l	%d2,%d4				# is also total length
	bra.w	L1600
L780:
	mov.l	%d6,%d1				# prec
L790:
	tst.b	(%a1)+				# search for end within prec
	beq.b	L770
	subq.l	&1,%d1				# one less character
	bne.b	L790
L800:
	mov.l	%d6,%d2				# length same as precision
	mov.l	%d2,%d4				# and same as total length
	bra.w	L1600

#
# 'e' and 'E' formats
#

L810:
	bsr.w	L2250				# check for inf's and NaN's
	mov.b	%d2,SUFFIXBUF(%a6)		# save exponent character
	movq	&PREFIX_NULL,%d3		# no prefix
	movq	&0,%d4				# total length
	or.w	&FSUFFIX,%d7			# we've got a suffix
#begin long double changes
	movq	&0,%d0
	or.l	&FLDOUBLE,%d0			# if (flagword & FLDOUBLE)
	and.l	%d7,%d0
	bne.b	LDBL
#end long double changes
	movq	&FDOT,%d1			# if (!(flagword & FDOT))
	and.b	%d7,%d1
	bne.b	L820
	movq	&DEF_FLOAT_PREC,%d6		# default precision
L820:
	movq	&MAXECVT,%d0			# max number significant digits
	cmp.l	%d6,%d0
	bhs.b	L830				# more digits required?
	mov.w	%d6,%d0				# no: calculate digits
	addq.w	&1,%d0				# one for left of decimal point
	bra.b	L910
L830:
	movq	&-(MAXECVT-1),%d4		# calculate total length
	add.l	%d6,%d4
	mov.l	%d4,DZERO(%a6)			# number of zeroes
	or.w	&FDZERO,%d7
	movq	&(MAXECVT-1),%d6		# max significant precision
#begin long double changes
	bra	L910
LDBL:
	movq	&FDOT,%d1			# if (!(flagword & FDOT))
	and.b	%d7,%d1
	bne.b	LDBL820
	movq	&DEF_FLOAT_PREC,%d6		# default precision
LDBL820:
	movq	&MAXQECVT,%d0			# max number significant digits
	cmp.l	%d6,%d0
	bhs.b	LDBL830				# more digits required?
	mov.w	%d6,%d0				# no: calculate digits
	addq.w	&1,%d0				#one for left of decimal point
	bra.b	LDBL910
LDBL830:
	movq	&-(MAXQECVT-1),%d4		# calculate total length
	add.l	%d6,%d4
	mov.l	%d4,DZERO(%a6)			# number of zeroes
	or.w	&FDZERO,%d7
	movq	&(MAXQECVT-1),%d6		# max significant precision
LDBL910:
	pea	SIGN(%a6)			# pointer to sign
	pea	DECPT(%a6)			# pointer to decimal point
	mov.l	%d0,-(%sp)			# push number of digits
	movq	&16,%d0
	adda	%d0,%a4
	mov.l	-(%a4),-(%sp)			# push lo word
	mov.l	-(%a4),-(%sp)			# push mid-lo word
	mov.l	-(%a4),-(%sp)			# push mid-hi word
	mov.l	-(%a4),-(%sp)			# push hi word
	adda	%d0,%a4				# adjust a4
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__ldecvt',`			# convert it
	bsr.l	_ldecvt				# convert it
')
	add	&28,%sp				# clean off stack
	mov.l	%d0,%a0				# result pointer
	bra.b	L920
#end long double changes
L910:
	pea	SIGN(%a6)			# pointer to sign
	pea	DECPT(%a6)			# pointer to decimal point
	mov.l	%d0,-(%sp)			# number of digits
	mov.l	(%a4)+,%d0			# get number
	mov.l	(%a4)+,-(%sp)
	mov.l	%d0,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__ecvt',`			# convert it
	bsr.l	_ecvt				# convert it
')
	mov.l	%d0,%a0				# result pointer
	add	&20,%sp				# clean off stack
L920:
	tst.l	SIGN(%a6)			# sign of number
	beq.b	L930
	movq	&PREFIX_MINUS,%d3		# prefix = '-'
	addq.l	&1,%d4				# one more character
	bra.b	L940
L930:
	movq	&FPLUS|FBLANK,%d1		# want a plus sign or space?
	and.b	%d7,%d1
	beq.w	L940
	addq.l	&1,%d4				# one more character
	movq	&PREFIX_PLUS,%d3		# assume prefix = '+'
	subq.b	&FPLUS,%d1			# is it to be a '+'?
	bpl.b	L940
	movq	&PREFIX_BLANK,%d3		# no: a blank
L940:
	lea	BUFFER(%a6),%a5			# where to store it
	mov.l	%a5,%a1
	mov.b	(%a0)+,(%a1)+			# move first digit
	mov.b	RADIX(%a6),(%a1)+		# store a decimal point
L960:
	mov.b	(%a0)+,(%a1)+			# move number
	bne.b	L960
	addq.l	&4,%d4				# size of exponent
	lea	SUFFIXBUF+1(%a6),%a0		# where to put exponent
	movq	&'+,%d1				# assume positive exponent
	movq	&0,%d0
	cmp.b	(%a5),&'0			# is the number a zero?
	beq.b	L970
	mov.w	DECPT+2(%a6),%d0		# calculate exponent
	subq.w	&1,%d0				# adjust appropriately
	bpl.b	L970
	movq	&'-,%d1				# negative exponent
	neg.w	%d0				# absolute value
L970:
	mov.b	%d1,(%a0)+			# store exponent sign
ifdef(`PIC',`
	bsr.l	LPROLOG1
	mov.l	__L40(%a1),%a1',`		# decimal ASCII character table
	lea	__L40,%a1			# decimal ASCII character table
')
	mov.b	&0,EXP_3_4(%a6)			# assume <= 3 digit exponent.
	movq	&100,%d1
	cmp.w	%d0,%d1				# 1/2 or 3/4 digit exponent?
	blt.b	L980
	divu	%d1,%d0				# get upper digit
	mov.b	0(%a1,%d0.w),%d1		# first digit a '0'?
	subi.b	&'0,%d1
	beq.b	L975
	mov.b	0(%a1,%d0.w),(%a0)+		# store it
	addq.l	&1,%d4				# totallength += extra exp digit
	mov.b	&1,EXP_3_4(%a6)			# its a 4 digit exponent
L975:
	mov.b	__L50-__L40(%a1,%d0.w),(%a0)+	# second digit
	swap	%d0				# remaining two digits
	addq.l	&1,%d4				# totallength += extra exp digit
	or.w	&FEXTEXP,%d7			# 3 or 4 digit exponent
L980:
	mov.b	0(%a1,%d0.w),(%a0)+		# third digit
	mov.b	__L50-__L40(%a1,%d0.w),(%a0)+	# fourth digit
L990:
	tst.l	%d6				# precision == 0?
	bne.b	L1010
	tst.b	%d7				# keep the decimal point?
	bmi.b	L1000
	movq.l	&1,%d2				# number length is 1
	addq.l	&1,%d4
	bra.w	L1600
L1000:
	movq.l	&2,%d2				# number length is 2
	addq.l	&2,%d4
	bra.w	L1600
L1010:
	mov.l	%d6,%d2				# length is precision plus
	addq.l	&2,%d2				#   d.p. and leading digit
	add.l	%d2,%d4
	bra.w	L1600

#
# 'f' format
#

L1020:
	bsr.w	L2250				# check for inf's and NaN's
	movq	&PREFIX_NULL,%d3		# no prefix
	movq	&0,%d4				# total length
	movq	&FDOT,%d1			# if (!(flagword & FDOT))
	and.b	%d7,%d1
	bne.b	L1120
	movq	&DEF_FLOAT_PREC,%d6		# default precision
L1120:
	mov.l	&MAXFCVT,%d0			# maximum significant precision
	cmp.l	%d6,%d0				# okay?
	bhs.b	L1130
	mov.l	%d6,%d0
L1130:
	pea	SIGN(%a6)			# pointer to sign
	pea	DECPT(%a6)			# pointer to decimal point
	mov.l	%d0,-(%sp)			# number of digits
#begin long double changes
	movq	&0,%d0
	or.l	&FLDOUBLE,%d0			# if (flagword & FLDOUBLE)
	and.l	%d7,%d0
	bne.w	LDBL1135
#end long double changes
	mov.l	(%a4)+,%d0			# get number
	mov.l	(%a4)+,-(%sp)
	mov.l	%d0,-(%sp)
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__fcvt',`			# convert it
	bsr.l	_fcvt				# convert it
')
	add	&20,%sp				# clean off stack
	mov.l	%d0,%a5				# pointer to number
L1140:
	tst.l	SIGN(%a6)			# sign of number
	beq.b	L1150
	movq	&PREFIX_MINUS,%d3		# prefix = '-'
	movq	&1,%d4				# total size = 1
	movq	&0,%d1
	bra.b	L1160
L1150:
	movq	&FPLUS|FBLANK,%d1		# want a plus sign or space?
	and.b	%d7,%d1
	beq.b	L1160
	movq	&1,%d4				# total size = 1
	movq	&PREFIX_PLUS,%d3		# assume prefix = '+'
	subq.b	&FPLUS,%d1			# is it to be a '+'?
	bpl.b	L1160
	movq	&PREFIX_BLANK,%d3		# no: a blank
L1160:
	mov.w	DECPT+2(%a6),%d1		# get decimal point
	subq.w	&1,%d1				# make into exponent
	bpl.b	L1170
	neg.w	%d1				# absolute value
	bra.b	L1230
L1170:
	cmp.w	%d1,&MAXFSIG-2			# small exponent?
	bls.w	L1300
	#
	# All significant digits are left of the decimal point
	#								
L1180:
	movq	&MAXFSIG,%d2			# size of number
	add.w	%d1,%d4				# increment total size
	addq.w	&1,%d4
	tst.l	%d6				# zeroes to right of d.p.?
	beq.b	L1200				
	addq.w	&1,%d4				# for '.'
	add.l	%d6,%d4				# for zeroes to right of d.p.
	sub.w	&MAXFSIG-1,%d1			# non-significant zeroes
	beq.b	L1190
	or.w	&FCZERO|FRDP|FDZERO,%d7		# 0's left & right of d.p.
	mov.l	%d1,CZERO(%a6)
	mov.l	%d6,DZERO(%a6)
	bra.w	L1600
L1190:
	or.w	&FRDP|FDZERO,%d7		# 0's only to right of d.p.
	mov.l	%d6,DZERO(%a6)
	bra.w	L1600
L1200:
	tst.b	%d7				# want a decimal point?
	bpl.b	L1220
	addq.w	&1,%d4				# for '.'
	sub.w	&MAXFSIG-1,%d1			# non-significant zeroes
	beq.b	L1210
	or.w	&FCZERO|FRDP,%d7		# decimal point on right
	mov.l	%d1,CZERO(%a6)
	bra.w	L1600
L1210:
	or.w	&FRDP,%d7			# only a decimal point
	bra.w	L1600
L1220:
	sub.w	&MAXFSIG-1,%d1			# non-significant zeroes
	beq.w	L1600
	mov.l	%d1,DZERO(%a6)
	or.w	&FDZERO,%d7
	bra.w	L1600
	#
	# All significant digits are right of the decimal point
	#								
L1230:
	tst.l	%d6				# precision?
	bne.b	L1250
	lea	BUFFER(%a6),%a5			# buffer
	mov.w	&0x3000,%d2			# 0 and radix point
	or.b	RADIX(%a6),%d2			# store a radix point
	mov.w	%d2,(%a5)
	tst.b	%d7				# decimal point or not?
	bmi.b	L1240
	movq	&1,%d2				# size of '0'
	addq.w	&1,%d4				# increment total size
	bra.w	L1600
L1240:
	movq	&2,%d2				# size  of '0.'
	addq.w	&2,%d4				# increment total size
	bra.w	L1600
L1250:
	movq	&1,%d0				# one zero before d.p.
	mov.l	%d0,AZERO(%a6)
	addq.w	&2,%d4				# for '0.'
	add.l	%d6,%d4				# for digits to right
	cmp.l	%d6,%d1				# room for all zeroes?
	bhs.b	L1260
	movq	&0,%d2				# no significant digits
	or.w	&FAZERO|FLDP|FBZERO,%d7
	mov.l	%d6,BZERO(%a6)			# e.g. '0.0000'
	bra.w	L1600
L1260:
	subq.w	&1,%d1				# zeroes to right of d.p.
	beq.b	L1270
	or.w	&FAZERO|FLDP|FBZERO,%d7		# decimal point on left
	mov.l	%d1,BZERO(%a6)			# number of zeroes
	sub.l	%d1,%d6				# significant digits left
	bra.w	L1280
L1270:
	or.w	&FAZERO|FLDP,%d7		# only decimal point on left
L1280:
	movq	&MAXFSIG,%d2
	cmp.l	%d6,%d2				# zeroes on right?
	bhi.b	L1290
	mov.l	%d6,%d2				# significant digits
	bra.w	L1600
L1290:
	sub.l	%d2,%d6				# zeroes on right
	mov.l	%d6,DZERO(%a6)
	or.w	&FDZERO,%d7
	bra.w	L1600
	#
	# digits on both left and right of decimal point
	#								
L1300:
	tst.l	%d6				# want digits to right of d.p.?
	beq.b	L1340
	mov.l	%d1,%d2				# digits left of d.p. - 1
	addq.w	&2,%d2				# extra digit and d.p.
	add.l	%d6,%d2				# digits to right of d.p.
	add.l	%d2,%d4				# increment total size
	movq	&MAXFSIG-1,%d0			# calculate significant digits
	sub.w	%d1,%d0				#   to right of decimal point
	cmp.l	%d0,%d6				# any padding needed?
	bhs.b	L1310
	sub.l	%d0,%d6				# calculate padding
	mov.l	%d6,DZERO(%a6)			# number of zeroes
	or.w	&FDZERO,%d7
	movq	&MAXFSIG+1,%d2			# size of number
	bra.b	L1320
L1310:
	mov.w	%d6,%d0				# digits to right of d.p.
L1320:
	lea	0(%a5,%d2.w),%a1		# end of buffer
	lea	-1(%a1),%a0			# and back one
	subq.w	&1,%d0				# counter
L1330:
	mov.b	-(%a0),-(%a1)			# shift digits
	dbra	%d0,L1330
	mov.b	RADIX(%a6),(%a0)		# put in decimal point
	bra.w	L1600
L1340:
	addq.w	&1,%d1				# digits to left of d.p.
	mov.l	%d1,%d2				# number size
	tst.b	%d7				# want decimal point?
	bpl.b	L1350
	addq.w	&1,%d2				# one more for '.'
	mov.b	RADIX(%a6),0(%a5,%d1.w)		# insert decimal point
L1350:
	add.w	%d2,%d4				# increment total size
	bra.w	L1600
# begin long double changes
LDBL1135:
	movq	&16,%d0
	adda	%d0,%a4
	mov.l	-(%a4),-(%sp)			# push lo word
	mov.l	-(%a4),-(%sp)			# push mid-lo word
	mov.l	-(%a4),-(%sp)			# push mid-hi word
	mov.l	-(%a4),-(%sp)			# push hi word
	adda	%d0,%a4
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__ldfcvt',`			# convert it
	bsr.l	_ldfcvt				# convert it
')
	add	&28,%sp				# clean off stack
	mov.l	%d0,%a5				# pointer to number
LDBL1140:
	tst.l	SIGN(%a6)			# sign of number
	beq.b	LDBL1150
	movq	&PREFIX_MINUS,%d3		# prefix = '-'
	movq	&1,%d4				# total size = 1
	movq	&0,%d1
	bra.b	LDBL1160
LDBL1150:
	movq	&FPLUS|FBLANK,%d1		# want a plus sign or space?
	and.b	%d7,%d1
	beq.b	LDBL1160
	movq	&1,%d4				# total size = 1
	movq	&PREFIX_PLUS,%d3		# assume prefix = '+'
	subq.b	&FPLUS,%d1			# is it to be a '+'?
	bpl.b	LDBL1160
	movq	&PREFIX_BLANK,%d3		# no: a blank
LDBL1160:
	mov.w	DECPT+2(%a6),%d1		# get decimal point
	subq.w	&1,%d1				# make into exponent
	bpl.b	LDBL1170
	neg.w	%d1				# absolute value
	bra.b	LDBL1230
LDBL1170:
	cmp.w	%d1,&MAXQFSIG-2			# small exponent?
	bls.w	LDBL1300
	#
	# All significant digits are left of the decimal point
	#								
LDBL1180:
	movq	&MAXQFSIG,%d2			# size of number
	add.w	%d1,%d4				# increment total size
	addq.w	&1,%d4
	tst.l	%d6				# zeroes to right of d.p.?
	beq.b	LDBL1200				
	addq.w	&1,%d4				# for '.'
	add.l	%d6,%d4				# for zeroes to right of d.p.
	sub.w	&MAXQFSIG-1,%d1			# non-significant zeroes
	beq.b	LDBL1190
	or.w	&FCZERO|FRDP|FDZERO,%d7		# 0's left & right of d.p.
	mov.l	%d1,CZERO(%a6)
	mov.l	%d6,DZERO(%a6)
	bra.w	L1600
LDBL1190:
	or.w	&FRDP|FDZERO,%d7		# 0's only to right of d.p.
	mov.l	%d6,DZERO(%a6)
	bra.w	L1600
LDBL1200:
	tst.b	%d7				# want a decimal point?
	bpl.b	LDBL1220
	addq.w	&1,%d4				# for '.'
	sub.w	&MAXQFSIG-1,%d1			# non-significant zeroes
	beq.b	LDBL1210
	or.w	&FCZERO|FRDP,%d7		# decimal point on right
	mov.l	%d1,CZERO(%a6)
	bra.w	L1600
LDBL1210:
	or.w	&FRDP,%d7			# only a decimal point
	bra.w	L1600
LDBL1220:
	sub.w	&MAXQFSIG-1,%d1			# non-significant zeroes
	beq.w	L1600
	mov.l	%d1,DZERO(%a6)
	or.w	&FDZERO,%d7
	bra.w	L1600
	#
	# All significant digits are right of the decimal point
	#								
LDBL1230:
	tst.l	%d6				# precision?
	bne.b	LDBL1250
	lea	BUFFER(%a6),%a5			# buffer
	mov.w	&0x3000,%d2			# 0 and radix point
	or.b	RADIX(%a6),%d2			# store a radix point
	mov.w	%d2,(%a5)
	tst.b	%d7				# decimal point or not?
	bmi.b	LDBL1240
	movq	&1,%d2				# size of '0'
	addq.w	&1,%d4				# increment total size
	bra.w	L1600
LDBL1240:
	movq	&2,%d2				# size  of '0.'
	addq.w	&2,%d4				# increment total size
	bra.w	L1600
LDBL1250:
	movq	&1,%d0				# one zero before d.p.
	mov.l	%d0,AZERO(%a6)
	addq.w	&2,%d4				# for '0.'
	add.l	%d6,%d4				# for digits to right
	cmp.l	%d6,%d1				# room for all zeroes?
	bhs.b	LDBL1260
	movq	&0,%d2				# no significant digits
	or.w	&FAZERO|FLDP|FBZERO,%d7
	mov.l	%d6,BZERO(%a6)			# e.g. '0.0000'
	bra.w	L1600
LDBL1260:
	subq.w	&1,%d1				# zeroes to right of d.p.
	beq.b	LDBL1270
	or.w	&FAZERO|FLDP|FBZERO,%d7		# decimal point on left
	mov.l	%d1,BZERO(%a6)			# number of zeroes
	sub.l	%d1,%d6				# significant digits left
	bra.w	LDBL1280
LDBL1270:
	or.w	&FAZERO|FLDP,%d7		# only decimal point on left
LDBL1280:
	movq	&MAXQFSIG,%d2
	cmp.l	%d6,%d2				# zeroes on right?
	bhi.b	LDBL1290
	mov.l	%d6,%d2				# significant digits
	bra.w	L1600
LDBL1290:
	sub.l	%d2,%d6				# zeroes on right
	mov.l	%d6,DZERO(%a6)
	or.w	&FDZERO,%d7
	bra.w	L1600
	#
	# digits on both left and right of decimal point
	#								
LDBL1300:
	tst.l	%d6				# want digits to right of d.p.?
	beq.b	LDBL1340
	mov.l	%d1,%d2				# digits left of d.p. - 1
	addq.w	&2,%d2				# extra digit and d.p.
	add.l	%d6,%d2				# digits to right of d.p.
	add.l	%d2,%d4				# increment total size
	movq	&MAXQFSIG-1,%d0			# calculate significant digits
	sub.w	%d1,%d0				#   to right of decimal point
	cmp.l	%d0,%d6				# any padding needed?
	bhs.b	LDBL1310
	sub.l	%d0,%d6				# calculate padding
	mov.l	%d6,DZERO(%a6)			# number of zeroes
	or.w	&FDZERO,%d7
	movq	&MAXQFSIG+1,%d2			# size of number
	bra.b	LDBL1320
LDBL1310:
	mov.w	%d6,%d0				# digits to right of d.p.
LDBL1320:
	lea	0(%a5,%d2.w),%a1		# end of buffer
	lea	-1(%a1),%a0			# and back one
	subq.w	&1,%d0				# counter
LDBL1330:
	mov.b	-(%a0),-(%a1)			# shift digits
	dbra	%d0,LDBL1330
	mov.b	RADIX(%a6),(%a0)		# put in decimal point
	bra.w	L1600
LDBL1340:
	addq.w	&1,%d1				# digits to left of d.p.
	mov.l	%d1,%d2				# number size
	tst.b	%d7				# want decimal point?
	bpl.b	LDBL1350
	addq.w	&1,%d2				# one more for '.'
	mov.b	RADIX(%a6),0(%a5,%d1.w)		# insert decimal point
LDBL1350:
	add.w	%d2,%d4				# increment total size
	bra.w	L1600
# end long double changes

#
# 'g' and 'G' formats
#

L1360:
	bsr.w	L2250				# check for inf's and NaN's
	subq.b	&'G-'E,%d2			# turn into 'e' or 'E'
	mov.b	%d2,SUFFIXBUF(%a6)		# save exponent character
	movq	&PREFIX_NULL,%d3		# no prefix
	movq	&0,%d4				# total length
	movq	&FDOT,%d1			# precision specified?
	and.b	%d7,%d1				# if (flagword & FDOT)
	bne.b	L1370
	movq	&DEF_FLOAT_PREC,%d6		# default precision
	bra.b	L1380
L1370:
	tst.l	%d6				# prec == 0?
	bne.b	L1380
	movq	&1,%d6				# yes: change to 1
L1380:
	movq	&0,%d0
	or.l	&FLDOUBLE,%d0			# if (flagword & FLDOUBLE)
	and.l	%d7,%d0
	bne.w	LDBL1385
	movq	&MAXECVT,%d0			# how many digits?
	cmp.l	%d6,%d0
	bhi.b	L1470
	mov.w	%d6,%d0				# 17 digits or less
L1470:
	pea	SIGN(%a6)			# pointer to sign
	pea	DECPT(%a6)			# pointer to decimal point
	mov.l	%d0,-(%sp)			# number of digits
	mov.l	%d0,%d2				# save number of digits
	movm.l	(%a4)+,%d0/%d1			# get number
	movm.l	%d0/%d1,-(%sp)
	add.l	%d0,%d0				# remove sign bit
	or.l	%d0,%d1				# check for zero
	beq.w	L1550
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__ecvt',`			# convert it
	bsr.l	_ecvt				# convert it
')
	add	&20,%sp				# clean off stack
	mov.l	%d0,%a5				# result pointer
	lea	0(%a5,%d2.l),%a1		# end of number
	movq	&MAXECVT-1,%d0
	sub.w	%d2,%d0				# counter to fill out buffer
	bcs.b	L1490
	mov.l	%a1,%a0
	movq	&'0,%d1				# zero
L1480:
	mov.b	%d1,(%a0)+			# pad buffer
	dbra	%d0,L1480
L1490:
	movq	&0,%d1				# Add NULL for end
	mov.b	%d1,(%a0)			# of string
	mov.l	%a5,%a0				# start of buffer
	mov.l	DECPT(%a6),%d1			# decimal point
	mov.l	%d6,%d0				# precision
	tst.b	%d7				# FSHARP?
	bmi.b	L1510
	movq	&'0,%d2				# search for trailing zeroes
L1500:
	cmp.b	%d2,-(%a1)			# trailing zeroes?
	beq.b	L1500
	mov.l	%a1,%d0				# calculate number of
	sub.l	%a0,%d0				#   significant digits
	addq.w	&1,%d0
L1510:
	cmp.w	%d1,&-3				# decpt : 3?
	blt.b	L1520
	addq.l	&1,%d6				# precision += 1
	cmp.l	%d1,%d6				# decpt : precision?
	blt.b	L1530
L1520:
	mov.l	%d0,%d6				# precision -= 1
	subq.w	&1,%d6
	or.w	&FSUFFIX,%d7			# e format
	movq	&MAXECVT-1,%d2			# calculate extra zeroes
	cmp.l	%d6,%d2
	bls.w	L920				# any extra zeroes?
	mov.l	%d6,%d4				# set extra length
	sub.l	%d2,%d4
	mov.l	%d2,%d6				# precision = 16
	mov.l	%d4,DZERO(%a6)			# trailing zeroes
	or.w	&FDZERO,%d7
	bra.w	L920
L1530:
	mov.l	%d0,%d6				# calculate precision
	sub.l	%d1,%d6				# subtract decimal point
	bpl.w	L1140
	movq	&0,%d6				# lower limit of 0
	bra.w	L1140				# use f format
#begin long double changes
LDBL1385:
	movq	&MAXQECVT,%d0			# how many digits?
	cmp.l	%d6,%d0
	bhi.b	LDBL1470
	mov.w	%d6,%d0				# 17 digits or less
LDBL1470:
	pea	SIGN(%a6)			# pointer to sign
	pea	DECPT(%a6)			# pointer to decimal point
	mov.l	%d0,-(%sp)			# number of digits
	mov.l	%d0,%d2				# save number of digits
	movq	&16,%d0
	adda	%d0,%a4
	mov.l	-(%a4),%d0
	mov.l	%d0,-(%sp)			# push lo word
	add.l	%d0,%d0				# remove sign bit
	mov.l	%d0,%d1
	mov.l	-(%a4),%d0
	mov.l	%d0,-(%sp)			# push mid-lo word
	or.l	%d0,%d1
	mov.l	-(%a4),%d0
	mov.l	%d0,-(%sp)			# push mid-hi word
	or.l	%d0,%d1
	mov.l	-(%a4),%d0
	mov.l	%d0,-(%sp)			# push hi word
	or.l	%d0,%d1				# check for zero
	movq	&16,%d0
	adda	%d0,%a4				# adjust a4
	tst.l	%d1
	beq.w	LDBL1550
LDBL1475:
ifdef(`_NAMESPACE_CLEAN',`
	bsr.l	__ldecvt',`			# convert it
	bsr.l	_ldecvt				# convert it
')
	add	&28,%sp				# clean off stack
	mov.l	%d0,%a5				# result pointer
	lea	0(%a5,%d2.l),%a1		# end of number
	movq	&MAXQECVT-1,%d0
	sub.w	%d2,%d0				# counter to fill out buffer
	bcs.b	LDBL1490
	mov.l	%a1,%a0
	movq	&'0,%d1				# zero
LDBL1480:
	mov.b	%d1,(%a0)+			# pad buffer
	dbra	%d0,LDBL1480
LDBL1490:
	movq	&0,%d1				# Add NULL for end
	mov.b	%d1,(%a0)			# of string
	mov.l	%a5,%a0				# start of buffer
	mov.l	DECPT(%a6),%d1			# decimal point
	mov.l	%d6,%d0				# precision
	tst.b	%d7				# FSHARP?
	bmi.b	LDBL1510
	movq	&'0,%d2				# search for trailing zeroes
LDBL1500:
	cmp.b	%d2,-(%a1)			# trailing zeroes?
	beq.b	LDBL1500
	mov.l	%a1,%d0				# calculate number of
	sub.l	%a0,%d0				#   significant digits
	addq.w	&1,%d0
LDBL1510:
	cmp.w	%d1,&-3				# decpt : 3?
	blt.b	LDBL1520
	addq.l	&1,%d6				# precision += 1
	cmp.l	%d1,%d6				# decpt : precision?
	blt.b	LDBL1530
LDBL1520:
	mov.l	%d0,%d6				# precision -= 1
	subq.w	&1,%d6
	or.w	&FSUFFIX,%d7			# e format
	movq	&MAXQECVT-1,%d2			# calculate extra zeroes
	cmp.l	%d6,%d2
	bls.w	L920				# any extra zeroes?
	mov.l	%d6,%d4				# set extra length
	sub.l	%d2,%d4
	mov.l	%d2,%d6				# precision = (MAXQECVT -1)
	mov.l	%d4,DZERO(%a6)			# trailing zeroes
	or.w	&FDZERO,%d7
	bra.w	L920
LDBL1530:
	mov.l	%d0,%d6				# calculate precision
	sub.l	%d1,%d6				# subtract decimal point
	bpl.w	LDBL1140
	movq	&0,%d6				# lower limit of 0
	bra.w	LDBL1140			# use f format
LDBL1550:
	add	&28,%sp				# clean off stack
	bra.b	L1560
#end long double changes
	#
	# special case zero
	#								
L1550:
	add	&20,%sp				# clean off stack
L1560:
	movq	&FPLUS|FBLANK,%d1		# want a plus sign or space?
	and.b	%d7,%d1
	beq.b	L1570
	movq	&1,%d4				# total size = 1
	movq	&PREFIX_PLUS,%d3		# assume prefix = '+'
	subq.b	&FPLUS,%d1			# is it to be a '+'?
	bpl.b	L1570
	movq	&PREFIX_BLANK,%d3		# no: a blank
L1570:
	tst.b	%d7				# FSHARP?
	bmi.b	L1580
	lea	BUFFER(%a6),%a5			# location for result
	mov.b	&'0,(%a5)			# answer is 0
	movq	&1,%d2				# formatted string length
	addq.w	&1,%d4				# increment total length
	bra.b	L1600
L1580:
	movq	&1,%d0
	mov.l	%d0,CZERO(%a6)			# one zero to left of dec pt
	movq	&0,%d2				# no formatted string length
	subq.l	&1,%d6				# precision
	beq.b	L1590
	or.w	&FCZERO|FRDP|FDZERO,%d7		# e.g. 0.0000
	addq.w	&2,%d4				# for decimal point, 0 to left
	add.l	%d6,%d4				# increment length
	mov.l	%d6,DZERO(%a6)			# zeroes to right of dec pt
	bra.b	L1600
L1590:
	or.w	&FCZERO|FRDP,%d7		# i.e. '0.'
	addq.w	&2,%d4				# length is 2

#
# Dump formatted string to buffer
#

L1600:
	sub.l	%d4,%d5				# field wide enough?
	bcs.b	L1610				# is it?
	add.l	%d5,%d4				# real width
	bra.b	L1620
L1610:
	movq	&0,%d5				# no padding
L1620:
	mov.l	BUFEND(%a6),%d6			# end of buffer
	sub.l	%a2,%d6				# room left
	cmp.l	%d4,%d6				# fit into buffer?
	bhi.w	L1850
	cmp.l	%d4,&65536			# huge buffer?
	bhs.w	L1850
	#
	# check for blank padding on the left
	#
	tst.l	%d5				# amount of padding
	beq.b	L1640
	movq	&FMINUS|FPADZERO,%d1		# left pad with blanks?
	and.b	%d7,%d1
	bne.b	L1640
	movq	&'\040,%d1			# yes
	subq.w	&1,%d5				# counter
L1630:
	mov.b	%d1,(%a2)+			# fill in blanks
	dbra	%d5,L1630
	movq	&0,%d5
	#
	# check for prefix
	#								
L1640:
	movq	&'0,%d0				# constant '0'
	tst.b	%d3				# prefix?
	beq.b	L1660
	bpl.b	L1650
	mov.b	%d0,(%a2)+			# must be hex prefix
	neg.b	%d3
L1650:
	mov.b	%d3,(%a2)+			# move 'x' or 'X'
	#
	# check for zero padding
	#								
L1660:
	tst.l	%d5				# padding needed?
	beq.b	L1680
	movq	&FMINUS,%d1			# left justify?
	and.b	%d7,%d1
	bne.b	L1680
	subq.w	&1,%d5				# counter
L1670:
	mov.b	%d0,(%a2)+			# fill in zeroes
	dbra	%d5,L1670
	movq	&0,%d5
	#
	# check for zeroes to left of formatted string
	# (e.g. 00004)
	#       ^^^^
	#								
L1680:
	tst.w	%d7				# zeroes to the left?
	bpl.b	L1700
	mov.w	AZERO+2(%a6),%d1
	subq.w	&1,%d1				# counter
L1690:
	mov.b	%d0,(%a2)+			# fill in zeroes
	dbra	%d1,L1690
	#
	# Check for decimal point on the left
	#								
L1700:
	add.w	%d7,%d7
	bpl.b	L1710
	mov.b	RADIX(%a6),(%a2)+		# store a decimal point
	#
	# check for zeroes to left of formatted string
	# (e.g. 0.00004)
	#         ^^^^
	#								
L1710:
	add.w	%d7,%d7				# more zeroes?
	bpl.b	L1730
	mov.w	BZERO+2(%a6),%d1
	subq.w	&1,%d1				# counter
L1720:
	mov.b	%d0,(%a2)+			# fill in zeroes
	dbra	%d1,L1720
	#
	# move formatted string
	#								
L1730:
	subq.w	&1,%d2				# actual string length
	bcs.b	L1750
L1740:
	mov.b	(%a5)+,(%a2)+			# move string
	dbra	%d2,L1740
	#
	# check for zeroes to right of formatted string
	# (e.g. 4000000.)
	#        ^^^^^^
	#								
L1750:
	mov.l	BUFEND(%a6),%a5			# end of buffer
	add.w	%d7,%d7				# zeroes to the right?
	bpl.b	L1770
	mov.w	CZERO+2(%a6),%d1
	subq.w	&1,%d1				# counter
L1760:
	mov.b	%d0,(%a2)+			# fill in zeroes
	dbra	%d1,L1760
	#
	# Check for decimal point on the right
	#								
L1770:
	add.w	%d7,%d7
	bpl.b	L1780
	mov.b	RADIX(%a6),(%a2)+		# store a decimal point
	#
	# check for zeroes to right of formatted string
	# (e.g. 4.0000)
	#         ^^^^
	#								
L1780:
	add.w	%d7,%d7				# more zeroes to the right?
	bpl.b	L1800
	mov.w	DZERO+2(%a6),%d1
	subq.w	&1,%d1				# counter
L1790:
	mov.b	%d0,(%a2)+			# fill in zeroes
	dbra	%d1,L1790
	#
	# check for suffix (e.g. 1.3E+14)
	#			    ^^^^
	#								
L1800:
	add.w	%d7,%d7				# have a suffix?
	bpl.b	L1830
	lea	SUFFIXBUF(%a6),%a0
	add.w	%d7,%d7				# calculate size of suffix
	bpl.b	L1820
	mov.b	(%a0)+,(%a2)+
L1820:	# FEXTEXP set
	tst.b	EXP_3_4(%a6)			# 3 or 4 digit exponent?
	beq.b	L1825
	mov.b	(%a0)+,(%a2)+			#
L1825:
	mov.b	(%a0)+,(%a2)+			#
	mov.b	(%a0)+,(%a2)+			#
	mov.b	(%a0)+,(%a2)+			#
	mov.b	(%a0)+,(%a2)+			#
	#
	# check for blank padding on the right
	#								
L1830:
	tst.l	%d5				# padding needed?
	beq.w	L70
	movq	&'\040,%d1			# pad with blanks
	subq.w	&1,%d5				# counter
L1840:
	mov.b	%d1,(%a2)+			# move blanks
	dbra	%d5,L1840
	bra.w	L70
	#
	# Either the formatted string will overflow the buffer
	# or the buffer is greater than 65535 bytes. (Probably
	# always the former.)
	#
	# %d5 = characters of padding
	# %d6 = space remaining in buffer

	#
	# check for blank padding on the left
	#								
L1850:
	mov.l	%d5,%d4				# padding needed?
	beq.b	L1860
	movq	&FMINUS|FPADZERO,%d1		# left pad with blanks?
	and.b	%d7,%d1
	bne.b	L1860
	movq	&'\040,%d1			# pad with blanks
	bsr.w	L2190
	movq	&0,%d5				# padding done
	#
	# check for prefix
	#								
L1860:
	tst.b	%d3				# prefix?
	beq.b	L1920
	bmi.b	L1880
	subq.l	&1,%d6				# any room left in buffer?
	bhs.b	L1870
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	subq.l	&1,%d6				# room for one less
L1870:
	mov.b	%d3,(%a2)+			# move prefix
	bra.b	L1920
L1880:
	subq.l	&2,%d6				# must be hex prefix
	bcs.b	L1890				# need space for two characters
	mov.b	&'0,(%a2)+			# move '0'
	neg.b	%d3
	mov.b	%d3,(%a2)+			# move 'x' or 'X'
	bra.b	L1920
L1890:
	addq.w	&1,%d6				# room for '0'?
	bne.b	L1900
	mov.b	&'0,(%a2)+			# '0' of hex prefix
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	subq.l	&1,%d6				# space for one less
	bra.b	L1910
L1900:
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	subq.l	&2,%d6				# space for two less
	mov.b	&'0,(%a2)+			# '0' of hex prefix
L1910:
	neg.b	%d3
	mov.b	%d3,(%a2)+			# 'x' or 'X'
	#
	# check for zero padding
	#								
L1920:
	mov.l	%d5,%d4				# padding needed?
	beq.b	L1930
	movq	&FMINUS,%d1			# left justify?
	and.b	%d7,%d1
	bne.b	L1930
	bsr.w	L2180				# pad with zeroes
	movq	&0,%d5				# padding done
	#
	# check for zeroes to left of formatted string
	# (e.g. 00004)
	#       ^^^^
	#								
L1930:
	tst.w	%d7				# zeroes to the left?
	bpl.b	L1940
	mov.l	AZERO(%a6),%d4
	bsr.w	L2180				# pad with zeroes
	#
	# Check for decimal point on the left
	#								
L1940:
	add.w	%d7,%d7
	bpl.b	L1960
	subq.l	&1,%d6				# room left in buffer?
	bhs.b	L1950
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	subq.l	&1,%d6				# room for one less
L1950:
	mov.b	RADIX(%a6),(%a2)+		# store a decimal point
	#
	# check for zeroes to left of formatted string
	# (e.g. 0.00004)
	#         ^^^^
	#								
L1960:
	add.w	%d7,%d7				# more zeroes?
	bpl.b	L1970
	mov.l	BZERO(%a6),%d4
	bsr.w	L2180				# pad with zeroes
	#
	# move formatted string
	#								
L1970:
	tst.l	%d2				# actual string length
	beq.b	L2020
L1980:
	sub.l	%d2,%d6				# room in buffer?
	bhs.b	L2010
	add.l	%d2,%d6				# buffer space
	beq.b	L2000
	sub.l	%d6,%d2				# string left
L1990:
	mov.b	(%a5)+,(%a2)+			# move string
	subq.l	&1,%d6
	bne.b	L1990
L2000:
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	bra.b	L1980
L2010:
	mov.b	(%a5)+,(%a2)+			# move string
	subq.l	&1,%d2
	bne.b	L2010
	#
	# check for zeroes to right of formatted string
	# (e.g. 4000000.)
	#        ^^^^^^
	#								
L2020:
	mov.l	BUFEND(%a6),%a5			# end of buffer
	add.w	%d7,%d7				# more zeroes?
	bpl.b	L2030
	mov.l	CZERO(%a6),%d4
	bsr.w	L2180				# pad with zeroes
	#
	# Check for decimal point on the right
	#								
L2030:
	add.w	%d7,%d7
	bpl.b	L2050
	subq.l	&1,%d6				# room left in buffer?
	bhs.b	L2040
	bsr.w	L2230				# flush the buffer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	subq.l	&1,%d6				# room for one less
L2040:
	mov.b	RADIX(%a6),(%a2)+		# store a decimal point
	#
	# check for zeroes to right of formatted string
	# (e.g. 4.0000)
	#         ^^^^
	#								
L2050:
	add.w	%d7,%d7				# more zeroes to the right?
	bpl.b	L2060
	mov.l	DZERO(%a6),%d4
	bsr.w	L2180				# pad with zeroes
	#
	# check for suffix (e.g. 1.3E+14)
	#			    ^^^^
	#								
L2060:
	add.w	%d7,%d7				# have a suffix?
	bpl.b	L2090
	lea	SUFFIXBUF(%a6),%a0
	movq	&3,%d4				# assume four characters
	add.w	%d7,%d7				# calculate size of suffix
	bpl.b	L2070
	tst.b	EXP_3_4(%a6)			# 3 or 4 digit exponent?
	beq.b	L2065
	movq	&5,%d4				# no: it's six characters
	bra.b	L2070
L2065:	
	movq	&4,%d4				# no: it's five characters
L2070:
	subq.l	&1,%d6				# room for one less
	bcc.b	L2080
	mov.l	%a0,%d6				# save pointer
	bsr.w	L2230				# flush the buffer
	mov.l	%d6,%a0				# restore pointer
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	bra.b	L2070
L2080:
	mov.b	(%a0)+,(%a2)+			# move suffix
	dbra	%d4,L2070
	#
	# check for blank padding on the right
	#								
L2090:
	mov.l	%d5,%d4				# padding needed?
	beq.w	L70
	movq	&'\040,%d1			# pad with blanks
	bsr.w	L2190
	bra.w	L70
L2100:
	tst.b	SPRINTF(%a6)			# is this 'sprintf'?
	beq.b	L2110
	mov.l	IOP(%a6),%a0			# stream
	mov.l	%a2,_ptr(%a0)			# update pointer
	mov.l	%a2,%d0				# string pointer
	sub.l	START(%a6),%d0			# number of characters
	bra.w	L2170
L2110:
	mov.l	%a2,%d2				# buffer pointer
	sub.l	START(%a6),%d2			# number of characters
	add.l	COUNT(%a6),%d2			# increment total count
	mov.l	%a2,%d0				# buffer pointer
	mov.l	%a2,%d1
	mov.l	IOP(%a6),%a2			# stream
	sub.l	_ptr(%a2),%d1			# pointer - iop->_ptr
	sub.l	%d1,_cnt(%a2)			# adjust count
	mov.l	%d0,_ptr(%a2)			# update pointer
	add.l	_cnt(%a2),%d0			# in case of recent
	cmp.l	%d0,BUFEND(%a6)			#   interrupt
	bls.b	L2120
	pea	(%a2)
	bsr.l	__bufsync
	add	&4,%sp				# clean off stack
L2120:
	movq	&0,%d0
	ori.w	&_IONBF|_IOLBF,%d0		# unbuffered or line buffered?
	and.b	_flag+1(%a2),%d0
	beq.b	L2160
	subq.b	&_IONBF,%d0			# unbuffered?
	beq.b	L2150
	movq	&EOL,%d0			# end of line character
	mov.l	_ptr(%a2),%d1
	mov.l	_base(%a2),%a0
	sub.l	%a0,%d1				# characters in buffer
	beq.b	L2160
	movq	&0,%d3				# long buffer?
	mov.w	%d1,%d3
	cmp.l	%d1,%d3
	bne.b	L2140
	subq.w	&1,%d1				# counter
L2130:
	cmp.b	%d0,(%a0)+			# look for a linefeed
	dbeq	%d1,L2130
	beq.b	L2150
	bra.b	L2160
L2140:
	cmp.b	%d0,(%a0)+			# look for a linefeed
	beq.b	L2150
	subq.l	&1,%d1
	bne.b	L2140
	bra.b	L2160
L2150:
	pea	(%a2)
	bsr.l	__xflsbuf			# flush the buffer
	add	&4,%sp				# clean off stack
L2160:
	mov.l	%d2,%d0				# character count
	movq	&_IOERR,%d1			# I/O error?
	and.b	_flag+1(%a2),%d1
	beq.b	L2170
	movq	&EOF,%d0			# error: return EOF
L2170:
	movm.l	REGS(%a6),%d2-%d7/%a2-%a5	# restore registers
	unlk	%a6
	rts
	#
	# Output pad characters to buffer
	# 	%d1 = character
	# 	%d4 = number of characters
	#								
L2180:
	movq	&'0,%d1				# zero padding
L2190:
	sub.l	%d4,%d6				# room in buffer?
	bhs.b	L2220
	add.l	%d4,%d6				# buffer space
	beq.b	L2210
	sub.l	%d6,%d4				# padding left
L2200:
	mov.b	%d1,(%a2)+			# fill in pad character
	subq.l	&1,%d6
	bne.b	L2200
L2210:
	mov.b	%d1,%d6				# save pad character
	bsr.b	L2230				# flush the buffer
	mov.b	%d6,%d1				# restore pad character
	mov.l	BUFEND(%a6),%d6			# calculate buffer size
	sub.l	%a2,%d6
	bra.b	L2190
L2220:
	mov.b	%d1,(%a2)+			# fill in blanks
	subq.l	&1,%d4
	bne.b	L2220
	rts
L2230:
	tst.b	SPRINTF(%a6)			# is this 'sprintf'?
	bne.b	L2240
	mov.l	%a2,%d0				# buffer pointer
	mov.l	%a2,%d1
	mov.l	IOP(%a6),%a2			# stream
	sub.l	_ptr(%a2),%d1			# pointer - iop->_ptr
	sub.l	%d1,_cnt(%a2)			# adjust count
	mov.l	%d0,_ptr(%a2)			# update pointer
	sub.l	START(%a6),%d0			# number of characters to flush
	add.l	%d0,COUNT(%a6)			# up the count
	pea	(%a2)
	bsr.l	__xflsbuf			# flush the buffer
	add	&4,%sp				# clean off stack
	mov.l	_ptr(%a2),%a2			# buffer pointer
	mov.l	%a2,START(%a6)			# new starting point
L2240:
	rts
L2250:
	mov.w	&0x7FF0,%d3			# exponent mask
	mov.w	%d3,%d4
	and.w	(%a4),%d3			# get exponent
	cmp.w	%d3,%d4				# inf or NaN?
	bne.b	L2260
	and.w	&0xFFEF,(%a4)			# set to max exponent
	or.l	&0x000FFFFF,(%a4)		# set upper mantissa
	movq	&-1,%d3
	mov.l	%d3,4(%a4)			# set lower mantissa
L2260:
	rts

ifdef(`PIC',`
LPROLOG1:
L2265:	mov.l	&DLT,%a1
	lea.l	L2265+2(%pc,%a1.l),%a1
	rts

LPROLOG0:
L2266:	mov.l	&DLT,%a0
	lea.l	L2266+2(%pc,%a0.l),%a0
	rts
')

L2270:
# Perform the equivalent of a "C" switch statement.  The following code
# is position independant
	mov.l	%d2,%d0
	sub.l	&32,%d0
	lea.l	(L11,%pc,%za0),%a0
	mov.l	(0,%a0,%d0.l*4),%d0
	jmp	2(%pc,%d0.l)
	#
	# jump table for formatting characters
	#
L10:
	lalign	4
L11:
	long	L140-L10			# blank
	long	L90-L10				# !
	long	L90-L10				# double quote
	long	L150-L10			# #
	long	L90-L10				# $
	long	L700-L10			# %
	long	L90-L10				# &
	long	L90-L10				# '
	long	L90-L10				# (
	long	L90-L10				# )
	long	L180-L10			# *
	long	L120-L10			# +
	long	L90-L10				# ,
	long	L130-L10			# -
	long	L160-L10			# .
	long	L90-L10				# /
	long	L200-L10			# 0
	long	L210-L10			# 1
	long	L210-L10			# 2
	long	L210-L10			# 3
	long	L210-L10			# 4
	long	L210-L10			# 5
	long	L210-L10			# 6
	long	L210-L10			# 7
	long	L210-L10			# 8
	long	L210-L10			# 9
	long	L90-L10				# :
	long	L90-L10				# ;
	long	L90-L10				# <
	long	L90-L10				# =
	long	L90-L10				# >
	long	L90-L10				# ?
	long	L90-L10				# @
	long	L90-L10				# A
	long	L90-L10				# B
	long	L90-L10				# C
	long	L90-L10				# D
	long	L810-L10			# E
	long	L90-L10				# F
	long	L1360-L10			# G
	long	L90-L10				# H
	long	L90-L10				# I
	long	L90-L10				# J
	long	L90-L10				# K
	long	L175-L10			# L
	long	L90-L10				# M
	long	L90-L10				# N
	long	L90-L10				# O
	long	LP530-L10			# P	
	long	L90-L10				# Q
	long	L90-L10				# R
	long	L710-L10			# S
	long	L90-L10				# T
	long	L90-L10				# U
	long	L90-L10				# V
	long	L90-L10				# W
	long	L530-L10			# X
	long	L90-L10				# Y
	long	L90-L10				# Z
	long	L90-L10				# [
	long	L90-L10				# \
	long	L90-L10				# ]
	long	L90-L10				# ^
	long	L90-L10				# _
	long	L90-L10				# `
	long	L90-L10				# a
	long	L90-L10				# b
	long	L690-L10			# c
	long	L250-L10			# d
	long	L810-L10			# e
	long	L1020-L10			# f
	long	L1360-L10			# g
	long	L170-L10			# h
	long	L250-L10			# i
	long	L90-L10				# j
	long	L90-L10				# k
	long	L110-L10			# l
	long	L90-L10				# m
	long	LN10-L10			# n
	long	L610-L10			# o
	long	LP530-L10			# p
	long	L90-L10				# q
	long	L90-L10				# r
	long	L710-L10			# s
	long	L90-L10				# t
	long	L500-L10			# u
	long	L90-L10				# v
	long	L90-L10				# w
	long	L530-L10			# x


ifdef(`PROFILE',`
	   data
p__doprnt: long	0
	')
