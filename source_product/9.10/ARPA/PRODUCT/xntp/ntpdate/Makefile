/*
 *
 * $Header: Makefile,v 1.2.109.6 94/11/08 13:09:50 mike Exp $
 *  New make file for ntpdate
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

NTPDATEOBJS=	ntpdate.o ver.o
NTPDATESRCS=	ntpdate.c ver.c
NTINS=		/usr/contrib/bin

LDLIBRARIES=	../lib/libntp.a ../adjtime/libadjtime.a -lBSD

NTPDATEOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
		-DHPUX=9 -DNOKMEM -DHAVE_UNISTD_H 
INSOPTSN=

CCFLAGS=	-O $(NTPDATEOPT) $(NEWCCFLAGS)
LDFLAGS=	-s -lBSD $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NTPDATEOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ntpdate.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(NTPDATEOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ntpdate :: $(NTPDATEOBJS)

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c

all : build

debug : build

bfa : build

build : $(TARGET:/^$/ntpdate/)


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -n $(NTINS) $(TARGET:/^$/ntpdate/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ntpdate/)

clean :
	$(RM) $(RMFLAGS) $(NTPDATEOBJS)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ntpdate/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(NTPDATEOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following telnet sources:\n" >version.log
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
