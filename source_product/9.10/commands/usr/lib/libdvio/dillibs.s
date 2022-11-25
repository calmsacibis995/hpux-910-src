 # HPUX_ID: @(#)dillibs.s	1.39     86/03/11
 #
 #	DIL memory mapped librarys
 #

 #
 #	auxilliary commands
 #
      		set	H_RHDF,0x02	# release holdoff
       		set	H_HDFA0,0x03	# turn off holdoff
       		set	H_HDFA1,0x83	# turn on holdoff
       		set	H_HDFE0,0x04	# off holdoff on end
       		set	H_HDFE1,0x84	# holdoff on end
      		set	H_FEOI,0x08	# assert EOI with byte
     		set	H_GTS,0x0b	# off ATN
     		set	H_TCA,0x0c	# ON ATN
     		set	H_TCS,0x0d	# ON ATN
      		set	H_TON1,0x8a	# enable to talk
      		set	H_TON0,0x0a	# disable to talk
      		set	H_LON1,0x89	# enable to listen
      		set	H_LON0,0x09	# disable to listen
     		set	H_RLC,0x12	# release control
      		set	h_dai1,0x93	# disable interrupts
      		set	h_dai0,0x13	# enable interrupts

      		set	L_BASE,0x20	# listen address base
      		set	T_BASE,0x40	# talk address base
   		set	UNT,0x5f	# un-talk
    		set	UNTP,0xdf	# un-talk
   		set	UNL,0x3f	# un-listen
    		set	UNLP,0xbf	# un-listen
   		set	TCT,0x09	# take control

 #
 #	offsets to ti9914 from (CP)
 #
       		set	EXTSTAT,0-12	# external status register
   		set	AUX,6		# auxcmd register
    		set	DATA,14		# datain/dataout register
 #

	data
eoi_flag:	byte	0	# flag for eoi control
do_pattern:	byte	0	# flag for pattern termination
dil_fd:		long	0	# will save dil fd's
is_dup:		byte	0	# flag for fcntl
dil_ioctl:	byte	0	# flag for ioctl
io_done:	byte	0	# flag for read/write termination
	text
	global	_open_fds	# keep track of open fd's
	global	_dil_fdp	# keep track of DIL fd's



 #******************************************
    	set	HPIBIO,119
	global	_hpibio
		
_hpibio:		
	mov.l	&HPIBIO,%d0
	trap	&0
	bcc.w	hpibionoerror
	jmp	__cerror
hpibionoerror:		
	rts	
 #******************************************





 #
 # DIL library -- open
 #
 # file = open(string, mode)
 #
 # file == -1 means error
		
	global	__cerror

    	set	OPEN,5
	global	_open
		
_open:		
	mov.l	&OPEN,%d0
	trap	&0
	bcc.w	opennoerror
	jmp	__cerror
opennoerror:		
	mov.l	%d0,-(%sp)
	jsr	_get_open_fd_info
	mov.l	(%sp)+,%d0
	rts	
 #
 # C library -- close
 # error =  close(file);
		
     	set	CLOSE,6
	global	__close
		
__close:		
	mov.l	&CLOSE,%d0
	trap	&0
	bcc.w	closenoerror
	jmp	__cerror
closenoerror:		
	clr.l	%d0
	rts	
 #
 # DIL library -- creat
 #
 # file = creat(string, mode)
 #
 # file == -1 means error
		

     	set	CREAT,8
	global	_creat
		
_creat:		
	mov.l	&CREAT,%d0
	trap	&0
	bcc.w	creatnoerror
	jmp	__cerror
creatnoerror:		
	mov.l	%d0,-(%sp)
	jsr	_get_open_fd_info
	mov.l	(%sp)+,%d0
	rts	

 #
 # DIL library -- dup
 #
 # newfd = dup(fd)
 #
 # newfd == -1 means error

   	set	DUP,41
	global	_dup
	global	_dil_fds
	global	_dil_dup_fd


_dup:
	mov.l	4(%sp),dil_fd
	mov.l	&DUP,%d0
	trap	&0
	bcc.w	dupnoerror
	jmp	__cerror
dupnoerror:
	mov.l	%d0,-(%sp)	#push newfd on the stack
	mov.l	dil_fd,-(%sp)	#push oldfd on the stack
	jsr	_dil_dup_fd
	addq.l	&4,%sp		#cleanup the stack
	mov.l	(%sp)+,%d0	#set the return value
	rts


 #
 # DIL library -- fcntl
 #
 # result = fcntl(fd,cmd,arg)
 #
 # result == -1 means error

     	set	FNCTL,62
        set	F_DUPFD,0
	global	_fcntl


_fcntl:
	mov.b	&0,is_dup
	mov.l	8(%sp),%d1	#see if cmd = F_DUPFD
	bne	dofcntl
	mov.b	&1,is_dup	#we are doing a dup
	mov.l	4(%sp),dil_fd
dofcntl:
	mov.l	&FNCTL,%d0
	trap	&0
	bcc.w	fcntlnoerror
	jmp	__cerror
fcntlnoerror:
	tst.b	is_dup		#did we do a dup?
	beq	fcntldone
	mov.l	%d0,-(%sp)	#push newfd on the stack
	mov.l	dil_fd,-(%sp)	#push oldfd on the stack
	jsr	_dil_dup_fd
	addq.l	&4,%sp		#cleanup the stack
	mov.l	(%sp)+,%d0	#set the return value
fcntldone:
	rts

 #
 # C library -- read
		
 # nread = read(file, buffer, count);
 # nread ==0 means eof; nread == -1 means error
		
    	set	READ,3
	global	_read
		
_read:		
	mov.l	4(%sp),%d1	# verify users parameter
	blt	_dilbadfd	
	cmp.l	%d1,&1024
	bge	_dilbadfd

	mov.l	%d1,-(%sp)
	jsr	_io_check
	addq.l	&4,%sp
	tst.l	%d0
	bgt	dil_mem_read
	blt	dil_ioctl_read

	mov.l	&READ,%d0
	trap	&0
	bcc.w	readnoerror
	jmp	__cerror
readnoerror:		
	rts	

dil_mem_read:
	mov.l	4(%sp),%d1
	lsl.l	&2,%d1
	mov.l	&_dil_fdp,%a0
	adda.l	%d1,%a0
	mov.l	(%a0),%d0
	bra	dil_read

dil_ioctl_read:
	mov.l	4(%sp),-(%sp)
	jsr	_dil_set_isc_state
	addq.l	&4,%sp
	mov.l	&READ,%d0
	trap	&0
	bcc.w	ioctl_readnoerror
	mov.l	%d0,-(%sp)
	mov.l	8(%sp),-(%sp)
	jsr	_dil_get_isc_state
	addq.l	&4,%sp
	mov.l	(%sp)+,%d0
	jmp	__cerror
