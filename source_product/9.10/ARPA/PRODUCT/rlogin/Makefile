/*
 *
 *  $Header: Makefile,v 1.5.109.1 91/11/21 12:03:54 kcs Exp $
 *  New make file for rlogin
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	rlogin.c
OBJS=	rlogin.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DHP_NFS -DNLS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DHP_NFS -DNLS $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef TRUX
CCFLAGS=	$(CCFLAGS) -DHP_NFS -DNLS -DTRUX -DSecureWare -DB1 -UAUDIT
#endif /* TRUX */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/rlogin.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DHP_NFS -DNLS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

rlogin :: $(SRCS)

all : build

build : $(TARGET:/^$/rlogin/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/rlogin/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/rlogin/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/rlogin/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DHP_NFS -DNLS *.[ch]
