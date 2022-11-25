/*
 *
 *  $Header: Makefile,v 1.6.109.1 91/11/21 12:12:53 kcs Exp $
 *  New make file for tftp
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	main.c tftp.c tftpsubs.c
OBJS=	main.o tftp.o tftpsubs.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/tftp.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

tftp :: $(SRCS)

all : build

build : $(TARGET:/^$/tftp/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/tftp/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/tftp/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/tftp/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include *.[ch]
