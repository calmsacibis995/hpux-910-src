#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of cd implemented using the
# posix shell built-in cd command.

cd $@
exit $?
