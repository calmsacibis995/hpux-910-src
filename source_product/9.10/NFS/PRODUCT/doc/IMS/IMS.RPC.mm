.\"	Format using "nroff -mm"
.nr Cl 4	\"	save 4 levels of headings for table of contents
.sp 4
.ce 1
Internal Maintenance Specification
.sp 2
.ce 3
NFS RPC/XDR Library
NFS RPC/XDR Commands
NFS Build Environment
.sp 2
.ce 1
John A Dilley
.sp 2
.ce 2
$Date: 91/11/19 14:27:05 $
$Revision: 1.8.109.1 $
.sp 4
.H 1 "Abstract"
This document describes the NFS RPC and XDR commands and
library code released with HP-UX 6.0 (based upon Sun 3.0).  I will
also describe the NFS build environment used by our group to build the
NFS services product.  The target audience for this document includes
HP online support engineers and new engineers assigned to work on the
RPC/XDR commands or libraries, or the NFS development/build environment.
.bp
.H 1 "Objectives"
The specific objectives of this document are:
to present an overview of the structure and function of the code I
have worked on;
to provide information about the NFS project team build and
development environment;
to discuss in detail the design, implementation, data structures,
algorithms, and tradeoffs in the RPC/XDR library and commands code,
in suitable detail for the next owner of my code; and
to pass along as much knowledge as I have about specific problems or
opportunities for improvement.
.sp
I will be starting out my discussion of the design and implementation
with a high level overview, followed quickly by implementation
details.  The remainder of this document will be mainly details of the
RPC/XDR implementation to bring a new engineer up to speed quickly.
.sp
.H 1 "References"
.I "Programming and Protocols with NFS Services"
written by Annette Parsons as part of the NFS product documentation.
This document discusses NFS, RPC and XDR, and gives a very good
overview of the external RPC/XDR interface.  I highly recommend you be
familiar with the information presented before reading this document.
.sp
.I "NFS Project Development Environment Overview"
written by Cristina Mahon in August, 1987.  This document discusses the
environment at a high level, and is appropriate for those wishing to
evaluate the development environment for possible use in a project.
.sp
.I "NFS Project Development Environment and Build Process"
written by Cristina Mahon and John Dilley, last revision in October 1987.
This document discusses the environment in detail suitable for users
and maintainers of the development environment and/or build process.
This document is sufficiently detailed and independent from RPC/XDR
and NFS, so I will not cover any other build/development environment
information.  The remainder of this document will concentrate mainly
on RPC and XDR design, implementation and data structures.
.bp
.sp
.H 1 "Design and Implementation"
I will begin by discussing the design (structure) of the RPC/XDR
library and commands code, followed by specific implementation
details including the programmatic interface (functions) and
user\-level interface (commands).
.sp
The interactions between RPC/XDR library modules are very extensive
and complex.  I will begin by describing the interactions at a higher
level, and will finish by showing example code which implements the
interactions.
.sp
You can think about RPC/XDR as a layer in the ISO protocol stack; the
following diagram is my view of how they fit together:
.nf

Service		ISO Layer		Description

NFS, YP		7 (Application)		Transparent access
High-RPC	6 (Presentation)	Programmatic access
Mid-RPC/XDR	5 (Session)		Moderate transparency
Low-RPC/XDR	5 (Session)		No transport transparency
UDP/TCP		4 (Transport)		End-to-end connection
IP		3/2 (Network)		[Inter]Network routing
IEEE/Ethernet	1 (Physical)		Transmission medium

