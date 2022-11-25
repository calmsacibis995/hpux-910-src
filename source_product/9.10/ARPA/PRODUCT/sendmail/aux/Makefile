/*
 *
 *  $Header: Makefile,v 1.5.109.2 94/08/30 13:37:40 mike Exp $
 *  New make file for sendmail/aux
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../src \
            $(SRCDIR:/^$/./)/../include $(SRCDIR:/^$/./)/../../include \
            /usr/include

LDLIBRARIES=	../../util/libutil.a -lndbm

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/sm-aux.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

all : build

praliases :: praliases.c

mailstats :: mailstats.c

build : $(TARGET:/^$/praliases mailstats/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) praliases mailstats freeze

rinstall: install
	$(RINSTALL) praliases mailstats freeze

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/praliases mailstats/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include *.[ch]
