/*
 *
 *  $Header: Makefile,v 1.5.109.1 91/11/21 11:49:17 kcs Exp $
 *  New make file for ftp
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

SRCS=	main.c vers.c cmds.c cmdtab.c ftp.c glob.c ruserpass.c domacro.c
OBJS=	main.o vers.o cmds.o cmdtab.o ftp.o glob.o ruserpass.o domacro.o
HDRS=	ftp_var.h pathnames.h

LDLIBRARIES=	../util/libutil.a

CCFLAGS=	-O $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif /* Debug */

#ifdef BFA
BFADB=		/bfa/ARPA/databases/ftp.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif

ACFLAGS=	$(CCFLAGS)
ALFLAGS=	$(LDFLAGS)
ALIBS=		$(LDLIBRARIES) ../libresolv.a 

ftp :: $(SRCS)

vers.o :
	/bin/sh $(SRCDIR:/^$/./)/newvers.sh
	$(CC) $(CCFLAGS) -c vers.c

all : build

build : $(TARGET:/^$/ftp/)

debug : build

alpha : .FOREGROUND
	$(MAKE) -e -f $(MAKEFILE) $(-) build \
		CCFLAGS="$(ACFLAGS)" LDFLAGS="$(ALFLAGS)" \
		LDLIBRARIES="$(ALIBS)"

bfa : build

install:
	$(INSTALL) $(TARGET:/^$/ftp/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ftp/)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET:/^$/ftp/)
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
			     tablen=15-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version.log
