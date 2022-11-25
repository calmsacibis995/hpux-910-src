/*
 *
 * $Header: Makefile,v 1.2.109.8 94/12/16 16:28:49 mike Exp $
 *  New make file for xntp
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

XNTPOBJS=	ntpdc.o ntpdc_ops.o ver.o
XNTPSRCS=	ntpdc.c ntpdc_ops.c newvers.sh
SRCS=		$(XNTPSRCS)
HDRS=

LDLIBRARIES=	../lib/libntp.a -lBSD

XNTPOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
	-DHPUX=9 -DNOKMEM -DCONFIG_FILE=/usr/contrib/etc/xntp.conf \
	-DHAVE_UNISTD_H 
XNINS=		/usr/contrib/bin
INSOPTSN=

#ifdef Debug
CCFLAGS=	-g $(XNTPOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
# else
CCFLAGS=	-O $(XNTPOPT) $(NEWCCFLAGS)
LDFLAGS=	-s -lBSD $(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/xntpdc.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(XNTPOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

xntpdc :: $(XNTPOBJS)

all : build

debug : build

bfa : build

build : $(TARGET:/^$/xntpdc/)

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -n $(XNINS) $(TARGET:/^$/xntpdc/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/xntpdc/)

clean :
	$(RM) $(RMFLAGS) $(XNTPOBJS)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/xntpdc/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(XNTPOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following xntpdc sources:\n" >version.log
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
