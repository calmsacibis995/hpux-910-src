.\"	@(#)Yellow Pages IMS:	$Revision: 1.6.109.1 $	$Date: 91/11/19 14:27:14 $
.\"
.\"  This is the Internal Maintenance Specification for the Yellow Pages (YP)
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
\fBYellow Pages\fR
.sp 2
.ce 1
David A. Erickson
.sp 2
.ce 2
$Revision: 1.6.109.1 $
$Date: 91/11/19 14:27:14 $
.sp 3
.H 1 "Abstract"
This document describes the Yellow Pages (YP) commands and
library code released with HP-UX 6.0 (based upon Sun 3.0).
The target audience for this document includes
HP online support engineers and new engineers assigned to work on the
YP commands or libraries.
.PH "''\fBYellow Pages IMS\fR''"
.bp
.PF "''-~\\\\nP~-''"
.H 1 "Objectives"
The objectives of this document are
to present an overview of the function and structure of the YP and
to discuss the components of the individual YP modules.
It is hoped that detail is sufficient for the next owner of the code.
Information on the YP implementation will be discussed by
module and noteworthy function within each module.
.sp
I'd also like to pass along as much knowledge as I have about specific
problems or opportunities for improvement.
Most of these ideas are interspersed among the function-specific
implementation discussions.
.H 1 "References"
It is assumed that you are familiar with the following documents.
.AL A
.LI
\fIUsing and Administering NFS.\fR
Of particular interest is the "YP Configuration and Maintenance" chapter.
.LI
\fINFS Reference Pages.\fR
The pages you should read are:
.ML . 6 1
.LI
\fIdomainname\fR(1)
.LI
\fIypcat\fR(1)
.LI
\fIypmatch\fR(1)
.LI
\fIyppasswd\fR(1)
.LI
\fIypwhich\fR(1)
.LI
\fImakedbm\fR(1M)
.LI
\fIrpcinfo\fR(1M)
.LI
\fIypinit\fR(1M)
.LI
\fIypmake\fR(1M)
.LI
\fIyppasswdd\fR(1M)
.LI
\fIyppoll\fR(1M)
.LI
\fIyppush\fR(1M)
.LI
\fIypset\fR(1M)
.LI
\fIypxfr\fR(1M)
.LI
\fIypclnt\fR(3C)
.LI
\fIyppasswd\fR(3N)
.LI
\fIypfiles\fR(4)
.LE
.LI
\fIProgramming and Protocols with NFS Services.\fR
Of particular interest is the "YP Protocol Specification" chapter.
.LE
.sp
You may also find the \fINFS RPC/XDR Internal Maintenance Specification\fR
document helpful in learning the intricacies of RPC and XDR.
.bp
.H 1 "Function and Structure of the Yellow Pages"
.H 2 "An Overview"
The Yellow Pages (YP) is a distributed network lookup service; it is
strictly a user-level service.
The YP makes use of the RPC and XDR components of the NFS product over the
network in a transparent fashion, using both TCP/IP and UDP/IP as
the transport protocols.
The YP is composed of a number of databases (maps) and processes which provide
access to the data in the maps.
YP maps are stored on one or more nodes, each called a YP server.
YP clients are nodes which require information from YP servers' maps.
.sp
The YP maps are files in a directory rooted at \fI/usr/etc/yp\fR
(see \fIypfiles\fR(4)) kept on each YP server.
The processes are \fI/usr/etc/ypserv\fR, the YP database lookup server, and
\fI/etc/ypbind\fR, the YP binder.
Both are daemon processes typically activated at system startup time from
within \fI/etc/netnfsrc\fR; \fIypserv\fR(1M) is executed on YP server nodes
and \fIypbind\fR(1M) is executed on YP client nodes.
.sp
If there is more than one YP server, the maps are fully replicated among
them.
One of the nodes is designated as the "master," while the others are
called "slaves."
Changes are made to the maps on the master server, and
the maps are periodically copied to the slave servers.
At steady state, the YP databases are consistent across the master and all
slaves, so a client's access to YP maps is independent of the relative
locations of the client and server machines.
.sp
By running servers on several different nodes, the Yellow Pages service
provides a reasonable degree of availability and reliability.
If a remote node running a YP server process crashes, client processes will
obtain YP services from another available server.
.sp
YP clients and servers are arbitrarily grouped into functional associations
called YP domains.
YP domains are named, and these names must be unique among all YP domains on a
network.
The YP maps within a YP domain are shared by the servers and accessed by the
clients who are members of that domain.
YP servers are located for YP clients using their YP domain names.
The YP domain name is used as the name of a subdirectory of \fI/usr/etc/yp\fR
on each YP server.
.sp
YP maps are files that are made up of any number of logical records; a
search key and related data form each record.
As a lookup service, YP maps may be queried, i.e., a client may ask for
the data associated with a particular key within a map.
Also, every key-data pair within a map may be retrieved.
.sp
A primary purpose of the YP service is to maintain a single set of user and
group ids for all nodes within a YP domain.
This is requisite for file sharing to occur via NFS, since NFS assumes these
ids to be the same from machine to machine, determining clients' file access
rights based on their ids' values.
.sp
The YP programmatic interface is described in \fIypclnt\fR(3C).
Administrative tools are described in \fIypwhich\fR(1), \fIyppoll\fR(1M),
\fIyppush\fR(1M), \fIypset\fR(1M) and \fIypxfr\fR(1M).
Tools to see the contents of YP maps are described in
\fIypcat\fR(1) and \fIypmatch\fR(1).
Database generation and maintenance tools are described in
\fImakedbm\fR(1M), \fIypinit\fR(1M) and \fIypmake\fR(1M).
The command to set or show the YP domain for a node is \fIdomainname\fR(1).
.sp
The \fIypserv\fR(1M)
daemon's primary function is to look up information in its local
collection of YP maps.
It runs only on YP server machines providing data from YP databases.
Communication to and from \fIypserv\fR(1M)
is by means of RPC with XDR.
Lookup functions are described in \fIypclnt\fR(3C)
and are supplied as C-callable functions in \fI/lib/libc.a\fR.
.sp
Four lookup functions perform on a specific map within a YP domain:
\fIMatch\fR, \fI"Get_first"\fR, \fI"Get_next"\fR and \fI"Get_all"\fR.
The \fIMatch\fR operation matches a key to a record in the database
and returns its associated value.
The \fI"Get_first"\fR
operation returns the first key-value pair (record) from the map, and
\fI"Get_next"\fR enumerates (sequentially retrieves) the remainder of the
records.
\fI"Get_all"\fR
returns all records in the map to the requester as the response to a single
RPC request.
.sp
Two other functions supply information about the map other than normal
map entries: \fI"Get_order_number"\fR and \fI"Get_master_name"\fR.
The order number is the time of last modification
of a map.
The master name is the host name of the
machine on which the master map is stored.
Both order number and master name exist in the map as special key-value
pairs, but the server does not return these through the normal lookup
functions.
(If you examine the map with \fImakedbm\fR(1M) or \fIyppoll\fR(1M),
however, they will be visible.)
Other functions are used within the YP
system and are not of general interest to YP clients.
They include \fI"Do_you_serve_this_domain?"\fR, \fI"Transfer_map,"\fR and
\fI"Reinitialize_internal_state"\fR.
.sp
The \fIypbind\fR(1M)
daemon remembers information that lets client processes on its machine
communicate with a \fIypserv\fR(1M) process.
The \fIypbind\fR(1M)
daemon must run on every machine using YP services, both YP servers and
clients.
The \fIypserv\fR(1M) daemon may or may not be running on a YP client machine,
but it must be running somewhere on the network or available through a gateway.
.sp
The information \fIypbind\fR(1M) remembers is called a \fIbinding\fR:
the association of a YP domain name with
the internet address of the YP server and the port on that host at
which the \fIypserv\fR(1M)
process is listening for service requests.
Client requests drive the binding process.
As a request for an unbound domain comes in, the
\fIypbind\fR(1M) process broadcasts on the network trying to find a
\fIypserv\fR(1M) process serving maps within that YP domain.
Since the binding is established by broadcasting, at least one
\fIypserv\fR(1M) process must exist on every network.
(Use the \fIypset\fR(1M)
command to force binding to a server through a gateway.)
Once a binding is established for a client,
it is given to subsequent client requests.
Execute \fIypwhich\fR(1) to query the \fIypbind\fR(1M)
process (local and remote) for its current binding.
.sp
Bindings are verified before they are given to a client process.  If
\fIypbind\fR(1M) is unable to transact with the \fIypserv\fR(1M)
process it is bound to, it marks the domain as unbound, tells the client
process that the domain is unbound, and tries to bind
again.  Requests received for an unbound domain fail
immediately.
Generally, a bound domain is marked as unbound when the node
running \fIypserv\fR(1M) crashes or is overloaded.  In such a case,
\fIypbind\fR(1M) binds to any YP server (typically
one that is less heavily loaded) available on the network.
.sp
The \fIypbind\fR(1M)
daemon also accepts requests to set its binding for a particular domain.
The \fIypset\fR(1M) command accesses the \fI"Set_domain"\fR
facility; it is for directing a client to bind to a specific YP server and is
not for casual use.
.H 2 "Current and Old YP Protocols"
The YP was first implemented by Sun in their 2.0 NFS release.
This version of the YP is known as the "old protocol," Version 1;
you will see it flagged by YPOLDVERS, YPVERS_ORIG or YPBINDVERS_ORIG
in the code.
.sp
You should know that any code you encounter that is old protocol code has been
only code read.
It has never been executed, which explains away many low Branch Flow Analysis
(BFA) numbers at the wave of a hand.
This old protocol code was not thoroughly examined and tested for a few
reasons.
.ML - 6
.LI
The number of customers whose installations are merely Sun 2.0 was estimated
by Sun to be low.
.LI
HP did not have any Sun systems with this release level available, so testing 
directly against old protocol clients was not possible.
.LI
Inasmuch as the above are true, the amount of effort it would take to create
code to test the Version 1 code would be great.
The return on investment would be minimal.
.LE
.sp
None of the Version 1 code is discussed in the functions below.
.H 1 "YP Header Files"
The following header files are delivered in \fI/usr/include/rpcsvc\fR.
.H 2 "yp_prot.h"
contains symbols and structures defining the RPC protocol used among YP
clients and servers.
This file holds the symbolic definitions for the RPC program and procedure
numbers for the current and old YP versions, YPVERS and YPVERS_ORIG,
respectively.
In addition, status values for \fIyppush\fR and standard
YP transactions are declared.
.sp
A few data structures should be noted, too.
.H 3 "datum"
defines the "unit" of data that is retrieved from YP maps.
This definition is also found in the \fIdbm.h\fR header file of the \fIdbm\fR
routines.
.H 3 "dom_binding"
defines the domain binding data structure.
.sp
.nf
	struct dom_binding {
		struct dom_binding *dom_pnext;
		char dom_domain[YPMAXDOMAIN + 1];
		struct sockaddr_in dom_server_addr;
		unsigned short int dom_server_port;
		int dom_socket;
		CLIENT *dom_client;
		unsigned short int dom_local_port;
		long int dom_vers;
	};