ioctl_readnoerror:
	mov.l	%d0,-(%sp)
	mov.l	8(%sp),-(%sp)
	jsr	_dil_get_isc_state
	addq.l	&4,%sp
	mov.l	(%sp)+,%d0
	rts


 #
 # C library -- write
		
 # nwritten = write(file, buffer, count);
 # nwritten == -1 means error
		
     	set	WRITE,4
	global	_write
		
_write:		
	mov.l	4(%sp),%d1	# verify users parameter
	blt	_dilbadfd	
	cmp.l	%d1,&1024
	bge	_dilbadfd

	mov.l	%d1,-(%sp)
	jsr	_io_check
	addq.l	&4,%sp
	tst.l	%d0
	bgt	dil_mem_write
	blt	dil_ioctl_write

	mov.l	&WRITE,%d0
	trap	&0
	bcc.w	writenoerror
	jmp	__cerror
writenoerror:		
	rts	

dil_mem_write:
	mov.l	4(%sp),%d1
	lsl.l	&2,%d1
	mov.l	&_dil_fdp,%a0
	adda.l	%d1,%a0
	mov.l	(%a0),%d0
	bra	dil_write

dil_ioctl_write:
	mov.l	4(%sp),-(%sp)
	jsr	_dil_set_isc_state
	addq.l	&4,%sp
	mov.l	&WRITE,%d0
	trap	&0
	bcc.w	ioctl_writenoerror
	mov.l	%d0,-(%sp)
	mov.l	8(%sp),-(%sp)
	jsr	_dil_get_isc_state
	addq.l	&4,%sp
	mov.l	(%sp)+,%d0
	jmp	__cerror
ioctl_writenoerror:
	mov.l	%d0,-(%sp)
	mov.l	8(%sp),-(%sp)
	jsr	_dil_get_isc_state
	addq.l	&4,%sp
	mov.l	(%sp)+,%d0
	rts
 #
 # C library -- ioctl
		
     	set	IOCTL,54
	global	_ioctl
		
_ioctl:		
	mov.b	&0,dil_ioctl
	mov.l	4(%sp),%d1	# verify users parameter
	blt	_dilbadfd	
	cmp.l	%d1,&1024
	bge	_dilbadfd
	mov.l	%d1,dil_fd

 	mov.l	12(%sp),-(%sp)		# push arg
	mov.l	12(%sp),-(%sp)		# push cmd
 	mov.l	%d1,-(%sp)		# push fd
 	jsr	_dil_ioctl_check
 	add.l	&12,%sp			# cleanup stack

 	tst.l	%d0
 	beq	do_ioctl
 	mov.b	&1,dil_ioctl

do_ioctl:
	mov.l	&IOCTL,%d0
	trap	&0
	bcc.w	ioctlnoerror

 	tst.b	dil_ioctl
 	beq	ioctl_error

 	mov.l	%d0,-(%sp)
 	mov.l	16(%sp),-(%sp)
 	mov.l	dil_fd,-(%sp)
 	jsr	_dil_ioctl_set
 	addq.l	&8,%sp
 	mov.l	(%sp)+,%d0
ioctl_error:
	jmp	__cerror

ioctlnoerror:		
 	tst.b	dil_ioctl
 	beq	ioctl_done

 	mov.l	%d0,-(%sp)
 	mov.l	16(%sp),-(%sp)
 	mov.l	dil_fd,-(%sp)
 	jsr	_dil_ioctl_set
 	addq.l	&8,%sp
 	mov.l	(%sp)+,%d0

ioctl_done:
	rts

	global	_dilbadfd
_dilbadfd:
	mov.l &9,%d0		# EBADF -> d0
	jmp	__cerror

dil_write:
	movm.l	%d2-%d3/%a2-%a4,-(%sp)		# save registers
	mov.l %d0,%a3				# get pointer to structure
	mov.b	FD_CARD_TYPE(%a3),%d1		# get card type
	cmp.b	%d1,&HP98622			# gpio card?
	beq	gpio_write
	mov.b	&0,eoi_flag			# clear eoi flag
	cmp.b	%d1,&HP98625			# simon card?
	beq	simon_write
	bra	ti9914_write			# must be ti9914

dil_read:
	movm.l	%d2-%d3/%a2-%a4,-(%sp)		# save registers
	mov.l %d0,%a3				# get pointer to structure
	mov.b	FD_CARD_TYPE(%a3),%d1		# get card type
	cmp.b	%d1,&HP98622			# gpio card?
	beq	gpio_read
	cmp.b	%d1,&HP98625			# simon card?
	beq	simon_read
	bra	ti9914_read			# must be ti9914

 #****************************************************************
 #								*
 #	TI9914 DIL memory mapped version			*
 #								*
 #****************************************************************
 #
 #	TI9914 fast handshake in routine
 #

	global	_ti9914_read

_ti9914_read:
ti9914_read:
	mov.w	(%a3),%d0			# get bus address
	mov.b	&0,do_pattern			# clear flag for read pattern
	btst	&14,%d0				# read pattern termination?
	beq.b	skip_pattern			# yes, go doit
	mov.b	&1,do_pattern			# set flag for read pattern
	mov.b	FD_PATTERN+1(%a3),%d3 		# get termination character
skip_pattern:
	mov.l FD_CP(%a3),%a1			# get pointer to instat reg
	mov.b	&h_dai1,AUX(%a1)		# dis-enable interrupts
	mov.b	&0x20,%d2			# for quick comparison
	cmp.b	%d0,&0x1f			# Raw device?
	beq.b	in_no_auto			# yes, skip
	pea	(%a3)
	jsr	_ti9914_atn			# set atn
	addq.l	&4,%sp
  
	mov.b	&UNLP,DATA(%a1)			# send unlisten
rbo1:
	btst	&4,(%a1)			# BO logged?
	beq.b	rbo1				# we didn't a bo
  
	or.b	&T_BASE,%d0			# add talk base
	mov.l	&_odd_partab,%a2		# get odd parity table
	and.w	&0xff,%d0			# clear out other bits
	adda.w	%d0,%a2				# get pointer to byte
	mov.b	(%a2),DATA(%a1)			# send  OTHER TALK ADDRESS
	mov.b	&H_LON1,AUX(%a1)		# tell dump chip it's listener
rbo2:
	btst	&4,(%a1)			# BO logged?
	beq.b	rbo2				# we didn't a bo
  
	mov.b	FD_CARD_ADDRESS(%a3),%d0	# get card address
	or.b	&L_BASE,%d0			# add listen base
	mov.l	&_odd_partab,%a2		# get odd parity table
	and.w	&0xff,%d0			# clear out other bits
	adda.w	%d0,%a2				# get pointer to byte
	mov.b	(%a2),DATA(%a1)			# send  MY LISTEN ADDRESS
