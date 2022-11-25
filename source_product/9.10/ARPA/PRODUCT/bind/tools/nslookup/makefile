#
# $Header: makefile,v 1.1.109.1 91/11/21 11:47:02 kcs Exp $
#
TMPCFLAGS 	= -O -DBSD=43 ${CFLAGS}
LDFLAGS		= -s
INCLUDE		= ../../../include

NSLOOKUP	= bindup.o commands.o debug.o getinfo.o hosttable.o \
		  list.o main.o send.o skip.o subr.o commands.o strcasecmp.o
HDRS		= bsd.h res.h

.c.o:
	${CC} -c ${CFLAGS} ${INCLUDE} $*.c

all: 
	CFLAGS="${TMPCFLAGS}" $(MAKE) nslookup

nslookup: ${NSLOOKUP} ${HDRS}
	${CC} -o $@ ${LDFLAGS} ${NSLOOKUP} ../../res/libresolv.a  -ll

clean: .FORCE
	rm -f *.o *.b core commands.c

clobber: clean
	-rm -f nslookup 

.FORCE:
	@rm -f .FORCE

