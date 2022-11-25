#
#	$Header: makefile,v 1.2.109.2 91/12/30 11:41:44 byrond Exp $
#

SRCS=	rlogind.c
OBJS=	rlogind.o

INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -DAUDIT -O
LDFLAGS=	${LIBS}

all:  rlogind

rlogind:  ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f rlogind
