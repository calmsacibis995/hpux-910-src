#! /bin/sh
# @(#) $Revision: 66.4 $

NAME=$0
print_usage () {
	echo "Usage: ${NAME} [-ku] files [-] [[-ku] files]"
}

while [ $# -gt 0 ] 
do 
	set -- `getopt uk- $* `
	if [ $? != 0 ]; then
		print_usage
		exit 2	
	fi

	# Grab the arguments 
	for i in $*
	do
		case $i in 
 		-k | -u) FLAGS="$FLAGS $i"; shift;;
		--)	shift; break;;
		esac
	done

	# Grab the files
	for i in $*
	do 
		case $i in 
		-)	shift; break;;
		*)	FILES="$FILES $i"; shift;;
		esac
	done

	for i in `cnodes -x`
	do
		echo $i:
		for file in $FILES
		do
			if [ `dirname $file` = "." ]; then
				file=`pwd`/$file
			fi
			remsh $i "/etc/fuser $FLAGS $file > /tmp/poo$$ 2>&1"
			remsh $i cat /tmp/poo$$
			remsh $i rm /tmp/poo$$
		done
	done

	echo "`cnodes -m`:"
	/etc/fuser $FLAGS $FILES
	FLAGS=""
	FILES=""

done


