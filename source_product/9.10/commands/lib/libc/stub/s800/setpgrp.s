;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(setpgrp)
	ldi	1,arg0
_dothecall(setpgrp)		; pgrp = setpgrp(1)
_doreturn
