#!/bin/sh
:
# @(#) $Revision: 27.1 $    
#
#	ccat.sh	4.1	83/02/11
#
PATH=/bin:/usr/bin:.

prog=`basename $0`

if [ $# = 0 ]
then
	echo "Usage: $prog file  ..."
	exit 1
fi

for file in $*
do
	uncompact < $file
done