rbo3:
	btst	&4,(%a1)			# BO logged?
	beq.b	rbo3				# we didn't a bo
  
in_no_auto:
	mov.b	&H_GTS,AUX(%a1)			# drop ATN
	mov.b	&H_HDFA0,AUX(%a1)		# turn off holdoff
	mov.b	&H_HDFE1,AUX(%a1)		# turn on holdoff on end
	mov.b	&0,FD_REASON(%a3)		# clear termination reason

	mov.l	FD_D_SC(%a3),%a4
	mov.b	1(%a4),%d0
	and.b	&1,%d0
	beq.b	no_rhdf				# no - skip it
	mov.b	&H_RHDF,AUX(%a1)		# release holdoff
no_rhdf:
	and.b	&0xfe,1(%a4)
	mov.l	28(%sp),%a0			# get users buffer
	mov.l	32(%sp),%d0			# get users count
	lea	DATA(%a1),%a2			# make pointer to datain reg
	tst.b	do_pattern			# see if we need to do this
	bne	read_pat			# yes, go do pattern read
  
	subq.l	&2,%d0				# count - 2
	bge.b	fti_w1				# test BI (n-1 loop)
	bra.b	fti_w2				# test BI (one byte)

 	##
 	## high speed for n-1 bytes
 	##
fti_w1:

	mov.b	(%a1),%d1			# get card status
	beq.b	fti_w1				# loop till we get something
	cmp.b	%d2,%d1				# BI status only
	bne.b	fti_s1				# no, process other conditions

fti_t1:

	mov.b	(%a2),(%a0)+			# get data byte
	dbra	%d0,fti_w1			# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# re-adjust count
	bge.b	fti_w1				# do nest 64k
	##
 	# 	last byte handling
 	#
fti_w2:

	mov.b	(%a1),%d1			# get card status
	beq.b	fti_w2				# loop until we get something
	cmp.b	%d2,%d1				# BI status only?
	beq.b	fti_t2				# yes, get the byte

	## probably saw EOI
	bclr	&5,%d1				# see if BI was logged
	beq.b	fti_w2				# no, keep waiting

fti_t2:
	mov.b	&H_HDFA1,AUX(%a1)		# turn on holdoff
	mov.b	&H_HDFE0,AUX(%a1)		# off end holdoff
	or.b	&1,1(%a4)			# we are in holdoff
	mov.b	(%a2),(%a0)+			# get last byte
	##
	##	if we get here it was normal termination
	##
	movq	&0,%d0				# a good termination
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	bra.b	eoi_check			# check for EOI
matched:	
	addq.l	&1,%d0				# adjust transfer count
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termination reason
eoi_check:
	bclr	&3,%d1				# was EOI set?
	beq.b	read_cleanup
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
	or.b	&1,1(%a4)			# we are in holdoff
read_cleanup:
	##	don't think I need to adjust count
	mov.b	&H_HDFA1,AUX(%a1)		# turn on holdoff
	mov.b	&H_HDFE0,AUX(%a1)		# off end holdoff
	bra	terminate			# leave

 	##
 	## special status handling n-l loop
 	##

fti_s1:
	bclr    &3,%d1                          # was EOI set?
	beq.b   bi_no_eoi                       # no, check for BI
	bclr    &5,%d1                          # BI logged?
	bne.b   bi_got_bi                       # yes, get llast byte

	##
	## We saw EOI but we didnt get BI.  This is a hardware failure
	## in the TI9914A chip.  Wait until we get BI and assume that
	## the EOI we saw previously was intended for this byte.
	##

bi_wait_bi:
	   mov.b   (%a1),%d1                       # get card status
	   beq.b   bi_wait_bi                      # loop till we get something
	   bclr    &5,%d1                          # BI logged?
	   beq.b   bi_wait_bi                      # no, keep waiting

bi_got_bi:
	mov.b   (%a2),(%a0)                     # get last byte
	addq.l  &1,%d0                          # adjust transfer count
	or.b    &TR_END,FD_REASON(%a3)          # set termination reason
	or.b    &1,1(%a4)                       # we are in holdoff
	bra     read_cleanup                    # leave
	rts
bi_no_eoi:
	bclr    &5,%d1                          # BI logged?
	beq     fti_w1                          # if not, keep testing status
	bra     fti_t1                          # yes, go get the byte





read_pat:
 	## need to handshake data one at a time, release holdoff
	mov.b	&H_HDFA1,AUX(%a1)		# turn on holdoff
	mov.b	&H_HDFE0,AUX(%a1)		# off end holdoff
 	##
	subq.l	&2,%d0				# count - 2
	bge.b	pfti_w1_start			# test BI (n-1 loop)
	bra.b	pfti_w2				# test BI (one byte)
 	##
 	##	high speed for n-1 bytes
 	##
pfti_w1:
	mov.b	&H_RHDF,AUX(%a1)		# release holdoff
pfti_w1_start:
	mov.b	(%a1),%d1			# get card status
	beq.b	pfti_w1_start			# loop till we get something
	cmp.b	%d2,%d1				# BI status only
	bne.b	pfti_s1				# no, process other conditions

pfti_t1:

	or.b	&1,1(%a4)			# we are in holdoff
	mov.b	(%a2),(%a0)			# get data byte
	cmp.b	%d3,(%a0)+			# check for early termination
	beq.w	matched				# yes, clean up
	bclr	&3,%d1				#was EOI set?
	beq.b	no_eoi_1	
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
	or.b	&1,1(%a4)			# we are in holdoff
	addq.l	&1,%d0				# adjust transfer count
	bra	read_cleanup
no_eoi_1:
	dbra	%d0,pfti_w1			# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# re-adjust count
	bge.b	pfti_w1				# do next 64k
	mov.b	&H_RHDF,AUX(%a1)		# release holdoff
	##
	## 	last byte handling
	##
pfti_w2:
	mov.b	(%a1),%d1			# get card status
	beq.b	pfti_w2				# loop until we get something
	cmp.b	%d2,%d1				# BI status only?
	beq.b	pfti_t2				# yes, get the byte

	## probably saw EOI
	bclr	&5,%d1				# see if BI was logged
	beq.b	pfti_w2				# no, keep waiting

pfti_t2:
	mov.b	&H_HDFA1,AUX(%a1)		# turn on holdoff
	mov.b	&H_HDFE0,AUX(%a1)		# off end holdoff
	or.b	&1,1(%a4)			# we are in holdoff
	mov.b	(%a2),(%a0)			# get last byte
	cmp.b	%d3,(%a0)+			# did we match?
	bne.b	no_match
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termination reason
no_match:
	bclr	&3,%d1				# was EOI set
	beq.b	no_eoi_2
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
no_eoi_2:
	##
	##	if we get here it was normal termination
	##
	movq	&0,%d0				# a good termination
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	bra	read_cleanup			# Yes, leave
	##
	##	special status handling n-l loop
	##
