#!/bin/sh
# @(#) $Revision: 66.1 $      
# 'perform disk accounting'
_dir=/usr/adm
_pickup=acct/nite
PATH=/usr/lib/acct:/bin:/usr/bin:/etc:
export PATH
set -- `getopt o $*`
if [ $? -ne 0 ]
then
	echo "Usage: $0 [ -o ] [ filesystem ... ]"
	exit 1
fi
for i in $*; do
	case $i in
	-o)	SLOW=1; shift;;
	--)	shift; break;;
	esac
done

cd ${_dir}
date

if [ "$SLOW" = "" ]
then
	if [ $# -lt 1 ]
	then
#set args to a list of file systems to check (from /etc/checklist)
		args=`awk '$3 ~ /hfs/ {print $1}' /etc/checklist`
	else
		args="$*"
	fi
	diskusg $args > dtmp
else
	if [ $# -lt 1 ]
	then
		args="/"
	else
		args="$*"
	fi
	for i in $args; do
		if [ ! -d $i ]
		then
			echo "$0: $i is not a directory -- ignored"
		else
			dir="$i $dir"
		fi
	done
	if [ "$dir" = "" ]
	then
		echo "$0: No data"
		> dtmp
	else
		find $dir -print | acctdusg > dtmp
	fi
fi

date
sort +0n +1 -o dtmp dtmp
acctdisk <dtmp >disktmp
chmod 644 disktmp
chown adm disktmp
mv disktmp ${_pickup}/disktacct