.fi
At the highest level, the NFS and YP services make the use of the
network transparent to the casual user.  He doesn't have to know that
the remote file being visited resides physically on a remote disk.  At
the highest RPC level, the user is able to programmatically access remote
functions as if they are in the same address space as his program.  No
explicit XDR translation is necessary, as that function is encoded
in the higher-level RPC functions themselves.  They call already
defined remote procedures, with existing XDR routines.
The user does not need to know anything about the underlying transport
protocols, since that is already chosen and transparent.  At the
middle and lower layers, the user must encode more protocol
information in the program, but the actual reading and writing from
the socket is provided transparently by the RPC/XDR package.
Using XDR enables programs to communicate with remote procedures
running on nodes with different hardware or operating systems.  The
data translation required is the responsibility of the user, but most
of the necessary XDR building blocks are provided -- the user has but
to choose which one to use for primitive data types or how to combine
them in the case of complex data types (made up of the primitives).
Note that RPC/XDR are tied together at the middle and lower layers.
Breaking them into separate ISO levels is misleading, since they work
together, and do not form cleanly distinct layers.
.sp
As noted, RPC (XDR) can communicate using either TCP/IP or UDP/IP as
the transport protocol.  The user is only allowed to choose which
transport to use at the lowest layer of RPC; the middle layer always
uses UDP/IP and the at highest layer the choice of which transport to use
is already made and encoded in the function.
The UDP based RPC functions automatically
handle retransmissions, to provide more reliability than the
underlying protocol; TCP based RPC uses a "record marking" standard
that defines RPC message boundaries in the TCP byte-stream.  Without
this, the remote application would be unable to determine where RPC
messages begin and end.  The choice of the underlying protocol (TCP or
UDP) depends upon the needs of the user -- if small amounts of data
are to be transmitted on a mainly reliable network (eg. a Local Area
Network (LAN)), then UDP is probably the best choice.  If, on the
other hand, the user needs to transmit a (potentially) large amount of
data, or if the network is less reliable (eg. a Wide Area Network (WAN)),
then the TCP protocol should be used.
.sp
.H 2 "Implementation Details"
The specific details of the implementation will be discussed by
functionality.  First, I will cover how the RPC/XDR library code fits
together, then I will discuss the RPC/XDR commands.
.sp
.H 3 "RPC/XDR Library"
.sp
The RPC/XDR library consists of all the routines which implement RPC.
The library code can be broken up into three distinct "layers" by
looking at how transparent access to the network is.  At the highest
layer, network access is completely transparent -- the user makes a
function call with a host name as an argument, and the function takes
care of making the remote procedure call itself.  It returns data
to the caller in the expected (host-dependent) format.  At the middle
layer, the user must have some knowledge about the RPC package, and
the service being contacted.  In specific, the user must know the host
name, the service number (program, version and procedure numbers), and
must supply XDR routines to encode and decode the data.  There are a
number of XDR routines already supplied, so the user does not
necessarily have to write new ones, but some more extensive knowledge
about the RPC system is required.  At the lowest layer, the user
explicitly creates the client and server side data structures, and may
or may not create the underlying transport socket.  RPC timeouts and
authentication methods also must be considered at the lowest layer;
the defaults can be used, but the lowest layer is the only one at
which the user can adjust these values.
.sp
.H 4 "RPC Highest Layer"
The highest layer RPC routines reside in the library librpcsvc.a;
these provide remote procedures such as
.I rnusers()
(how many users are
on the named system).  They use the middle layer RPC to provide
this functionality, and in themselves are not very interesting.
Consult
.I "Programming and Protocols for NFS Services"
for a list of which higher-level functions are available and what
they do.
.sp
.H 4 "RPC Middle Layer"
The middle layer of RPC consists of the functions
.I callrpc()
and
.I registerrpc().
They are the simplest interface to the RPC package, and allow the user
to call or register an RPC procedure by host, program, version and
procedure numbers.  These values uniquely identify a remote procedure.
.H 5 "callrpc()"
When calling a remote procedure the user must also specify the input
and output locations, as well as the XDR translation routines to be
used to encode the input to the remote procedure, and decode the
resulting output from the remote procedure.  There are a number of
predefined XDR routines (such as
.I xdr_long()
to translate a long integer, or
.I xdr_void()
to pass no data), which can be used as-is or as building blocks for
user defined special-purpose XDR.  The middle layer can only use
UDP/IP as its transport mechanism, and most configurable values
(timeouts, most notably) are already chosen.  To alter the default
values for timeouts, or to use TCP/IP as the transport the lowest
layer must be used.  Using
.I callrpc()
it is possible to call any UDP-based remote procedure.  Typically,
these procedures will have registered themselves using
.I registerrpc().
.H 5 "registerrpc()"
The interface on the server side takes arguments for the program,
version and procedure numbers, the name of the function to implement
the procedure (called the dispatch function) and two XDR routines to
encode and decode the data to and from the dispatch routine.  The
dispatch routine is somewhat simplified (at least as compared with the
lowest layer dispatch routines); it takes as its sole argument a
generic [character] pointer
.I (char\ *),
which it treats as its input
(post-XDR, of course).  The function may use the input (or not), and
computes its result.  A pointer (again, a generic character pointer)
is returned to the caller (RPC subsystem), which encodes the data to
be sent out according to the appropriate XDR routine, and sends it as
the response message to the caller.  More details on the exact
method of delivery of RPC messages to servers will be discussed under
"Algorithms".  See the discussion of
.I svc_run()
and
.I svc_getreq()
in particular.
.sp
.H 4 "RPC Lowest Layer"
The lowest layer RPC functions allow the user to choose which
transport mechanism to use, what the timeouts and buffer sizes should
be, allow the user to control XDR allocation and deallocation of
memory, and also allow the use of authentication.  These routines
allow the user to deal directly with the transport sockets, while
still providing the ability to use XDR easily.  On the client side,
the
.I clntudp_create()
and 
.I clnttcp_create()
functions return a pointer to a "client handle" (typedef CLIENT).
This handle is a structure which is described in the "Data
Structures" section.  It is used by the client side to make remote
procedure calls; more details will be provided under "Algorithms".
The corresponding routines on the server side are
.I svcudp_create()
and
.I clnttcp_create()
which create and return a pointer to a "service transport" (typedef
SVCXPRT).  This handle is used to receive remote procedure call
requests, and will be discussed in enormous detail soon.
.sp
The client and service handles contain private data areas for
send and receive buffers, the socket used for the particular connection,
and they encode the functions which are to
be called for the different operations on the handle; for instance
"call remote procedure" or "destroy client handle" on the client side,
"receive request", "reply to request", "get RPC arguments", "free RPC
arguments" or "destroy transport handle".
.sp
.H 3 "RPC Commands"
The RPC commands fall into three groups.  The first group includes the
portmapper,
.I portmap.
The second group contains the RPC client programs
.I rpcinfo,
.I rup,
.I rusers,
.I rwall
and
.I spray.
The third group contains the RPC servers for these client programs:
.I rpc.rusersd,
.I rpc.rstatd,
.I rpc.sprayd,
.I rpc.mountd
and
.I rpc.rwalld.
The rpc.* servers are all started from the inetd.
.H 4 "portmap"
is the central registry point for the RPC package.  It listens at the well
known ports
.I 111/UDP
and
.I 111/TCP
for RPC service requests.  When an RPC service is started it makes an
RPC call to the portmapper to register itself (using RPC program
PMAPPROG, procedure PMAPPROC_SET).  The portmapper keeps an entry in
an internal table of all registered programs.  When an RPC client
wishes to contact a server on a given host, it contacts the portmapper
on that host (using procedure PMAPPROC_GET) with the program, version
and procedure numbers of the desired remote service.  The portmapper
either responds with a port number on which the service is listening,
or zero (0) which indicates that there is no such service on that
host.  Since the portmapper is the only program which uses a well
known port, the RPC system only has to know that one port to be able
to contact remote RPC services.  This prevents the need for a table of
service to port mappings for RPC (the file
.I /etc/services
maps ARPA/Berkeley services to their standard (well known) port numbers).
.sp
The RPC client programs will be discussed briefly -- consult the
on-line manual pages for more information about how these commands
work.
.sp
.H 4 "rpcinfo"
is used to contact a single remote service, or dump the portmapper's
table of available remote services.  It can make a TCP or UDP
connection to any given remote program or just contact the portmapper
for the list (using procedure PMAPPROC_DUMP).
It uses lower-level RPC to accomplish these tasks.
When dumping the
.I portmap(1M)
table it uses a TCP connection (since the data may be more than 8K).
When making a contact to a remote service (to simply see if it is up)
it contacts the program and version with procedure zero (0), also
known as NULLPROC.  By convention, all remote programs implement
procedure NULLPROC, which takes no arguments and returns no results
(use
.I xdr_void 
in both cases).  Middle-layer RPC enforces this itself by responding
to calls to NULLPROC on the server side itself; it does not invoke the
dispatch routine to handle NULLPROC.
.H 4 "rup"
is used to get information similar to the output of
.I uptime(1)
on remote hosts.  It is similar to Berkeley
.I ruptime(1C)
except that it gathers the data interactively; Berkeley
.I ruptime(1C)
merely displays data sent and gathered by the
.I rwhod(1M).
If given host name arguments it only contacts those
hosts using UDP, contacting
.I rpc.rstatd
for the kernel information (kept in the kernel data structure called
.I avenrun ;
other information is also returned, but
.I rup
ignores it).
If given no host name arguments, it sends an RPC UDP broadcast to all
hosts on the network requesting this information.  The RPC broadcast
call receives responses and displays the new ones until it times out
(after about three minutes).  RPC broadcast messages are directed to
the portmapper on all hosts on the network.  The portmapper then makes
an indirect call (called via procedure PMAPPROC_CALLIT) to the appropriate
routine (in this case, program RSTATPROG), and has the results sent back to
the caller.
.H 4 "rusers"
is very similar to
.I rup(1)
except that it displays information about which users are on the
system.  It also either sends to the named hosts or uses the RPC
broadcast to send to all hosts listening on the network.  The Berkeley
service
.I rwho(1)
is its equivalent.
.H 4 "rwall"
is used to send
.I wall(1M)
messages to remote hosts.  It takes host or netgroup names as
arguments, and text to send as standard input.  It packages up the
text (which must be no larger than 8K, since UDP is used as the
transport) and sends it to the remote
.I rpc.rwalld
server(s).  The message is then sent to all users via
.I wall(1M)
on their system.
.H 4 "spray"
is used for rudimentary performance measurements.  It sends a number of
UDP packets of a given size (default is just headers (RPC/UDP/IP; 86 bytes),
with zero bytes of data) to the named remote system, and then asks how
many packets made it to the remote server,
.I rpc.sprayd .
The 
.I spray
client then displays the total number of packets sent, received, the
throughput (in bytes/sec) and the number of dropped packets.  This
information can be used to estimate how efficient the remote system is
at receiving RPC/UDP data and what the maximum rate of transfer is.
Note that larger packets typically cause higher throughput.
.sp 2
The last group includes the servers which handle RPC client requests.
.H 4 "rpc.mountd"
is the server for remote
.I mount(1M)
requests.  It receives requests to mount or unmount a file system, or
to dump the current list of mounts.  It can also reply with the
current export list, from
.I /etc/exports .
When requested to mount a file system, it first determines if the
request is valid (ie. the requested directory exists and is not a
remote device), and if the directory is being exported to the
requester (as determined from the information in
.I /etc/exports ).
If all that passes, the mountd adds the new mount to the list (kept in
.I /etc/rmtab )
and returns a file handle to the caller.
.H 4 "rpc.rusersd"
is the server for
.I rusers(1)
requests.  It reads through
.I /etc/utmp
to determine who is logged in and on which tty.
.H 4 "rpc.rstatd"
is the server for
.I rup(1)
requests.  It reads the kernel symbol table (using
.I nlist(2)
on
.I /hp-ux )
and reads the relevant data structures from
.I /dev/kmem .
It packages up the kernel data and returns it in its RPC reply.  The
data gathered by 
.I rpc.rstatd
includes:
cpu time spent in each of the 4 CPU states,
virtual memory performance data,
kernel network interface statistics,
disk transfers,
system boot time and
system load average.
.H 4 "rpc.sprayd"
receives
.I spray(1M)
requests.  First it receives a request to clear the statistics, then
it receives a number of UDP data packets, which it counts but returns
no reply.  Finally it receives a request to return the statistics,
which include the number of packets received and the elapsed time.
Note that if the rpc.sprayd has not been previously started,
the first spray request will behave poorly (due to
the time it takes the inetd to start the rpc.sprayd server).  If the
server is configured to be an "exit" server (-e option in
.I /etc/inetd.conf )
then all requests will start a new rpc.sprayd, and performance will be
consistently poor.
.H 4 "rpc.rwalld"
handles
.I rwall(1M)
requests; it receives the UDP message and opens a pipe to
.I wall(1M)
which actually sends the message to all users.
.sp
.H 1 "Data Structures"
There are a large number of important data structures used in the
RPC/XDR library code.  I will present them and explain briefly what their
fields are used for.  More information will follow in the "Algorithms"
section next.  Most of the major data structures contain structures
within them which act as "switches" -- the contained structure is an
"operations" structure which is comprised  of pointers to
functions which implement the various operations which can be
performed on that particular data type.  Become familiar and
comfortable with this concept, or life in RPC land will be difficult.
Basically, the "operations" structure contains a number of entries
which look like the following line:
.nf
	int	(*operation)();		/* some operation */