pfti_s1:
	bclr	&5,%d1				# BI logged?
	beq.w	pfti_w1				# if not, keep testing status
	bclr	&3,%d1				# BI set, was EOI?
	beq.w	pfti_t1				# no, go transfer the byte
	mov.b	(%a2),(%a0)			# get last byte
	cmp.b	%d3,(%a0)			# did we match?
	bne.b	no_match_2
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termination reason
no_match_2:
	addq.l	&1,%d0				# adjust transfer count
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
	or.b	&1,1(%a4)			# we are in holdoff
	bra	read_cleanup			# leave
	rts
	##
	## fast handshake out routine
	##
	global	_ti9914_write

_ti9914_write:			# JJE ??
ti9914_write:
	mov.w	FD_STATE(%a3),%d0		# get state
	bpl.b	skip_eoi			# no eoi? then skip
	mov.b	&1,eoi_flag			# mark we should write an eoi
skip_eoi:
	mov.l   FD_CP(%a3),%a1			# get card pointer to instat reg
	mov.b	&h_dai1,AUX(%a1)		# dis-enable interrupts
	mov.b	&0x10,%d2			# for quick comparison
	mov.b	FD_BA(%a3),%d0			# get bus address
	cmp.b	%d0,&0x1f			# Raw device?
	beq.b	out_no_auto			# yes, skip

	#
	# Auto addressing sequence
	#
	pea	(%a3)
	jsr	_ti9914_atn			# set atn
	addq.l	&4,%sp
  
	mov.b	&UNLP,DATA(%a1)			# send unlisten
wbo1:
	btst	&4,(%a1)			# BO logged?
	beq.b	wbo1				# no, keep waiting
  
	mov.w	&T_BASE,%d1			# add talk base
	or.b	FD_CARD_ADDRESS(%a3),%d1	# get card address
	mov.l	&_odd_partab,%a2		# get odd parity table
	adda.w	%d1,%a2				# get pointer to byte
	mov.b	(%a2),DATA(%a1)			# send  MY TALK ADDRESS
	mov.b	&H_TON1,AUX(%a1)		# tell dump chip it's a talker
wbo2:
	btst	&4,(%a1)			# BO logged?
	beq.b	wbo2				# no, keep waiting
  
	or.b	&L_BASE,%d0			# add listen base
	mov.l	&_odd_partab,%a2		# get odd parity table
	and.w	&0xff,%d0			# clear out other bits
	adda.w	%d0,%a2				# get pointer to byte
	mov.b	(%a2),DATA(%a1)			# send  HIS TALK ADDRESS
wbo3:
	btst	&4,(%a1)			# BO logged?
	beq.b	wbo3				# no, keep waiting
  
out_no_auto:
	mov.l	28(%sp),%a0			# get users buffer
	mov.l	32(%sp),%d0			# get users count
	lea	DATA(%a1),%a2			# make pointer to datain reg
	mov.b	&H_GTS,AUX(%a1)			# drop ATN

 	##
 	## fast handshake out routine
 	##
	subq.l	&2,%d0				# count - 2
	bge.b	fto_t1				# loop n-1 times
	bra	fto_last			# send last byte

 	##
 	## high speed loop for n-1 bytes
 	##
fto_w1:
	btst	&4,(%a1)			# BO logged?
	beq.b	fto_w1				# no, keep waiting
fto_t1:
	mov.b	(%a0)+,(%a2)			# transfer byte out
	dbra	%d0,fto_w1			# loop until lower count out
	clr.w	%d0				# check for total count
	subq.l	&1,%d0				# adjust count
	bge.b	fto_w1				# do next 64K
fto_w2:
	btst	&4,(%a1)			# BO logged?
	beq.b	fto_w2				# no, keep waiting

 	# 
 	# last byte handling
 	# check for EOI control here
 	#
fto_last:
	tst.b	eoi_flag
	beq.b	no_eoi
	mov.b	&H_FEOI,AUX(%a1)		# send EOI with this byte
no_eoi:
	mov.b	(%a0)+,(%a2)			# transfer last byte out
fto_w3:
	btst	&4,(%a1)			# BO logged?
	beq.b	fto_w3				# no, keep waiting
  
	movq	&0,%d0				# a good termination
 	##
 	## FHS transfer termination
 	##
terminate:
	mov.b	&h_dai0,AUX(%a1)		# re-enable interrupts
	mov.b	&0,eoi_flag			# clear eoi flag
	mov.l	32(%sp),%d2			# get initial count
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value
	movm.l	(%sp)+,%d2-%d3/%a2-%a4		# save registers
	rts					# return

 ##################################################################
 #
 # ti9914_atn(*fd_info)
 #
	global	_ti9914_atn

_ti9914_atn:
	movm.l	%d2/%a2-%a4,-(%sp)	# save registers
	mov.l	20(%sp),%a2		# get the fd_info pointer
	mov.l	FD_CP(%a2),%a3		# get the card pointer
	mov.l	FD_D_SC(%a2),%a4	# get the sc_info pointer
	mov.b	4(%a3),%d2		# read the address state register
	btst	&5,%d2			# is ATN set?
	beq	set_atn			# if not then set it
	movm.l	(%sp)+,%d2/%a2-%a4	# already set so lets go home
	rts
set_atn:
	btst	&2,%d2			# are we listener?
	beq	set_atn_tca		# if not the go async
	btst	&0,1(%a4)		# are we in holdoff?
	beq	set_atn_tca		# if not go async
	mov.b	&H_TCS,AUX(%a3)		# take control synchronously
set_atn_wait_bo:
	btst	&4,(%a3)		# do we have bo?
	beq	set_atn_wait_bo		# if not keep looking
	movm.l	(%sp)+,%d2/%a2-%a4	# all done so lets go home
	rts
set_atn_tca:
	mov.b	&H_TCA,AUX(%a3)		# take control asynchronously
	bra.b	set_atn_wait_bo		# wait for bo



 #*********************************************************************
 #
 # hpib_send_map(*fd_info, *buffer, count);	/* send data with ATN set */
 #
	global _hpib_send_map			# byte out routine

