#	@(#)$Header: makefile,v 1.1.109.1 91/11/21 12:13:32 kcs Exp $
##
#	Makefile for trivial file transfer protocol daemon and client.
#

DAEMON	= tftpd
TFTPD	= tftpd.o tftpsubs.o

IDIRS	= -I../include
CFLAGS	= -DSYSLOG -O
LDFLAGS	= -s ../util/libutil.a

.c.o:
	${CC} ${IDIRS} ${CFLAGS} -c $<

all:	${DAEMON}

${DAEMON}:	${TFTPD}
	${CC} ${TFTPD} -o $@ ${LDFLAGS}

clean:
	-rm -f *.o a.out core ERRS

clobber:	clean
	-rm -f ${DAEMON}