.fi
This declares "operation" as a pointer to a function which returns an
integer.  When used, the function will be dereferenced and passed
arguments.  Its return value will be used or stored somewhere (unless
it is of type
.I void).
.sp
.H 2 "RPC/XDR Library Data Structures"
The data structures used most frequently by the RPC/XDR package are
the client handle (typedef CLIENT) and service transport (typedef
SVCXPRT).  These are defined in the <rpc/clnt.h> and <rpc/svc.h>
include files, and they are created and used by RPC library routines.
These data structures are used by the RPC commands, but only through
the predefined RPC library interface.
.sp
I will also discuss other relevant data structures such
as the XDR structure, structures used for authentication, the
portmapper structures, etc.
.sp
.H 3 "CLIENT"
First, the CLIENT structure.  This structure is used on the client
side primarily to encode the operations used.  The operations are
encoded as pointers to functions to do the various tasks, such as
"call a remote procedure" or "destroy the CLIENT handle".  These
functions will be different for each type of CLIENT (eg. UDP or TCP)
and are set and returned by the CLIENT creation routine (eg.
.I clntudp_create() 
or
.I clnttcp_create()).
There is also space in the structure for authentication information (a
pointer to an authentication structure (AUTH\ *)), and a pointer to
private data.  This private data is used by the RPC subsystem and
should not be examined or modified by the user.  Examples of its use
are: saving the TCP socket number, transmission wait interval, RPC
error (if any), pointer to an XDR structure and a private data area.
For UDP, the private data area has a socket number, remote address,
timeout, RPC error, XDR structure and some other data.  These private
areas are how the RPC subsystem keeps track of where it is in the data
stream, and manages the underlying transport transparently to the user.
For more details on private data, you must refer to the appropriate
source code.
.sp
The CLIENT structure is defined in <rpc/clnt.h> and looks like:
.ne 15
.nf

typedef struct {
    AUTH		*cl_auth;	    /* authenticator */
    struct clnt_ops {
        enum clnt_stat	(*cl_call)();	    /* call remote procedure */
        void		(*cl_abort)();	    /* abort RPC call */
        void		(*cl_geterr)();	    /* get RPC error code */
        bool_t		(*cl_freeres)();    /* free alloc'ed results */
        void		(*cl_destroy)();    /* destroy this structure */
    } *cl_ops;
    caddr_t		cl_private;	    /* private data area */
} CLIENT;

.fi
.H 3 "SVCXPRT"
The SVCXPRT (service transport) structure is used on the server side
to encode the information needed to provide the RPC server as
transparent an interface as possible to the underlying transport.  It
encodes the operations which may be performed on the service transport
(again as pointers to functions), as well as holding transport
information such as socket number, and remote address.  The SVCXPRT
structure is created by the
.I svcudp_create()
and 
.I svctcp_create()
functions.
.sp
The SVCXPRT structure is defined in <rpc/svc.h> and looks like:
.ne 20
.nf

