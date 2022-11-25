#
#	$Header: makefile,v 1.1.109.1 92/04/02 15:12:06 gomathi Exp $
#

SRCS=	ocd.c dfa_account.c
OBJS=	ocd.o dfa_account.o

INCLUDES=	-I. -I../include
LIBS=		-s ../../seshadri/util/libutil.a

CFLAGS=		-O ${INCLUDES} 
# DFLAGS=		${CFLAGS} -DDEBUG
DFLAGS=		${CFLAGS} -DMYTELNET -DDEBUG
LDFLAGS=	${LIBS}

all:  ocd

ocd:  ${SRCS}
	${CC} -o $@ ${CFLAGS} ${SRCS} ${LDFLAGS} -DMYTELNET

ocdebug:  ${SRCS}
	${CC} -o $@ ${DFLAGS} ${SRCS} ${LDFLAGS}

clean:
	rm -f ${OBJS} core

clobber: clean
	rm -f ocd
	rm -f ocdebug
