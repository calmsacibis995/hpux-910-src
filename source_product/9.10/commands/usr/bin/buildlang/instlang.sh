#!/bin/sh
# @(#) $Revision $
# instlang:  install language.
#	     This script is invoked by buildlang(1m).
#
# Usage: instlang y langid langname language territory codeset
#    or: instlang n language territory codeset
#
# Install "locale.def" file in the correct directory under /usr/lib/nls.
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
elif [ "$flag" = "n" ]			# don't update the config file
then
	dir1=$2
	dir2=$3
	dir3=$4
else
	echo "ERROR(instlang): invalid usage" 
fi


# verify the existence of "locale.def" file and "/usr/lib/nls" directory.

if [ ! -f locale.def ]
then
	echo "ERROR(instlang): file locale.def does not exist"
	exit 2
fi

if [ ! -d /usr/lib/nls ]
then
	echo "ERROR(instlang): directory /usr/lib/nls does not exist"
	exit 2
fi


# make directories (language/territory/codeset) under /usr/lib/nls.

pathname=/usr/lib/nls

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


# move "locale.def" file to the correct directory.

if [ -f $pathname/locale.def ]
then
	echo "Warning(instlang): file $pathname/locale.def exists"
	echo "Overwrite the current $pathname/locale.def file? (y/n) \c"
	read response
	if [ "$response" = "y" -o "$response" = "yes" ]
	then
		mv locale.def $pathname/locale.def
	else
		echo "$pathname/locale.def not updated"
		exit 1
	fi
else
	mv locale.def $pathname/locale.def
fi


# update /usr/lib/nls/config file

if [ "$flag" = "y" ]
then
	echo "$id $lang" >> /usr/lib/nls/config
fi

exit 0
