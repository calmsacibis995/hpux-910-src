#!/bin/sh -
#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)newvers.sh	4.6 (Berkeley) 5/11/89
#

PATH=/bin:/sbin:/usr/sbin:/usr/bin
export PATH
SRCDIR=${SRCDIR:-"."}

rm -f version.[oc]
u=${USER-root} d=`pwd` h=`hostname` t=`TZ=GMT date`
sed -e "s|%WHEN%|${t}|" -e "s|%WHOANDWHERE%|${u}@${h}:${d}|" \
	< ${SRCDIR}/Version.c > version.c
