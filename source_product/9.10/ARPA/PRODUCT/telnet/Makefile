/*
 *
 *  $Header: Makefile,v 1.6.109.2 94/11/15 15:32:51 mike Exp $
 *  New make file for telnet
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

HDRS=	defines.h externs.h fdset.h general.h \
	ring.h types.h

SRCS=   main.c vers.c commands.c network.c ring.c \
        sys_bsd.c telnet.c terminal.c \
        tn3270.c utilities.c

OBJS=   main.o vers.o commands.o network.o ring.o sys_bsd.o \
        telnet.o terminal.o tn3270.o utilities.o 

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/telnet.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

telnet :: $(SRCS)

vers.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c vers.c

all : build

build : $(TARGET:/^$/telnet/)

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/telnet/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/telnet/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/telnet/)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR:/^$/./)
	ccount -p -I$(SRCDIR:/^$/./) -I$(SRCDIR:/^$/./)/../include *.[ch]

versionlog:
	rm -f version.log
	VERSION=`awk '{print $2}' version`
	echo "Revision $VERSION contains the following telnet sources:\n" >version.log
	for i in $(SRCS) $(HDRS) ; do
		if [ "$i" = "vers.c" ] ; then
			continue;
		fi
		ident $i | 
		grep Header |
		awk 'BEGIN { spaces="                ";\
			     tablen=5-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
