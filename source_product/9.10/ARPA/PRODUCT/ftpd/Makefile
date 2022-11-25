/*
 *
 *  $Header: Makefile,v 1.9.109.2 92/04/06 18:00:48 byrond Exp $
 *  New make file for ftpd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	ftpd.c vers.c ftpcmd.c glob.c popen.c 
OBJS=	ftpd.o vers.o ftpcmd.o glob.o popen.o 

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O -DNEW_LOGGING -DAUDIT -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DNEW_LOGGING -DAUDIT -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ftpd.B
CC=		bfacc
CCFLAGS=	-B-i\ -d$(BFADB) -DNEW_LOGGING -DAUDIT -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

#ifdef TRUX
.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include $(SRCDIR:/^$/./)/../audit /usr/include

LDLIBRARIES=	 -lsec ../util/libutil.a -lBSD 
CCFLAGS=	-O -DNEW_LOGGING -DAUDIT -DSETPROCTITLE $(NEWCCFLAGS) \
		-DTRUX -DSecureWare -DB1 -UAUDIT -DNO_AUDIT
LDFLAGS=	-v -s -Wl,-a,archive $(NEWLDFLAGS)
#endif /* TRUX */

all:  ftpd

ACFLAGS=	-O -DNEW_LOGGING
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ftpd :: $(SRCS)

vers.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c vers.c

ftpcmd.c : $(SRCDIR:/^$/./)/ftpcmd.y
	$(YACC) $(>)
	$(MV) y.tab.c $(>:BS=.c)

all : build

build : $(TARGET:/^$/ftpd/)

debug : build

alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)" ALPHA="1"

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/ftpd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ftpd/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ftpd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DNEW_LOGGING -DAUDIT -DSETPROCTITLE *.[chy]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following ftpd sources:\n" >version.log
	for i in $(SRCS) ftpcmd.y ; do
		if [ "$i" = "vers.c" ] ; then
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
