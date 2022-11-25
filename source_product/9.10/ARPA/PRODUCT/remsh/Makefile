/*
 *
 *  $Header: Makefile,v 1.6.109.1 91/11/21 12:03:10 kcs Exp $
 *  New make file for remsh
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	remsh.c
OBJS=	remsh.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef TRUX
CCFLAGS= 	$(CCFLAGS) -DHP_NFS -DTRUX -DSecureWare -DB1 -UAUDIT
#endif /* TRUX */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/remsh.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DAUDIT -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

remsh :: $(SRCS)

rexec : remsh

all : build

build : $(TARGET:/^$/remsh/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/remsh/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/remsh/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/remsh/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DAUDIT -DHP_NFS *.[ch]