.fi
.sp
In order, we have
.ML - 6 1
.LI
A pointer to another dom_binding structure, used by \fIyp_bind\fR(3C)
and \fIypbind\fR(1M) when constructing linked lists of bound domains.
.LI
The name of the YP domain.
.LI
The address of the bound server for that YP domain.
.LI
The port of the bound server for that YP domain.
.LI
The socket being used by the connection.
.LI
The RPC client handle.
.LI
The local port number being used by the connection.
.LI
The YP protocol version number of the server for this domain.
.LE
.H 2 "ypclnt.h"
defines the symbolic error codes for the YP client interface.
The \fIypall_callback\fR structure is also defined, used by the \fIyp_all\fR()
function.
.H 2 "yppasswd.h"
defines the RPC program and version numbers for the YP password program,
\fIyppasswdd\fR(1M).
The structure \fIyppasswd\fR is defined, too, containing the old YP password
and a password structure used to replace the existing entry.
.H 2 "ypv1_prot.h"
contains definitions that are applicable only to Version 1 YP transactions.
For the reasons mentioned in the "Special Note" above, I will discuss this file
no further.
.H 1 "YP Library Functions"
The following functions have been integrated into the HP-UX standard C
library, \fI/lib/libc.a\fR.
In general, each will fail with a \fIYPERR_BADARGS\fR error code if
the \fIdomain\fR parameter is null, or if the map name or key string,
if either is a parameter, is null.
(This is a key issue to "turning off" the YP.
If the YP domain name, as set by \fIdomainname\fR(1), is null, none of the YP
client interface routines will work proceed.)
.H 2 "yp_all()"
is used to transfer all entries in a YP map from YP server to
client in a single RPC request using TCP, rather than UDP.
This provides a much more reliable result than if the map entries were
enumerated one at a time, though the results should be the same if no
problems arise.
.sp
The \fIcallback\fR function passed to \fIyp_all\fR manipulates each of the YP
map's records as they are retrieved, one by one.
The \fIypclnt\fR(3C) page describes this in more detail.
The \fIyp_all\fR() function is used by \fIypcat\fR(1), for example.
.H 2 "yp_bind()"
is at the heart of every YP client request.
To provide a connection to a YP server, \fIyp_bind\fR() is called.
(\fBNote\fR that the actual binding is performed by the \fI_yp_dobind\fR()
function, internal to the \fIyp_bind\fR() module.)
.H 3 "_yp_dobind()"
performs the actual location and verification of a server for a specific YP
domain.
A key issue with \fI_yp_dobind\fR() is that it will \fBnot\fR return, as long
as \fIportmap\fR is up and \fIypbind\fR is registered with it.
.sp
The steps that \fI_yp_dobind\fR() performs in securing a binding are:
.ML - 6
.LI
Talk to \fIportmap\fR; if it is not running, return outright.
\fI_yp_dobind\fR() can go no further.
.LI
If \fIportmap\fR is running, get the port of \fIypbind\fR(1M).
If it is not registered with \fIportmap\fR, return.
.LI
Talk to ypbind, asking it to look up a binding for the required domain in its
list of bound domains.
\fI_yp_dobind\fR() will, as noted above, continue to query ypbind until the
binding succeeds or some internal error is encountered.
.LE
.H 3 "check_binding()"
determines if a binding exists for a given YP domain.
If so, \fIcheck_binding\fR() verifies that the socket that has been bound
has not been closed or changed somehow.
If it has, the domain is made unbound.
.H 3 "check_pmap_up()"
uses a stream to determine if \fIportmap\fR is running.
The \fIconnect\fR() is hard closed, to prevent lingering TCP sockets in
TIME_WAIT states.
\fBNote\fR that this hard close was a fix applied after seeing that
repetitive calls to \fIyp_bind\fR() led to as many TIME_WAIT sockets appearing.
Without doing the hard close, these TIME_WAIT sockets fragmented the
networking memory to a point where no process requiring a large chunk of
memory would run; the system required rebooting to rejoin the fragments.
.H 2 "yp_first()"
calls \fI_yp_dobind\fR() to latch onto a YP server (recall that this call may
"never" return).
Once a binding is established, the "first" record from the chosen map is
returned.
.H 2 "yp_get_default_domain()"
is simply a wrapper for the \fIgetdomainname\fR(2) system call.
.H 2 "yp_master()"
calls \fI_yp_dobind\fR() to latch onto a YP server (recall that this call may
"never" return).
Once a binding is established, the record whose key is "YP_MASTER_NAME" is
returned.
.H 2 "yp_match()"
calls \fI_yp_dobind\fR() to latch onto a YP server (recall that this call may
"never" return).
Once a binding is established, the record whose key matches the desired key is
returned.
.H 2 "yp_next()"
calls \fI_yp_dobind\fR() to latch onto a YP server (recall that this call may
"never" return).
Once a binding is established, the "next" record from the chosen map is
returned.
The "next" record is determined relative to the last record retrieved, either
by a \fIyp_first\fR() or another \fIyp_next\fR() call.
If used in succession, the contents of a map are exhausted once
\fIyp_next\fR() returns the error code \fIYPERR_NOMORE\fR.
.H 2 "yp_order()"
calls \fI_yp_dobind\fR() to latch onto a YP server (recall that this call may
"never" return).
Once a binding is established, the record whose key is "YP_LAST_MODIFIED"
is returned.
.H 2 "yp_unbind()"
will search the local [not the ypbind daemon's] bound domain list and remove
a matching domain's entry, if found.
Associated resources are discarded, as well.
.H 2 "yperr_string()"
simply returns a pointer to an error message that is appropriate for the given
YP client transaction error code, as defined in <rpcsvc/ypclnt.h>.
.H 2 "ypprot_err()"
converts a YP protocol error code to a
YP client transaction error code, as defined in <rpcsvc/ypclnt.h>.
.H 2 "ypxdr functions"
I will not discuss these functions, though they are fundamental to may of the
YP transactions.
Refer to the \fINFS RPC/XDR Internal Maintenance Specification\fR
for more info on XDR.
\fBNote\fR that from 3.0 to 3.2, Sun removed a number of the functions from
this module.
I do not know why, since the functions removed are used!
.H 1 "YP Commands"
\fBNote\fR that all of the administrative YP commands, save \fIyppoll\fR, are
delivered with mode bits set "r-x------" to provide some measure of security
from user-level hackers.
This is contrary to what is generally found in the rest of HP-UX, where mode
bits are often seen to be "r-xr--r--" on files in /etc.
.H 2 "/bin/domainname"
is used to set or examine the YP domain name.
It is quite straightforward, calling the system call \fIsetdomainname\fR() if
there is an argument; calling the system call \fIgetdomainname\fR() if
there is no argument.
.H 2 "/etc/ypbind"
is the YP binder process.
\fIypbind\fR is the link between local YP client processes and a YP server.
\fBNote\fR that at this date the ypbind code has been brought to the 3.2 level;
the same changes as Sun made between 3.0 and 3.2 were made to the HP code.
The only exception is that the code that verifies that \fIypbind\fR is
registered with \fIportmap\fR before the parent exits was left in place.
.sp
One enhancement that could be applied to \fIypbind\fR would be to replace the
#ifdef DEBUG statements with tracing statements as can be found in
\fIypserv\fR.
.sp
The general sequence of operation of \fIypbind\fR is as follows.
.ML - 6
.LI
Scan the arguments; perhaps a log file or the "invisible" option, -v (verbose)
was specified.
The -v option may be useful to you, the developer, because it tells
\fIypbind\fR to not fork and run as a daemon.
Rather, it can be run in the foreground, so you could more easily use the
symbolic debugger.
.LI
Open the log file if one was specified.
.LI
Unregister both Version 1 and 2 occurrences of \fIypbind\fR with
\fIportmap\fR, should they exist.
This makes certain that only one \fIypbind\fR is there for client processes to
use.
.LI
Fork; the parent waits for the child to register its last service with
\fIportmap\fR before exiting.
.LI
The child breaks its terminal affiliation, sets up a signal handler and
registers all of its services with \fIportmap\fR.
The services are TCP and UDP versions of the Version 1 and 2 YP protocols;
hence, you see four \fIypbind\fRs registered if you try "rpcinfo -p" once
\fIypbind\fR has been started.
.sp
The signal handler was added to \fIypbind\fR to unregister it from
\fIportmap\fR, should \fIypbind\fR be either HUP'd, INTerrupted, IOT'd (as by
an internal \fIabort\fR()), QUITted or TERMed (killed).
If it is not unregistered, any client processes will query \fIportmap\fR and
"think" that \fIypbind\fR is still registered and may try to contact it.
Not until an RPC timeout period of roughly 30 seconds occurs will the client
processes give up.
By unregistering \fIypbind\fR, the client processes will quickly know that no
YP transaction can occur.
.LI
Finally, \fIypbind\fR loops, waiting for client requests via
\fIsvc_getreq\fR().
.LE
.sp
Internal functions of note follow.
.H 3 "dispatch()"
handles incoming requests to either@
.ML . 6 1
.LI
simply reply to an "are you alive?" query (YPBINDPROC_NULL),
.LI
set the binding for a given domain (YPBINDPROC_SETDOM - used by
\fIypset\fR and children of \fIypbind\fR) or
.LI
get the binding for a given domain (YPBINDPROC_DOMAIN - used by
\fIyp_bind\fR(), \fIypwhich\fR and \fIyppoll\fR).
.LE
.H 3 "ypbind_get_binding()"
returns the current binding for a given YP domain.
It is the source for the infamous "Server not responding for YP domain XYZ;
still trying" message.
This function
.ML - 6 1
.LI
determines if a server has already been located for the specified YP domain;
.LI
if not, fork children to locate Version 1 and 2 YP protocol servers.
(You may notice that when \fIypbind\fR is searching for a YP server, two
children are visible when doing a "ps -ef.")
.LI
if so, "ping" the server to see if it is alive; if it is still OK, let the
client process know.
If the YP server is not responding (this could occur if the server actually
died or if the server is overloaded to the point where it cannot respond),
\fIypbind\fR will fork children as above to search for another YP server.
.LE
.H 3 "ypbind_ping()"
is used to determine if a YP server is "alive" by executing the YP server's 
YPPROC_NULL procedure.
.H 3 "ypbind_point_to_domain()"
scans through \fIypbind\fR's linked list of "known_domains" to see if the
particular domain is there, whether bound to a server or not.
The YP domain is added if it is not there, with its binding parameters
initialized to indicate no binding has occurred.
.H 3 "ypbind_send_setdom()"
is used by the broadcasting children of \fIypbind\fR which locate the YP
servers.
It makes an RPC client call to the parent process (requesting the
\fIypbind_set_binding\fR() function), asking it to
set the appropriate parameters in its linked list to indicate the YP domain
is bound to a YP server.
.H 3 "broadcast_proc_exit()"
is the signal handler that is called when either of the broadcasting children
exit.
The linked list of known YP domains is traversed to locate the PID of the dead
child, and the broadcast-related parameters are cleared.
.H 3 "ypbind_set_binding()"
is called by \fIypset\fR and children of \fIypbind\fR) to set the internet
address and port of a YP server in the linked list of YP domains.
.H 3 "ypbind_init_default()"
is commented out.
Sun suppressed initial default binding anyway; by commenting
out this code, the \fIypbind\fR object module is smaller and the BFA
numbers are a bit better.
.H 2 "/usr/bin/ypcat"
is the user interface allowing viewing the contents of a YP map.
Its function is pretty basic.
.ML - 6
.LI
Get the command line arguments.
.LI
Just print the transtable if the -x option was specified.
\fBNote\fR:  this table is different from that of \fIypmatch\fR; "hosts" is
mapped to "hosts.byaddr" by \fIypcat\fR, while "hosts" is mapped to
"hosts.byname" by \fIypmatch\fR.
.LI
If -x was not specified, the contents of the specified map are listed.
If the YP server is a Version 1 server, the contents are enumerated in a
one-by-one fashion, using the \fIone_by_one_all\fR() function which uses the
libc functions \fIyp_first\fR() and \fIyp_next\fR().
If the YP server is a Version 2 server, the contents are listed by using the
libc function \fIyp_all\fR(), coupled with the local \fIcallback\fR()
function.
.LE
.H 2 "/usr/bin/ypmatch"
is the user interface allowing matching one or more keys to the contents of
a YP map.
Its function is pretty basic.
.ML - 6
.LI
Get the command line arguments.
.LI
Just print the transtable if the -x option was specified.
\fBNote\fR:  this table is different from that of \fIypcat\fR; "hosts" is
mapped to "hosts.byaddr" by \fIypcat\fR, while "hosts" is mapped to
"hosts.byname" by \fIypmatch\fR.
.LI
If -x was not specified, the specified map is searched for each key listed
by using the libc function \fIyp_match\fR().
.LE
.H 2 "/usr/bin/yppasswd"
is the user interface allowing a user to change their user password in the
passwd YP maps.
Its function is pretty basic.
.ML - 6
.LI
Determine which host is the master server for the YP password maps.
.LI
Verify that the master YP server has the \fIyppasswdd\fR daemon running.
If not, do not continue.
.LI
Verify that the user's name is viable.
.LI
Prompt the user for an acceptable password; once it is OK, forward the
information to the \fIyppasswdd\fR daemon on the master YP server and wait for
its response.
.LE
.H 2 "/usr/bin/ypwhich"
is the user interface allowing a user to determine which host is the bound YP
server for a given domain or even a selected host.
It can also print the host name of the master for each YP map served by the
currently bound YP server.
The general steps \fIypwhich\fR uses in execution follow.
.ML - 6
.LI
Get the command line arguments.
.LI
Just print the transtable if the -x option was specified.
\fBNote\fR:  this table is different from that of \fIypmatch\fR; "hosts" is
mapped to "hosts.byaddr" by \fIypwhich\fR, while "hosts" is mapped to
"hosts.byname" by \fIypmatch\fR.
.LI
If -x was not specified,
.ML . 6
.LI
determine the host that is serving this node's YP client processes,
.LI
determine the host that is serving a remote node's YP client processes or
.LI
list the host name of the master of a specified map or all maps served.
.LE
.LE
.sp
\fBNote\fR specifically that \fIypwhich\fR does not use the \fIyp_bind\fR()
libc function to determine bindings.
Instead, it contacts the local or remote \fIypbind\fR process directly,
querying it for its binding information.
.sp
This is unlike the other /usr/bin commands in that \fIypwhich\fR does not
potentially "wait forever" until a binding occurs if it has not already
occurred.
Instead, it is satisfied by a "not bound" answer from \fIypbind\fR.
As such, \fIypwhich\fR is the command of choice to cause \fIypbind\fR to
seek a binding for a selected YP domain.
.sp
Internal functions of note follow.
.H 3 "getrmthost()"
gets an internet address for a remote node by accessing the YP hosts map, if
YP is running locally.
.H 3 "get_server_name()"
is the source of the "Domain XYZ not bound" message after having queried
a binder process for a binding.
.H 3 "call_binder()"
does the direct call to the local \fIypbind\fR to get binding info.
.H 2 "/usr/etc/rpc.yppasswdd"
is the daemon that responds to requests by \fIyppasswd\fR to change a password
in the YP passwd maps.
.sp
The general sequence of operation of \fIyppasswdd\fR is as follows.
.ML - 6
.LI
Set up a signal handler;
the signal handler was added to \fIyppasswdd\fR to unregister it from
\fIportmap\fR, should \fIyppasswdd\fR be either HUP'd, INTerrupted,
QUITted or TERMed (killed).
If it is not unregistered, any client processes will query \fIportmap\fR and
"think" that \fIyppasswdd\fR is still registered and may try to contact it.
Not until an RPC timeout period of roughly 30 seconds occurs will the client
processes give up.
By unregistering \fIyppasswdd\fR, \fIyppasswd\fR will quickly know that no
changes can be made to YP passwords.
.sp
\fBNote\fR:  it may be worth considering moving this section of code that makes the
signal calls to a point after the process forks to become a daemon.
.LI
Scan the arguments; perhaps a log file or the "invisible" option, -v (verbose)
was specified.
The -v option may be useful to you, the developer, because it tells
\fIyppasswdd\fR to not fork and run as a daemon.
Rather, it can be run in the foreground, so you could more easily use the
symbolic debugger.
.LI
Open the log file if one was specified.
.LI
Unregister \fIyppasswdd\fR with \fIportmap\fR.
This makes certain that only one \fIyppasswdd\fR is there for client
processes to use.
Register itself with \fIportmap\fR.
.LI
The child of a fork breaks its terminal affiliation
and waits for requests from \fIyppasswd\fR via \fIsvc_run\fR().
.LE
.sp
An internal function of note is
.H 3 "changepasswd()"
which responds to a \fIyppasswd\fR request as follows.
.ML - 6
.LI
It matches the current password's entry with the encrypted one provided
by the user.
.LI
If this succeeds, the inherited signals are ignored and a lockfile is created
as a workfile for updating the password file.
\fBNote\fR that this lockfile, /etc/ptmp, is the same one used by
\fIvipw\fR(1M), newly released with 6.0.
.LI
The entire password file is constructed in /etc/ptmp, and renamed to the
original password file.
.LI
If the -m option was specified, fork to perform a \fIypmake\fR(1M).
It may be worth considering forcing a make of the YP passwd maps by adding
"passwd" to the cmdbuf string that is passed to \fIsystem\fR() to execute
\fIypmake\fR(1M).
This would avoid the problem of a user purposely not specifying "passwd" on
the invocation of \fIrpc.yppasswdd\fR in /etc/netnfsrc.
If "passwd" is listed twice on the invocation, it will cause no problems.
.LI
Reinstate the signal handler and send a response to the client process.
.LE
.sp
A point to consider for improving the \fIyppasswd/yppasswdd\fR interface would
be to have the variable "ans" in \fIchangepasswd\fR() have more values than
just 0 or 1, which could be interpretable by \fIyppasswd\fR and printed nicely
for the user.
As it currently stands, all that \fIyppasswd\fR says when the password change
fails is "Couldn't change the YP passwd."
.H 2 "/usr/etc/yp/Makefile (make.script)"
is a makefile that calls /usr/etc/yp/ypmake directly to organize the build of
YP maps on a master YP server.
This file exists only to be compatible with Sun's method of map construction.
The reasons for essentially replacing the makefile by a shell script are
discussed in the \fIypmake\fR section.
.H 2 "/usr/etc/yp/makedbm"
is the tool that creates YP maps; it is used directly by the \fIypmake\fR
shell script.
.sp
The general sequence of operation of \fImakedbm\fR is as follows.
.ML - 6
.LI
Scan the arguments; \fBnote\fR that the options -d, -i, -m and -o
are not used in general.
.LI
Open the input file and the temporary dbm files.
.LI
Build the database in the temporary dbm files using the input file's data,
insert the special YP_* flagged records and rename the temp files to the
final dbm file names.
.LE
.sp
\fBNote\fR - \fImakedbm\fR specifically does not permit records whose keys
are the same to be stored.
This is a key point - we can guarantee to the
user that records encountered in /etc/hosts, for example, will be stored in a
"first one encountered is the only one stored" fashion.
.sp
A fix that needs making is the creation of the temporary and final dbm file
names to account for short or long filename systems.
.sp
Another point to consider would be to have \fImakedbm\fR access the newly (as
of 6.0) provided /usr/lib/libdbm.a functions rather than the YP's own
separate versions of the routines.
.H 2 "/usr/etc/yp/stdhosts"
is a tool used only by the \fIypmake\fR shell script; in fact, there is no man
page for this command.
It works to read either stdin or a passed file name, and line by line,
converting the first word to an internet address from characters and
back to characters "in the same breath."
Lines beginning with a '#' in column 1 are skipped over.
The conversion is written to stdout.
.sp
\fBNote\fR that you can get weird results if the first word on a line is not an
internet address.
For example, someone wrote a comment in a file, but the '#' was not in column
1 of the input file.
The output contained 255.255.255.255 as the internet address.
It may be worth considering changing \fIstdhosts\fR to skip any line
whose first non-whitespace character is a '#'.
.H 2 "/usr/etc/yp/ypinit"
is a shell script used to initialize master and slave YP servers.
It is a considerable revamp of what Sun provided, for a couple of reasons.
I figured that it was a fairly visible bit of code since it is
human-readable, so it was worth making it more organized and more thoroughly
commented.
.sp
I will let the internal comments themselves describe the script.
One addition to Sun's basic script is the inclusion of interrupt handling.
According to SEs, they were unsure of the state of things if the script was
interrupted or an error was encountered.
In either case, any databases that have been built are now removed entirely.
.sp
By SEs suggestion, \fIypinit\fR could be improved by allowing a user to
specify an alternative YP domain name on the command line, rather than always
retrieving the name by using \fIdomainname\fR.
This addition was made for the 6.2 release of NFS Services/300 - see the DOM
parameter for ypinit(1M).
.H 2 "/usr/etc/yp/ypmake"
was created to replace the /usr/etc/yp/Makefile that Sun uses to generate YP
maps.
The fundamental reason for creating a shell script rather than using the
Makefile is this:  the Sun's Makefile has a defect that cannot be fixed easily
or cleanly by keeping the file as a makefile.
The defect is that if you want your host to act as the master YP server for
more than one YP domain, you need a way to determine if EACH of the maps in
EACH of the domains is current with respect to its source file.
.sp
Sun uses empty files as timestamps, but they are not domain-specific in their
identities.
Instead, they are only map-specific, i.e., the files exist in /usr/etc/yp with
names like "hosts.time" and "protocols.time".
The time of last modification of such a file is changed when the corresponding
map is updated.
Consequently, if one YP domain is made current by building all its maps and
all the timestamp files are updated, ALL domains are thought to be current.
.sp
While writing the \fIypmake\fR script, I moved the timestamp files into the
domain subdirectories, i.e., /usr/etc/yp/<domain_name>, so this problem would
not occur.
.sp
Another reason for changing the \fIypmake\fR command from a makefile to a
shell script was to make it more generalized and hopefully more easily adapted
to custom YP map generation.
Last, with a shell script, it was easier to make a friendlier user interface
with more meaningful error messages and codes.
.H 2 "/usr/etc/yp/yppoll"
allows the user to find out which host is the master YP server for a map, the
order number for the map and the domain the map is part of.
.sp
The general sequence of operation of \fIyppoll\fR is as follows.
.ML - 6
.LI
Scan the arguments and determine if the local or a remote host is to be
queried.
.LI
Contact the server of choice to retrieve the master name and order number.
.LI
Print the results.
.LE
.sp
An improvement to \fIyppoll\fR would be to make its error messages more
understandable.
For example, if the selected host does not have \fIypserv\fR running, the
current message that is produced is "Can't create UDP connection to
<hostname>."
.H 2 "/usr/etc/yp/yppush"
is used [generally] on a master YP server to cause slave YP servers to
\fIypxfr\fR a map to themselves.
\fBNote\fR that this command may be executed on any node, but the slave
YP servers will still contact the master of the given map for a copy.
.sp
The \fIyppush\fR command is executed by the \fIypmake\fR shell script to get
newly-made maps to be copied by each of the YP servers.
.sp
The 3.0 to 3.2 change that Sun implemented was to double the value of
GRACE_PERIOD to 80 from 40 seconds.
I made this change to our code, but
\fBnote\fR that in large maps like that built
from /etc/hosts (which on our machine has over 6000 lines), even 80 seconds
may not be long enough for a slave YP server to complete the transfer and send
a reply.
Not to worry, since even though the initiator of the \fIyppush\fR 
may not get a response from \fIypxfr\fR on the slave YP server, the slave
server will completely transfer the map.
The response, once sent by the slave YP server, will just not be received by
the \fIyppush\fRing host, but that will not affect the success of the transfer
done by the slave.
.sp
The general sequence of operation of \fIyppush\fR is as follows.
.ML - 6
.LI
Scan the arguments to determine which map in which domain is to be pushed.
.LI
Make a linked list of the servers' names from the keys of the
\fIypservers\fR map.
.LI
Register a UDP service that can be called by the \fIypxfr\fR processes to tell
of their successes.
.LI
For each of the hosts in the list of servers,
.ML . 6 1
.LI
contact its \fIypserv\fR, asking to execute its YPPROC_XFR procedure,
.LI
print the state of the transfer request and
.LI
if a call to perform a transfer actually occurred, loop, waiting for either
the alarm timeout (the aforementioned GRACE_PERIOD) or an actual response from
the server.
.LE
.LI
Unregister the temporary UDP callback service.
.LE
.sp
Internal functions of note follow.
.H 3 "make_server_list()"
reads the ypservers YP map, retrieving each of the host names used as keys.
.H 3 "add_server()"
actually builds the linked list of YP server host names, adding in the
internet address.
.H 3 "generate_callback()"
generates the UDP service that will be used by the remote \fIypxfr\fR
processes to call back and tell of their successes.
The program number that is registered with \fIportmap\fR is arbitrarily large
and unusual - 
\fBnote\fR the suggestion below regarding unregistering this service
upon interrupt of \fIyppush\fR.
.H 3 "main_loop()"
traverses the list of YP servers to be contacted, and oversees the calling of
each and interpretation of their responses.
.H 3 "listener_dispatch()"
handles the responses from the remote \fIypxfr\fR processes.
.H 3 "print_state_msg()"
scans through the state_duples array, looking for a matching state code
and printing the associated message.
.H 3 "print_callback_msg()"
scans through the status_duples array, looking for a matching status code
and printing the associated message.
.H 3 "rpcerr_msg()"
scans through the rpcerr_duples array, looking for a matching error code
and printing the associated message.
.H 3 "get_xfr_response()"
is called by \fIlistener_dispatch\fR() function to gather a client call
from a \fIypxfr\fR process.
.H 3 "send_message()"
contacts the \fIypserv\fR process on a YP server, telling it to execute its
YPPROC_XFR procedure.
.sp
An improvement to \fIyppush\fR would be to make its error messages more
understandable.
.sp
Another improvement would be to set up a signal handler like
\fIlistener_exit\fR() to unregister the temporary UDP service from
\fIportmap\fR if \fIyppush\fR is HUP'd, INTerrupted, QUITted or TERMed.
By doing this, no inexplicable service with a large program number would
remain visible when the user enters "rpcinfo -p."
.sp
Last, consider this: could \fIyppush\fR determine that the master for the
chosen map, whose name is certainly [or should be] in the ypservers map, could
be skipped in the calling process?
This would save one unneeded call to a server, since its response will always
be "master's version isn't newer."
.H 2 "/usr/etc/yp/ypset"
allows the user to set the binding for a specific YP domain on a given server.
.sp
The general sequence of operation of \fIypset\fR is as follows.
.ML - 6
.LI
Scan the arguments to determine which host and which domain should be bound to
which host.
.LI
Get the internet address of the host to which the binder process should bind.
.LI
Call the YPBINDPROC_SETDOM procedure on the host's \fIypbind\fR to set the
binding.
.LE
.sp
An internal function of note is
.H 3 "get_host_addr()"
which may be used twice, [possibly] first to get the internet address of
the remote host to which the set is directed, and second, to get that of
the host to which the binder process should bind.
.sp
\fBNote\fR:  this function uses the YP to do a \fIyp_match\fR() on the
host names.
In fact, \fIyp_bind\fR() is called, so if you do not currently have a
binding for the requested domain and you specify the host name rather than
its internet address directly, you may be unable to complete the \fIypset\fR.
This is especially true if your server is on the other side of a gateway, in
which case, your only alternative is to set the binding by specifying the
internet address.
.sp
An improvement to \fIypset\fR would be to make its error messages more
understandable.
For example, if the host name argument is too long, the
error message produced is "Sorry, the hostname argument is bad."\ \ 
See \fIypwhich\fR for better ways of describing this and similar errors.
.H 2 "/usr/etc/yp/ypxfr"
is the command to transfer a specific map from a specific host for a
particular YP domain.
This command is used by the \fIypinit\fR shell script when initializing slave
YP servers, to copy each map from the master YP server.
.sp
The general sequence of operation of \fIypxfr\fR is as follows.
.ML - 6
.LI
Scan the arguments to determine which host and which domain should be bound to
which host.
.LI
Get the name of master server for the map, if it was not specified on the
command line.
.LI
Get the address of the server from which the map is to be copied.
.LI
Try contacting the YP server, by "pinging" it, i.e., calling its YPPROC_NULL
procedure.
.LI
Get the order number and master YP server's name for later insertion into the
locally built map.
This information is retrieved either from the map's actual master or the host
that was specified using the -h option.
.LI
Transfer the map from the specified host.
.LI
Ask the local \fIypserv\fR process to run its YPPROC_CLEAR procedure, if the
-c option was specified on the command line.
.LI
If -C was specified on the command line, send a message back to the invoking
\fIyppush\fR process that started this whole thing off.
.LE
.sp
A point to consider would be to have \fIypxfr\fR access the newly (as
of 6.0) provided /usr/lib/libdbm.a functions rather than the YP's own
separate versions of the routines.
.sp
If we are willing to "risk" it, we could ship \fIypxfr\fR without PARANOID
defined.
This would decrease the size of the code and would also speed up the
transfers.
.sp
Internal functions of note follow.
.H 3 "bind_to_server()"
sets up a UDP connection to talk to the YP server from which the map is to be
retrieved.
This server is either the actual master of the map, of the host as specified
with the -h option.
.H 3 "ping_server()"
talks to the host connected to by \fIbind_to_server\fR(), testing to see that
it is alive by executing its YPPROC_NULL RPC procedure.
\fBNote\fR that the \fIping_server\fR() code is technically wrong.
It uses YPBINDPROC_NULL as the RPC procedure number; this works since
the actual values given these symbolic names are identical.
.H 3 "get_private_recs()"
oversees the retrieval of the map's order number and master name, whose values
are placed in the global variables master_version and master_name,
respectively.
The ASCII character representation of the order number is placed in the global
variable master_ascii_version.
.H 3 "get_order()"
asks the YP server (either the master or the host specified by the -h option)
to execute its YPPROC_ORDER RPC procedure to return the
order number of the requested map.
.H 3 "get_master_name()"
asks the YP server (either the master or the host specified by the -h option)
to execute its YPPROC_MASTER RPC procedure to return the
master name of the requested map.
.H 3 "find_map_master()"
is called only if the -h option is not used.
It uses the libc function \fIyp_master\fR() to get the host name of the
master of the map.
.H 3 "move_map()"
oversees the map's transfer from the host of choice to the local host.
It works in this way.
.ML - 6
.LI
Create the full pathname of the desired map.
.LI
Determine the order number of the local map, if it exists.
Compare this with that of the map to be transferred - does the transfer
need to be done?
Of course, if the -f option has been specified on the command line, the
transfer proceeds regardless.
.LI
Create a filename for a temporary map to hold the transferred map.
It is important to use a temporary map rather than writing to the existing
file because:
.ML . 6 1
.LI
the transfer may fail; if its does, you are left with an incomplete map
instead of an originally good one and
.LI
the contents of the map are unusable until the transfer is complete - this
time lag is much greater than that needed to do a \fIrename\fR().
.LE
.LI
Verify that a temporary database (call it TDB) can be created with this
temporary name.
.LI
Initialize the TDB and copy the records into it using the YPPROC_ALL RPC
procedure of the YP server of choice.
.LI
Add the key records YP_LAST_MODIFIED and YP_MASTER_NAME to the TDB, using
the information retrieved by \fIget_private_recs\fR().
.LI
Verify that the order number has not changed during the transfer process
by getting the order number again from the original server and comparing it to
that just stored.
.LI
Rename the temporary map to the name of the final map.
.LE
.H 3 "get_local_version()"
is used only to retrieve the order number of the local map.
If no local map exists, as would be the case when a new slave YP server is
being initialized by \fIypinit\fR, the order number returned is zero.
In this way, any map will be "older" than this version, and the transfer will
occur.
.H 3 "mkfilename()"
creates the full pathname of the map, as it will be stored once the transfer
is complete.
This is where long standard mapnames are translated to shorter names.
.H 3 "mk_tempname()"
creates a full pathname for the TDB, placing the name in the
/usr/etc/yp/<domain> directory with no prefix.
It may be worth considering changing the null prefix argument to
\fItmpnam\fR(3S) to "ypxfr" or "YPXFR", in case some files get left behind
and their source may more easily be traced.
.H 3 "del_mapfiles()"
is called to remove any temporary databases that are formed.
.H 3 "rename_map()"
renames the files "<TDB>.dir" and "<TDB>.pag" to the final mapname.
.H 3 "check_map_existence()"
is used to determine if the ".dir" and ".pag" files exist for a given name.
.H 3 "new_mapfiles()"
verifies that the files "<TDB>.dir" and "<TDB>.pag" can be created OK.
.H 3 "count_callback()"
is used only if REALLY_PARANOID is defined.
The code is not shipped with this defined, and this function not been tested.
.H 3 "count_mismatch()"
compares the number of entries in the new map with the count of
records that were transferred.
If these values are not equal, the map transfer has been a failure.
.H 3 "get_map()"
gets the YP server of choice to execute its YPPROC_ALL RPC procedure to
transfer all of the records in one TCP fell swoop.
The records are stored in the TDB.
.H 3 "ypall_callback()"
does the actual storing of records into the TDB; it is the function associated
with the YPPROC_ALL transaction.
.H 3 "add_private_entries()"
places the key records YP_LAST_MODIFIED and YP_MASTER_NAME to the TDB, using
the information retrieved by \fIget_private_recs\fR().
.H 3 "send_ypclear()"
talks to the local \fIypserv\fR process to run its YPPROC_CLEAR procedure.
.H 3 "xfr_exit()"
is just the generic exit routine, used as a matter of course when \fIypxfr\fR
detects an error or completes satisfactorily.
.H 3 "send_callback()"
is called by \fIxfr_exit\fR(), and it talks to the invoking \fIyppush\fR
procedure, telling it if the transfer succeeded.
.H 2 "/usr/etc/yp/revnetgroup"
is an undocumented utility used by \fIypmake\fR when creating the netgroup
maps.
The 3.0 to 3.2 change that Sun made was to get it to read entries from stdin,
rather than from the hard-coded name, /etc/netgroup.
This change was made to the HP code.
I will purposefully not delve further into the \fIrevnetgroup\fR code.
.H 2 "/usr/etc/ypserv"
is the server process of the YP.
In general, it is an RPC server that waits for client requests to perform
various tasks.
Most commonly, it provides read access to the YP maps stored on the local host
for any number of YP domains.
Other tasks it performs are described in the procedure descriptions, below.
.sp
The general sequence of operation of \fIypserv\fR is as follows.
.ML - 6
.LI
Scan the arguments; perhaps a log file or the "invisible" option, -v (verbose)
was specified.
The -v option may be useful to you, the developer, because it tells
\fIypserv\fR to not fork and run as a daemon.
Rather, it can be run in the foreground, so you could more easily use the
symbolic debugger.
.LI
Open the log file if one was specified.
.LI
Unregister both Version 1 and 2 occurrences of \fIypserv\fR with
\fIportmap\fR, should they exist.
This makes certain that only one \fIypserv\fR is there for client processes to
use.
.LI
Fork; the child breaks its terminal affiliation, sets up a signal handler
and registers all of its services with \fIportmap\fR.
The services are TCP and UDP versions of the Version 1 and 2 YP protocols;
hence, you see four \fIypserv\fRs registered if you try \fIrpcinfo -p\fR once
\fIypserv\fR has been started.
.sp
The signal handler was added to \fIypserv\fR to unregister it from
\fIportmap\fR, should \fIypserv\fR be either HUP'd, INTerrupted, IOT'd (as by
an internal \fIabort\fR()), QUITted or TERMed (killed).
If it is not unregistered, any client processes will query \fIportmap\fR and
"think" that \fIypserv\fR is still registered and may try to contact it.
Not until an RPC timeout period of roughly 30 seconds occurs will the client
processes give up.
By unregistering \fIypserv\fR, the client processes will quickly know that no
YP transaction can occur.
.LI
Finally, \fIypserv\fR loops, waiting for client requests via
\fIsvc_getreq\fR().
.LE
.sp
The \fIypserv\fR process makes use of a "current map" (call it the CM), one
that has been \fIdbminit\fR()ed and whose name is a global variable.
This is done to avoid the overhead of initializing the database with each
client call, since it is expected that client requests often require more than
one record from the same map in rapid succession.
.sp
Internal functions of note follow.
.H 3 "ypdispatch()"
is the function that diverts RPC client calls to the appropriate internal
procedure for execution.
.H 3 "translate_mapname()"
is a function shared by \fIypxfr\fR to convert the standard YP mapnames to
shorter names suitable for use on 14-character max filename
length HP-UX filesystems.
The number and names of the standard YP map names must remain constant.
.H 3 "ypserv_map.c module"
These functions provide general map-access features.
.H 4 "ypcheck_map_existence()"
just does a \fIstat\fR on the argument (after appending ".dir" and ".pag") to
see if the map exists.
This does not guarantee, however, that the map contains anything of interest.
.H 4 "ypget_map_order()"
uses the dbm \fIfetch\fR() function to get the YP_LAST_MODIFIED record from
the CM.
.H 4 "ypget_map_master()"
uses the dbm \fIfetch\fR() function to get the YP_MASTER_NAME record from
the CM.
.H 4 "ypset_current_map()"
sets the value of the CM to the requested map, after building its full
pathname.
The CM is then \fIdbminit\fR()ed.
.H 4 "ypclr_current_map()"
\fIdbmclose\fR()s the CM and clears the value of the CM.
This is also the YPPROC_CLEAR procedure,
called by the \fIypdispatch\fR() function.
This clearing function is important to \fIypxfr\fR, since \fIypxfr\fR
.ML . 6 1
.LI
creates a temporary map into which the remote map is copied,
.LI
renames that new map to an existing mapname after the transfer is complete and
.LI
since the CM may very well be that existing map, the file must be closed
before the new contents can be seen.
This clearing process is readily seen as when someone tries
.ML "" 6 1
.LI
% ypmatch xyz passwd
.LI
xyz:lPfx4Ofgd3t9U:711:100:....
.LI
% yppasswd xyz  # Change the password
.LI
% ypmatch xyz passwd  #  No change, yet
.LI
xyz:lPfx4Ofgd3t9U:711:100:....
.LI
% ypmatch xyz passwd  #  Change seen
.LI
xyz:amb8931vgk4lq:711:100:....
.LE
.LE
.H 3 "ypserv_proc.c module"
These functions provide the RPC procedures executable by YP [RPC] clients.
.H 4 "ypdomain()"
provides the YPPROC_DOMAIN and YPPROC_DOMAIN_NONACK procedures,
called by the \fIypdispatch\fR() function.
When YPPROC_DOMAIN_NONACK is called by \fIypbind\fR, it is asking if this
server serves the specified YP domain.
This is the broadcast method used by \fIypbind\fR to bind to a YP server.
.H 4 "ypmatch()"
is the YPPROC_MATCH procedure,
called by the \fIypdispatch\fR() function.
It sets the CM and directly queries the map, looking for the passed key.
.H 4 "ypfirst()"
is the YPPROC_FIRST procedure,
called by the \fIypdispatch\fR() function.
It sets the CM and directly queries the map, fetching its first record.
.H 4 "ypnext()"
is the YPPROC_NEXT procedure,
called by the \fIypdispatch\fR() function.
It sets the CM and directly queries the map, fetching its next record.
.H 4 "ypxfr()"
is the YPPROC_XFR procedure,
called by the \fIypdispatch\fR() function.
It forks and executes \fIypxfr\fR as the result of a request by \fIyppush\fR.
.H 4 "ypall()"
is the YPPROC_ALL procedure,
called by the \fIypdispatch\fR() function.
It forks, clears the CM, and transfers each of the map's records.
.H 4 "ypmaster()"
is the YPPROC_MASTER procedure,
called by the \fIypdispatch\fR() function.
It sets the CM and directly queries the map, fetching the special
YP_MASTER_NAME record.
.H 4 "yporder()"
is the YPPROC_ORDER procedure,
called by the \fIypdispatch\fR() function.
It sets the CM and directly queries the map, fetching the special
YP_LAST_MODIFIED record.
.H 4 "ypmaplist()"
is the YPPROC_MAPLIST procedure,
called by the \fIypdispatch\fR() function.
This routine is used by a "ypwhich -m," which is literally saying, "Tell me
the names of all of the maps that you serve."
The work is done by the \fIyplist_maps\fR() function in the
\fIypserv_ancil.c\fR group, below.
.H 3 "ypserv_ancil.c module"
These functions provide ancillary utilities to the rest of the \fIypserv\fR
functions.
.H 4 "ypmkfilename()"
creates the full pathname of the map, as it is expected to exist locally.
This is where long standard mapnames are translated to shorter names; it
is just like the \fImkfilename() function of \fIypxfr\fR.
.H 4 "ypcheck_domain()"
verifies that the directory /usr/etc/yp/<domain> exists.
This is the test used by \fIypdomain\fR() (YPPROC_DOMAIN_NONACK)
that determines if this YP server serves a particular YP domain.
\fBNote\fR - it in no way guarantees that any necessary maps exist within the
directory.
A possible change to this routine might be to take things a step further, and
verify that a few key databases exist, like the passwd and hosts maps.
.H 4 "yplist_maps()"
provides a list of the names of maps (as determined
by those files that exist with both ".pag"and ".dir" suffixes) stored in
some /usr/etc/yp/<domain>  directory.
This functionality is used by the \fIypmaplist\fR() function which is used in
a "ypwhich -m" scenario.
.sp
A point to consider would be to have \fIypserv\fR access the newly (as
of 6.0) provided /usr/lib/libdbm.a functions rather than the YP's own
separate versions of the routines.
.H 1 "Potential Improvements"
As noted earlier, most ideas in this department are interspersed among the
function-specific implementation discussions.
I'll make specific note of a few here, too.
.ML . 6
.LI
First and most important is to ask if you can come up with methods to
speed up the YP, overall.
It is a big task, yes.
.LI
Could some or all of the \fIget*by*\fR() functions cache values?
This, of course, is not a change to the YP, but the introduction of the YP has
has caused some serious performance degradations to these functions.
For example, try
.ML % 6 1
.LI
netstat -a
.LI
domainname ""
.LI
netstat -a
.LE
.sp
and see the remarkable difference.
This may be caused in particular by the \fIgetservby*\fR() functions, which do
not do keyed YP lookups in the services maps.
Instead, they do sequential searches, each of which is a separate networking
transaction that consumes readily noticeable time.
.LI
Is there a way to automatically update the ypservers map when a new YP server
comes online?
This way, the system administrator on the master YP server would not have to
guess a priori which hosts will be the slave servers.
.LI
Could the YP routines all be linked with the new 6.0 /usr/lib/libdbm.a
functions, thus eliminating the local YP libdbm code?
.LE
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
.TC 2 1 6 0 "\fBYellow Pages IMS\fR" "~" "~" "~" "Contents"
