/*
 *
 *  $Header: Makefile,v 1.3.109.1 91/11/21 11:49:00 kcs Exp $
 *  New make file for fingerd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	fingerd.c
OBJS=	fingerd.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DOPTIONS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DOPTIONS $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/fingerd.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DOPTIONS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

fingerd :: $(SRCS)

all : build

build : $(TARGET:/^$/fingerd/)

debug : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/fingerd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/fingerd/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/fingerd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
