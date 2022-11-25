;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(setsid)
	ldi	2,arg0
_dothecall(setpgrp)		; pgrp = setpgrp(2)
_doreturn
