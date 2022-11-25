.\"	Format using "nroff -mm"
.nr Cl 4	\"	save 4 levels of headings for table of contents
.sp 4
.ce 1
Internal Maintenance Specification
.sp 2
.ce 3
Remote Execution (REX)
.sp 2
.ce 1
Mark J. Kean
.sp 2
.ce 2

.sp 4
.H 1 "Abstract"
.sp 2

This document describes the design and implementation of the remote
execution facility know as (REX), which allows a user to execute
commands on a remote host.  REX was originally developed by Sun
Microsystems as part of the NFS release 3.2. This document focuses on
HP's initial release of REX on version 6.5 of HP-UX on the series 300.

This document is provided as an aid to code owners, CPE engineers,
technical support and on-line support.

.bp
.H 1 "Introduction"
.sp 1

This document describes the design and implementation of the remote
execution facility know as (REX), which allows a user to execute
commands on a remote host.  This section provides an introduction to
REX and an overview of the purpose and organization of the document.

.sp 1
.H 2 "Objectives"
.sp 1

The purpose of this document is to provide an overview of the design
and implementation of REX. The intent is that this document will
provide enough information for CPE engineers and code owner's to come
up to speed on REX in a timely manner.

.sp 1
.H 2 "Overview of Functionality"
.sp 1

Rex consists of two parts, the
.ft B
on(1)
.ft
command and
.ft B
rexd(1M)
.ft
(remote execution
daemon). The
.ft B
on
.ft
command provides the user interface on the client.
.ft B
On
.ft
also communicates with
.ft B
rexd
.ft
in order to execute a command remotely.
.ft B
Rexd
.ft
executes on the server and facilitates the execution of the remote command.

The functionality of REX is similar to that of
.ft B
remsh(1)
.ft
with one
important difference. REX will execute the command in an environment
similar to that of the invoking user. The user's environment is
simulated by:
.AL
.LI
Copying all of the user's environment variables to the remote machine
.LI
Mounting the file system containing the user's current working
directory on the remote machine, via NFS (if it is not already mounted
on the remote machine)
.LE

The command is then executed on the remote machine in the remote
version of the user's current working directory, using the invoking
user's environment variables.

.sp 1
.H 2 "References"
.sp 1

.I "Using and Administering NFS Services"
.sp
Chapter 6, "Remote Execution Facility" contains the user information
for REX.
.sp
.I "NFS project ERS"
.sp
The chapter on REX contains the external reference specification for REX. 
This chapter is
similar to the chapter in "Using and Administering NFS Services" however,
it also contains additional information for internal use only.
.sp
.I "Man Pages"

The \fBon(1)\fR and \fBrexd(1M)\fR man pages contain information about
the command and daemon which implement the REX service.

.I "Sun Support"

As an NFS vendor and with customer support contracts HP is entitled to support
from Sun Microsystems both for issues regarding the implementation of
NFS and the machines covered by the support contracts. The numbers to
contact for support are given below.
.ne 7
.nf

	Sun Microsystems NFS Support
	Phone:	(415) 354-4781

	Sun Microsystems Customer Support
	Phone:	(800) 872-4786

.fi

.sp 1
.H 2 "Document Organization"
.sp 1

The remainder of this document consists of five major sections. The
first section describes the design and implementation of the \fBon\fR
command.  The second section describes the design and implementation
of remote execution daemon (\fBrexd\fR). 
The third section gives an overview of the process and communication model
used to carry out the remote execution of a command.
The forth section describes the
BSD terminal emulation facility which is required by \fBon\fR and
\fBrexd\fR in order to execute interactive commands. The final section
describes differences between Sun and HP's implementations.

.sp 1
.H 1 "The on(1) command"
.sp 1

.H 2 "Overview"
.sp 1

The \fBon\fR command provides the user interface for the remote
execution facility. In general, the user
specifies a host to run a command on, the command to run and the
arguments for the command.
\fBOn\fR then sends information to the \fBrexd\fR server on the appropriate
host which allows the host to execute the requested command in an environment
similar to that of the invoking user.
The environment simulation includes:
.AL
.LI
setting environment variables for the
command to the same value they had for the invoking user
.LI
NFS mounting
the invoking user's current working file system (if it is not already mounted)
.LI
\fBcd(1)\fRing into
the current working directory before executing the command.
.LE

Hence, in theory a command executed on a remote machine via \fBon\fR will have
the same results as running the command on the local system. Use of absolute
path names, some relative path names, software organization
and heterogeneous machines can
cause remote command execution to produce different results than local
command execution.

.sp 1
.H 2 "Operation"
.sp 1

As mentioned above, the \fBon\fR command provides the user interface
for remote execution of commands. In order to carry out remote command
execution the \fBon\fR command determines which host in the network
has the user's current working file system physically mounted and
sends the user's request to the \fBrexd\fR daemon on the requested
host. The request is sent to the daemon via RPC calls.

After completion of the RPC calls the \fBon\fR command acts as an i/o
switch for the user and the remote command which is executing. That
is, \fBon\fR receives the output from the remote command and writes it
to the user's standard output and reads input from the user's standard
input and sends it to the remote process.

The details of determining where the user's current working file
system is physically mounted, the RPC calls and the i/o switch are
discussed below.

.sp 1
.H 3 "Locating the User's Current Working File System"
.sp 1

In order for the \fBrexd\fR on the server to execute the command in
the invoking user's current working directory, \fBrexd\fR must NFS
mount the user's current working file system (if it is not already
mounted) and \fBcd\fR to the current working directory.

