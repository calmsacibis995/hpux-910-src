#
#	$Header: makefile,v 1.2.109.2 91/12/30 11:38:08 byrond Exp $
#

SRCS=	commands.c main.c network.c ring.c \
	sys_bsd.c telnet.c terminal.c \
	tn3270.c utilities.c vers.c

ALLHC=	${SRCS} \
	defines.h externs.h fdset.h general.h \
	ring.h types.h

OBJS=	commands.o main.o network.o ring.o sys_bsd.o \
	telnet.o terminal.o tn3270.o utilities.o vers.o

INCLUDES=	-I../include
LIBS=		../util/libutil.a

CFLAGS=		${INCLUDES} -O
LDFLAGS=	-s ${LIBS}

all: telnet

telnet:	${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LDFLAGS}

vers.c:
	sh newvers.sh

clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f telnet
