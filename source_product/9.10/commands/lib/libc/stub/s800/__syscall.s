;
; @(#) $Revision: 66.3 $
;

include(mac800.mh)

_entry(__syscall)
        copy    arg0,SYS_CN             ; Pass call number to kernel
        copy    arg1,arg0               ; Copy arg1 to arg0
        copy    arg2,arg1               ; Copy arg2 to arg1
        copy    arg3,arg2               ; Copy arg3 to arg2
        ldw     -52(sp),arg3            ; Copy arg4 to arg3
        ldw     -56(sp),r1
        stw     r1,-52(sp)              ; Copy arg5 to arg4
        ldw     -60(sp),r1
        stw     r1,-56(sp)              ; Copy arg6 to arg5
        ldw     -64(sp),r1
        stw     r1,-60(sp)              ; Copy arg7 to arg6
        ldw     -68(sp),r1
        stw     r1,-64(sp)              ; Copy arg8 to arg7
        ldw     -72(sp),r1
        stw     r1,-68(sp)              ; Copy arg9 to arg8
        ldw     -76(sp),r1
	stw	r19,-28(sp)		; kernel trashes it
        ldil    L%SYSCALLGATE,r31
        .call
        ble     R%SYSCALLGATE`(sr7,r31)' ; Branch to kernel gateway
        stw     r1,-72(sp)              ; Copy arg10 to arg9
        or,=    r0,RS,r0
        .call
        .import $cerror
        b       $cerror                 ; Branch to common error
	ldw	-28(sp),r19		; kernel trashes it
_doreturn
