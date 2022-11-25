#!/bin/sh
# @(#) $Revision: 66.1 $     
: endnote - move footnotes to separate file

if test $# -eq 0
then
	echo "Usage: endnote textfile ...
	moves all footnotes to separate file of endnotes
	sends text, then endnotes, to standard output"
	exit
fi
if test -f endnotes
then
	echo "endnotes exist - mv or rm before proceeding"
	exit
fi

awk -f /usr/lib/ms/end.awk $*
cat endnotes
rm -f endnotes
