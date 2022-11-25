#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of alias implemented using the
# posix shell built-in alias command.

alias $@
exit $?
