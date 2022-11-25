#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of fg implemented using the
# posix shell built-in fg command.

fg $@
exit $?
