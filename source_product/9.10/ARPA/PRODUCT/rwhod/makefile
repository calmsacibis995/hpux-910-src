#
#	$Header: makefile,v 1.1.109.2 91/12/30 11:35:46 byrond Exp $
#

SRCS=	rwhod.c
OBJS=	rwhod.o

INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		 -O ${INCLUDES} -DPSTAT 
LDFLAGS=	${LIBS}

all:  rwhod

rwhod:  ${OBJS}
	${CC} -s -o  $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f rwhod
