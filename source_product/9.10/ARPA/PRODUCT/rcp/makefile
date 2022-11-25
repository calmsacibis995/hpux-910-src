#
#	@(#)$Header: makefile,v 1.2.109.1 91/11/21 12:02:49 kcs Exp $
#


SRCS=	rcp.c
OBJS=	rcp.o

INCLUDES=	-I../include
LIBS=		../util/libutil.a

CFLAGS=		-O ${INCLUDES}
LDFLAGS=	-s ${LIBS}

all: rcp

rcp:	${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f rcp

