#!/bin/sh -
#
# Copyright (c) 1990 Hewlett Packard Corporation
# All rights reserved.
#
# @(#)$Header: newvers.sh,v 1.2.109.3 94/11/08 13:13:45 mike Exp $
#

SRCDIR=${SRCDIR:-"."}
VERSION=`awk '{print $2}' ${SRCDIR}/version`
DATE=`TZ=GMT date`;

echo "char *Version = \"@(#)Revision ${VERSION}  ${DATE}\";" >ver.c
exit 0
