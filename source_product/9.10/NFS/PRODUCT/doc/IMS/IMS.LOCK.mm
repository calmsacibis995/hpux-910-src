.\"	@(#)LOCK MANAGER IMS:	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:26:50 $
.\"
.\"  This is the Internal Maintenance Specification for the Lock Manager (LM)
.\"  component of the NFS Services/300 product.
.\"
.\"  This document is made printable by entering the following command:
.\"
.\"		nroff -cm filename > formatted_file
.\"		lp -ogothic < formatted_file
.\"
.tr ~
.PH ""
.nr Cl 6	\"	save 6 levels of headings for table of contents
.sp 4
.ce 1
\fBInternal Maintenance Specification\fR
.sp 2
.ce
\fBLock Manager\fR
.sp 2
.ce 2
Author: Jeff Elison
Modified By: Dominic Ruffatto
.sp 2
.ce 2
$Revision: 1.4.109.1 $
$Date: 91/11/19 14:26:50 $
.sp 3
.H 1 "Abstract"
This document describes the Lock Manager 
code released with HP-UX 6.5 (based upon Sun 3.2).
The target audience for this document includes
HP online support engineers and new engineers assigned to work on the
Lock Manager.
.PH "''\fBLock Manager IMS\fR''"
.bp
.PF "''-~\\\\nP~-''"
.H 1 "Objectives"
The objectives of this document are
to present an overview of the function and structure of the Lock Manager and
to discuss the components of the individual Lock Manager modules.
It is hoped that detail is sufficient for the next owner of the code.
Information on the Lock Manager implementation will be discussed by
explaining the major data structures, followed by several examples.
.H 1 "References"
It is assumed that you are familiar with the following documents.
.AL A
.LI
\fIUsing and Administering NFS.\fR
Of particular interest is the "Configuration and Maintenance" chapter.
.LI
\fIHP-UX Reference Pages.\fR
The pages you should read are:
.ML . 6 1
.LI
\fIfcntl\fR(2)
.LI
\fIlockf\fR(2)
.LI
\fINFS Reference Pages.\fR
The pages you should read are:
.ML . 6 1
.LI
\fIlockd\fR(1m)
.LI
\fIstatd\fR(1m)
.LE
.sp
You may also find the \fINFS RPC/XDR Internal Maintenance Specification\fR
document helpful in learning the intricacies of RPC and XDR.
.bp
.H 1 "Function and Structure of the Lock Manager"
.H 2 "An Overview"
The Lock Manager (LM) and the Status Monitor (SM) are used to implement remote
file locking.  These user-level processes must run on both the client and the
server.  When an application process makes a call to lock a file (lockf or
fcntl), the call goes into the kernel and (in the case where the file is remote)
the kernel makes an RPC call to the local lock manager in user space.  The local
lock manager may then need to call the lock manager on the remote node.  In
addition, the status monitor may be called in this sequence to verify the status
of the remote node.  All the status monitor actually knows is if a node has
done a reboot.
.sp
Even in this high-level description we differ from Sun's implementation.  This
document will try to explain these design decisions.  For a more concise, high-
level view of these differences, read the NFS ERS.
.sp
The LM and SM make use of the RPC and XDR components of the NFS product over 
the network in a transparent fashion, using both TCP/IP and UDP/IP as
the transport protocols.  Note that the RPC calls between Lock Managers are
"one-way" calls.  By specifying a timeout of 0, the Lock Manager causes UDP
to send the message without waiting for a status.
.sp
The SM uses files in a directory rooted at \fI/etc/sm\fR
on each node running the SM.  These files each contain the name of a node
which the SM has been in contact with since its initialization.  It also
uses the file \fI/etc/state\fR to keep track of the local node's state.
.sp
The state is increased by 2 every time the SM is initialized.  After
changing the state, the SM continues the initialization process by sending
an RPC message to the LM on each of the nodes listed in files in 
the \fI/etc/sm\fR
directory.  The remote Lock Managers will then check their lock lists.  If
they had locks established on the rebooted node, they will try to reclaim
them.  Note that the Lock Manager on the rebooted node will wait for a
"grace period" (currently 45 seconds) for locks to be reclaimed.  During
this period the Lock Manager will ignore all lock requests unless they
are marked as "reclaim" requests.
.sp
If a LM sends a reclaim request, but the request is denied for some reason,
the LM must notify the local process which holds the lock.  This notification
is done using a SIGLOST signal from the LM to the process.
.sp
The processes responsible for remote file locking and monitoring are
\fI/usr/etc/rpc.statd\fR, the Status Monitor, and \fI/usr/etc/rpc.lockd\fR,
the Lock Manager.
Both are daemon processes typically activated at system startup time from
within \fI/etc/netnfsrc\fR.
.bp
.H 2 "Block Diagram"
In the discussion above, we considered two entities the Lock Manager (LM)
and the Status Monitor (SM).  The diagram below shows this scheme in
greater detail.  The paths illustrate a first time remote lock request.
Note that the Status Monitor is implemented by the single
RPC program, SM_PROG.  However, the Lock Manager is implemented as three
RPC programs, the Network Lock Manager (NLM_PROG), the Kernel Lock Manager
(KLM_PROG), and the PRIVate communication program (PRIV_PROG).
.sp
The PRIV_PROG is the least active of all these RPC programs.  It is there
only to handle crash and recovery interactions.  Its main function is to
receive the "recovery message" sent out by the SM_PROG when the Status
Monitor is initialized (i.e., re-started).
.sp
Notice that the HP-UX kernel talks only to the KLM_PROG.  The KLM_PROG
will contact the local SM_PROG only if this is a remote request and it
is the first request sent to
this particular remote node.  Likewise, the KLM_PROG will contact the
NLM_PROG only if this is a remote request.  Finally, the remote NLM_PROG
will call into the kernel to try the lock.
.sp
.sp
.nf