It is the client's responsibility to tell the server which host has
the invoking user's current working file system physically mounted
(the user may be in an NFS mounted file system which is physically
mounted on another node), which file system the user is in and which
directory within that file system is the user's current working
directory.

The function findmount() determines if the user's current working
file system is local, remote via NFS, remote via HP discless or remote
via RFA. This is done using the following algorithm:

.AL
.LI
Search the mount table for the mount point which
is the longest prefix of the path name of the user's
current working directory.
.LI
Stat the user's current working directory. If
st_remote in the stat structure is TRUE then the
directory is being accessed via RFA. Hence, findmount()
returns with a value indicating this fact.
.LI
If the mount entry associated with the file system
contains a host name followed by a colon then the file
system is remote via NFS. Hence, findmount()
returns with a value indicating this fact.
.LI
Call mysite() to see if we are in a discless cluster.
If mysite() returns 0 we are not in a discless cluster
and we know the file system is local. Hence, findmount()
returns with a value indicating this fact.
.LI
If mysite() returned a value other than 0 we are in a
discless cluster. We must now determine which node in the cluster
has the disc physically mounted. With HP-UX 6.5 it could only
be the root server. In the future, if local discs are
supported, it could be local or on another client. The
following steps will locate the root server but are general
enough to work for the future case.
.LI
Check to see if the value returned by mysite() is equal to
the st_cnode value in the stat structure. If so the disc is
local.
.LI
If not, then call getcccid() with st_cnode to get more
information about the node which has the file system. Use
the string pointed to by cnode_name as the hostname which
has the file system physically mounted.
.LE

If the file system is remote via RFA \fBon\fR will print an error
message and exit. This is done because it is not possible for \fBon\fR
to determine which of the file systems on the remote machine contains
the current working directory.

If an NFS file system is mounted on a symbolic link and the user is in
that NFS file system the search of the mount table will return the
wrong mount entry. This happens because a getwd() call in \fBon\fR
will return the path name for the working directory based on the real
path name, not the symbolic link and the mount table contains the path
name of the mounted file system based on the symbolic link. Therefore
a proper match will not occur when \fBon\fR searches the mount table.
The path name will match a local file system name.

This incorrect information will be sent to \fBrexd\fR and \fBrexd\fR
will mount the wrong file system. When \fBrexd\fR attempts to \fBcd\fR
to the directory within the file system it will fail and exit. I could
not find a reasonable fix for this problem other than to not mount
file systems on symbolic links. Mount them on the real path name. A
simple work around.

.sp 1
.H 3 "The RPC Calls"
.sp 1

The \fBon\fR command communicates the user's request to the server's
\fBrexd\fR via three RPC calls. These RPC calls are REXPROC_START
REXPROC_MODES and REXPROC_WINCH. For noninteractive \fBon\fR commands only the
REXPROC_START call is made. For interactive \fBon\fR commands the
REXPROC_MODES and REXPROC_WINCH calls must also be made. The
REXPROC_MODES call tells the server what the user's current terminal
mode settings are and REXPROC_WINCH tells the server the size of the 
invoking user's tty.

The \fBon\fR command can also make two other RPC calls to the
\fBrexd\fR.  The first RPC call is REXPROC_SIGNAL. When the executing
\fBon\fR command receives a SIGINT, SIGTERM or SIGQUIT it propagates
the signal to the remote process by calling REXPROC_SIGNAL.  All other
signals are handled by the \fBon\fR command.

The second RPC call is REXPROC_WAIT which is used to synchronize
\fBon\fR and \fBrexd\fR when the remote command completes or when one
of the socket connections used to pass input and output between the
user and the command is broken.

Each of the RPC calls is discussed in more detail below.

.sp 1
.H 4 "The REXPROC_START Call"
.sp 1

The REXPROC_START call gives \fBrexd\fR enough information to start a
noninteractive command. Thus only one RPC call is required to start a
noninteractive command.  For interactive commands the REXPROC_MODES
call and REXPROC_WINCH call must also be made before \fBrexd\fR can
start the remote command.

The structure which is passed from the client to the server when a
REXPROC_START call is made is given below.  This structure tells the
server the command to run, where to find the user's current working
file system, what the user's environment variables are, what ports to
use for the command's stdin, stdout and stderr and a flags field.
Currently only one flag is defined. If it is set, it indicates to
the server that the command is interactive.

.nf

/* flags for rst_flags field */
#define REX_INTERACTIVE		1	/* Interactive mode */

struct rex_start {
  /*
   * Structure passed as parameter to start function
   */
	char	**rst_cmd;	/* list of command and args */
	char	*rst_host;	/* working directory host name */
	char	*rst_fsname;	/* working directory file system name */
	char	*rst_dirwithin;	/* working directory within file system */
	char	**rst_env;	/* list of environment */
	u_short	rst_port0;	/* port for stdin */
	u_short	rst_port1;	/* port for stdout */
	u_short	rst_port2;	/* port for stderr */
	u_long	rst_flags;	/* options - see #defines above */
};

.fi

The return value for the REXPROC_START call is the rex_result
structure.  It contains an integer status code and a message string.
If the call was successful then the integer value is 0 and the message
is a NULL string.  If an error occurred during the processing of the
call then the integer value contains a non 0 value and a string which
provides information about the error which occurred. The \fBon\fR
command passes this string on to the user and exits.

.nf

