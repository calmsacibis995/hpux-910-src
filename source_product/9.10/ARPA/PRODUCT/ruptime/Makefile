/*
 *
 *  $Header: Makefile,v 1.4.109.1 91/11/21 12:04:37 kcs Exp $
 *  New make file for ruptime
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	ruptime.c
OBJS=	ruptime.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ruptime.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

ruptime :: $(SRCS)

all : build

build : $(TARGET:/^$/ruptime/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/ruptime/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ruptime/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ruptime/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include *.[ch]