typedef struct {
    int			xp_sock;	    /* transport socket */
    u_short		xp_port;	    /* associated port number */
    struct xp_ops {
        bool_t		(*xp_recv)();	    /* receive incoming req */
        enum xprt_stat	(*xp_stat)();	    /* get transport status */
        bool_t		(*xp_getargs)();    /* get arguments */
        bool_t		(*xp_reply)();	    /* send reply */
        bool_t		(*xp_freeargs)();   /* free alloc'ed args */
        void		(*xp_destroy)();    /* destroy this struct */
    } *xp_ops;
    int			xp_addrlen;	     /* length of remote addr */
    struct sockaddr_in 	xp_raddr;	     /* remote address */
    struct opaque_auth 	xp_verf;	     /* raw response verifier */
    caddr_t		xp_p1;		     /* private */
    caddr_t		xp_p2;		     /* private */
} SVCXPRT;

.fi
Both of these structures are used implicitly by the RPC subsystem in a
couple of ways.  First, and most common is the use by the client side
macros: CLNT_CALL, CLNT_ABORT, CLNT_GETERR, CLNT_FREERES and
CLNT_DESTROY, and the server side macros: SVC_RECV, SVC_STAT,
SVC_GETARGS, SVC_REPLY, SVC_FREEARGS and SVC_DESTROY.  Each of these
macros invokes one of the function operations of the handle, passing
whatever arguments are necessary.  For example, the CLNT_CALL macro is
defined like this:
.nf

#define	CLNT_CALL(rh,proc,xargs,argsp,xres,resp,secs)	\\
	((*(rh)->cl_ops->cl_call)(rh,proc,xargs,argsp,xres,resp,secs))

.fi
This rather murky invocation dereferences the first argument (which is
presumed to be a CLIENT handle), and uses the cl_call field of the
cl_ops structure within it as a pointer to a function, which it calls
giving the CLIENT handle itself, and the other arguments passed to
CLNT_CALL.  The include file <rpc/clnt.h> contains the definitions of
all the client side macros.  The server side macros are defined in
<rpc/svc.h>, along with SVCXPRT.
.sp
On the server side, there is one more data structure tied in with the
SVCXPRT.  When an RPC request arrives, the dispatch routine (lower
layer only) receives a pointer to the service transport set up, plus a
pointer to a service request (svc_req) structure.  This structure
describes the RPC call being made, including the program, version and
procedure being called, the credentials and a pointer to the service
transport associated with this call.  This structure is also defined
in <rpc/svc.h>, and looks like:
.ne 10
.nf

struct svc_req {
	u_long		rq_prog;	/* service program number */
	u_long		rq_vers;	/* service protocol version */
	u_long		rq_proc;	/* the desired procedure */
	struct opaque_auth rq_cred;	/* raw creds from the wire */
	caddr_t		rq_clntcred;	/* read only cooked cred */
	SVCXPRT	*rq_xprt;		/* associated transport */
};

.fi
.H 3 "XDR"
The XDR structure is used in the conversion between the internal and external
data representations.  It is similar in design to the CLIENT and
SVCXPRT structures, in that it has a structure containing pointers to
functions which perform the basic operations required, and various
private (XDR also has public) data areas.  The operations include "get
a long integer from the stream", "put a long integer into the stream",
"get [put] some bytes from [to] the stream" or "destroy the stream".  An
XDR stream can be used to encode data, decode it, or free it.  The
operational functions (such as putlong) are responsible for
determining what direction is being used, and doing the right thing.
The XDR structure is defined in <rpc/xdr.h> and looks like:
.ne 19
.nf

typedef struct {
    enum xdr_op	x_op;		  /* XDR operation direction */
    struct xdr_ops {
	bool_t	(*x_getlong)();   /* get long from stream */
	bool_t	(*x_putlong)();	  /* put long to stream */
	bool_t	(*x_getbytes)();  /* get bytes from stream */
	bool_t	(*x_putbytes)();  /* put bytes to stream */
    	u_int	(*x_getpostn)();  /* bytes off from beginning */
    	bool_t  (*x_setpostn)();  /* reposition the stream */
    	long *	(*x_inline)();	  /* pointer to buffered data */
    	void	(*x_destroy)();	  /* free privates of this XDR */
    } *x_ops;
    caddr_t 	x_public;	  /* users' data */
    caddr_t	x_private;	  /* pointer to private data */
    caddr_t 	x_base;		  /* private used for position info */
    int		x_handy;	  /* extra private word */
} XDR;

.fi
The x_public pointer is available to the user, and can be used to pass
more authentication information (more than the RPC package provides),
or for any other use.  It is passed without any interpretation between
client and server.  Note: this means no XDR translation is done, so
the data is not necessarily portable.  The x_base field is used
internally; it is used to store the base memory address of the arena
used by xdr_mem (which provides XDR translation in memory).  It is not
used at all by certain other XDR modules (like xdr_rec which we cover
shortly).  The x_private pointer is used by the XDR subsystem to hold
instance-specific information, such as the standard-IO file pointer
(FILE\ *) being used by xdr_stdio (XDR on top of stdio); or a pointer
to the current position in memory for xdr_mem; or a pointer to the
instance of the record stream for xdr_rec (which provides the record
marking used with TCP).  Just for kicks, here is the RECSTREAM
structure that xdr_rec's xdrs\->x_private points to:
.ne 26
.nf

typedef struct rec_strm {
	caddr_t	tcp_handle;	/* tcp socket used for transport */
	/*
	 *	outgoing data
	 */
	int	(*writeit)();	/* function to write outgoing data */
	caddr_t	out_base;	/* output buffer (points to frag hdr) */
	caddr_t	out_finger;	/* next output position */
	caddr_t	out_boundry;	/* data cannot go past this address */
	u_long  *frag_header;	/* beginning of current fragment */
	bool_t	frag_sent;	/* true if buf. sent in middle of rec */
	/*
	 *	incoming data
	 */
	int	(*readit)();	/* function to read incoming data */
	u_long	in_size;	/* fixed size of the input buffer */
	caddr_t	in_base;	/* base of private XDR arena */
	caddr_t	in_finger;	/* location of next byte to be had */
	caddr_t	in_boundry;	/* can read up to this location */
	long	fbtbc;		/* fragment bytes to be consumed */
	bool_t	last_frag;	/* true if last fragment to send */
	u_int	sendsize;	/* send buffer size */
	u_int	recvsize;	/* receive buffer size */
} RECSTREAM;

