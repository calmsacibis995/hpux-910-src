#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of jobs implemented using the
# posix shell built-in jobs command.

jobs $@
exit $?
