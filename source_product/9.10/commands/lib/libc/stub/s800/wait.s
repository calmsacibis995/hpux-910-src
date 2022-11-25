;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(wait)
	copy	r0,arg3		; indicate wait, not wait3
_dothecall(wait)		; pid = wait()
_doreturn
