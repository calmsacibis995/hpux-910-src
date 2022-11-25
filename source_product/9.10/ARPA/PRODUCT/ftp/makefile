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
SRCS=	cmds.c cmdtab.c ftp.c glob.c main.c ruserpass.c \
	domacro.c vers.c
OBJS=	cmds.o cmdtab.o ftp.o glob.o main.o ruserpass.o \
	domacro.o vers.o
INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -O
LDFLAGS=	-s ${LIBS}

all:  ftp

ftp: ${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

vers.c:
	sh newvers.sh

clean:
	rm -f ${OBJS} core 

clobber: clean
	rm -f ftp

