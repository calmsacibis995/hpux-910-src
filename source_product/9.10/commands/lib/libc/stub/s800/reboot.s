;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)
include(break.mh)

_entry(reboot)
_dothecall(reboot)
	break	BI1_AZURE,BI2_AZURE_REBOOT
_doreturn
