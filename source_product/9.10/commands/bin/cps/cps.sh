#! /bin/sh
# @(#) $Revision: 56.1 $
#	cps.sh - cluster wide version of ps(1).

argv=$*
goremote=0

# Parse the arguments.
# If information about the current tty is all that is requested,
# then don't execute remote ps.

set -- `getopt edaflc:s:n:t:p:u:g: $* 2> /dev/null`
if [ $? = 0 ]
then
	for i in $*
	do
	   case $i in
		-e) goremote=1;;
		-d) goremote=1;;
		-a) goremote=1;;
		-t) goremote=1;;
		-p) goremote=1;;
		-u) goremote=1;;
		-g) goremote=1;;
		--) break;;
	   esac
	done
fi

# Print process information for local node.

echo "`cnodes -m`:" ; echo
ps $argv

# Execute remote ps for each node in the cluster.

if [ $goremote -eq 1 ]
then
	echo ; echo
	for i in `cnodes -x`
	do
		echo $i:
		remsh $i ps $argv
		echo ; echo
	done
fi