_hpib_send_map:
	movm.l	%d2-%d4/%a2-%a3,-(%sp)		# save registers
	mov.l	24(%sp),%a3			# get fd_info pointer
	mov.b	FD_CARD_TYPE(%a3),%d1		# get card type
	cmp.b	%d1,&HP98625			# simon card?
	beq	simon_send

	mov.l FD_CP(%a3),%a1			# get card pointer
	mov.b	EXTSTAT(%a1),%d0		# get extstatus
	and.b	&0x40,%d0			# are we active controller?
	beq.b	am_controller
	movq	&-1,%d0				# not controller - cant do!
	movm.l	(%sp)+,%d2-%d4/%a2-%a3		# restore registers
	rts
am_controller:	
	mov.b	FD_CARD_ADDRESS(%a3),%d4	# get my address
	movq	&0,%d0
	pea	(%a3)
	jsr	_ti9914_atn
	addq.l	&4,%sp
	mov.l	28(%sp),%a0			# get buffer pointer
	mov.l	32(%sp),%d2			# get count
	##
	## loop for the data
	##
loop:
	mov.b	(%a0)+,%d0			# get data from buffer
	btst	&7,(%a3)			# check for parity
	beq	no_parity
	and.b	&0x7f,%d0			# mask out parity
	mov.l	&_odd_partab,%a2		# get odd parity table
	adda.w	%d0,%a2				# add offset
	mov.b	(%a2),DATA(%a1)			# write out the byte
	bra	check_byte
no_parity:
	mov.b	%d0,DATA(%a1)			# write out the byte
check_byte:
	mov.b	%d4,%d3				# make copy of my address
	or.b	&0x20,%d3			# make listen address
	cmp.b	%d0,%d3				# see if my listen address
	bne.b	talker				# no, try talker
	mov.b	&H_LON1,AUX(%a1)		# tell dumb chip it's listener
	bra.b	bottom_loop			# now get out of here
talker:
	eor.b	&0x60,%d3			# change my addressing
	cmp.b	%d0,%d3				# see if my talk address
	bne.b	unlisten			# no, try unlisten
	mov.b	&H_TON1,AUX(%a1)		# tell dumb chip it's a talker
	bra.b	bottom_loop			# now get out of here
unlisten:
	cmpi.b	%d0,&UNL			# see if UNL
	bne.b	untalk				# no, try untalk
	mov.b	&H_LON0,AUX(%a1)		# tell dumb chip to unlisten
	bra.b	bottom_loop			# now get out of here
untalk:
	cmpi.b	%d0,&UNT			# see if UNT
	bne.b	tct				# no, try TCT
	mov.b	&H_TON0,AUX(%a1)		# tell dumb chip to untalk
	bra.b	bottom_loop			# now get out of here
tct:
	cmpi.b	%d0,&TCT			# see if TCT
	bne.b	otalk				# no, try other talk
	mov.b	4(%a1),%d0			# get address state
	btst	&1,%d0				# in talk state?
	bne.b	otalk				# no, try other talk address
tct_wbo:
	btst	&4,(%a1)			# BO logged?
	beq.b	tct_wbo				# no, keep looking
	mov.b	&H_RLC,AUX(%a1)			# tell chip to release control
	bra.b	leave				# now get out of here
otalk:
	andi.b	&0x60,%d0			# mask control bits
	cmpi.b	%d0,&0x40			# other talk address?
	bne.b	bottom_loop			# no, let's stop this
	mov.b	&H_TON0,AUX(%a1)		# tell chip it's not a talker
bottom_loop:
	btst	&4,(%a1)			# BO logged?
	beq.b	bottom_loop			# no, keep looking
	subq.l	&1,%d2				# --count
	bne	loop				# keep going
leave:
	clr.l	%d0				# return value
	movm.l	(%sp)+,%d2-%d4/%a2-%a3		# restore registers
	rts


 #****************************************************************
 #								*
 #	DIL SIMON memory mapped version				*
 #								*
 #****************************************************************

simon_send:
	mov.l 	FD_CP(%a3),%a1			# get card pointer
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	bne.b	simon_am_controller		# not controller - cant do!
	movq	&-1,%d0				# error
	movm.l	(%sp)+,%d2-%d4/%a2-%a3		# restore registers
	rts
simon_am_controller:	
	mov.b	&0,SIM_CTRL(%a1)		# disable interrupts
	mov.b	&0,PHI_STATUS(%a1)
	mov.b	&0x20,PHI_IMSK(%a1)
	or.b	&P_ROOM_IDLE,PHI_IMSK(%a1)	# fifo_room, fifo_idle
	mov.l	28(%sp),%a0			# get buffer pointer
	mov.l	32(%sp),%d2			# get count
	##
	## loop for the data
	##
simon_loop:
	btst	&3,PHI_INTR(%a1)		# wait for fifo room
	beq	simon_loop
	##
	mov.b	&P_FIFO_ATN,PHI_STATUS(%a1)	# ATN
	mov.b	(%a0)+,PHI_FIFO(%a1)		# send data
	subq.l	&1,%d2
	bne	simon_loop
simon_loop_1:
	btst	&1,PHI_INTR(%a1)		# wait for fifo idle
	beq	simon_loop_1
	and.b	&CLR_IDLE_ROOM,PHI_IMSK(%a1)
	or.b	&0x20,PHI_IMSK(%a1)
	mov.b	&S_ENAB,SIM_CTRL(%a1)		# enable interrupts
	clr.l	%d0				# return value
	movm.l	(%sp)+,%d2-%d4/%a2-%a3		# restore registers
	rts

	global _sim_read
	global _sim_write

_sim_read:
simon_read:
	mov.w	(%a3),%d0			# get state
	mov.b	&0,do_pattern			# clear flag for read pattern
	btst	&14,%d0				# read pattern termination?
	beq.b	simon_skip_pattern		# yes, go doit
	mov.b	&1,do_pattern			# set flag for read pattern
	mov.b	FD_PATTERN+1(%a3),%d3 		# get termination character
simon_skip_pattern:
	mov.l 	FD_CP(%a3),%a1			# get card pointer
	mov.b	&0,SIM_CTRL(%a1)		# disable interrupts
	mov.b	&0,PHI_STATUS(%a1)
	mov.b	&0,PHI_IMSK(%a1)
	cmp.b	%d0,&0x1f			# Raw device?
	beq.b	simon_in_no_auto		# yes, skip

	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	bne.b	r_simon_am_controller
	movm.l	(%sp)+,%d2-%d3/%a2-%a4		# restore registers
	movq	&22,%d0				# EINVAL -> d0
	jmp	__cerror
r_simon_am_controller:	
	or.b	&P_FIFO_IDLE,PHI_IMSK(%a1)
simon_wait:
	btst	&1,PHI_INTR(%a1)		# fifo idle?
	beq	simon_wait
  
	mov.b	&P_FIFO_ATN,PHI_STATUS(%a1)	# ATN
	mov.b	&UNL,PHI_FIFO(%a1)		# send unlisten
  
	or.b	&T_BASE,%d0			# add talk base
	mov.b	%d0,PHI_FIFO(%a1)		# send talk address
  
	mov.b	&MA,%d0				# get card address
	or.b	&L_BASE,%d0			# add listen base
	mov.b	%d0,PHI_FIFO(%a1)		# send my listen address
  