+--------+                             |
|App Proc|                             |
+--------+                             |
  |        +--------+<__...............|...(8).........___+--------+
  |    ----|NLM_PROG|                 RPC                 |NLM_PROG|
  |    |   +--------+                  |    ----(4)------>+--------+
  |    |         ---------------(4)----|----|                 | |  ^
  |    |         |  +-------+___.......|........__>+-------+ (5)|  |
  |   (9)       (4) |SM_PROG|         RPC          |SM_PROG|<-+ | (7)
  |    |         |  +---^---+<__.......|........___+-------+    |  |
 (1)   |         |      |  |           |                      (HP only)
  |    |   +--------+  (3) |         L | R                      |  |
  |    |   |KLM_PROG|---+  V         O | E                     (6) |
  |    |   +--------+  +---------+   C | M      +---------+     |  |
  |    |        ^      |PRIV_PROG|   A | O      |PRIV_PROG|     |  | User
  V  User Space |      +---------+   L | T      +---------+     V  | Space
=======|=========================      | E    =================================
  |    |        ^      Kernel          |                        |  ^  Kernel
  V    V        |                    N | N                      V  | (HP only)
+------+        |                    O | O                 +------------+
| Stub |--(2)-->+                    D | D                 | _nfs_fcntl |
+------+                             E | E                 +------------+


.fi
At this point it is necessary to look at the differences between our
implementation and Sun's.  Number 1, is the fact that ALL of Sun's file
locking happens in user-space.  The kernel code is only a stub that always
calls the KLM_PROG.  Without rpc.lockd and rpc.statd running, they have no
file locking; not even local locking.  This is one reason why the LM was
broken into KLM_PROG and NLM_PROG.  The KLM_PROG could handle local locking
entirely on its own.
.sp
To add remote file locking to HP-UX, we had to consider the fact that local
locking was already in HP-UX 6.0.  It was not reasonable to make every 
customer who wanted local locking run the Lock Manager.  In addition, HP-UX
6.0 supported mandatory (or enforcement mode) locks.  This functionality is
not supported by Sun's Lock Manager, nor is it easy to move the code into
user-space.  The simplest solution appeared to be leave the Lock Manager
and the HP-UX kernel alone as much as possible.  Then let the kernel handle
local locks (without contacting the Lock Manager) and let the Lock Manager
handle remote lock requests.  However, to handle remote lock requests the LM
must call the kernel to make sure that no local locks will block this remote
request.
.sp
In terms of the block diagram, these changes from Sun's implementation show
up in three ways.  First, the KLM_PROG will ALWAYS call the NLM_PROG, since the
KLM_PROG no longer handles local requests.  Second, the kernel "Stub" is more
than just a stub in HP-UX.  It determines whether the request is local or 
remote; if it is local, it performs the appropriate actions itself.  Thirdly,
the NLM_PROG on the remote node will call _nfs_fcntl in the kernel to obtain
the lock.  In Sun's implementation, this interaction with the kernel is 
totally absent; the NLM_PROG does everything necessary to grant or deny the
lock request.
.H 1 "Lock Manager Header Files"
The following header files are delivered in \fI/usr/include/rpcsvc\fR.
.H 2 "klm_prot.h"
contains symbols and structures defining the RPC protocol used between 
KLM_PROG and the kernel.
This file holds the symbolic definitions for the RPC program and procedure
numbers for the current KLM_PROG version.
In addition, status values for a klm_reply are defined.
.H 2 "nlm_prot.h"
contains symbols and structures defining the RPC protocol used between 
a local NLM_PROG and a remote NLM_PROG.
This file holds the symbolic definitions for the RPC program and procedure
numbers for the current NLM_PROG version.
In addition, status values for a nlm_reply are defined.
.H 2 "sm_inter.h"
contains symbols and structures defining the RPC protocol used between 
a local SM_PROG and a remote SM_PROG.
This file holds the symbolic definitions for the RPC program and procedure
numbers for the current SM_PROG version.
In addition, status values for a sm_reply are defined.
.H 1 "Lock Manager - low level"
Perhaps the easiest way to illustrate the lock manager modules and data
structures is with a couple of examples.  First, the major data structures
will be defined, and then several examples will be used to show how the
data structures are manipulated.
.H 2 "Major Data Structures"
The major data structures used by the lock manager are a set of 4 queues.
Understanding how they are manipulated gives a pretty good understanding
of the lock manager.  I will list them here with a brief description.
Then with each step in the examples, I will show how they have changed.
When the request gets over to the remote lock manager, I will show the
remote queues also.
.sp
.nf
msg_q     - the MESSAGE QUEUE - this queue holds requests (from the
	    kernel) which have been processed by the local lock
	    manager and are awaiting a reply from the remote lock
	    manager.  A request is not queued until a message has
	    been sent to the remote.  A request is of type "reclock".
	    So, this is a linked list of reclocks.