.fi
Describing in detail the function of this data structure and the XDR record
marking standard is out of the scope of this document.  Suffice it to
say that record marking provides the same functions as the other XDR
schemes, but it does it on top of a byte-stream protocol instead of a
datagram protocol.  Record marking inserts its own private information
into the stream so that individual RPC messages (records) can be
passed.  The internal implementation details are really too gross for words.
.sp
As with the other major structures we've seen so far, there are a
number of macros for dealing with XDR handles: XDR_GETLONG,
XDR_PUTLONG, XDR_GETBYTES, XDR_PUTBYTES, XDR_GETPOS, XDR_SETPOS,
XDR_INLINE and XDR_DESTROY.  These all invoke the appropriate field of
x_ops with any needed arguments.  These macros are also defined in
<rpc/xdr.h>.
.sp
.H 3 "AUTH"
The data structure used for authentication will look very familiar.
It also uses the structure containing pointers to functions to
implement different types of authentication using one data type.  The
AUTH structure is defined in <rpc/auth.h>, and looks like:
.ne 15
.nf

typedef struct {
    struct	opaque_auth	ah_cred;	/* auth credentials */
    struct	opaque_auth	ah_verf;	/* auth verifier */
    union	des_block	ah_key;		/* DES key (future) */
    struct auth_ops {
	void	(*ah_nextverf)();
	int	(*ah_marshal)();	/* nextverf & serialize */
	int	(*ah_validate)();	/* validate verifier */
	int	(*ah_refresh)();	/* refresh credentials */
	void	(*ah_destroy)();	/* destroy this structure */
    } *ah_ops;
    caddr_t ah_private;
} AUTH;

.fi

The ah_cred field contains the opaque credentials passed between
client and server; ah_verf is the response verifier passed between
server and client.  The opaque_auth structure is defined as:
.ne 7
.nf

struct opaque_auth {
    enum_t	oa_flavor;		/* flavor of authentication */
    caddr_t	oa_base;		/* address of auth. data */
    u_int	oa_length;		/* at most MAX_AUTH_BYTES */
};

.fi
The oa_flavor field determines the type (or "flavor") of
authentication being used; oa_base is a pointer to the authentication
data itself, and oa_length is the length of the data.  There are three
predefined authentication types, AUTH_NONE (which passes no data),
AUTH_UNIX (which passes standard UNIX values (see 
.I "Programming and Protocols for NFS Services" )),
and AUTH_SHORT, which is
basically a "compiled" version of AUTH_UNIX credentials, used to save
time and space when passing authentication information.  The server
returns the AUTH_SHORT verifier in response to a valid request from
the client.
.sp
.H 2 "RPC Commands Data Structures"
All of the data structures discussed so far are handled (created,
modified and used) by library routines.  The RPC commands all use this
predefined interface to access them.  There are only a few RPC
commands which have data structures interesting and important enough
to discuss separately.  These include the rpc.mountd with its mountlist
structure, and the rpc.sprayd with its spraycumul structure.
.sp
.H 3 "Portmapper Structures"
The portmapper
.I ( /etc/portmap )
keeps an internal list of registered program, version and procedure
numbers, along with the associated port on which a program is assumed
to be listening.  The internal data structure is available to remote
programs as a pre-defined XDR structure.  There are library calls to
interface with the portmapper (which will be discussed under
"Algorithms") and XDR routines defined to translate the results.  The
relevant structures are defined in <rpc/pmap_prot.h> and look like:
.ne 14
.nf

struct pmap {
	long unsigned pm_prog;		/* program number */
	long unsigned pm_vers;		/* version number */
	long unsigned pm_prot;		/* protocol number */
	long unsigned pm_port;		/* port number */
};

struct pmaplist {
	struct pmap	pml_map;	/* as above */
	struct pmaplist *pml_next;	/* next in the chain */
};

.fi
There are routines for dealing with these structures, namely:
PMAPPROC_NULL, PMAPPROC_SET, PMAPPROC_UNSET, PMAPPROC_GETPORT,
PMAPPROC_DUMP and PMAPPROC_CALLIT.  They are all defined procedure
numbers in <rpc/pmap_prot.h>, and access the portmapper via the
.I callrpc()
or
.I CLNT_CALL()
interfaces.  They will be discussed further under "Algorithms".
.sp
.H 3 "mountlist"
The rpc.mountd keeps a list of all file systems which have been
remotely mounted.  The list consists of machine, path name pairs.
When a remote mount is requested, it adds an entry to this list.  The
mountlist is also kept in a file
.I ( /etc/rmtab )
in case the rpc.mountd dies, or the system crashes.  When the system
comes back up, the previously mounted file systems are available for
remote access again (the mounting system does not have to re-mount the
file system since NFS is stateless).  The mountlist structure is
defined in <rpcsvc/mount.h>, and looks like:
.ne 7
.nf

struct mountlist {			/* what is mounted */
	char *ml_name;			/* remote system name */
	char *ml_path;			/* local file system */
	struct mountlist *ml_nxt;	/* rest of list ... */
};

.fi
The mountlist can be displayed by the 
.I showmount
command.
.sp
.H 3 "spraycumul/sprayarr"
The rpc.sprayd is a rather interesting case.  It is sent UDP requests
(to procedure SPRAYPROC_SPRAY) and keeps track of how many it
successfully receives.  This data is summed up until the next "clear"
(SPRAYPROC_CLEAR) request is sent.  The
.I spray
command first sends a SPRAYPROC_CLEAR request to clear the numbers,
and then sends a number of SPRAYPROC_SPRAY requests (the number and
size are determined by command line arguments).  After sending all of
its spray requests (without waiting for confirmation -- similar to
"batch-mode" UDP) it make an RPC call for SPRAYPROC_GET, which dumps
the contents of the spraycumul structure.  The structure itself is
defined in <rpcsvc/spray.h>, and looks like:
.ne 11
.nf

struct spraycumul {
	unsigned counter;		/* number of packets received */
	struct timeval clock;		/* time taken to receive data */
};

struct sprayarr {
	int *data;			/* arbitrary data of	*/
	int lnth;			/*    predefined length */
};

