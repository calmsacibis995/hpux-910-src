/*
 *
 * $Header: Makefile,v 1.2.109.8 94/12/16 16:49:53 mike Exp $ 
 *  New make file for ntptrace
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

NTPTRACEOBJS=	ntptrace.o ver.o
NTPTRACESRCS=	ntptrace.c newvers.sh
SRCS=		$(NTPTRACESRCS)
HDRS=		ntptrace.h
NTINS=		/usr/contrib/bin

LDLIBRARIES=    ../lib/libntp.a ../adjtime/libadjtime.a -lBSD

NTPTRACEOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
	-DHPUX=9 -DNOKMEM -DCONFIG_FILE=/usr/contrib/etc/xntp.conf \
	-DHAVE_UNISTD_H 
INSOPTSN=

CCFLAGS=	-O $(NTPTRACEOPT) $(NEWCCFLAGS)
LDFLAGS=	-s -lBSD $(NEWLDFLAGS)


#ifdef Debug
CCFLAGS=	-g $(NTPTRACEOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ntptrace.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPTRACEOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ntptrace :: $(NTPTRACEOBJS)

all : build

debug : build

bfa : build

build : $(TARGET:/^$/ntptrace/)

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -n $(NTINS) $(TARGET:/^$/ntptrace/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ntptrace/)

clean :
	$(RM) $(RMFLAGS) $(NTPTRACEOBJS)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ntptrace/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPTRACEOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following ntptrace sources:\n" >version.log
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
