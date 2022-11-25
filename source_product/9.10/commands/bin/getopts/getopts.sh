#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is an execable version of getopts utility implemented
# using posix shell built-in getopts command

getopts $@
exit $?
