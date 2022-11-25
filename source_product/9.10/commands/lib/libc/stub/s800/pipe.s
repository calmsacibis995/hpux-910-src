;
; @(#) $Revision: 66.1 $
;

include(mac800.mh)

_entry(pipe)
	stw	arg0,-36(sp)
_dothecall(pipe)
	ldw	-36(sp),arg0
	stw	ret0,0(arg0)
	stw	ret1,4(arg0)
	copy	r0,ret0
_doreturn
