#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of command utility implemented
# using the posix shell built-in 'command' command.

command $@
exit $?
