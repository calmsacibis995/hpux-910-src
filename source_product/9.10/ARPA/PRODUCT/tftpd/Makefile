/*
 *
 *  $Header: Makefile,v 1.4.109.1 91/11/21 12:13:24 kcs Exp $
 *  New make file for tftpd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	tftpd.c tftpsubs.c
OBJS=	tftpd.o tftpsubs.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DSYSLOG $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DSYSLOG $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/tftpd.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DSYSLOG $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

tftpd :: $(SRCS)

all : build

build : $(TARGET:/^$/tftpd/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/tftpd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/tftpd/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/tftpd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DSYSLOG *.[ch]
