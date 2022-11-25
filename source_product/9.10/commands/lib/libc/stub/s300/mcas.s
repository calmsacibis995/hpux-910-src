# @(#) $Revision: 70.2 $
#
# Series 300 _mcas routine for msemaphore support.
#
#     This code uses a lightweight call into the kernel to
#     obtain the current processes msemaphore lock id.
#     It then uses the cas (compare and swap) instruction
#     to arbitrate access to the semaphore, and at the same
#     time, record who gets the lock.
#
#     _mcas is called by msem_lock(2).

    text

    global __mcas

__mcas:
    clr.l   %d0             # for lw get lock id, %d0 = 0, %d1 = 1
    mov.l   &1,%d1
    trap    &0xb            # lightweight call trap number
    cmpi.l  %d0,&-1
    bne.b   Lgot_id
    mov.l   &1,%d0          # return code for id not allocated
    rts

Lgot_id:
    mov.l   4(%sp),%a0      # get address of semaphore
    clr.l   %d1             # check locker & wanted field for 0 in cas2 operation
    addq.l   &0x8,%a0       # get address of wanted & locker fields
    mov.l   %a0,%a1         #     in %a0
    addq.l   &0x4,%a1       #     and %a1 respectively
    cas2.l  %d1:%d1,%d1:%d0,%a1:%a0  # Do compare and swap
    beq.b   Lgot_lock
    mov.l   &3,%d0          # return code for lock not available
    rts

Lgot_lock:
    clr.l   %d0             # return 0 to indicate success
    rts
