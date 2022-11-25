#!/bin/sh
# @(#) $Revision $
# instlang:  install language.
#	     This script is invoked by localedef(1m).
#
# Usage: instlang y langid langname language territory codeset [locale_name]
#    or: instlang n language territory codeset [locale_name]
#
# Install "locale.inf" file in the correct directory under /usr/lib/nls, 
#   unless locale_name is specified, in which case, intall it under that
#   the path specified by locale_name (this is a POSIX requirement).
# Update /usr/lib/nls/config if the first argument is 'y'.


# read and assign arguments.

flag=$1
if [ "$flag" = "y" ]			# update the config file
then
	id=$2
	lang=$3
	dir1=$4
	dir2=$5
	dir3=$6
	install_path=$7
	if [ "$install_path" = "" ]
	then
		install_path="/usr/lib/nls"
		nls_dir="y"
	else
		nls_dir="n"
	fi
elif [ "$flag" = "n" ]			# don't update the config file
then
	dir1=$2
	dir2=$3
	dir3=$4
	nls_dir="y"
	install_path="/usr/lib/nls"
else
	echo "ERROR(instlang): invalid usage" 
	exit 4
fi


# verify the existence of "locale.inf" file and nstall_path directory.

if [ ! -f locale.inf ]
then
	echo "ERROR(instlang): file locale.inf does not exist"
	exit 2
fi

if [ "$nls_dir" = "y" ]
then
	dirname="/usr/lib/nls"
else
	dirname=`dirname $install_path`
fi

if [ ! -d $dirname ]
then
	echo "ERROR(instlang): directory $dirname does not exist"
	exit 4
fi

# make directories (language/territory/codeset) if installing under /usr/lib/nls.

pathname=$install_path

if [ "$nls_dir" = "y" ]
then
   if [ "$dir1" != "" ]
   then
	pathname=$pathname/$dir1
	if [ ! -d $pathname ]
	then
		mkdir $pathname
	fi
   fi

   if [ "$dir2" != "" ]
   then
	pathname=$pathname/$dir2
	if [ ! -d $pathname ]
	then
		mkdir $pathname
	fi
   fi

   if [ "$dir3" != "" ]
   then
	pathname=$pathname/$dir3
	if [ ! -d $pathname ]
	then
		mkdir $pathname
	fi
   fi
fi


# move "locale.inf" file to the correct directory.

if [ "$nls_dir" = "y" ]
then
   if [ -f $pathname/locale.inf ]
   then
	echo "Warning(instlang): file $pathname/locale.inf exists"
	echo "Overwrite the current $pathname/locale.inf file? (y/n) \c"
	read response
	if [ "$response" = "y" -o "$response" = "yes" ]
	then
		mv locale.inf $pathname/locale.inf
	else
		echo "$pathname/locale.inf not updated"
		exit 1
	fi
   else
	mv locale.inf $pathname/locale.inf
   fi
else
   if [ -f $install_path ]
   then
	echo "Warning(instlang): file $install_path exists"
	echo "Overwrite the current $install_path file? (y/n) \c"
	read response
	if [ "$response" = "y" -o "$response" = "yes" ]
	then
		mv locale.inf $install_path
	else
		echo "$install_path not updated"
		exit 1
	fi
   else
	mv locale.inf $install_path
   fi
fi

# update /usr/lib/nls/config file, if installing under /usr/lib/nls.

if [ "$flag" = "y" -a "$install_path" = "/usr/lib/nls" ]
then
	echo "$id $lang" >> /usr/lib/nls/config
fi

exit 0
