#!/bin/sh
# @(#) $Revision: 66.2 $      
# Shell script to post-process cat-able manual pages after catman.
# This script reduces the amount of storage space needed.

# Usage: <script>  (arguments are ignored)

# Removes backspaces from files, but does not touch blank lines,
# on the assumption that man(1) will call rmnl(1) to take care of this.

# Run as super-user to ensure success.


# Initialize:

	PATH=/bin:/usr/bin:/usr/local/bin

#  If MANPATH is set, it will govern our path search
	if [ "$MANPATH" ]
	then
	    dirs=`/usr/lib/parseman $MANPATH`
	else
		dirs="/usr/man /usr/local/man /usr/contrib/man"
	fi
	
	temp=/tmp/fm$$
	trap "rm -f $temp; trap '' 0; exit" 0 1 2 3 15


# Convert all (char, backspace) pairs to nothing in each file:
# Note that there is a control-H after the backslash in the sed script.

	for dir in $dirs
	do
		cd $dir
		for catdir in `ls -d cat* | grep -v .Z`
		do
			for file in `find $catdir -type f -print`
			do
				sed < $file 's/.\//g' | unexpand -a > $temp
				mv -f $temp $file
				chmod 444 $file
			done
		done
		for catdir in cat*.Z
		do
			for file in `find $catdir -type f -print`
			do
				uncompress < $file | sed 's/.\//g' | unexpand -a | compress > $temp
				mv -f $temp $file
				chmod 444 $file
			done
		done
	done
