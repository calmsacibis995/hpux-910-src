#
# $Header: makefile,v 1.1.109.3 94/11/17 10:58:01 mike Exp $
#

SRCS= config.c inetd.c internal.c sem.c signal.c 
OBJS= config.o inetd.o internal.o sem.o signal.o vers.o

INCLUDES=	-I. -I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -O -DAUDIT -DSETPROCTITLE
LDFLAGS=	-s ${LIBS}

all:  inetd

inetd: ${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

vers.o: 
	sh newvers.sh
	${CC} ${CFLAGS} -c vers.c

clean:
	rm -f ${OBJS} core inetd

clobber: clean

