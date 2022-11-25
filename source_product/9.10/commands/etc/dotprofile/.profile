# @(#) $Revision: 66.1 $    

# Default (example of) super-user's .profile file (/bin/sh initialization).

Set_columns_and_lines()
{
	   echo "Is your console one of the following: a 2392A, 2393A, 2397A or 700/92? [y/n]: \c"
	   read reply
	   if [ "$reply" = "y" ]   
	   then		
		LINES=24
		COLUMNS=80
		TERM=hp
		export LINES COLUMNS TERM
	   else
		default_columns=80
		default_lines=24
		columns=$default_columns
		lines=$default_lines
		ok="no"
		attempts=1
		while [ "$ok" = "no" ]
		do
 		    echo "A:      10        20        30        40        50        60        70        80        90       100       110       120       130       140       150       160       170"
 		    echo "B: 45678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
		    echo "Using lines A and B above, count the number of digits that print on a single"
		    echo "line. Enter the number of digits here. (default: \"$columns\"): \c"
		    read columns
		    if [ "$columns" != "" ]
		    then
	    	        columns=`expr "$columns" : '\([0-9]*\)'`
		        if [ "$columns" = "" ]
		        then
			  if [ $attempts -ge 2 ]
			  then
 		    	     echo "\nUnrecognized input...\c"
			  else
			     echo "\nUnrecognized input...try again"
			     columns=$default_columns    #set to go round again
			  fi
			  attempts=`expr $attempts + 1`
			else
			  ok="yes"
			fi
		    fi
		    if [ "$columns" = "" ]
		    then
			echo "using default: $default_columns"
		        columns=$default_columns
			ok="yes"
		    fi
		  done
		  ok="no"
		  attempts=1
		  while [ "$ok" = "no" ]
		  do
		    echo "60\n59\n58\n57\n56\n55\n54\n53\n52\n51"
		    echo "50\n49\n48\n47\n46\n45\n44\n43\n42\n41"
		    echo "40\n39\n38\n37\n36\n35\n34\n33\n32\n31"
		    echo "30\n29\n28\n27\n26\n25\n24\n23\n22\n21"
		    echo "20\n19\n18\n17\n16\n15\n14\n13\n12\n11"
		    echo "10\n9\n8\n7\n6\n5\n4\n3"
		    echo "2    What is the largest number in the"
		    echo "1    upper left corner? (default \"$default_lines\"): \c"
		    read lines

		    if [ "$lines" != "" ]
		    then
	    	        lines=`expr "$lines" : '\([0-9]*\)'`
		        if [ "$lines" = "" ]
		        then
			  if [ $attempts -ge 2 ]
			  then
		    	     echo "\nUnrecognized input...\c"
			  else
			     echo "\nUnrecognized input...try again"
			     lines=$default_lines  # set to go round again
			  fi
			  attempts=`expr $attempts + 1`
			else
			  ok="yes"
			fi
		    fi
		    if [ "$lines" = "" ]
		    then
			echo "using default: $default_lines"
		        lines=$default_lines
			ok="yes"
		    fi
		done
		echo "\nCOLUMNS set to \"$columns\" and LINES set to \"$lines\""
		COLUMNS=$columns
		LINES=$lines
		export COLUMNS LINES
	   fi
}

