# @(#) $Revision: 66.3 $     
# Count subroutine calls during profiling
#
# The basic idea here is a call of the form:
#
#	move.l	#LABEL,a0
#	jsr	mcount
#	.data
# LABEL:	dc.l	0
#	.text
#
#which is generated automatically by the C compiler for each subroutine call.
#The first time mcount is called, the contents of LABEL will be 0 so mcount
#will allocate two words from its buffer like:
#
#struct
#{
#	char *fncptr;		/* ptr to subroutine */
#	long fnccnt;		/* number of calls to this function */
#};
#
#and fill int fncptr from the return pc, fill in LABEL with a ptr to
#the newly allocated structure and bump the count.  On subsequent calls,
#the contents of LABEL will be used to just bump the count.
		
	global	mcount	
	comm	__cntbase,4		#originally 2 MFM 
		
mcount:		
	move.l	(%a0),%d0		#fetch contents of LABEL
	bne.w	mcount1S		#something there
ifdef(`PIC',`
	mov.l	%a2,-(%sp)
	mov.l  &DLT,%a2
	lea.l   -6(%pc,%a2.l),%a2
	move.l	__cntbase(%a2),%a2
	move.l	(%a2),%a1		# a1 is ptr to next avail cnt struct
	add.l	&8,(%a2)		#bump cntbase to followng structure
	move.l	(%sp),(%a1)+		#save ptr to function
	move.l	%a1,(%a0)		#save ptr to cnt in structure in LABEL
	addq.l	&1,(%a1)		#bump the function count
	mov.l	(%sp)+,%a2
	rts',`
	move.l  __cntbase,%a1           #ptr to next available cnt structure
	add.l   &8,__cntbase            #bump cntbase to followng structure
	move.l  (%sp),(%a1)+            #save ptr to function
	move.l  %a1,(%a0)               #save ptr to cnt in structure in LABEL
	bra.w   mcount2S
')
mcount1S:	move.l	%d0,%a1		#so we can use it as a ptr
mcount2S:	addq.l	&1,(%a1)		#bump the function count
	rts	
