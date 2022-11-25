/*
 *
 * $Header: Makefile,v 1.2.109.6 94/11/08 16:43:55 mike Exp $ 
 *  New make file for ntpq
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

NTPQOBJS=	ntpq.o ntpq_ops.o ver.o
NTPQSRCS=	ntpq.c ntpq_ops.c newvers.sh
SRCS=		$(NTPQSRCS)
HDRS=		ntpq.h
NTINS=		/usr/contrib/bin

LDLIBRARIES=	../lib/libntp.a -lBSD

NTPQOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
	-DHPUX=9 -DNOKMEM -DCONFIG_FILE=/usr/contrib/etc/xntp.conf \
	-DHAVE_UNISTD_H 
INSOPTSN=

CCFLAGS=	-O $(NTPQOPT) $(NEWCCFLAGS)
LDFLAGS=	-s -lBSD $(NEWLDFLAGS)


#ifdef Debug
CCFLAGS=	-g $(NTPQOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ntpq.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPQOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ntpq :: $(NTPQOBJS)

all : build

debug : build

bfa : build

build : $(TARGET:/^$/ntpq/)

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c

alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -n $(NTINS) $(TARGET:/^$/ntpq/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ntpq/)

clean :
	$(RM) $(RMFLAGS) $(NTPQOBJS)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ntpq/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPQOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following ntpq sources:\n" >version.log
	for i in $(SRCS) $(HDRS) ; do
		if [ "$i" = "ver.c" ] ; then
			continue;
		fi
		ident $i | 
		grep Header |
		awk 'BEGIN { spaces="                ";\
			     tablen=15-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
