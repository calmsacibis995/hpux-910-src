/*
 *
 *  $Header: Makefile,v 1.7.109.2 92/03/19 17:10:52 seshadri Exp $
 *  New make file for telnetd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	telnetd.c search.c
OBJS=	telnetd.o search.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DAUDIT $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DAUDIT $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef TRUX
CCFLAGS=	$(CCFLAGS) -DTRUX -DSecureWare -DB1 -UAUDIT -UOLD_LOGIN
#endif /* TRUX */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/telnetd.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DAUDIT $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

telnetd :: $(SRCS)

all : build

build : $(TARGET:/^$/telnetd/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/telnetd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/telnetd/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/telnetd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DAUDIT *.[ch]
