#!/bin/sh

PREFIX=gated.conf.

cd $1
shift 
for i in $*
do
	SUFFIX=`/bin/echo $i | sed -e s/$PREFIX//`
	mv $i $SUFFIX
done
	