.fi
The counter is the number of packets received, and the clock is the
total amount of time between receiving the SPRAYPROC_CLEAR and the
SPRAYPROC_GET requests.  After receiving this data,
.I spray
computes the number of packets dropped, and the total number of
packets per second received by the
.I rpc.sprayd.
The sprayarr structure is the one sent by
.I spray
to
.I rpc.sprayd.
The packets have a user-determined length (lnth), and the contents are
lnth integers (of no determined value).  The data is received but
ignored by the sprayd.
.sp
.H 1 "Algorithms"
This chapter will address how the different parts of the RPC and XDR
code fit together.  The primary links are within the RPC/XDR library
code again, involving the CLIENT and SVCXPRT structures and how they
tie in with CLNT_CALL and the underlying XDR method.  I will describe
how a single XDR routine is used to both encode and decode data,
the steps in a RPC request on the client and server sides in detail,
and will concentrate on how certain crucial routines operate (eg. the
svc_run() and svc_getreq() team).  I will include information about
both the UDP/IP and TCP/IP transports where relevant, but most of the
description  will be the same for both transports.  I will discuss
only the lowest layer of RPC; the higher layers use this one, thus it
is where the "real action" is.
.sp
.H 2 "XDR Encode/Decode"
The XDR structure defined above contains a field (x_op) which defines
whether this XDR call is encoding data to the stream, or decoding data
from the stream.  Each of the low-level XDR functions (eg.
x_putlong()) must examine this information and do the right thing.  At
the higher level, the user does not need to examine this information;
the basic XDR functions provide this transparency.  For this reason,
the user can write a single XDR function which is used for both
encoding and decoding, using the low-level XDR routines (such as
xdr_long()).
.sp
.H 2 "Calling Side"
On the calling side, the RPC client must first obtain a CLIENT handle,
either via
.I clntudp_create()
or
.I clnttcp_create().
If using UDP, the remote address must be
determined, and a value for the call per-try timeout must be selected; these
values are used along with the program and version numbers in the call to
.I clntudp_create().
A valid CLIENT pointer is returned, or NULL if a problem occurs.  When the
CLIENT structure is set up, memory is allocated for the CLIENT itself
and the private client UDP data area.  The private data includes space
for the send and receive buffers (which will hold an entire RPC
packet, header (but only RPC header, not UDP or IP) and all).  If the
remote host address has a port of zero,
.I clntudp_create()
contacts the remote portmapper for the port number of the remote
service.  If the user requests that a new socket be created (by giving
RPC_ANYSOCK), this is done, and the authentication is set by default
to AUTH_NONE.  The creation routine creates an XDR memory stream using
.I xdrmem_create()
and uses it to serialize the outgoing RPC header.
.sp
If using TCP, the procedure is essentially the same except no timeouts
are specified, and the buffer sizes to use must be explicitly stated.
As with UDP, a new socket will be created and the remote portmapper
will be contacted if need be.  If the user passes in a socket, it must
be an active socket (ie. the user has already done a connect() to the
remote port).  The TCP CLIENT private data also includes an XDR record
stream (created by
.I xdrrec_create())
which is used to serialize and de-serialize the RPC header and message.
.sp
After getting a valid CLIENT structure, RPC calls may be made using
it.  The CLNT_CALL() interface is the one typically used, which uses
the clnt_ops switch in the CLIENT structure to invoke
.I clntudp_call()
in the case of UDP, or
.I clnttcp_call()
for TCP.  In both cases, the first thing done is the RPC header is put
in the outgoing stream.  The macros
.I XDR_PUTLONG()
and friends are used to put data to the underlying XDR stream (a simple
memory buffer for UDP, a more complex one for TCP).  Then the
authentication data is sent, followed by the user's data.  The caller
must supply XDR routines to encode and decode the arguments and
results.  These are assumed to be pointers to functions, and are called
(dereferenced) with the XDR instance and pointer to the outgoing (or
incoming, in the receiving case) data.  After the user's data is sent
(to the private data area used for "staging" of the RPC message), the
caller function attempts to send the data.  In the UDP case, this
means calling
.I sendto()
using the socket given, and the data just written to the XDR memory.
.I clntudp_call()
will keep trying to retransmit after intervals of the per-try timeout
until either a response is received, or the call times out after
taking longer than the total timeout.  If the call times out,
.I clntudp_call()
returns with the error code RPC_TIMEDOUT.  Otherwise, a response has
been received and
.I clntudp_call()
calls
.I recvfrom()
to receive the UDP data from the network.  The reply message is
checked for validity (ie. it is a legal XDR header with a response to
this transaction ID), and sufficient authorization.  The user's XDR
decode routine is applied when checking for validity, and its result
is returned (assuming everything else worked).  This completes the call.
.sp
In the TCP case, the handling of the underlying transport is done by
the xdr_rec code.  TCP RPC calls
.I xdrrec_endofrecord()
which determines if the buffer should be sent now, or if it should
remain queued up on the local host.  If the buffer was not actually
sent, the call returns with code RPC_SUCCESS.  Otherwise
.I clnttcp_call()
attempts to receive a response message which matches the transaction
ID of the message that was sent.  Once a valid response is received,
it is authenticated and the results are decoded using the XDR routine
specified by the caller, and a status is returned, either indicating
RPC_SUCCESS or describing the error which occurred.  This completes
the TCP call.
.sp
The other activities which happen on the calling side include
destroying the CLIENT handle, freeing results allocated by XDR, etc.
These functions are all local, and very simple as compared to the
sending and receiving of data done by CLNT_CALL and SVC_RECV.
.sp
.H 2 "Receiving Side"
The interactions on the server side are a little more complex than
those on the client side.  First, the server side socket must be set
up and bound, if desired; otherwise us RPC_ANYSOCK to have the RPC
package do it for you.  Next, the server must set up a service
transport (SVCXPRT).  This transport should then be registered with
the portmapper on the local host using
.I svc_register(),
which may be called any number of times with distinct program, version
pairs which the transport (dispatch) will service.  Finally, call
.I svc_run()
to have the RPC system wait for requests to come in.  When RPC
requests come in,
.I svc_run()
will cause the dispatch routine named by the user to be called with
a pointer to a service request structure and the service transport.
.sp
The purpose and behavior of the functions just mentioned will now be
discussed in more detail.
.H 3 "svcudp_create()"
is called to create the UDP service transport.  If its socket
parameter is RPC_ANYSOCK, a new socket will be created.  In either
case, the socket is bound to a random port on the local host (chosen
by the system when the
.I bind()
system call is handed a port of zero).  Next, memory is allocated for
the SVCXPRT itself, plus local data space to hold incoming and
outgoing messages; memory-based XDR is used in the UDP case for both
of these (similar to the client case).  A pointer to the SVCXPRT
structure is then returned, of NULL if an error occurred.
.H 3 "svctcp_create()"
is used to create the TCP service transport.  The TCP case is more
complex, since there are different "states" a socket can be in: active
(used to send and receive data) or passive (used to receive TCP
connections using the
.I listen()
system call).  The TCP service must be able to accept new sockets, and
then send data on the newly created socket.  To do this, the SVCXPRT
xp_ops (transport operations) switch is set to one of two switches.
One is used to accept new connections on the passive socket, the other
is used to send and receive data on the active socket.
.sp
The first thing that
.I svctcp_create()
does is to create a new socket, if needed (ie. if passed in as
RPC_ANYSOCK).  Next, the socket is bound and a
.I listen()
is executed on it, to make it a passive socket (no connection has been
made yet; when one is made, it will
.I connect()
to this passive socket).  If the user passes in a socket, it must not
be an active socket (since
.I listen()
will fail on active sockets), but it may already be bound to a port
(the return value of the
.I bind()
call is ignored by
.I svctcp_create()
specifically to allow users to pass in already bound sockets).  Next,
memory is allocated for the SVCXPRT structure, which is set up to have
the xp_ops for a rendezvous.  The transport is registered with the RPC
system, and then the structure is returned to the
user, who has the responsibility of registering it as a service
(see then next section covering
.I svc_register()).
.sp
When a call is made to the port on which this service is listening,
.I svc_run()
indirectly (via the SVCXPRT switch) calls
.I rendezvous_request(),
which does the
.I accept()
call to get the new socket, and sets the SVCXPRT to have the other set
of switches, which handle sending and receiving data.
.H 3 "svc_register()"
is used to register a valid service transport with the portmapper
on the local host.  It keeps a record of what transports have been
registered by this process (used by
.I svc_getreq()
when an RPC request comes in), and sends an RPC message to the local
portmap informing it of the program, version, protocol and port being
used by this service.
.H 3 "svc_run()"
is a function which never returns (barring some fatal error).  It
basically just does a
.I select()
system call on all of the registered RPC transport sockets, waiting
for UDP data or TCP connection request packets to arrive.  When
activity is noted on one or more (readable) file descriptors,
.I svc_getreq()
is called with a bitmask describing the sockets with activity.
.H 3 "svc_getreq()"
is the function which does the real work on the server side.  For each
active socket it does a
.I SVC_RECV()
call, which invokes the
.I xp_recv()
function of the SVCXPRT switch.  This function attempts to receive the
data on the socket.  In the UDP case, this amounts to doing a
.I recvfrom()
call on the socket, and verifying that the data received looks valid
(ie. it is a valid RPC call message).  In the TCP case (as already
noted), the new connection is accepted and the SVCXPRT is set to use
.I svctcp_recv()
to receive data when the next
.I xp_recv()
is called.  If the
.I SVC_RECV()
returns successfully,
.I svc_getreq()
checks the list of registered services for a program and version
number match (procedure number need not match -- that is the
responsibility of the dispatch routine itself).
If an exact match was not found,
.I svc_getreq()
sends back an error message indicating that fact, and includes the
lowest and highest version supported if program number was matched
successfully.  Otherwise, the dispatch routine associated with the
program, version pair is invoked with the svc_req structure generated
from the received message.  The dispatch in this case is responsible
for determining what procedure was called, invoking the appropriate
XDR routines to decode the arguments and encode the results, and
sending the reply message back to the caller.  The dispatch must also
handle authentication, if needed.  When sending replies back to the
caller, there are several routines which can be helpful (especially in
error conditions).  These are: svcerr_noproc (no procedure),
svcerr_decode (can't decode arguments), svcerr_systemerr (some system
error), svcerr_auth (authentication error), svcerr_weakauth
(authentication too weak), svcerr_noprog (program unavailable), and
svcerr_progvers (version unavailable).
.sp
.H 2 "Summary"
This concludes the discussion of the major algorithms associated with
sending and receiving an RPC message.  I consider this the major point
of interest, and will not delve into the algorithms implemented by the
RPC commands, or other library functions.  I feel this document has
gotten too large already, although it is difficult to eliminate any of
this information.  For more information, the source code is the best
reference; hopefully I've covered all the truly obscure algorithms and
interactions, and the rest will be more straightforward.
.sp
.H 1 "Implementation Limitations"
This section covers limitations placed on the design by specific
implementations.  This covers such subjects as limitations on users'
use of authentication, maximum number of remote mounts or remote
connections, maximum UDP transmission size, performance(?), etc.
.H 2 "Authentication"
The biggest limitation (undocumented by Sun) is the fact that users
actually cannot add their own form of authentication without modifying
source code (found in libc/rpc/svc_auth.c).  There is a switch
structure called svcauthsw, which is used by server side
authentication; to add a new flavor of authentication, the user must
modify the structure and add a new entry to the switch (the name of
the new function to perform authentication).  The function must
also be added to the library code, of course.
.H 2 "Resource Limits"
The main resource limit is the UDP buffer size.  This is not
controlled by NFS itself, but rather the underlying transport.  The
UDP transport on HP-UX has been modified to allow UDP packets up to
8Kbytes, the size used by Sun.  The assumption that UDP buffers may be
up to 8Kbytes are implicitly coded in various places (very poor coding
style, but what do you expect from Sun?).  This limits the amount of
data that can be transmitted across RPC/UDP connections, as noted
earlier.
.sp
There is no theoretical limit on the number of remote mounts a server
can handle, nor on the number of mounts a client can attempt.  However
we've found that 512 seems to be the practical limit.  We have
observed core dumps and other unfriendly behavior when we reach the
maximum number.  As the number increases, performance will degrade (as
would be expected).  With a large number of active connections, a
server can become swamped, and lose requests.  Messages will appear on
the consoles of the clients to the effect of "NFS Server not
responding" followed by "NFS Server OK" when the next message gets
through.
.bp
.H 1 "Network Headers"
Having a map of the header formats proved useful when debugging LAN
packet dump traces (in Hex) and so on.
Following are the headers for various protocols,
starting from the bottom of the protocol stack and going up.
The main reason this is being provided here is so that people who have
only a LAN trace can work backwards and figure out who sent what RPC
message to what remote server, and what was the response.  The XID
(transaction identifier) is very useful here, since the XID uniquely
identifies an RPC call message and its reply.  RPC messages are easy to
read from Hex dumps once you become familiar with where they are in the
packet (word offset 8, if using UDP), since all of the data units are a
full word.  Identifying RPC CALL and REPLY messages is straightforward,
and determining the program, version and procedure numbers is usually
done on sight.
.sp
The headers included here are:
.nf
	IP;
	UDP, TCP;
	RPC CALL, RPC ACCEPTED_REPLY, RPC REJECTED_REPLY.


