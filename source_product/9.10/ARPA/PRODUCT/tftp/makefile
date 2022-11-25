#	@(#)$Header: makefile,v 1.6.109.1 91/11/21 12:13:06 kcs Exp $
##
#	Makefile for trivial file transfer protocol daemon and client.
#

SERVICE	= tftp
TFTP	= main.o tftp.o tftpsubs.o

IDIRS	= -I../include
CFLAGS	= -O
LDFLAGS	= -s ../util/libutil.a

.c.o:
	${CC} ${IDIRS} ${CFLAGS} -c $<

all:	${SERVICE}

${SERVICE}:	${TFTP}
	${CC} ${TFTP} -o $@ ${LDFLAGS}

clean:
	-rm -f *.o a.out core ERRS

clobber:	clean
	-rm -f ${SERVICE}