struct rex_result {
  /*
   * Structure returned from the start function
   */
   	int	rlt_stat;	/* integer status code */
	char	*rlt_message;	/* string message for human consumption */
};

.fi

.sp 1
.H 4 "The REXPROC_MODES Call"
.sp 1

The REXPROC_MODES call is used for interactive \fBon\fR commands to
tell \fBrexd\fR what the tty modes are for the invoking user's tty.
This is required because the user's terminal on the client is put in
raw mode for the duration of the \fBon\fR command and the tty
processing is done on the server through the use of a psuedo terminal
known as a pty. See \fBpty(7)\fR.

The rex_ttymode structure used in this call is based on Berkeley
terminal modes. HP systems use System V terminal modes so the System V
terminal modes must be converted to Berkeley terminal modes before
making this call. For more information on this conversion process see
the section "BSD vs. System V Terminal Modes"

.nf

struct rex_ttymode {
    /*
     * Structure sent to set-up the tty modes
     */
	struct sgttyb basic;	/* standard unix tty flags */
	struct tchars more;	/* interrupt, kill characters, etc. */
	struct ltchars yetmore;	/* special Bezerkeley characters */
	u_long andmore;		/* and Berkeley modes */
};

.fi

The REXPROC_MODES call has no return value.

.sp 1
.H 4 "The REXPROC_WINCH Call"
.sp 1

The REXPROC_WINCH call is used for interactive \fBon\fR commands to
tell \fBrexd\fR the size of invoking user's tty.  This is
required because the user's terminal on the client is put in raw mode
for the duration of the \fBon\fR command and the tty processing is
done on the server through the use of a psuedo terminal known as a
pty. See \fBpty(7)\fR.

Sun systems support SIGWINCH which is a signal that is sent to a
process when its window size changes. When Sun's \fBon\fR command
receives SIGWINCH it will propagate the window changes information to
\fBrexd\fR using the REXPROC_WINCH RPC call. HP-UX does not currently
support SIGWINCH so it is not possible for a command to determine if
it's window size has changed. Hence, the information can not be
propagated to the server.

.nf

struct ttysize {
	int     ts_lines;       /* number of lines on terminal */
	int     ts_cols;        /* number of columns on terminal */
};

.fi

The REXPROC_WINCH call has no return value.

.sp 1
.H 4 "The REXPROC_SIGNAL Call"
.sp 1

The REXPROC_SIGNAL call is made whenever the \fBon\fR command
receives a SIGINT, SIGTERM or SIGQUIT signal. All other signals are
handled by the \fBon\fR command and not propagated to the remote
command.  The REXPROC_SIGNAL call propagates the signal to the remote
command. This is done by sending the integer value of the signal to
\fBrexd\fR. \fBRexd\fR is responsible for propagating the signal to 
the remote command.

The REXPROC_SIGNAL call has no return value.

.sp 1
.H 4 "The REXPROC_WAIT Call"
.sp 1

The REXPROC_WAIT call is used to synchronize the \fBon\fR command and
\fBrexd\fR when the remote command terminates. This is done to
ensure that \fBrexd\fR has completed all cleanup processing associated with the
remote command execution before the \fBon\fR command exits.

The REXPROC_WAIT call has no input parameters but it does have a
return value. The return value structure is rex_result (the same as
REXPROC_START's return value structure) and is used to indicate any
errors that occurred during the cleanup processing.  It contains an
integer status code and a message string.

If the cleanup was successful then the integer value is 0 and the
message is a NULL string.  If an error occurred during the processing
of the call then the integer value contains a non 0 value and a string
which provides information about the error which occurred. The
\fBon\fR command passes this string on to the user and exits.

.nf

struct rex_result {
  /*
   * Structure returned from the start function
   */
   	int	rlt_stat;	/* integer status code */
	char	*rlt_message;	/* string message for human consumption */
};

.fi

.sp 1
.H 3 "The i/o switch"
.sp 1

Once \fBon\fR has made the necessary calls to start remote execution
and has created a mechanism to send and receive data to and from the remote
command, \fBon\fR acts as an i/o switch for the user and the remote command
which is executing. That is, \fBon\fR receives the output from the
remote command and writes it to the user's standard output
and reads input from the user's standard input
and sends it to the remote process.

The \fBon\fR command stays in the i/o switch loop until the
communication mechanism to the remote command is closed by the remote
system, an error occurs on the communication mechanism or \fBon\fR
receives a signal which causes it to exit.

If the remote system closes the communication mechanism \fBon\fR will
call REXPROC_WAIT to make sure that \fBrexd\fR has finished it's post
command execution cleanup before exiting.

For a pictorial representation of the i/o switch see the section "The
on/rexd Process and Communication Model".

The code for the i/o switch if given below:
.nf

--------------------------- the i/0 switch -----------------
while (remmask) {
	selmask = remmask | zmask;
	nfds = select(32, &selmask, 0, 0, 0);
	if (nfds <= 0) {
		if (errno == EINTR) continue;
		perror((catgets(nlmsg_fd,NL_SETN,26, "on: select")));
		Die(1);
	}
	if (selmask & (1<<fd0)) {
                /* read the remote command's stdout and write it to 
                   the user's stdout */
		cc = read(fd0, buf, sizeof buf);
		if (cc > 0)
			write(1, buf, cc);
		else
			remmask &= ~(1<<fd0);
	}
	if (!Only2 && selmask & (1<<fd2)) {
                /* If stdout and stderr are not going to the same file,
                   read the remote command's stderr and write it to 
                   the user's stderr */
		cc = read(fd2, buf, sizeof buf);
		if (cc > 0)
			write(2, buf, cc);
		else
			remmask &= ~(1<<fd2);
	}
	if (!NoInput && selmask & (1<<0)) {
                /* read the remote command's stdin and write it to 
                   the user's stdin */
		cc = read(0, buf, sizeof buf);
		if (cc > 0)
			write(fd0, buf, cc);
		else {
			/*
			 * End of standard input - shutdown outgoing
			 * direction of the TCP connection.
			 */
			zmask = 0;
			shutdown(fd0, 1);
		}
	}
}

--------------------------- the i/0 switch -----------------

.fi
.sp 1

.sp 1
.H 2 "Software Organization"
.sp 1

The code specific to the \fBon\fR command is organized into three
C files, three include files and one library file. The file
names relative to the the development environment and a short
description are given below.

.nf

\fBfile name\fR                         \fBdescription\fR
cmds/usr.etc/rexd/on.c            Main program for the \fBon\fR command
                                    and some functions used by main.
cmds/usr.etc/rexd/where.c         Functions to determine where the
                                    user's working directory is
                                    physically mounted.
cmds/usr.etc/rexd/bsdtermio.c     TTY conversion functions.
cmds/usr.etc/rexd/bsdundef.h      Undefine of symbols common to BSD
                                     and System V tty structures.
include/bsdterm.h                 Defines and structures for BSD
                                     terminal structure.
include/rpcsvc/rex.h              Defines and structures for rex.
cmds/usr.lib/librpcsvc/rex_xdr.c  XDR routines for rex.

.fi

.sp 1
.H 1 "The Remote Execution Daemon (rexd(1m))"
.sp 1

.H 2 "Overview"
.sp 1

The remote execution daemon (\fBrexd\fR) executes on the server and
facilitates the execution of the command specified by the client. The
\fBrexd\fR process authenticates the remote user, NFS mounts the
user's current working file system (if it is not already mounted)
and \fBfork\fRs a child process which \fBexec\fRs the requested command. Upon
completion the \fBrexd\fR process does cleanup tasks such as
unmounting the user's current working file system (if it was mounted by
the \fBrexd\fR process).

