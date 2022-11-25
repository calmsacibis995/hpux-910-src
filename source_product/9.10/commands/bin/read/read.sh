#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of read implemented using the
# posix shell built-in read command.

read $@
exit $?
