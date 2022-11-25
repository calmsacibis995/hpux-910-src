;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)
include(break.mh)

_entry(_sigreturn)
	copy	ret0,arg0	; handler return value
_dothecall(SIGCLEANUP)
	break	BI1_AZURE,BI2_AZURE_SIGRETURN
_doreturn
