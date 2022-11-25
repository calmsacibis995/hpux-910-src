#!/bin/posix/sh
# @(#) $Revision: 72.1 $

# This is the execable version of the type utility implemented
# using the posix shell built-in 'type' command.

type $@
exit $?
