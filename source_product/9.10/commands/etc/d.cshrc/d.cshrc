#
# Default user .cshrc file (/bin/csh initialization).

# Usage:  Copy this file to a user's home directory and edit it to
# customize it to taste.  It is run by csh each time it starts up.

# For 9.x 10.x interoperability determine which major revision 
# of the operating system we are running on.

	set rev=`uname -r | cut -d. -f2`
	set rev=`expr $rev + 0`

# Set up default command search path:
#
# (For security, this default is a minimal set.)

	# OS revision dependent operations
	if ( $rev >= 10 ) then
		# OS revisions 10.0 and beyond

		set path=( /usr/bin /usr/ccs/bin )

	else
		# OS revisions prior to 10.0

		set path=( /bin /usr/bin )
	endif

# Set up C shell environment:

	if ( $?prompt ) then		# shell is interactive.
	    set history=20		# previous commands to remember.
	    set savehist=20		# number to save across sessions.
	    set system=`hostname`	# name of this system.
	    set prompt = "$system \!: "	# command prompt.

	    # Sample alias:

	    alias	h	history		

	    # More sample aliases, commented out by default:

	    # alias	d	dirs
	    # alias	pd	pushd
	    # alias	pd2	pushd +2
	    # alias	po	popd
	    # alias	m	more
	endif
