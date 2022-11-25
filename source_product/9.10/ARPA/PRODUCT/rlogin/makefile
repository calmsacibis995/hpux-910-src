#	@(#)$Header: makefile,v 1.22.109.1 91/11/21 12:04:00 kcs Exp $
##
#	Makefile for remote login and daemon
#

SERVICE	= rlogin

IDIRS=-I../include -I.
CFLAGS	= -DNLS -O
LDFLAGS	= -s

.c.o:
	${CC} ${IDIRS} ${CFLAGS} -c $<

all:	${SERVICE}

${SERVICE}:	${SERVICE}.o
	${CC} ${LDFLAGS} $? -o $@

clean:
	-rm -f *.o a.out core ERRS

clobber:	clean
	-rm -f ${SERVICE}
