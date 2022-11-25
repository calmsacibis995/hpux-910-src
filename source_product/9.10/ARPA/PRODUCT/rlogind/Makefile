/*
 *
 *  $Header: Makefile,v 1.7.109.2 92/04/06 17:56:39 byrond Exp $
 *  New make file for rlogind
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=   rlogind.c 
OBJS=   rlogind.o 

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
BFADB=		/bfa/ARPA/databases/rlogind.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DAUDIT $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

rlogind :: $(SRCS)

all : build

build : $(TARGET:/^$/rlogind/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/rlogind/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/rlogind/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/rlogind/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DAUDIT *.[ch]
