#!/bin/sh -
#
# Copyright (c) 1990 Hewlett Packard Corporation
# All rights reserved.
#
# @(#)$Header: newvers.sh,v 1.1.109.1 91/11/21 11:50:05 kcs Exp $
#

SRCDIR=${SRCDIR:-"."}
VERSION=`awk '{print $2}' ${SRCDIR}/version`
DATE=`TZ=GMT date`;

echo "char ftp_version[] = \"@(#)Revision ${VERSION}  ${DATE}\";" >vers.c
exit 0
