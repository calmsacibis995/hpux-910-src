#
#	$Header: makefile,v 1.2.109.3 92/03/19 17:11:12 seshadri Exp $
#

SRCS=	telnetd.c search.c
OBJS=	telnetd.o search.o

INCLUDES=	-I. -I../include
LIBS=		-s ../util/libutil.a

CFLAGS=		-O ${INCLUDES} -DAUDIT 
LDFLAGS=	${LIBS}

all:  telnetd

telnetd:  ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

search:  ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}
clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f telnetd
