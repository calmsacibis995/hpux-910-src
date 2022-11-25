#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is an execable version of umask utility implemented
# using the posix shell built-in umask command.

umask $@
exit $?

