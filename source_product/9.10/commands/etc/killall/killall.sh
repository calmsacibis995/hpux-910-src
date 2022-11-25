#!/bin/ksh
#
# @(#) $Revision: 72.3 $
#
# Script to kill all processes except special system processes and 
# those associated with the controlling terminal of the user who
# invokes the script.

# Usage:  killall [-signumber]

#
# first_pass() --
#    get list of processes to kill during the first pass.
#
# The following processes are excluded:
#
#   o) Anything attached to our tty
#      (Note that tty info will be lost if the this script is invoked
#       via cron or remsh.)
#   o) killall
#      (this is to avoid the killing of the parent of killall when
#       the tty info is not available.)
#   o) lcsp
#   o) gcsp
#   o) netisr
#   o) mirrorlog
#   o) portmap
#   o) sockregd
#   o) netlogstart
#   o) nfsd
#   o) biod
#   o) rfadaemon
#   o) syslogd
#   o) unhashdaemon
#   o) syncdaemon
#   o) Anything ending in "rc"
#   o) shutdown
#   o) All processes with a pid <= 4
#   o) This script
#   o) HP audit daemon
#   o) ACL, MAC, and Audit daemons
#
function first_pass
{
    mypid=${mypid:-$$}

    tty=${tty:-`/bin/tty`}
    tty=${tty##*/}

# If "FSID" is present in the header for ps -ef, then the Fair Share
# Scheduler is on.  This means there is an extra column (FSID) between the UID
# and the PID.  $fsspattern will hold the modifications needed for this 
# sed script to work on a system with the Fair Share Scheduler.

    fss=`/bin/ps -ef | /bin/sed -n -e '1p' | /bin/grep FSID`

    if [ "$fss" ] 
    then
	fsspattern="[^ ]\{1,8\}[ ]*"
    fi

    pids=`/bin/ps -ef | /bin/sed \
	-e '1d'						\
	-e '/^.*[: ][0-9]\{1,2\}[ ]*'"$tty"'[ ]*[0-9]\{1,\}:/d'\
	-e '/^[ ]*root .* .*killall/d'			\
	-e '/^[ ]*root .* .*lcsp/d'			\
	-e '/^[ ]*root .* .*gcsp/d'			\
	-e '/^[ ]*root .* .*netisr/d'			\
	-e '/^[ ]*root .* .*mirrorlog/d'		\
	-e '/^[ ]*root .* .*portmap/d'			\
	-e '/^[ ]*root .* .*sockregd/d'			\
	-e '/^[ ]*root .* .*netlogstart/d'		\
	-e '/^[ ]*root .* .*nfsd/d'			\
	-e '/^[ ]*root .* .*biod/d'			\
	-e '/^[ ]*root .* .*rfadaemon/d'		\
	-e '/^[ ]*root .* .*syslogd/d'			\
	-e '/^[ ]*root .* .*unhashdaemon/d'		\
	-e '/^[ ]*root .* .*syncdaemon/d'		\
	-e "/^[ ]*root .* .*rc$/d"			\
	-e '/^[ ]*root .* .*shutdown/d'			\
	-e '/^[ ]*root .* .*audomon/d'			\
	-e "/^ *tcb .* .* \/tcb\/bin\/acld$/d"		\
	-e "/^ *tcb .* .* \/tcb\/bin\/macd$/d"		\
	-e "/^ *audit .* .* \/tcb\/bin\/auditd$/d"	\
	-e 's/^[ ]*[^ ]\{1,8\}[ ]*'"$fsspattern"'\([0-9]\{1,5\}\).*/\1/'`

    for pid in $pids; do
	[ "$pid" -ge 5 -a "$pid" -ne "$mypid" -a "$pid" -ne $$ ] && \
	    echo $pid
    done
}

#
# second_pass() --
#    get list of processes to kill during the second pass.
#
# The following processes are excluded:
#
#   o) Anything attached to our tty
#   o) killall
#   o) lcsp
#   o) gcsp
#   o) netisr
#   o) mirrorlog
#   o) sockregd
#   o) unhashdaemon
#   o) syncdaemon
#   o) Anything ending in "rc"
#   o) shutdown
#   o) All processes with a pid <= 4
#   o) This script
#   o) HP audit daemon
#   o) ACL, MAC, and Audit daemons
#
function second_pass
{
    mypid=${mypid:-$$}
    tty=${tty:-`/bin/tty`}

    tty=${tty##*/}

# If "FSID" is present in the header for ps -ef, then the Fair Share
# Scheduler is on.  This means there is an extra column (FSID) between the UID
# and the PID.  $fsspattern will hold the modifications needed for this 
# sed script to work on a system with the Fair Share Scheduler.

    fss=`/bin/ps -ef | /bin/sed -n -e '1p' | /bin/grep FSID`

    if [ "$fss" ] 
    then
	fsspattern="[^ ]\{1,8\}[ ]*"
    fi

    pids=`/bin/ps -ef | /bin/sed \
	-e '1d'						\
	-e '/^.*[: ][0-9]\{1,2\}[ ]*'"$tty"'[ ]*[0-9]\{1,\}:/d'\
	-e '/^[ ]*root .* .*killall/d'			\
	-e '/^[ ]*root .* .*lcsp/d'			\
	-e '/^[ ]*root .* .*gcsp/d'			\
	-e '/^[ ]*root .* .*netisr/d'			\
	-e '/^[ ]*root .* .*mirrorlog/d'		\
	-e '/^[ ]*root .* .*sockregd/d'			\
	-e '/^[ ]*root .* .*unhashdaemon/d'		\
	-e '/^[ ]*root .* .*syncdaemon/d'		\
	-e "/^[ ]*root .* .*rc$/d"			\
	-e '/^[ ]*root .* .*shutdown/d'			\
	-e '/^[ ]*root .* .*audomon/d'			\
	-e "/^ *tcb .* .* \/tcb\/bin\/acld$/d"		\
	-e "/^ *tcb .* .* \/tcb\/bin\/macd$/d"		\
	-e "/^ *audit .* .* \/tcb\/bin\/auditd$/d"	\
	-e 's/^[ ]*[^ ]\{1,8\}[ ]*'"$fsspattern"'\([0-9]\{1,5\}\).*/\1/'`

    for pid in $pids; do
	[ "$pid" -ge 5 -a "$pid" -ne "$mypid" -a "$pid" -ne $$ ] && \
	    echo $pid
    done
}

set +u

if [ "`/bin/pwd`" != / ]; then
    echo "killall: you must be in the root directory to use killall" >&2
    exit 2
fi

#
# Get the signal number specified [if any, defaults to -9]
#
signo="${1#-}"
signo=${signo:-9}

if [ $# -gt 1 -o $signo -lt 0 -o $signo -gt 31 ]; then
    echo "Usage: killall [ -signo ]" >&2
    exit 2
fi

export mypid=$$

#
# Get the terminal name
#
tty=`/bin/tty`
tty=${tty##*/}

#
# Determine the state, standalone, localroot or remoteroot.
# We currently only use this for deciding if we should shutdown the
# lp system.
#
state=standalone
if [ -x /bin/getcontext ]; then
    case "`/bin/getcontext`" in
    *localroot*)
	state=localroot
	;;
    *remoteroot*)
	state=remoteroot
	;;
    *)
	state=standalone
	;;
    esac
fi

#
# First try to shutdown the lp spooler in a nice way.
# Don't do this on a client in a diskless system
#
[ "$state" != "remoteroot" ] && /usr/lib/lpshut >/dev/null 2>&1

#
# Since the first pass will kill the YP daemon /etc/ypbind, we must
# set the domainname to "".  Failure to do this can cause processes
# to hang while they try to connect to the YP daemon.
#
# First check to see if /etc/ypbind is running.  If not, don't execute
# /bin/domainname, otherwise we will get an error if NFS is not configured
# into the kernel.
#

if [ -n "`/bin/ps -ef | /bin/sed -n -e '/^[ ]*root .* .*\/etc\/ypbind/p'`" ]
then
	[ -x /bin/domainname ] && /bin/domainname ""
fi

#
# If no signal was specified, use signal 15 to let things die
# gracefully before we send the real signal.
# We sleep for a while too, to give everyone a chance to die.
#
if [ $# -eq 0 ]; then
    kill -15 `first_pass` >/dev/null 2>&1
    sleep 3
fi

#
# Now kill with the real signal.  We warn with the "second pass"
# processes with a SIGTERM before actually killing them with SIGKILL.
# We sleep for a while too, to give everyone a chance to die.
#
kill -$signo `first_pass` >/dev/null 2>&1
sleep 3
if [ $signo -eq 9 ]; then
    kill -15 `second_pass` >/dev/null 2>&1
    sleep 2
fi
kill -$signo `second_pass` >/dev/null 2>&1

exit 0
