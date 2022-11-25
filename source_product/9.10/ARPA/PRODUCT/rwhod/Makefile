/*
 *
 *  $Header: Makefile,v 1.5.109.1 91/11/21 12:05:18 kcs Exp $
 *  New make file for rwhod
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	rwhod.c
OBJS=	rwhod.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DPSTAT $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DPSTAT $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/rwhod.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DPSTAT $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

rwhod :: $(SRCS)

all : build

build : $(TARGET:/^$/rwhod/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/rwhod/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/rwhod/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/rwhod/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DPSTAT *.[ch]