Header format:
		   1         2         3
bit:	 01234567890123456789012345678901	(most-to-least significant)

word#	|---|---|---|---|---|---|---|---|	(32 bits wide)

IP Header
	|---|---|---|---|---|---|---|---|
1	| V | H | T.o.S |    Length     |	V:	Version
2	|   IP Ident    |   IP Offset   |	H:	Header Length
3	|  TTL  | Proto |   Checksum    |	ToS:	Type of Service
4	|  Source IP Address (32 bit)   |	IP_off:	Fragment Offset
5	| Destination IP Address (32b)  |	TTL:	Time To Live
	|---|---|---|---|---|---|---|---|	Proto:	Higher Level Protocol


UDP Header
	|---|---|---|---|---|---|---|---|
1 (6)	|Source UDP Port| Dest. UDP Port|
2 (7)	|  UDP Length   |  UDP Checksum |
	|---|---|---|---|---|---|---|---|

TCP Header
	|---|---|---|---|---|---|---|---|
1 (6)	|Source TCP Port| Dest. TCP Port|	Off:	Offset of TCP data
2 (7)	|  TCP Sequence Number (32 bit) |	X:	UNUSED (reserved)
3 (8)	|  TCP Acknowledgement (32 bit) |
4 (9)	|Off| X | Flags |    Window	|
5 (10)	| TCP Checksum  |  Urgent ptr	|
	|---|---|---|---|---|---|---|---|