.sp 1
.H 2 "Operation"
.sp 1

As mentioned above, \fBrexd\fR facilitates the execution of a command
for the client. When a client makes a request for remote execution on
a server, the server's \fBinetd(1M)\fR starts up a \fBrexd\fR process
to handle the request (If the server is configured to run \fBrexd\fR.
See the \fBrexd(1M)\fR man page). The \fBrexd\fR process then
accepts RPC calls from the client which provide information about the
command to execute.

The \fBrexd\fR process uses the information in the RPC calls to mount
the client's current working file system (if it is not already
mounted) and \fBfork\fR a child which will \fBexec\fR the requested
command in the user's current working directory.

In the case of a noninteractive command execution, once the child
process has \fBexec\fRed the command the \fBrexd\fR process returns to a
loop waiting to process RPC calls from the \fBon\fR process. The \fBon\fR
process may make RPC calls during the execution of the remote command to
propagate signals to the remote command or synchronize with
\fBrexd\fR when the remote command has completed.

In the case of interactive command execution, \fBrexd\fR also handles
the same RPC calls from the client along with an RPC call which
propagates a change in window size to the remote command.  On HP-UX it
is impossible for the \fBrexd\fR process to change the window size for
the command process so all but the first REXPROC_WINCH call is
effectively ignored by HP's \fBrexd\fR.  

For interactive \fBon\fR commands the \fBrexd\fR process also
acts as an i/o switch between a pty which the \fBexec\fRed command uses for
it's stdin, stdout and stderr and the socket(s) that the client uses
to send and receive stdin, stdout and stderr for the command.

The details of mounting the user's current working file system, the
RPC calls and the i/o switch (for interactive commands) are discussed
below.

.sp 1
.H 3 "Mounting The User's Current Working File System"
.sp 1

In order to execute the command in the user's current working
directory \fBrexd\fR must make sure the user's current working file
system is mounted on the server node. The current working file system
may be local to the server, visible to the server as a member of a
discless cluster, already mounted from a remote node via NFS  or it may
need to be mounted from a remote node via NFS.

A simplified version of the algorithm used to determine if \fBrexd\fR
needs to mount the current working file system is given below.

.nf

