/*
 *
 *  $Header: Makefile,v 1.2.109.2 94/12/15 14:57:06 mike Exp $
 *  New make file for adjtime (s700 only)
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

ADJSRCS=	adjtime.c adjtimed.c

ALL=		libadjtime.a adjtimed

ADJOBJS=	adjtime.o adjtimed.o

LDLIBRARIES=	../lib/libntp.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/bootp.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPLIBOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

all : build

libadjtime.a :: $(ALL)

debug : build

bfa : build

build : $(TARGET:/^$/libadjtime.a/) adjtimed

libadjtime.a: adjtime.o
	ar rv libadjtime.a adjtime.o


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	# Nothing to install since we are using archive libs.
	# $(INSTALL) $(TARGET:/^$/libntp.a/)

rinstall: install
	# Nothing to install since we are using archive libs.
	# $(RINSTALL) $(TARGET:/^$/libntp.a/)

clean :
	$(RM) $(RMFLAGS) $(NTPLIBOBJS)


clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/libntp.a/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPLIBOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]