set_columns_and_lines()
{
	   ask="no"
	   echo "Is your terminal or display an HP product [y/n]: \c"
	   read reply
	   reply="n"  # until problems with 2621, 2622, 2392, others? are solved
	   if [ "$reply" = "y" ]   
	   then				#attempt to auto-set COLUMNS and LINES
		echo "If terminal/display hangs or if keyboard locks up, then press reset and return." 
        	echo '\033&a999c999Y\033`\c' # lower-right corner, cursor sense
        	read x				# read current cursor position
        	y=`echo $x | sed -e 's/^.*a//'`  # strip off preceding chars
        	#COLUMNS=`expr "$x" : '.*a\(.*\)[cx]' + 1`	# extract width
        	COLUMNS=`expr "$y" : '\(.*\)[cx]' + 1`		# extract width
        	LINES=`expr "$x" : '.*[cx]\(.*\)Y' + 1`		# extract height
		if [ "$COLUMNS" != "" -a "$LINES" != "" ]
		then
        	   export COLUMNS LINES
        	   echo "Screen size is ${COLUMNS}x${LINES}"
		else
		   ask="yes"
		fi
	   else
		ask="yes"
	   fi
	   if [ "$ask" = "yes" ]
	   then
		default_columns=80
		default_lines=24
		columns=$default_columns
		lines=$default_lines
		ok="no"
		attempts=1
		while [ "$ok" = "no" ]
		do
 		    echo "A:      10        20        30        40        50        60        70        80        90       100       110       120       130       140       150       160       170"
 		    echo "B: 45678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
		    echo "Using lines A and B above, count the number of digits that print on a single"
		    echo "line. Enter the number of digits here. (default: \"$columns\"): \c"
		    read columns
		    if [ "$columns" != "" ]
		    then
	    	        columns=`expr "$columns" : '\([0-9]*\)'`
		        if [ "$columns" = "" ]
		        then
			  if [ $attempts -ge 2 ]
			  then
 		    	     echo "\nUnrecognized input...\c"
			  else
			     echo "\nUnrecognized input...try again"
			     columns=$default_columns    #set to go round again
			  fi
			  attempts=`expr $attempts + 1`
			else
			  ok="yes"
			fi
		    fi
		    if [ "$columns" = "" ]
		    then
			echo "using default: $default_columns"
		        columns=$default_columns
			ok="yes"
		    fi
		  done
		  ok="no"
		  attempts=1
		  while [ "$ok" = "no" ]
		  do
#		    lines=60
#		    while [ $lines -ge 3 ]  
#		    do
#			echo $lines  # faster approach is to type them as below
#			lines=`expr $lines - 1`
#		    done
		    echo "60\n59\n58\n57\n56\n55\n54\n53\n52\n51"
		    echo "50\n49\n48\n47\n46\n45\n44\n43\n42\n41"
		    echo "40\n39\n38\n37\n36\n35\n34\n33\n32\n31"
		    echo "30\n29\n28\n27\n26\n25\n24\n23\n22\n21"
		    echo "20\n19\n18\n17\n16\n15\n14\n13\n12\n11"
		    echo "10\n9\n8\n7\n6\n5\n4\n3"
		    echo "2    What is the largest number in the"
		    echo "1    upper left corner? (default \"$default_lines\"): \c"
		    read lines

		    if [ "$lines" != "" ]
		    then
	    	        lines=`expr "$lines" : '\([0-9]*\)'`
		        if [ "$lines" = "" ]
		        then
			  if [ $attempts -ge 2 ]
			  then
		    	     echo "\nUnrecognized input...\c"
			  else
			     echo "\nUnrecognized input...try again"
			     lines=$default_lines  # set to go round again
			  fi
			  attempts=`expr $attempts + 1`
			else
			  ok="yes"
			fi
		    fi
		    if [ "$lines" = "" ]
		    then
			echo "using default: $default_lines"
		        lines=$default_lines
			ok="yes"
		    fi
		done
		echo "\nCOLUMNS set to \"$columns\" and LINES set to \"$lines\""
		COLUMNS=$columns
		LINES=$lines
		export COLUMNS LINES

	        how_to_avoid  # print "how to avoid" message
	   fi
}

how_to_avoid()
{
    echo "\nTo bypass these questions:"
    echo "1)  Be sure the command \"/usr/bin/tset\" is available on your system."
    echo "2)  Edit the file \".profile\" in root's home directory.  In the first line of "
    echo "    the file place a \"tset\" command, with the appropriate parameters to set"
    echo "    the TERM variable.  "
    echo "\nFor more information on using \"tset\" and setting TERM, see the section on "
    echo "\"How Terminal Characteristics Are Defined\" in either Chapter 9 of the AXE"
    echo "User's Manual, or Chapter 4 of A Beginner's Guide to Using Shells.\n"
}

# Do not put "." in PATH; it is a potential security breach.
# Do not put "/usr/local/bin" in PATH; it is a potential security breach.
# Example assumes /users/root exists.

	PATH=/bin:/usr/bin:/etc:/usr/contrib/bin:/users/root:/usr/lib:/usr/lib/acct


# if TERM is hp then see if tset can do any better if available
ask="no"
if [ "$TERM" = "hp" -o "$TERM" = "" -o "$TERM" = "unknown" ]
then
  TERM=hp
  if [ -x /usr/bin/tset ]
  then
	eval `tset -s -Q -h`			# look in /etc/ttytype
	if [ "$TERM" = "" -o "$TERM" = "unknown" ]
	then
  	   TERM=hp
	   ask="yes"
	else
	   ask="yes"	# change to "no" if sure of value in /etc/ttytype
#	   echo "TERM value from /etc/ttytype: \"$TERM\""   # and uncomment
	fi
  else	
 	echo "The command \"/usr/bin/tset\" was not found."
	ask="yes"
  fi
  if [ "$ask" = "yes" ]
  then
	   # set_columns_and_lines
	   Set_columns_and_lines
	   #how_to_avoid
  fi
fi

echo
echo "Value of TERM has been set to \"$TERM\". "
export TERM 

# Set up the terminal

        stty erase "^H" kill "^U" intr "^C" eof "^D"
	if [ -x /usr/bin/tabs ]
	then
	    tabs
	else
 	    echo "The command \"/usr/bin/tabs\" was not found."
	fi

# Set up shell environment:

	set -u					# error if undefined variable.
	trap "echo 'logout root'" 0  		# what to do on exit.


# Set up shell variables:

	MAIL=/usr/mail/root
	# don't export, so only login shell checks.

	EDITOR=vi
	export EDITOR

	echo "WARNING:  YOU ARE SUPERUSER !!\n"
