# @(#) $Revision: 30.1 $      
 #Routine to simulate the Modcal TRY/RECOVER construct.
 #The following macros can be used to simulate the syntax of try/recover
 #	#define TRY if ((escapecode = Try())==-65535) {
 #	#define RECOVER Recover();} else 
		
 #The following declarations must appear in each procedure using TRY/RECOVER
 #	int escapecode;
 #	register int *xx;  /* to reserve A5 for try/recover */
 #it is necessary to protect A5 in this way until further details are
 #worked out between Modcal and C if the current Modcal compiler is used 
 #(with its current definition of A5).
		
	global	Try,Recover
		
 #The block that simulates the Pascal global area is put on to the stack
 #and pointed to by A5.  It is a "negative running" as described below.
 #Certain fields have a different meaning for this routine than for Modcal
 #because they are meaningless to C.
        set	escapecode,-2
       	set	routine,-6
       	set	markstk,-10
      	set	bksize,10	#The total size of the structure above

 #try is called with no parameters.  It squrrels away a try/recover block
 #on the stack and sets things up to return to the try call with a non-zero
 #value if an escape occurs.  Try initially returns -65536, which cannot
 #be an escapecode.  (Escapecodes are shorts, and can be zero).
		
Try:	mov.l	(%sp)+,%a0		#Get return address
	movm.l	&0x3f3a,-(%sp)		#save registers (Modcal won't (for now))
	link	%a5,&-bksize		#Make space for pseudo global 
	mov.l	&errcal,-(%sp)		#put recover routine address on TOS
	mov.l	%sp,markstk(%a5)	#put mark stack in global
	mov.l	%a0,routine(%a5)	#put away return address
	mov.l	&-65536,%d0		#return "nothing" flag
	jmp	(%a0)		
		
Recover:	mov.l	(%sp),%a0
	unlk	%a5		#clean off block
	add.l	&-40,%sp	#pop off saved regs
	jmp	(%a0)
		
errcal:	mov.l	routine(%a5),%a0
	mov.w	escapecode(%a5),%d0		#put result in d0
	ext.l	%d0
	unlk	%a5		#clean off junk
	movm.l	(%sp)+,&0x5cfc
	jmp	(%a0)
		
		
