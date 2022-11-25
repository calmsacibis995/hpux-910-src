/*
 *
 *  $Header: Makefile,v 1.5.109.1 91/11/21 12:02:45 kcs Exp $
 *  New make file for rcp
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	rcp.c
OBJS=	rcp.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef TRUX
CCFLAGS=	$(CCFLAGS) -DTRUX -DSecureWare -DB1 -UAUDIT
#endif /* TRUX */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/rcp.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

rcp :: $(SRCS)

all : build

build : $(TARGET:/^$/rcp/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/rcp/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/rcp/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/rcp/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include *.[ch]
