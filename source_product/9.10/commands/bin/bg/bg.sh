#!/bin/posix/sh
# @(#) $Revision: 70.2 $

# This is the execable version of bg implemented using the
# posix shell built-in bg command.

bg $@
exit $?