simon_wait_1:
	btst	&1,PHI_INTR(%a1)		# fifo idle?
	beq	simon_wait_1
	and.b	&CLR_IDLE,PHI_IMSK(%a1)
  
simon_in_no_auto:
	mov.l	32(%sp),%d0			# get users count
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	beq.b	simon_in_not_controller
	or.b	&P_FIFO_IDLE,PHI_IMSK(%a1)
	cmp.l	%d0,&256
	bgt	simon_uncounted
	mov.b	&P_FIFO_LF_INH,PHI_STATUS(%a1)	# counted transfer
	mov.b	%d0,PHI_FIFO(%a1)
	bra.w	simon_in_not_controller
simon_uncounted:
	mov.b	&P_FIFO_UCT_XFR,PHI_STATUS(%a1)	# uncounted transfer
	mov.b	&0,PHI_FIFO(%a1)
simon_in_not_controller:	
	or.b	&P_FIFO_BYTE,PHI_IMSK(%a1)
	mov.b	&0,FD_REASON(%a3)		# clear termination reason
	mov.l	28(%sp),%a0			# get users buffer
	subq.l	&1,%d0				# count - 1
	tst.b	do_pattern			# see if we need to do this
	bne	simon_read_pat			# yes, go do pattern read
  
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	beq.b	n_in_test
	bra.w	in_test				# start in middle of loop      
in_byte:	
	mov.b	PHI_FIFO(%a1),(%a0)+		# transfer a byte              
in_test:	
	cmp.b	PHI_INTR(%a1),&P_FIFO_BYTE	# fifo byte only?              
	dbne	%d0,in_byte			# if so, decrement and transfer

	beq.w	in_count			# branch if WORD count expired 
	btst	&1,PHI_INTR(%a1)		# fifo idle bit set?           
	beq	in_test				# if not, loop
	addq.l	&1,%d0				# fix up count
	bra.w	in_idle				# if so, start looking for EOI 

in_count:	
	clr.w	%d0				# clear lower WORD only        
	subq.l	&1,%d0				# decrement entire LONG        
	bhi.w	in_byte				# branch if count not expired  
	addq.l	&2,%d0				# fix up count
	bra	in_idle

n_in_byte:	
	mov.b	PHI_FIFO(%a1),(%a0)+		# transfer a byte              
	btst	&7,PHI_STATUS(%a1)		# eoi?
	beq	n_in_test
	btst	&6,PHI_STATUS(%a1)
	beq	n_in_test
	or.b	&TR_END,FD_REASON(%a3)		# set termnation reason
	addq.l	&1,%d0				# fix up count        
	bra	in_term
n_in_test:	
	cmp.b	PHI_INTR(%a1),&P_FIFO_BYTE	# fifo byte only?              
	dbne	%d0,n_in_byte			# if so, decrement and transfer

	beq.w	n_in_count			# branch if WORD count expired 
	btst	&1,PHI_INTR(%a1)		# fifo idle bit set?           
	beq	n_in_test				# if not, loop
	addq.l	&1,%d0				# fix up count
	bra.w	in_idle				# if so, start looking for EOI 

n_in_count:	
	clr.w	%d0				# clear lower WORD only        
	subq.l	&1,%d0				# decrement entire LONG        
	bhi.w	n_in_byte			# branch if count not expired  
	addq.l	&2,%d0				# fix up count
in_idle:					                             
	btst	&2,PHI_INTR(%a1)		# fifo byte bit set?           
	beq	in_idle				# loop till we get something
	mov.b	PHI_FIFO(%a1),(%a0)+		# transfer a byte              
	subq.l	&1,%d0				# decrement entire LONG        
	bgt	r_no_count
	mov.b	&1,io_done
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termnation reason
r_no_count:
	btst	&7,PHI_STATUS(%a1)		# eoi?
	beq	simon_no_eoi
	btst	&6,PHI_STATUS(%a1)
	beq	simon_no_eoi
	mov.b	&1,io_done
	or.b	&TR_END,FD_REASON(%a3)		# set termnation reason
simon_no_eoi:
	tst.b	io_done
	beq	in_idle
in_term:					                             
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	beq.b	term_idle
	btst	&1,PHI_INTR(%a1)		# fifo idle bit set?           
	bne	term_idle
	and.b	&P_NOT_REN,PHI_CTRL(%a1)	# clear ren
	or.b	&P_IFC,PHI_CTRL(%a1)		# set ifc
	or.b	&P_INIT_FIFO,PHI_CTRL(%a1)	# init fifo
	#	delay for 100 us here
	and.b	&P_NOT_IFC,PHI_CTRL(%a1)	# clear ifc
	or.b	&P_REN,PHI_CTRL(%a1)		# set ifc
term_idle:
	btst	&2,PHI_INTR(%a1)		# fifo byte bit set?           
	beq	in_exit
	mov.b	PHI_FIFO(%a1),%d3		# cleanup inbound fifo
	bra	term_idle
in_exit:					                             
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	beq.b	simon_not_controller
	and.b	&CLR_IDLE,PHI_IMSK(%a1)
simon_not_controller:
	and.b	&CLR_BYTE,PHI_IMSK(%a1)
	bra	simon_terminate

simon_read_pat:
	bra.w	p_in_test			# start in middle of loop      
p_in_byte:	
	mov.b	PHI_FIFO(%a1),(%a0)		# transfer a byte              
	cmp.b	%d3,(%a0)+			# did we match?
	beq	p_simon_matched
	btst	&7,PHI_STATUS(%a1)		# eoi?
	beq	p_in_test
	btst	&6,PHI_STATUS(%a1)
	beq	p_in_test
	or.b	&TR_END,FD_REASON(%a3)		# set termnation reason
	addq.l	&1,%d0				# fix up count        
	mov.b	&1,io_done
	bra	p_no_match
p_in_test:	
	cmp.b	PHI_INTR(%a1),&P_FIFO_BYTE	# fifo byte only?              
	dbne	%d0,p_in_byte			# if so, decrement and transfer

	beq.w	p_in_count			# branch if WORD count expired 
	btst	&1,PHI_INTR(%a1)		# fifo idle bit set?           
	beq	p_in_test			# if not, loop
	addq.l	&1,%d0				# fix up count
	bra.w	p_in_idle			# if so, start looking for EOI 

p_simon_matched:
	addq.l	&1,%d0				# fix up count
	bra	simon_matched

