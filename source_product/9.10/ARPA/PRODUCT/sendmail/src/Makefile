/*
 *
 *  $Header: Makefile,v 1.8.109.10 95/02/21 16:06:29 mike Exp $
 *  Nmake Makefile for sendmail/src
 *
 */
#ifdef PATCH_STRING
# static char *patch_3997="@(#) PATCH_9.03: Makefile $Revision: 1.8.109.10 $ 94/03/24 PHNE_3997";
#endif

.SOURCE :   $(SRCDIR|".")

.SOURCE.h : $(SRCDIR|".") $(SRCDIR|".")/../include $(SRCDIR|".")/../../include /usr/include

SRCS=	conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c codes.c \
	sysexits.c util.c arpadate.c version.c collect.c remote.c mimefy.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c domain.c \
	sem.c convtime.c conf.h mailstats.h sendmail.h pathnames.h getpwentNS.c

OBJS=	conf.o deliver.o main.o parseaddr.o err.o alias.o savemail.o codes.o \
	sysexits.o util.o arpadate.o version.o collect.o remote.o mimefy.o \
	macro.o headers.o readcf.o stab.o recipient.o stats.o daemon.o \
	usersmtp.o srvrsmtp.o queue.o clock.o trace.o envelope.o domain.o \
	sem.o convtime.o getpwentNS.o version.o

LDLIBRARIES=	../../util/libutil.a -lndbm

PRODUCTS=	usr/lib/sendmail etc/smtpd  usr/bin/newaliases  usr/bin/mailq

#ifdef TRUX
LDLIBRARIES=	../../util/libutil.a -lndbm -lsec
#endif /* TRUX */

CCFLAGS=	-O -DDEBUG -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)

#ifdef Debug
CCFLAGS=	-g -DDEBUG -DHP_NFS $(NEWCCFLAGS)
LDFLAGS=	$(NEWLDFLAGS)
#endif Debug

#ifdef BFA
BFADB=		/bfa/ARPA/databases/sendmail.B
CC=		bfacc
CCFLAGS=	-B-d$(BFADB) -DDEBUG $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* BFA */

#ifdef TRUX
CCFLAGS=	-O -DSecureWare -DB1 -DTRUX -DDEBUG $(NEWCCFLAGS)
LDFLAGS=	-s $(NEWLDFLAGS)
#endif /* TRUX */

sendmail :: $(SRCS)

sendmail.NEW : sendmail
	mv sendmail sendmail.NEW

all : build

build : $(TARGET|"sendmail")

links:
	/bin/rm -f mailq
	ln -s /usr/lib/sendmail mailq
	/bin/rm -f newaliases
	ln -s /usr/lib/sendmail newaliases
	/bin/rm -f smtpd
	ln -s /usr/lib/sendmail smtpd

debug : build

alpha : build

bfa : build

install:
	$(INSTALL) $(TARGET|"sendmail")

rinstall: install
	$(RINSTALL) $(TARGET|"sendmail")

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET|"sendmail")
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

metrics : ccount

ccount :
	cd $(SRCDIR|".")
	ccount -p -I$(SRCDIR|".") -I$(SRCDIR|".")/../include -DDEBUG *.[ch]

versionlog : smversion
	rm -f version.log
	ln version version.log

smversion :
	rm -f version
	VERSION=`ident version.c | fgrep Header | awk '{print $3}'`
	echo "Version $VERSION contains the following versions from src:\n" >version
	for i in $(SRCS) ; do
		ident $i | 
		fgrep Header |
		awk 'BEGIN { spaces="                ";\
			     tablen=5-length(FILE)+1;\
			     tab=substr(spaces,1,tablen);\
			    }\
		     {print FILE tab $3}' FILE=$i -
	done | sort | pr -2 -t >>version
