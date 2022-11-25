
#	@(#)$Header: makefile,v 1.17.109.2 94/08/30 14:00:21 mike Exp $
#
#  Makefile for sendmail/aux directory
#
#       Assorted programs related {perhaps distantly} to Sendmail.
#

SHELL=   /bin/sh
CMDS=	 mailstats praliases

CFLAGS	= -O
LDFLAGS	= -s
IDIRS	= -I../src -I../include -I../../include
LIBS	= ../../util/libutil.a -lndbm

.c.o:
	${CC} ${IDIRS} ${CFLAGS} -c $<
	
all: ${CMDS}

praliases: praliases.o
	${CC} -o $@ ${LDFLAGS} $@.o ${LIBS}

mailstats: mailstats.o
	${CC} -o $@ ${LDFLAGS} $@.o ${LIBS}

clean:
	rm -f *.o core a.out make.out lint.out

clobber: clean
	rm -f ${CMDS}