p_in_count:	
	clr.w	%d0				# clear lower WORD only        
	subq.l	&1,%d0				# decrement entire LONG        
	bhi.w	p_in_byte			# branch if count not expired  
	addq.l	&2,%d0				# fix up count

p_in_idle:					                             
	btst	&2,PHI_INTR(%a1)		# fifo byte bit set?           
	beq	p_in_idle			# loop till we get something
	mov.b	PHI_FIFO(%a1),(%a0)		# transfer a byte              
	subq.l	&1,%d0				# decrement entire LONG        
	cmp.b	%d3,(%a0)+			# did we match?
	bne	p_no_match
simon_matched:
	mov.b	&1,io_done
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termnation reason
p_no_match:
	btst	&7,PHI_STATUS(%a1)		# eoi?
	beq	p_no_eoi
	btst	&6,PHI_STATUS(%a1)
	beq	p_no_eoi
	mov.b	&1,io_done
	or.b	&TR_END,FD_REASON(%a3)		# set termnation reason
p_no_eoi:
	tst.l	%d0				# count
	bne	p_no_count
	mov.b	&1,io_done
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termnation reason
p_no_count:
	tst.b	io_done				# are we done?
	beq	p_in_idle
	bra	in_term

	##
	## fast handshake out routine
	##
_sim_write:
simon_write:
	mov.w	(%a3),%d0			# get state
	bpl.b	simon_skip_eoi			# no eoi? then skip
	mov.b	&1,eoi_flag			# mark we should write an eoi
simon_skip_eoi:
	mov.l 	FD_CP(%a3),%a1			# get card pointer
	mov.b	&0,SIM_CTRL(%a1)		# disable interrupts
	mov.b	&0,PHI_STATUS(%a1)
	mov.b	&0,PHI_IMSK(%a1)
	cmp.b	%d0,&0x1f			# Raw device?
	beq.b	simon_out_no_auto		# yes, skip
	btst	&4,PHI_STATUS(%a1)		# are we active controller?
	bne.b	w_simon_am_controller
	movm.l	(%sp)+,%d2-%d3/%a2-%a4		# restore registers
	movq	&22,%d0				# EINVAL -> d0
	jmp	__cerror
w_simon_am_controller:	
	or.b	&P_FIFO_IDLE,PHI_IMSK(%a1)
	##
w_simon_wait:
	btst	&1,PHI_INTR(%a1)		# fifo idle?
	beq	w_simon_wait
  
	mov.b	&P_FIFO_ATN,PHI_STATUS(%a1)	# ATN
	mov.b	&UNL,PHI_FIFO(%a1)		# send unlisten
  
	mov.w	&T_BASE,%d1			# add talk base
	or.b	&MA,%d1				# get card address
	mov.b	%d1,PHI_FIFO(%a1)		# send talk address
  
	or.b	&L_BASE,%d0			# add listen base
	mov.b	%d0,PHI_FIFO(%a1)		# send listen address
  
w_simon_wait_1:
	btst	&1,PHI_INTR(%a1)		# fifo idle?
	beq	w_simon_wait_1
	and.b	&CLR_IDLE,PHI_IMSK(%a1)
simon_out_no_auto:
	mov.l	28(%sp),%a0			# get users buffer
	mov.l	32(%sp),%d0			# get users count
	or.b	&P_FIFO_ROOM,PHI_IMSK(%a1)
	##
	##	fast handshake out routine
	##
	subq.l	&1,%d0
	ble	out_last

	bra.w	out_test			# start in middle of loop      
out_byte:		
	mov.b	(%a0)+,PHI_FIFO(%a1)		# transfer a byte              
out_test:		
	cmp.b	PHI_INTR(%a1),&P_FIFO_ROOM	# fifo room (only)?            
	dbne	%d0,out_byte			# if so, decrement and transfer

	beq.w	out_count			# branch if WORD count expired 
	bra.w	out_test			# resume polling               

out_count:		
	clr.w	%d0				# clear lower WORD only        
	subq.l	&1,%d0				# decrement entire LONG        
	bhi.w	out_byte			# branch if count not expired  
out_last:
	btst	&3,PHI_INTR(%a1)		# fifo room?            
	beq	out_last
	## check for EOI control here
	##
	mov.b	&0,PHI_STATUS(%a1)
	tst.b	eoi_flag
	beq.b	w_simon_no_eoi
	mov.b	&P_FIFO_EOI,PHI_STATUS(%a1)	# send EOI with this byte
w_simon_no_eoi:
	mov.b	(%a0)+,PHI_FIFO(%a1)		# transfer a byte              
  
	movq	&0,%d0				# a good termination
	and.b	&CLR_ROOM,PHI_IMSK(%a1)
	##
	##	FHS transfer termination
	##
simon_terminate:
	or.b	&0x20,PHI_IMSK(%a1)		# enable ppoll rupt
	mov.b	&S_ENAB,SIM_CTRL(%a1)		# enable interrupts
	mov.b	&0,eoi_flag			# clear eoi flag
	mov.b	&0,io_done			# clear io_done flag
	mov.l	32(%sp),%d2			# get initial count
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value
	movm.l	(%sp)+,%d2-%d3/%a2-%a4		# save registers
	rts					# return

 #****************************************************************
 #								*
 #	DIL GPIO memory mapped version				*
 #								*
 #****************************************************************
 #
 #	GPIO fast handshake in routine
 #
gpio_read:
	mov.b	&0,FD_REASON(%a3)		# clear termination reason
	mov.b	&0,do_pattern			# clear flag for read pattern
	mov.w	(%a3),%d1			# get dil state
	btst	&14,%d1				# read pattern termination?
	beq.b	gpio_skip_pattern		# yes, go doit
	mov.b	&1,do_pattern			# set flag for read pattern
gpio_skip_pattern:
	mov.l	FD_CP(%a3),%a1			# get card pionter
	mov.b	&0,3(%a1)			# disable interrupts

	##	let the card know that we are ready for the first byte/word
	mov.b	5(%a1),%d0			# set IO line
	mov.b	&1,(%a1)			# set PCTL

	##	finish read setup
	mov.l	28(%sp),%a0			# get users buffer
	mov.l	32(%sp),%d0			# get users count

	##	branch to the appropriate read routine
	tst.b	do_pattern			# see if we need to do this
	bne	gpio_read_pat			# yes, go do pattern read
	btst	&9,%d1				# 16 bit transfer?
	bne	gpio_read_word
	subq.l	&1,%d0				# decrement entire long
	bra	r_gpio_wait_byte
 
r_gpio_byte_ready:
	mov.b	&1,(%a1)			# set PCTL
r_gpio_wait_byte:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1                  	# clear out all but eir	 bit
	beq	r_no_eir_byte			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	r_eir_exit_byte			# yes - then cleanup and leave
