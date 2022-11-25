;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(fork)
_dothecall(fork)
	or,=	ret1,r0,r0	; ret1 == 0 in parent, 1 in child
	copy	r0,ret0		; return 0 to child
_doreturn			; pid = fork()
