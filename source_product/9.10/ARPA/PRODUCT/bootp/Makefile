/*
 *
 *  $Header: Makefile,v 1.10.109.1 91/11/21 11:47:51 kcs Exp $
 *  New make file for bootpd/bootpquery
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

HDRS=	bootp.h bootpd.h hash.h
BOOTPDSRCS=	bootpd.c readfile.c hash.c
BOOTPDOBJS=	bootpd.o readfile.o hash.o

LDLIBRARIES=	../util/libutil.a

BOOTPDOPT=-DDEBUG -DVEND_CMU -DSYSLOG -DPRIVATE=static

CCFLAGS=	-O $(BOOTPDOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(BOOTPDOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/bootp.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(BOOTPDOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

bootpd :: version.c $(BOOTPDSRCS)

version.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c version.c

bootpquery :: bootpquery.c

bootptab :

all : build

debug : build

bfa : build

build : $(TARGET:/^$/bootpd bootpquery bootptab/)


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(TARGET:/^$/bootpd bootpquery bootptab/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/bootpd bootpquery bootptab/)

clean :
	$(RM) $(RMFLAGS) $(BOOTPDOBJS) bootpquery.o version.o version.c

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/bootpd bootpquery bootptab/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(BOOTPDOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`cat version`
	echo "Version #$VERSION contains the following bootpd sources:\n" >version.log
	for i in $(BOOTPDSRCS) $(HDRS) ; do
		ident $i | 
		grep Header |
		awk 'BEGIN { spaces="                ";\
			     tablen=15-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
