#! /bin/sh 
# @(#) $Revision: 51.4 $
#	calendar.sh - calendar command, uses /usr/lib/calprog

PATH=/bin:/usr/bin
_1=/tmp/$$.1
_2=/tmp/$$.2
trap "rm -f ${_1} ${_2}; trap '' 0; exit" 0 1 2 13 15
case $# in
0)  if [ -f calendar ]; then
	/usr/lib/calprog > ${_1}
	LANG="" egrep -f ${_1} calendar
    else
	echo $0: `pwd`/calendar not found >&2
    fi;;
*)  pwget | sed 's/\([^:]*\):.*:\(.*\):[^:]*$/_dir=\2 _user=\1/' |
    while read _token; do
	eval ${_token}			# evaluates _dir= and _user=
	if [ -s ${_dir}/calendar ]; then
		/usr/lib/calprog ${_dir}/calendar > ${_1}
		LANG="" egrep -f ${_1} ${_dir}/calendar > ${_2} 2> /dev/null
	    if [ -s ${_2} ]; then
		mail ${_user} < ${_2}
	    fi
	    /bin/rm -f ${_2}
	fi
    done;;
esac
/bin/rm -f ${_1}
exit 0
