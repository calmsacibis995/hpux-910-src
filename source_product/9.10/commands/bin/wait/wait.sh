#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of wait utility implemented 
# using posix shell built-in wait command.

wait $@
exit $?

