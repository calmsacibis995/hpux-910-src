#!/bin/sh
# @(#) $Revision: 66.1 $    
# lpr script -- which makes lp look like lpr

options=
files=
remove=false

while [ $# -gt 0 ]
do
	case $1 in
		-c)
			options="$options $1";;
		-d*)
			options="$options $1";;
		-m)
			options="$options $1";;
		-n*)
			options="$options $1";;
		-o*)
			options="$options $1";;
		-r)
			remove=true;;
		-s)
			options="$options $1";;
		-t*)
			options="$options $1";;
		-w)
			options="$options $1";;
		-*)
			echo "$0: option $1 not recognized"
			exit 1;;
		*)
			files="$files $1";;
	esac
	shift
done

cat $files | lp -s $options

if [ $remove = true ]
then
	rm -f $files
fi
