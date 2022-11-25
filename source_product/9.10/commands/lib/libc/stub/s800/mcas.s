# @(#) $Revision: 70.1 $
#
# PA-RISC _mcas routine for msemaphore support.
#
#     This code uses a lightweight call into the kernel to
#     perform a atomic compare and swap. This lightweight call
#     also obtains the current processes msemaphore lock id
#     to use in the compare and swap operation.
#
#     Since taking a fault on the gateway page is a problem,
#     the lightweight call checks to see if the page containing
#     the semaphore is currently valid. If it is not, the call
#     returns status in the ret0 register, indicating that the
#     page should be touched, and the lightweight call retried.
#     The lightweight call also returns other errors, which are
#     acted upon by msem_lock(). A zero is returned in the ret0
#     register to indicate that the lock has been obtained.
#
#     _mcas is called by msem_lock(2).

    .import $global$,DATA
    .import _mcas_util_addr,DATA    ; mcas_util_addr contains the cached

    .space  $TEXT$
    .subspa $CODE$
    .export _mcas
    .proc
    .callinfo
    .entry
_mcas
    ;
    ; Get address of mcas_util() routine (into arg1).
    ;
changequote(<,>)
ifdef(<PIC>,<
    ldw     T'_mcas_util_addr(0,r19),arg1
    ldw     0(0,arg1),arg1
>,<
    addil   LR'_mcas_util_addr-$global$,dp
    ldw     RR'_mcas_util_addr-$global$(0,r1),arg1
>)
changequote()

m$tryagain
    .call
    ble,n   (sr7,arg1)		    ; call mcas_util(arg0)
    nop
    comibf,=  2,ret0,m$pageok
    nop				    ; nullifies are slow, avoid them
    ldb     4(arg0),arg2            ; read and write part of the magic #
    b       m$tryagain
    stb     arg2,4(arg0)            ;     to get page in memory

m$pageok
    .exit
    bv      (rp)
    nop
    .procend
    .end