--------------------------------------------------------------------
-- algorithm for accessing the user's current working file system --
--------------------------------------------------------------------

  if (server is in a discless cluster)
   {
    if (current working file system is visible in the cluster)
     {
      /* the file system is visible locally */
      wdhost= my_hostname;
     }
   }

  if (wdhost=my_hostname) 
   {
    /* file system appears locally, just cd to it */
    chdir(user's current working directory);
   }
  else
   {
    if (AlreadyMounted(user's current working file system))
     {
      /* file system is already NFS mounted, cd to it */
      chdir(NFS mount of user's current working directory);
     }
    else 
     {
      /* need to NFS mount the user's current working file system */
      mount(user's current working file system);
      /* cd to the user's current working directory */
      chdir(NFS mount of user's current working directory);
     }
   }   

--------------------------------------------------------------------
-- algorithm for accessing the user's current working file system --
--------------------------------------------------------------------

.fi

By default, HP's \fBrexd\fR mounts the user's current working file
system in a subdirectory of /usr/spool/rexd. This can be changed by
using the -m option (see \fBrexd(1M)\fR). With SunOS 3.2 \fBrexd\fR
mounts the current working file system in a subdirectory of /tmp. With
SunOS 4.0 \fBrexd\fR mounts the current working file system in a
subdirectory of /tmp_rex. In each case there is no option for Sun's 
\fBrexd\fR to change the directory in which mount points are created.

If \fBrexd\fR mounted the current working file system \fBrexd\fR will
attempt to unmount the current working file system when the command
has completed.  This may fail if some other process has a file or
directory open in the NFS mounted file system. HP's \fBrexd\fR will
attempt 10 times to unmount the file system if it is busy, \fBsleep\fRing
one second in between each attempt. With SunOS 3.2 one attempt is made
to unmount the file system. With SunOS 4.0 6 attempts are made to
unmount the file system if it is busy, \fBsleep\fRing 10 seconds in between.

.sp 1
.H 3 "The RPC Calls"
.sp 1

The \fBon\fR command communicates the user's request to \fBrexd\fR via
three RPC calls. These RPC calls are REXPROC_START REXPROC_MODES and
REXPROC_WINCH. For noninteractive commands only the REXPROC_START call
is made. For interactive \fBon\fR commands the REXPROC_MODES and
REXPROC_WINCH calls must also be made. The REXPROC_MODES call tells
the server what the user's current terminal mode settings are and
REXPROC_WINCH tells the server the user's current terminal window
size.

The \fBon\fR command can also make two other RPC calls to \fBrexd\fR.
The first RPC call is REXPROC_SIGNAL. When the executing \fBon\fR
command receives a SIGINT, SIGTERM or SIGQUIT it propagates the signal
to the remote process by calling REXPROC_SIGNAL.  All other signals
are handled by the \fBon\fR command.

The second RPC call is REXPROC_WAIT which is used to synchronize
\fBon\fR and \fBrexd\fR when the remote command completes or when one
of the socket connections used to pass input and output between the
user and the command is broken.

\fBRexd's\fR processing for each RPC call is described in detail below.

.sp 1
.H 4 "The REXPROC_START Call"
.sp 1

The REXPROC_START call gives \fBrexd\fR enough information to start a
noninteractive command. Thus only one RPC call is required to start a
noninteractive command.  For interactive commands the REXPROC_MODES
call and REXPROC_WINCH call must also be made before \fBrexd\fR can
start the remote command.

Upon receiving a REXPROC_START call rexd will do the following:

.AL
.LI
Validate the remote user.
.LI
If the \fBon\fR command is interactive, make the user look logged in.
.LI
NFS mount the user's current working file system if it is not already 
mounted.
.LI 
If the \fBon\fR command is interactive, allocate a pty for use as the 
command's stdin, stdout and stderr.
.LI
\fBfork\fR a child process which will \fBexec\fR the command.
.LI 
If the \fBon\fR command is interactive the child will wait until the 
parent was received and processed a REXPROC_WINCH call before \fBexec\fRing
the command.
.LI 
If the command is not interactive the child will \fBexec\fR the 
command immediately.
.LE

By default, user validation simply consists of checking to see if the
UID sent by the client is allocated to a user on the server. By
starting \fBrexd\fR with the -r option stronger authentication,
similar but not equivalent to that used by \fBrlogin(1)\fR and
\fBremsh(1)\fR is enforced. This is accomplished by calling
\fBruserok(3X)\fR. The SunOS versions of \fBrexd\fR only support the
UID checking method of user authentication.

If the \fBon\fR command is interactive, the user is logged in using
the account() function call. This function was taken from the
\fBrlogind(1M)\fR code. The account() function is also used to log the user
out when the interactive command completes. The Sun releases use a the
LoginUser() and LogoutUser() functions. The functions used vary between HP
and Sun due to differences in the utmp structure and file.

Allocation of ptys is different on HP and Sun systems. The HP version
uses the function getpty() taken from the \fBrlogin(1M)\fR code.

On Sun systems \fBrexd\fR may start an interactive command before
receiving the first REXPROC_WINCH call.  This is done for efficiency, because
it is possible on a Sun for a process to set the window size for
another process on the fly. However, this means that an interactive
command running on a sun server may have incorrect modes and window
size until the REXPROC_MODES and REXPROC_WINCH calls are processed.

On HP systems it is not possible for a process to set the window size
for another process. Therefore, \fBrexd\fR does as much processing as
possible to start the command and then waits until it has received and
processed the first REXPROC_MODES and REXPROC_WINCH calls before it \fBexec\fRs
the command. This may cause the command startup to take longer than a
Sun system but the command will start with the correct modes and
window size.

The structure which is passed from the client to the server when a
REXPROC_START call is made is given below.  This structure tells the
server the command to run, where to find the user's current working
file system, what the user's environment variables are, what ports to
use for the command's stdin, stdout and stderr and a flags field.
Currently only one flag is defined. If it is set, it indicates to
the server that the command is interactive.

.nf

/* flags for rst_flags field */
#define REX_INTERACTIVE		1	/* Interactive mode */

struct rex_start {
  /*
   * Structure passed as parameter to start function
   */
	char	**rst_cmd;	/* list of command and args */
	char	*rst_host;	/* working directory host name */
	char	*rst_fsname;	/* working directory file system name */
	char	*rst_dirwithin;	/* working directory within file system */
	char	**rst_env;	/* list of environment */
	u_short	rst_port0;	/* port for stdin */
	u_short	rst_port1;	/* port for stdout */
	u_short	rst_port2;	/* port for stderr */
	u_long	rst_flags;	/* options - see #defines above */
};

.fi

After processing the REXPROC_START call, \fBrexd\fR will return a
rex_result structure to the client.  It contains an integer status
code and a message string.  If the call was successful then the
integer value is 0 and the message is a NULL string.  If an error
occurred during the processing of the call then the integer value
contains a non 0 value and string which provides information about the
error which occurred.

.nf

struct rex_result {
  /*
   * Structure returned from the start function
   */
   	int	rlt_stat;	/* integer status code */
	char	*rlt_message;	/* string message for human consumption */
};

.fi

.sp 1
.H 4 "The REXPROC_MODES Call"
.sp 1

The REXPROC_MODES call is used for interactive \fBon\fR commands to
tell \fBrexd\fR what the tty modes are for the invoking user's tty.
This is required because the user's terminal on the client is put in
raw mode for the duration of the \fBon\fR command and the tty
processing is done on the server through the use of a psuedo terminal
known as a pty. See \fBpty(7)\fR. Allocation of the pty is done during
the processing of the REXPROC_START call.

The rex_ttymode structure used in this call is based on Berkeley
terminal modes. HP systems use System V terminal modes so the System V
terminal modes must be converted to Berkeley terminal modes before
making this call. For more information on this conversion process see
the section "BSD vs. System V Terminal Modes"

.nf

struct rex_ttymode {
    /*
     * Structure sent to set-up the tty modes
     */
	struct sgttyb basic;	/* standard unix tty flags */
	struct tchars more;	/* interrupt, kill characters, etc. */
	struct ltchars yetmore;	/* special Bezerkeley characters */
	u_long andmore;		/* and Berkeley modes */
};

.fi

The REXPROC_MODES call has no return value.

.sp 1
.H 4 "The REXPROC_WINCH Call"
.sp 1

The REXPROC_WINCH call is used for interactive \fBon\fR commands to
tell \fBrexd\fR the size of the invoking user's tty.  This is
required because the user's terminal on the client is put in raw mode
for the duration of the \fBon\fR command and the tty processing is
done on the server through the use of a psuedo terminal known as a
pty. See \fBpty(7)\fR.

On HP systems the window size is set by setting the two environment
variables LINES and COLUMNS. These environment variables must be set
for the command before it starts executing since HP-UX does not
provide a mechanism for another process to change them once a process
has started executing. Therefore the child process started by
\fBrexd\fR will wait for the parent to process the first REXPROC_WINCH
call before \fBexec\fRing the command. The values for LINES and
COLUMNS are passed from the parent to the child via a pipe. Once the
command is \fBexec\fRed there is no way for \fBrexd\fR to change the
value of LINES and COLUMNS for the command.

On Sun systems the window size is stored with the tty structure.  Sun
systems support SIGWINCH which is a signal that is sent to a process
when its window size changes. When Sun's \fBon\fR command receives
SIGWINCH it will propagate the window change information to \fBrexd\fR
using the REXPROC_WINCH RPC call. Sun's \fBrexd\fR will in turn
propagate the window change to the command by sending it a SIGWINCH
with the appropriate information.

HP-UX does not currently support SIGWINCH. If \fBrexd\fR receives a
SIGWINCH RPC call from a non HP client after the command has started
executing, the RPC call is effectively ignored since there is no way
to change the window size for the executing command.

The structure used to send the window size from the client to the
server is given below.

.nf

struct ttysize {
	int     ts_lines;       /* number of lines on terminal */
	int     ts_cols;        /* number of columns on terminal */
};

.fi

The REXPROC_WINCH call has no return value.

.sp 1
.H 4 "The REXPROC_SIGNAL Call"
.sp 1

The REXPROC_SIGNAL call is made whenever the \fBon\fR command
receives a SIGINT, SIGTERM or SIGQUIT signal. All other signals are
handled by the \fBon\fR command and not propagated to the remote
command.  The REXPROC_SIGNAL call propagates the signal to the remote
command. This is done by sending the integer value of the signal to
\fBrexd\fR. \fBRexd\fR then does a kill with the signal number on the
process which is the remote command.

The REXPROC_SIGNAL call has no return value.

.sp 1
.H 4 "The REXPROC_WAIT Call"
.sp 1

The REXPROC_WAIT call is used to synchronize the \fBon\fR command and
\fBrexd\fR when the remote command terminates. This is done to
ensure that \fBrexd\fR has completed all cleanup processing associated
with the remote command execution before the \fBon\fR command exits.

The cleanup process includes the following:

.AL
.LI
Make sure the child process has really terminated. If not, kill it.
.LI
If the command was interactive, log the user out and deallocate the 
pty.
.LI
If \fBrexd\fR mounted the user's current working file system, attempt
to unmount it.
.LE

The REXPROC_WAIT call has no input parameters but it does have a
return value. The return value structure is rex_result (the same as
REXPROC_START's return value structure) and is used to indicate any
errors that occurred during the cleanup processing.  It contains an
integer status code and a message string.

If the cleanup was successful then the integer value is 0 and the
message is a NULL string.  If an error occurred during the processing
of the call then the integer value contains a non 0 value and string
which provides information about the error which occurred. The
\fBon\fR command passes this string on to the user and exits.

.nf

struct rex_result {
  /*
   * Structure returned from the start function
   */
   	int	rlt_stat;	/* integer status code */
	char	*rlt_message;	/* string message for human consumption */
};

.fi

.sp 1
.H 3 "The i/o switch"
.sp 1

For interactive \fBon\fR commands \fBrexd\fR must also act as an i/o
switch. That is, the command writes it's stdout and stderr to the
slave side of a pty and \fBrexd\fR reads this output from the master
side of the pty and writes it to the socket(s) which are used to send
the command's stdout and stderr to the corresponding \fBon\fR command.

\fBRexd\fR also reads input data from the socket which the
corresponding \fBon\fR command uses to send stdin to the remote
command. \fBRexd\fR writes this data to the master side of the pty and
the command reads the input from the slave side of the pty. For a
pictorial representation of the i/o switch see the section "The
on/rexd Process and Communication Model".

.sp 1
.H 2 "Software Organization"
.sp 1

The code specific to \fBrexd\fR is organized into four C files, three
include files and one library file. The file names relative to the the
development environment and a short description are given below.

.nf

\fBfile name\fR                         \fBdescription\fR
cmds/usr.etc/rexd/rexd.c          Main program for \fBrexd\fR and 
                                    some functions used by \fBrexd\fR
cmds/usr.etc/rexd/unix_login.c    Functions to make the remote user 
                                    look logged in. Various other 
                                    functions such as pty allocation.
cmds/usr.etc/rexd/mount_nfs.c     Functions to mount the user's current 
                                    working file system.
cmds/usr.etc/rexd/bsdtermio.c     TTY conversion functions.
cmds/usr.etc/rexd/bsdundef.h      Undefine of symbols common to BSD
                                     and System V tty structures.
include/bsdterm.h                 Defines and structures for BSD
                                     terminal structure.
include/rpcsvc/rex.h              Defines and structures for rex.
cmds/usr.lib/librpcsvc/rex_xdr.c  XDR routines for rex.

.fi

.sp 1
.H 1 "The on/rexd Process and Communication Model"
.sp 1

This section provides a pictorial representation of the processes
involved in the remote execution of a command and the communication
which takes place between the processes. The section is broken up into
two parts. The first covers noninteractive \fBon\fR commands and the
second covers interactive \fBon\fR commands.

.bp
.H 2 "Noninteractive Commands"
.sp 1

The process and communication model for noninteractive commands is
given in figure 1 below.

.nf

          client                         server
       ------------                    ------------

 user's stdin, stdout and stderr
 -------------------------------
	   ^ ^ i
	   e o i
           e o V
       ------------                    ------------
       |   on     |  ***************>  |   rexd	  |
       |          |                    |          |
       ------------  <==============   ------------
       	  ^  ^ 	V    	        	     |
          e  o  iiiiiiiiiiiii  		     |
	  e  o        	     i  	     |
	  e  ooooooooooooooo  i  	     |
	  e                 o  i  	     |
	  eeeeeeeeeeeeeeee   o  i  	     |
	                  e   o  i  	     |
			   e   o  i  	     |
			    e   o  i         V
			     e   o  i> ------------
			      e   o oo | command  |
			       eeeeeee |          |
				       ------------

             Key
   -------------------------
   |--->  child process    |
   |***>  RPC Call         |
   |===>  RPC Call Return  |
   |iii>  stdin of command |
   |ooo>  stdout of command|
   |eee>  stderr of command|
   -------------------------

                            \fBFigure 1\fR

.fi

With noninteractive commands the \fBon\fR and \fBrexd\fR processes
communicate using RPC calls to start the remote command and facilitate
execution of the remote command (propagating signals and synchronizing
when the command completes).

At command start up the \fBrexd\fR process \fBfork\fRs a child which in turn
\fBexec\fRs the requested command. Before \fBexec\fRing the command the child
process sets it's stdin, stdout and stderr descriptors to be the
socket(s) which are connected to the \fBon\fR command.

Once the remote command is started the \fBon\fR process acts as an
i/o switch between the invoking user's stdin, stdout and stderr
and the socket connection(s) to the remote command which
are used as the remote command's stdin, stdout and stderr.

.bp
.H 2 "Interactive Commands"
.sp 1

The process and communication model for interactive commands is given
in figure 2 below.

.nf

          client                         server
       ------------                    ------------

 user's stdin, stdout, stderr
 ----------------------------
	   ^ ^ i
	   e o i
           e o V
       ------------                    ------------
       |   on     |  ****************> |   rexd	  |
       |          | <===============   |          |
       |          | iiiiiiiiiiiiiiii>  |          |
       |          | <ooooooooooooooo   |          |
       |          | <eeeeeeeeeeeeeee   |	  |
       ------------                    ------------
       	       	       	       	       	|   ^ ^ i
					|   e o i
					|   e o V
             Key                        |   --------------
   -------------------------            |   | Master Pty |
   |--->  child process    |            |   --------------
   |***>  RPC Call         |            |   | Slave Pty  |
   |===>  RPC Call Return  |            |   --------------
   |iii>  stdin of command |    	|   ^ ^ i
   |ooo>  stdout of command|   	        |   e o i
   |eee>  stderr of command|   	        V   e o	V
   -------------------------           ------------
                                       | command  |
				       |          |
                                       ------------

                              \fBFigure 2\fR

.fi 

With interactive commands the \fBon\fR and \fBrexd\fR processes
communicate using RPC calls to start the remote command and facilitate
execution of the remote command (propagating signals and synchronizing
when the command completes).

Before command start up the \fBrexd\fR process sets up a psuedo
terminal (see \fBpty(7)\fR) to do the terminal specific i/o processing
for the interactive command.

At command start up the \fBrexd\fR process \fBfork\fRs a child which in turn
\fBexec\fRs the requested command. Before \fBexec\fRing the command the child
process sets it's stdin, stdout and stderr to the slave
side of the pty. The \fBrexd\fR process acts as an i/o switch between
the master side of the pty and the socket(s) which the \fBon\fR process
is using to send and receive the remote command's stdin, stdout and
stderr.

.sp 1
.H 1 "BSD vs. System V Terminal Modes"
.sp 1

As mentioned above, the REXPROC_MODES call uses the BSD terminal modes
structure to pass information about the user's terminal modes from the
client to the server. HP uses the System V terminal modes.
Not only is the structure different but BSD
and System V define different terminal modes which do not exist in the
other.

HP's \fBon\fR command must attempt to convert the user's System V
terminal modes into BSD terminal modes. As mentioned before the
mapping is not one to one. In some cases we must attempt to set the
modes as best we can. The \fBrexd\fR must attempt to convert the BSD
modes it receives in the rex_ttymodes structure into System V modes.
Again this mapping is not one to one so we must attempt to set the
modes as best we can. The mappings have been tested and work for the
most used terminal mode settings.

The code specific to BSD to System V terminal mode conversion consists
of a C file and two include files. The file names relative to the the
development environment and a short description are given below.

.nf

\fBfile name\fR                         \fBdescription\fR
cmds/usr.etc/rexd/bsdtermio.c     TTY conversion functions.
cmds/usr.etc/rexd/bsdundef.h      Undefine of symbols common to BSD
                                     and System V tty structures.
include/bsdterm.h                 Defines and structures for BSD
                                     terminal structure.

.fi

.sp 1
.H 1 "Deviations from Sun"
.sp 1

This section contains a list of differences between HP and Sun's
implementation of REX. Most of these are also discussed at the
relevant points within this document.

.AL
.LI
Sun 4.0 \fBrexd\fR tries longer (60 seconds) to unmount the client's 
working file system
if it is busy. Sun 3.2 \fBrexd\fR does not wait at all. HP's \fBrexd\fR tries
for 10 seconds. The corresponding \fBon\fR command can't exit until the 
\fBrexd\fR has completed it's attempts to unmount the file system.
.LI
Sun and the REX protocol use Berkeley TTY modes. HP-UX uses System V 
terminal modes. It may not be possible to translate all possible 
mode settings correctly. Some translations may not be accurate.
.LI
Sun 3.2 \fBrexd\fR servers will lose data. You may or may not see it
(timing problem).
.LI
Sun 3.2 \fBrexd\fR servers will hang if a client doing an interactive
\fBon\fR command sends the job control suspend character.

The command that rexd has started on the remote system will become
suspended thus the \fBon\fR command is hung. The only way to restart
the \fBon\fR 
command is to log into the server and send the command process a
SIGCONT.

Sun 4.0 systems try to suspend the \fBon\fR command and the associated
remote command and allow you to restart the command as you would any
other command from the shell. However, there are timing and other bugs
in the solution and it may hang or function incorrectly.

HP's \fBrexd\fR currently effectively ignores the suspend character. The
parent \fBrexd\fR process receives a SIGCHLD whenever the command
process is suspended or terminates. The \fBrexd\fR process does a wait
and examines the returned structure to see if the child is suspended.
If so, \fBrexd\fR sends the child a SIGCONT to make it continue.
Hence, you can not suspend an interactive \fBon\fR command which is 
running to an HP server.
.LI
Sun 3.2 \fBon\fR command will hang is given both the -n and -i option.
On Sun 4.0 and HP the -i and -n options are mutually exclusive.
.LI
The Sun \fBrexd\fR server will start the process associated with an 
interactive \fBon\fR command and set the terminal size associated with 
the command when it arrives from the client.
Changes in the clients window size are propagated to the server
as they occur.

The HP \fBrexd\fR server will wait until the client sends the terminal size
for the client before starting the command. The terminal size for the
process associated with an interactive \fBon\fR command is set once and only
once. Changes in window size can not be propagated to the server.
Also, if the HP server receives a REXPROC_WINCH call during command
execution it can not change the window size for the executing command.
.LI
Sun's \fBrexd\fR writes all warnings and errors to the console.
HP's \fBrexd\fR does not write any error or warning messages by default.
Use of the -l option (logging) allows error and warning messages to be 
written to a file. Using -l /dev/console will write error and 
warning messages to the console.
.LI
Sun 3.2 \fBrexd\fR servers mount client file systems in a subdirectory of
/tmp. Sun 4.0 \fBrexd\fR servers mount client file systems in a subdirectory
of /tmp_rexd. There is no option to change the location.

HP \fBrexd\fR servers mount client file systems in a subdirectory of
/usr/spool/rexd by default. This can be changed to any directory by 
use of the -m
option.
.LI
Sun only allows user authentication based on matching UIDs. HP allows
the \fBrexd\fR server to be started with the -r option which enforces
stronger user authentication similar but not equivalent to the
authentication used by \fBrlogin(1)\fR and \fBremsh(1)\fR.
.LI 
Sun and HP use different functions for doing pty allocation.
.LI 
Sun and HP use different functions to log the user in and out for 
interactive \fBon\fR commands.
.LE

.TC 1 1 4	\"	create the table of contents, 4 levels