r_no_eir_byte:
	btst	&0,(%a1)			# get card ready status
	beq.b	r_gpio_wait_byte		# loop till we get something

	mov.b	5(%a1),(%a0)+			# get data byte
	dbra	%d0,r_gpio_byte_ready		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	r_gpio_byte_ready		# loop until lower count
 
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave
 
gpio_read_word:
	lsr.l	&1,%d0				# COUNT / 2
	subq.l	&1,%d0				# decrement entire long
	bra	r_gpio_wait_word

r_gpio_word_ready:
	mov.b	&1,(%a1)			# set PCTL
r_gpio_wait_word:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1                 		# clear out all but eir bit
	beq	r_no_eir_word			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	r_eir_exit_word			# yes - then cleanup and leave
r_no_eir_word:
	btst	&0,(%a1)			# get card ready status
	beq.b	r_gpio_wait_word		# loop till we get something
	mov.w	4(%a1),(%a0)+			# get data byte
	dbra	%d0,r_gpio_word_ready		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	r_gpio_word_ready		# loop until lower count
 
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave


gpio_read_pat:
	mov.w	FD_PATTERN(%a3),%d3		# get termination word
	btst	&9,%d1				# 16 bit transfer?
	bne	p_gpio_read_word
	mov.b	FD_PATTERN+1(%a3),%d3		# get termination character
	subq.l	&1,%d0				# decrement entire long
	bra	p_gpio_wait_byte
 
p_gpio_byte_ready:
	mov.b	&1,(%a1)			# set PCTL
p_gpio_wait_byte:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1         			# clear out all but eir bit
	beq	p_no_eir_byte 			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	r_eir_exit_byte			# yes - then cleanup and leave
p_no_eir_byte:
	btst	&0,(%a1)			# get card ready status
	beq.b	p_gpio_wait_byte		# loop till we get something

	mov.b	5(%a1),(%a0)			# get data byte
	cmp.b	%d3,(%a0)+			# check for early termination
	beq	matched_byte			# yes, clean up

	dbra	%d0,p_gpio_byte_ready		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	p_gpio_byte_ready		# loop until lower count
 
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave
 
p_gpio_read_word:
	lsr.l	&1,%d0				# COUNT / 2
	subq.l	&1,%d0				# decrement entire long
	bra	p_gpio_wait_word
 
p_gpio_word_ready:
	mov.b	&1,(%a1)			# set PCTL
p_gpio_wait_word:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1                   	# clear out all but eir bit
	beq	p_no_eir_word 			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	r_eir_exit_word			# yes - then cleanup and leave
p_no_eir_word:
	btst	&0,(%a1)			# get card ready status
	beq.b	p_gpio_wait_word		# loop till we get something

	mov.w	4(%a1),(%a0)			# get data byte
	cmp.w	%d3,(%a0)+			# check for early termination
	beq	matched_word			# yes, clean up

	dbra	%d0,p_gpio_word_ready		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	p_gpio_word_ready		# clear lower word
 
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave

	##
	## GPIO fast handshake out routine
	##
gpio_write:
	mov.l FD_CP(%a3),%a1			# get card pointer to instat reg
	mov.b	&0,3(%a1)			# disable interrupts
	mov.w	(%a3),%d1			# get dil state
	mov.l	28(%sp),%a0			# get users buffer
	mov.l	32(%sp),%d0			# get users count

	btst	&9,%d1				# 16 bit transfer?
	bne	gpio_write_word
	subq.l	&1,%d0				# decrement entire long

w_gpio_wait_byte:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1 				# clear out all but eir bit
	beq	w_no_eir_byte 			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	w_eir_exit_byte			# yes - then cleanup and leave
w_no_eir_byte:
	btst	&0,(%a1)			# get card ready status
	beq.b	w_gpio_wait_byte		# loop till we get something

	mov.b	(%a0)+,5(%a1)			# transfer byte out
	mov.b	&1,(%a1)			# set PCTL
	dbra	%d0,w_gpio_wait_byte		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	w_gpio_wait_byte		# clear lower word
 
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave
 
gpio_write_word:
	lsr.l	&1,%d0				# COUNT / 2
	subq.l	&1,%d0				# decrement entire long

w_gpio_wait_word:
	mov.b	7(%a1),%d1			# peripheral asserting EIR?
	andi.b	&4,%d1                 		# clear out all but eir bit
	beq	w_no_eir_word 			# no - then skip cleanup     
	mov.b	FD_STATE(%a3),%d1		# get dil state
	btst	&EIR_CONTROL,%d1		# cleanup on EIR?
	bne	w_eir_exit_word			# yes - then cleanup and leave
w_no_eir_word:
	btst	&0,(%a1)			# get card ready status
	beq.b	w_gpio_wait_word		# loop till we get something

	mov.w	(%a0)+,4(%a1)			# transfer byte out
	mov.b	&1,(%a1)			# set PCTL
	dbra	%d0,w_gpio_wait_word		# loop until lower count
	clr.w	%d0				# clear lower word
	subq.l	&1,%d0				# decrement entire long
	bge	w_gpio_wait_word		# loop until lower count
 
	mov.l	32(%sp),%d0			# get initial count
	bra	gpio_terminate			# leave

	##
	## GPIO FHS transfer termination
	##
r_eir_exit_byte:
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
w_eir_exit_byte:
	mov.b	&1,1(%a1)			# reset the card
	mov.l	32(%sp),%d2			# get initial count
 	add.l	&1,%d0
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value
	bra	gpio_terminate			# leave

r_eir_exit_word:
	or.b	&TR_END,FD_REASON(%a3)		# set termination reason
w_eir_exit_word:
	mov.b	&1,1(%a1)			# reset the card
 	add.l	&1,%d0
	lsl.l	&1,%d0				# COUNT * 2
	mov.l	32(%sp),%d2			# get initial count
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value
	bra	gpio_terminate			# leave

matched_word:
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termination reason
	tst.l	%d0				# count satisfied?
	bgt	no_count_word
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
no_count_word:
	lsl.l	&1,%d0				# COUNT * 2
	mov.l	32(%sp),%d2			# get initial count
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value
	bra	gpio_terminate			# leave

matched_byte:
	or.b	&TR_MATCH,FD_REASON(%a3)	# set termination reason
	tst.l	%d0				# count satisfied?
	bgt	no_count_byte
	or.b	&TR_COUNT,FD_REASON(%a3)	# set termination reason
no_count_byte:
	mov.l	32(%sp),%d2			# get initial count
	sub.l	%d0,%d2				# compute return value
	mov.l	%d2,%d0				# set return value

gpio_terminate:
	movm.l	(%sp)+,%d2-%d3/%a2-%a4		# restore registers
	rts					# return
