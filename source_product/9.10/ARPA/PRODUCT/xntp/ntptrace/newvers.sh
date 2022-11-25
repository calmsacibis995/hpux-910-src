# $Source: /source/hpux_source/networking/rcs/arpa90_800/xntp/ntptrace/RCS/newvers.sh,v $
# $Revision: 1.2.109.1 $	$Author: root $
# $State: Exp $   	$Locker:  $
# $Date: 94/12/16 16:37:58 $

#!/bin/sh -
#
# Copyright (c) 1990 Hewlett Packard Corporation
# All rights reserved.
#
# @(#)$Header: newvers.sh,v 1.2.109.1 94/12/16 16:37:58 root Exp $
#

SRCDIR=${SRCDIR:-"."}
VERSION=`awk '{print $2}' ${SRCDIR}/version`
DATE=`TZ=GMT date`;

echo "char *Version = \"@(#)Revision ${VERSION}  ${DATE}\";" >ver.c
exit 0
