#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of unalias implemented using the
# posix shell built-in unalias command.

unalias $@
exit $?
