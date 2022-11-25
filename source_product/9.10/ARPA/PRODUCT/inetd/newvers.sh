#!/bin/sh -
#
# Copyright (c) 1990 Hewlett Packard Corporation
# All rights reserved.
#
# @(#)$Header: newvers.sh,v 1.1.109.1 91/11/21 12:01:08 kcs Exp $
#

SRCDIR=${SRCDIR:-"."}
VERSION=`awk '{print $2}' ${SRCDIR}/version`
DATE=`TZ=GMT date`;

echo "char inetd_version[] = \"@(#)Revision ${VERSION}  ${DATE}\";" >vers.c
exit 0