Flags:
	0x01	FIN
	0x02	SYN
	0x04	RST
	0x08	PUSH
	0x10	ACK
	0x20	URG


RPC CALL Header (total word offset assumes RPC/UDP/IP) {all data are 32 bits}
	|-------------------------------|
1 (8)	|    Transaction ID (XID)	|
2 (9)	|    CALL Indicator [0]		|	(MUST be 0 for CALL message)
3 (10)	|    RPC_VERSION [2]		|	(only valid for RPC version 2)
4 (11)	|    RPC Program number		|
5 (12)	|    RPC Version number		|
6 (13)	|    RPC Procedure number	|
7 (14)	|    Authentication type	|
8 (15)	|    Authentication length (al)	|
...	| Authentication data >= 0bytes	|
8+al	|    Verification type		|
9+al	|    Verification length (vl)	|
...	| Verification data >= 0bytes	|
9+al+vl	|    RPC Data (RPC arguments)	|
...	|	(arbitrary length)	|
	|-------------------------------|

RPC Accepted Reply Header (total word offset assumes RPC/UDP/IP)
	|-------------------------------|
1 (8)	|    Transaction ID (XID)	|
2 (9)	|    REPLY Indicator [1]	|	(MUST be 1 for REPLY message)
3 (10)	|    ACCEPTED Status [0]	|	(MUST be 0 for ACCEPTED reply)
4 (11)	|    Verification type		|
5 (12)	|    Verification length (vl)	|
...	|	Verification data	|
5+vl	|    RPC accept_stat		|
either	|-------------------------------|
6+vl	| low_version   supported	|	(if PROG_MISMATCH)
7+vl	| high_version  supported	|	(if PROG_MISMATCH)
 or	|-------------------------------|
6+vl	|	caddr_t result_data	|	(if RPC_SUCCESS)
7+vl	|	xdrproc result_XDR	|	(if RPC_SUCCESS)
	|-------------------------------|
Note:
	The last two words are dependent upon the RPC accept_stat value.

RPC Rejected Reply Header (total word offset assumes RPC/UDP/IP)
	|-------------------------------|
1 (8)	|    Transaction ID (XID)	|
2 (9)	|    REPLY Indicator [1]	|	(MUST be 1 for REPLY message)
3 (10)	|    REJECTED Status [1]	|	(MUST be 1 for REJECTED reply)
4 (11)	|    Rejection status		|	(MISMATCH=0, AUTH_ERROR=1)
either	|-------------------------------|
5 (12)	|  low_version   supported	|	(if MISMATCH)
6 (13)	|  high_version  supported	|	(if MISMATCH)
 or	|-------------------------------|
5 (12)	|  why authentication failed	|	(if AUTH_ERROR)
	|-------------------------------|
.bp
.H 1 "The Last Word"
One of my objectives was to pass along as much knowledge as I have
about specific problems or opportunities for improvement; at the time
of this writing, all of the outstanding DTS reports have been fixed
and resolved, so I do not know of any outstanding code defects.  There
are some performance problems which are present; these fall under both
"problems" and "opportunities", but it is not clear that they can be
solved from within the RPC/XDR library or commands.
.sp
First, the 
.I spray
command -- the HP-UX
.I rpc.sprayd
typically has much worse performance than the corresponding Sun
daemon.  As an example,
.I spray
between two HP9000 Model 350's
often shows a large number of packets dropped, and a lower packet
throughput than
.I spray
between two Sun 3/260's (roughly equivalent hardware).  The Sun 3/260
almost never loses a packet, where we almost always lose a number of
them.  The reason for this is in the kernel; the Sun kernel has no
limit on the amount of memory networking can consume, so all incoming
UDP packets are saved in kernel mbufs.  The HP-UX kernel limits the
number of UDP packets which can be saved to three.  The 
.I spray
program sends as many UDP packets as it can to the remote host and it
is clear that the one with the UDP packet queue is likely to drop more
packets than the one without the limit.
.sp
There have also been problems seen with RPC servers in general.  Our
.I inetd
appears to take a much longer time to spawn servers than the Sun
.I inetd.
This is most likely because ours has much greater flexibility
(including security checking, which Sun lacks).  It would be very nice
if the
.I inetd
could be tailored for speed, though; this would help all servers, not
just RPC servers.  With RPC we see timeouts, or multiple receipt of
the UDP request packet, so it is more obvious than with the typical
TCP server (like
.I remshd).
 Making a "profiling" executable of the
.I inetd
and the RPC servers would give valuable clues as to where the programs
are spending their time, and where to look for possible performance
improvements.  The same could be done in the kernel.
.sp
I could go into Sun design defects, but a better source of this
information is the DTS project "nfssun".  It contains a list of
defects found due to Sun design errors; there are also bugs of this
sort in the "nfscmds" DTS group.
.sp
Finally, the original outline for this document included a chapter for
"Environment" (process, execution and development),
"Conventions" (programming and documentataion) and
"Global Data Structures".  However, there wasn't enough new
information to fill out those three chapters.  The build/development
environment information is contained in the second and third
references, and the first contains sufficient information about the
other aspects of the environment and global data structures that it
does not make sense to go into it here.  I've already mentioned the
major programming convention used in the RPC/XDR library code (that
being the use of the "function switch structure"), so I think we're
covered there.  I have listed three documents for further reference
and will be available personally on a limited basis for consulting.
For code developers with questions or problems, though, Sun itself
should be the first call.  The Sun support numbers are:
.ne 7
.nf

	Sun Microsystems NFS Support 
	Phone:	(415) 354-4781

	Sun Microsystems Customer Support 
	Phone:	(800) 872-4786

.fi
.TC 1 1 4	\"	create the table of contents, 4 levels
