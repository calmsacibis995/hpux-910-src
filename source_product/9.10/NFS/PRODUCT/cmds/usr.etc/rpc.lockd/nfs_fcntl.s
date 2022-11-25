# @(#) $Revision: 1.3.109.1 $      
# C library -- fcntl
		
     	set	NFS_FCNTL,230
	global	_nfs_fcntl
	global	__cerror
		
_nfs_fcntl:		
	mov.l	&NFS_FCNTL,%d0
	trap	&0
	bcc.b	noerror
	jmp	__cerror
noerror:		
	rts	
