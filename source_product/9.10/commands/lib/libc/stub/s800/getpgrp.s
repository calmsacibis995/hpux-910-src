;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(getpgrp,getpgid)
	copy	r0,arg0
_dothecall(setpgrp)		; pgrp = setpgrp(0)
_doreturn
