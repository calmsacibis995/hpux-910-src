#ifdef hpux
#ifdef V4FS
  #!/usr/bin/sh -
#else
  #!/bin/sh -
#endif /* V4FS */
#endif /* hpux */
#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)newvers.sh	4.4 (Berkeley) 3/28/88
#
SRCDIR=${SRCDIR:-"."}
if [ ! -f ${SRCDIR}/version ] ; then
	echo "Cannot find version!" >&2
	exit 1
fi
rm -f ver.[oc]
v=`cat ${SRCDIR}/version` u=${USER-root} d=`pwd` h=`hostname` t=`TZ=GMT date`
sed "s/%WHOANDWHERE%/${t}/"  ${SRCDIR}/version.c >ver.c
