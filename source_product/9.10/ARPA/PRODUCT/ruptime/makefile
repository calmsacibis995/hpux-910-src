#	@(#)$Header: makefile,v 1.1.109.1 91/11/21 12:04:43 kcs Exp $

SERVICE	= ruptime

IDIRS=-I../include -I.
CFLAGS	= -O
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
