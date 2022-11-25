;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(mount)
_dothecall(mount)
	copy	r0,ret0
_doreturn
