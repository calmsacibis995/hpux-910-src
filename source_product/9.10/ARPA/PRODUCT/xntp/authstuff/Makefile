/*
 *
 * $Header: Makefile,v 1.2.109.9 95/01/24 16:54:17 mike Exp $
 *  New make file for authspeed
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS= authcert.c authspeed.c keyparity.c makePC1.c makePC2.c makeSP.c \
      makeIPFP.c md5driver.c mkrandkeys.c omakeIPFP.c unixcert.c ver.c \
      newvers.sh
HDRS=

AUTHOBJ=	authspeed.o ver.c ver.o
AUTHCOBJ=	authcert.o ver.o
MD5OBJ=		md5driver.o ver.o
AUTHINS=	/usr/contrib/bin
AUTHCINS=	/usr/contrib/bin
MD5INS=		/usr/contrib/bin
MD5KINS=	/usr/contrib/etc
AUTHSRC=	authspeed.c version.c newvers.sh
AUTHCSRC=	authcert.c version.c newvers.sh
MD5SRC=		md5driver.c version.c newvers.sh

INSOPTSC=	-u bin -g sys
INSOPTSN=

LDLIBRARIES=	../lib/libntp.a -lBSD

AUTHOPT=	-DREFCLOCK -DNO_SIGNED_CHAR_DECL -DDEBUG -DHPUXDEBUG \
		-DHPUX=9 -DNOKMEM -DHAVE_UNISTD_H 

CCFLAGS=	-O $(AUTHOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(AUTHOPT) $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif

#ifdef BFA
BFADB=		/bfa/ARPA/databases/authspeed.B
CC=		bfacc
CCFLAGS=	-DBFA -B-d$(BFADB) $(AUTHOPT) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ver.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c ver.c

authspeed :: $(AUTHOBJ) ver.o

authcert :: $(AUTHCOBJ) ver.o

md5 :: $(MD5OBJ) ver.o

all : build

debug : build

bfa : build

build : $(TARGET:/^$/authspeed/) $(TARGET:/^$/authcert) $(TARGET:/^$/md5)


alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

install:
	$(INSTALL) $(INSOPTSN) -n $(AUTHINS) $(TARGET:/^$/authspeed/)
	# Do not install authcert. Used only by builder to prove correctness.
	# $(INSTALL) $(TARGET:/^$/authcert/)
	$(INSTALL) $(INSOPTSC) -c $(MD5KINS) $(SRCDIR:/^$/ntp.keys/)
	$(INSTALL) $(INSOPTSN) -n $(MD5INS) $(SRCDIR:/^$/md5/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/authspeed/)
	# Do not install authcert. Used only by builder to prove correctness.
	# $(RINSTALL) $(TARGET:/^$/authcert/)
	$(RINSTALL) $(SRCDIR:/^$/ntp.keys/)
	$(RINSTALL) $(SRCDIR:/^$/md5/)

clean :
	$(RM) $(RMFLAGS) $(AUTHOBJ) $(AUTHCOBJ) $(MD5OBJ)
	
clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/authspeed/)
	$(RM) $(RMFLAGS) $(TARGET:/^$/authcert)
	$(RM) $(RMFLAGS) $(TARGET:/^$/md5)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p $(AUTHOPT) -I$(SRCDIR:/^$/./) \
		-I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following authstuff sources:\n" >version.log
	for i in $(SRCS) $(HDRS) ; do
		if [ "$i" = "ver.c" ] ; then
			continue;
		fi
		ident $i | 
		grep Header |
		awk 'BEGIN { spaces="      ";\
			     tablen=5-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
