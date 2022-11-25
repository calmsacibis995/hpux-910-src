#	@(#)$Header: makefile,v 1.21.109.1 91/11/21 12:03:36 kcs Exp $
##
#	Makefile for remote shell and daemon
#

DAEMON	= remshd rexecd

CFLAGS	= -DAUDIT -DHP_NFS -O
LDFLAGS	= -s

.c.o:
	${CC} -I../include ${IDIRS} ${CFLAGS} -c $<

all:	${DAEMON}

remshd.o:	remshd.c
	${CC} -I../include ${IDIRS} ${CFLAGS} -c -DREMSHD remshd.c

remshd:		remshd.o
	${CC} ${LDFLAGS} remshd.o -o remshd

rexecd.o:	remshd.c
	${CC} -I../include ${IDIRS} ${CFLAGS} -c -DREXECD remshd.c -o rexecd.o

rexecd:		rexecd.o
	${CC} ${LDFLAGS} rexecd.o -o rexecd

clean:
	-rm -f *.o a.out core ERRS

clobber:	clean
	-rm -f ${DAEMON}
