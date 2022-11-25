#!/bin/sh -
#
#	$Header: newvers.sh,v 1.1.109.1 91/11/21 11:47:06 kcs Exp $
#

PATH=/bin:/sbin:/usr/sbin:/usr/bin
export PATH
SRCDIR=${SRCDIR:-"."}

rm -f version.[oc]
u=${USER-root} d=`pwd` h=`hostname` t=`TZ=GMT date`
sed -e "s|%WHEN%|${t}|" -e "s|%WHOANDWHERE%|${u}@${h}:${d}|" \
	< ${SRCDIR}/Version.c > version.c
