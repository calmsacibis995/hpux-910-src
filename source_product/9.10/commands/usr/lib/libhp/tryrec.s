# @(#) $Revision: 30.1 $      
 #Routine to simulate the Modcal TRY/RECOVER construct for use in/by C.
 #The necessary data is squirreled away in C__trystuff as follows:
 # struct C__trystuff {
 #	int	pctr;
 #	int	link;
 #	int	regs[12];
 #	};
 #
 #The following macros can be used to simulate the syntax of try/recover
 #	#define try if (C__tst()){struct C__trystuff TR; C__try(TR);
 #	#define recover c__rec(TR);} else 
		
	global	C__try,C__rec,C__tst,escape
		
    	set	pctr,0
    	set	link,4
    	set	regs,8
		
 # C__tst is used to save the return address of the if/else, which will return
 # 1 on a try, and 0 on an escape/recover.
 # C__try is called with a parameter of a structure in which to save the
 # current status.
 # The return address (and mark stack) are put in the area specified by the
 # parameter, and linked to the head (as used by escape) and the previous 
 # (to be used on a normal recover).
		
		
C__tst:	mov.l	(%sp),retadd
	movq	&1,%d0
	rts	
		
C__try:	mov.l	4(%sp),%a0		#Get location of structure
	mov.l	retadd,pctr(%a0)		#save return address
	movm.l	&0xFCFC,regs(%a0)		#save registers
	mov.l	head,link(%a0)		#link stack
	mov.l	%a0,head		#update the head
	rts	
		
C__rec:	mov.l	4(%sp),%a0		#get block address
	mov.l	link(%a0),head		#unlink it
	rts	#|return
		
escape:	mov.l	4(%sp),escapecode		#put in global escapecode
	mov.l	head,%a0		#put queue head pointer
	movm.l	regs(%a0),&0xFCFC		#restore registers
	mov.l	link(%a0),head		#restore the head
	mov.l	pctr(%a0),%a0		#get return address
	clr.l	%d0		#return to C__tst location with a 0
	addq.l	&8,%sp		#clear off try's arg and pc
	jmp	(%a0)
		
		
		
	bss	
retadd:	space	4		#temp for return address in case of escape
head:	space	4		#head of linked list of try blocks.
		
	global	escapecode
escapecode:	space	4