grant_q   - the GRANTED QUEUE - this is for granted locks (locks that
            are currently being held).  There really is no "grant_q".
	    It is implemented as a table of queues. The table (array)
	    is called table_fp.  When a lock is granted, lockd 
	    performs a hashing function on the fh (file handle) field
	    associated with that lock.  This is used as the subscript
	    for table_fp.  table_fp[i] is a pointer to a linked list
	    of granted locks.  Granted locks are of type fs_rlck.

wait_q    - the WAIT QUEUE - this queue is for blocked lock requests.
	    These are still just requests, so they are "reclocks".

call_q    - the CALL BACK QUEUE - When a server processes an unlock
	    request, it may "unblock" one or more blocked lock 
	    requests (on the wait_q).  When this happens, it adds the
	    blocked request (reclock) to the call_q and the grant_q,
	    and deletes it from the wait_q.  The call_q is its way of
	    remembering that once it replies to the unlock request
	    with a GRANTED, it also needs to reply with GRANTED for
	    this (or these) requests.
.fi
.H 2 "Example 1 - simple lock"
For this example a lock request for a remote file will be traced through
the kernel, to the local lock manager, to the remote lock manager, and
back again.  Assume that this is the first lock requested since the
lockd was started.  At the start of this example, we have not yet had
a request to the local lock manager.  Therefore, all its queues are
empty at this point.
.H 3 "Example 1: user to local kernel"
To start we need a lock request from a user process.  The user will do this
with an fcntl(2) or lockf(2) call.  It does not matter which call is used,
but let's make it a request for a write lock.
.nf
	msg_q   : empty
	grant_q : empty
	wait_q  : empty
	call_q  : empty
.fi
.H 3 "Example 1: kernel to local lockd"
This request will first go to the kernel.  The kernel determines if this is
a request for a local file or an nfs file.  If it is a local file, the kernel
handles the request.  Since this is an nfs file, the kernel makes an RPC call
to the local lock manager (KLM_PROG), procedure KLM_LOCK.
.nf
	msg_q   : empty
	grant_q : empty
	wait_q  : empty
	call_q  : empty
