
# @(#) $Revision: 66.1.1.1 $      

# Default user .profile file (/bin/sh initialization).

# Set up the terminal:
	eval ` tset -s -Q -m ':?hp' `
	stty erase "^H" kill "^U" intr "^C" eof "^D"
	stty hupcl ixon ixoff
	tabs

# Set up the search paths:
	PATH=$PATH:.

# Set up the shell environment:
	set -u
	trap "echo 'logout'" 0

# Set up the shell variables:
	EDITOR=vi
	export EDITOR

# For 9.x 10.x interoperability determine which major revision
# of the operating system we are running on and only do OS revision 
# dependent operations on the appropriate OS.
#
#        rev=`uname -r | cut -d. -f2`
#        rev=`expr $rev + 0`
#
#        # OS revision dependent operations
#        if [ $rev -ge 10 ]
#        then
#               # OS revisions 10.0 and beyond
#		# put OS revision dependent code here
#        else
#               # OS revisions prior to 10.0
#		# put OS revision dependent code here
#        fi
