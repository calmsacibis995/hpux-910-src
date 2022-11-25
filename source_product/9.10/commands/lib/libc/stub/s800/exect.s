;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)
include(break.mh)

_entry(exect)
	break	BI1_AZURE,BI2_AZURE_EXECT
_dothecall(execve)
_doreturn
