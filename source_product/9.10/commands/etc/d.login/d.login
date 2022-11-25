
# @(#) $Revision: 64.2.1.1 $     


# Default user .login file ( /bin/csh initialization )

# For 9.x 10.x interoperability determine which major revision
# of the operating system we are running on.

set rev=`uname -r | cut -d. -f2`
set rev=`expr $rev + 0`


# Set up the default search paths (search paths are OS revision dependent):
if ( $rev >= 10 ) then
        # OS revisions 10.0 and beyond

        set path=( /usr/bin /usr/ccs/bin /usr/contrib/bin /usr/local/bin . )
else

	# OS revisions prior to 10.0
	set path=(/bin /usr/bin /usr/contrib/bin /usr/local/bin .)
endif

#set up the terminal
eval `tset -s -Q -m ':?hp' `
stty erase "^H" kill "^U" intr "^C" eof "^D" susp "^Z" hupcl ixon ixoff tostop
tabs	

# Set up shell environment:
set noclobber
set history=20
