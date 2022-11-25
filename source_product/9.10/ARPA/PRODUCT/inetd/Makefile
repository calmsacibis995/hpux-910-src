/*
 *
 *  $Header: Makefile,v 1.6.109.4 94/11/17 10:56:18 mike Exp $
 *  New make file for inetd
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	config.c internal.c signal.c inetd.c inetd.h sem.c \
	vers.c
OBJS=	config.o internal.o signal.o inetd.o sem.o vers.o

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O  -DAUDIT -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef TRUX
CCFLAGS=	$(CCFLAGS) -DSETPROCTITLE -DHP_NFS \
		-DTRUX -DSecureWare -DB1 -UAUDIT
#endif /* TRUX */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/inetd.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DSETPROCTITLE $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

inetd :: $(SRCS)

all : build

build : $(TARGET:/^$/inetd/)

vers.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c vers.c

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/inetd/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/inetd/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/inetd/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include \
		-DSETPROCTITLE *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following inetd sources:\n" >version.log
	for i in $(SRCS) ; do
		if [ "$i" = "vers.c" ] ; then
			continue;
		fi
		ident $i | 
		fgrep Header |
		awk 'BEGIN { spaces="                ";\
			     tablen=15-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