.fi
.H 3 "Example 1: local lockd - client - KLM_PROG"
The first thing KLM_PROG will do is block SIGALRM.  SIGALRM is used to
indicate the need to retransmit a request.  Space is then allocated for
a lock request.  For this example, this request will be called REQ-A.  This
is of type reclock and these entries are
referred to as le's for lock entries.  The appropriate XDR routine is
used to decode the arguments.  Then map_kernel_klm(REQ-A) is called to
translate the request into the lockd reclock format.  This includes
the normal lock information such as: a boolean for blocking or not,
exclusive or shared, upper bound, lock length, and pid of the requesting
process.  In addition, lockd creates an "owner handle" which is a character
string comprised of the hostname concatenated with the pid.  It also
assigns a "cookie" to this request.  This is simply a monitonically increasing
number.  Finally, it fills in the hostname for clnt.
.sp
At this point, klm_prog checks to see if the graceperiod is still in affect.
If it is, then lockd will only accept "reclaim" requests for locks that were
lost by the apparent crash of this lockd.  Since this is not a reclaim
request the lockd would queue it and ignore it until the graceperiod is over.
This seems rather strange because once the graceperiod is over, the lockd
merely throws away these queued requests anyway.  The assumption is that
the kernel will retransmit the request.  The only apparent reason for queueing
these requests is so that the lockd can send a "KLM_WORKING" reply back to
the kernel when the lockd gets its SIGALRM.  This reply is to let the kernel
know that the lockd is alive, but does not have a real reply yet.  It seems
that it would be better and safer to send the KLM_WORKING reply immediately
and never queue the request.
.sp
Assuming that the graceperiod is over, the lockd will continue to process
this request.  For lock requests (like this one) the lockd will call the
statd to monitor the request (add_mon()).  The lockd allocates a monitor
entry (me) to record this action.  The statd adds an entry to its monitor_q
and for a new request like this one it will create a file - "/etc/sm/nodename"
and place nodename in this file.  Note: on short filename systems, this 
filename will be truncated to 14 characters and multiple node names (node
who's first 14 characters are the same) may appear in the file.  
.sp
The lockd now calls proc_klm_lock.  This simply calls klm_msg_routine with
the appropriate values for "local" and "remote".  These variables are the
names of the routines to process local and remote requests of this type.  In
this case they are "local_lock" and "remote_lock".  
.sp
klm_msg_routine calls "retransmitted" which simply checks the msg_q.  Since
this is not a retransmitted message, klm_msg_routine continues.  If this was
a request for a local file, it would call local_lock.  However, since this is
a remote request, it will call remote_lock.
.sp
remote_lock calls blocked() to see if this request is blocked by another
lock held by this client.  It is not blocked.  The lockd will now call
nlm_call() to send a one-way RPC lock request to the remote NLM_PROG
(procedure NLM_LOCK_MSG).  nlm_call
makes the one-way call by setting the timeout in call_udp to 0.  This should
get RPC_TIMEDOUT.  If it gets this, rather than some other error, it will 
queue the request in the msg_q, set the alarm (SIGALRM), and return.  This
return will work its way back up and the lock manager is done until a reply
or another request comes in.  This lock request will sit on the msg_q waiting
for a reply.
.nf
	msg_q   : REQ-A
	grant_q : empty
	wait_q  : empty
	call_q  : empty
.fi
.H 3 "Example 1: local lockd - client - xtimer"
For the sake of this example we will assume that the server does not reply
right away.  The timer has been set to 10 seconds (15 on Sun).  After the
10 seconds go by the lockd gets a SIGALRM.  This transfers control to routine
xtimer().  xtimer will run through the message queue (msg_q) checking each
message.  If the request has not had a reply, it may retransmit the request
to the remote NLM_PROG depending on how long it has been waiting.  (Basically,
it retransmits after the first 10 seconds and then after every 20 seconds
from then on.) 
.sp
xtimer now sends a KLM_WORKING reply to the kernel in response to the last
request received from the kernel.  This is to let the kernel know that it
is still alive, but does not have a reply yet.  Finally, if the msg_q is
not empty, as in this example, it will reset the alarm.
.nf
	msg_q   : REQ-A
	grant_q : empty
	wait_q  : empty
	call_q  : empty
.fi
.H 3 "Example 1: remote lockd - server"
Meanwhile, over on the server, the request from our client KLM_PROG has been
received by the NLM_PROG.  NLM_PROG is set up much like KLM_PROG.  Again,
it will block SIGALRM, allocate a lock entry (le) for the request, do the
XDR decoding, do some mapping to nlm format, check grace_period, and request
monitoring for the client.  It then calls proc_nlm_lock_msg().
.sp
proc_nlm_lock_msg calls local_lock().  local_lock calls blocked() to see if the
lock manager is holding another remote lock which would block this one.  If
it does not have such a lock, local_lock must now check with the kernel (by
way of nfs_fcntl()) to see if a local lock exists that would block this
request.  This kernel involvement is an important difference from Sun's
implementation.  Their kernel knows nothing about locks - both local and
remote locks are handled by the lock manager.  The kernel (we are still on
the server) will handle the nfs_fcntl, by setting a local lock if possible,
or by returning an error (ENOLCK or EACCES) if it is not possible. 
If at either step, lockd finds that the req is blocked, it would add this
request to the wait_q.  Assuming nfs_fcntl returns without
error, local_lock will call add_reclock to add this new lock to the lock
managers list of granted locks.  (This is actually multiple lists hashed by
file handle and pointed to by a table called table_fp.)  local_lock now returns
the status (GRANTED in this case) to proc_nlm_lock_msg which calls nlm_reply to
send a GRANTED reply (one-way RPC call) back to the CLIENT NLM_PROG (procedure
NLM_LOCK_RES).  Note, nlm_prog uses nlm_reply here - therefore the reply is
not queued.  It doesn't matter if this reply is lost, since the client will
retransmit if it doesn't get a reply in 10 seconds.
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : empty
	grant_q : empty      grant_q : REQ-A
	wait_q  : empty      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 1: local lockd - client - NLM_PROG"
CLIENT NLM_PROG will receive the reply from the server NLM_PROG.  Again,
it will block SIGALRM and allocate space for the reply.  It then calls
proc_nlm_lock_res() which fills in the "cont" variable which is the name
of the procedure that will continue handling the initial request.  In this
case that is cont_lock.  It calls nlm_res_routine with this value.
.sp
nlm_res_routine calls search_msg() to find the request in the msg_q that
matches this reply.  (They are matched by the value of cookie.)  If no matching
request is found, the reply is discarded.  In this case, we find the request
and pass it along with the reply to the "continue" routine, cont_lock.
.sp
cont_lock does a switch on the reply status.  In this case, it is GRANTED.
So cont_lock calls blocked().  This is a redundant, but necessary check.
Under normal conditions this call to blocked() will not return true.  Given
that our request is still not blocked, cont_lock now calls add_reclock() to
add this request to the granted list.  This operation mirrors what happened
in the remote lock manager.  cont_lock now returns to nlm_res_routine.
.sp
nlm_res_routine calls add_reply which links our request and reply.  Then
add_reply calls klm_reply which sends the GRANTED reply to the kernel.
Finally, add_reply calls dequeue() to remove the request from the msg_q and
the example is finished.
.nf
            LOCAL                 REMOTE
	msg_q   : empty      msg_q   : empty
	grant_q : REQ-A      grant_q : REQ-A
	wait_q  : empty      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 2 "Example 2 - blocked lock"
This example will illustrate how a blocked lock is handled.
The lock request will be a fcntl(F_SETLKW) for a write lock
on a remote file.  (This is the same as a lockf(F_LOCK).)  This means that
the process is willing to wait for the lock in case it is blocked.  In this
example, only the differences from the above example will be discussed.
.H 3 "Example 2: user to local kernel"
This is the same as example (1).
.H 3 "Example 2: kernel to local lockd"
This is the same as example (1).
.nf
	msg_q   : empty
	grant_q : empty
	wait_q  : empty
	call_q  : empty
.fi
.H 3 "Example 2: local lockd - client - KLM_PROG"
This example is to illustrate a blocking lock.  The lock could be blocked by
a process on the local node which is holding a lock on the remote file or it
could be blocked by some process on any other node which is holding a lock on
the remote file.  To make this example more complete, the process will be on
some other node.  This is illustrated by the existence of LOCK-X in the remote
queues below.  In either case, this step is basically the same as example
(1).  Since this process is willing to wait, even if blocked() returned true
(indicating that a process on this node already has a lock on the remote file),
KLM_PROG will still send the RPC request to the remote NLM_PROG.  This is to
ensure that the remote lockd will grant this blocked lock as soon as the first
lock is released.
.sp
If this had been a fcntl(F_SETLK) or a lockf(F_TLOCK) (indicating that the 
process is not willing to wait), and blocked() had returned true at this
point, the local KLM_PROG would simply reply with a DENIED to the kernel.
This would be returned to the process as EACCES.  The local lockd would not
talk to the remote lockd at all.
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : empty
	grant_q : empty      grant_q : LOCK-X
	wait_q  : empty      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 2: remote lockd - server"
This section would start out like the first example until it gets to the
call to blocked() in local_lock().  Now blocked() returns true.  So 
local_lock() calls add_wait() to put this request on the wait_q.  Then it
sets the response status to "blocking".  It now returns to proc_nlm_lock_msg
which continues as before - it sends the response back to the client NLM_PROG.
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : empty
	grant_q : empty      grant_q : LOCK-X
	wait_q  : empty      wait_q  : REQ-A
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 2: local lockd - client - NLM_PROG"
Again, this starts out like example (1), until it gets down to where
cont_lock does a switch on the reply status.  In this case, it is "blocking".
So cont_lock calls add_wait() to put this request on the wait_q.
This operation mirrors what happened
in the remote lock manager.  cont_lock now returns to nlm_res_routine.
.sp
nlm_res_routine calls add_reply which links our request and reply.  Then
add_reply calls klm_reply which sends the BLOCKED reply to the kernel.
However, at this point add_reply DOES NOT remove the request from the msg_q.
Therefore, it is on both the msg_q and the wait_q.
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : empty
	grant_q : empty      grant_q : LOCK-X
	wait_q  : REQ-A      wait_q  : REQ-A
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 2: remote lockd - server - unlock"
Now to illustrate the process of granting a blocked lock, assume that the
process that is holding LOCK-X unlocks it.  The actual processing of the
unlock request by the NLM_PROG on the remote server is almost identical
to the way it handles the lock request in example (1) except that the
procedures called are proc_nlm_unlock_msg and local_unlock.
.sp
The interesting part is in local_unlock.  After the lock has been deleted
from the "grant_q", local_unlock calls wakeup().  wakeup() runs through the
wait_q, checking for locks that "overlap" the freed lock and are no longer
blocked().  For each lock that meets these conditions, the lockd: calls
add_reclock to add it to the "grant_q"; calls add_call to put the lock request
on the call_q; and calls remove_wait to remove it from the wait_q.  The 
queues now look like:
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : empty
	grant_q : empty      grant_q : REQ-A
	wait_q  : REQ-A      wait_q  : empty
	call_q  : empty      call_q  : REQ-A
.fi
But wait, there's more.  As usual, local_unlock returns to proc_nlm_unlock_msg
which takes the result and calls nlm_reply() to send the result of the unlock
request back to the unspecified requestor.  Then it returns to nlm_prog.  Here
is another point of interest.  Just before it finishes, nlm_prog always calls
call_back(), but this time call_back does something because the call_q is not
empty.  It runs through the call_q calling nlm_call()
to send back a NLM_GRANTED_MSG message for the newly granted (previously
blocked) REQ-A.  It also removes it from the call_q.
.sp
Note that it uses
nlm_call and a NLM_GRANTED_MSG instead of nlm_reply and a NLM_LOCK_RES.  This
is because nlm_prog wants to retransmit until this granted message is
acknowledged.  This is necessary because once nlm_prog sent back the BLOCKED
reply to the initial REQ-A lock request, the client will keep the request on
the wait_q until it is granted or cancelled.  If nlm_prog replied here as
before with an nlm_reply and the reply was lost, the client would never get
the reply.  The retransmissions will happen if necessary since nlm_call calls
queue() to put the NLM_GRANTED_MSG (reply) on the msg_q.  So now the queues
look like:
.nf
            LOCAL                 REMOTE
	msg_q   : REQ-A      msg_q   : REPLY-A
	grant_q : empty      grant_q : REQ-A
	wait_q  : REQ-A      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 2: local lockd - client - NLM_PROG"
Back on the client, NLM_PROG gets the NLM_GRANTED_MSG, goes through the usual
set-up, and calls proc_nlm_granted_msg().  It in turn calls local_granted().
local_granted searches for the original request in the wait_q.  After finding
it there, it makes sure that it is not blocked, makes sure that it is in the
msg_q also, and calls add_reply() as before to set up the reply to the kernel
and remove it from the msg_q.
local_granted then calls add_reclock() to add it to the grant_q and
remove_wait() to remove it from the wait_q.  It then returns to
proc_nlm_granted_msg which calls nlm_reply() to send back a NLM_GRANTED_RES to
the server NLM_PROG.
.nf
            LOCAL                 REMOTE
	msg_q   : empty      msg_q   : REPLY-A
	grant_q : REQ-A      grant_q : REQ-A
	wait_q  : empty      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 3 "Example 2: remote lockd - server - granted reply"
Finally the server receives its ACK (NLM_GRANTED_RES) which ends up in
proc_nlm_granted_res().  This simply looks for the msg on the msg_q and
calls dequeue() on it.  DONE!!
.nf
            LOCAL                 REMOTE
	msg_q   : empty      msg_q   : empty
	grant_q : REQ-A      grant_q : REQ-A
	wait_q  : empty      wait_q  : empty
	call_q  : empty      call_q  : empty
.fi
.H 2 "Example 3 - server crash"
This example will illustrate how the crash of a server is handled.
Assume that before the crash, the server was holding a lock for our client.
.H 3 "Example 3: Overview"
The first 
thing to realize, is that the crash of a node is not detected until its
recovery!!  You also need to remember that a client or a server lockd will
monitor the nodes with which it is dealing.  This is explained to some degree
in the context of a lock request in example (1).
.sp
This monitoring involves 2 parts.  First the statd and lockd must add the
name of remote nodes to their respective monitor_q's.  This is in case a remote
node crashes.  During its recovery, the remote will send our statd a recovery
notice.  The monitor_q is used to see if we care about that node.
The second part is the creation of files /etc/sm/nodename by statd.  This is
for the case where we crash.  When we recover, statd will read the
contents of files found in /etc/sm (node names) and notify those nodes 
that we have crashed and recovered.
.H 3 "Example 3: server recovery - main()"
This server was holding a lock for our client and something bad happened;
either the whole server node died or rpc.lockd died.  At any rate, things
have been re-started.
.sp
During its initialization, rpc.lockd calls init().  Among other things, init
will call cancel_mon().  cancel_mon calls stat_mon() several times.  These
cause the lockd to call the statd (SM_PROG) each time.  The first two calls
to statd cause it to clean up its monitor_q.  The third call (SM_SIMU_CRASH) 
causes it to clean up it record_q and call statd_init().
.H 3 "Example 3: server recovery - statd"
As mentioned above, the statd cleans up its monitor_q.  This queue is for
the names of nodes which this statd would like to monitor for a crash.  Since
this node's lockd just crashed, it no longer cares about the crash of other
nodes.  All it needs to do now is clean up its environment and notify the
nodes which it was dealing with (as server or client).
.sp
In statd_init(), the statd will handle this notification process.  First
it will read the state file (/etc/sm/state).  This contains an integer which
is used to keep track of the "state" of the lockd.  statd will increase this
value to the next odd integer.  This value is then written back into
/etc/sm/state.  statd then reads all entries of files in the "backup" directory
(/etc/sm.bak) and the "current" directory (/etc/sm) and places them in the 
recovery_q.  Note that the
backup directory may have already contained entries from the last crash for
nodes which could not be notified.  New backup directory files are then created
from the recovery_q and all files in the current directory are removed.  For 
each entry in the recovery_q, the statd
will call statd_call_statd() to notify the remote node of the recovery.  This
notification includes the value of the local state.  If the call_tcp() to the
remote succedes, then statd removes this entry from the recovery_q and the 
corresponding backup directory file.  Otherwise the entry remains on the 
recovery_q and in the backup directory file.  This way, statd knows to retry 
these entries later.  After it is done looping through these entries, 
it checks to see if the recovery_q is empty.  If it is not empty, statd sets
alarm().
.sp
When statd gets the SIGALRM, it will be handled by sm_try().  sm_try will
retry the statd_call_statd() notification for each entry in the recovery_q.
If the notification is successful, it calls delete_name() to remove the entry
from the recovery_q.  After re-trying each entry once, it checks the recovery_q
again; and if it is not empty it resets alarm().
Note that if a node that was recorded in /etc/sm is down, or has been taken 
out of service, or has been put on a different LAN, this retry process will 
continue forever.  
.H 3 "Example 3: client - statd"
Back on the client, the lockd still thinks it is holding a lock on a live
server.  Now the client statd gets the SM_NOTIFY with the new state from
the server statd.  SM_PROG (really sm_prog_1) calls sm_notify() which calls
send_notice().  send_notice() runs through the monitor_q checking each entry
for a match.  If an entry matches the name of the recovered node, then it
calls statd_call_lockd() which uses call_tcp() to notify the local lockd
(PRIV_PROG) of the recovery.
.H 3 "Example 3: client - lockd"
priv_prog receives the PRIV_RECOVERY message and calls proc_priv_recovery().
proc_priv_recovery performs a couple of checks to make sure that this notice is
valid.  Assuming that the notice is valid, it then calls delete_hash() to
remove the hash entry for this node, since it is no longer valid.  Then it
runs through the list of locks associated with this server.  Each lock is
changed into a NLM_LOCK_RECLAIM request (by setting the req->reclaim field)
and an nlm_call() is made to send the RECLAIM request (actually NLM_LOCK_MSG)
to the recovered server.
.H 3 "Example 3: server - lockd"
Now the recovered lockd on the server should still be in its grace period.
Therefore, it should be rejecting all requests except RECLAIM requests.
Since this is a reclaim request, it will handle it.  It then goes through
the normal lock path described in example 1 and replies with an NLM_LOCK_RES.
.H 3 "Example 3: client - lockd"
When the client NLM_PROG gets the reply it also follows the path described
in example 1 until it hits the call to cont_lock.  At this point cont_reclaim()
is called instead.  In cont_reclaim, a switch is done on the status.  If it 
was granted, the request is taken off of the msg_q and wait_q and added to
the grant_q as needed.  If it was denied or blocked, kill_process() is called
to send a SIGLOST to the process that requested the lock.  The default action 
is for SIGLOST to kill the process.
.H 2 "Example 4 - client crash"
This example will illustrate how the crash of a client is handled.
Assume that before the crash, the client was holding a lock on our server.
.H 3 "Example 4: client recovery - main()"
This is basically the same as the server example above.
.H 3 "Example 4: client recovery - statd"
This is basically the same as the server example above.
.H 3 "Example 4: server - statd"
This is basically the same as the server example above except it sends a
PRIV_CRASH to the local lockd.
.H 3 "Example 4: server - lockd"
priv_prog receives the PRIV_CRASH message and calls proc_priv_crash().
proc_priv_crash performs a couple of checks to make sure that this notice is
valid.  Assuming that the notice is valid, it then calls delete_hash() to
remove the hash entry for this node, since it is no longer valid.  Then it
runs through the list of locks associated with this server.  For blocked
locks, it calls remove_wait to remove them from the wait_q.  For granted
locks, it calls delete_le() to remove the lock entry and then calls wakeup()
to check for any lock requests that may have been blocked by this lock.
.H 1 "Improvements Made"
This section describes a couple of improvements that have been put in place
in the lock manager and status monitor.
.H 2 "BIND Support"
In HP-UX 7.0 support for Berkeley Internet Name Daemon (BIND) was 
included.  BIND allows the system administrator to assign a number of 
different internet addresses to a particular node.  Each call to 
gethostby*() passes back the 
list of internet addresses for a particular node.  The node itself may not 
respond to all or any of the given addresses.  This magnifies problems in
the Lock Manager and Status Monitor when using syncronous calls to communicate
with remote daemons.
.sp
Problems occur when the Lock Manager and/or Status Monitor try to contact 
a remote daemon syncronously for the first time.  First, the local daemon 
tries to contact the remotes portmap daemon to determine the port 
binding.  It will try to do this using every internet address that it has 
until either the remote portmap daemon responds, or it runs out of internet
addresses.  The normal RPC way
of doing this is by calling clntudp_create() or clnttcp_create() successively
until the internet addresses are exhausted or a response is 
received.  The clntudp_create() and clnttcp_create()
routines use the pmap_getport() call to contact the remote portmap daemon.  In 
the case were the remote portmap daemon is not reachable, the 
pmap_getport() call will time out after the default timeout period 
(60 seconds).  Meanwhile the RPC program is stuck waiting for a response from
the remote
portmap daemon.  If the node that we are trying to contact is down, the local
daemon will not be able to service any other requests until all attempts to 
contact the remote (with the various internet addresses) have timed out.  This
could cause lockd and statd to be out of commission for several minutes at a
time.  This is certainly unacceptable behavior.
.sp
To fix this problem, lockd and statd attempt to contact the remote portmap
daemon asyncronously.  This is done using the various routines in pmap.c
udp.c and tcp.c.  The basic idea is this.  When the local daemon attempts
contact a remote portmap daemon, it uses the getport() routine found in 
pmap.c.  getport() sends a request to the remote portmap daemon with an RPC
timeout of zero.  It then returns immediately.  When xtimer() goes off, a
check is made to see if we have an outstanding (not replied to) portmap 
request.  If so, the next internet address in the list is tried.
.sp 
Since getport() does not wait for a response from the remote portmap daemon,
we need a mechanism to receive a portmap request.  The svc_run() routine
has been modified in lockd and statd to also listen for portmap replies.  The
new svc_run routine, called pmap_svc_run() is found in pmap.c.  When the
select() call returns, pmap_svc_run() checks to see if the input detected is
on a file descriptor used for portmap requests.  If so, it calls the
recv_from_pmap() which collects the information from the portmap reply, caches
the information and allows the daemon to contact the remote daemon via the 
returned port.  If the select() call does not return a portmap file descriptor,
then svc_getreqset is called (as in the normal svc_run() case).
.H 2 "Mutual Exclusion on the Lock Manager Process"
When a Lock Manager is executed, lots of kernel data structures get cleaned up
as a result of the locking protocol's crash recovery mechanism.  When two 
Lock Managers are started at near the same time it is possible that they will
interfere with each other in the cleanup process since there is no mutual 
exclusion on the nfs_fcntl() system call.  To solve this, the decision was 
made in HP-UX 8.0 to only allow one lock manager to be running at any 
particular time.  Mutual exclusion is accomplished though a binary semaphore.
The semaphore related routines are found in sem.c.  The semaphore code was 
borrowed from inetd with slight modifications.
.sp
On startup, lockd calls seminit() which checks to see if the semaphore is
already set.  seminit() checks to see if the semaphore is set.  If the 
semaphore is not set, we return happily (0).  If the 
semaphore is set, it tries a kill(0) (the rubber knife) on the process id 
that set the semaphore.  If the kill fails, then we 
know that the previous lockd process died 
non-gracefully (i.e. not an exit call).  We then remove the semaphore and 
return again happily (0).  If the kill succedes, we know that another 
Lock Manager deamon is running and we return unhappily (1).  Back in the
main program, if seminit() returns a non-zero value, we log a message and
die.  If seminit() returns 0, we go merily along.
.sp
After the Lock Manager becomes a daemon (fork()), the child calls semsetpid()
which sets the semaphore.  semsetpid() also found in sem.c.  There is a 
window (that should be closed) between 
the time when seminit() is called and the time when semsetpid() is called where
another lockd process could conceivably be started.  This is difficult because
we would like to check the semaphore when we still have terminal affiliations,
but we need to set the semaphore after we have forked so that the pid on the
semaphore is actually the pid of the child.
.sp
Whenever lockd dies gracefully (as a result of an exit call), prior to the
exit, lockd calls semclose() which removes the lock manager semaphore.  The
semclose() routine is also found in sem.c.
.H 1 "Potential Improvements"
This section describes several problems with the lock manager and status
monitor designs which were noted for 6.5, but were not fixed due to time
constraints.  These are possible improvements for the future.
.H 2 "softbreak option"
As usual, Sun assumes that everything is beautiful - the lockd and statd are
alive and well on both the local and remote nodes.  This may not be true for
a variety of reasons, like: the remote has died, the remote does not have
3.2 features (VMS), or simply that they were never started.  If a process on
a Sun requests a lock (local or remote) and the necessary daemons are not
alive, that process cannot be killed and will hang until the node is rebooted
or all the necessary daemons are started.
.sp
On HP-UX, due to our vast superiority, most of these cases are handled 
gracefully.  Since the kernel handles local locks, local locking works
without any of the daemons.
.nf
        1) no local lockd - the kernel catches this and returns ENOLCK.
        2) remote lockd not started - the local lockd catches this and
				      returns ENOLCK.
        3) remote node cannot be reached - the local lockd catches this
				      and returns ENOLCK.
        4) remote lockd has died after it registered with portmap - we
				      act like Sun.
.fi
To handle case (4), we had investigated the possibility of a "softbreak"
option, sort of like the -soft option to mount.  Most of the code to
support this is in place, but commented out.  The idea was for the option,
"-s soft_timeout" to be specified when rpc.lockd is started.  It would then
be turned on for all remote lock requests FROM that node.  Most of the
code to implement this is in routine xtimer().  xtimer would keep a count
of the number of retransmissions.  When (count * timeout) became greater than
soft_timeout, xtimer would fake an ENOLCK response.  Unfortunately, there may
be a bug in this code and we did not have time to track it.
.H 2 "don't queue requests during graceperiod"
This is described in example (1).
.H 1 "Sun Support"
If you require help from Sun, they are obliged to provide it to us as NFS
vendors.
Their support phone numbers are:
.sp
.nf
	Sun Microsystems NFS Support
	(415) 691-2770
.sp
	Sun Microsystems Technical Support
	(800) 872-4786 (USA-4SUN - yeeeaaach.)
.fi
.TC 2 1 6 0 "\fBLOCK MANAGER / STATUS MONITOR IMS\fR" "~" "~" "~" "Contents"
