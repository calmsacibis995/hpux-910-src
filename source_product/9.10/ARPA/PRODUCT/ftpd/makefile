#
# Copyright (c) 1988 Regents of the University of California.
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
#	@(#)Makefile	5.8 (Berkeley) 9/21/88
#
SRCS=	ftpcmd.y ftpd.c glob.c popen.c ftpd_sec.c
OBJS=	ftpcmd.o ftpd.o glob.o popen.o ftpd_sec.o vers.o

INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -O -DNEW_LOGGING -DAUDIT -DSETPROCTITLE
LDFLAGS=	-s ${LIBS}

all:  ftpd

ftpd: ${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

vers.o: ftpd.c ftpcmd.y
	sh newvers.sh
	${CC} ${CFLAGS} -c vers.c

clean:
	rm -f ${OBJS} core ftpd

clobber: clean
